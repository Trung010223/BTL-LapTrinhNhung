@echo off
REM Security Setup Script for Windows (Buổi 1-5)

setlocal enabledelayedexpansion

echo ============================================================
echo BTL-MayHaiTre SECURITY SETUP (Buổi 1-5)
echo ============================================================
echo.

REM Check if we're in backend directory
if not exist "requirements.txt" (
    echo ERROR: Run this script from backend/ directory
    exit /b 1
)

REM 1. Install dependencies
echo [1/4] Installing Python packages...
pip install -r requirements.txt
if errorlevel 1 (
    echo ERROR: Failed to install packages
    exit /b 1
)
echo ✓ Packages installed

echo.

REM 2. Create directories
echo [2/4] Creating directories...
if not exist "mqtt_certs" mkdir mqtt_certs
if not exist "data" mkdir data
if not exist "log" mkdir log
echo ✓ Directories created

echo.

REM 3. Check OpenSSL
echo [3/4] Checking OpenSSL...
where openssl >nul 2>&1
if errorlevel 1 (
    echo WARNING: OpenSSL not found in PATH
    echo Install: choco install openssl  (or download from https://slproweb.com/products/Win32OpenSSL.html)
    echo.
) else (
    echo ✓ OpenSSL found
)

echo.

REM 4. Check Mosquitto
echo [4/4] Checking Mosquitto...
where mosquitto >nul 2>&1
if errorlevel 1 (
    echo WARNING: Mosquitto not found in PATH
    echo Install: choco install mosquitto  (Windows Package Manager)
    echo Or download from: https://mosquitto.org/download/
    echo.
) else (
    echo ✓ Mosquitto found
)

echo.
echo ============================================================
echo SETUP COMPLETE!
echo ============================================================
echo.
echo Next steps:
echo 1. Generate MQTT certificates:
echo    bash generate_mqtt_certs.sh  (requires OpenSSL)
echo.
echo 2. Create Mosquitto password file:
echo    mosquitto_passwd -c mosquitto_passwords.txt admin
echo    mosquitto_passwd -a mosquitto_passwords.txt user
echo    mosquitto_passwd -a mosquitto_passwords.txt backend
echo    mosquitto_passwd -a mosquitto_passwords.txt esp32
echo.
echo 3. Configure Mosquitto:
echo    Copy mosquitto_tls.conf to C:\Program Files\mosquitto\
echo    Copy mqtt_certs\ to C:\Program Files\mosquitto\
echo    Copy mosquitto_passwords.txt to C:\Program Files\mosquitto\
echo    Copy mosquitto_acl.conf to C:\Program Files\mosquitto\
echo.
echo 4. Start Mosquitto:
echo    net start Mosquitto
echo.
echo 5. Run security tests:
echo    python test_security.py
echo.
echo 6. Start backend:
echo    python main.py
echo.
echo For detailed guide, see: SECURITY_GUIDE.md
echo ============================================================

pause
