#include "web_content.h"
const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="vi"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Robot ARM Setup</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#0d0d1a;color:#e0e0ff;font-family:'Segoe UI',sans-serif;
    display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
  .box{background:#13132a;border:1px solid #1e1e3a;border-radius:16px;
    padding:32px 28px;width:100%;max-width:400px}
  h2{color:#5577ff;margin-bottom:6px;font-size:1.3rem}
  p{color:#7788aa;font-size:.85rem;margin-bottom:24px}
  label{display:block;font-size:.82rem;color:#aac4ff;margin-bottom:5px}
  input{width:100%;padding:10px 14px;background:#0d0d1a;border:1px solid #1e1e3a;
    border-radius:8px;color:#e0e0ff;font-size:.95rem;margin-bottom:16px;outline:none}
  input:focus{border-color:#5577ff}
  button{width:100%;padding:13px;background:#5577ff;border:none;border-radius:10px;
    color:#fff;font-size:1rem;font-weight:600;cursor:pointer}
</style>
</head><body><div class="box">
  <h2>🦾 Robot ARM Setup</h2>
  <p>Kết nối WiFi để sử dụng hệ thống</p>
  <form action="/save" method="POST">
    <label>Tên WiFi (SSID)</label><input type="text" name="ssid" required>
    <label>Mật khẩu WiFi</label><input type="password" name="pass" required>
    <label>MQTT Broker IP</label><input type="text" name="mqtt_ip" value="192.168.100.101">
    <label>MQTT Port</label><input type="number" name="mqtt_port" value="1883">
    <button type="submit">💾 Lưu &amp; Kết nối</button>
  </form>
</div></body></html>
)rawliteral";
 

const char PORTAL_SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="vi"><head><meta charset="UTF-8">
<style>
  body{background:#0d0d1a;color:#e0e0ff;font-family:'Segoe UI',sans-serif;
    display:flex;justify-content:center;align-items:center;min-height:100vh;text-align:center}
  .box{background:#13132a;border:1px solid #1e1e3a;border-radius:16px;
    padding:32px;max-width:360px}
  h2{color:#4cff8f;font-size:1.4rem;margin-bottom:12px}
  p{color:#7788aa}
</style>
</head><body><div class="box">
  <h2>✅ Đã lưu!</h2>
  <p>Robot ARM khởi động lại và kết nối WiFi.<br>Đợi 10 giây rồi truy cập dashboard.</p>
</div></body></html>
)rawliteral";

