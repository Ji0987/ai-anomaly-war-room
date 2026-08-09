// 事件資料的 runtime schema:情境 JSON 在編譯時只被 TS 當成字面量型別信任,
// 沒有任何執行期防呆。這裡把「頁面需要哪些欄位」明確定義成型別 + 型別守衛,
// 讓 stage 存取一律經過驗證,不再是 `SCENARIO.stages[N]!` 的裸陣列索引假設。

export type StageId = 'baseline' | 'sensing' | 'diagnosis' | 'decision'
export const STAGE_ID_ORDER: StageId[] = ['baseline', 'sensing', 'diagnosis', 'decision']

export type StageStatus = 'normal' | 'warning' | 'critical' | 'resolved'

export interface Signals {
  temperature_c: number
  vibration_rms: number
  current_a: number | null
  source: string
}

export interface SignalSample {
  time: string
  temperature_c: number
}

export interface RuleHit {
  signal: string
  label: string
  weight: number
  observed: number | null
  baseline: number
  ruleThreshold: number
  triggered: boolean
  note: string
}

export interface EdgeScoring {
  method: string
  sensorQuality: number
  sensorQualityNote: string
  rawTriggeredWeightSum: number
}

export interface EdgeAlert {
  deviceId: string
  score: number
  threshold: number
  windowSeconds: number
  hysteresisSeconds: number
  suppressed: boolean
  suppressionRule: string
  trend: SignalSample[]
  ruleHits: RuleHit[]
  scoring: EdgeScoring
  source: string
}

export interface EvidenceItem {
  time: string
  description: string
  metric: string
  value: number
}

export interface ReviewInfo {
  status: string
  reviewer: string
  reviewedAt: string
}

export interface DispositionInfo {
  status: string
  actor: string
  actedAt: string
  note: string
}

export interface Provenance {
  cardVersion: string
  knowledgeVersion: string
  modelId: string
  promptId: string
}

export interface DiagnosisCard {
  verdict: string
  rootCause: string
  confidence: number
  confidenceBasis: string
  evidence: EvidenceItem[]
  knowledge: string
  action: string
  contentReview: ReviewInfo
  caseDisposition: DispositionInfo
  provenance: Provenance
  source: string
}

export interface LossModel {
  unitsPerMinute: number
  defectRateDelta: number
  costPerDefect: number
  source: string
}

export interface RecoveryVerification {
  status: string
  targetYieldPct: number
  observedYieldAtCheckPct: number | null
  checkWindowMinutes: number
  reopenCondition: string
}

interface BaseStage {
  id: StageId
  label: string
  time: string
  yield: number
  status: StageStatus
}

export interface BaselineStage extends BaseStage {
  id: 'baseline'
  signals: Signals
}
export interface SensingStage extends BaseStage {
  id: 'sensing'
  signals: Signals
  edge: EdgeAlert
}
export interface DiagnosisStage extends BaseStage {
  id: 'diagnosis'
  card: DiagnosisCard
}
export interface DecisionStage extends BaseStage {
  id: 'decision'
  truth: string
  loss: LossModel
  recovery: RecoveryVerification
}

export interface ScenarioMeta {
  eventId: string
  scenarioType: string
  scenarioLabel: string
  line: string
  station: string
  summary: string
  materialChangeEvent: { time: string; description: string }
}

// ---- 基本型別守衛 ----
export function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null
}
const isStr = (v: unknown): v is string => typeof v === 'string'
const isNum = (v: unknown): v is number => typeof v === 'number' && Number.isFinite(v)
const isBool = (v: unknown): v is boolean => typeof v === 'boolean'
const isArr = (v: unknown): v is unknown[] => Array.isArray(v)
const isNullableNum = (v: unknown): v is number | null => v === null || isNum(v)

function isSignals(v: unknown): v is Signals {
  return isRecord(v) && isNum(v.temperature_c) && isNum(v.vibration_rms) && isNullableNum(v.current_a) && isStr(v.source)
}

function isSignalSample(v: unknown): v is SignalSample {
  return isRecord(v) && isStr(v.time) && isNum(v.temperature_c)
}

function isRuleHit(v: unknown): v is RuleHit {
  return (
    isRecord(v) &&
    isStr(v.signal) &&
    isStr(v.label) &&
    isNum(v.weight) &&
    isNullableNum(v.observed) &&
    isNum(v.baseline) &&
    isNum(v.ruleThreshold) &&
    isBool(v.triggered) &&
    isStr(v.note)
  )
}

function isEdgeScoring(v: unknown): v is EdgeScoring {
  return isRecord(v) && isStr(v.method) && isNum(v.sensorQuality) && isStr(v.sensorQualityNote) && isNum(v.rawTriggeredWeightSum)
}

