#include "dashboard_html.h"

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Robot Control Hub</title>
<link rel='stylesheet' href='/app.css'>
</head>
<body>
 
<div class="alert-box" id="alertBox"></div>
 
<div class="header">
  <div class="logo">⚡ ROBOT HUB</div>
  <!-- BÀI 4: Hiển thị trạng thái MQTT và Queue -->
  <div class="mqtt-badge bad" id="mqttBadge">● MQTT –</div>
  <div class="queue-badge" id="queueBadge">📦 Queue: 0</div>
  <div class="ws-badge" id="wsBadge">● OFFLINE</div>
</div>
 
<div class="grid">
 
  <div class="panel grid-full">
    <div class="panel-title">◈ TELEMETRY XE (ESP32 #2)</div>
    <div class="tilt-wrap">
      <div class="tilt-canvas-wrap">
        <canvas id="tiltCanvas" width="110" height="110"></canvas>
      </div>
      <div class="tilt-values">
        <div class="tval-row">
          <span class="tval-label">PITCH (X)</span>
          <span class="tval-num" id="pitchVal">0.00°</span>
        </div>
        <div class="tval-row">
          <span class="tval-label">ROLL (Y)</span>
          <span class="tval-num" id="rollVal">0.00°</span>
        </div>
        <div class="balance-badge no" id="balanceBadge">✗ MẤT CÂN BẰNG</div>
      </div>
      <div style="flex:1.2">
        <div style="font-size:0.62rem;color:var(--dim);letter-spacing:1px;margin-bottom:8px;">MOTOR OUTPUT</div>
        <div class="motor-bars">
          <div class="mbar-row"><span class="mbar-label">A</span><div class="mbar-track"><div class="mbar-fill" id="barA"></div></div><span class="mbar-val" id="valA">0</span></div>
          <div class="mbar-row"><span class="mbar-label">B</span><div class="mbar-track"><div class="mbar-fill" id="barB"></div></div><span class="mbar-val" id="valB">0</span></div>
          <div class="mbar-row"><span class="mbar-label">C</span><div class="mbar-track"><div class="mbar-fill" id="barC"></div></div><span class="mbar-val" id="valC">0</span></div>
          <div class="mbar-row"><span class="mbar-label">D</span><div class="mbar-track"><div class="mbar-fill" id="barD"></div></div><span class="mbar-val" id="valD">0</span></div>
          <div class="mbar-row"><span class="mbar-label">E</span><div class="mbar-track"><div class="mbar-fill" id="barE"></div></div><span class="mbar-val" id="valE">0</span></div>
        </div>
      </div>
    </div>
  </div>
 
  <div class="panel">
    <div class="panel-title">◈ TỐC ĐỘ XE</div>
    <div class="speed-display" id="speedDisplay">0</div>
    <div class="speed-unit">PWM (0 – 255)</div>
    <input type="range" class="speed-slider" id="speedSlider"
           min="0" max="255" value="0" oninput="onSpeedChange(this.value)">
    <div style="display:flex;justify-content:space-between;margin-top:4px;">
      <span style="font-size:0.6rem;color:var(--dim)">0</span>
      <span style="font-size:0.6rem;color:var(--dim)">255</span>
    </div>
  </div>
 
  <div class="panel">
    <div class="panel-title">◈ ĐIỀU HƯỚNG XE</div>
    <div class="dpad">
      <div></div>
      <div class="dpad-btn" id="btnFwd" onpointerdown="dirPress('fwd')" onpointerup="dirRelease()" onpointerleave="dirRelease()">▲</div>
      <div></div>
      <div class="dpad-btn" id="btnLeft" onpointerdown="dirPress('left')" onpointerup="dirRelease()" onpointerleave="dirRelease()">◀</div>
      <div class="dpad-btn dpad-center" id="btnStop" onpointerdown="dirPress('stop')" onpointerup="dirRelease()">STOP</div>
      <div class="dpad-btn" id="btnRight" onpointerdown="dirPress('right')" onpointerup="dirRelease()" onpointerleave="dirRelease()">▶</div>
      <div></div>
      <div class="dpad-btn" id="btnBwd" onpointerdown="dirPress('bwd')" onpointerup="dirRelease()" onpointerleave="dirRelease()">▼</div>
      <div></div>
    </div>
  </div>
 
  <div class="panel grid-full">
    <div class="panel-title">◈ ROBOT ARM</div>
    <div class="motor-status-row">
      <span class="ms-label">⚙ Quạt hút (Vacuum)</span>
      <span class="ms-badge off" id="vacuumBadge">⛔ TẮT</span>
    </div>
    <div class="servo-list">
      <div class="servo-row"><div class="servo-header"><span class="servo-name">🔄 Base (Xoay đế)</span><span class="servo-deg" id="v0">0°</span></div><input type="range" class="servo-slider" id="s0" min="150" max="600" value="330" oninput="sendServo(0,this.value)"></div>
      <div class="servo-row"><div class="servo-header"><span class="servo-name">⬆ Khớp dưới</span><span class="servo-deg" id="v1">0°</span></div><input type="range" class="servo-slider" id="s1" min="150" max="600" value="150" oninput="sendServo(1,this.value)"></div>
      <div class="servo-row"><div class="servo-header"><span class="servo-name">💪 Khớp trên</span><span class="servo-deg" id="v2">0°</span></div><input type="range" class="servo-slider" id="s2" min="150" max="600" value="300" oninput="sendServo(2,this.value)"></div>
      <div class="servo-row"><div class="servo-header"><span class="servo-name">✊ Tay gắp</span><span class="servo-deg" id="v3">0°</span></div><input type="range" class="servo-slider" id="s3" min="150" max="600" value="410" oninput="sendServo(3,this.value)"></div>
    </div>
    <div class="arm-btns">
      <button class="arm-btn home" onclick="armCmd('home')">🏠 VỀ GỐC</button>
      <button class="arm-btn auto" onclick="armCmd('auto')">▶ TỰ ĐỘNG</button>
      <button class="arm-btn stop" onclick="armCmd('stop')">⏹ DỪNG</button>
      <button class="arm-btn cut" id="btnCut" onclick="armCmd('cut')">✂ CẮT</button>
    </div>
  </div>
 
</div>
 
<script src='/app.js'></script>
</body>
</html>
)rawliteral";

