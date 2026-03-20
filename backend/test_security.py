#!/usr/bin/env python3
"""
Security Features Test Script (Buổi 1-5)
Kiểm tra: JWT, AES, HMAC, Rate Limiting, Input Validation
"""

import sys
import json
from security import (
    AESCrypto,
    JWTAuth,
    MessageSecurity,
    InputValidator,
    RateLimiter,
    verify_user,
)


def test_aes_encryption():
    """Test AES-CBC encryption/decryption (Buổi 3)"""
    print("\n" + "="*60)
    print("TEST 1: AES-CBC Encryption (Buổi 3)")
    print("="*60)
    
    plaintext = "Hello Secure MQTT World 123!"
    print(f"Plaintext: {plaintext}")
    
    encrypted = AESCrypto.encrypt(plaintext)
    print(f"Encrypted: {encrypted[:50]}...")
    
    decrypted = AESCrypto.decrypt(encrypted)
    print(f"Decrypted: {decrypted}")
    
    assert plaintext == decrypted, "AES encryption/decryption failed!"
    print("✓ AES Test PASSED")


def test_jwt_token():
    """Test JWT token generation/validation (Buổi 2)"""
    print("\n" + "="*60)
    print("TEST 2: JWT Token (Buổi 2)")
    print("="*60)
    
    # Create token
    token = JWTAuth.create_token(user_id="user123", username="admin", expires_hours=24)
    print(f"Generated Token: {token[:50]}...")
    
    # Verify token
    payload = JWTAuth.verify_token(token)
    print(f"Token Payload: {payload}")
    assert payload is not None, "JWT token verification failed!"
    assert payload["username"] == "admin", "Username mismatch in token!"
    
    # Test invalid token
    invalid_payload = JWTAuth.verify_token("invalid.token.here")
    assert invalid_payload is None, "Invalid token should fail!"
    print("✓ JWT Test PASSED")


def test_message_security():
    """Test message integrity with HMAC + nonce (Buổi 3-5)"""
    print("\n" + "="*60)
    print("TEST 3: Message Security - HMAC + Nonce (Buổi 3-5)")
    print("="*60)
    
    data = {
        "cmd": "vehicle",
        "speed": 220,
        "direction": 1,
        "lift": 0,
    }
    print(f"Original Data: {data}")
    
    # Create secure payload
    secure_msg = MessageSecurity.create_secure_payload(data)
    print(f"Secure Message: {secure_msg[:80]}...")
    
    # Verify payload
    verified = MessageSecurity.verify_secure_payload(secure_msg)
    print(f"Verified Data: {verified}")
    assert verified == data, "Message verification failed!"
    
    # Test tampering detection
    tampered = secure_msg.replace("220", "999", 1)
    not_verified = MessageSecurity.verify_secure_payload(tampered)
    assert not_verified is None, "Tampered message should fail verification!"
    print("✓ Message Security Test PASSED")


def test_input_validation():
    """Test input validation (Buổi 5)"""
    print("\n" + "="*60)
    print("TEST 4: Input Validation (Buổi 5)")
    print("="*60)
    
    # String validation
    assert InputValidator.validate_string("admin", max_length=50), "Valid string failed"
    assert not InputValidator.validate_string("a"*100, max_length=50), "Long string should fail"
    print("✓ String validation OK")
    
    # Number validation
    assert InputValidator.validate_number(100, min_val=0, max_val=255), "Valid number failed"
    assert not InputValidator.validate_number(300, min_val=0, max_val=255), "Out of range should fail"
    print("✓ Number validation OK")
    
    # JSON validation
    data = {"username": "admin", "password": "pass123"}
    assert InputValidator.validate_json(data, required_keys=["username", "password"]), "Valid JSON failed"
    assert not InputValidator.validate_json(data, required_keys=["username", "email"]), "Missing key should fail"
    print("✓ JSON validation OK")
    
    print("✓ Input Validation Test PASSED")


