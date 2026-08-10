# 給新 session 的執行清單

> 如果你是一個剛開始接手的 AI session(不知道之前的討論脈絡),讀完這份文件應該就有足夠上下文動手。這是「AI 生產異常戰情室」專題的個人硬體加碼展示(TinyML Shadow PoC),獨立於 `src/` 下的離線 SPA 正式交付——不要動 `src/`、`docs/`、`.github/` 或專案根目錄設定檔。
>
> 完整材料表(含已有零件、建議採購、開發板型號細節)見 [`BOM.md`](BOM.md),本文件只講操作步驟。
>
> **零件已按方案 C(振動+聲音融合)的完整需求一次買齊**(2 顆 TT 馬達、L298N、MG90S、ACS712-5A),但**組裝與驗證順序不變**:H0 先只接 1 顆目標馬達 + GY-91,用手壓橡皮擦模擬負載,快速驗證訊號可分性;H0 過關才動手組裝干擾馬達、舵機自動加壓機構,進入方案 A/B/C。零件到齊不代表要一次全部接上——先求 H0 這關過,再逐步擴充。

## 現況(已完成,不用重做)

分支 `feature/tinyml-shadow-poc` 上已經有 H0(振動頻譜可分性驗證)的完整程式碼:

- `firmware/tinyml_shadow_h0/tinyml_shadow_h0.ino`——ESP32-S3 韌體,原始 I2C 讀 MPU9250(GY-91)加速度計,1kHz 取樣、1024 點視窗,透過 Serial(115200 baud)以 JSON 輸出。暫存器位址與 ±4g 量程換算(8192 LSB/g)已對照官方資料表核實。零外部函式庫依賴。
- `tools/serial_capture.py`——收集序列埠資料到 `data/raw/*.jsonl`。
- `tools/analyze_baseline.py`——時域特徵(RMS/peak-to-peak/crest factor,已修正重力直流偏移問題)+ FFT 頻帶功率(5-50/50-150/150-300/300-500Hz),輸出頻譜比對圖供人工判讀。
- 兩個已知且已修復的問題(細節見 git log 該次 commit 訊息):重力偏移蓋過 RMS、Windows 主控台編碼(`g²`/`±`)會讓腳本當機。
- 已用合成資料端到端驗證過邏輯正確(見 commit 訊息),但**從未在真實硬體上測試過**——這是新 session 的第一件事。

## 第一步:環境與燒錄

**用 Arduino IDE,不是 VS Code + ESP-IDF 擴充套件。**

理由:韌體只用 `Arduino.h`/`Wire.h`,Arduino IDE 裝好 ESP32 開發板套件(Boards Manager 加 `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`,搜尋安裝 `esp32 by Espressif Systems`)就能直接編譯,不用額外設定。若想要 VS Code 的編輯體驗,裝 VS Code 的「**Arduino**」擴充套件(不是「ESP-IDF」擴充套件)——底層一樣是 arduino-cli,不用重寫任何程式碼。

ESP-IDF 更強大(FreeRTOS 精細控制、I2S DMA、ESP-DSP 硬體優化 FFT),但現在切換等於把已驗證的韌體整份重寫,H0 用不到這些進階能力。等真的要做方案 B(INMP441 聲音,需要 I2S DMA)或要上板做即時 FFT 時,再重新評估要不要換。

步驟:
1. Arduino IDE → Boards Manager 裝 ESP32 套件 → 選板子「ESP32S3 Dev Module」
2. 開啟 `firmware/tinyml_shadow_h0/tinyml_shadow_h0.ino`,上傳(接開發板標示「USB SERIAL」的那個 micro-USB 埠,不是「USB SLAVE」)
3. 開序列埠監控視窗(115200 baud),確認開機有印出 `{"type":"status",...}`,檢查 `who_am_i` 欄位——MPU9250 應回 `0x71`(113),MPU6500 clone 應回 `0x70`(112)。如果讀到別的值或讀取失敗,先查 GY-91 接線(GPIO8=SDA、GPIO9=SCL,已在韌體裡寫死,見 `BOM.md`)

## 第二步:接線

見 `README.md` 的接線表與 `BOM.md` 的採購清單(動力/驅動零件已改為全新採購,不拆機械手臂車套件;驅動板已改推薦 TB6612FNG,電源改穩壓 6V,細節見 BOM.md 的元件選型段落)。重點提醒:TT 馬達由驅動板獨立電源供電,**電源地要跟 ESP32-S3 共地**,但 ESP32-S3 不要直接供電給馬達或舵機;GY-91 的 I2C 接 GPIO8(SDA)/GPIO9(SCL);「負載」用手壓橡皮擦/氈布即可,不用先做舵機自動化機構。

