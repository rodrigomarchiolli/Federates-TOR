#pragma once

#include <Arduino.h>
#include <PicoMQTT.h>

#include "metrics.h"
#include "socks5_client.h"

// MqttBridge implementa o nucleo da arquitetura: um broker MQTT local
// (PicoMQTT::Server) que recebe os dados dos sensores e os reencaminha para um
// broker remoto atraves do Tor, usando Socks5Client como transporte de saida.
//
// Fluxo (equivalente ao do doc.md, mas todo dentro do ESP32):
//   Sensor -> broker local (1883) -> ponte -> SOCKS5/Tor -> broker remoto
class MqttBridge {
public:
    MqttBridge(Metrics &metrics);

    // Inicia apenas o broker local (Fase 2).
    void beginBrokerOnly();

    // Inicia broker local + ponte de saida via Tor (Fase 4).
    void beginIntegrated();

    void loop();

    // Publica um payload diretamente no broker local (usado pelo simulador).
    void publishLocal(const char *topic, const char *payload);

private:
    Metrics &metrics;
    // ServerLocalSubscribe (e nao Server) garante que callbacks de inscricao
    // tambem disparem para mensagens publicadas localmente (ex.: pelo
    // simulador interno de sensores).
    PicoMQTT::ServerLocalSubscribe localBroker;

    bool bridging = false;
    Socks5Client socks;
    PicoMQTT::Client *remote = nullptr;

    void handleLocalMessage(const char *topic, const char *payload);
    void forwardThroughTor(const char *payload);
};