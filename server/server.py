import socket
import json
from datetime import datetime, timezone, timedelta

# ===============================
# CONFIGURAÇÕES
# ===============================
TCP_IP = "0.0.0.0"   # escuta em todas as interfaces
TCP_PORT = 9999
OUTPUT_FILE = "..\\solar_station_v2\\server\\data.txt"
BUFFER_SIZE = 4096

# ===============================
# SOCKET TCP
# ===============================
server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_sock.bind((TCP_IP, TCP_PORT))
server_sock.listen(5)   # permite até 5 conexões em espera

print(f"\nServidor TCP escutando em {TCP_IP}:{TCP_PORT}")

# ===============================
# LOOP PRINCIPAL
# ===============================
while True:
    try:
        conn, addr = server_sock.accept()
        print(f"\nConexão estabelecida com: {addr}")

        # cada cliente fica dentro deste loop até fechar conexão
        while True:
            data = conn.recv(BUFFER_SIZE)
            if not data:
                print(f"Conexão encerrada por {addr}")
                break

            raw_message = data.decode("utf-8")

            try:
                payload = json.loads(raw_message)

                # timestamp caso não tenha
                if "received_at" not in payload:
                    payload["received_at"] = datetime.now(timezone(timedelta(hours=-3))).isoformat(timespec='minutes')

                # salva cada JSON em uma linha
                with open(OUTPUT_FILE, "a", encoding="utf-8") as f:
                    f.write(json.dumps(payload) + "\n")

                print(f"Dados recebidos de {addr}: {payload}")

            except json.JSONDecodeError:
                print(f"JSON inválido recebido de {addr}: {raw_message}")

    except Exception as e:
        print(f"Erro inesperado: {e}")