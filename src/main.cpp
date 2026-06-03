#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "config.h"
#include "metrics.h"
#include "mqtt_bridge.h"
#include "socks5_client.h"

static Metrics metrics;
static MqttBridge bridge(metrics);

static void connectWiFi() {
    Serial.printf("[wifi] conectando a '%s'", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.printf("\n[wifi] conectado, IP=%s\n",
                  WiFi.localIP().toString().c_str());
}

#if ENABLE_SENSOR_SIMULATOR
// Gera um payload no formato do doc.md (secao 4.1) com sinais simulados.
static void publishSimulatedSample() {
    JsonDocument doc;
    doc["device_id"] = DEVICE_PSEUDONYM;
    doc["timestamp"] = millis();
    JsonArray ecg = doc["ecg"].to<JsonArray>();
    JsonArray emg = doc["emg"].to<JsonArray>();
    JsonArray eda = doc["eda"].to<JsonArray>();
    for (int i = 0; i < 4; i++) {
        ecg.add(0.10f + (float)random(0, 50) / 1000.0f);
        emg.add(0.45f + (float)random(0, 100) / 1000.0f);
        eda.add(1.80f + (float)random(0, 80) / 1000.0f);
    }
    char buf[256];
    serializeJson(doc, buf, sizeof(buf));
    bridge.publishLocal(LOCAL_SUBSCRIBE_TOPIC, buf);
}
#endif

#if defined(MODE_SOCKS_TEST)
// Fase 3: valida o wrapper SOCKS5 abrindo uma conexao TCP atraves do Tor.
static void runSocksConnectivityTest() {
    const char *host = "example.com";
    const uint16_t port = 80;

    Socks5Client client(TOR_SOCKS_HOST, TOR_SOCKS_PORT, TOR_SOCKS_USER,
                        TOR_SOCKS_PASS);
    Serial.printf("[socks] conectando a %s:%u via Tor (%s:%d)...\n", host, port,
                  "127.0.0.1", TOR_SOCKS_PORT);

    const uint32_t t0 = millis();
    metrics.onTorConnectAttempt();
    if (client.connect(host, port)) {
        const uint32_t dt = millis() - t0;
        metrics.onTorConnectSuccess(dt);
        Serial.printf("[socks] handshake OK em %lu ms\n", (unsigned long)dt);

        client.print("HEAD / HTTP/1.0\r\nHost: example.com\r\n\r\n");
        const uint32_t deadline = millis() + 15000;
        bool gotData = false;
        while (millis() < deadline && client.connected()) {
            while (client.available()) {
                Serial.write(client.read());
                gotData = true;
            }
            delay(10);
        }
        Serial.printf("\n[socks] teste %s\n",
                      gotData ? "SUCESSO" : "sem resposta");
        client.stop();
    } else {
        Serial.printf("[socks] FALHA (erro=%d). O toresp32 esta rodando e o "
                      "circuito Tor pronto?\n",
                      client.lastError());
    }
}
#endif

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println(F("=== ESP32 Broker MQTT + Cliente Tor (PoC TCC) ==="));

#if defined(ESP32)
    if (psramFound()) {
        Serial.printf("[mem] PSRAM detectada: %u bytes\n",
                      (unsigned)ESP.getPsramSize());
    } else {
        Serial.println(F("[mem] AVISO: PSRAM nao detectada. O Tor (toresp32) "
                         "provavelmente nao roda neste hardware."));
    }
#endif

    connectWiFi();
    metrics.begin(METRICS_REPORT_INTERVAL_MS);
    randomSeed(esp_random());

#if defined(MODE_BROKER_ONLY)
    Serial.println(F("[modo] BROKER_ONLY (Fase 2)"));
    bridge.beginBrokerOnly();
#elif defined(MODE_SOCKS_TEST)
    Serial.println(F("[modo] SOCKS_TEST (Fase 3)"));
    runSocksConnectivityTest();
#else  // MODE_INTEGRATED
    Serial.println(F("[modo] INTEGRATED (Fase 4)"));
    bridge.beginIntegrated();
#endif
}

void loop() {
#if defined(MODE_SOCKS_TEST)
    // Em modo de teste, o trabalho ocorre uma unica vez no setup().
    metrics.maybeReport();
    delay(1000);
    return;
#else
    bridge.loop();
    metrics.maybeReport();

#if ENABLE_SENSOR_SIMULATOR
    static uint32_t lastPublish = 0;
    if ((uint32_t)(millis() - lastPublish) >= SENSOR_PUBLISH_INTERVAL_MS) {
        lastPublish = millis();
        publishSimulatedSample();
    }
#endif
#endif
}