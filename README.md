# AI 生產異常戰情室——90 秒異常追兇

離線事件回放型戰情室:在 90 秒內走完「看見異常 → 重播證據 → AI 診斷 → 根因揭曉 → 損失量化」的決策閉環。

> 模擬資料|離線事件回放|不含即時控制

## 快速開始

```bash
npm install
npm run dev            # 開發伺服器
npm run build          # 正式建置(GitHub Pages 用)
npm run build:offline  # 單檔 dist-offline/index.html,可離線雙擊開啟(展示日用)
```

## 四個頁面(= 四個階段)

| # | 頁面 | 工作包 | 分支 |
|---|------|--------|------|
| 1 | 正常基線|全域感知 | A 導覽殼層 | `feature/replay-shell` |
| 2 | 異常事件|邊緣告警 | B 數位孿生告警 | `feature/twin-alert` |
| 3 | 現場知識診斷|AI 老師傅卡 | C 六格診斷卡 | `feature/diagnosis-cards` |
| 4 | 根因閉環|證據、處置與止損 | D 決策與損失 | `feature/decision-loss` |
| — | 手機版/錯誤提示/離線建置 | E 響應式與部署 | `feature/responsive-offline` |

## 協作規範(對應課程評分表)

- 分支流程:`feature/* → dev → main`;`main`/`dev` 禁止直接 push
- 每個功能先開 Issue,再從最新 `dev` 建分支;PR 附需求編號、驗收條件、畫面、測試結果
- 環狀互審:A 審 E、B 審 A、C 審 B、D 審 C、E 審 D
- 文件追蹤編號:`REQ-xx → AC-xx → TC-xx → PR #xx`,見 [docs/rubric-traceability.md](docs/rubric-traceability.md)
- 每人的貢獻連結彙整於 [docs/contribution-matrix.md](docs/contribution-matrix.md)
- 機密不進 Git:見 [SECURITY.md](SECURITY.md)

## 展示日鐵律

- 現場一律使用 `dist-offline/index.html`(斷網可跑,`file://` 直開)
- 發表前 24 小時打 `demo-final` tag,之後不再改功能
- 備援順序:離線單檔 → GitHub Pages → 90 秒螢幕錄影
