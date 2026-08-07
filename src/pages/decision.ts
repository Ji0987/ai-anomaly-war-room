import { SCENARIO_META, loadState, saveState, stageOrNull } from '../app/state'
import { isDecisionStage } from '../app/schema'
import { renderDataError } from '../app/error-guard'
import { escapeHtml } from '../app/html'
import { renderTwin, uninstrumentedStations } from '../components/twin'
import type { DecisionStage } from '../app/schema'

const RECOVERY_LABEL: Record<string, string> = {
  pending_recheck: '⏳ 復機驗證中(尚未確認良率回歸基線)',
  verified: '✅ 復機驗證通過',
  reopened: '🔴 已重開案,尚未結案',
}

// 工作包 D:根因閉環 + 提前預警損失 what-if slider
// 避免損失 = 縮短的延遲分鐘 × 每分鐘產量 × 缺陷率差 × 單位不良成本
export function computeLoss(stage: DecisionStage, delayMinutes: number): number {
  const l = stage.loss
  return Math.round(delayMinutes * l.unitsPerMinute * l.defectRateDelta * l.costPerDefect)
}

export function renderDecision(root: HTMLElement): void {
  const stage = stageOrNull('decision', isDecisionStage)
  if (!stage || !SCENARIO_META) {
    renderDataError(root, '決策階段的 loss 或 recovery 資料缺漏或格式不正確,請確認情境資料。')
    return
  }
  const state = loadState()

  root.innerHTML = `
    <section class="stage stage-resolved">
      <h1>${escapeHtml(stage.label)}</h1>
      <p class="meta">${escapeHtml(SCENARIO_META.line)}|${escapeHtml(SCENARIO_META.station)}|${escapeHtml(stage.time)}|來源:${escapeHtml(stage.loss.source)}</p>
      <div id="twin-mount" class="twin twin-mini" role="group" aria-label="產線數位孿生視圖,可點選站點查看訊號"></div>
      <div class="truth-card">
        <h2>根因揭曉</h2>
        <p>${escapeHtml(stage.truth)}</p>
      </div>
      <div class="recovery-card">
        <h2>復機驗證(不等於「已解決」就代表指標已回歸正常)</h2>
        <p>${RECOVERY_LABEL[stage.recovery.status] ?? escapeHtml(stage.recovery.status)}</p>
        <p class="note">目標良率 ${stage.recovery.targetYieldPct}%｜複檢窗口 ${stage.recovery.checkWindowMinutes} 分鐘｜目前觀測 ${stage.recovery.observedYieldAtCheckPct === null ? '尚未複檢' : `${stage.recovery.observedYieldAtCheckPct}%`}</p>
        <p class="note">重開案條件:${escapeHtml(stage.recovery.reopenCondition)}</p>
      </div>
      <div class="whatif">
        <h2>處置延遲 what-if</h2>
        <label for="delay-slider">延遲處置:<strong id="delay-value">${state.delayMinutes}</strong> 分鐘</label>
        <input id="delay-slider" type="range" min="0" max="30" step="1" value="${state.delayMinutes}" />
        <p>預估額外損失:<strong id="loss-value">NT$ ${computeLoss(stage, state.delayMinutes).toLocaleString()}</strong></p>
        <p class="note">單價與產能為模擬營運假設</p>
      </div>
    </section>`

  renderTwin(root.querySelector('#twin-mount')!, [
    {
      dotId: 'dot-machine',
      name: SCENARIO_META.station,
      status: stage.status,
      detail: RECOVERY_LABEL[stage.recovery.status] ?? stage.recovery.status,
    },
    ...uninstrumentedStations(),
  ], { mini: true })

  const slider = root.querySelector<HTMLInputElement>('#delay-slider')!
  slider.addEventListener('input', () => {
    const minutes = Number(slider.value)
    root.querySelector('#delay-value')!.textContent = String(minutes)
    root.querySelector('#loss-value')!.textContent = `NT$ ${computeLoss(stage, minutes).toLocaleString()}`
    saveState({ ...loadState(), delayMinutes: minutes })
  })
}
