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
  // 導覽固定從第一頁開始播放。AC-01b 要求「走完四頁、總時長 90±5 秒」,
  // 若從當前頁接續,上一輪導覽結束會停在第 4 頁,再按一次只會空轉 25 秒完全不換頁
  // (按鈕仍顯示「暫停導覽」),展示日連按兩次就會踩到。
  if (currentStageId() !== STAGE_IDS[0]) {
    navigate(STAGE_IDS[0])
  }
  const advance = () => {
    const idx = (STAGE_IDS as readonly string[]).indexOf(currentStageId())
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
  schedule(0)
  onTick?.()
}

export function stop(): void {
  if (timer !== null) {
    clearTimeout(timer)
    timer = null
  }
}
