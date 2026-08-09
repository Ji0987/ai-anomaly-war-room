import evt001 from './event-001.json'
import evt002 from './event-002-normal-noise.json'
import evt003 from './event-003-sensor-dropout.json'
import evt004 from './event-004-suspected-false-positive.json'
import evt005 from './event-005-repeat-alert-suppressed.json'

export interface ScenarioOption {
  id: string
  label: string
  // 個別情境 JSON 的欄位形狀不完全相同(如 EVT-003 電流訊號離線,以 null 標記缺值),
  // 一律當 unknown 存,實際存取一律經 schema.ts 的 runtime 驗證,不依賴 TS 由字面量推斷的形狀。
  data: unknown
}

// 建置時內嵌五種情境(呼應 architecture.md「無 runtime fetch」決策),
// 不只有單一戲劇化的成功案例:確診異常 / 正常巡檢 / 感測缺值 / 疑似誤報 / 重複告警抑制。
export const SCENARIO_OPTIONS: ScenarioOption[] = [
  { id: 'EVT-001', label: 'EVT-001・確診異常|換料後模具溫度漂移', data: evt001 },
  { id: 'EVT-002', label: 'EVT-002・正常巡檢|訊號未跨過門檻', data: evt002 },
  { id: 'EVT-003', label: 'EVT-003・感測缺值|電流感測器離線,證據不完整', data: evt003 },
  { id: 'EVT-004', label: 'EVT-004・疑似誤報|人工複判退回', data: evt004 },
  { id: 'EVT-005', label: 'EVT-005・重複告警抑制|遲滯窗口內第二次評分不重複派工', data: evt005 },
]

export const DEFAULT_SCENARIO_ID = 'EVT-001'

export function findScenarioOption(id: string): ScenarioOption {
  return SCENARIO_OPTIONS.find((option) => option.id === id) ?? SCENARIO_OPTIONS[0]
}
