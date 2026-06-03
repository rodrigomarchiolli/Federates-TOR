#pragma once

// ---------------------------------------------------------------------------
// Configuracao do firmware. Ajuste os valores abaixo antes de compilar.
// As credenciais ficam aqui apenas para facilitar a prova de conceito; em
// producao use NVS / armazenamento seguro (ver doc.md, secao 12).
// ---------------------------------------------------------------------------

// --- Wi-Fi ---
#define WIFI_SSID "SUA_REDE_WIFI"
#define WIFI_PASSWORD "SUA_SENHA_WIFI"

// --- Broker MQTT local (rodando neste ESP32) ---
// Os sensores ESP32 / wearables publicam aqui.
#define LOCAL_BROKER_PORT 1883
#define LOCAL_SUBSCRIBE_TOPIC "esp32/data"

// --- Proxy SOCKS5 do Tor (toresp32 rodando neste mesmo ESP32) ---
// Aponte para a porta em que o toresp32 expoe o proxy SOCKS5.
#define TOR_SOCKS_HOST IPAddress(127, 0, 0, 1)
#define TOR_SOCKS_PORT 9050
// Deixe nullptr quando o proxy nao exige autenticacao.
#define TOR_SOCKS_USER nullptr
#define TOR_SOCKS_PASS nullptr

// --- Destino remoto (no de treinamento / broker remoto) ---
// Pode ser um endereco .onion (resolvido pelo Tor) ou um host normal.
#define REMOTE_BROKER_HOST "exemplodestino123abc.onion"
#define REMOTE_BROKER_PORT 1883
#define REMOTE_PUBLISH_TOPIC "iomt/stress/data"
#define REMOTE_CLIENT_ID "esp32_tor_bridge"
#define REMOTE_USER nullptr
#define REMOTE_PASS nullptr

// --- Pseudonimo do dispositivo (ver doc.md, secao 4.1) ---
#define DEVICE_PSEUDONYM "esp32_pseudo_001"

// --- Simulador interno de sensores (opcional) ---
// Quando 1, o proprio firmware publica payloads ECG/EMG/EDA simulados no
// broker local, util para testar sem um segundo dispositivo.
#define ENABLE_SENSOR_SIMULATOR 1
#define SENSOR_PUBLISH_INTERVAL_MS 5000

// --- Relatorio de metricas ---
#define METRICS_REPORT_INTERVAL_MS 10000

// ---------------------------------------------------------------------------
// Selecao de modo de build (definido via build_flags no platformio.ini):
//   MODE_BROKER_ONLY  -> Fase 2: somente broker local + recepcao JSON
//   MODE_SOCKS_TEST   -> Fase 3: testa o wrapper SOCKS5 contra o Tor
//   MODE_INTEGRATED   -> Fase 4: broker + ponte de saida via Tor
// Caso nenhum seja definido, assume MODE_INTEGRATED.
// ---------------------------------------------------------------------------
#if !defined(MODE_BROKER_ONLY) && !defined(MODE_SOCKS_TEST) && \
    !defined(MODE_INTEGRATED)
#define MODE_INTEGRATED
#endif

