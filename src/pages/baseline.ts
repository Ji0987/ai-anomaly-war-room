import { SCENARIO } from '../app/state'
import { isRecord, renderDataError } from '../app/error-guard'

// 工作包 A/B 共同起點:正常基線頁(feature/replay-shell 與 feature/twin-alert 從這裡擴充)
export function renderBaseline(root: HTMLElement): void {
  const stage = SCENARIO.stages[0] as Record<string, unknown> | undefined
  const s = stage?.signals
  if (!stage || typeof stage.label !== 'string' || typeof stage.time !== 'string' || typeof stage.yield !== 'number' || !isRecord(s) || typeof s.temperature_c !== 'number' || typeof s.vibration_rms !== 'number' || typeof s.current_a !== 'number') {
    renderDataError(root, '基準階段的 signals 資料缺漏或格式不正確，請確認情境資料。')
    return
  }
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
      <div id="twin-placeholder" class="twin">
        <!-- TODO(工作包 B): 以 assets/factory-map.svg 取代,狀態圓點吃同一份 stage 資料 -->
        <p>產線數位孿生視圖(全站綠燈)</p>
      </div>
    </section>`
}
