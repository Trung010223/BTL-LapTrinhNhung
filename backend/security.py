"""
Bảo mật toàn diện cho hệ thống (Buổi 1-5)
- Buổi 1: Mã hóa Flash/TLS/AES, Watchdog, Failsafe
- Buổi 2: MQTT Auth + JWT Authentication
- Buổi 3: TLS MQTT + AES-CBC, Timestamp + Nonce, AES-GCM
- Buổi 4: Hard-code Preferences, Dump Firmware, AES-128-ECB
- Buổi 5: DTLS, HMAC-SHA256, JWT 24h, Rate Limiting
"""

import json
import hmac
import hashlib
import base64
import secrets
import time
from datetime import datetime, timedelta
from typing import Optional, Tuple

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from jwt import encode, decode, ExpiredSignatureError, InvalidTokenError
from pydantic import BaseModel

# ============= KEYS & CONSTANTS =============
# Buổi 1-5: Hard-coded keys (should be in environment in production)
HMAC_KEY = b"a" * 32  # 32 bytes for HMAC-SHA256
AES_KEY = b"b" * 32  # 32 bytes for AES-256
JWT_SECRET = "your-jwt-secret-key-change-me-in-production"
JWT_ALGORITHM = "HS256"
JWT_EXPIRATION_HOURS = 24

# IV for AES (should rotate per message in production)
AES_IV = b"c" * 16  # 16 bytes for AES-CBC


# ============= AES ENCRYPTION (Buổi 3-4) =============
class AESCrypto:
    """AES-CBC encryption/decryption với HMAC-SHA256 (Buổi 3-5)"""
    
    @staticmethod
    def encrypt(plaintext: str, key: bytes = AES_KEY, iv: bytes = AES_IV) -> str:
        """
        Mã hóa AES-256-CBC + HMAC-SHA256
        Returns: {nonce}:{timestamp}:{ciphertext}:{hmac_tag}
        """
        try:
            # Generate nonce for replay attack prevention (Buổi 3)
            nonce = secrets.token_hex(8)
            timestamp = str(int(time.time()))
            
            # Encrypt với AES-256-CBC
            cipher = Cipher(
                algorithms.AES(key),
                modes.CBC(iv),
                backend=default_backend()
            )
            encryptor = cipher.encryptor()
            
            # Add PKCS7 padding
            plaintext_bytes = plaintext.encode('utf-8')
            padding_len = 16 - (len(plaintext_bytes) % 16)
            padded = plaintext_bytes + bytes([padding_len] * padding_len)
            
            ciphertext = encryptor.update(padded) + encryptor.finalize()
            ciphertext_b64 = base64.b64encode(ciphertext).decode('utf-8')
            
            # Create HMAC-SHA256 tag (Buổi 5)
            hmac_data = f"{nonce}:{timestamp}:{ciphertext_b64}".encode('utf-8')
            hmac_tag = hmac.new(HMAC_KEY, hmac_data, hashlib.sha256).hexdigest()
            
            return f"{nonce}:{timestamp}:{ciphertext_b64}:{hmac_tag}"
        except Exception as e:
            raise ValueError(f"Encryption failed: {str(e)}")
    
    @staticmethod
    def decrypt(ciphertext_full: str, key: bytes = AES_KEY, iv: bytes = AES_IV) -> str:
        """
        Giải mã AES-256-CBC + Verify HMAC-SHA256
        Format: {nonce}:{timestamp}:{ciphertext}:{hmac_tag}
        """
        try:
            parts = ciphertext_full.split(':')
            if len(parts) != 4:
                raise ValueError("Invalid ciphertext format")
            
            nonce, timestamp, ciphertext_b64, hmac_tag = parts
            
            # Verify HMAC tag
            hmac_data = f"{nonce}:{timestamp}:{ciphertext_b64}".encode('utf-8')
            expected_tag = hmac.new(HMAC_KEY, hmac_data, hashlib.sha256).hexdigest()
            if not hmac.compare_digest(hmac_tag, expected_tag):
                raise ValueError("HMAC verification failed - possible tampering detected")
            
            # Verify timestamp (anti-replay, Buổi 3)
            msg_time = int(timestamp)
            current_time = int(time.time())
            if current_time - msg_time > 300:  # 5 minutes tolerance
                raise ValueError("Message timestamp too old - possible replay attack")
            
            # Decrypt
            cipher = Cipher(
                algorithms.AES(key),
                modes.CBC(iv),
                backend=default_backend()
            )
            decryptor = cipher.decryptor()
            
            ciphertext = base64.b64decode(ciphertext_b64)
            plaintext_padded = decryptor.update(ciphertext) + decryptor.finalize()
            
            # Remove PKCS7 padding
            padding_len = plaintext_padded[-1]
            plaintext = plaintext_padded[:-padding_len].decode('utf-8')
            
            return plaintext
        except Exception as e:
            raise ValueError(f"Decryption failed: {str(e)}")


