import { SCENARIO_META, stageOrNull } from '../app/state'
import { isBaselineStage } from '../app/schema'
import { renderDataError } from '../app/error-guard'
import { escapeHtml } from '../app/html'
import { renderTwin, uninstrumentedStations } from '../components/twin'

// 工作包 A/B 共同起點:正常基線頁
export function renderBaseline(root: HTMLElement): void {
  const stage = stageOrNull('baseline', isBaselineStage)
  if (!stage || !SCENARIO_META) {
    renderDataError(root, '基準階段的 signals 資料缺漏或格式不正確,請確認情境資料。')
    return
  }
  const s = stage.signals
  root.innerHTML = `
    <section class="stage stage-normal">
      <h1>${escapeHtml(stage.label)}</h1>
      <p class="meta">${escapeHtml(SCENARIO_META.line)}|${escapeHtml(SCENARIO_META.station)}|${escapeHtml(stage.time)}|來源:${escapeHtml(s.source)}</p>
      <div class="kpi-row">
        <div class="kpi"><span class="kpi-value">${stage.yield}%</span><span class="kpi-label">良率</span></div>
        <div class="kpi"><span class="kpi-value">${s.temperature_c}°C</span><span class="kpi-label">模具溫度</span></div>
        <div class="kpi"><span class="kpi-value">${s.vibration_rms}</span><span class="kpi-label">振動 RMS</span></div>
        <div class="kpi"><span class="kpi-value">${s.current_a}A</span><span class="kpi-label">電流</span></div>
      </div>
      <div id="twin-mount" class="twin" role="img" aria-label="產線數位孿生視圖"></div>
    </section>`

  renderTwin(root.querySelector('#twin-mount')!, [
    {
      dotId: 'dot-machine',
      name: SCENARIO_META.station,
      status: stage.status,
      detail: `模具溫度 ${s.temperature_c}°C｜振動 RMS ${s.vibration_rms}｜電流 ${s.current_a}A(正常範圍)`,
    },
    ...uninstrumentedStations(),
  ])
}
