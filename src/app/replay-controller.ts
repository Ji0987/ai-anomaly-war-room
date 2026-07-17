import { STAGE_IDS } from './state'
import { currentStageId, navigate } from './router'

// 90 秒導覽:四頁自動推進(每頁停留秒數可調),隨時可暫停
const STAGE_SECONDS = [15, 20, 30, 25]

let timer: number | null = null

export function isPlaying(): boolean {
  return timer !== null
}

export function play(onTick?: () => void): void {
  stop()
  const advance = () => {
    const idx = STAGE_IDS.indexOf(currentStageId())
    if (idx >= STAGE_IDS.length - 1) {
      stop()
      onTick?.()
      return
    }
    navigate(STAGE_IDS[idx + 1])
    onTick?.()
    schedule(idx + 1)
  }
  const schedule = (idx: number) => {
    timer = window.setTimeout(advance, STAGE_SECONDS[idx] * 1000)
  }
  schedule(STAGE_IDS.indexOf(currentStageId()))
  onTick?.()
}

export function stop(): void {
  if (timer !== null) {
    clearTimeout(timer)
    timer = null
  }
}
