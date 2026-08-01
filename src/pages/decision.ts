import { SCENARIO, loadState, saveState } from '../app/state'
import { isRecord, renderDataError } from '../app/error-guard'

// 工作包 D:根因閉環 + 提前預警損失 what-if slider(feature/decision-loss)
// 避免損失 = 縮短的延遲分鐘 × 每分鐘產量 × 缺陷率差 × 單位不良成本
export function computeLoss(delayMinutes: number): number {
  const l = (SCENARIO.stages[3] as Record<string, unknown> | undefined)?.loss
  if (!isRecord(l) || typeof l.unitsPerMinute !== 'number' || typeof l.defectRateDelta !== 'number' || typeof l.costPerDefect !== 'number') return 0
  return Math.round(delayMinutes * l.unitsPerMinute * l.defectRateDelta * l.costPerDefect)
}

export function renderDecision(root: HTMLElement): void {
  const stage = SCENARIO.stages[3] as Record<string, unknown> | undefined
  const loss = stage?.loss
  if (!stage || typeof stage.label !== 'string' || typeof stage.time !== 'string' || !isRecord(loss) || typeof loss.unitsPerMinute !== 'number' || typeof loss.defectRateDelta !== 'number' || typeof loss.costPerDefect !== 'number' || typeof loss.source !== 'string' || typeof stage.truth !== 'string') {
    renderDataError(root, '決策階段的 loss 或 truth 資料缺漏或格式不正確，請確認情境資料。')
    return
  }
  const state = loadState()
  root.innerHTML = `
    <section class="stage stage-resolved">
      <h1>${stage.label}</h1>
       <p class="meta">${SCENARIO.line}|${SCENARIO.station}|${stage.time}|來源:${loss.source}</p>
      <div class="truth-card">
        <h2>根因揭曉</h2>
        <p>${stage.truth}</p>
      </div>
      <div class="whatif">
        <h2>處置延遲 what-if</h2>
        <label for="delay-slider">延遲處置:<strong id="delay-value">${state.delayMinutes}</strong> 分鐘</label>
        <input id="delay-slider" type="range" min="0" max="30" step="1" value="${state.delayMinutes}" />
        <p>預估額外損失:<strong id="loss-value">NT$ ${computeLoss(state.delayMinutes).toLocaleString()}</strong></p>
        <p class="note">單價與產能為模擬營運假設</p>
      </div>
    </section>`

  const slider = root.querySelector<HTMLInputElement>('#delay-slider')!
  slider.addEventListener('input', () => {
    const minutes = Number(slider.value)
    root.querySelector('#delay-value')!.textContent = String(minutes)
    root.querySelector('#loss-value')!.textContent = `NT$ ${computeLoss(minutes).toLocaleString()}`
    saveState({ ...loadState(), delayMinutes: minutes })
  })
}
