# 團隊環境建置(第一次設定)

> 給沒用過 git、終端機、AI coding 工具的組員。照順序做完,之後每次只要打開 AI 助手問「我該做什麼」就好。

## 你需要裝的東西

1. **[GitHub Desktop](https://desktop.github.com/)** — 圖形介面操作 git,不用背指令。負責:clone 專案、看修改了什麼(diff)、按 commit、按 push、開 PR。
2. **[Node.js LTS 版](https://nodejs.org/)** — 讓專案跑得起來(`npm install`、`npm run dev`)。
3. **一款 AI coding 工具**(擇一,問負責人現在用哪個就跟著裝哪個):Claude Code、Codex CLI,或至少有 VS Code + 一般網頁版 ChatGPT/Claude 搭配使用。

## 設定步驟

### 1. 接受 GitHub 邀請

打開信箱或 GitHub 通知,接受 [Ji0987/ai-anomaly-war-room](https://github.com/Ji0987/ai-anomaly-war-room) 的 collaborator 邀請。沒收到就跟負責人要邀請連結。

### 2. 用 GitHub Desktop clone 專案

打開 GitHub Desktop → 用**你自己的** GitHub 帳號登入(不是同學的、不是負責人的)→ `File > Clone Repository` → 找到 `ai-anomaly-war-room` → 選一個你自己電腦上的資料夾 → Clone。

> 為什麼要自己 clone、自己登入?因為你之後的 commit 要掛在你自己的帳號下,這是評分表要看的「個人實作紀錄」的來源——負責人不能替你做這步。

### 3. 安裝專案套件

打開終端機(或直接讓 AI 助手幫你跑),切到專案資料夾,執行:

```bash
npm install
```

跑完之後可以用 `npm run dev` 打開本機預覽,確認畫面看得到。

### 4. 打開你的 AI 助手,說「我該做什麼」

在專案資料夾裡打開你選的 AI 工具(例如 Claude Code),它會自動讀到 `CLAUDE.md`/`AGENTS.md`,知道你是誰、該做什麼——前提是你的終端機/git 已經是用你自己的帳號登入。

如果你用的是**網頁版** ChatGPT 或 Claude(沒辦法直接讀你的電腦),就把 `AGENTS.md` 內容複製貼給它,並照 [AGENTS.md](../AGENTS.md) 最後一節的固定開場白跟它說。

### 5. 每次要送出修改前,確認身分

在 GitHub Desktop 左上角確認目前登入的帳號是不是你自己——尤其是共用電腦時容易搞錯,commit 會直接掛錯人名字。

## 卡住了怎麼辦

- **GitHub Desktop 看不到我 clone 的專案有變化**:先按左上角 `Fetch origin`,再按 `Repository > Pull`。
- **AI 說它不能存取 GitHub**:通常代表終端機沒有裝 `gh` CLI 或沒登入,這不影響 GitHub Desktop 操作,照樣可以用 GitHub Desktop 完成 commit/push/PR,不一定要靠 AI 跑 git 指令。
- **不確定自己該改哪個檔案**:看你對應的 GitHub Issue(#1/#2/#3/#4,見 [AGENTS.md](../AGENTS.md) 分工表),裡面寫了起點檔案。
- **完全卡住**:截圖問負責人或在小組群組問,不要自己瞎猜著改。
