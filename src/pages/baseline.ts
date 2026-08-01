import { SCENARIO } from '../app/state'
import { renderTwin } from '../components/twin'

export function renderBaseline(root: HTMLElement): void {
  const stage = SCENARIO.stages[0]
  const s = stage.signals!
  root.innerHTML = `
    <section class="stage stage-normal">
      <h1>${stage.label}</h1>
      <p class="meta">${SCENARIO.line}|${SCENARIO.station}|${stage.time}</p>
      <div class="kpi-row">
        <div class="kpi"><span class="kpi-value">${stage.yield}%</span><span class="kpi-label">良率</span></div>
        <div class="kpi"><span class="kpi-value">${s.temperature_c}°C</span><span class="kpi-label">模具溫度</span></div>
        <div class="kpi"><span class="kpi-value">${s.vibration_rms}</span><span class="kpi-label">振動 RMS</span></div>
        <div class="kpi"><span class="kpi-value">${s.current_a}A</span><span class="kpi-label">電流</span></div>
      </div>
      <div id="twin-mount" class="twin" role="img" aria-label="產線數位孿生視圖"></div>
    </section>`
  renderTwin(root.querySelector('#twin-mount')!, stage.status)
}
