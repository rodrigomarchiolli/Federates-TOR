# Fase 5 - Medicao e comparacao para a monografia

Objetivo: coletar evidencia quantitativa do firmware integrado e compara-la com
o cenario de referencia "gateway externo" descrito no doc.md (secoes 8 e 13).

## Metricas coletadas pelo firmware

O firmware imprime no Serial, em CSV, a cada `METRICS_REPORT_INTERVAL_MS`:

| Coluna           | Significado                                              |
| ---------------- | -------------------------------------------------------- |
| `uptime_s`       | tempo de execucao em segundos                            |
| `recv`           | mensagens recebidas no broker local                      |
| `fwd_ok`         | mensagens reencaminhadas com sucesso via Tor             |
| `fwd_fail`       | falhas de reencaminhamento                               |
| `loss_pct`       | percentual de perda (fwd_fail / total)                   |
| `avg/min/max_lat_ms` | latencia de reencaminhamento (local -> publish)      |
| `tor_attempts`   | tentativas de conexao ao broker remoto via Tor           |
| `tor_ok`         | conexoes Tor bem-sucedidas                               |
| `avg_connect_ms` | tempo medio de estabelecimento de circuito/conexao       |
| `free_heap`      | RAM interna livre (bytes)                                |
| `free_psram`     | PSRAM livre (bytes)                                      |

## Procedimento sugerido

1. Capture o Serial em arquivo:
bash pio device monitor -e integrated > medicao_tor_esp32.csv
2. Rode o publicador de teste com carga controlada:
bash python tools/publish_test.py --host <ip-esp32> --count 100 --interval 1
3. Repita variando `--interval` (1s, 2s, 5s) e `--samples` (4, 16, 64) para
   estressar memoria e latencia.

## Cenario de comparacao (referencia)

Para o cenario "gateway externo" do doc.md, replique o mesmo teste com:

- ESP32 publicando em um Mosquitto local (sem Tor embarcado);
- um gateway Python no PC/Raspberry Pi encaminhando via SOCKS5 do Tor.

Compare ao menos:

- latencia media e maxima de entrega ponta a ponta;
- taxa de perda de mensagens;
- estabilidade (tempo ate primeira falha / reconexoes);
- consumo de memoria no ESP32 (com vs. sem Tor embarcado).

## Tabela de resultados (preencher)

| Metrica              | Tor no ESP32 | Gateway externo |
| -------------------- | ------------ | --------------- |
| Bootstrap Tor (s)    |              | n/a             |
| Latencia media (ms)  |              |                 |
| Latencia max (ms)    |              |                 |
| Perda (%)            |              |                 |
| Heap livre (KB)      |              |                 |
| PSRAM livre (KB)     |              | n/a             |
| Reconexoes / hora    |              |                 |