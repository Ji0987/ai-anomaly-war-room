# AI 生產異常戰情室

離線事件回放型戰情室:在 90 秒內走完「看見異常 → 重播證據 → AI 診斷 → 根因揭曉 → 損失量化」的決策閉環。

> 模擬資料|離線事件回放|不含即時控制

**第一次看這個 repo?先讀 [docs/kickoff-summary.md](docs/kickoff-summary.md)。**
**沒用過 git/GitHub?先照 [docs/team-setup.md](docs/team-setup.md) 把環境裝好,之後打開 AI 助手問「我該做什麼」就好(AI 會讀 [AGENTS.md](AGENTS.md) 引導你)。**

## 快速開始

```bash
npm install
npm run dev            # 開發伺服器
npm run build          # 正式建置(GitHub Pages 用)
npm run build:offline  # 單檔 dist-offline/index.html,可離線雙擊開啟(展示日用)
```

## 四個頁面(= 四個工作包,4 人組)

| # | 頁面 | 工作包 | 主功能 | 額外負責 | 分支 |
|---|------|--------|--------|----------|------|
| 1 | 正常基線|全域感知 | A | 導覽/路由/90 秒導覽 | 離線建置與部署驗證(build:offline、GitHub Pages) | `feature/replay-shell` |
| 2 | 異常事件|邊緣告警 | B | 數位孿生告警(SVG) | 響應式(手機/投影尺寸) | `feature/twin-alert` |
| 3 | 現場知識診斷|AI 老師傅卡 | C | 六格診斷卡 | 防呆(空資料/錯誤提示) | `feature/diagnosis-cards` |
| 4 | 根因閉環|證據、處置與止損 | D | 決策與損失 slider | 測試結果彙整與展示前驗收掃描 | `feature/decision-loss` |

> 4 人組不設第 5 個「只做測試/響應式」的角色;每人主功能 + 一項跨頁職責,理由見各 Issue。

topbar 右側「情境」下拉可切換 4 種內建情境(確診異常/正常巡檢/感測缺值/疑似誤報),
不是只有一個戲劇化的成功案例,見 [docs/data-model.md](docs/data-model.md#情境庫不是只有一個戲劇化的成功案例)。

## 協作規範(對應課程評分表)

- 分支流程:`feature/* → dev → main`;`main`/`dev` 禁止直接 push
- 每個功能先開 Issue,再從最新 `dev` 建分支;PR 附需求編號、驗收條件、畫面、測試結果
- 環狀互審(4 人):A 審 D、B 審 A、C 審 B、D 審 C
- 文件追蹤編號:`REQ-xx → AC-xx → TC-xx → PR #xx`,見 [docs/rubric-traceability.md](docs/rubric-traceability.md)
- 每人的貢獻連結彙整於 [docs/contribution-matrix.md](docs/contribution-matrix.md)
- 機密不進 Git:見 [SECURITY.md](SECURITY.md)

## 展示日鐵律

- 現場一律使用 `dist-offline/index.html`(斷網可跑,`file://` 直開)
- 發表前 24 小時打 `demo-final` tag,之後不再改功能
- 備援順序:離線單檔 → GitHub Pages → 90 秒螢幕錄影
