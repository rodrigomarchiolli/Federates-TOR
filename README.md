# Firmware PoC: ESP32 como Broker MQTT + Cliente Tor

Prova de conceito do TCC. Um unico ESP32 (com PSRAM) atua como **broker MQTT
local** para os sensores e reencaminha os dados para um broker/servico remoto
**atraves da rede Tor**, dispensando o gateway externo descrito no `doc.md`.
Sensor ESP/PC --MQTT 1883--> [ Broker local (PicoMQTT) ] | v ponte [ Socks5Client -> 127.0.0.1:9050 ] | v [ toresp32 (proxy SOCKS5 + Tor) ] | v rede Tor Broker remoto / servico .onion
## Pre-requisitos

- ESP32 **com PSRAM** (ESP32-WROVER, S2 ou S3). Sem PSRAM o Tor nao roda.
- VSCode + PlatformIO.
- `toresp32` rodando neste mesmo ESP32, expondo SOCKS5 em `127.0.0.1:9050`
  (ver [docs/01_tor_spike.md](docs/01_tor_spike.md)).
- Para os testes de host: `pip install paho-mqtt`.

## Configuracao

Edite [src/config.h](src/config.h): Wi-Fi, porta do broker local, host/porta do
proxy Tor, destino remoto (`.onion` ou host comum) e pseudonimo do dispositivo.

## Como construir e gravar (por fase)

Cada fase do plano tem um ambiente PlatformIO dedicado:

| Fase | Ambiente            | O que valida                                        |
| ---- | ------------------- | --------------------------------------------------- |
| 1    | (projeto toresp32)  | Tor isolado no ESP32 - ver `docs/01_tor_spike.md`   |
| 2    | `broker_only`       | broker local recebendo JSON ECG/EMG/EDA             |
| 3    | `socks_test`        | wrapper SOCKS5 abrindo conexao via Tor              |
| 4    | `integrated`        | broker + ponte de saida via Tor (fluxo completo)    |
| 5    | `integrated`        | medicao - ver `docs/05_measurement.md`              |
bash pio run -e broker_only -t upload -t monitor pio run -e socks_test -t upload -t monitor pio run -e integrated -t upload -t monitor
Para enviar dados de teste ao broker local:
bash python tools/publish_test.py --host <ip-do-esp32> --count 20 --interval 2
## Estrutura
firmware/ platformio.ini # 3 ambientes (broker_only, socks_test, integrated) src/ config.h # parametros e selecao de modo socks5_client.{h,cpp} # transporte SOCKS5 (interface Arduino Client) mqtt_bridge.{h,cpp} # broker local + ponte de saida via Tor metrics.{h,cpp} # instrumentacao (latencia, perda, memoria) main.cpp # setup/loop + simulador de sensor tools/publish_test.py # publicador MQTT de teste (host) docs/ # guias das Fases 1 e 5
## Integracao com o Tor (toresp32)

`PicoMQTT::Client` aceita qualquer `Client` Arduino como transporte. O
`Socks5Client` implementa essa interface e faz o handshake SOCKS5 contra o
proxy local do `toresp32`. Como o metodo `connect(host, port)` usa o tipo de
endereco "dominio" do SOCKS5, enderecos `.onion` sao resolvidos pelo proprio
Tor.

Para empacotar o broker (Arduino) e o Tor (`toresp32`, ESP-IDF) no mesmo
binario, a abordagem do plano e usar **arduino-esp32 como componente do
ESP-IDF**. Em PlatformIO isso corresponde a montar um projeto ESP-IDF que
inclui `toresp32` como componente e o core Arduino como componente, compartilhando
a stack TCP/IP (lwIP). O `Socks5Client` conecta ao proxy via `127.0.0.1` (ou o
IP do proprio dispositivo), de modo que broker e Tor coexistem no mesmo ESP32.

## Limitacoes (relevantes para o TCC)

- `toresp32` e projeto educacional sem auditoria (doc.md, secao 11.1).
- Memoria e o gargalo: Tor + broker + Arduino podem estourar a RAM mesmo com
  PSRAM. Reduza circuitos prontos e buffers se necessario.
- O Tor nao criptografa o payload (doc.md, secao 7). Use MQTTS/criptografia de
  payload em producao.
- Performance limitada por design (240 MHz). A medicao da Fase 5 quantifica o
  custo em relacao ao gateway externo.