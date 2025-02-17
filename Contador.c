#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#define WIFI_SSID "brisa-2724025"
#define WIFI_PASSWORD "ahfx2xc9"

#define BT_A 5  
#define BT_B 6 
#define LED 12  

static struct tcp_pcb *server_pcb;
static int contador = 0;


void send_webpage(struct tcp_pcb *client_pcb) {
    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
             "<html><head><title>Contador Pico W</title></head>"
             "<body><h1>Contador: %d</h1>"
             "<p>Pressione o botão físico para alterar.</p></body></html>",
             contador);

    tcp_write(client_pcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
}


err_t server_callback(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    if (err == ERR_OK) {
        send_webpage(client_pcb);
        tcp_output(client_pcb);
        tcp_close(client_pcb);
    }
    return ERR_OK;
}


void start_server() {
    server_pcb = tcp_new();
    if (!server_pcb) {
        printf("Erro ao criar socket!\n");
        return;
    }
    tcp_bind(server_pcb, IP_ADDR_ANY, 80);
    server_pcb = tcp_listen_with_backlog(server_pcb, 1);
    tcp_accept(server_pcb, server_callback);
}


void setup() {
    stdio_init_all();
    sleep_ms(2000); 

    gpio_init(BT_A);
    gpio_init(BT_B);
    gpio_init(LED);
    gpio_set_dir(BT_A, GPIO_IN);
    gpio_set_dir(BT_B, GPIO_IN);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_pull_up(BT_A);
    gpio_pull_up(BT_B);

    printf("Iniciando Wi-Fi...\n");

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return;
    }
    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha ao conectar no Wi-Fi.\n");
        return;
    }

    printf("Conectado!\n");
    start_server();
}


void atualizar_contador() {
    printf("Contador: %d\n", contador);
    gpio_put(LED, 1);
    sleep_ms(100);
    gpio_put(LED, 0);
}

void loop() {
    while (1) {
        if (!gpio_get(BT_A)) {
            contador = (contador + 1) % 10; 
            atualizar_contador();
            sleep_ms(300);
        }

        if (!gpio_get(BT_B)) {
            contador = 0;
            printf("Contador Resetado!\n");
            sleep_ms(300);
        }

        cyw43_arch_poll();
    }
}

int main() {
    setup();
    loop();
    return 0;
}
