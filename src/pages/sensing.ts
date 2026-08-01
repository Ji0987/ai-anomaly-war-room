import { SCENARIO } from '../app/state'
import { renderTwin } from '../components/twin'

// 工作包 B:異常事件頁(feature/twin-alert)
export function renderSensing(root: HTMLElement): void {
  const stage = SCENARIO.stages[1]
  const s = stage.signals!
  const e = stage.edge!
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
      <div id="twin-mount" class="twin" role="img" aria-label="產線數位孿生視圖"></div>
    </section>`
  renderTwin(root.querySelector('#twin-mount')!, stage.status)
}
