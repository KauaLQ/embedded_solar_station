#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdbool.h>

#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

// --- TCP ---
#define SERVER_IP "192.168.1.105" // IP do servidor Python
#define SERVER_PORT 9999
#define PENDING_MSG_MAX 1024

// --- TCP state (declarações) ---
extern struct tcp_pcb *client_pcb;
extern ip_addr_t server_addr;
extern volatile int tcp_connected_flag;
extern volatile int tcp_trying_connect;
extern volatile int has_pending_msg;
extern volatile int sending_pending_msg;

// inicializa o cliente TCP (cria PCB e tenta conectar)
void tcp_client_start(void);

// envia mensagem (ou armazena se não conectado)
void tcp_client_send(const char *msg);

// tenta conectar ou reconectar ao TCP
void tcp_client_try_reconnect(void);

// limpa mensagens antigas se for necessário
void tcp_client_flush_pending_if_possible(void);

// fecha conexão TCP (opcional)
void tcp_client_close(void);

#endif // TCP_CLIENT_H