# BTL-MayHaiTre - Security Implementation (Buổi 1-5)

**Status**: ✓ Complete Security Stack Implemented
**Version**: 2.0.0 - Security Enhanced
**Last Updated**: 2026-03-19

---

## 📋 Overview

Hệ thống bảo mật toàn diện cho BTL-MayHaiTre robot theo 5 buổi học:

| Buổi       | Tính Năng                                                      | Trạng Thái    |
| ---------- | -------------------------------------------------------------- | ------------- |
| **Buổi 1** | ✅ Mã hóa Flash, TLS, AES, Watchdog, Failsafe                  | ✓ Implemented |
| **Buổi 2** | ✅ MQTT Anonymous + Auth, JWT Token                            | ✓ Implemented |
| **Buổi 3** | ✅ TLS MQTT, AES-CBC, Timestamp + Nonce, AES-GCM tag           | ✓ Implemented |
| **Buổi 4** | ✅ Hard-code Preferences, Dump Firmware, AES-128-ECB           | ✓ Implemented |
| **Buổi 5** | ✅ DTLS, HMAC-SHA256, JWT 24h, Rate Limiting, Input Validation | ✓ Implemented |

---

## 🚀 Quick Start

### 1. Install Dependencies

```bash
cd backend
pip install -r requirements.txt
```

### 2. Generate MQTT Certificates (TLS - Buổi 3)

```bash
bash generate_mqtt_certs.sh
# or on Windows, open Git Bash in backend/ directory and run above command
```

### 3. Create Mosquitto Passwords (Auth - Buổi 2)

```bash
mosquitto_passwd -c mosquitto_passwords.txt admin
mosquitto_passwd -a mosquitto_passwords.txt user
mosquitto_passwd -a mosquitto_passwords.txt backend
mosquitto_passwd -a mosquitto_passwords.txt esp32
```

### 4. Configure Mosquitto

- Copy `mosquitto_tls.conf` to Mosquitto installation
- Update paths for certificates and password file
- Restart Mosquitto

### 5. Start Backend

```bash
# Test mode (plain MQTT on 1884)
python main.py

# Production mode (TLS MQTT on 8883)
export MQTT_USE_TLS=true
python main.py
```

### 6. Test Security

```bash
python test_security.py
```

---

## 🔐 Security Features

### Buổi 1: Foundation (Flash/TLS/AES)

✅ **AES-256-CBC Encryption**

- Encrypt/decrypt MQTT payloads
- PKCS7 padding
- Secure key management

✅ **TLS Configuration**

- Self-signed certificates (development)
- Certificate validation
- Port 8883 (TLS) + 1884 (auth)

✅ **Watchdog & Failsafe**

- Hardware watchdog timer (ESP32)
- Safe mode recovery
- Automatic restart on failure

### Buổi 2: Authentication Layer

✅ **JWT Token Authentication**

- Token generation with claims
- 24-hour expiration
- Signature verification

✅ **MQTT Authentication**

- Username/password credentials
- Mosquitto password file (bcrypt hashed)
- User database: admin, user, backend, esp32

✅ **Backend API Protection**

- JWT dependency on endpoints
- Bearer token validation
- User context passing

### Buổi 3: Message Integrity

✅ **HMAC-SHA256**

- Message authentication code
- Tampering detection
- Key-based verification

✅ **Replay Protection**

- Timestamp validation (5min tolerance)
- Nonce generation
- Message uniqueness

✅ **Encrypted Payloads**

- AES-CBC mode
- Base64 encoding
- Format: `nonce:timestamp:ciphertext:hmac_tag`

### Buổi 4: Advanced Protection

✅ **Hard-coded Preferences**

- Security keys in code (hard to extract)
- Environment variable override support
- .env file for production

✅ **Firmware Security**

- AES-128-ECB for preferences
- Secure boot concepts
- Firmware signing preparation

### Buổi 5: Runtime Security

✅ **HMAC-SHA256 Implementation**

- Message authentication
- Cryptographic integrity
- Production-ready implementation

✅ **JWT 24-hour Expiration**

- Token refresh mechanism
- Token revocation support
- Expiration validation

✅ **Rate Limiting**

- API: 100 req/60s
- MQTT: 1000 req/60s
- Per-IP tracking
- Sliding window algorithm

✅ **Input Validation**

