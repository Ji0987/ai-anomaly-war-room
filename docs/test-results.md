# 測試結果紀錄

> 對應評分項 6。每輪測試新增一節,勿覆寫歷史。狀態:✅ 通過 / ❌ 失敗(附 Issue 連結)/ ⏭ 未執行。

## 第 1 輪|2026-08-01|AI 技術優化驗證(非各工作包負責人手動驗收)

> ⚠️ **用語已過時(2026-08-09 補註)**:下方「四位組員」「AGENTS.md 三道人工關卡」「工作包負責人」
> 等說法,對應的治理框架(AGENTS.md、contribution-matrix.md、team-setup.md)已於 2026-08-08
> commit `55aba34` 整份移除,改為「Claude Code 與 Codex 直接協作處理此專案」。保留原文不覆寫,
> 僅在此補註避免讀者誤以為該框架仍在運作;細節見 rubric-traceability.md 評分項 3 的已知缺口。

> 本輪由 AI 依審查報告整合改動後執行,涵蓋建置、指令列與瀏覽器互動驗證;
> **不是**四位組員各自的 AC-理解/操作驗收(AGENTS.md 三道人工關卡),
> 團隊送交前仍應各自重跑一次自己工作包的項目並簽名記錄。

| 編號 | 狀態 | 實際結果 / 備註 |
|------|------|------------------|
| TC-01a | ⏭ | 未動導覽切換邏輯,建議人工快速確認 |
| TC-01b | ⏭ | 90 秒導覽計時未變更,建議人工確認 |
| TC-01c | ⏭ | 重設邏輯未變更,建議人工確認 |
| TC-02a | ✅ | ⚠️ **已勘誤,見下方「第 2 輪」與勘誤說明,不代表目前仍通過** — 四頁走過,三號機台圓點依 status 綠/黃/紅/綠正確切換;非機台站點固定綠燈並標示「未模擬」 |
| TC-02b | ✅ | 點選/鍵盤 Enter 三號機台圓點,下方即顯示訊號文字與狀態標籤 |
| TC-03a | ✅ | 六格齊全,證據 3 條且皆含時間點(T+8/T+20) |
| TC-04a | ✅ | slider=0 → NT$0;slider=30 → 30×40×0.07×25=NT$2,100,無 NaN |
| TC-04b | ✅ | slider=10 → 顯示 NT$700,與公式一致 |
| TC-05a | ⏭ | 未執行——TC-05a 要求使用者在 390×844 實機操作。AI 僅在 Browser pane 以 375×812 做過輔助檢查(`scrollWidth`=`clientWidth`=375,無橫向捲動),這**不是**正式驗收證據,待負責人(B)本人於 390×844 實機操作四頁與 slider 後記錄結果 |
| TC-06a | ✅ | ⚠️ **已勘誤,見下方「第 2 輪」與勘誤說明,不代表目前仍通過** — `npm run build:offline` 成功產出 dist-offline/index.html(單檔),但這只證明產物生成,不等於離線執行驗證通過 |
| TC-06b | ⏭ | 離線單檔未實機飛航模式開啟,僅確認 build:offline 產物存在,建議人工用 file:// 實測 |
| TC-07a | ✅ | 切至 EVT-003 第 2 頁顯示「此階段資料無法顯示」防呆訊息;第 1/3/4 頁不受影響 |
| TC-08a | ✅ | 擴大掃描範圍後(排除 package-lock.json)對全部 git 追蹤檔執行,無命中 |
| TC-08b | ✅ | `npm audit` 從 1 high(vite)+1 moderate(esbuild)修正為 0 vulnerabilities(升級 vite ^5.4.0→^6.4.3) |
| TC-08c | ✅ | 第 2 頁規則命中表:已觸發權重合計 0.8 × 感測器品質係數 0.975 = 0.78,與畫面顯示分數一致 |
| TC-09a | ✅ | ⚠️ **已勘誤,見下方「第 3 輪」與勘誤說明,不代表目前仍通過** — EVT-001~004 四種情境皆完整瀏覽四頁,無 JS 例外(console 無錯誤訊息) |
| TC-09b | ⏭ | 尚未跑展示前總驗收(待團隊決定送審時程後再執行) |

