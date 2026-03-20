from fastapi_mqtt import FastMQTT, MQTTConfig
from pathlib import Path
import os

# ============= MQTT Configuration (Buổi 2-3) =============
# TLS configuration cho MQTT Broker
CERT_DIR = Path(__file__).resolve().parent / "mqtt_certs"

# Using TLS on port 8883 (recommended) or plain on 1884 for testing
USE_TLS = os.getenv("MQTT_USE_TLS", "false").lower() == "true"

if USE_TLS:
    # TLS mode (port 8883) - Buổi 3
    mqtt_config = MQTTConfig(
        host=os.getenv("MQTT_HOST", "127.0.0.1"),
        port=int(os.getenv("MQTT_PORT", 8883)),
        username=os.getenv("MQTT_USER", "backend"),
        password=os.getenv("MQTT_PASS", "backend123"),
        keepalive=60,
        will_message_topic="backend/status",
        will_message_payload="OFFLINE",
        will_delay_interval=10,
        # TLS settings
        tls_version="3",  # TLS 1.2
        ca_certs=str(CERT_DIR / "ca.crt"),
        certfile=str(CERT_DIR / "client.crt"),
        keyfile=str(CERT_DIR / "client.key"),
        cert_reqs="CERT_REQUIRED",
        tls_insecure=False,
        protocol="MQTTv311",
    )
    print("✓ MQTT configured with TLS (port 8883)")
else:
    # Plain mode (port 1884) - for testing/internal
    mqtt_config = MQTTConfig(
        host=os.getenv("MQTT_HOST", "127.0.0.1"),
        port=int(os.getenv("MQTT_PORT", 1884)),
        username=os.getenv("MQTT_USER", "backend"),
        password=os.getenv("MQTT_PASS", "backend123"),
        keepalive=60,
        will_message_topic="backend/status",
        will_message_payload="OFFLINE",
        will_delay_interval=10,
        protocol="MQTTv311",
    )
    print("✓ MQTT configured with username/password (port 1884)")

fast_mqtt = FastMQTT(config=mqtt_config)
