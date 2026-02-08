#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "drivers/angle/mpu6050.h"
#include "drivers/energy/ina219.h"
#include "drivers/i2c/i2c_bus.h"
#include "drivers/lux/bh1750.h"
#include "drivers/network/tcp_client.h"
#include "drivers/display_2.0/ssd1306_i2c.h"
#include "drivers/temperature/ds18b20.h"
#include "drivers/security/crypto.h"

// --- Wi-Fi ---
#define WIFI_SSID     "KAUA_LQ"
#define WIFI_PASS     "12345678"

// --- Watchdog ---
// Bitmask para controle das tasks (Bit 0: Sensor, Bit 1: Comm, Bit 2: Tracking)
static uint8_t tasks_alive = 0;
#define ALL_TASKS_OK  0x07 // 0b111

// --- Intervalos de Tempo ---
#define HEARTBEAT_INTERVAL_SEC   30
#define DATA_SEND_INTERVAL_SEC   600  // 10 minutos

#define BTN_A 5
#define LED_RED 13
bool flag_btn = 0;
bool flag_wf_state = 1;

/* ---------------- Configurações do Tracking ---------------- */
#define ENABLE_SERIAL_MOCK  0   // <-- troque para 0 para sensores reais

#define PIN_STEP  4
#define PIN_DIR   9
#define PIN_ENA   8

#define KP              800.0f     // ganho proporcional (ajuste fino depois)
#define DEADZONE        0.05f      // erro mínimo (~5%)
#define MAX_STEPS_CYCLE 200        // limite por iteração
#define STEP_DELAY_US   800        // velocidade do motor

#define TRACKING_INTERVAL_SEC   600     // 10 minutos
#define LUX_DELTA_THRESHOLD     0.10f   // 10% de variação mínima
#define VB_DELTA_MIN            0.05f   // 50 mV (ajuste depois)
#define RL_LIMIT_DEG            45.0f

// --- Configurações do Homing ---
#define RL_HOME_TOLERANCE    1.0f    // ±1 grau
#define RL_HOME_STEP_SIZE    20      // passos por ajuste fino
#define RL_HOME_DELAY_MS     200

// estrutura para armazenar dados dos sensores da task de sensores
typedef struct {
    float lux[3];
    float angle[2];
    float energy[4];
    float temperature;
} sensor_data_t;

QueueHandle_t sensor_queue; // Fila para comunicar os dados dos sensores entre as tasks

// funções auxiliares
void button_callback(uint gpio, uint32_t events);
bool wifi_is_connected();
bool wifi_reconnect();
void write_oled_values(const sensor_data_t *data);
static void step_motor(uint32_t steps, bool direction);