**買馬達之前**:先用電表量測 TT 馬達在 6V 的堵轉電流(短暫卡住轉軸量測,測完立即放開避免燒毀馬達),決定驅動板要用 TB6612FNG(堵轉電流 <1A/channel)還是 DRV8833(超過的話),見 `BOM.md`。

**組裝方案 C 的完整機構(H0 過關後)時,注意兩個容易造成資料洩漏的細節**:
- MG90S 移動到定位後要**等機構完全靜止**才開始錄振動/聲音,不要邊移動邊錄——否則模型可能學到的是「舵機在動的嗡嗡聲」而不是真正的摩擦異常訊號。
- MAX98357 的警示嗶聲**只能在推論完成、確定要播放時才出聲**,絕對不能跟感測擷取視窗重疊——喇叭自己的聲音/振動可能反過來汙染 INMP441 的錄音或 GY-91 的振動讀值。

## 第三步:跑 H0

```bash
python -m pip install -r hardware/tinyml/requirements.txt
python hardware/tinyml/tools/serial_capture.py --port <你的序列埠> --label normal_pwm150_run1
python hardware/tinyml/tools/serial_capture.py --port <你的序列埠> --label loaded_pwm150_run1
# 各收至少 3 組
python hardware/tinyml/tools/analyze_baseline.py --files hardware/tinyml/data/raw/*.jsonl
```

人工判讀 `data/analysis/baseline_spectra.png`:normal/loaded 頻譜有沒有肉眼可辨的差異(建議 2 倍以上功率差異視為可分)。**這是關卡,沒有就停,不要往下做模型。**

## 第四步(H0 通過後):模型選擇

**不要預設用 RandomForest。** 已獨立重新評估過(Claude + Codex 交叉討論,見 `.agent-bridge` 信箱歸檔 MSG 28-29),建議排序:

1. **限制深度的單一決策樹(首選)**——C 匯出風險低(深度 2-4 的樹可直接寫成幾個 `if/else`,不需要外部轉換工具)、可解釋性高、跟 `src/pages/sensing.ts` 既有的「可解釋規則評分」敘事最一致。用 `sklearn.tree.DecisionTreeClassifier(max_depth=3或4, min_samples_leaf=適當值)`,訓練後手動或寫小腳本把樹結構轉成 C 的 if/else(深度這麼淺,人工轉譯都不難,不需要 micromlgen)。
2. **正則化邏輯迴歸**——C 匯出最安全(就是縮放+內積+sigmoid,幾乎不可能出錯),小資料集通常比 RandomForest 穩定,如果決策樹效果不夠好可以試這個。
3. **人工門檻+加權評分**——如果只做二分類且頻譜差異很大,可能直接夠用,做法比照 `sensing.ts` 現有的規則評分模式。
4. **RandomForest**——只有在前三者都不夠、且用「分組交叉驗證」證明 RF 明顯穩定勝出時才用。真的要用,搭配 `micromlgen` 或 `emlearn`(不要沿用舊的 `D:\D1\畢業專題` 那套轉換器,那套有 bug,轉出來的 C 程式碼不會真的做決策樹遍歷),而且轉換後要**逐筆比對 Python 推論結果跟 C 推論結果**,不能只信轉換工具名稱。

**關鍵方法論(不管選哪個模型都適用)**:同一次擷取切出的多個 window 彼此高度相關(同一個 run、同一個馬達狀態),訓練/驗證切分**必須以「run」為單位**分組(例如 3 組 normal run 裡 2 組訓練 1 組驗證),**不能用「window」為單位隨機切**——否則同一個 run 的 window 會同時出現在訓練集和驗證集,驗證分數會虛高,無法反映模型真的能不能認出新的 run。

## 這條線的邊界(提醒新 session 不要越界)

- 不要碰 `src/`、`docs/`、`.github/`、`package.json` 等主線檔案
- 不要開 PR 併入 `dev`——這是個人加碼展示,要不要正式併入是使用者的決定,不是工程判斷
- 如果做出真實資料,最後只匯出成一份新的 `event-XXX.json` 靜態情境檔餵給現有 SPA(如果使用者決定要整合的話),不要做即時序列連線或修改 SPA 的離線架構
