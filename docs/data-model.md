# 資料模型

> 對應評分項 1。情境檔:`src/data/scenarios/event-*.json`;runtime schema:`src/app/schema.ts`。

## 從「TS 字面量推斷」改為「runtime schema 驗證」

舊版直接 `import scenario from '../data/scenarios/event-001.json'`,型別完全由 TypeScript
對這一份 JSON 的字面量推斷而來——沒有任何執行期防呆,也無法接第二份形狀不同的情境檔。
現在 `src/app/schema.ts` 明確定義每個 stage 的型別 + 型別守衛(`isBaselineStage` /
`isSensingStage` / `isDiagnosisStage` / `isDecisionStage`),情境 JSON 一律當 `unknown` 讀入,
頁面用 `stageOrNull(id, guard)` 依 stage id 查找並驗證,而不是 `stages[0]`~`stages[3]` 的
固定陣列索引假設。缺欄位或格式錯誤時,該頁顯示防呆訊息、其餘頁面不受影響(見
`src/app/error-guard.ts`,對應 REQ-07/AC-07a)。這是「事件轉換器」介面的最小可行版本:
未來要接 MES/告警系統,只要新增一份符合 schema 的 JSON(或轉接層產生符合 schema 的物件)
就能掛進來,不需要改頁面程式。

## 情境(Scenario)

| 欄位 | 型別 | 說明 |
|------|------|------|
| eventId | string | 事件編號,四頁共用(貫穿性證明) |
| scenarioType | string | `confirmed_anomaly` / `normal_noise` / `sensor_dropout` / `suspected_false_positive` |
| scenarioLabel | string | 情境類型的中文顯示名稱 |
| line / station | string | 產線/站點 |
| summary | string | 一句話事件摘要 |
| materialChangeEvent | object | 換料事件的時間點與說明,證據鏈裡「換料後 N 分鐘」的時間錨點 |
| stages | Stage[] | 固定四階段:baseline / sensing / diagnosis / decision |

## 階段(Stage)共同欄位

`id / label / time / yield / status`,其中 `status` 為 `normal / warning / critical / resolved`,
驅動數位孿生站點圓點顏色(見 architecture.md)。

## signals(出現於 baseline、sensing)

`temperature_c / vibration_rms / current_a / source`,`source` 一律標註
`raw_sensor(模擬)`。
`current_a` 可為 `null`,代表感測器當下離線、無法讀值,與觀測值恰好是 0 安培是兩件不同的事,兩者不得用同一種寫法表示。

## edge(出現於 sensing)——可解釋規則評分,不是裝飾數字

| 欄位 | 說明 |
|------|------|
| deviceId / score / threshold | 裝置、評分、告警門檻 |
| windowSeconds / hysteresisSeconds | 評分時間窗與遲滯秒數 |
| suppressed / suppressionRule | 是否被抑制、抑制規則說明(避免抖動誤報) |
| trend | 該時間窗內的溫度取樣點,用來佐證「持續上升」而非單點快照 |
| ruleHits | 每個訊號的規則命中明細:權重、觀測值、基線、規則門檻、是否觸發、備註 |
| scoring | 計分公式說明 + 感測器品質係數(`score = Σ(已觸發規則權重) × 感測器品質係數`) |
| source | 一律標註 `edge(規則評分)`——**與舊版不同**:舊版誤寫成 `raw_sensor(模擬)`,
  和本文件原本的分層定義矛盾,已修正資料本身,不再是文件說一套、資料做一套。

## card(出現於 diagnosis)

| 欄位 | 說明 |
|------|------|
| verdict | `anomaly_confirmed` / `no_anomaly` / `suspected_false_positive` / `insufficient_data` |
| rootCause / confidence / confidenceBasis | 推定根因、信心值、信心值的依據與限制說明(不是裸百分比) |
| evidence | 每條證據都帶 `time`(具體時間點)+ `metric`/`value`(具體數值),不是模糊敘述 |
| knowledge | 知識依據,細到 SOP 章節/歷史事件編號 |
| contentReview | 「AI 卡內容本身是否審核過」——`status`(approved/pending/rejected)+ 審核角色 + 時間戳 |
| caseDisposition | 「這次現場事件的處置是否被人確認」——`status`(pending/confirmed/returned_for_review/completed)+ 操作角色 + 時間 + 備註。**這是兩件不同的事**,拆成兩個獨立欄位,不再混在同一個 `reviewStatus` |
| provenance | 卡片版本、知識版本、模型識別、提示詞識別 |
| source | 一律標註 `diagnosis(Dify 預生成)` |

## loss / recovery(出現於 decision)

| 欄位 | 說明 |
|------|------|
| loss | `unitsPerMinute / defectRateDelta / costPerDefect / source`,`source` 標註 `loss(what-if 假設模型)` |
| recovery | 復機驗證:`status`(pending_recheck/verified/reopened)、目標良率、複檢窗口觀測值、重開案條件。**`status: resolved` 只代表根因已鎖定且處置已啟動,不代表指標已回歸正常**——兩者是否一致由 `recovery` 獨立表示,UI 上不會把「已處置」講成「已完全解決」 |

## 資料來源分層(誠實標示,對雙評審都加分)

1. `raw_sensor(模擬)`——自造感測值
2. `edge(規則評分)`——門檻/評分邏輯(EVT-001 起 `event-*.json` 的 `edge.source` 已與此一致)
3. `diagnosis(Dify 預生成)`——LLM 產出、人工審核後凍結(`contentReview.status`)
4. `loss(what-if 假設模型)`——透明公式 + 模擬營運假設

## 情境庫(不是只有一個戲劇化的成功案例)

`src/data/scenarios/index.ts` 目前內嵌五種情境,建置時全部靜態 import(呼應「無 runtime
fetch」的離線決策),topbar 下拉選單以 `?event=EVT-00x` 切換:

| eventId | scenarioType | 用途 |
|---------|--------------|------|
| EVT-001 | confirmed_anomaly | 主線案例:確診異常,完整規則命中 + 診斷 + 待復機驗證 |
| EVT-002 | normal_noise | 訊號小幅波動但未跨門檻,示範「為什麼沒有觸發」 |
| EVT-003 | sensor_dropout | 電流感測器於 T+8 離線,`current_a` 以 `null` 明確標記缺值(不是省略欄位、也不是觀測值 0),示範真實的感測資料品質分級情境 |
| EVT-004 | suspected_false_positive | 評分壓線超標,診斷卡標記疑似誤報、人工複判退回 |
| EVT-005 | repeat_alert_suppressed | 60 秒遲滯窗口內第二次評分,示範 `suppressed: true` 抑制生效、不重複派工,規則命中表仍可見達門檻的訊號 |
