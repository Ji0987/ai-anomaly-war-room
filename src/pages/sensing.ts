import { SCENARIO } from '../app/state'
import { isRecord, renderDataError } from '../app/error-guard'

// 工作包 B:異常事件頁(feature/twin-alert)
export function renderSensing(root: HTMLElement): void {
  const stage = SCENARIO.stages[1] as Record<string, unknown> | undefined
  const s = stage?.signals
  const e = stage?.edge
  if (!stage || typeof stage.label !== 'string' || typeof stage.time !== 'string' || typeof stage.yield !== 'number' || !isRecord(s) || typeof s.temperature_c !== 'number' || typeof s.vibration_rms !== 'number' || typeof s.current_a !== 'number' || !isRecord(e) || typeof e.deviceId !== 'string' || typeof e.score !== 'number' || typeof e.threshold !== 'number' || typeof e.source !== 'string') {
    renderDataError(root, '感測階段的 signals 或 edge 資料缺漏或格式不正確，請確認情境資料。')
    return
  }
  root.innerHTML = `
    <section class="stage stage-warning">
      <h1>${stage.label}</h1>
      <p class="meta">${SCENARIO.line}|${SCENARIO.station}|${stage.time}|來源:${e.source}</p>
      <div class="kpi-row">
        <div class="kpi warn"><span class="kpi-value">${stage.yield}%</span><span class="kpi-label">良率 ↓</span></div>
        <div class="kpi warn"><span class="kpi-value">${s.temperature_c}°C</span><span class="kpi-label">模具溫度 ↑</span></div>
        <div class="kpi"><span class="kpi-value">${s.vibration_rms}</span><span class="kpi-label">振動 RMS</span></div>
        <div class="kpi warn"><span class="kpi-value">${s.current_a}A</span><span class="kpi-label">電流 ↑</span></div>
      </div>
      <div class="edge-card">
        <h2>邊緣異常評分</h2>
        <p>裝置 ${e.deviceId}|評分 <strong>${e.score}</strong>(門檻 ${e.threshold})→ 觸發告警</p>
      </div>
      <div id="twin-placeholder" class="twin">
        <p>產線數位孿生視圖(三號機台轉黃)</p>
      </div>
    </section>`
}
