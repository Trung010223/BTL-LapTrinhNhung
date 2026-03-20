#!/bin/bash
# Script tạo MQTT TLS certificates (Buổi 3)
# Chạy: bash generate_mqtt_certs.sh

set -e

CERT_DIR="mqtt_certs"
mkdir -p $CERT_DIR

# 1. Tạo CA key
openssl genrsa -out $CERT_DIR/ca.key 2048
echo "✓ Created CA key"

# 2. Tạo CA certificate
openssl req -new -x509 -days 3650 -key $CERT_DIR/ca.key -out $CERT_DIR/ca.crt \
  -subj "/CN=MQTT-CA/O=BTL/C=VN"
echo "✓ Created CA certificate"

# 3. Tạo server key
openssl genrsa -out $CERT_DIR/server.key 2048
echo "✓ Created server key"

# 4. Tạo server CSR
openssl req -new -key $CERT_DIR/server.key -out $CERT_DIR/server.csr \
  -subj "/CN=127.0.0.1/O=BTL/C=VN"
echo "✓ Created server CSR"

# 5. Sign server certificate
openssl x509 -req -days 365 -in $CERT_DIR/server.csr \
  -CA $CERT_DIR/ca.crt -CAkey $CERT_DIR/ca.key \
  -CAcreateserial -out $CERT_DIR/server.crt
echo "✓ Created server certificate"

# 6. Tạo client key
openssl genrsa -out $CERT_DIR/client.key 2048
echo "✓ Created client key"

# 7. Tạo client CSR
openssl req -new -key $CERT_DIR/client.key -out $CERT_DIR/client.csr \
  -subj "/CN=mqtt-client/O=BTL/C=VN"
echo "✓ Created client CSR"

# 8. Sign client certificate
openssl x509 -req -days 365 -in $CERT_DIR/client.csr \
  -CA $CERT_DIR/ca.crt -CAkey $CERT_DIR/ca.key \
  -CAcreateserial -out $CERT_DIR/client.crt
echo "✓ Created client certificate"

echo ""
echo "========================================="
echo "✓ TLS Certificates created successfully!"
echo "========================================="
echo "Files created in: $CERT_DIR/"
ls -lah $CERT_DIR/
