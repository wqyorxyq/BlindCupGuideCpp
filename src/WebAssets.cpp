#include "blind_cup_guide/WebAssets.h"

namespace bcg::web_assets {

std::string indexHtml() {
    return R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Blind Cup Guide</title>
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <header class="topbar">
    <div>
      <p class="eyebrow">Blind Cup Guide</p>
      <h1>盲人杯子辅助抓取</h1>
    </div>
    <div id="connectionState" class="state-pill state-idle">未连接</div>
  </header>

  <main class="workspace">
    <section class="live-panel" aria-label="实时画面">
      <div class="preview-frame">
        <img id="preview" src="/snapshot.jpg" alt="实时摄像头画面">
      </div>
      <div id="instructionText" class="instruction instruction-idle" aria-live="polite">等待开始</div>
    </section>

    <aside class="controls-panel" aria-label="操作控制">
      <div class="control-row">
        <button id="startButton" class="primary-button" type="button">开始识别</button>
        <button id="stopButton" class="secondary-button" type="button">停止</button>
        <button id="testSpeechButton" class="tertiary-button" type="button">测试语音</button>
      </div>

      <form id="settingsForm" class="settings-form">
        <label class="field">
          <span>摄像头编号</span>
          <input id="cameraIndex" name="cameraIndex" type="number" min="0" step="1" value="0">
        </label>

        <label class="field">
          <span>置信度阈值</span>
          <input id="confidenceThreshold" name="confidenceThreshold" type="number" min="0.05" max="0.95" step="0.05" value="0.35">
        </label>

        <label class="field">
          <span>推理间隔(ms)</span>
          <input id="inferenceIntervalMs" name="inferenceIntervalMs" type="number" min="100" max="3000" step="50" value="650">
        </label>

        <label class="field">
          <span>预览间隔(ms)</span>
          <input id="previewIntervalMs" name="previewIntervalMs" type="number" min="100" max="3000" step="50" value="220">
        </label>

        <label class="field wide">
          <span>模型路径</span>
          <input id="modelPath" name="modelPath" type="text" value="models/cup_detector.onnx">
        </label>

        <label class="field wide">
          <span>标签路径</span>
          <input id="labelsPath" name="labelsPath" type="text" value="models/labels.txt">
        </label>

        <div class="toggle-grid">
          <label class="toggle-line">
            <input id="useMockDetector" name="useMockDetector" type="checkbox">
            <span>模拟检测</span>
          </label>
          <label class="toggle-line">
            <input id="speechEnabled" name="speechEnabled" type="checkbox" checked>
            <span>语音播报</span>
          </label>
          <label class="toggle-line">
            <input id="consoleSpeech" name="consoleSpeech" type="checkbox">
            <span>控制台播报</span>
          </label>
          <label class="toggle-line">
            <input id="keepRunningAfterSuccess" name="keepRunningAfterSuccess" type="checkbox" checked>
            <span>成功后继续</span>
          </label>
        </div>
      </form>

      <div class="status-grid">
        <div>
          <span class="metric-label">模式</span>
          <strong id="modeValue">stopped</strong>
        </div>
        <div>
          <span class="metric-label">帧数</span>
          <strong id="frameCount">0</strong>
        </div>
        <div>
          <span class="metric-label">画面</span>
          <strong id="frameSize">0 x 0</strong>
        </div>
        <div>
          <span class="metric-label">识别数</span>
          <strong id="detectionCount">0</strong>
        </div>
      </div>
    </aside>
  </main>

  <section class="detections-panel" aria-label="识别结果">
    <div class="section-heading">
      <h2>识别结果</h2>
      <span id="lastError" class="error-text"></span>
    </div>
    <div class="table-wrap">
      <table>
        <thead>
          <tr>
            <th>标签</th>
            <th>角色</th>
            <th>置信度</th>
            <th>位置</th>
          </tr>
        </thead>
        <tbody id="detectionsBody">
          <tr><td colspan="4" class="empty-row">暂无结果</td></tr>
        </tbody>
      </table>
    </div>
  </section>

  <script src="/app.js"></script>
</body>
</html>)HTML";
}

