# 評分表對照(Rubric Traceability)

> 課程「程式碼評分標準」100 分 → 本 repo 對應證據的所在位置。交作業前逐項自檢。

| # | 評分項 | 分數 | 本 repo 證據 |
|---|--------|------|--------------|
| 1 | 專案文件完整度 | 15 | docs/ 全套:requirements(10 條 REQ)、architecture、data-model(5 種情境的完整欄位定義)、development-guide、acceptance-criteria(23 條 AC)、test-plan(25 條 TC)、test-results(3 輪紀錄,含自我勘誤) |
| 2 | 需求符合度與驗收標準 | 15 | requirements.md 痛點/使用者/REQ-01~10 表 + acceptance-criteria.md AC-01a~10a,逐條對應 |
| 3 | 每位組員的實作與 Git 協作紀錄 | 20 | git commit/branch/PR 歷史、Issue 連結;feature→dev→main 流程。**已知缺口(定位已確認,非待決事項)**:專題已定位為「Claude Code 與 Codex 直接協作處理」,不套用個人工作包分配;獨立審查估算本項約 9/20(區間 8-10),對應評分表「少數人完成、無法證明個人實作」一檔。這是定位選擇下的已知取捨,不是需要補文件掩蓋的缺失 |
| 4 | 頁面設計與操作一致性 | 10 | styles/main.css 設計 token;四頁共用元件樣式與導覽;數位孿生站點的顏色/文字標籤/點擊與鍵盤操作在四頁一致(twin.ts) |
| 5 | 功能完整度 | 20 | 四頁 + 90 秒導覽 + slider + 5 種情境切換,核心流程可完整操作 |
| 6 | 功能測試列表與檢核結果 | 10 | test-plan.md(25 條 TC)+ test-results.md(3 輪紀錄;第 2、3 輪主動勘誤第 1 輪過寬的判定,不是只記錄通過) |
| 7 | 可使用性與基本品質 | 5 | REQ-05/REQ-07:手機尺寸、空資料提示(TC-05a/TC-07a);另 AC-02b/TC-02b:數位孿生站點支援點擊與鍵盤 Enter 操作,狀態同時有文字標籤不只靠顏色 |
| 8 | 安全與資料處理基本規範 | 5 | SECURITY.md;.gitignore 排除 .env;TC-08a 機密掃描(全 repo)、TC-08b `npm audit` 0 vulnerabilities |

追蹤鏈範例:`REQ-08 → AC-08a/AC-08b → TC-08c/TC-08d → PR #6`
