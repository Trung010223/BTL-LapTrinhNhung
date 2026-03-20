#include "dashboard_js.h"

const char DASHBOARD_JS[] PROGMEM = R"rawliteral(
let ws, wsOk=false, servoTimer=null, servoPending={}, currentSpeed=0;
 
function connect() {
  ws = new WebSocket('ws://'+location.hostname+':81/');
  ws.onopen = () => {
    wsOk = true;
    const b = document.getElementById('wsBadge');
    b.textContent = '● ONLINE'; b.className = 'ws-badge on';
  };
  ws.onclose = () => {
    wsOk = false;
    const b = document.getElementById('wsBadge');
    b.textContent = '● OFFLINE'; b.className = 'ws-badge';
    setTimeout(connect, 2000);
  };
  ws.onmessage = e => {
    const d = JSON.parse(e.data);
 
    if (d.type === 'state') {
      d.pos.forEach((v,i) => {
        document.getElementById('s'+i).value = v;
        document.getElementById('v'+i).textContent = pwmToDeg(v)+'°';
      });
    }
    if (d.type === 'vacuum') {
      const badge = document.getElementById('vacuumBadge');
      const btn   = document.getElementById('btnCut');
      if (d.state === 'on') {
        badge.textContent='✅ ĐANG CHẠY'; badge.className='ms-badge on';
        btn.classList.add('active'); btn.textContent='⏹ TẮT QUẠT';
      } else {
        badge.textContent='⛔ TẮT'; badge.className='ms-badge off';
        btn.classList.remove('active'); btn.textContent='✂ CẮT';
      }
    }
    if (d.type === 'tele') { updateTelemetry(d); }
    if (d.type === 'alert') { showAlert(d.msg, d.level||'info'); }
 
    // BÀI 4: Cập nhật MQTT status và Queue indicator
    if (d.type === 'queue') {
      const mb = document.getElementById('mqttBadge');
      const qb = document.getElementById('queueBadge');
      if (d.mqttState === 'connected') {
        mb.textContent = '● MQTT OK'; mb.className = 'mqtt-badge ok';
      } else {
        mb.textContent = '● MQTT –'; mb.className = 'mqtt-badge bad';
      }
      const pending = parseInt(d.pending) || 0;
      qb.textContent = '📦 Queue: ' + pending;
      qb.className = pending > 0 ? 'queue-badge has' : 'queue-badge';
    }
  };
}
 
// BÀI 3: Alert Box
function showAlert(msg, level) {
  const box = document.getElementById('alertBox');
  box.textContent = '⚠ '+msg;
  box.className = 'alert-box '+level;
  box.style.display = 'block';
  clearTimeout(box._t);
  box._t = setTimeout(() => box.style.display='none', 4000);
}
 
function pwmToDeg(v) { return Math.round((v-150)/(600-150)*180); }
 
function updateTelemetry(d) {
  const pitch = parseFloat(d.pitch), roll = parseFloat(d.roll);
  const pEl = document.getElementById('pitchVal');
  const rEl = document.getElementById('rollVal');
  pEl.textContent = pitch.toFixed(2)+'°';
  rEl.textContent = roll.toFixed(2)+'°';
  const cf = v => Math.abs(v)>20?'danger':Math.abs(v)>10?'warn':'';
  pEl.className = 'tval-num '+cf(pitch);
  rEl.className = 'tval-num '+cf(roll);
  const bb = document.getElementById('balanceBadge');
  if (d.isBalanced) { bb.textContent='✓ CÂN BẰNG';    bb.className='balance-badge ok'; }
  else              { bb.textContent='✗ MẤT CÂN BẰNG'; bb.className='balance-badge no'; }
  drawTilt(pitch, roll);
  ['A','B','C','D','E'].forEach((m,i) => {
    const v   = parseInt([d.curA,d.curB,d.curC,d.curD,d.curE][i])||0;
    const pct = Math.abs(v)/255*50;
    const bar = document.getElementById('bar'+m);
    const val = document.getElementById('val'+m);
    val.textContent = v;
    if (v>0)      { bar.className='mbar-fill fwd'; bar.style.width=pct+'%'; }
    else if (v<0) { bar.className='mbar-fill bwd'; bar.style.width=pct+'%'; }
    else          { bar.style.width='0%'; }
  });
}
 
