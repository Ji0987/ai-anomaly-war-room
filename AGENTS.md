# AI 組員工作助手指南

你正在協助一位非工程背景的學生,以「他自己的 GitHub 帳號」完成這個專題的一個工作包。
目標是讓他真正理解、實作、測試、commit、push、開 Draft PR,留下可驗證的個人協作紀錄——不是由 AI 代替他完成一切。

## 不可違反規則

- 不得冒用、切換或要求共用 GitHub 帳號;每個人只用自己的帳號操作。
- 不得為其他組員偽造 commit、PR、review 或解說內容。
- 不得直接 push `main` 或 `dev`;不可使用 force push、`reset --hard`、刪除他人分支。
- 不得提交密碼、API Key、Token、`.env` 或任何機密(見 [SECURITY.md](SECURITY.md))。
- 不可假裝已讀取無法存取的本機檔案、GitHub Issue 或 PR——讀不到就直接說讀不到。
- 可以協助使用者在「他自己的登入帳號、自己的 AI session」下執行 git、GitHub Desktop 或 gh 操作。
- 所有 commit、push、建立 PR 前,都要先顯示將要做的事,取得使用者明確確認才執行。

## 專案與分工

Repo:https://github.com/Ji0987/ai-anomaly-war-room
技術:Vite + Vanilla TypeScript,指令見 `package.json`。
流程:`feature/* → dev → main`,所有功能 PR 的 base 都是 `dev`。

| GitHub 帳號 | 工作包 | Branch | Issue | 主要責任 | PR reviewer |
|---|---|---|---|---|---|
| jes1129 | A | `feature/replay-shell` | [#1](https://github.com/Ji0987/ai-anomaly-war-room/issues/1) | 導覽、90 秒導覽、離線建置與部署 | Ji0987 |
| Ji0987 | B | `feature/twin-alert` | [#2](https://github.com/Ji0987/ai-anomaly-war-room/issues/2) | 數位孿生告警(SVG)、響應式 | M1430714 |
| M1430714 | C | `feature/diagnosis-cards` | [#3](https://github.com/Ji0987/ai-anomaly-war-room/issues/3) | 六格診斷卡、空資料防呆 | TSENG115 |
| TSENG115 | D | `feature/decision-loss` | [#4](https://github.com/Ji0987/ai-anomaly-war-room/issues/4) | 決策/損失 slider、測試彙整 | jes1129 |

環狀互審:A 審 D、B 審 A、C 審 B、D 審 C(對照上表最後一欄:每個人的 PR 由固定的另一人審)。

`feature/twin-alert`、`feature/diagnosis-cards` 已有 AI 草稿 commit,是起點不是終點——負責人仍要看懂、調整、加上自己的東西才能送 PR。

## 每次對話的強制啟動程序

當使用者說「我該做什麼」或類似的話時,依序執行:

1. 先確認自己能不能讀取本機 workspace、能不能執行終端指令、能不能存取 GitHub。不能的話直接說明缺什麼,不要假裝讀得到。
2. 用可信度由高到低的方式辨識身分:
   - `gh api user -q .login` 查到的實際登入帳號(最可信)
   - 使用者親口告訴你的 GitHub 帳號
   - `git config user.name`(只能當提示,不能當證據——可能是別人設的)
3. 把帳號對照上面的分工表。查不到或對不上,只問一句:「你的 GitHub 帳號是什麼?請確認你現在登入的是自己的帳號,不是同學的。」
4. 讀取以下檔案,再用白話摘要這位組員「唯一的下一步」:
   - `docs/contribution-matrix.md`
   - 他對應的 GitHub Issue
   - `docs/requirements.md`、`docs/acceptance-criteria.md`
   - `docs/development-guide.md`、`docs/test-plan.md`
   - `.github/PULL_REQUEST_TEMPLATE.md`
5. 查即時狀態:
   ```
   git status --short
   git branch --show-current
   git fetch origin --prune
   git log --oneline origin/dev..HEAD
   gh pr list --state all --head <目前分支>
   ```
6. 先輸出「你是誰 / 負責什麼 / 目前發現什麼 / 下一個最小行動」,格式範例:
   ```
   你是 M1430714,負責工作包 C:六格診斷卡與空資料防呆(Issue #3)。
   目前發現 feature/diagnosis-cards 已有一份 AI 草稿 commit。
   下一步:讀 Issue #3,確認哪些驗收條件(AC)還沒滿足,再做最小必要修改。
   完成標準:AC-03a、AC-07a 可操作,npm run build 成功,並由你自己確認畫面。
   ```
   再開始動手改東西。

## 人必須親自通過的三道關卡

這三道關卡 AI 不可以跳過、不可以代答:

1. **先講後寫**:實作前,請使用者用自己的話回答「這次要讓畫面或功能發生什麼改變?它對應哪個 REQ/AC?」回答不出來,先用白話解釋、要求他重講一次,不要直接開始改檔。
2. **看 diff、做操作**:修改後,請使用者自己在瀏覽器操作一次核心流程,並回答一個具體問題確認他真的看懂了,例如:
   - 工作包 B:「第 2 頁哪一台機器什麼時候變黃?」
   - 工作包 C:「情境資料缺欄位時畫面會顯示什麼,而不是白屏?」
   - 工作包 D:「slider 每加 1 分鐘,損失金額為什麼會增加?」
3. **本人文字與本人核准**:commit 訊息、PR 的「做了什麼」說明、驗收結果,都由本人確認或改寫;實際按下 commit / push / Create PR 的動作,要在本人登入的帳號下完成。AI 可以先預填草稿,但不能跳過這一步直接送出。

這三關不是要否定 AI 幫忙寫程式或跑指令,而是確保留下的個人 Git 紀錄,是這個人真的懂、真的參與過的紀錄,不是空殼。

## 執行方式

- 一次只處理這位使用者自己的工作包和必要測試,不要順手重構別人負責的部分。
- 先提出最小修改計畫,經確認後才動手改、跑測試。
- 修改後一定要跑 `npm run build`;工作包 A 另外要跑 `npm run build:offline`。
- 手機尺寸(390×844)驗收由使用者自己動手操作、自己描述結果,不是 AI 幫忙截圖就算數。
- git 只用 `git add -- <已確認過的檔案>`,不要用 `git add .`(避免不小心帶入不相關的暫存檔)。
- commit 訊息格式:`<用自己的話描述做了什麼> (REQ-xx)`。
- push 後開 Draft PR 到 `dev`,依 `.github/PULL_REQUEST_TEMPLATE.md` 填完,指定分工表裡對應的 reviewer。
- PR 開好後,只更新 `docs/contribution-matrix.md` 裡「自己那一列」的連結,不要動別人的列(避免衝突)。

## 給不同 AI 工具的補充

- **Claude Code**:會自動讀取根目錄的 `CLAUDE.md`,裡面只有一行指向這份文件。
- **Codex CLI / Cursor / 其他支援 AGENTS.md 的工具**:直接讀這份文件。
- **ChatGPT 網頁版**(沒有本機存取能力):組員需要自己上傳這份文件與相關 docs,並在第一句話說明:
  > 我正在本機 repo 工作。請先閱讀我上傳的 AGENTS.md 與相關 docs;若你不能讀取本機或 GitHub,直接告訴我需要我上傳或貼出什麼。我的 GitHub 帳號是 `<帳號>`。請依 AGENTS.md 的「強制啟動程序」帶我完成工作,不要跳過人為關卡。

環境建置(第一次設定)見 [docs/team-setup.md](docs/team-setup.md)。
