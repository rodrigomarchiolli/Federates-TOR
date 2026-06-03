#!/usr/bin/env python3
"""Publicador de teste para o broker MQTT local do ESP32.

Simula um sensor IoMT enviando payloads ECG/EMG/EDA no formato do doc.md
(secao 4.1) para o broker que roda no ESP32. Use para validar as Fases 2 e 4
sem precisar de um segundo dispositivo.

Exemplo:
    python publish_test.py --host 192.168.0.50 --count 20 --interval 2
"""

import argparse
import json
import random
import time
from datetime import datetime, timezone

try:
    import paho.mqtt.client as mqtt
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "Instale a dependencia: pip install paho-mqtt"
    ) from exc


def build_payload(pseudonym: str, samples: int) -> dict:
    return {
        "device_id": pseudonym,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "ecg": [round(0.10 + random.uniform(0, 0.05), 3) for _ in range(samples)],
        "emg": [round(0.45 + random.uniform(0, 0.10), 3) for _ in range(samples)],
        "eda": [round(1.80 + random.uniform(0, 0.08), 3) for _ in range(samples)],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, help="IP do ESP32 (broker local)")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--topic", default="esp32/data")
    parser.add_argument("--pseudonym", default="esp32_pseudo_001")
    parser.add_argument("--samples", type=int, default=4, help="amostras por sinal")
    parser.add_argument("--count", type=int, default=10, help="qtd de mensagens")
    parser.add_argument("--interval", type=float, default=2.0, help="segundos")
    args = parser.parse_args()

    client = mqtt.Client()
    client.connect(args.host, args.port, keepalive=30)
    client.loop_start()

    try:
        for i in range(args.count):
            payload = json.dumps(build_payload(args.pseudonym, args.samples))
            result = client.publish(args.topic, payload, qos=0)
            result.wait_for_publish()
            print(f"[{i + 1}/{args.count}] -> {args.topic} ({len(payload)} bytes)")
            time.sleep(args.interval)
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()