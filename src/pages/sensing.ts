import { SCENARIO_META, stageOrNull } from '../app/state'
import { isBaselineStage, isSensingStage } from '../app/schema'
import { renderDataError } from '../app/error-guard'
import { escapeHtml } from '../app/html'
import { renderTwin, uninstrumentedStations } from '../components/twin'

// 異常事件頁——邊緣評分不是裝飾數字,規則命中明細、時間窗、
// 感測器品質係數與遲滯/抑制規則都要能被追問(P0 可解釋性要求)。
export function renderSensing(root: HTMLElement): void {
  const stage = stageOrNull('sensing', isSensingStage)
  if (!stage || !SCENARIO_META) {
    renderDataError(root, '感測階段的 signals 或 edge 資料缺漏或格式不正確,請確認情境資料。')
    return
  }
  const s = stage.signals
  const e = stage.edge
  const crossed = e.score >= e.threshold
  const baseline = stageOrNull('baseline', isBaselineStage)
  const tempDelta = baseline ? (s.temperature_c - baseline.signals.temperature_c).toFixed(1) : null
  const kpiState = (signal: string) => {
    const hit = e.ruleHits.find((ruleHit) => ruleHit.signal === signal)
    const warn = hit?.triggered === true
    const arrow = warn ? (hit.observed >= hit.baseline ? ' ↑' : ' ↓') : ''
    return { warn, arrow }
  }
  const temperatureKpi = kpiState('temperature_c')
  const vibrationKpi = kpiState('vibration_rms')
  const currentKpi = kpiState('current_a')
  const yieldWarn = crossed
  const yieldArrow = crossed && baseline && stage.yield < baseline.yield ? ' ↓' : ''

  const trendText = e.trend.map((t) => `${escapeHtml(t.time)} ${t.temperature_c}°C`).join(' → ')

  const ruleRows = e.ruleHits
    .map(
      (hit) => `
      <tr class="${hit.triggered ? 'rule-hit' : ''}">
        <td>${escapeHtml(hit.label)}</td>
        <td>${hit.observed}(基線 ${hit.baseline})</td>
        <td>${hit.ruleThreshold}</td>
        <td>權重 ${hit.weight}</td>
        <td>${hit.triggered ? '✅ 觸發' : '—'}</td>
        <td class="rule-note">${escapeHtml(hit.note)}</td>
      </tr>`,
    )
    .join('')

  root.innerHTML = `
    <section class="stage stage-${stage.status}">
      <h1>${escapeHtml(stage.label)}</h1>
      <p class="meta">${escapeHtml(SCENARIO_META.line)}|${escapeHtml(SCENARIO_META.station)}|${escapeHtml(stage.time)}|來源:${escapeHtml(s.source)}</p>
      <div class="kpi-row">
        <div class="kpi${yieldWarn ? ' warn' : ''}"><span class="kpi-value">${stage.yield}%</span><span class="kpi-label">良率${yieldArrow}</span></div>
        <div class="kpi${temperatureKpi.warn ? ' warn' : ''}"><span class="kpi-value">${s.temperature_c}°C</span><span class="kpi-label">模具溫度${temperatureKpi.arrow}</span></div>
        <div class="kpi${vibrationKpi.warn ? ' warn' : ''}"><span class="kpi-value">${s.vibration_rms}</span><span class="kpi-label">振動 RMS${vibrationKpi.arrow}</span></div>
        <div class="kpi${currentKpi.warn ? ' warn' : ''}"><span class="kpi-value">${s.current_a}A</span><span class="kpi-label">電流${currentKpi.arrow}</span></div>
      </div>
      <div id="twin-mount" class="twin" role="group" aria-label="產線數位孿生視圖,可點選站點查看訊號"></div>
      <div class="edge-card">
        <h2>邊緣異常評分 — 可解釋規則告警</h2>
        <p>裝置 ${escapeHtml(e.deviceId)}|評分 <strong>${e.score}</strong>(門檻 ${e.threshold})→ ${crossed ? '觸發告警' : '未觸發'}</p>
        <p class="note">時間窗 ${e.windowSeconds} 秒｜遲滯 ${e.hysteresisSeconds} 秒｜${e.suppressed ? '本次已被抑制,不重複派發' : '未被抑制'}｜抑制規則:${escapeHtml(e.suppressionRule)}</p>
        <p class="note">溫度趨勢:${trendText}</p>
        <div class="rule-table-wrap">
          <table class="rule-table">
            <thead>
              <tr><th>訊號</th><th>觀測值</th><th>規則門檻</th><th>權重</th><th>是否觸發</th><th>備註</th></tr>
            </thead>
            <tbody>${ruleRows}</tbody>
          </table>
        </div>
        <p class="note">計分方式:${escapeHtml(e.scoring.method)}｜已觸發權重合計 ${e.scoring.rawTriggeredWeightSum} × 感測器品質係數 ${e.scoring.sensorQuality} = ${e.score}</p>
        <p class="note">感測器品質係數說明:${escapeHtml(e.scoring.sensorQualityNote)}</p>
      </div>
    </section>`

  renderTwin(root.querySelector('#twin-mount')!, [
    {
      dotId: 'dot-machine',
      name: SCENARIO_META.station,
      status: stage.status,
      detail: `模具溫度 ${s.temperature_c}°C${tempDelta ? `(較基線 ${Number(tempDelta) >= 0 ? '+' : ''}${tempDelta}°C)` : ''}｜邊緣評分 ${e.score}(門檻 ${e.threshold})→ ${crossed ? '觸發告警' : '未觸發'}`,
    },
    ...uninstrumentedStations(),
  ])
}
