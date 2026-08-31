# TinyML Shadow PoC — H0：振動頻譜可分性

這個資料夾是「AI 生產異常戰情室」的個人硬體加碼展示，用來驗證 TT 馬達在**正常**與**負載**兩種狀態下的振動頻譜是否有肉眼可辨的差異。它不會修改、匯入或影響專題主線的離線 SPA（`src/`）；正式評分交付仍以 SPA 為準。

開發板已確認型號與接線細節，完整材料表見 [`BOM.md`](BOM.md)。

H0 刻意不做機器學習：若頻譜沒有可分性，就在此停止，不進入 RandomForest 或任何模型訓練。

## 目錄

```text
hardware/tinyml/
├─ firmware/tinyml_shadow_h0/tinyml_shadow_h0.ino
├─ tools/serial_capture.py
├─ tools/analyze_baseline.py
├─ live-dashboard/          # 展示現場用的即時儀表板，見該資料夾 README.md
├─ requirements.txt
└─ data/
   ├─ raw/                 # 本機擷取的 JSONL（不進版控）
   └─ analysis/            # 本機圖表（不進版控）
```

## 展示現場即時儀表板

`live-dashboard/` 是獨立於這份離線 SPA 評分交付的額外現場展示工具：瀏覽器透過 Web Serial API 直連 ESP32-S3，即時畫出振動波形與 RMS/peak-to-peak/crest factor，用來現場證明資料是真硬體量出來的，不是模擬情境。不共用、不影響 `src/` 的任何程式碼或建置流程。用法見 [`live-dashboard/README.md`](live-dashboard/README.md)。

## 接線與供電

開發板為 [ATK-DNESP32S3M-MiniBoard](https://github.com/openedv/ATK-DNESP32S3M-MiniBoard)，以下腳位已對照官方接腳圖核實。

| 元件 | 連接 |
| --- | --- |
| GY-91 VCC | ESP32-S3 3.3V |
| GY-91 GND | ESP32-S3 GND |
| GY-91 SCL | ESP32-S3 **GPIO9** |
| GY-91 SDA | ESP32-S3 **GPIO8** |
| TT 馬達 | 驅動板馬達輸出端（由驅動板的獨立馬達電源供電；驅動板選型見 `BOM.md`） |
| 驅動板 IN 腳 | ESP32-S3 任一可用 PWM GPIO（避開 GPIO8/9、GPIO35-37） |
| 驅動板電源 GND | **與 ESP32-S3 GND 共地** |
| 編碼器馬達 A/B 相 | ESP32-S3 任一可用 GPIO（避開 GPIO8/9、GPIO35-37），建議接支援硬體 PCNT 的腳位以利解碼；**接線前先確認編碼器輸出邏輯電位**，5V push-pull 直接接會超過 ESP32-S3 GPIO 3.6V 絕對上限，需分壓或位準轉換 |

韌體已把 `I2C_SDA_PIN`/`I2C_SCL_PIN` 寫死為 8、9（對應這塊板子）。**如果換成別的 ESP32-S3 開發板，先查該板子自己的接腳圖再改這兩個常數，不要假設所有板子都一樣。**

TT 馬達必須由驅動板與其獨立電源驅動。馬達電源地與 ESP32-S3 的 GND 務必共地，但 **ESP32-S3 不要直接供電給馬達或舵機**。完整採購清單與元件選型理由見 [`BOM.md`](BOM.md)。

**H0 的「負載」不用自動化機構**：拿橡皮擦或氈布之類的材料，錄 loaded run 時用手壓著頂住轉動中的馬達軸或外殼即可，模擬機構（舵機自動加壓）留到 H0 過關、要做正式可重複實驗時再考慮。

**燒錄用哪個 USB 埠**：這塊板子有兩個 micro-USB 埠，標示「USB SERIAL」的那個（經 CH34x 晶片）才是燒錄/序列埠，不要用標示「USB SLAVE」的原生 USB 埠。

## H0 驗收標準

收集的正常與負載 run，在頻譜圖或頻帶功率上是否有肉眼可辨的差異？可先把約 2 倍以上的功率差異視為值得繼續驗證的訊號。

- 有明顯差異：H0 可視為有可分性，可再規劃下一階段特徵/模型實驗。
- 完全重疊或差異不穩定：H0 未通過，停在此處，不進下一步機器學習。

這是人工判讀閘門；分析程式不會自動宣告通過或失敗。

## 操作步驟

1. 依上表接線，確認 L298N、馬達與 ESP32-S3 已共地；將 `firmware/tinyml_shadow_h0/tinyml_shadow_h0.ino` 燒錄至 ESP32-S3。
2. 安裝 Python 依賴：`python -m pip install -r hardware/tinyml/requirements.txt`。
3. 找出序列埠後收集至少三組正常 run 與三組負載 run。每一組執行一次（Windows 範例）：

   ```powershell
   python hardware/tinyml/tools/serial_capture.py --port COM5 --label normal_pwm150_run1
   python hardware/tinyml/tools/serial_capture.py --port COM5 --label loaded_pwm150_run1
   ```

   可用 `--seconds 30` 延長時間。程式會在 `data/raw/` 建立帶時間戳的 `.jsonl` 檔；將 PWM、固定方式、負載形式寫進 label，便於事後比較。
4. 將六個檔案傳給分析工具；未傳 `--labels` 時會由檔名中的 `normal` 或 `loaded` 推斷標籤：

   ```powershell
   python hardware/tinyml/tools/analyze_baseline.py --files `
     hardware/tinyml/data/raw/normal_pwm150_run1_*.jsonl `
     hardware/tinyml/data/raw/normal_pwm150_run2_*.jsonl `
     hardware/tinyml/data/raw/normal_pwm150_run3_*.jsonl `
     hardware/tinyml/data/raw/loaded_pwm150_run1_*.jsonl `
     hardware/tinyml/data/raw/loaded_pwm150_run2_*.jsonl `
     hardware/tinyml/data/raw/loaded_pwm150_run3_*.jsonl
   ```

   PowerShell 的萬用字元若展開方式不符預期，可改為逐一貼上實際檔名。圖表預設存到 `data/analysis/baseline_spectra.png`。
5. 人工比較正常/負載 run 的頻譜峰值與四個頻帶功率。若沒有清楚、可重現的差異，就記錄 H0 未通過並停止，不進入模型階段。

## 序列協定

韌體使用 115200 baud，每筆資料都是一行 JSON。開機會輸出 `type: status` 的就緒資訊；主機可送：

- `start`：開始連續輸出 1024 點視窗。
- `stop`：停止輸出。
- `status`：回報串流狀態與已送出視窗數。

每個資料視窗包含 `sample_rate_hz`（視窗內以實際時間戳計算）和 `ax_g`、`ay_g`、`az_g` 三個 1024 點加速度陣列。因 115200 baud 對完整三軸 JSON 視窗的傳輸會形成視窗間空檔，H0 的比較單位是各個完整視窗的頻譜，不應把相鄰視窗視為無間斷的連續波形。