std::string stylesCss() {
    return R"CSS(:root {
  color-scheme: light;
  --bg: #f6f7f4;
  --surface: #ffffff;
  --ink: #171a16;
  --muted: #667064;
  --line: #d9ded5;
  --accent: #176f4d;
  --accent-strong: #0f5d40;
  --warning: #a76100;
  --danger: #b42318;
  --ok-bg: #e5f4ec;
  --warn-bg: #fff2d8;
  --danger-bg: #fde8e4;
  --shadow: 0 12px 34px rgba(35, 45, 30, 0.12);
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  min-width: 320px;
  font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI", Arial, sans-serif;
  background: var(--bg);
  color: var(--ink);
}

.topbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 18px 24px;
  border-bottom: 1px solid var(--line);
  background: var(--surface);
}

.eyebrow {
  margin: 0 0 2px;
  color: var(--muted);
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0;
  text-transform: uppercase;
}

h1,
h2 {
  margin: 0;
  letter-spacing: 0;
}

h1 {
  font-size: 24px;
  line-height: 1.2;
}

h2 {
  font-size: 18px;
}

.state-pill {
  min-width: 96px;
  padding: 8px 12px;
  border: 1px solid var(--line);
  border-radius: 8px;
  text-align: center;
  font-weight: 700;
  background: #f0f2ee;
  white-space: nowrap;
}

.state-running {
  color: var(--accent-strong);
  background: var(--ok-bg);
  border-color: #9fd4bc;
}

.state-error {
  color: var(--danger);
  background: var(--danger-bg);
  border-color: #f3afa5;
}

.workspace {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 360px;
  gap: 18px;
  padding: 18px 24px;
}

.live-panel,
.controls-panel,
.detections-panel {
  background: var(--surface);
  border: 1px solid var(--line);
  border-radius: 8px;
  box-shadow: var(--shadow);
}

.live-panel {
  display: grid;
  grid-template-rows: minmax(280px, 1fr) auto;
  min-height: 540px;
  overflow: hidden;
}

.preview-frame {
  display: grid;
  place-items: center;
  min-height: 0;
  background: #11140f;
}

.preview-frame img {
  display: block;
  width: 100%;
  height: 100%;
  min-height: 280px;
  max-height: 68vh;
  object-fit: contain;
}

.instruction {
  min-height: 76px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
  font-size: 28px;
  font-weight: 800;
  line-height: 1.25;
  text-align: center;
  border-top: 1px solid var(--line);
}

.instruction-ok {
  color: var(--accent-strong);
  background: var(--ok-bg);
}

.instruction-warn {
  color: var(--warning);
  background: var(--warn-bg);
}

.instruction-error {
  color: var(--danger);
  background: var(--danger-bg);
}

.instruction-idle {
  color: var(--muted);
  background: #f0f2ee;
}

.controls-panel {
  padding: 16px;
}

.control-row {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 10px;
  margin-bottom: 16px;
}

button {
  height: 44px;
  border-radius: 8px;
  border: 1px solid transparent;
  font: inherit;
  font-weight: 800;
  cursor: pointer;
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.56;
}

.primary-button {
  color: #ffffff;
  background: var(--accent);
}

.primary-button:hover:not(:disabled) {
  background: var(--accent-strong);
}

.secondary-button {
  color: var(--danger);
  background: #ffffff;
  border-color: #e6ada5;
}

.secondary-button:hover:not(:disabled) {
  background: var(--danger-bg);
}

.tertiary-button {
  color: var(--accent-strong);
  background: #ffffff;
  border-color: #a8d1bf;
}

.tertiary-button:hover:not(:disabled) {
  background: var(--ok-bg);
}

.settings-form {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.field {
  display: grid;
  gap: 6px;
}

.field.wide {
  grid-column: 1 / -1;
}

.field span,
.toggle-line span,
.metric-label {
  color: var(--muted);
  font-size: 13px;
  font-weight: 700;
}

input[type="number"],
input[type="text"] {
  width: 100%;
  height: 38px;
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 8px 10px;
  font: inherit;
  background: #fbfcfa;
}

input:focus {
  outline: 2px solid #94cdb6;
  outline-offset: 1px;
}

.toggle-grid {
  grid-column: 1 / -1;
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  padding-top: 4px;
}

.toggle-line {
  display: flex;
  align-items: center;
  gap: 8px;
  min-height: 34px;
}

.toggle-line input {
  width: 18px;
  height: 18px;
  accent-color: var(--accent);
}

.status-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 16px;
}