> **勘誤(2026-08-08,對應 PR #6 commit 95ffb8b 之後的審查)**:上表兩列判定已因後續變更/檢視
> 失效,不代表現在仍然通過,請以下方「第 2 輪」為準:
> - **TC-02a**:當時「非機台站點固定綠燈並標示『未模擬』」是如實描述,但事後(commit 95ffb8b)
>   已把未設感測資料的三個站點從綠色改成灰色 `unavailable`,原始備註與現況不符。
> - **TC-06a**:當時只確認 `npm run build:offline` 產出檔案成功,**並未**實際以 `file://` 開啟
>   單檔操作過任何流程;build 成功不等於離線執行通過,原本標記 ✅ 過於寬鬆。

> **勘誤(2026-08-09,第二輪審查報告發現)**:
> - **TC-09a**:當時標記 ✅,但 git 考古(commit 255cfd2 的驗證紀錄)顯示實際只瀏覽器實測過
>   EVT-001/002/004 三種情境,**EVT-003 從未被實際開啟驗證過**。EVT-003 的 sensing 階段
>   `signals.current_a` 當時整個欄位缺失(非設為 0),導致 runtime schema guard 驗證失敗,
>   實際畫面顯示「此階段資料無法顯示」防呆錯誤,不是感測缺值情境的內容——與本列「4 種情境皆完整
>   瀏覽」的判定矛盾。原本標記 ✅ 是未涵蓋 EVT-003 的假陽性,已於下方「第 3 輪」修復並重新驗證。

## 第 2 輪|2026-08-08|AI(Claude Code,實際以 file:// 開啟離線單檔操作)|測試對象 commit: `95ffb8b`

> 本輪針對 PR #6 對 commit 95ffb8b 的審查意見,重新驗證 TC-02a(unavailable 站點行為)並
> **第一次真正執行** TC-06a/TC-06b(先前僅測過 build 是否成功)。同樣不是四位組員本人的
> AC 驗收,執行者是 AI。

| 編號 | 狀態 | 實際結果 / 備註 |
|------|------|------------------|
| TC-02a | ✅ | 重新產出 dist-offline 後以 `file://` 開啟,DOM 檢查四站點:三號機台 `fill: var(--normal)`、`aria-label: "三號機台:正常,..."`(依階段仍會變黃/紅/綠);原料/換料、品質檢測、包裝三站點皆 `fill: var(--muted)`、`aria-label` 皆為「...:無感測資料,點選查看訊號」,點擊後明細面板顯示「無感測資料:本情境未針對此站點模擬感測數據...」,不再出現「正常」字樣 |
| TC-06a | ✅ | 實際以 `file:///.../dist-offline/index.html` 開啟(非 `http://localhost`),完整操作:導覽列 1→4 切頁、第 4 頁 slider 拖到 15 分鐘(顯示 NT$1,050,公式正確)、點選未設感測站點查看「無感測資料」明細、第 3 頁診斷卡內容正常顯示。全程 console 無錯誤訊息。**有一項未能驗證**:topbar 情境下拉選單(EVT-001→EVT-002)依賴 `location.search` 觸發整頁重新載入,在目前測試工具的 `file://` 沙盒下,連不帶任何參數變更的原生 `location.reload()` 都不會真的重新載入(以 `window.__marker` 值在呼叫前後不變的方式雙重確認,含另開全新分頁的對照測試),這是測試工具本身對「專案資料夾以外的本機檔案」的限制,不是這次改動或程式碼本身的證據——但我沒有真實桌面瀏覽器可以排除萬一,建議負責人(A)用自己電腦實際雙擊開啟 `dist-offline/index.html` 後,額外測一次情境下拉選單是否正常切換 |
| TC-06b | ✅ | 同一輪操作全程用瀏覽器網路面板檢查,離線單檔零筆 request(含分頁切換、slider、站點點擊、reload 測試期間) |

## 第 3 輪|2026-08-09|AI(Claude Code 委派 Codex/gpt-5.6-terra 實作 + Claude 逐檔審查與瀏覽器驗收)|測試對象 branch:`chore/review-round2-fixes`

> 依第二輪雙視角審查報告(repo 外部文件,不公開)修復。範圍:(1) EVT-003
> `signals.current_a` 由缺失欄位改明確 `null` 編碼,修復 runtime schema guard 導致的整頁防呆錯誤;
> (2) baseline/diagnosis/decision 三頁 `<section class="stage stage-xxx">` 由寫死字面量改
> `stage-${stage.status}` 動態帶入,並補上 main.css 對應樣式(先前是死碼,四種情境中已有三種的顏色
> 判定實際是錯的但肉眼看不出來);(3) edge score 由 JSON 預先算好改頁面即時運算(`computeEdgeScore`,
> 呼應 `computeLoss` 模式);(4) 新增 EVT-005 示範 `suppressed: true`;(5) 診斷卡新增
> `contentReview` 未審核時的視覺提示;(6) 文件追溯鏈補齊(AC-08b/TC-08d、TC-10a)。
> 不在本輪範圍:「4 人組」參賽定位與治理證據問題(需使用者自行決定,非工程問題)。
> 執行分工:JSON/schema/sensing.ts 邏輯委派 Codex(gpt-5.6-terra/high),Claude 逐檔 `git diff`
> 核對後獨立重跑驗證指令;main.css 視覺樣式與 baseline/diagnosis/decision.ts 由 Claude 直接處理。

| 編號 | 狀態 | 實際結果 / 備註 |
|------|------|------------------|
| TC-02a(重驗) | ✅ | 逐一開啟 EVT-002/003/004 的 diagnosis、decision 頁,DOM 檢查 `section.stage` 的 `class` 與 `border-top-color`:EVT-002 diagnosis 現為 `stage-normal`(綠,先前寫死顯示 `stage-critical`,因 main.css 當時無對應樣式規則,肉眼不可見但屬資料契約錯誤);EVT-003 decision 現為 `stage-critical`(紅,先前寫死顯示 `stage-resolved`);EVT-004 diagnosis 現為 `stage-warning`(黃,先前寫死 `stage-critical`)。三個實際不一致案例皆已修正 |
| TC-07a(新方法) | ✅ | 依新版操作步驟,已用「EVT-003 修復前的實際狀態」作為等價驗證證據:`signals.current_a` 缺失時,`isSignals()` guard 判定失敗,`http://localhost:5173/?event=EVT-003` 的 sensing 頁面實測顯示「此階段資料無法顯示」防呆訊息,其餘三頁不受影響——與預期結果一致。修復後(`current_a` 改明確 `null`)已重新驗證同一頁面改為正常顯示內容,不再觸發此防呆路徑 |
| TC-08c(重驗) | ✅ | 第 2 頁計分公式行改為「即時計算」:EVT-001 顯示「已觸發權重合計 0.8 × 感測器品質係數 0.975 = 0.78(與裝置回報值一致,已重新驗算)」,EVT-002~005 亦逐一確認即時計算值與 JSON 原宣告分數一致,無數值回歸 |
| TC-08d(新增) | ✅ | 切至 EVT-005 第 2 頁:規則命中表模具溫度、電流兩項標記「✅ 觸發」,評分 0.8 ≥ 門檻 0.6;主文字顯示「未觸發」並附註「本次已被抑制,不重複派發」,兩者不矛盾,四個既有情境(`suppressed` 皆 `false`)首次有情境示範 `suppressed: true` 生效畫面 |
| TC-09a(重驗,EVT-001~005) | ✅ | 5 種情境(含新增 EVT-005)× 4 頁,共 20 個頁面組合逐一實際開啟,`read_console_messages(onlyErrors)` 全程無錯誤訊息,無白屏、無資料防呆誤觸發(EVT-003 sensing 已修復為正常內容) |
| TC-10a(新增) | ✅ | 第 3 頁確認 `contentReview`(內容審核,⏳/✅/❌ 標籤)與 `caseDisposition`(現場處置確認,⏳/✅/↩ 標籤)為獨立顯示欄位,且新增未審核時的 `.content-caveat` 警示文字;第 4 頁確認復機驗證卡顯示目標良率、複檢窗口、目前觀測值(EVT-001「尚未複檢」/EVT-005 同)、重開案條件,標題本身即註明「不等於『已解決』就代表指標已回歸正常」 |
| TC-05a(部分) | ⏭ | 仍非正式 390×844 實機驗收。但本輪新增 EVT-005 情境標籤過長,在 Browser pane 390×844 下實測發現真實回歸:`document.documentElement.scrollWidth`(433)> `clientWidth`(390),根因是 `#scenario-select` 於 mobile media query 只設 `flex:1` 未設 `min-width:0`,flex 子項預設不會縮小到小於內容最小寬度,長情境標籤撐開整頁橫向捲動。已修正(`main.css` 加 `min-width:0`)並重新量測 EVT-004/EVT-005 頁面,`scrollWidth`=`clientWidth`=390,無橫向捲動;完整四頁+slider 的正式人工驗收仍待負責人於實機執行 |
| 建置/依賴驗證 | ✅ | `npm run build`(含 `tsc --noEmit`)、`npm run build:offline`、`npm audit` 三項獨立重跑,全部通過,0 vulnerabilities;`dist-offline/index.html` 內容 grep 確認 EVT-005 與新增文案已正確內嵌 |

## 第 4 輪|2026-08-12|AI(Claude Code)於工作包 A 負責人本機實測|測試對象 branch:`feature/replay-shell`

> 補齊 REQ-01 導覽/90 秒導覽自第 1~3 輪起始終掛在 ⏭ 的三條(TC-01a/01b/01c),並回應第 2 輪
> TC-06a 留給負責人(A)的那項未決疑問。
>
> **執行環境(與前三輪的差別)**:Windows 11 家用桌機 + 本機安裝的 **桌面版 Google Chrome**
> (以 Playwright `channel: 'chrome'` 驅動真實瀏覽器,非測試工具內建 Browser pane),
> 測試對象是實際 `npm run build:offline` 產出的 `dist-offline/index.html`,以真正的
> `file:///c:/.../dist-offline/index.html` 開啟。第 2 輪無法排除的 `file://` 沙盒限制在此環境不存在。
>
> **仍須誠實標註**:執行者是 AI,不是負責人本人坐在電腦前手動點擊;本輪提供的是可重現的
> 自動化操作與量測數據(腳本與原始輸出見 PR 說明),不取代展示前的人工總驗收(TC-09b)。
>
> **量測注意事項**:首次量測 TC-01b 時,瀏覽器視窗未取得焦點,Chrome 對背景/被遮蔽視窗的
> `setTimeout` 節流使計時數據失真(量到 1.81 秒的假性失敗)。已改為 headless + 停用背景計時節流
> (`--disable-background-timer-throttling` 等)後重測,數據才穩定可信 —— 這是量測環境問題,
> 不是程式缺陷,記錄於此避免後人重踩。

| 編號 | 狀態 | 實際結果 / 備註 |
|------|------|------------------|
| TC-01a | ✅ | 依序點擊導覽列 1→4,四頁 `location.hash` 分別為 `#/baseline`、`#/sensing`、`#/diagnosis`、`#/decision`,`#app h1` 依序為「正常基線\|全域感知」「異常事件\|邊緣告警」「現場知識診斷\|AI 老師傅卡」「根因閉環\|證據、處置與止損」;每頁 `.nav-btn.active` 皆恰好 1 個且 `data-stage` 與當前頁一致(無多重高亮/高亮殘留) |
| TC-01b | ✅ | **修正後**通過。完整導覽實測總時長 **90.11 秒**(規格 90±5),換頁時間點 `#/sensing@15.02s → #/diagnosis@35.02s → #/decision@65.05s`,結束後按鈕自動復原為「▶ 90 秒導覽」。暫停:播放中按鈕為「⏸ 暫停導覽」,按下後變回「▶ 90 秒導覽」,續抱 25 秒(第 1 頁停留時間僅 15 秒)期間 `hashchange` 事件數 = 0,確認暫停後不會自動換頁 |
| TC-01b(邊界,修正前的實際缺陷) | ❌→✅ | 修正前:從第 3 頁按「90 秒導覽」只走 2 頁、55.11 秒;**從第 4 頁按下時按鈕顯示「⏸ 暫停導覽」卻空轉 25 秒完全不換頁**——第 4 頁正是前一輪導覽結束後的停留位置,展示日連續講兩次必然踩到。根因:`play()` 以當前頁索引起算 `schedule(indexOf(currentStageId()))`。已修正為導覽固定自第 1 頁開始;重測兩種起點皆為 `#/baseline@0s → #/sensing@15s → #/diagnosis@35.02s → #/decision@65.02s`,總時長 90.10 / 90.11 秒,四頁走完 |
| TC-01c | ✅ | 第 4 頁把 slider 拉到 18 分鐘(顯示 18 分、NT$ 1,260)後按「重設」:`location.hash` 回到 `#/baseline`、`.nav-btn.active` 為 baseline、`#app h1` 為「正常基線\|全域感知」;再切回第 4 頁確認 slider `value=0`、顯示 0 分、NT$ 0,localStorage 內的 `delayMinutes` 已清除 |
| TC-06a | ✅ | 以真實 `file://` 開啟離線單檔完成全流程:四頁切換、slider、重設、90 秒導覽全數正常。**並已補上第 2 輪留給負責人(A)的未決項——topbar 情境下拉選單在 `file://` 下確實可正常切換**:EVT-001→EVT-002(`location.search` 變為 `?event=EVT-002`,badge 顯示「EVT-002・正常巡檢…」)、EVT-002→EVT-005(`?event=EVT-005`,badge 顯示「EVT-005・重複告警抑制…」)、EVT-005→EVT-001(回到預設,`search` 清空為 `""`,`select.value=EVT-001`,badge 顯示「EVT-001・確診異常…」)。第 2 輪懷疑的 `location.search` 整頁重載失效,確認是當時測試工具的沙盒限制,不是程式問題 |
| TC-06b | ✅ | 整場 session(含四頁切換、90 秒完整導覽、slider、重設、三次情境切換重載)以 `page.on('request')` 全程錄製,`http(s)` 開頭的 runtime 請求數 = **0**;同場 `console.error` = 0、未捕捉例外 = 0 |
| GitHub Pages 部署 | ✅ | `https://ji0987.github.io/ai-anomaly-war-room/` 實際以瀏覽器開啟,HTTP **200**,`waitUntil: networkidle` 後導覽列 4 顆按鈕、badge、第 1 頁 h1 皆正常渲染,`requestfailed` = 0;對應 workflow run 為 `Deploy GitHub Pages` on `main`(最近一次 success)。截圖見 `docs/screenshots/github-pages-deploy.png` |
| 建置驗證 | ✅ | 修正後重跑 `npm run build`(含 `tsc --noEmit`)與 `npm run build:offline`,皆無錯誤;`npm ci` 回報 0 vulnerabilities |
| TC-05a | ⏭ | 仍非正式人工實機驗收(屬工作包 B)。本輪順手記錄:390×844 viewport 下第 4 頁 `document.documentElement.scrollWidth` = `clientWidth` = **390**,無橫向捲動,slider=15 顯示 NT$ 1,050(15×40×0.07×25,公式相符);截圖見 `docs/screenshots/mobile-390-decision.png` |

## 範例格式(供下一輪測試複製)

### 第 N 輪|日期|執行者|commit: `xxxxxxx`

| 編號 | 狀態 | 實際結果 / 備註 |
|------|------|------------------|
| TC-01a | ⏭ | |
