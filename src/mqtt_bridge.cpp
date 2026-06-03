#include "mqtt_bridge.h"

#include <ArduinoJson.h>

#include "config.h"

MqttBridge::MqttBridge(Metrics &metrics)
    : metrics(metrics),
      localBroker(LOCAL_BROKER_PORT),
      socks(TOR_SOCKS_HOST, TOR_SOCKS_PORT, TOR_SOCKS_USER, TOR_SOCKS_PASS) {}

void MqttBridge::beginBrokerOnly() {
    bridging = false;
    localBroker.subscribe(
        LOCAL_SUBSCRIBE_TOPIC,
        [this](const char *topic, const char *payload) {
            handleLocalMessage(topic, payload);
        });
    localBroker.begin();
    Serial.printf("[broker] escutando na porta %d, topico '%s'\n",
                  LOCAL_BROKER_PORT, LOCAL_SUBSCRIBE_TOPIC);
}

void MqttBridge::beginIntegrated() {
    bridging = true;

    // Cliente de saida usando o Socks5Client como transporte. Assim toda
    // conexao ao broker remoto atravessa o Tor local.
    remote = new PicoMQTT::Client(socks, REMOTE_BROKER_HOST, REMOTE_BROKER_PORT,
                                  REMOTE_CLIENT_ID, REMOTE_USER, REMOTE_PASS);

    static uint32_t connectStartMs = 0;
    remote->disconnected_callback = [this]() {
        connectStartMs = millis();
        metrics.onTorConnectAttempt();
        Serial.println(F("[tor] tentando (re)conectar ao broker remoto..."));
    };
    remote->connected_callback = [this]() {
        uint32_t dt = millis() - connectStartMs;
        metrics.onTorConnectSuccess(dt);
        Serial.printf("[tor] conectado ao broker remoto em %lu ms\n",
                      (unsigned long)dt);
    };
    remote->connection_failure_callback = [this]() {
        Serial.printf("[tor] falha de conexao (socks err=%d)\n",
                      socks.lastError());
    };

    localBroker.subscribe(
        LOCAL_SUBSCRIBE_TOPIC,
        [this](const char *topic, const char *payload) {
            handleLocalMessage(topic, payload);
        });

    localBroker.begin();
    remote->begin();
    Serial.printf(
        "[bridge] broker local na porta %d -> Tor (%s:%d) -> %s:%d\n",
        LOCAL_BROKER_PORT, "127.0.0.1", TOR_SOCKS_PORT, REMOTE_BROKER_HOST,
        REMOTE_BROKER_PORT);
}

void MqttBridge::loop() {
    localBroker.loop();
    if (bridging && remote != nullptr) {
        remote->loop();
    }
}

void MqttBridge::publishLocal(const char *topic, const char *payload) {
    localBroker.publish(topic, payload);
}

void MqttBridge::handleLocalMessage(const char *topic, const char *payload) {
    const size_t len = strlen(payload);
    metrics.onLocalReceived(len);

    // Valida o formato do payload (doc.md, secao 4.1) sem armazenar dados
    // brutos: apenas conta as amostras de cada sinal.
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[broker] payload invalido em '%s': %s\n", topic,
                      err.c_str());
        return;
    }
    const size_t ecg = doc["ecg"].is<JsonArray>() ? doc["ecg"].size() : 0;
    const size_t emg = doc["emg"].is<JsonArray>() ? doc["emg"].size() : 0;
    const size_t eda = doc["eda"].is<JsonArray>() ? doc["eda"].size() : 0;
    Serial.printf("[broker] %s (%u bytes) ecg=%u emg=%u eda=%u\n", topic,
                  (unsigned)len, (unsigned)ecg, (unsigned)emg, (unsigned)eda);

    if (bridging) {
        forwardThroughTor(payload);
    }
}

void MqttBridge::forwardThroughTor(const char *payload) {
    if (remote == nullptr) return;
    const uint32_t t0 = millis();
    const bool ok = remote->publish(REMOTE_PUBLISH_TOPIC, payload);
    if (ok) {
        metrics.onForwardSuccess(millis() - t0);
    } else {
        metrics.onForwardFailure();
        Serial.println(F("[bridge] falha ao reencaminhar (sem conexao Tor?)"));
    }
}