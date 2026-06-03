#include "metrics.h"

#if defined(ESP32)
#include <esp_heap_caps.h>
#endif

void Metrics::begin(uint32_t reportIntervalMs) {
    this->reportIntervalMs = reportIntervalMs;
    lastReportMs = millis();
}

void Metrics::onLocalReceived(size_t payloadBytes) {
    localReceived++;
    totalBytes += payloadBytes;
}

void Metrics::onForwardSuccess(uint32_t latencyMs) {
    forwardOk++;
    latencySumMs += latencyMs;
    if (latencyMs > latencyMaxMs) latencyMaxMs = latencyMs;
    if (latencyMs < latencyMinMs) latencyMinMs = latencyMs;
}

void Metrics::onForwardFailure() { forwardFail++; }

void Metrics::onTorConnectAttempt() { torAttempts++; }

void Metrics::onTorConnectSuccess(uint32_t connectMs) {
    torConnects++;
    connectSumMs += connectMs;
}

void Metrics::maybeReport() {
    if ((uint32_t)(millis() - lastReportMs) >= reportIntervalMs) {
        report();
    }
}

void Metrics::report() {
    lastReportMs = millis();

    if (!headerPrinted) {
        Serial.println(F("# METRICS CSV"));
        Serial.println(F(
            "uptime_s,recv,fwd_ok,fwd_fail,loss_pct,avg_lat_ms,min_lat_ms,"
            "max_lat_ms,tor_attempts,tor_ok,avg_connect_ms,free_heap,"
            "free_psram"));
        headerPrinted = true;
    }

    const uint32_t totalFwd = forwardOk + forwardFail;
    const float lossPct =
        totalFwd > 0 ? (100.0f * (float)forwardFail / (float)totalFwd) : 0.0f;
    const uint32_t avgLat =
        forwardOk > 0 ? (uint32_t)(latencySumMs / forwardOk) : 0;
    const uint32_t minLat = (latencyMinMs == UINT32_MAX) ? 0 : latencyMinMs;
    const uint32_t avgConnect =
        torConnects > 0 ? (uint32_t)(connectSumMs / torConnects) : 0;

    uint32_t freeHeap = 0;
    uint32_t freePsram = 0;
#if defined(ESP32)
    freeHeap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#endif

    Serial.printf("%lu,%lu,%lu,%lu,%.2f,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
                  (unsigned long)(millis() / 1000), (unsigned long)localReceived,
                  (unsigned long)forwardOk, (unsigned long)forwardFail, lossPct,
                  (unsigned long)avgLat, (unsigned long)minLat,
                  (unsigned long)latencyMaxMs, (unsigned long)torAttempts,
                  (unsigned long)torConnects, (unsigned long)avgConnect,
                  (unsigned long)freeHeap, (unsigned long)freePsram);
}