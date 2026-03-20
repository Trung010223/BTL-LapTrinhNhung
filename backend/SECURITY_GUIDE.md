# Hướng Dẫn Bảo Mật Hệ Thống (Buổi 1-5)

## Tóm Tắt An Toàn

**Buổi 1**: Mã hóa Flash, TLS, AES, Watchdog, Failsafe
**Buổi 2**: MQTT Authentication, JWT Token Backend
**Buổi 3**: TLS MQTT, AES-CBC, Timestamp + Nonce, AES-GCM
**Buổi 4**: Hard-code Preferences, Secure Firmware, AES-128-ECB
**Buổi 5**: DTLS, HMAC-SHA256, JWT 24h, Rate Limiting, Input Validation

---

## 1. CẤU HÌNH BAN ĐẦU

### 1.1 Cài Đặt Packages

```bash
cd backend
pip install -r requirements.txt
```

**Packages mới thêm:**

- `cryptography>=41.0` - Mã hóa AES
- `pyjwt>=2.8` - JWT Token
- `slowapi>=0.1.8` - Rate Limiting

---

## 2. MQTT TLS + AUTHENTICATION (Buổi 2-3)

### 2.1 Sinh TLS Certificates

**Trên Linux/Mac:**

```bash
cd backend
bash generate_mqtt_certs.sh
```

**Trên Windows (sử dụng WSL hoặc Git Bash):**

```bash
cd backend
bash generate_mqtt_certs.sh
```

Hoặc tạo thủ công bằng OpenSSL:

```bash
# Tạo CA key
openssl genrsa -out mqtt_certs/ca.key 2048

# Tạo CA certificate
openssl req -new -x509 -days 3650 -key mqtt_certs/ca.key -out mqtt_certs/ca.crt \
  -subj "/CN=MQTT-CA/O=BTL/C=VN"

# Tạo server key
openssl genrsa -out mqtt_certs/server.key 2048

# Tạo server CSR
openssl req -new -key mqtt_certs/server.key -out mqtt_certs/server.csr \
  -subj "/CN=127.0.0.1/O=BTL/C=VN"

# Sign server certificate
openssl x509 -req -days 365 -in mqtt_certs/server.csr \
  -CA mqtt_certs/ca.crt -CAkey mqtt_certs/ca.key \
  -CAcreateserial -out mqtt_certs/server.crt

# Tạo client key
openssl genrsa -out mqtt_certs/client.key 2048

# Tạo client CSR
openssl req -new -key mqtt_certs/client.key -out mqtt_certs/client.csr \
  -subj "/CN=mqtt-client/O=BTL/C=VN"

# Sign client certificate
openssl x509 -req -days 365 -in mqtt_certs/client.csr \
  -CA mqtt_certs/ca.crt -CAkey mqtt_certs/ca.key \
  -CAcreateserial -out mqtt_certs/client.crt
```

### 2.2 Sinh Mosquitto Password File

```bash
# Install mosquitto-clients
# Ubuntu/Debian: sudo apt install mosquitto-clients
# macOS: brew install mosquitto
# Windows: choco install mosquitto-clients

# Tạo password file
mosquitto_passwd -c mosquitto_passwords.txt admin
mosquitto_passwd -a mosquitto_passwords.txt user
mosquitto_passwd -a mosquitto_passwords.txt backend
mosquitto_passwd -a mosquitto_passwords.txt esp32
```

**Hoặc dùng Mosquitto tool:**

```bash
cd backend

# Tạo file
mosquitto_passwd -b -c mosquitto_passwords.txt admin admin123
mosquitto_passwd -b -a mosquitto_passwords.txt user user123
mosquitto_passwd -b -a mosquitto_passwords.txt backend backend123
mosquitto_passwd -b -a mosquitto_passwords.txt esp32 esp32123
```

### 2.3 Cấu Hình Mosquitto

**Sử dụng config bảo mật:**

```bash
# Thay thế config cũ
cp mosquitto_tls.conf C:\\Program\ Files\\mosquitto\\mosquitto.conf

# Hoặc chỉnh sửa C:\\Program Files\\mosquitto\\mosquitto.conf:
# - Cpy nội dung mosquitto_tls.conf
# - Include files: mqtt_certs/, mosquitto_passwords.txt, mosquitto_acl.conf
```

**Khởi động Mosquitto:**

```bash
# Windows (Admin terminal):
net start Mosquitto

# Hoặc chạy trực tiếp:
"C:\Program Files\mosquitto\mosquitto.exe" -c "C:\Program Files\mosquitto\mosquitto.conf" -v
```

### 2.4 Test MQTT Connection

**Test TLS (port 8883):**

