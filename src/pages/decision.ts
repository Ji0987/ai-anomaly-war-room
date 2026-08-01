import { SCENARIO, loadState, saveState } from '../app/state'
import { renderTwin } from '../components/twin'

// 工作包 D:根因閉環 + 提前預警損失 what-if slider(feature/decision-loss)
// 避免損失 = 縮短的延遲分鐘 × 每分鐘產量 × 缺陷率差 × 單位不良成本
export function computeLoss(delayMinutes: number): number {
  const l = SCENARIO.stages[3].loss!
  return Math.round(delayMinutes * l.unitsPerMinute * l.defectRateDelta * l.costPerDefect)
}

export function renderDecision(root: HTMLElement): void {
  const stage = SCENARIO.stages[3]
  const state = loadState()
  root.innerHTML = `
    <section class="stage stage-resolved">
      <h1>${stage.label}</h1>
      <p class="meta">${SCENARIO.line}|${SCENARIO.station}|${stage.time}|來源:${stage.loss!.source}</p>
      <div id="twin-mount" class="twin twin-mini" role="img" aria-label="產線數位孿生視圖"></div>
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
  renderTwin(root.querySelector('#twin-mount')!, stage.status)

  const slider = root.querySelector<HTMLInputElement>('#delay-slider')!
  slider.addEventListener('input', () => {
    const minutes = Number(slider.value)
    root.querySelector('#delay-value')!.textContent = String(minutes)
    root.querySelector('#loss-value')!.textContent = `NT$ ${computeLoss(minutes).toLocaleString()}`
    saveState({ ...loadState(), delayMinutes: minutes })
  })
}
