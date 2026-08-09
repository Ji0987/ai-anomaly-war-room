import { SCENARIO_META, stageOrNull } from '../app/state'
import { isDiagnosisStage } from '../app/schema'
import { renderDataError } from '../app/error-guard'
import { escapeHtml } from '../app/html'
import { renderTwin, uninstrumentedStations } from '../components/twin'

const REVIEW_LABEL: Record<string, string> = {
  approved: '✅ 內容已審核',
  pending: '⏳ 內容待審核',
  rejected: '❌ 內容已退回',
}

const DISPOSITION_LABEL: Record<string, string> = {
  pending: '⏳ 現場待確認',
  confirmed: '✅ 現場已確認採納',
  returned_for_review: '↩ 已退回複判',
  completed: '✅ 處置已完成',
}

// AI 老師傅六格診斷卡。
// 「AI 卡內容本身是否審核過」與「這次現場事件的處置是否被人確認」是兩件不同的事,
// 拆成 contentReview / caseDisposition 兩個獨立欄位,不再混在同一個 reviewStatus 裡。
export function renderDiagnosis(root: HTMLElement): void {
  const stage = stageOrNull('diagnosis', isDiagnosisStage)
  if (!stage || !SCENARIO_META) {
    renderDataError(root, '診斷階段的 card 資料缺漏或格式不正確,請確認情境資料。')
    return
  }
  const c = stage.card

  const evidenceItems = c.evidence
    .map((item) => `<li><span class="evidence-time">${escapeHtml(item.time)}</span>${escapeHtml(item.description)}</li>`)
    .join('')

  const contentUnreviewed = c.contentReview.status !== 'approved'

  root.innerHTML = `
    <section class="stage stage-${stage.status}">
      <h1>${escapeHtml(stage.label)}</h1>
      <p class="meta">${escapeHtml(SCENARIO_META.line)}|${escapeHtml(SCENARIO_META.station)}|${escapeHtml(stage.time)}|來源:${escapeHtml(c.source)}</p>
      <div id="twin-mount" class="twin twin-mini" role="group" aria-label="產線數位孿生視圖,可點選站點查看訊號"></div>
      ${contentUnreviewed ? `<p class="content-caveat">⚠ 本卡片內容尚未經內容審核(${REVIEW_LABEL[c.contentReview.status] ?? escapeHtml(c.contentReview.status)}),為 AI 生成之示範建議,現場採用前應由人工覆核,不視為已核准可直接引用的知識。</p>` : ''}
      <div class="diagnosis-card">
        <div class="cell"><h3>推定根因</h3><p>${escapeHtml(c.rootCause)}</p></div>
        <div class="cell"><h3>信心</h3><p>${c.confidence}%</p><p class="note">${escapeHtml(c.confidenceBasis)}</p></div>
        <div class="cell"><h3>證據(≥3 條,含時間點)</h3><ul>${evidenceItems}</ul></div>
        <div class="cell"><h3>知識依據</h3><p>${escapeHtml(c.knowledge)}</p></div>
        <div class="cell"><h3>建議處置</h3><p>${escapeHtml(c.action)}</p></div>
        <div class="cell">
          <h3>人工狀態</h3>
          <p>${REVIEW_LABEL[c.contentReview.status] ?? escapeHtml(c.contentReview.status)}${c.contentReview.reviewer ? `(${escapeHtml(c.contentReview.reviewer)})` : ''}</p>
          <p>${DISPOSITION_LABEL[c.caseDisposition.status] ?? escapeHtml(c.caseDisposition.status)}</p>
          ${c.caseDisposition.note ? `<p class="note">${escapeHtml(c.caseDisposition.note)}</p>` : ''}
        </div>
      </div>
      <p class="governance">AI 提供可追溯建議,最終處置由現場人員確認。</p>
      <p class="provenance">卡片版本 ${escapeHtml(c.provenance.cardVersion)}｜知識版本 ${escapeHtml(c.provenance.knowledgeVersion)}｜模型識別 ${escapeHtml(c.provenance.modelId)}｜提示詞識別 ${escapeHtml(c.provenance.promptId)}</p>
    </section>`

  renderTwin(root.querySelector('#twin-mount')!, [
    {
      dotId: 'dot-machine',
      name: SCENARIO_META.station,
      status: stage.status,
      detail: `AI 研判:${c.rootCause}(信心 ${c.confidence}%)`,
    },
    ...uninstrumentedStations(),
  ], { mini: true })
}