.status-grid > div {
  min-height: 68px;
  padding: 10px;
  border: 1px solid var(--line);
  border-radius: 8px;
  background: #fbfcfa;
}

.status-grid strong {
  display: block;
  margin-top: 7px;
  font-size: 18px;
  line-height: 1.1;
  overflow-wrap: anywhere;
}

.detections-panel {
  margin: 0 24px 24px;
  overflow: hidden;
}

.section-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 14px 16px;
  border-bottom: 1px solid var(--line);
}

.error-text {
  color: var(--danger);
  font-weight: 700;
  text-align: right;
  overflow-wrap: anywhere;
}

.table-wrap {
  overflow-x: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  min-width: 620px;
}

th,
td {
  padding: 12px 16px;
  border-bottom: 1px solid var(--line);
  text-align: left;
  vertical-align: middle;
}

th {
  color: var(--muted);
  font-size: 13px;
  background: #fbfcfa;
}

td {
  font-size: 14px;
}

.empty-row {
  color: var(--muted);
  text-align: center;
}

@media (max-width: 980px) {
  .workspace {
    grid-template-columns: 1fr;
  }

  .live-panel {
    min-height: 420px;
  }

  .preview-frame img {
    max-height: 56vh;
  }
}

@media (max-width: 620px) {
  .topbar {
    align-items: flex-start;
    flex-direction: column;
    padding: 16px;
  }

  .workspace {
    padding: 12px;
    gap: 12px;
  }

  .detections-panel {
    margin: 0 12px 12px;
  }

  .settings-form,
  .toggle-grid,
  .status-grid,
  .control-row {
    grid-template-columns: 1fr;
  }

  .instruction {
    font-size: 22px;
  }
}
)CSS";
}

std::string appJs() {
    return R"JS(const $ = (id) => document.getElementById(id);

const els = {
  form: $("settingsForm"),
  start: $("startButton"),
  stop: $("stopButton"),
  testSpeech: $("testSpeechButton"),
  preview: $("preview"),
  connection: $("connectionState"),
  instruction: $("instructionText"),
  mode: $("modeValue"),
  frameCount: $("frameCount"),
  frameSize: $("frameSize"),
  detectionCount: $("detectionCount"),
  detectionsBody: $("detectionsBody"),
  lastError: $("lastError")
};

let settingsHydrated = false;
let statusInFlight = false;
let previewInFlight = false;

function formData() {
  const data = new URLSearchParams();
  for (const element of els.form.elements) {
    if (!element.name) continue;
    if (element.type === "checkbox") {
      data.set(element.name, element.checked ? "true" : "false");
    } else {
      data.set(element.name, element.value);
    }
  }
  return data;
}

function hydrateSettings(settings) {
  if (settingsHydrated || !settings) return;
  $("cameraIndex").value = settings.cameraIndex ?? 0;
  $("confidenceThreshold").value = settings.confidenceThreshold ?? 0.35;
  $("inferenceIntervalMs").value = settings.inferenceIntervalMs ?? 650;
  $("previewIntervalMs").value = settings.previewIntervalMs ?? 220;
  $("modelPath").value = settings.modelPath || "models/cup_detector.onnx";
  $("labelsPath").value = settings.labelsPath || "models/labels.txt";
  $("useMockDetector").checked = Boolean(settings.useMockDetector);
  $("speechEnabled").checked = Boolean(settings.speechEnabled);
  $("consoleSpeech").checked = Boolean(settings.consoleSpeech);
  $("keepRunningAfterSuccess").checked = Boolean(settings.keepRunningAfterSuccess);
  settingsHydrated = true;
}

async function postForm(path) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: formData()
  });
  if (!response.ok) {
    throw new Error(await response.text());
  }
  return response.json();
}

