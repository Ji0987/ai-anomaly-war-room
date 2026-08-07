# 評分表對照(Rubric Traceability)

> 課程「程式碼評分標準」100 分 → 本 repo 對應證據的所在位置。交作業前逐項自檢。

| # | 評分項 | 分數 | 本 repo 證據 |
|---|--------|------|--------------|
| 1 | 專案文件完整度 | 15 | docs/ 全套:requirements、architecture、data-model、development-guide、test-plan(內容需依專題持續更新,非 AI 原樣) |
| 2 | 需求符合度與驗收標準 | 15 | requirements.md 痛點/使用者/REQ 表 + acceptance-criteria.md AC 表 |
| 3 | 每位組員的實作與 Git 協作紀錄 | 20 | git commit/branch/PR 歷史、Issue 連結;feature→dev→main 流程。**目前缺口**:分工模式(work-package 對照表)已移除,若課程仍要求逐人紀錄,需另外補齊證據 |
| 4 | 頁面設計與操作一致性 | 10 | styles/main.css 設計 token;四頁共用元件樣式與導覽 |
| 5 | 功能完整度 | 20 | 四頁 + 90 秒導覽 + slider 核心流程可完整操作 |
| 6 | 功能測試列表與檢核結果 | 10 | test-plan.md + test-results.md(TC 對齊 AC) |
| 7 | 可使用性與基本品質 | 5 | REQ-05/REQ-07:手機尺寸、空資料提示(TC-05a/TC-07a) |
| 8 | 安全與資料處理基本規範 | 5 | SECURITY.md;.gitignore 排除 .env;TC-08a 機密掃描(全 repo)、TC-08b `npm audit` 0 vulnerabilities |

追蹤鏈範例:`REQ-03 → AC-03a → TC-03a → PR #12`