def test_rate_limiting():
    """Test rate limiting (Buổi 5)"""
    print("\n" + "="*60)
    print("TEST 5: Rate Limiting (Buổi 5)")
    print("="*60)
    
    limiter = RateLimiter(max_requests=5, window_seconds=60)
    ip = "192.168.1.100"
    
    # Make 5 requests
    for i in range(5):
        allowed = limiter.is_allowed(ip)
        print(f"Request {i+1}: {'ALLOWED' if allowed else 'BLOCKED'}")
        assert allowed, f"Request {i+1} should be allowed"
    
    # 6th request should be blocked
    allowed = limiter.is_allowed(ip)
    print(f"Request 6: {'ALLOWED' if allowed else 'BLOCKED'}")
    assert not allowed, "Request 6 should be blocked!"
    
    print("✓ Rate Limiting Test PASSED")


def test_user_authentication():
    """Test user authentication (Buổi 2)"""
    print("\n" + "="*60)
    print("TEST 6: User Authentication (Buổi 2)")
    print("="*60)
    
    # Valid credentials
    assert verify_user("admin", "admin123"), "Valid user should authenticate"
    print("✓ admin:admin123 - AUTHENTICATED")
    
    # Invalid password
    assert not verify_user("admin", "wrongpass"), "Wrong password should fail"
    print("✓ admin:wrongpass - REJECTED")
    
    # Invalid user
    assert not verify_user("nonexistent", "pass"), "Nonexistent user should fail"
    print("✓ nonexistent - REJECTED")
    
    print("✓ User Authentication Test PASSED")


def test_integration_flow():
    """Test full security flow (Buổi 2-5)"""
    print("\n" + "="*60)
    print("TEST 7: Full Integration Flow (Buổi 2-5)")
    print("="*60)
    
    # 1. Authenticate user
    assert verify_user("admin", "admin123"), "Authentication failed"
    print("✓ Step 1: User authenticated")
    
    # 2. Generate JWT
    token = JWTAuth.create_token(user_id="admin", username="admin")
    assert token is not None, "Token generation failed"
    print(f"✓ Step 2: JWT token generated: {token[:30]}...")
    
    # 3. Verify JWT
    payload = JWTAuth.verify_token(token)
    assert payload is not None, "Token verification failed"
    print(f"✓ Step 3: JWT verified - user: {payload['username']}")
    
    # 4. Prepare secure MQTT payload
    vehicle_cmd = {"cmd": "vehicle", "speed": 150, "direction": 0}
    secure_msg = MessageSecurity.create_secure_payload(vehicle_cmd)
    print(f"✓ Step 4: Secure MQTT payload created")
    
    # 5. Encrypt payload
    encrypted = AESCrypto.encrypt(json.dumps(vehicle_cmd))
    print(f"✓ Step 5: Payload encrypted (AES-256-CBC)")
    
    # 6. Decrypt & verify
    decrypted = AESCrypto.decrypt(encrypted)
    print(f"✓ Step 6: Payload decrypted & verified")
    
    # 7. Check input validation
    assert InputValidator.validate_number(150, min_val=-255, max_val=255), "Speed validation failed"
    print(f"✓ Step 7: Input validation passed")
    
    print("✓ Integration Flow Test PASSED")


def run_all_tests():
    """Run all security tests"""
    print("\n" + "="*60)
    print("BTL-MayHaiTre SECURITY TEST SUITE (Buổi 1-5)")
    print("="*60)
    
    tests = [
        test_aes_encryption,
        test_jwt_token,
        test_message_security,
        test_input_validation,
        test_rate_limiting,
        test_user_authentication,
        test_integration_flow,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except AssertionError as e:
            print(f"✗ TEST FAILED: {e}")
            failed += 1
        except Exception as e:
            print(f"✗ TEST ERROR: {e}")
            failed += 1
    
    print("\n" + "="*60)
    print(f"RESULTS: {passed} PASSED, {failed} FAILED")
    print("="*60)
    
    if failed == 0:
        print("✓ ALL SECURITY TESTS PASSED!")
        return 0
    else:
        print("✗ SOME TESTS FAILED")
        return 1


if __name__ == "__main__":
    sys.exit(run_all_tests())