function setInstruction(state) {
  els.instruction.className = "instruction";
  if (!state.running) {
    els.instruction.classList.add("instruction-idle");
    els.instruction.textContent = state.error || "等待开始";
    return;
  }

  const text = state.instructionText || "识别中";
  els.instruction.textContent = text;

  if (state.instructionKind === "correct") {
    els.instruction.classList.add("instruction-ok");
  } else if (state.error || state.instructionKind === "cupNotFound") {
    els.instruction.classList.add("instruction-error");
  } else {
    els.instruction.classList.add("instruction-warn");
  }
}

function setConnection(state) {
  els.connection.className = "state-pill";
  if (state.error) {
    els.connection.classList.add("state-error");
    els.connection.textContent = "异常";
  } else if (state.running) {
    els.connection.classList.add("state-running");
    els.connection.textContent = "运行中";
  } else {
    els.connection.classList.add("state-idle");
    els.connection.textContent = "已停止";
  }
}

function renderDetections(detections) {
  if (!detections || detections.length === 0) {
    els.detectionsBody.innerHTML = '<tr><td colspan="4" class="empty-row">暂无结果</td></tr>';
    return;
  }

  els.detectionsBody.innerHTML = detections.map((item) => {
    const confidence = `${Math.round(item.confidence * 100)}%`;
    const box = `${item.x}, ${item.y}, ${item.width} x ${item.height}`;
    return `<tr>
      <td>${escapeHtml(item.label)}</td>
      <td>${escapeHtml(item.role)}</td>
      <td>${confidence}</td>
      <td>${box}</td>
    </tr>`;
  }).join("");
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function applyStatus(state) {
  hydrateSettings(state.settings);
  setConnection(state);
  setInstruction(state);
  els.mode.textContent = state.mode || "stopped";
  els.frameCount.textContent = state.frameCount ?? 0;
  els.frameSize.textContent = `${state.frameWidth ?? 0} x ${state.frameHeight ?? 0}`;
  els.detectionCount.textContent = state.detections ? state.detections.length : 0;
  els.lastError.textContent = state.error || "";
  els.start.disabled = Boolean(state.running);
  els.stop.disabled = !state.running;
  renderDetections(state.detections);
}

async function refreshStatus() {
  if (statusInFlight) return;
  statusInFlight = true;
  try {
    const response = await fetch("/status", { cache: "no-store" });
    applyStatus(await response.json());
  } catch (error) {
    applyStatus({ running: false, error: "连接后端失败", detections: [] });
  } finally {
    statusInFlight = false;
  }
}

function refreshPreview() {
  if (previewInFlight) return;
  previewInFlight = true;
  els.preview.src = `/snapshot.jpg?t=${Date.now()}`;
  setTimeout(() => {
    previewInFlight = false;
  }, 900);
}

els.preview.addEventListener("load", () => {
  previewInFlight = false;
});

els.preview.addEventListener("error", () => {
  previewInFlight = false;
});

els.start.addEventListener("click", async () => {
  els.start.disabled = true;
  try {
    applyStatus(await postForm("/api/start"));
  } catch (error) {
    els.lastError.textContent = error.message;
  }
});

els.stop.addEventListener("click", async () => {
  els.stop.disabled = true;
  try {
    const response = await fetch("/api/stop", { method: "POST" });
    applyStatus(await response.json());
  } catch (error) {
    els.lastError.textContent = error.message;
  }
});

els.testSpeech.addEventListener("click", async () => {
  els.testSpeech.disabled = true;
  try {
    const response = await fetch("/api/test-speech", { method: "POST" });
    applyStatus(await response.json());
  } catch (error) {
    els.lastError.textContent = error.message;
  } finally {
    els.testSpeech.disabled = false;
  }
});

els.form.addEventListener("change", async () => {
  try {
    const response = await fetch("/api/settings", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: formData()
    });
    applyStatus(await response.json());
  } catch (error) {
    els.lastError.textContent = error.message;
  }
});

setInterval(refreshStatus, 1200);
setInterval(refreshPreview, 550);
refreshStatus();
refreshPreview();
)JS";
}

} // namespace bcg::web_assets