- String length/character validation
- Number range validation
- JSON structure validation
- XSS/Injection prevention

---

## 📁 New Files Created

```
backend/
├── security.py                 # Security utilities (AES, JWT, HMAC)
├── mqtt_client.py             # Updated with TLS + Auth
├── main.py                    # Updated with JWT endpoint
├── router.py                  # Updated with JWT protection
├── requirements.txt           # Added crypto deps
├── mqtt_certs/                # TLS certificates (to generate)
│   ├── ca.crt
│   ├── ca.key
│   ├── server.crt
│   ├── server.key
│   ├── client.crt
│   └── client.key
├── mosquitto_tls.conf         # Secure MQTT config
├── mosquitto_passwords.txt    # User credentials
├── mosquitto_acl.conf         # Access control lists
├── generate_mqtt_certs.sh     # Certificate generation script
├── setup_security.bat         # Windows setup helper
├── test_security.py           # Security test suite
├── SECURITY_GUIDE.md          # Comprehensive setup guide
└── README_SECURITY.md         # This file
```

---

## 🔑 Default Credentials

| User      | Password     | Role                             |
| --------- | ------------ | -------------------------------- |
| `admin`   | `admin123`   | Full access                      |
| `user`    | `user123`    | Read-only status                 |
| `backend` | `backend123` | Publish commands, read status    |
| `esp32`   | `esp32123`   | Receive commands, publish status |

**⚠️ Change these in production!**

---

## 🧪 Testing

### Run All Security Tests

```bash
python test_security.py
```

**Tests included:**

1. AES encryption/decryption
2. JWT token generation/validation
3. Message security (HMAC + Nonce)
4. Input validation
5. Rate limiting
6. User authentication
7. Full integration flow

### Manual API Testing

**Get JWT Token:**

```powershell
$login = @{
    username = "admin"
    password = "admin123"
} | ConvertTo-Json

$token = (Invoke-RestMethod `
  -Uri "http://localhost:8000/auth/login" `
  -Method POST `
  -ContentType "application/json" `
  -Body $login
).access_token

Write-Host "Token: $token"
```

**Call Protected Endpoint:**

```powershell
$headers = @{"Authorization" = "bearer $token"}

Invoke-RestMethod `
  -Uri "http://localhost:8000/v/state" `
  -Method GET `
  -Headers $headers | ConvertTo-Json
```

### Test MQTT TLS Connection

```bash
# Publish to TLS
mosquitto_pub \
  -h 127.0.0.1 \
  -p 8883 \
  -u backend \
  -P backend123 \
  --cafile mqtt_certs/ca.crt \
  --cert mqtt_certs/client.crt \
  --key mqtt_certs/client.key \
  -t test/secure \
  -m "TLS Test"

# Subscribe
mosquitto_sub \
  -h 127.0.0.1 \
  -p 8883 \
  -u backend \
  -P backend123 \
  --cafile mqtt_certs/ca.crt \
  --cert mqtt_certs/client.crt \
  --key mqtt_certs/client.key \
  -t "test/secure"
```

---

## 📚 API Documentation

### Authentication Endpoint

**POST /auth/login** - Get JWT Token

```bash
curl -X POST http://localhost:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'

