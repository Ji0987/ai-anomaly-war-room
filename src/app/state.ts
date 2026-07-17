import scenario from '../data/scenarios/event-001.json'

export type Stage = (typeof scenario.stages)[number]

export const SCENARIO = scenario
export const STAGE_IDS = scenario.stages.map((s) => s.id)

const KEY = 'war-room-state'

export interface AppState {
  stageIndex: number
  delayMinutes: number
}

export function loadState(): AppState {
  try {
    const raw = localStorage.getItem(KEY)
    if (raw) return JSON.parse(raw) as AppState
  } catch {
    /* 空資料或損毀時回到初始狀態 */
  }
  return { stageIndex: 0, delayMinutes: 0 }
}

export function saveState(state: AppState): void {
  localStorage.setItem(KEY, JSON.stringify(state))
}

export function resetState(): AppState {
  localStorage.removeItem(KEY)
  return { stageIndex: 0, delayMinutes: 0 }
}
