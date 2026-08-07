# AI 生產異常戰情室

離線事件回放型戰情室:在 90 秒內走完「看見異常 → 重播證據 → AI 診斷 → 根因揭曉 → 損失量化」的決策閉環。

> 模擬資料|離線事件回放|不含即時控制

## 快速開始

```bash
npm install
npm run dev            # 開發伺服器
npm run build          # 正式建置(GitHub Pages 用)
npm run build:offline  # 單檔 dist-offline/index.html,可離線雙擊開啟(展示日用)
```

## 四個頁面

| # | 頁面 | 主功能 |
|---|------|--------|
| 1 | 正常基線|全域感知 | 導覽/路由/90 秒導覽 |
| 2 | 異常事件|邊緣告警 | 數位孿生告警(SVG)、可解釋規則評分 |
| 3 | 現場知識診斷|AI 老師傅卡 | 六格診斷卡、空資料防呆 |
| 4 | 根因閉環|證據、處置與止損 | 決策與損失 slider |

topbar 右側「情境」下拉可切換 4 種內建情境(確診異常/正常巡檢/感測缺值/疑似誤報),
不是只有一個戲劇化的成功案例,見 [docs/data-model.md](docs/data-model.md#情境庫不是只有一個戲劇化的成功案例)。

## 協作規範(對應課程評分表)

- 分支流程:`feature/* → dev → main`;`main`/`dev` 禁止直接 push
- 每個功能先開 Issue,再從最新 `dev` 建分支;PR 附需求編號、驗收條件、畫面、測試結果
- PR 審查:見 [docs/development-guide.md](docs/development-guide.md)
- 文件追蹤編號:`REQ-xx → AC-xx → TC-xx → PR #xx`,見 [docs/rubric-traceability.md](docs/rubric-traceability.md)
- 機密不進 Git:見 [SECURITY.md](SECURITY.md)

## 展示日鐵律

- 現場一律使用 `dist-offline/index.html`(斷網可跑,`file://` 直開)
- 發表前 24 小時打 `demo-final` tag,之後不再改功能
- 備援順序:離線單檔 → GitHub Pages → 90 秒螢幕錄影