# ============= JWT AUTHENTICATION (Buổi 2) =============
class JWTAuth:
    """JWT token management (Buổi 2-5)"""
    
    @staticmethod
    def create_token(user_id: str, username: str, expires_hours: int = JWT_EXPIRATION_HOURS) -> str:
        """Tạo JWT token với 24h expiration (Buổi 5)"""
        payload = {
            "user_id": user_id,
            "username": username,
            "exp": datetime.utcnow() + timedelta(hours=expires_hours),
            "iat": datetime.utcnow(),
        }
        token = encode(payload, JWT_SECRET, algorithm=JWT_ALGORITHM)
        return token
    
    @staticmethod
    def verify_token(token: str) -> Optional[dict]:
        """Xác thực JWT token"""
        try:
            payload = decode(token, JWT_SECRET, algorithms=[JWT_ALGORITHM])
            return payload
        except ExpiredSignatureError:
            return None  # Token expired
        except InvalidTokenError:
            return None  # Invalid token


class TokenRequest(BaseModel):
    username: str
    password: str


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = "bearer"
    expires_in: int = JWT_EXPIRATION_HOURS * 3600


# ============= USER AUTHENTICATION STORAGE (Buổi 2) =============
# In production, use proper database with hashed passwords
VALID_USERS = {
    "admin": "admin123",  # Change this in production!
    "user": "user123",
}


def verify_user(username: str, password: str) -> bool:
    """Xác thực username/password"""
    return VALID_USERS.get(username) == password


# ============= RATE LIMITING (Buổi 5) =============
class RateLimiter:
    """Rate limiter để chống brute-force"""
    
    def __init__(self, max_requests: int = 100, window_seconds: int = 60):
        self.max_requests = max_requests
        self.window_seconds = window_seconds
        self.requests: dict = {}  # {ip: [(timestamp, count)]}
    
    def is_allowed(self, client_ip: str) -> bool:
        """Check if request is allowed for this IP"""
        now = time.time()
        
        if client_ip not in self.requests:
            self.requests[client_ip] = []
        
        # Remove old requests outside window
        self.requests[client_ip] = [
            req_time for req_time in self.requests[client_ip]
            if now - req_time < self.window_seconds
        ]
        
        # Check if exceeded limit
        if len(self.requests[client_ip]) >= self.max_requests:
            return False
        
        # Add current request
        self.requests[client_ip].append(now)
        return True


# Global rate limiters
api_rate_limiter = RateLimiter(max_requests=100, window_seconds=60)
mqtt_rate_limiter = RateLimiter(max_requests=1000, window_seconds=60)


# ============= INPUT VALIDATION (Buổi 5) =============
class InputValidator:
    """Kiểm tra input để chống injection/XSS"""
    
    @staticmethod
    def validate_string(value: str, max_length: int = 256, allowed_chars: str = None) -> bool:
        """Validate string input"""
        if not isinstance(value, str):
            return False
        if len(value) > max_length:
            return False
        if allowed_chars and not all(c in allowed_chars for c in value):
            return False
        return True
    
    @staticmethod
    def validate_number(value: int, min_val: int = None, max_val: int = None) -> bool:
        """Validate number range"""
        if not isinstance(value, int):
            return False
        if min_val is not None and value < min_val:
            return False
        if max_val is not None and value > max_val:
            return False
        return True
    
    @staticmethod
    def validate_json(data: dict, required_keys: list = None) -> bool:
        """Validate JSON has required keys"""
        if not isinstance(data, dict):
            return False
        if required_keys:
            if not all(key in data for key in required_keys):
                return False
        return True


# ============= MESSAGE INTEGRITY & REPLAY PROTECTION (Buổi 3-5) =============
class MessageSecurity:
    """HMAC-SHA256 + Timestamp + Nonce (Buổi 3-5)"""
    
    @staticmethod
    def create_secure_payload(data: dict, secret: bytes = HMAC_KEY) -> str:
        """Tạo secure payload với message authentication"""
        timestamp = str(int(time.time()))
        nonce = secrets.token_hex(16)
        
        payload_dict = {
            "data": data,
            "timestamp": timestamp,
            "nonce": nonce,
        }
        payload_json = json.dumps(payload_dict, sort_keys=True)
        
        # Create HMAC
        hmac_tag = hmac.new(secret, payload_json.encode('utf-8'), hashlib.sha256).hexdigest()
        
        return json.dumps({
            "payload": payload_json,
            "hmac": hmac_tag,
        })
    
    @staticmethod
    def verify_secure_payload(secure_msg: str, secret: bytes = HMAC_KEY) -> Optional[dict]:
        """Xác thực secure payload"""
        try:
            outer = json.loads(secure_msg)
            payload_json = outer["payload"]
            hmac_tag = outer["hmac"]
            
            # Verify HMAC
            expected_tag = hmac.new(secret, payload_json.encode('utf-8'), hashlib.sha256).hexdigest()
            if not hmac.compare_digest(hmac_tag, expected_tag):
                return None
            
            # Parse and verify timestamp
            payload_dict = json.loads(payload_json)
            msg_time = int(payload_dict["timestamp"])
            if int(time.time()) - msg_time > 300:
                return None  # Too old
            
            return payload_dict["data"]
        except Exception:
            return None


print("✓ Security module loaded: AES-CBC, HMAC-SHA256, JWT, Rate Limiting, Input Validation")
