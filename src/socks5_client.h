#pragma once

#include <Arduino.h>
#include <Client.h>
#include <IPAddress.h>
#include <WiFiClient.h>

// Socks5Client adapta uma conexao TCP comum (WiFiClient) para sair atraves de
// um proxy SOCKS5 local. No contexto deste projeto, o proxy SOCKS5 e o
// toresp32 rodando no proprio ESP32 (por padrao 127.0.0.1:9050), de modo que
// todo trafego de saida atravessa a rede Tor.
//
// A classe implementa a interface Arduino ::Client, o que permite passa-la
// diretamente para PicoMQTT::Client como transporte customizado. Como o
// metodo connect(const char*, uint16_t) usa o ATYP de dominio (0x03) do
// SOCKS5, enderecos .onion sao resolvidos pelo proprio Tor, sem DNS local.
class Socks5Client : public Client {
public:
    Socks5Client(const IPAddress &proxyHost = IPAddress(127, 0, 0, 1),
                 uint16_t proxyPort = 9050,
                 const char *proxyUser = nullptr,
                 const char *proxyPass = nullptr);

    // Conexao por hostname: usa ATYP=dominio (necessario para .onion).
    int connect(const char *host, uint16_t port) override;
    // Conexao por IP: usa ATYP=IPv4.
    int connect(IPAddress ip, uint16_t port) override;

    size_t write(uint8_t b) override;
    size_t write(const uint8_t *buf, size_t size) override;
    int available() override;
    int read() override;
    int read(uint8_t *buf, size_t size) override;
    int peek() override;
    void flush() override;
    void stop() override;
    uint8_t connected() override;
    operator bool() override { return connected(); }

    // Timeout (ms) para o handshake SOCKS5 e para a conexao TCP ao proxy.
    void setHandshakeTimeout(uint32_t ms) { handshakeTimeoutMs = ms; }

    // Ultimo codigo de erro SOCKS5 (REP) ou negativo para erros locais.
    int lastError() const { return lastErr; }

private:
    enum LocalError : int {
        ERR_NONE = 0,
        ERR_PROXY_TCP = -1,
        ERR_METHOD = -2,
        ERR_AUTH = -3,
        ERR_REPLY = -4,
        ERR_TIMEOUT = -5,
        ERR_HOST_TOO_LONG = -6,
    };

    WiFiClient underlying;
    IPAddress proxyHost;
    uint16_t proxyPort;
    const char *proxyUser;
    const char *proxyPass;
    uint32_t handshakeTimeoutMs = 30000;
    int lastErr = ERR_NONE;

    bool negotiateMethod();
    bool authenticate();
    bool sendConnectRequest(const uint8_t *addr, uint8_t addrLen,
                            uint8_t atyp, uint16_t port);
    bool readReply();
    bool readExact(uint8_t *buf, size_t len, uint32_t deadline);
};