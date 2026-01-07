#include "tcp_client.h"

// --- TCP state & buffers ---
struct tcp_pcb *client_pcb = NULL;
ip_addr_t server_addr;
volatile int tcp_connected_flag = 0;
volatile int tcp_trying_connect = 0;

// mensagem pendente (simples fila de 1 elemento)
char pending_msg[PENDING_MSG_MAX];
volatile int has_pending_msg = 0;

/* ------------- TCP callbacks --------------- */

// chamado quando conexão estabelecida (ou falha)
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("tcp_client_connected: erro ao conectar: %d\n", err);
        tcp_connected_flag = 0;
        tcp_trying_connect = 0;
        // tpcb pode ser fechado pelo stack, deixe o poll/err tratar
        return err;
    }

    tcp_connected_flag = 1;
    tcp_trying_connect = 0;
    printf("TCP conectado ao servidor %s:%d\n", SERVER_IP, SERVER_PORT);

    // Registrar callbacks úteis
    tcp_err(tpcb, NULL); // registramos error callback separadamente se quisermos
    tcp_sent(tpcb, NULL); // podemos usar para saber quando o TX buffer foi liberado
    tcp_poll(tpcb, NULL, 0);

    // se tinha mensagem pendente, tenta enviar agora
    if (has_pending_msg) {
        // nada aqui: o envio será feito pela função pública tcp_client_send
    }

    return ERR_OK;
}

// erro fatal da conexão: o LWIP chama isso quando o pcb é eliminado
static void tcp_client_err(void *arg, err_t err) {
    (void) arg;
    printf("tcp_client_err: conexão encerrada com erro %d\n", err);
    tcp_connected_flag = 0;
    tcp_trying_connect = 0;
    client_pcb = NULL; // pcb inválido agora
    // marcar reconexão
}

// poll callback: se passarmos NULL para tcp_poll, não entra; vamos registrar um poll manualmente na criação
static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    (void) arg;
    (void) tpcb;

    // se desconectado ou pcb inválido, tente reconectar
    if (!tcp_connected_flag && !tcp_trying_connect) {
        printf("tcp_client_poll: detectado desconexão, tentando reconectar...\n");
        tcp_trying_connect = 1;
        tcp_client_try_reconnect();
    }

    // se há mensagem pendente e estamos conectados, tente enviar
    if (tcp_connected_flag && has_pending_msg) {
        // nada: a lógica de envio central fará tcp_output
    }

    return ERR_OK;
}

// callback quando os dados foram efetivamente enviados (confirmados)
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    (void) arg;
    (void) tpcb;
    (void) len;
    // espaço liberado — podemos limpar a mensagem pendente se for a que estava na fila
    if (has_pending_msg) {
        // assumimos que a mensagem foi enviada corretamente, então marcamos como enviada
        has_pending_msg = 0;
        pending_msg[0] = '\0';
        // se você quiser manter histórico ou enviar a próxima mensagem, faria aqui
    }
    return ERR_OK;
}

/* ------------- TCP helpers ---------------- */

// cria novo pcb, registra callbacks e tenta conectar
void tcp_client_start(void) {
    if (client_pcb) {
        // se já existe um pcb, não cria outro
        return;
    }

    client_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!client_pcb) {
        printf("tcp_client_start: erro ao criar PCB\n");
        tcp_trying_connect = 0;
        return;
    }

    // define callbacks: precisamos usar ponteiros não-NULL para captar erros, poll e sent
    tcp_arg(client_pcb, NULL);
    tcp_err(client_pcb, tcp_client_err);
    tcp_poll(client_pcb, tcp_client_poll, 4);  // poll a cada 4 * (tcp timer) - ajuste se necessário
    tcp_sent(client_pcb, tcp_client_sent);

    err_t err = tcp_connect(client_pcb, &server_addr, SERVER_PORT, tcp_client_connected);
    if (err != ERR_OK) {
        printf("tcp_client_start: falha tcp_connect: %d\n", err);
        // cleanup e sinaliza tentativa futura
        tcp_trying_connect = 0;
        tcp_connected_flag = 0;
        tcp_abort(client_pcb); // força liberar
        client_pcb = NULL;
        return;
    }

    tcp_trying_connect = 1;
    printf("tcp_client_start: iniciando tentativa de conexão...\n");
}

