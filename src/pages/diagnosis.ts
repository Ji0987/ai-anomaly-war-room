import { SCENARIO } from '../app/state'

// AI 老師傅六格診斷卡
export function renderDiagnosis(root: HTMLElement): void {
  const stage = SCENARIO.stages[2]
  const c = stage.card!
  root.innerHTML = `
    <section class="stage stage-critical">
      <h1>${stage.label}</h1>
      <p class="meta">${SCENARIO.line}|${SCENARIO.station}|${stage.time}|來源:${c.source}</p>
      <div class="diagnosis-card">
        <div class="cell"><h3>推定根因</h3><p>${c.rootCause}</p></div>
        <div class="cell"><h3>信心</h3><p>${c.confidence}%</p></div>
        <div class="cell"><h3>證據</h3><ul>${c.evidence.map((e) => `<li>${e}</li>`).join('')}</ul></div>
        <div class="cell"><h3>知識依據</h3><p>${c.knowledge}</p></div>
        <div class="cell"><h3>建議處置</h3><p>${c.action}</p></div>
        <div class="cell"><h3>人工狀態</h3><p>${c.reviewStatus}</p></div>
      </div>
      <p class="governance">AI 提供可追溯建議,最終處置由現場人員確認。</p>
    </section>`
}
