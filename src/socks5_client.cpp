#include "socks5_client.h"

namespace {
constexpr uint8_t SOCKS_VERSION = 0x05;
constexpr uint8_t METHOD_NO_AUTH = 0x00;
constexpr uint8_t METHOD_USERPASS = 0x02;
constexpr uint8_t METHOD_NONE_ACCEPTABLE = 0xFF;
constexpr uint8_t CMD_CONNECT = 0x01;
constexpr uint8_t ATYP_IPV4 = 0x01;
constexpr uint8_t ATYP_DOMAIN = 0x03;
constexpr uint8_t RSV = 0x00;
constexpr uint8_t AUTH_VERSION = 0x01;
constexpr uint8_t REP_SUCCEEDED = 0x00;
}  // namespace

Socks5Client::Socks5Client(const IPAddress &proxyHost, uint16_t proxyPort,
                           const char *proxyUser, const char *proxyPass)
    : proxyHost(proxyHost),
      proxyPort(proxyPort),
      proxyUser(proxyUser),
      proxyPass(proxyPass) {}

bool Socks5Client::readExact(uint8_t *buf, size_t len, uint32_t deadline) {
    size_t got = 0;
    while (got < len) {
        if ((int32_t)(millis() - deadline) >= 0) {
            lastErr = ERR_TIMEOUT;
            return false;
        }
        if (!underlying.connected() && underlying.available() == 0) {
            lastErr = ERR_PROXY_TCP;
            return false;
        }
        int n = underlying.read(buf + got, len - got);
        if (n > 0) {
            got += (size_t)n;
        } else {
            delay(1);
        }
    }
    return true;
}

bool Socks5Client::negotiateMethod() {
    const bool useAuth = (proxyUser != nullptr && proxyPass != nullptr);
    uint8_t greeting[4];
    size_t n = 0;
    greeting[n++] = SOCKS_VERSION;
    if (useAuth) {
        greeting[n++] = 0x02;  // NMETHODS
        greeting[n++] = METHOD_NO_AUTH;
        greeting[n++] = METHOD_USERPASS;
    } else {
        greeting[n++] = 0x01;  // NMETHODS
        greeting[n++] = METHOD_NO_AUTH;
    }
    if (underlying.write(greeting, n) != n) {
        lastErr = ERR_PROXY_TCP;
        return false;
    }

    uint8_t resp[2];
    if (!readExact(resp, 2, millis() + handshakeTimeoutMs)) {
        return false;
    }
    if (resp[0] != SOCKS_VERSION || resp[1] == METHOD_NONE_ACCEPTABLE) {
        lastErr = ERR_METHOD;
        return false;
    }
    if (resp[1] == METHOD_USERPASS) {
        return authenticate();
    }
    if (resp[1] != METHOD_NO_AUTH) {
        lastErr = ERR_METHOD;
        return false;
    }
    return true;
}

bool Socks5Client::authenticate() {
    if (proxyUser == nullptr || proxyPass == nullptr) {
        lastErr = ERR_AUTH;
        return false;
    }
    const size_t ulen = strlen(proxyUser);
    const size_t plen = strlen(proxyPass);
    if (ulen > 255 || plen > 255) {
        lastErr = ERR_AUTH;
        return false;
    }
    // RFC 1929: VER | ULEN | UNAME | PLEN | PASSWD
    underlying.write(AUTH_VERSION);
    underlying.write((uint8_t)ulen);
    underlying.write((const uint8_t *)proxyUser, ulen);
    underlying.write((uint8_t)plen);
    underlying.write((const uint8_t *)proxyPass, plen);

    uint8_t resp[2];
    if (!readExact(resp, 2, millis() + handshakeTimeoutMs)) {
        return false;
    }
    if (resp[1] != 0x00) {
        lastErr = ERR_AUTH;
        return false;
    }
    return true;
}