void sensor_task(void *param) {
    sensor_data_t data;

    // Inicializa sensores I2C (mantendo seu fluxo)
    ina219_init();
    bh1750_initialize();
    mpu6050_init();

    while (true) {
        mux_sweep(data.lux);
        mpu6050_get_values(data.angle);
        ina219_get_values(data.energy);

        data.temperature = ds18b20_rtos_read_temperature((ds18b20_t *)param);

        // Envia snapshot completo (sobrescreve se necessário)
        xQueueOverwrite(sensor_queue, &data);

        // Marca a si mesma como viva
        tasks_alive |= (1 << 0);

        // Se todas as tasks deram check-in, alimenta o hardware
        if (tasks_alive == ALL_TASKS_OK) {
            watchdog_update();
            tasks_alive = 0; // Reseta para a próxima rodada
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz
    }
}

void mock_serial_task(void *param) {
    char buffer[128];
    int idx = 0;

    printf("\n[MOCK] Envie dados no formato:\n");
    printf("lux_left,lux_right,vb_before,vb_after,rl\n");

    while (true) {

        int c = getchar_timeout_us(1000);

        if (c == PICO_ERROR_TIMEOUT) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (c == '\n' || c == '\r') {
            buffer[idx] = '\0';
            idx = 0;

            float lux_left, lux_right, vb_before, vb_after, rl;

            if (sscanf(buffer, "%f,%f,%f,%f,%f",
                       &lux_left, &lux_right,
                       &vb_before, &vb_after,
                       &rl) == 5) {

                sensor_data_t mock = {0};

                /* Lux */
                mock.lux[2] = lux_left;
                mock.lux[0] = lux_right;
                mock.lux[1] = (lux_left + lux_right) / 2.0f;

                /* Energia */
                mock.energy[0] = vb_before;

                /* Ângulo */
                mock.angle[1] = rl;
                mock.angle[0] = 0.0f;

                /* Injeta BEFORE */
                xQueueOverwrite(sensor_queue, &mock);

                printf("[MOCK] BEFORE -> L=%.1f R=%.1f VB=%.2f RL=%.2f\n",
                       lux_left, lux_right, vb_before, rl);

                /* Simula tempo de movimento */
                vTaskDelay(pdMS_TO_TICKS(1200));

                /* Atualiza VB AFTER */
                mock.energy[0] = vb_after;
                xQueueOverwrite(sensor_queue, &mock);

                printf("[MOCK] AFTER  -> VB=%.2f\n", vb_after);
            }
            else {
                printf("[MOCK] Formato invalido\n");
            }
        }
        else if (idx < sizeof(buffer) - 1) {
            buffer[idx++] = (char)c;
        }
    }
}

void tracking_task(void *param) {
    sensor_data_t data;
    float erro;
    uint32_t steps;

    gpio_init(PIN_STEP);
    gpio_init(PIN_DIR);
    gpio_init(PIN_ENA);

    gpio_set_dir(PIN_STEP, GPIO_OUT);
    gpio_set_dir(PIN_DIR, GPIO_OUT);
    gpio_set_dir(PIN_ENA, GPIO_OUT);

    gpio_put(PIN_ENA, 1); // ENA ativo em LOW (mais seguro)

    float last_lux_error = 0.0f;
    float last_vb = 0.0f;
    absolute_time_t last_move_time = get_absolute_time();
    bool led_state = false;
    bool is_aligned = false;
    absolute_time_t last_blink = get_absolute_time();

    while (true) {
        tasks_alive |= (1 << 2); // Avisa que o tracking está rodando
        if (xQueuePeek(sensor_queue, &data, pdMS_TO_TICKS(1000)) != pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        float lux_left  = data.lux[2];
        float lux_right = data.lux[0];
        float vb = data.energy[0];
        float rl = data.angle[1];

        // rotina de alinhamento inicial do painel (homing)
        if (!is_aligned) {
            // Pisca Led para indicar alinhamento em progresso
            if (absolute_time_diff_us(last_blink, get_absolute_time()) >= 250000) {
                led_state = !led_state;
                gpio_put(LED_RED, led_state);
                last_blink = get_absolute_time();
            }

            printf("[HOME] RL atual = %.2f\n", rl);

            if (fabsf(rl) <= RL_HOME_TOLERANCE) {
                printf("[HOME] Alinhado com sucesso\n");
                is_aligned = true;
                gpio_put(LED_RED, 0);
                last_move_time = get_absolute_time(); // evita mover logo após
                continue;
            }

            bool dir = (rl > 0.0f);
            step_motor(RL_HOME_STEP_SIZE, dir);
            vTaskDelay(pdMS_TO_TICKS(RL_HOME_DELAY_MS));
            continue;
        }

        /* Proteção noturna */
        if ((lux_left + lux_right) < 50.0f) {
            printf("[TRACK] Baixa luminosidade (L=%.1f R=%.1f), aguardando\n", lux_left, lux_right);
            for(int i=0; i<5; i++) {
                tasks_alive |= (1 << 2); 
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            continue;
        }

        /* Limite mecânico */
        if (rl < -RL_LIMIT_DEG || rl > RL_LIMIT_DEG) {
            printf("[TRACK] RL fora do limite: %.2f\n", rl);
            for(int i=0; i<5; i++) {
                tasks_alive |= (1 << 2); 
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            continue;
        }

        /* Intervalo mínimo entre movimentos */
        if (absolute_time_diff_us(last_move_time, get_absolute_time()) <
            TRACKING_INTERVAL_SEC * 1000000LL) {
            printf("[TRACK] Aguardando intervalo de 10 min\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        /* Erro normalizado */
        erro = (lux_right - lux_left) / (lux_right + lux_left);

        /* Mudança relevante? */
        if (fabsf(erro - last_lux_error) < LUX_DELTA_THRESHOLD) {

            printf("[TRACK] Lux estável, sem movimento\n");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        /* Deadzone */
        if (fabsf(erro) < DEADZONE) {
            printf("[TRACK] Dentro da deadzone (erro=%.3f), sem movimento\n", erro);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        steps = (uint32_t)(fabsf(erro) * KP);
        if (steps > MAX_STEPS_CYCLE) steps = MAX_STEPS_CYCLE;

        bool dir = (erro > 0);

        printf("[TRACK] MOVENDO | erro=%.3f steps=%lu dir=%s vb=%.2f rl=%.2f\n",
               erro, steps, dir ? "DIR" : "ESQ", vb, rl);

        /* Movimento */
        step_motor(steps, dir);
        tasks_alive |= (1 << 2); // Check-in logo após o motor parar
        vTaskDelay(pdMS_TO_TICKS(1000)); // estabilização

        /* Reavalia VB */
        if (xQueuePeek(sensor_queue, &data, pdMS_TO_TICKS(1000)) == pdTRUE) {
            float vb_new = data.energy[0];

            if ((vb_new - vb) < VB_DELTA_MIN) {
                printf("[TRACK] Movimento inefetivo, revertendo\n");
                step_motor(steps, !dir); // volta
            } else {
                printf("[TRACK] Movimento efetivo (VB %.2f -> %.2f)\n", vb, vb_new);
                last_vb = vb_new;
                last_lux_error = erro;
                last_move_time = get_absolute_time();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void comm_task(void *param) {
    sensor_data_t data;
    char data_json[384];
    char hmac_hex[65];
    char payload[512];

    absolute_time_t last_heartbeat = get_absolute_time();
    absolute_time_t last_data_send = get_absolute_time();

    while (true) {
        tasks_alive |= (1 << 1); // Avisa que a comunicação está rodando
        absolute_time_t now = get_absolute_time();

        // --- Wi-Fi / TCP management ---
        if (!wifi_is_connected()) {
            flag_wf_state = 0;
            write_oled_values(&data); // Mostra o ícone de sem Wi-Fi

            if (!wifi_reconnect()) {
                // Se falhar, aguarda 5 segundos sem travar o processador
                // Isso permite que outras tasks (como a de sensores) continuem rodando
                for(int i=0; i<5; i++) {
                    tasks_alive |= (1 << 1); 
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                continue; 
            }

            // Se reconectou com sucesso
            tcp_client_close();
            tcp_client_start();
            flag_wf_state = 1; 
        }

        // --- Atualização de display (1 Hz) ---
        if (xQueueReceive(sensor_queue, &data, 0) == pdTRUE) {
            write_oled_values(&data);
        }

        // --- Heartbeat (30 s) ---
        if (absolute_time_diff_us(last_heartbeat, now) >=
            HEARTBEAT_INTERVAL_SEC * 1000000LL) {

            tcp_client_send("{\"meta\":{\"type\":\"hb\"}}\n");
            last_heartbeat = now;
        }

        // --- Envio de dados reais (10 min) ---
        if (absolute_time_diff_us(last_data_send, now) >=
            DATA_SEND_INTERVAL_SEC * 1000000LL) {

            snprintf(data_json, sizeof(data_json),
                "{"
                "\"lux1\":%.2f," "\"lux2\":%.2f," "\"lux3\":%.2f,"
                "\"pt\":%.2f," "\"rl\":%.2f,"
                "\"tp\":%.2f," "\"vb\":%.2f," "\"vs\":%.4f," "\"i\":%.4f," "\"p\":%.4f"
                "}",
                data.lux[0], data.lux[1], data.lux[2],
                data.angle[0], data.angle[1],
                data.temperature,
                data.energy[0], data.energy[1], data.energy[2], data.energy[3]
            );

            // HMAC somente do data_json
            hmac_sha256(data_json, hmac_hex);

            // Payload final
            snprintf(payload, sizeof(payload),
                "{"
                "\"meta\":{"
                    "\"type\":\"data\","
                    "\"pend\":%s,"
                    "\"hmac\":\"%s\""
                "},"
                "\"data\":%s"
                "}\n",
                data_was_pending ? "true" : "false",
                hmac_hex,
                data_json
            );

            bool was_connected = tcp_connected_flag;

            tcp_client_send(payload);
            /* Se não estava conectado OU mensagem ficou pendente,
            * então houve perda de dados */
            if (!was_connected || has_pending_msg) {
                data_was_pending = true;
            }
            if (!has_pending_msg && tcp_connected_flag) {
                data_was_pending = false;
            }
            last_data_send = now;
        }

        tcp_client_flush_pending_if_possible();
        cyw43_arch_poll();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ----------------- main -------------------- */
int main() {
    stdio_init_all();
    sleep_ms(3000);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A); // Ativa pull-up interno no botão
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, 0); // Desliga o LED inicialmente

    // inicializa o barramento i2c
    i2c_bus_init();
    i2c_oled_init();
    SSD1306_init(); // inicia o display OLED

    // Wi-Fi init
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    SSD1306_clear();
    SSD1306_draw_string(5, 32, "wifi init...");
    SSD1306_update();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        SSD1306_clear();
        SSD1306_draw_string(5, 32, "falha no wifi");
        SSD1306_update();
        cyw43_arch_deinit();
        return -1;
    }

    SSD1306_clear();
    SSD1306_draw_string(5, 32, "wifi conectado");
    SSD1306_update();

    sleep_ms(1000); // espera estabilizar

    // IP do servidor
    if (!ip4addr_aton(SERVER_IP, &server_addr)) {
        printf("IP inválido: %s\n", SERVER_IP);
        SSD1306_clear();
        SSD1306_draw_string(5, 8, "Server IP");
        SSD1306_draw_string(5, 16, "Desconhecido");
        SSD1306_draw_string(5, 32, SERVER_IP);
        SSD1306_update();
        return -1;
    }

    // Inicializa o sensor de temperatura
    ds18b20_t sensor;
    ds18b20_rtos_init(&sensor, pio0, 17);

    // Configura interrupção para o botão
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // inicia tentativa de conexão TCP
    tcp_client_start();

    SSD1306_clear();
    SSD1306_draw_image(8, 8, 100, 48, icon_embarca_100px48px);
    SSD1306_update();
    sleep_ms(5000);

    sensor_queue = xQueueCreate(1, sizeof(sensor_data_t));
    configASSERT(sensor_queue != NULL);

    #if !ENABLE_SERIAL_MOCK
    xTaskCreate(sensor_task, "SensorTask", 1024, &sensor, 2, NULL);
    #endif
    #if ENABLE_SERIAL_MOCK
    xTaskCreate(mock_serial_task, "MockSerialTask", 2048, NULL, 2, NULL);
    #endif
    xTaskCreate(tracking_task, "TrackingTask", 2048, NULL, 1, NULL);
    xTaskCreate(comm_task, "CommTask", 4096, NULL, 1, NULL);

    if (watchdog_caused_reboot()) {
        printf("Reboot causado pelo Watchdog!\n");
    }
    watchdog_enable(8000, 1);

    vTaskStartScheduler();

    while (true){ /*nada pra fazer aqui, tudo roda em RTOS*/ }

    // nunca chega aqui, mas boa prática
    tcp_client_close();
    cyw43_arch_deinit();
    return 0;
}

//Callback do botão A
void button_callback(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Evita múltiplas detecções rápidas (debouncing)
    if (current_time - last_time > 200) {
        flag_btn = !flag_btn;
    }
}

bool wifi_is_connected() {
    return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
}

bool wifi_reconnect() {
    int err = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASS,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (err) {
        return false;
    }

    flag_wf_state = 1;
    vTaskDelay(pdMS_TO_TICKS(500));

    return true;
}

void write_oled_values(const sensor_data_t *data){
    char lux1_str[16];
    snprintf(lux1_str, sizeof(lux1_str), "l1=%.2f", data->lux[0]);
    char lux2_str[16];
    snprintf(lux2_str, sizeof(lux2_str), "l2=%.2f", data->lux[1]);
    char lux3_str[16];
    snprintf(lux3_str, sizeof(lux3_str), "l3=%.2f", data->lux[2]);

    char pitch_str[16];
    snprintf(pitch_str, sizeof(pitch_str), "pt=%.2f", data->angle[0]);
    char roll_str[16];
    snprintf(roll_str, sizeof(roll_str), "rl=%.2f", data->angle[1]);
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "tp=%.2f", data->temperature);
    char vbus_str[16];
    snprintf(vbus_str, sizeof(vbus_str), "vb=%.2f", data->energy[0]);
    char vshunt_str[16];
    snprintf(vshunt_str, sizeof(vshunt_str), "vs=%.4f", data->energy[1]);
    char current_str[16];
    snprintf(current_str, sizeof(current_str), "i=%.4f", data->energy[2]);
    char power_str[16];
    snprintf(power_str, sizeof(power_str), "p=%.4f", data->energy[3]);

    if (!flag_btn) {
        SSD1306_clear();
        SSD1306_draw_string(5, 12, lux1_str);
        SSD1306_draw_string(5, 20, lux2_str);
        SSD1306_draw_string(5, 28, lux3_str);
        SSD1306_draw_string(5, 36, pitch_str);
        SSD1306_draw_string(5, 44, roll_str);
        SSD1306_draw_string(5, 52, temp_str);
        SSD1306_update();
    }
    else {
        SSD1306_clear();
        SSD1306_draw_string(5, 20, vbus_str);
        SSD1306_draw_string(5, 30, vshunt_str);
        SSD1306_draw_string(5, 38, current_str);
        SSD1306_draw_string(5, 46, power_str);
        SSD1306_update();
    }
    if (flag_wf_state) {
        SSD1306_draw_image(110, 8, 16, 16, icon_wifi_preto);
        if (tcp_connected_flag) {
            SSD1306_draw_image(110, 28, 16, 16, icon_cloud_preto);
            SSD1306_update();
        }
        else {
            SSD1306_draw_image(110, 28, 16, 16, icon_nocloud_preto);
            SSD1306_update();
        }
    }
    else {
        SSD1306_draw_image(110, 8, 16, 16, icon_nowifi_preto);
        SSD1306_draw_image(110, 28, 16, 16, icon_nocloud_preto);
        SSD1306_update();
    }
}

static void step_motor(uint32_t steps, bool direction) {
    gpio_put(PIN_DIR, direction);

    for (uint32_t i = 0; i < steps; i++) {
        gpio_put(PIN_STEP, 1);
        sleep_us(STEP_DELAY_US);
        gpio_put(PIN_STEP, 0);
        sleep_us(STEP_DELAY_US);
    }
}