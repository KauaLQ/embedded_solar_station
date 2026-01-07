#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

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

// vetores para armazenar leituras dos sensores
float arrayBH1750[3];
float arrayMPU6050[2];
float arrayINA219[5];

// variável do sensor de temperatura
float g_temp = 0.0;

// funções auxiliares
void button_callback(uint gpio, uint32_t events);
bool wifi_is_connected();
bool wifi_reconnect();
void write_oled_values(void);

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

    // Inicializa sensores I2C (mantendo seu fluxo)
    bh1750_initialize();
    mpu6050_init();
    ina219_init();

    // Inicializa o sensor de temperatura
    ds18b20_t sensor;
    ds18b20_init(&sensor, pio0, 17);

    // Configura interrupção para o botão
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // inicia tentativa de conexão TCP
    tcp_client_start();

    SSD1306_clear();
    SSD1306_draw_image(8, 8, 100, 48, icon_embarca_100px48px);
    SSD1306_update();
    sleep_ms(5000);

    while (true) {
        // Verifica conexão Wi-Fi
        if (!wifi_is_connected()) {
            flag_wf_state = 0;
            write_oled_values(); // atualiza o display oled com símbolo de nowifi

            if (!wifi_reconnect()) {
                // Se falhar, espera um pouco e tenta novamente no próximo loop
                sleep_ms(5000);
                continue;
            }

            // Se reconectou, reinicia cliente TCP
            tcp_client_close();
            tcp_client_start();
        }

        // varredura dos sensores I2C
        mux_sweep(arrayBH1750);
        mpu6050_get_values(arrayMPU6050);
        ina219_get_values(arrayINA219);

        // leitura do sensor de temperatura
        g_temp = ds18b20_read_temperature(&sensor);

        write_oled_values();

        // monta payload JSON
        char payload[512];
        snprintf(payload, sizeof(payload),
            "{ \"meta\": { \"pend\": %s }, \"data\": { \"lux1\": %.2f, \"lux2\": %.2f, \"lux3\": %.2f, \"pt\": %.2f, \"rl\": %.2f, \"tp\": %.2f, \"vb\": %.2f, \"vs\": %.4f, \"i\": %.4f, \"p\": %.4f }\n}\n",
            has_pending_msg ? "true" : "false",
            arrayBH1750[0], arrayBH1750[1], arrayBH1750[2],
            arrayMPU6050[0], arrayMPU6050[1],
            g_temp,
            arrayINA219[0], arrayINA219[1], arrayINA219[2], arrayINA219[3]
        );

        // tenta enviar (ou armazena e gere reconexão)
        tcp_client_send(payload);

        // Faz polling do Wi-Fi / lwip e tenta descarregar pending messages
        for (int i = 0; i < 2; i++) {
            sleep_ms(1000);        // 1 segundo × 600 = 10 minutos
            cyw43_arch_poll();     // mantém o Wi-Fi vivo
            // a cada loop pequeno, tentamos flush se possível
            tcp_client_flush_pending_if_possible();
        }
    }

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
    sleep_ms(1000);

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
    sleep_ms(500);

    return true;
}

void write_oled_values(void){
    char lux1_str[16];
    snprintf(lux1_str, sizeof(lux1_str), "l1=%.2f", arrayBH1750[0]);
    char lux2_str[16];
    snprintf(lux2_str, sizeof(lux2_str), "l2=%.2f", arrayBH1750[1]);
    char lux3_str[16];
    snprintf(lux3_str, sizeof(lux3_str), "l3=%.2f", arrayBH1750[2]);

    char pitch_str[16];
    snprintf(pitch_str, sizeof(pitch_str), "pt=%.2f", arrayMPU6050[0]);
    char roll_str[16];
    snprintf(roll_str, sizeof(roll_str), "rl=%.2f", arrayMPU6050[1]);
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "tp=%.2f", g_temp);

    char vbus_str[16];
    snprintf(vbus_str, sizeof(vbus_str), "vb=%.2f", arrayINA219[0]);
    char vshunt_str[16];
    snprintf(vshunt_str, sizeof(vshunt_str), "vs=%.4f", arrayINA219[1]);
    char current_str[16];
    snprintf(current_str, sizeof(current_str), "i=%.4f", arrayINA219[2]);
    char power_str[16];
    snprintf(power_str, sizeof(power_str), "p=%.4f", arrayINA219[3]);

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