```bash
mosquitto_pub \
  -h 127.0.0.1 \
  -p 8883 \
  -u backend \
  -P backend123 \
  --cafile mqtt_certs/ca.crt \
  --cert mqtt_certs/client.crt \
  --key mqtt_certs/client.key \
  -t test/tls \
  -m "TLS Test Success"

mosquitto_sub \
  -h 127.0.0.1 \
  -p 8883 \
  -u backend \
  -P backend123 \
  --cafile mqtt_certs/ca.crt \
  --cert mqtt_certs/client.crt \
  --key mqtt_certs/client.key \
  -t "test/tls"
```

**Test Plain (port 1884 - internal):**

```bash
mosquitto_pub -h 127.0.0.1 -p 1884 -u backend -P backend123 -t test/plain -m "Plain Test"
mosquitto_sub -h 127.0.0.1 -p 1884 -u backend -P backend123 -t "test/plain"
```

---

## 3. BACKEND SECURITY (Buổi 2-5)

### 3.1 Lấy JWT Token

```powershell
# PowerShell
$login = @{
    username = "admin"
    password = "admin123"
} | ConvertTo-Json

$response = Invoke-RestMethod `
  -Uri "http://localhost:8000/auth/login" `
  -Method POST `
  -ContentType "application/json" `
  -Body $login

$token = $response.access_token
Write-Host "Token: $token"
```

### 3.2 Sử Dụng Token trong Requests

```powershell
# Gọi API với JWT token
$headers = @{
    "Authorization" = "bearer $token"
}

$cmd = @{
    speed = 100
    direction = 0
    lift = 0
    stop = $false
} | ConvertTo-Json

Invoke-RestMethod `
  -Uri "http://localhost:8000/v/command" `
  -Method POST `
  -ContentType "application/json" `
  -Headers $headers `
  -Body $cmd
```

### 3.3 Khởi Động Backend Bảo Mật

```bash
cd backend

# Sử dụng MQTT TLS
export MQTT_USE_TLS=true
export MQTT_HOST=127.0.0.1
export MQTT_PORT=8883
export MQTT_USER=backend
export MQTT_PASS=backend123

# Hoặc MQTT Plain (internal)
export MQTT_USE_TLS=false
export MQTT_HOST=127.0.0.1
export MQTT_PORT=1884
export MQTT_USER=backend
export MQTT_PASS=backend123

python main.py
```

**Trên Windows:**

```powershell
$env:MQTT_USE_TLS = "false"
$env:MQTT_HOST = "127.0.0.1"
$env:MQTT_PORT = "1884"
$env:MQTT_USER = "backend"
$env:MQTT_PASS = "backend123"

python main.py
```

---

## 4. ESP32 FIRMWARE INTEGRATION (Buổi 1-4)

### 4.1 Cổng MQTT Mẫu (C++)

**Firmware cần kết nối với MQTT Auth:**

```cpp
#include <PubSubClient.h>

// MQTT Credentials (Buổi 2)
const char* mqttUser = "esp32";
const char* mqttPass = "esp32123";
const char* mqttHost = "192.168.100.248";
const int mqttPort = 1884;  // or 8883 for TLS

WiFiClient espClient;
PubSubClient client(espClient);

void connectMQTT() {
    if (!client.connect("esp32-node", mqttUser, mqttPass)) {
        Serial.println("[MQTT] Connection failed");
        return;
    }
    Serial.println("[MQTT] Connected");
}

void setup() {
    client.setServer(mqttHost, mqttPort);
    connectMQTT();
}

void loop() {
    if (!client.connected()) {
        connectMQTT();
    }
    client.loop();
}
```

### 4.2 Payload AES Encryption (Optional - Buổi 3)

```cpp
// Include mbedtls libraries
#include "mbedtls/aes.h"
#include "mbedtls/md.h"

// Encrypt trước khi publish
void publishSecure(const char* topic, const char* data) {
    // AES-256 encrypt (Buổi 3)
    // Thêm nonce + timestamp (Buổi 3)
    // Tính HMAC-SHA256 (Buổi 5)
    // Publish encrypted payload
}
```

---

## 5. SECURITY BEST PRACTICES (Buổi 1-5)

### 5.1 Hardcoded Keys (Buổi 4)

**Hiện tại (Development):**

```python
# backend/security.py
HMAC_KEY = b"your-secret-hmac-key-32-bytes-12345"
AES_KEY = b"your-secret-aes-key-32-bytes-1234!"
JWT_SECRET = "your-jwt-secret-key-change-me-in-production"
```

**Production (Buổi 4):**

```python
import os
from dotenv import load_dotenv

load_dotenv()  # Load from .env file

HMAC_KEY = os.getenv("HMAC_KEY", "").encode()
AES_KEY = os.getenv("AES_KEY", "").encode()
JWT_SECRET = os.getenv("JWT_SECRET")
```

**Tạo .env file (không commit lên Git):**

```
HMAC_KEY=your-32-byte-secret-here
AES_KEY=your-32-byte-secret-here
JWT_SECRET=your-jwt-secret-production
```

### 5.2 Watchdog & Failsafe (Buổi 1)

**ESP32 Watchdog:**

```cpp
#include <esp_task_wdt.h>

