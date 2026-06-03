#pragma once

#include <Arduino.h>

// Coleta metricas simples para a avaliacao do TCP (latencia, perda e memoria).
// Os numeros sao impressos periodicamente no Serial em formato CSV, o que
// facilita exportar e comparar o cenario "Tor no ESP32" com o cenario
// "gateway externo" descrito no doc.md.
class Metrics {
public:
    void begin(uint32_t reportIntervalMs = 10000);

    void onLocalReceived(size_t payloadBytes);
    void onForwardSuccess(uint32_t latencyMs);
    void onForwardFailure();
    void onTorConnectAttempt();
    void onTorConnectSuccess(uint32_t connectMs);

    // Imprime o relatorio se ja passou o intervalo configurado.
    void maybeReport();
    // Forca a impressao imediata do relatorio.
    void report();

private:
    uint32_t reportIntervalMs = 10000;
    uint32_t lastReportMs = 0;
    bool headerPrinted = false;

    uint32_t localReceived = 0;
    uint32_t forwardOk = 0;
    uint32_t forwardFail = 0;
    uint32_t torAttempts = 0;
    uint32_t torConnects = 0;

    uint64_t totalBytes = 0;
    uint64_t latencySumMs = 0;
    uint32_t latencyMaxMs = 0;
    uint32_t latencyMinMs = UINT32_MAX;
    uint64_t connectSumMs = 0;
};