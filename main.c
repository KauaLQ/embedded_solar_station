#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "mbedtls/md.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "drivers/angle/mpu6050.h"
#include "drivers/energy/ina219.h"
#include "drivers/i2c/i2c_bus.h"
#include "drivers/lux/bh1750.h"
#include "drivers/network/tcp_client.h"
#include "drivers/display_2.0/ssd1306_i2c.h"
#include "drivers/temperature/ds18b20.h"

// --- Wi-Fi ---
#define WIFI_SSID     "KAUA_LQ"
#define WIFI_PASS     "12345678"

#define BTN_A 5
bool flag_btn = 0;
bool flag_wf_state = 1;

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

#define HMAC_SECRET "solar_station_secret_2025"

void hmac_sha256(const char *input, char *output_hex) {
    unsigned char hmac[32];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info;

    info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1);

    mbedtls_md_hmac_starts(&ctx,
        (const unsigned char *)HMAC_SECRET,
        strlen(HMAC_SECRET)
    );

    mbedtls_md_hmac_update(&ctx,
        (const unsigned char *)input,
        strlen(input)
    );

    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    // Converte para hexadecimal
    for (int i = 0; i < 32; i++) {
        sprintf(output_hex + (i * 2), "%02x", hmac[i]);
    }
    output_hex[64] = '\0';
}

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

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz
    }
}

void comm_task(void *param) {
    sensor_data_t data;
    char data_json[384];
    char hmac_hex[65];
    char payload[512];

    while (true) {
        // GERENCIAMENTO DE CONEXÃO
        if (!wifi_is_connected()) {
            flag_wf_state = 0;
            write_oled_values(&data); // Mostra o ícone de sem Wi-Fi

            if (!wifi_reconnect()) {
                // Se falhar, aguarda 5 segundos sem travar o processador
                // Isso permite que outras tasks (como a de sensores) continuem rodando
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue; 
            }

            // Se reconectou com sucesso
            tcp_client_close();
            tcp_client_start();
            flag_wf_state = 1; 
        }

        if (xQueueReceive(sensor_queue, &data, pdMS_TO_TICKS(200)) == pdTRUE) {

            // Atualiza display OLED com os novos dados
            write_oled_values(&data);

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
                    "\"pend\":%s,"
                    "\"hmac\":\"%s\""
                "},"
                "\"data\":%s"
                "}\n",
                has_pending_msg ? "true" : "false",
                hmac_hex,
                data_json
            );

            tcp_client_send(payload);
            tcp_client_flush_pending_if_possible();
        }

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

    xTaskCreate(sensor_task, "SensorTask", 1024, &sensor, 2, NULL);
    xTaskCreate(comm_task, "CommTask", 4096, NULL, 1, NULL);

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
    cyw43_arch_deinit();
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        return false;
    }
    cyw43_arch_enable_sta_mode();

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