void setup() {
    // Enable watchdog (5 second timeout)
    esp_task_wdt_init(5, true);
    esp_task_wdt_add(NULL);
}

void loop() {
    // Feed watchdog
    esp_task_wdt_reset();

    // Your code...
}
```

### 5.3 Firmware Security (Buổi 4)

**Signed Firmware Updates:**

```bash
# Sign firmware
espsecure.py sign_data --version 2 --private-key private-key.pem firmware.bin

# Verify in code
esp_secure_boot_verify_ota_digest_begin()
```

### 5.4 Rate Limiting (Buổi 5)

Backend đã gồm rate limiter:

- API: 100 requests/60s
- MQTT: 1000 requests/60s

```python
# backend/security.py
api_rate_limiter = RateLimiter(max_requests=100, window_seconds=60)

# Validate before endpoint
if not api_rate_limiter.is_allowed(client_ip):
    raise HTTPException(status_code=429, detail="Too many requests")
```

### 5.5 Input Validation (Buổi 5)

```python
# Tất cả input validate trước process
InputValidator.validate_string(username, max_length=50)
InputValidator.validate_number(speed, min_val=-255, max_val=255)
InputValidator.validate_json(data, required_keys=["cmd", "speed"])
```

---

## 6. TESTING & VERIFICATION

### 6.1 Test Full Pipeline

```powershell
# 1. Get JWT Token
$token = (Invoke-RestMethod `
  -Uri "http://localhost:8000/auth/login" `
  -Method POST `
  -Headers @{"Content-Type"="application/json"} `
  -Body '{"username":"admin","password":"admin123"}'
).access_token

# 2. Send Vehicle Command
$headers = @{"Authorization" = "bearer $token"}
Invoke-RestMethod `
  -Uri "http://localhost:8000/v/command" `
  -Method POST `
  -Headers $headers `
  -ContentType "application/json" `
  -Body '{"speed":100,"direction":0,"lift":0,"stop":false}'

# 3. Check status
Invoke-RestMethod `
  -Uri "http://localhost:8000/v/state" `
  -Method GET `
  -Headers $headers
```

### 6.2 Test MQTT Encryption

```python
from security import AESCrypto, MessageSecurity

# Test AES
plaintext = "Hello Secure World"
encrypted = AESCrypto.encrypt(plaintext)
decrypted = AESCrypto.decrypt(encrypted)
print(f"Original: {plaintext}")
print(f"Decrypted: {decrypted}")
assert plaintext == decrypted

# Test Message Security
data = {"cmd": "vehicle", "speed": 100}
secure_msg = MessageSecurity.create_secure_payload(data)
verified = MessageSecurity.verify_secure_payload(secure_msg)
print(f"Verified: {verified}")
```

---

## 7. DEPLOYMENT CHECKLIST

- [ ] Generate TLS certificates (mqtt_certs/)
- [ ] Create Mosquitto password file
- [ ] Update Mosquitto config (TLS + ACL)
- [ ] Update MQTT credentials in backend
- [ ] Test MQTT connection (TLS + Auth)
- [ ] Update backend .env file (production keys)
- [ ] Test JWT token endpoint
- [ ] Test secure API endpoints
- [ ] Verify rate limiting works
- [ ] Test input validation
- [ ] Enable firmware security on ESP32
- [ ] Document user credentials (admin/pass)
- [ ] Setup monitoring/alerting

---

## 8. TROUBLESHOOTING

### MQTT TLS Connection Failed

```bash
# Test certificate
openssl s_client -connect 127.0.0.1:8883 -CAfile mqtt_certs/ca.crt

# Check Mosquitto logs
tail -f log/mosquitto.log

# Verify passwords
mosquitto_passwd -c mosquitto_passwords.txt admin
mosquitto_passwd -v mosquitto_passwords.txt
```

### JWT Token Invalid

```python
# Verify token
from security import JWTAuth

token = "your-token-here"
payload = JWTAuth.verify_token(token)
if payload:
    print(f"Valid: {payload}")
else:
    print("Invalid or expired")
```

### Rate Limit Exceeded

- Cách tăng giới hạn (Buổi 5 tunable):
  ```python
  api_rate_limiter = RateLimiter(max_requests=200, window_seconds=60)
  ```

---

## 9. REFERENCES

- [Mosquitto TLS](https://mosquitto.org/man/mosquitto-tls-7.html)
- [JWT Security](https://tools.ietf.org/html/rfc7519)
- [AES Encryption](https://docs.cryptography.io/en/latest/hazmat/)
- [ESP32 Security](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/)
- [OWASP Best Practices](https://owasp.org/www-project-top-ten/)

---

**Version**: 2.0.0 - Security Enhanced (Buổi 1-5)
**Last Updated**: 2026-03-19
