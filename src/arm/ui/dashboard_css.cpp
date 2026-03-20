#include "dashboard_css.h"

const char DASHBOARD_CSS[] PROGMEM = R"rawliteral(
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@400;700;900&display=swap');
:root {
  --bg:#080c14; --panel:#0d1520; --border:#1a2d4a;
  --accent:#00d4ff; --accent2:#ff6b35;
  --green:#00ff88; --red:#ff3355; --yellow:#ffd700;
  --text:#c8e0f4; --dim:#4a6080;
}
* { box-sizing:border-box; margin:0; padding:0; }
body { background:var(--bg); color:var(--text); font-family:'Share Tech Mono',monospace;
  min-height:100vh; padding:12px;
  background-image:
    linear-gradient(rgba(0,212,255,0.03) 1px,transparent 1px),
    linear-gradient(90deg,rgba(0,212,255,0.03) 1px,transparent 1px);
  background-size:40px 40px; }
.header { display:flex; align-items:center; justify-content:space-between;
  padding:10px 16px; border:1px solid var(--border); border-radius:8px;
  background:var(--panel); margin-bottom:12px; gap:8px; }
.logo { font-family:'Orbitron',monospace; font-weight:900; font-size:1.1rem;
  color:var(--accent); letter-spacing:3px; text-shadow:0 0 20px rgba(0,212,255,0.5); }
.ws-badge { font-size:0.72rem; padding:3px 10px; border-radius:4px;
  border:1px solid var(--dim); color:var(--dim); transition:all 0.3s; }
.ws-badge.on { border-color:var(--green); color:var(--green); box-shadow:0 0 8px rgba(0,255,136,0.3); }
/* BÀI 4: MQTT + Queue status badges */
.mqtt-badge { font-size:0.68rem; padding:3px 10px; border-radius:4px;
  border:1px solid var(--dim); color:var(--dim); transition:all 0.3s; }
.mqtt-badge.ok  { border-color:var(--green); color:var(--green); }
.mqtt-badge.bad { border-color:var(--red);   color:var(--red); }
.queue-badge { font-size:0.68rem; padding:3px 10px; border-radius:4px;
  border:1px solid var(--dim); color:var(--dim); transition:all 0.3s; }
.queue-badge.has { border-color:var(--yellow); color:var(--yellow);
  animation:blink 1s infinite; }
@keyframes blink { 0%,100%{opacity:1} 50%{opacity:.5} }
/* Layout */
.grid { display:grid; grid-template-columns:1fr 1fr; gap:10px; }
.grid-full { grid-column:1/-1; }
.panel { background:var(--panel); border:1px solid var(--border); border-radius:8px;
  padding:14px; position:relative; overflow:hidden; }
.panel::before { content:''; position:absolute; top:0; left:0; right:0; height:2px;
  background:linear-gradient(90deg,transparent,var(--accent),transparent); opacity:0.5; }
.panel-title { font-family:'Orbitron',monospace; font-size:0.62rem; letter-spacing:2px;
  color:var(--accent); margin-bottom:12px; text-transform:uppercase; }
