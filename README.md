<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=4f46e5&height=120&section=header"/>

# Estação Inteligente de Monitoramento e Otimização de Eficiência de Painéis Solares

## Visão Geral

Este repositório contém o código-fonte e a documentação do projeto **Estação Inteligente de Monitoramento e Otimização de Eficiência de Painéis Solares**, desenvolvido como parte da **Residência Tecnológica em Software Embarcado – EMBARCATECH (IFCE)**.

O projeto propõe uma solução embarcada e conectada (IoT) capaz de **monitorar variáveis críticas de um sistema fotovoltaico** e **otimizar automaticamente a orientação do painel solar** por meio de rastreamento ativo (*solar tracking*). A solução integra hardware, firmware em tempo real, comunicação em rede e um dashboard web para visualização de dados em tempo real e históricos, aproximando-se de um **Produto Mínimo Viável (MVP)** funcional.

## Objetivos

### Objetivo Geral

Desenvolver e consolidar um sistema embarcado para monitoramento e rastreamento solar, integrado a uma infraestrutura IoT, capaz de adquirir, processar e transmitir dados em tempo real, disponibilizando-os ao usuário por meio de um dashboard web.

### Objetivos Específicos

* Implementar um algoritmo de **rastreamento solar inteligente**, baseado em múltiplos sensores;
* Estruturar o firmware de forma **modular e escalável**, utilizando **FreeRTOS**;
* Desenvolver um **dashboard web** para visualização em tempo real e análise histórica;
* Integrar hardware, firmware e aplicação web de forma coesa;
* Validar o funcionamento do sistema completo por meio de testes práticos.

## Arquitetura do Sistema

O sistema é dividido em três grandes camadas:

### Hardware

* **Microcontrolador:** Raspberry Pi Pico W (RP2040)
* **Sensores:**

  * BH1750 – Iluminância (3 sensores via multiplexador I²C PCA9548A)
  * MPU6050 – Orientação e inclinação
  * INA219 – Tensão, corrente e potência
  * DS18B20 – Temperatura do painel
* **Atuador:**

  * Motor de passo **NEMA 23**
  * Driver **TB6600**
* **Outros:**

  * Display OLED SSD1306
  * LED de status
  * Estrutura mecânica com peça impressa em 3D para sombreamento dos sensores

### Firmware Embarcado

* Desenvolvido em **C/C++ com Pico SDK**
* Sistema operacional de tempo real: **FreeRTOS**
* Arquitetura modular organizada em drivers
* Comunicação Wi-Fi via **lwIP**
* Segurança com **HMAC (mbedTLS)**
* Watchdog baseado em *bitmask* para tolerância a falhas

### IoT e Aplicação Web

* **Servidor TCP:** Python (sockets)
* **Backend:** Node.js + Express
* **Banco de Dados:** PostgreSQL (LISTEN / NOTIFY)
* **Comunicação em tempo real:** WebSockets
* **Frontend:** React
* **API REST:** consulta histórica e dados recentes

## Tarefas FreeRTOS

* **sensor_task**: Leitura periódica dos sensores e consolidação dos dados
* **comm_task**: Gerenciamento Wi-Fi, envio TCP, atualização do display e heartbeat
* **tracking_task**: Algoritmo de rastreamento solar e controle do motor de passo
* **mock_serial_task**: Injeção de dados simulados via UART para testes

> [!NOTE]
> Apenas uma entre `sensor_task` ou `mock_serial_task` pode estar ativa, controlado pela macro `ENABLE_SERIAL_MOCK`.

## Algoritmo de Rastreamento Solar

O algoritmo de *solar tracking* implementado vai além da simples comparação entre sensores de luminosidade. Ele inclui:

* Rotina de **homing inicial** baseada no sensor inercial;
* Zona morta (*deadzone*) para evitar oscilações;
* Limiar mínimo de variação de iluminância;
* Intervalo mínimo entre movimentos;
* Limitação mecânica do ângulo de rotação;
* Detecção de baixa luminosidade (modo noturno);
* Controle proporcional do número de passos do motor;
* **Validação energética**: movimentos só são mantidos se houver ganho real de tensão/potência.

Essa abordagem torna o sistema mais robusto, eficiente e adequado a condições ambientais reais.

## Dashboard Web

O dashboard permite:

* Visualização **em tempo real** das variáveis do sistema;
* Consulta **histórica** por intervalo de tempo;
* Monitoramento de:

  * Iluminância
  * Temperatura do painel
  * Tensão, corrente e potência
  * Ângulo de inclinação

A atualização em tempo real é feita via **WebSockets**, evitando *polling* constante.

## Evidências de Funcionamento

O sistema foi validado em testes práticos, inclusive sob condições adversas (dia nublado e iluminação difusa). Mesmo nesses cenários, o algoritmo demonstrou:

* Capacidade de evitar movimentos desnecessários;
* Correlação entre ajuste angular e aumento de tensão;
* Funcionamento estável ao longo de horas contínuas de operação.

Os resultados confirmam a **viabilidade técnica** da solução proposta.

## Trabalhos Futuros

* Uso de painel fotovoltaico de maior potência com redução mecânica;
* Expansão para **rastreamento em dois eixos**;
* Integração de **modelos astronômicos** de posição solar;
* Aplicação de técnicas de **controle adaptativo ou preditivo**;
* Análise energética de longo prazo.

## Estrutura do Repositório (resumida)

```
├── drivers/        # Firmware embarcado (sensores, rede, segurança, atuadores)
├── server/         # Servidor TCP em Python
├── client/
│   ├── api/        # Backend Node.js
│   └── src/        # Frontend React
├── CMakeLists.txt
├── main.c
└── README.md
```

## Autor

**Kauã Lima de Queiroz**
Instituto Federal do Ceará – IFCE

Residência Tecnológica em Software Embarcado (EMBARCATECH)

_Gostou do meu perfil? Você pode saber mais sobre mim em:_ &nbsp;&nbsp;[![LinkedIn](https://img.shields.io/badge/LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white)](https://www.linkedin.com/in/kaualimaq/)

_Ou me contatar através do:_ &nbsp;&nbsp;[![Gmail](https://img.shields.io/badge/Gmail-333333?style=for-the-badge&logo=gmail&logoColor=red)](mailto:limakaua610@gmail.com)

> [!IMPORTANT]
> Este projeto é de caráter acadêmico e educacional. Consulte o autor para usos comerciais ou redistribuição.
