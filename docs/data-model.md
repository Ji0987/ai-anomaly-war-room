# 資料模型

> 對應評分項 1。情境檔:`src/data/scenarios/event-XXX.json`

## 情境(Scenario)

| 欄位 | 型別 | 說明 |
|------|------|------|
| eventId | string | 事件編號,四頁共用(貫穿性證明) |
| line / station | string | 產線/站點 |
| summary | string | 一句話事件摘要 |
| stages | Stage[] | 固定四階段:baseline / sensing / diagnosis / decision |

## 階段(Stage)

| 欄位 | 型別 | 出現於 | 說明 |
|------|------|--------|------|
| id / label / time | string | 全部 | 階段識別、顯示名稱、相對時間(T-10 等) |
| yield | number | 全部 | 良率 % |
| status | string | 全部 | normal / warning / critical / resolved |
| signals | object | 1-2 | temperature_c / vibration_rms / current_a,標註 `source: raw_sensor(模擬)` |
| edge | object | 2 | deviceId / score / threshold —— 邊緣異常評分(非 Edge AI,命名紅線) |
| card | object | 3 | 六格診斷卡,標註 `source: diagnosis(Dify 預生成)` |
| truth / loss | — | 4 | 預埋根因;損失參數標註 `source: loss(what-if 假設模型)` |

## 資料來源分層(誠實標示,對雙評審都加分)

1. `raw_sensor(模擬)`——自造感測值
2. `edge(規則評分)`——門檻/評分邏輯
3. `diagnosis(Dify 預生成)`——LLM 產出、人工審核後凍結
4. `loss(what-if 假設模型)`——透明公式 + 模擬營運假設
