Objetivo: provar que o `toresp32` compila e abre um circuito Tor no hardware
escolhido, medindo tempo de bootstrap e uso de memoria. Esta e a fase de maior
risco; valide-a antes de investir nas demais.

## Hardware necessario

- ESP32 **com PSRAM**: ESP32-WROVER, ESP32-S2 ou ESP32-S3.
  Um ESP32-WROOM comum (sem PSRAM) muito provavelmente nao roda o Tor.
- Cabo USB e, idealmente, um cartao microSD ou particao SPIFFS/LittleFS
  (o `toresp32` salva consenso/cache em filesystem).

## Passos

1. Instale VSCode + PlatformIO + toolchain ESP-IDF.
2. Clone o projeto:
bash git clone https://github.com/briand-hub/toresp32 cd toresp32
3. Abra com PlatformIO. O alvo padrao e a WEMOS LOLIN D32; ajuste o `board`
   para a sua placa com PSRAM e habilite SPI RAM no `menuconfig`/`platformio.ini`
   (`-DBOARD_HAS_PSRAM`).
4. Configure as credenciais Wi-Fi conforme a Wiki do projeto.
5. Defina a porta do proxy SOCKS5 para **9050** (a mesma usada por este
   firmware em `src/config.h`).
6. Compile, grave e abra o monitor serial.

## Criterios de sucesso

- O log indica circuitos prontos ("ready circuits").
- O proxy SOCKS5 aceita conexoes (teste a partir de um PC na mesma rede,
  apontando `curl --socks5-hostname <ip-esp32>:9050 https://check.torproject.org`).
- Registre:
  - tempo de bootstrap ate o primeiro circuito pronto;
  - RAM interna livre e PSRAM livre apos o bootstrap;
  - estabilidade ao longo de ~30 min.

## Observacoes para o TCC

- `toresp32` e um projeto educacional (GPLv3), sem auditoria de seguranca.
  Cite isso como limitacao (doc.md, secao 11.1).
- A performance e limitada por design (240 MHz, RAM apertada). Documente os
  numeros reais medidos; eles sustentam a comparacao com o gateway externo.
- Se o bootstrap nao concluir ou a memoria estourar, a conclusao tecnica
  (valida para o TCC) e que a abordagem "Tor no ESP32" nao e viavel no
  hardware testado, reforcando a recomendacao do gateway externo.