function drawTilt(pitch, roll) {
  const canvas = document.getElementById('tiltCanvas');
  const ctx    = canvas.getContext('2d');
  const W=canvas.width, H=canvas.height, cx=W/2, cy=H/2, r=W/2-4;
  ctx.clearRect(0,0,W,H);
  ctx.beginPath(); ctx.arc(cx,cy,r,0,Math.PI*2);
  ctx.strokeStyle='#1a2d4a'; ctx.lineWidth=1.5; ctx.stroke();
  ctx.strokeStyle='#0f1e30'; ctx.lineWidth=0.8;
  for (let ang=0; ang<180; ang+=30) {
    const rad=ang*Math.PI/180;
    ctx.beginPath();
    ctx.moveTo(cx-r*Math.cos(rad), cy-r*Math.sin(rad));
    ctx.lineTo(cx+r*Math.cos(rad), cy+r*Math.sin(rad));
    ctx.stroke();
  }
  [0.33,0.66].forEach(f => {
    ctx.beginPath(); ctx.arc(cx,cy,r*f,0,Math.PI*2); ctx.stroke();
  });
  const maxDisp = r*0.7;
  const clampedDx = Math.max(-maxDisp, Math.min(maxDisp, (roll/30)*maxDisp));
  const clampedDy = Math.max(-maxDisp, Math.min(maxDisp, (pitch/30)*maxDisp));
  const lineLen=r*0.6, ang=-roll*Math.PI/180;
  ctx.beginPath();
  ctx.moveTo(cx-lineLen*Math.cos(ang), cy-lineLen*Math.sin(ang)+clampedDy*0.5);
  ctx.lineTo(cx+lineLen*Math.cos(ang), cy+lineLen*Math.sin(ang)+clampedDy*0.5);
  const balanced = Math.abs(pitch) <= 2 && Math.abs(roll) <= 2;
  ctx.strokeStyle = balanced?'#00ff88':(Math.abs(pitch)>20||Math.abs(roll)>20?'#ff3355':'#ffd700');
  ctx.lineWidth=2; ctx.stroke();
  ctx.beginPath(); ctx.arc(cx+clampedDx, cy+clampedDy, 5, 0, Math.PI*2);
  ctx.fillStyle = balanced?'#00ff88':'#ff6b35'; ctx.fill();
  ctx.beginPath(); ctx.arc(cx,cy,2,0,Math.PI*2);
  ctx.fillStyle='#4a6080'; ctx.fill();
}
drawTilt(0,0);
 
function onSpeedChange(v) {
  currentSpeed = parseInt(v);
  document.getElementById('speedDisplay').textContent = currentSpeed;
}
 
let dirHoldTimer = null;
function dirPress(dir) {
  clearInterval(dirHoldTimer); sendDir(dir);
  dirHoldTimer = setInterval(() => sendDir(dir), 200);
  const map={fwd:'btnFwd',bwd:'btnBwd',left:'btnLeft',right:'btnRight',stop:'btnStop'};
  if (map[dir]) document.getElementById(map[dir]).classList.add('pressed');
}
function dirRelease() {
  clearInterval(dirHoldTimer);
  ['btnFwd','btnBwd','btnLeft','btnRight','btnStop'].forEach(id =>
    document.getElementById(id).classList.remove('pressed'));
  wsSend({type:'vehicle', speed:currentSpeed, direction:0, stop:false});
}
function sendDir(dir) {
  const map = {
    fwd:  {speed:currentSpeed,  direction:0,  stop:false},
    bwd:  {speed:-currentSpeed, direction:0,  stop:false},
    left: {speed:currentSpeed,  direction:-1, stop:false},
    right:{speed:currentSpeed,  direction:1,  stop:false},
    stop: {speed:0,             direction:0,  stop:true},
  };
  if (map[dir]) wsSend({type:'vehicle', ...map[dir]});
}
 
function sendServo(ch, val) {
  document.getElementById('v'+ch).textContent = pwmToDeg(+val)+'°';
  servoPending[ch] = val;
  if (!servoTimer) servoTimer = setTimeout(() => {
    Object.entries(servoPending).forEach(([c,v]) =>
      wsSend({type:'servo', ch:+c, val:+v}));
    servoPending={}; servoTimer=null;
  }, 20);
}
function armCmd(c) {
  if (c==='home') [[0,330],[1,150],[2,300],[3,410]].forEach(([ch,v]) => {
    document.getElementById('s'+ch).value = v;
    document.getElementById('v'+ch).textContent = pwmToDeg(v)+'°';
  });
  wsSend({type:'cmd', cmd:c});
}
function wsSend(obj) {
  if (ws && ws.readyState===1) ws.send(JSON.stringify(obj));
}
connect();
)rawliteral";

