# 開發規範

> 對應評分項 1、3。

## 分支流程(評分項 3 的 20 分就在這裡)

```
feature/<工作包> → dev → main → GitHub Pages
```

1. 先開 Issue(標題含 REQ 編號),再從**最新的 dev** 建分支。
2. commit 訊息:說明「做了什麼」,例如 `新增診斷卡信心度顯示 (REQ-03)`。
3. PR 用模板,附:需求編號、驗收條件、畫面截圖、測試結果。
4. **Codex 自動審查**(2026-08-01 起,取代原本的 4 人環狀互審):PR 開啟時
   Codex 自動審查(ChatGPT 設定 → 程式碼審查 → 審查觸發條件:開啟 PR)。
   要讓這個 repo 生效,需先在 ChatGPT 程式碼審查設定的「儲存庫喜好設定」把
   `ai-anomaly-war-room` 加進去。Codex 審查通過(或無 blocking 建議)後,
   由 PR 作者自行 merge,不再需要另一位組員手動核准。
5. 採 merge commit(保留分支歷史),不 squash。
6. `main`、`dev` 禁止直接 push。
7. 每週由**不同成員**建立一次 `dev → main` 發布 PR。

> 拿掉人工互審後,評分項 3「協作紀錄」的證據改由 Issue 討論、PR 描述與 commit
> 紀錄承擔——PR 描述仍要用自己的話說清楚改了什麼,不能只留 Codex 的審查紀錄。

## 程式風格

- Vanilla TypeScript,不引入框架(範圍控制)。
- 一頁一檔(`src/pages/`),共用邏輯放 `src/app/`。
- 顏色/圓角等設計 token 一律用 `src/styles/main.css` 的 CSS 變數(評分項 4:一致性)。
- 頁面資料一律來自情境 JSON,不在頁面內寫死數值。

## AI 協作規範

- 可用 AI 生成程式與文件,但**必須依專題調整並理解後才提交**(評分項 1:「只有 AI 產生內容」僅 7 分)。
- PR 描述需能用自己的話說明改了什麼——展示日每人要講解自己負責的部分。

## 展示凍結

- 發表前 24 小時:`git tag demo-final`,之後只修 blocking bug。
- 展示一律用 `dist-offline/index.html`,以飛航模式驗證後才算完成。
