import './styles/main.css'
import { SCENARIO, STAGE_IDS, resetState } from './app/state'
import { register, startRouter, navigate, currentStageId } from './app/router'
import { play, stop, isPlaying } from './app/replay-controller'
import { renderBaseline } from './pages/baseline'
import { renderSensing } from './pages/sensing'
import { renderDiagnosis } from './pages/diagnosis'
import { renderDecision } from './pages/decision'

register('baseline', renderBaseline)
register('sensing', renderSensing)
register('diagnosis', renderDiagnosis)
register('decision', renderDecision)

const app = document.getElementById('app')!
const nav = document.getElementById('nav')!
const badge = document.getElementById('event-badge')!
const btnPlay = document.getElementById('btn-play') as HTMLButtonElement
const btnReset = document.getElementById('btn-reset') as HTMLButtonElement

badge.textContent = `${SCENARIO.eventId}|${SCENARIO.summary}`

const NAV_LABELS: Record<string, string> = {
  baseline: '1 正常基線',
  sensing: '2 異常事件',
  diagnosis: '3 AI 診斷',
  decision: '4 根因閉環',
}

function renderNav(activeId: string): void {
  nav.innerHTML = STAGE_IDS.map(
    (id) =>
      `<button type="button" class="nav-btn${id === activeId ? ' active' : ''}" data-stage="${id}">${NAV_LABELS[id]}</button>`,
  ).join('')
  nav.querySelectorAll<HTMLButtonElement>('.nav-btn').forEach((btn) =>
    btn.addEventListener('click', () => {
      stop()
      updatePlayButton()
      navigate(btn.dataset.stage!)
    }),
  )
}

function updatePlayButton(): void {
  btnPlay.textContent = isPlaying() ? '⏸ 暫停導覽' : '▶ 90 秒導覽'
}

btnPlay.addEventListener('click', () => {
  if (isPlaying()) {
    stop()
  } else {
    play(updatePlayButton)
  }
  updatePlayButton()
})

btnReset.addEventListener('click', () => {
  stop()
  resetState()
  updatePlayButton()
  navigate(STAGE_IDS[0])
  if (currentStageId() === STAGE_IDS[0]) {
    // 已在第一頁時 hashchange 不會觸發,手動重繪
    window.dispatchEvent(new HashChangeEvent('hashchange'))
  }
})

startRouter(app, renderNav)