bool Socks5Client::sendConnectRequest(const uint8_t *addr, uint8_t addrLen,
                                      uint8_t atyp, uint16_t port) {
    // VER | CMD | RSV | ATYP | DST.ADDR | DST.PORT
    underlying.write(SOCKS_VERSION);
    underlying.write(CMD_CONNECT);
    underlying.write(RSV);
    underlying.write(atyp);
    if (atyp == ATYP_DOMAIN) {
        underlying.write(addrLen);  // tamanho do dominio precede os bytes
    }
    if (underlying.write(addr, addrLen) != addrLen) {
        lastErr = ERR_PROXY_TCP;
        return false;
    }
    underlying.write((uint8_t)(port >> 8));
    underlying.write((uint8_t)(port & 0xFF));
    return true;
}

bool Socks5Client::readReply() {
    uint32_t deadline = millis() + handshakeTimeoutMs;
    uint8_t head[4];  // VER | REP | RSV | ATYP
    if (!readExact(head, 4, deadline)) {
        return false;
    }
    if (head[0] != SOCKS_VERSION) {
        lastErr = ERR_REPLY;
        return false;
    }
    if (head[1] != REP_SUCCEEDED) {
        lastErr = head[1];  // expoe o REP do SOCKS5 (1-8)
        return false;
    }
    // Consome BND.ADDR conforme o ATYP retornado e BND.PORT (2 bytes).
    size_t addrLen = 0;
    switch (head[3]) {
        case ATYP_IPV4:
            addrLen = 4;
            break;
        case ATYP_DOMAIN: {
            uint8_t len;
            if (!readExact(&len, 1, deadline)) return false;
            addrLen = len;
            break;
        }
        case 0x04:  // IPv6
            addrLen = 16;
            break;
        default:
            lastErr = ERR_REPLY;
            return false;
    }
    uint8_t scratch[16];
    while (addrLen > 0) {
        size_t chunk = addrLen > sizeof(scratch) ? sizeof(scratch) : addrLen;
        if (!readExact(scratch, chunk, deadline)) return false;
        addrLen -= chunk;
    }
    if (!readExact(scratch, 2, deadline)) return false;  // BND.PORT
    return true;
}

int Socks5Client::connect(const char *host, uint16_t port) {
    lastErr = ERR_NONE;
    const size_t hostLen = strlen(host);
    if (hostLen > 255) {
        lastErr = ERR_HOST_TOO_LONG;
        return 0;
    }
    if (!underlying.connect(proxyHost, proxyPort,
                            (int32_t)handshakeTimeoutMs)) {
        lastErr = ERR_PROXY_TCP;
        return 0;
    }
    underlying.setNoDelay(true);
    if (!negotiateMethod()) {
        underlying.stop();
        return 0;
    }
    if (!sendConnectRequest((const uint8_t *)host, (uint8_t)hostLen,
                            ATYP_DOMAIN, port)) {
        underlying.stop();
        return 0;
    }
    if (!readReply()) {
        underlying.stop();
        return 0;
    }
    return 1;
}

int Socks5Client::connect(IPAddress ip, uint16_t port) {
    lastErr = ERR_NONE;
    if (!underlying.connect(proxyHost, proxyPort,
                            (int32_t)handshakeTimeoutMs)) {
        lastErr = ERR_PROXY_TCP;
        return 0;
    }
    underlying.setNoDelay(true);
    if (!negotiateMethod()) {
        underlying.stop();
        return 0;
    }
    uint8_t addr[4] = {ip[0], ip[1], ip[2], ip[3]};
    if (!sendConnectRequest(addr, 4, ATYP_IPV4, port)) {
        underlying.stop();
        return 0;
    }
    if (!readReply()) {
        underlying.stop();
        return 0;
    }
    return 1;
}

size_t Socks5Client::write(uint8_t b) { return underlying.write(b); }

size_t Socks5Client::write(const uint8_t *buf, size_t size) {
    return underlying.write(buf, size);
}

int Socks5Client::available() { return underlying.available(); }

int Socks5Client::read() { return underlying.read(); }

int Socks5Client::read(uint8_t *buf, size_t size) {
    return underlying.read(buf, size);
}

int Socks5Client::peek() { return underlying.peek(); }

void Socks5Client::flush() { underlying.flush(); }

void Socks5Client::stop() { underlying.stop(); }

uint8_t Socks5Client::connected() { return underlying.connected(); }