function isEdgeAlert(v: unknown): v is EdgeAlert {
  return (
    isRecord(v) &&
    isStr(v.deviceId) &&
    isNum(v.score) &&
    isNum(v.threshold) &&
    isNum(v.windowSeconds) &&
    isNum(v.hysteresisSeconds) &&
    isBool(v.suppressed) &&
    isStr(v.suppressionRule) &&
    isArr(v.trend) &&
    v.trend.every(isSignalSample) &&
    isArr(v.ruleHits) &&
    v.ruleHits.every(isRuleHit) &&
    isEdgeScoring(v.scoring) &&
    isStr(v.source)
  )
}

function isEvidenceItem(v: unknown): v is EvidenceItem {
  return isRecord(v) && isStr(v.time) && isStr(v.description) && isStr(v.metric) && isNum(v.value)
}

function isReviewInfo(v: unknown): v is ReviewInfo {
  return isRecord(v) && isStr(v.status) && isStr(v.reviewer) && isStr(v.reviewedAt)
}

function isDispositionInfo(v: unknown): v is DispositionInfo {
  return isRecord(v) && isStr(v.status) && isStr(v.actor) && isStr(v.actedAt) && isStr(v.note)
}

function isProvenance(v: unknown): v is Provenance {
  return isRecord(v) && isStr(v.cardVersion) && isStr(v.knowledgeVersion) && isStr(v.modelId) && isStr(v.promptId)
}

function isDiagnosisCard(v: unknown): v is DiagnosisCard {
  return (
    isRecord(v) &&
    isStr(v.verdict) &&
    isStr(v.rootCause) &&
    isNum(v.confidence) &&
    isStr(v.confidenceBasis) &&
    isArr(v.evidence) &&
    v.evidence.length >= 3 &&
    v.evidence.every(isEvidenceItem) &&
    isStr(v.knowledge) &&
    isStr(v.action) &&
    isReviewInfo(v.contentReview) &&
    isDispositionInfo(v.caseDisposition) &&
    isProvenance(v.provenance) &&
    isStr(v.source)
  )
}

function isLossModel(v: unknown): v is LossModel {
  return isRecord(v) && isNum(v.unitsPerMinute) && isNum(v.defectRateDelta) && isNum(v.costPerDefect) && isStr(v.source)
}

function isRecoveryVerification(v: unknown): v is RecoveryVerification {
  return (
    isRecord(v) &&
    isStr(v.status) &&
    isNum(v.targetYieldPct) &&
    isNullableNum(v.observedYieldAtCheckPct) &&
    isNum(v.checkWindowMinutes) &&
    isStr(v.reopenCondition)
  )
}

function isBase(v: unknown, id: StageId): v is BaseStage {
  return (
    isRecord(v) &&
    v.id === id &&
    isStr(v.label) &&
    isStr(v.time) &&
    isNum(v.yield) &&
    (v.status === 'normal' || v.status === 'warning' || v.status === 'critical' || v.status === 'resolved')
  )
}

export function isBaselineStage(v: unknown): v is BaselineStage {
  return isBase(v, 'baseline') && isSignals((v as unknown as Record<string, unknown>).signals)
}

export function isSensingStage(v: unknown): v is SensingStage {
  return isBase(v, 'sensing') && isSignals((v as unknown as Record<string, unknown>).signals) && isEdgeAlert((v as unknown as Record<string, unknown>).edge)
}

export function isDiagnosisStage(v: unknown): v is DiagnosisStage {
  return isBase(v, 'diagnosis') && isDiagnosisCard((v as unknown as Record<string, unknown>).card)
}

export function isDecisionStage(v: unknown): v is DecisionStage {
  return (
    isBase(v, 'decision') &&
    isStr((v as unknown as Record<string, unknown>).truth) &&
    isLossModel((v as unknown as Record<string, unknown>).loss) &&
    isRecoveryVerification((v as unknown as Record<string, unknown>).recovery)
  )
}

export function isScenarioMeta(v: unknown): v is ScenarioMeta {
  return (
    isRecord(v) &&
    isStr(v.eventId) &&
    isStr(v.scenarioType) &&
    isStr(v.scenarioLabel) &&
    isStr(v.line) &&
    isStr(v.station) &&
    isStr(v.summary) &&
    isRecord(v.materialChangeEvent) &&
    isStr(v.materialChangeEvent.time) &&
    isStr(v.materialChangeEvent.description) &&
    isArr(v.stages)
  )
}

// 以 stage id 查找,取代 `stages[0]`~`stages[3]` 的固定陣列索引假設
export function findRawStage(scenario: unknown, id: StageId): unknown {
  if (!isRecord(scenario) || !Array.isArray(scenario.stages)) return undefined
  return scenario.stages.find((s) => isRecord(s) && s.id === id)
}