// fecha e limpa pcb (uso quando quiser desconectar explicitamente)
void tcp_client_close(void) {
    if (client_pcb) {
        tcp_arg(client_pcb, NULL);
        tcp_sent(client_pcb, NULL);
        tcp_close(client_pcb);
        client_pcb = NULL;
    }
    tcp_connected_flag = 0;
    tcp_trying_connect = 0;
}

// tenta reconectar recriando o PCB
void tcp_client_try_reconnect(void) {
    // se já existe pcb, tenta conectar usando ele (mas normalmente client_pcb == NULL aqui)
    if (client_pcb) {
        // se pcb existe mas não conectado, tente conectar novamente
        err_t err = tcp_connect(client_pcb, &server_addr, SERVER_PORT, tcp_client_connected);
        if (err != ERR_OK) {
            printf("tcp_client_try_reconnect: reconnect falhou: %d\n", err);
            tcp_abort(client_pcb);
            client_pcb = NULL;
            tcp_trying_connect = 0;
        } else {
            tcp_trying_connect = 1;
        }
        return;
    }

    // cria novo pcb
    tcp_client_start();
}

// função de envio resiliente: tenta enviar; se não puder, armazena em pending_msg
void tcp_client_send(const char *msg) {
    if (!msg) return;

    // se já tem pending, sobrescreve (simples política). Pode ser alterado para fila real.
    int len = strlen(msg);
    if (len <= 0 || len >= PENDING_MSG_MAX) {
        printf("tcp_client_send: tamanho inválido (%d bytes)\n", len);
        return;
    }

    // se não conectado, armazena e tenta reconectar
    if (!tcp_connected_flag || client_pcb == NULL) {
        printf("tcp_client_send: não conectado, armazenando mensagem e tentando reconectar\n");
        strncpy(pending_msg, msg, PENDING_MSG_MAX - 1);
        pending_msg[PENDING_MSG_MAX - 1] = '\0';
        has_pending_msg = 1;
        if (!tcp_trying_connect) {
            tcp_client_try_reconnect();
        }
        return;
    }

    // tentamos escrever diretamente
    err_t err = tcp_write(client_pcb, msg, len, TCP_WRITE_FLAG_COPY);

    if (err == ERR_OK) {
        // solicita envio inmediato do stack
        err = tcp_output(client_pcb);
        if (err != ERR_OK) {
            printf("tcp_client_send: tcp_output retornou %d\n", err);
            // manter a mensagem como pendente para tentar depois
            strncpy(pending_msg, msg, PENDING_MSG_MAX - 1);
            has_pending_msg = 1;
        } else {
            // tudo bem: registramos que não há pendente
            has_pending_msg = 0;
        }
        return;
    }

    if (err == ERR_MEM) {
        // Buffer do TCP cheio: armazena a mensagem e deixe o callback tcp_client_sent liberar
        printf("tcp_client_send: ERR_MEM (buffer cheio). Salvando mensagem para retry\n");
        strncpy(pending_msg, msg, PENDING_MSG_MAX - 1);
        has_pending_msg = 1;
        return;
    }

    // outros erros: marca desconexão e tenta reconectar
    printf("tcp_client_send: erro ao tcp_write: %d\n", err);
    strncpy(pending_msg, msg, PENDING_MSG_MAX - 1);
    has_pending_msg = 1;

    // força reconexão
    if (client_pcb) {
        tcp_abort(client_pcb);
        client_pcb = NULL;
    }
    tcp_connected_flag = 0;
    tcp_trying_connect = 0;
    tcp_client_try_reconnect();
}

/* periodic check to flush pending messages (chamado no main loop) */
void tcp_client_flush_pending_if_possible(void) {
    if (has_pending_msg && tcp_connected_flag && client_pcb) {
        tcp_client_send(pending_msg);
    }
}