/* Tilt */
.tilt-wrap { display:flex; gap:12px; align-items:center; }
.tilt-canvas-wrap { flex:1; display:flex; justify-content:center; }
canvas#tiltCanvas { border:1px solid var(--border); border-radius:50%;
  background:radial-gradient(circle,#0a1825 60%,#0d1f30); }
.tilt-values { flex:1; display:flex; flex-direction:column; gap:8px; }
.tval-row { display:flex; flex-direction:column; gap:2px; }
.tval-label { font-size:0.62rem; color:var(--dim); letter-spacing:1px; }
.tval-num { font-family:'Orbitron',monospace; font-size:1.3rem; font-weight:700; color:var(--accent); }
.tval-num.warn { color:var(--yellow); }
.tval-num.danger { color:var(--red); }
.balance-badge { font-size:0.68rem; padding:4px 10px; border-radius:4px;
  border:1px solid var(--dim); color:var(--dim); text-align:center; margin-top:4px; transition:all 0.3s; }
.balance-badge.ok { border-color:var(--green); color:var(--green); background:rgba(0,255,136,0.07); }
.balance-badge.no { border-color:var(--red);   color:var(--red);   background:rgba(255,51,85,0.07); }
/* Motor bars */
.motor-bars { display:flex; flex-direction:column; gap:6px; }
.mbar-row { display:flex; align-items:center; gap:8px; }
.mbar-label { font-size:0.65rem; color:var(--dim); width:22px; }
.mbar-track { flex:1; height:10px; background:#0a1520; border-radius:5px;
  overflow:hidden; border:1px solid var(--border); position:relative; }
.mbar-fill { height:100%; border-radius:5px; transition:width 0.2s,background 0.2s;
  position:absolute; top:0; }
.mbar-fill.fwd { left:50%; background:var(--green); }
.mbar-fill.bwd { right:50%; background:var(--red); }
.mbar-val { font-size:0.65rem; color:var(--text); width:32px; text-align:right; }
/* Speed */
.speed-display { font-family:'Orbitron',monospace; font-size:2.4rem; font-weight:900;
  text-align:center; color:var(--accent); text-shadow:0 0 30px rgba(0,212,255,0.4); margin:6px 0; }
.speed-unit { font-size:0.7rem; color:var(--dim); text-align:center; margin-bottom:8px; }
input[type=range].speed-slider { -webkit-appearance:none; width:100%; height:6px;
  border-radius:3px; background:#0a1520; border:1px solid var(--border); outline:none; cursor:pointer; }
input[type=range].speed-slider::-webkit-slider-thumb { -webkit-appearance:none; width:20px; height:20px;
  border-radius:50%; background:var(--accent); box-shadow:0 0 10px rgba(0,212,255,0.6); cursor:pointer; }
/* D-Pad */
.dpad { display:grid; grid-template-columns:repeat(3,1fr); grid-template-rows:repeat(3,1fr);
  gap:6px; width:150px; height:150px; margin:0 auto; }
.dpad-btn { background:#0a1825; border:1px solid var(--border); border-radius:6px;
  color:var(--text); font-size:1.2rem; cursor:pointer;
  display:flex; align-items:center; justify-content:center;
  transition:all 0.1s; user-select:none; -webkit-tap-highlight-color:transparent; }
.dpad-btn:active,.dpad-btn.pressed { background:rgba(0,212,255,0.15);
  border-color:var(--accent); color:var(--accent);
  box-shadow:0 0 12px rgba(0,212,255,0.3); transform:scale(0.94); }
.dpad-center { background:rgba(255,51,85,0.1); border-color:var(--red);
  color:var(--red); font-size:0.7rem; font-family:'Orbitron',monospace; }
.dpad-center:active { background:rgba(255,51,85,0.25); border-color:var(--red);
  box-shadow:0 0 12px rgba(255,51,85,0.3); }
/* Servo */
.servo-list { display:flex; flex-direction:column; gap:10px; }
.servo-row { display:flex; flex-direction:column; gap:3px; }
.servo-header { display:flex; justify-content:space-between; }
.servo-name { font-size:0.65rem; color:var(--dim); }
.servo-deg  { font-size:0.65rem; color:var(--accent); font-family:'Orbitron',monospace; }
input[type=range].servo-slider { -webkit-appearance:none; width:100%; height:5px;
  border-radius:3px; background:#0a1520; border:1px solid var(--border); outline:none; cursor:pointer; }
input[type=range].servo-slider::-webkit-slider-thumb { -webkit-appearance:none; width:16px; height:16px;
  border-radius:50%; background:var(--accent2); box-shadow:0 0 8px rgba(255,107,53,0.5); cursor:pointer; }
/* Arm buttons */
.arm-btns { display:flex; gap:8px; margin-top:10px; flex-wrap:wrap; }
.arm-btn { flex:1; min-width:70px; padding:9px 6px; border:1px solid var(--border);
  border-radius:6px; background:#0a1825; color:var(--text);
  font-family:'Share Tech Mono',monospace; font-size:0.72rem; cursor:pointer; transition:all 0.15s; }
.arm-btn:active { transform:scale(0.95); }
.arm-btn.home { border-color:#2a5a9a; color:#7ab4ff; }
.arm-btn.auto { border-color:#2a6a4a; color:#7affc4; }
.arm-btn.stop { border-color:#6a2a3a; color:#ff7a9a; }
.arm-btn.cut  { border-color:#5a2a7a; color:#d07aff; }
.arm-btn.cut.active { background:rgba(90,42,122,0.3); color:#e0aaff;
  box-shadow:0 0 10px rgba(160,100,255,0.3); }
/* Motor status */
.motor-status-row { display:flex; justify-content:space-between; align-items:center;
  padding:8px 0; border-bottom:1px solid var(--border); margin-bottom:10px; }
.ms-label { font-size:0.7rem; color:var(--dim); }
.ms-badge { font-size:0.68rem; padding:3px 10px; border-radius:4px;
  border:1px solid var(--dim); color:var(--dim); transition:all 0.3s; }
.ms-badge.on  { border-color:var(--green); color:var(--green); background:rgba(0,255,136,0.08); }
.ms-badge.off { border-color:var(--dim);   color:var(--dim); }
/* Alert box (Bài 3) */
.alert-box { position:fixed; top:60px; right:12px; background:var(--panel);
  border:1px solid var(--yellow); border-radius:8px; padding:10px 16px;
  font-size:0.72rem; color:var(--yellow); z-index:100; display:none; max-width:280px; }
.alert-box.info { border-color:var(--accent); color:var(--accent); }
.alert-box.warn { border-color:var(--yellow); color:var(--yellow); }
.alert-box.err  { border-color:var(--red);    color:var(--red); }
::-webkit-scrollbar { width:4px; }
::-webkit-scrollbar-track { background:var(--bg); }
::-webkit-scrollbar-thumb { background:var(--border); border-radius:2px; }
)rawliteral";

