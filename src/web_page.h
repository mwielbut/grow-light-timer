#pragma once

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Grow Light</title>
<style>
  :root { --bg:#f6f7f4; --card:#fff; --fg:#1d2a1f; --muted:#6b776d; --accent:#2f7d43; --on:#e8b20e; --border:#dfe4dc; }
  @media (prefers-color-scheme: dark) {
    :root { --bg:#141a15; --card:#1d251e; --fg:#e7ede8; --muted:#94a196; --accent:#5cbf73; --on:#f2c94c; --border:#2c372e; }
  }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,-apple-system,sans-serif; background:var(--bg); color:var(--fg); }
  main { max-width:420px; margin:0 auto; padding:24px 16px; }
  h1 { font-size:1.4rem; margin:0 0 4px; }
  .sub { color:var(--muted); margin:0 0 20px; font-size:.9rem; }
  .card { background:var(--card); border:1px solid var(--border); border-radius:12px; padding:16px; margin-bottom:16px; }
  .status { display:flex; align-items:center; gap:14px; }
  .bulb { width:44px; height:44px; border-radius:50%; background:var(--border); transition:background .2s, box-shadow .2s; }
  .bulb.on { background:var(--on); box-shadow:0 0 24px var(--on); }
  .state { font-size:1.3rem; font-weight:600; }
  .muted { color:var(--muted); font-size:.85rem; }
  .row { display:flex; gap:12px; }
  .row > label { flex:1; }
  label { display:block; font-size:.85rem; color:var(--muted); }
  input[type=time] { width:100%; margin-top:4px; padding:8px; font-size:1rem; border:1px solid var(--border); border-radius:8px; background:var(--bg); color:var(--fg); }
  button { font:inherit; padding:10px 14px; border-radius:8px; border:1px solid var(--border); background:var(--bg); color:var(--fg); cursor:pointer; }
  button.primary { background:var(--accent); border-color:var(--accent); color:#fff; width:100%; margin-top:12px; }
  .buttons { display:flex; gap:8px; flex-wrap:wrap; margin-top:12px; }
  .buttons button { flex:1; }
  #msg { min-height:1.2em; margin-top:8px; }
</style>
</head>
<body>
<main>
  <h1>Grow Light</h1>
  <p class="sub">Fiddle leaf fig</p>

  <div class="card">
    <div class="status">
      <div id="bulb" class="bulb"></div>
      <div>
        <div id="state" class="state">&hellip;</div>
        <div id="detail" class="muted"></div>
      </div>
    </div>
    <div class="buttons">
      <button onclick="override('on')">Turn on</button>
      <button onclick="override('off')">Turn off</button>
      <button id="resume" onclick="override('clear')">Resume schedule</button>
    </div>
    <p class="muted">Manual on/off lasts until the next scheduled change.</p>
  </div>

  <form class="card" onsubmit="save(event)">
    <div class="row">
      <label>On at <input id="onTime" type="time" required></label>
      <label>Off at <input id="offTime" type="time" required></label>
    </div>
    <div id="hours" class="muted" style="margin-top:8px"></div>
    <button class="primary" type="submit">Save schedule</button>
    <div id="msg" class="muted"></div>
  </form>

  <p id="clock" class="muted"></p>
</main>
<script>
let editing = false;
const $ = id => document.getElementById(id);

function fmt12(hhmm) {
  let [h, m] = hhmm.split(':').map(Number);
  const ap = h < 12 ? 'AM' : 'PM';
  h = h % 12 || 12;
  return h + ':' + String(m).padStart(2, '0') + ' ' + ap;
}

function hoursOn() {
  const [a, b] = [$('onTime').value, $('offTime').value];
  if (!a || !b) return '';
  const toMin = t => { const [h, m] = t.split(':').map(Number); return h * 60 + m; };
  let d = (toMin(b) - toMin(a) + 1440) % 1440;
  return (d / 60).toFixed(d % 60 ? 1 : 0) + ' hours of light per day';
}

function render(s) {
  $('bulb').className = 'bulb' + (s.on ? ' on' : '');
  $('state').textContent = s.on ? 'Light on' : 'Light off';
  $('detail').textContent = s.override ? 'Manual override'
    : !s.synced ? 'Waiting for time sync - light held off' : 'Following schedule';
  $('resume').disabled = !s.override;
  $('clock').textContent = s.synced ? 'Device time: ' + s.now : 'Device time not yet synced';
  if (!editing) { $('onTime').value = s.onTime; $('offTime').value = s.offTime; }
  $('hours').textContent = hoursOn();
}

async function refresh() {
  try { render(await (await fetch('/api/status')).json()); } catch (e) {}
}

async function post(url, body) {
  const r = await fetch(url, { method: 'POST', body: new URLSearchParams(body) });
  if (!r.ok) throw new Error(await r.text());
  render(await r.json());
}

async function override(state) {
  try { await post('/api/override', { state }); } catch (e) { alert(e.message); }
}

async function save(e) {
  e.preventDefault();
  try {
    await post('/api/schedule', { onTime: $('onTime').value, offTime: $('offTime').value });
    editing = false;
    $('msg').textContent = 'Saved: on ' + fmt12($('onTime').value) + ', off ' + fmt12($('offTime').value);
  } catch (err) { $('msg').textContent = 'Error: ' + err.message; }
}

for (const id of ['onTime', 'offTime']) {
  $(id).addEventListener('input', () => { editing = true; $('hours').textContent = hoursOn(); $('msg').textContent = ''; });
}
refresh();
setInterval(refresh, 5000);
</script>
</body>
</html>
)HTML";
