import socket
import json
import threading
import hmac
import hashlib
import re
from datetime import datetime, timezone, timedelta

# ===============================
# CONFIGURAÇÕES
# ===============================
TCP_IP = "0.0.0.0"
TCP_PORT = 9999
OUTPUT_FILE = "..\\solar_station_v2\\server\\data.txt"
BUFFER_SIZE = 4096
HMAC_SECRET = b"solar_station_secret_2025"

def extract_data_json(raw_message: str) -> str:
    """
    Extrai o objeto JSON de 'data' preservando a formatação original.
    """
    match = re.search(r'"data"\s*:\s*(\{.*\})\s*$', raw_message)
    if not match:
        return None
    return match.group(1)

def verify_hmac(raw_message: str) -> bool:
    try:
        payload = json.loads(raw_message)
    except json.JSONDecodeError:
        return False

    received_hmac = payload.get("meta", {}).get("hmac")
    if not received_hmac:
        return False

    data_json = extract_data_json(raw_message)
    if not data_json:
        return False

    # Encontra onde começa o trecho desejado
    inicio = data_json.find('{"lux1"')

    # Encontra o primeiro "}" APÓS a posição de início
    # O segundo argumento do find diz por onde ele deve começar a procurar
    fim = data_json.find('}', inicio)

    # Extrai o pedaço (usamos +1 para incluir o caractere "}" no resultado)
    data_json_split = data_json[inicio:fim+1]

    expected_hmac = hmac.new(HMAC_SECRET, data_json_split.encode(), hashlib.sha256).hexdigest()

    return hmac.compare_digest(received_hmac, expected_hmac)

def handle_client(conn, addr):
    """Função que gerencia cada conexão de cliente em uma thread separada."""
    print(f"\n[NOVA CONEXÃO] {addr}")
    
    # Configura o Keep-Alive no socket da conexão específica
    # Isso detecta se o cliente caiu sem fechar a conexão
    conn.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
    
    # Configurações específicas para Linux/Windows (opcional, ajusta o tempo de detecção)
    # No Windows, o padrão de detecção de queda pode ser longo. 
    # Com Keep-alive, o SO gerencia isso.
    
    try:
        while True:
            try:
                data = conn.recv(BUFFER_SIZE)
                if not data:
                    print(f"[DESCONECTADO] Conexão encerrada de forma limpa por {addr}")
                    break

                raw_message = data.decode("utf-8")

                try:
                    payload = json.loads(raw_message)

                    if not verify_hmac(raw_message):
                        print(f"[HMAC INVÁLIDO] Mensagem descartada de {addr}")
                        continue

                    # HMAC validado → remover do payload
                    payload.get("meta", {}).pop("hmac", None)

                    if "received_at" not in payload:
                        payload["received_at"] = datetime.now(timezone(timedelta(hours=-3))).isoformat(timespec='minutes')

                    with open(OUTPUT_FILE, "a", encoding="utf-8") as f:
                        f.write(json.dumps(payload) + "\n")

                    print(f"[DADOS] {addr}: {payload}")

                except json.JSONDecodeError:
                    print(f"[ERRO JSON] Dados inválidos de {addr}: {raw_message}")
        
            except socket.timeout:
                continue # O timeout de leitura não fecha a conexão, apenas permite o loop rodar
            except ConnectionResetError:
                print(f"[ERRO] Conexão forçada a fechar (Raspberry caiu) em {addr}")
                break
    except Exception as e:
        print(f"[ERRO DESCONHECIDO] {addr}: {e}")
    finally:
        conn.close()
        print(f"[ENCERRADO] Socket fechado para {addr}")

def start_server():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # SO_REUSEADDR permite reiniciar o server imediatamente sem erro de "Porta em uso"
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((TCP_IP, TCP_PORT))
    server_sock.listen(5)

    print(f"\nServidor TCP escutando em {TCP_IP}:{TCP_PORT}")
    print("Aguardando conexões...")

    while True:
        try:
            conn, addr = server_sock.accept()
            # Cria uma thread para cada cliente. 
            # Assim, se a Rasp antiga "travar", uma nova thread atenderá a Rasp nova.
            thread = threading.Thread(target=handle_client, args=(conn, addr))
            thread.daemon = True # Thread morre se o programa principal fechar
            thread.start()
        except Exception as e:
            print(f"Erro ao aceitar conexão: {e}")

if __name__ == "__main__":
    start_server()