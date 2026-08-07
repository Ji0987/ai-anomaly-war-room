# 開發規範

> 對應評分項 1、3。

## 分支流程

```
feature/<主題> → dev → main → GitHub Pages
```

1. 先開 Issue(標題含 REQ 編號),再從**最新的 dev** 建分支。
2. commit 訊息:說明「做了什麼」,例如 `新增診斷卡信心度顯示 (REQ-03)`。
3. PR 用模板,附:需求編號、驗收條件、畫面截圖、測試結果。
4. 採 merge commit(保留分支歷史),不 squash。
5. `main`、`dev` 禁止直接 push。

## 程式風格

- Vanilla TypeScript,不引入框架(範圍控制)。
- 一頁一檔(`src/pages/`),共用邏輯放 `src/app/`,跨頁 UI 元件放 `src/components/`。
- 顏色/圓角等設計 token 一律用 `src/styles/main.css` 的 CSS 變數(評分項 4:一致性)。
- 頁面資料一律來自情境 JSON,不在頁面內寫死數值;新增/修改欄位要同步更新 `src/app/schema.ts`
  的型別與型別守衛,頁面一律用 `stageOrNull(id, guard)` 取值,不直接假設 JSON 形狀正確。
- 拼進 `innerHTML` 的資料一律先過 `src/app/html.ts` 的 `escapeHtml`,情境資料未來可能換成外部
  來源,不能預設內容一定安全。

## AI 協作規範

- 可用 AI 生成程式與文件,但需依專題調整並理解後才提交,不是原樣照搬。
- PR 描述需能用自己的話說明改了什麼,不要照抄 AI 輸出。

## 展示凍結

- 發表前 24 小時:`git tag demo-final`,之後只修 blocking bug。
- 展示一律用 `dist-offline/index.html`,以飛航模式驗證後才算完成。