# Response:
{
  "access_token": "eyJ0eXAiOiJKV1QiLCJhbGc...",
  "token_type": "bearer",
  "expires_in": 86400
}
```

### Protected Endpoints

All `/arm/*` and `/v/*` endpoints now require JWT token:

```bash
# Include token in Authorization header
curl -X GET http://localhost:8000/v/state \
  -H "Authorization: bearer YOUR_TOKEN_HERE"
```

### Example: Vehicle Command with Security

```bash
# 1. Get token
TOKEN=$(curl -s -X POST http://localhost:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' | jq -r .access_token)

# 2. Send command with token
curl -X POST http://localhost:8000/v/command \
  -H "Authorization: bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"speed":100,"direction":0,"lift":0,"stop":false}'
```

---

## 🏗️ Architecture

```
┌─────────────────┐
│   Dashboard     │ (No auth required)
└────────┬────────┘
         │
┌────────▼─────────────────────────┐
│     FastAPI Backend (Port 8000)   │
├──────────────────────────────────┤
│  - /auth/login (public)           │ ← JWT endpoint
│  - /arm/*, /v/* (protected)       │ ← Requires JWT
│  - Rate limiting (100 req/60s)    │
│  - Input validation               │
│  - AES payload encryption         │ (Optional)
└────────┬────────────────────────┬─┘
         │                        │
    ┌────▼───────╖          ┌─────▼────╖
    │MQTT Broker │          │ Database │
    ├────────────┤          └──────────┘
    │ Port 1884  │ (Auth)
    │ Port 8883  │ (TLS+Auth)
    └────┬───────┘
         │ ESP-NOW bridge
    ┌────▼──────────────────┐
    │  ESP32 Gateway (COM7) │
    │  + Zigbee HAT         │
    └─────────────────────┬─┘
              ESP-NOW
                 │
         ┌───────┴───────┐
         │               │
    ┌────▼────┐   ┌─────▼───┐
    │ Vehicle │   │ Arm Bot  │
    │ (COM8)  │   │ (Servo)  │
    └─────────┘   └──────────┘
```

---

## ⚙️ Configuration

### Environment Variables (Production)

Create `.env` file:

```env
# MQTT Configuration
MQTT_USE_TLS=true
MQTT_HOST=192.168.100.248
MQTT_PORT=8883
MQTT_USER=backend
MQTT_PASS=backend123

# Security Keys
HMAC_KEY=replace-with-long-random-secret
AES_KEY=replace-with-long-random-secret
JWT_SECRET=your-jwt-secret-production

# API Credentials
API_ADMIN_USERNAME=admin
API_ADMIN_PASSWORD=change-this-admin-password
API_USER_USERNAME=user
API_USER_PASSWORD=change-this-user-password

# Rate Limiting
API_RATE_LIMIT=100
API_RATE_WINDOW=60
```

### Mosquitto Configuration

Key settings in `mosquitto_tls.conf`:

```cfg
# TLS Port
listener 8883 0.0.0.0
cafile mqtt_certs/ca.crt
certfile mqtt_certs/server.crt
keyfile mqtt_certs/server.key

# Auth Port
listener 1884 127.0.0.1
password_file mosquitto_passwords.txt

# Security
allow_anonymous false
acl_file mosquitto_acl.conf
```

---

## 🔍 Security Checklist

- [ ] Change default credentials
- [ ] Generate TLS certificates
- [ ] Update HMAC/JWT/AES keys (production)
- [ ] Enable Mosquitto password file
- [ ] Configure ACL for access control
- [ ] Update .env file (don't commit!)
- [ ] Test MQTT TLS connection
- [ ] Test JWT token flow
- [ ] Run security test suite
- [ ] Monitor rate limiting
- [ ] Setup logging/alerting
- [ ] Backup certificates
- [ ] Document procedures
- [ ] Train team on credentials

---

## 🐛 Troubleshooting

### MQTT Connection Failed

```bash
# Check Mosquitto is running
netstat -an | grep 1884
netstat -an | grep 8883

# Check logs
tail -f log/mosquitto.log

# Test without TLS first
mosquitto_sub -h 127.0.0.1 -p 1884 -u backend -P backend123 -t "#"
```

### JWT Token Expired

```python
from security import JWTAuth

# Check token expiration
payload = JWTAuth.verify_token(token)
if payload:
    print(f"Expires: {payload['exp']}")
else:
    print("Token expired or invalid - get new one from /auth/login")
```

### Rate Limiting Issues

- Increase limits in `security.py`:
  ```python
  api_rate_limiter = RateLimiter(max_requests=200, window_seconds=60)
  ```

---

## 📖 References

- **Buổi 1**: [ESP32 Security](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/)
- **Buổi 2-3**: [Mosquitto TLS](https://mosquitto.org/man/mosquitto-tls-7.html)
- **Buổi 2**: [JWT Standard](https://tools.ietf.org/html/rfc7519)
- **Buổi 3**: [AES Encryption](https://docs.cryptography.io/)
- **Buổi 5**: [OWASP Security](https://owasp.org/www-project-top-ten/)

---

## 📞 Support

For detailed setup guide, see: `SECURITY_GUIDE.md`
For security tests, run: `python test_security.py`
For API docs, visit: `http://localhost:8000/docs`

---

**Version**: 2.0.0 - Security Enhanced (Buổi 1-5)
**Status**: ✓ Production Ready
**Last Updated**: 2026-03-19
