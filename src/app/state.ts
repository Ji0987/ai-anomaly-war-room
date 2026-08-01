import { DEFAULT_SCENARIO_ID, SCENARIO_OPTIONS, findScenarioOption } from '../data/scenarios'
import { STAGE_ID_ORDER, findRawStage, isScenarioMeta, type ScenarioMeta, type StageId } from './schema'

export { STAGE_ID_ORDER }
export const STAGE_IDS: StageId[] = STAGE_ID_ORDER

// 情境切換走 URL query(?event=EVT-00x),仍是建置時內嵌的靜態資料、
// 只是換一份已內嵌好的 JSON,不违反「無 runtime fetch」的離線決策。
export function currentScenarioId(): string {
  const params = new URLSearchParams(location.search)
  const id = params.get('event')
  return SCENARIO_OPTIONS.some((option) => option.id === id) ? (id as string) : DEFAULT_SCENARIO_ID
}

export function setScenario(id: string): void {
  const params = new URLSearchParams(location.search)
  if (id === DEFAULT_SCENARIO_ID) params.delete('event')
  else params.set('event', id)
  const query = params.toString()
  location.search = query
}

const ACTIVE_OPTION = findScenarioOption(currentScenarioId())
export const RAW_SCENARIO: unknown = ACTIVE_OPTION.data
export const SCENARIO_META: ScenarioMeta | null = isScenarioMeta(RAW_SCENARIO) ? RAW_SCENARIO : null

// 以 stage id 查找取代 `stages[0]`~`stages[3]` 的固定陣列索引;
// 呼叫端自行帶入對應的型別守衛(見 schema.ts),拿不到就回傳 null 交給頁面顯示防呆訊息。
export function stageOrNull<T>(id: StageId, guard: (value: unknown) => value is T): T | null {
  const raw = findRawStage(RAW_SCENARIO, id)
  return guard(raw) ? raw : null
}

const STATE_KEY = `war-room-state:${ACTIVE_OPTION.id}`

export interface AppState {
  stageIndex: number
  delayMinutes: number
}

export function loadState(): AppState {
  try {
    const raw = localStorage.getItem(STATE_KEY)
    if (raw) return JSON.parse(raw) as AppState
  } catch {
    /* 空資料或損毀時回到初始狀態 */
  }
  return { stageIndex: 0, delayMinutes: 0 }
}

export function saveState(state: AppState): void {
  localStorage.setItem(STATE_KEY, JSON.stringify(state))
}

export function resetState(): AppState {
  localStorage.removeItem(STATE_KEY)
  return { stageIndex: 0, delayMinutes: 0 }
}
