# 即時感測器儀表板（standalone，不影響評分 SPA）

> 這個資料夾是「AI 生產異常戰情室」TinyML 硬體加碼展示的**額外現場展示工具**，完全獨立於 `src/` 下的離線評分 SPA——不共用程式碼、不共用建置流程、不影響 REQ-06 的離線可展示要求。用途是展示現場證明 ESP32-S3 量到的是真實硬體資料，不是模擬情境。

> **⚠ 本文件以下內容(除「怎麼開」「常見失敗與預防」外)仍停留在 H0(單振動視窗)階段,`index.html` 實際已推進到 H1**:現有伺服滑桿控制、雙波形視窗(加速度／電流+溫度)、FK+IK 3D 運動學視窗(含 IMU 軌跡疊圖)、獨立手機控制頁、模擬資料模式、H3 標籤化資料蒐集面板(定步幅夾持試驗+下載匯出,已在模擬模式驗證過,尚未用真實硬體跑過)。完整功能與協定說明見權威來源 [`../WARROOM-2.0-PLAN.md`](../WARROOM-2.0-PLAN.md)「即時儀表板」節,快速上手見 [`../HANDOFF.md`](../HANDOFF.md)。

## 為什麼不能用 `file://` 雙擊開啟

`index.html` 用 [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API) 直接連接 USB 序列埠。這個 API 要求 [secure context](https://developer.mozilla.org/en-US/docs/Web/Security/Secure_contexts)，`file://` 是否算數由瀏覽器實作決定，不保證可用——**一定要透過本機伺服器用 `http://localhost` 開啟**，展示前務必先驗證過一次,不要臨場才發現打不開。

瀏覽器支援(已查證,2026-08 現況):Chrome/Edge 自 v89 起完全支援;Firefox 於 v151 才新增支援(v150 以下不支援);Safari 目前仍不支援。**展示現場固定用 Chrome 或 Edge**,不要依賴 Firefox/Safari。

## 怎麼開

```bash
cd hardware/tinyml/live-dashboard
python -m http.server 8081
```

再用 Chrome 或 Edge 開 `http://localhost:8081`。也可以用 Claude Code 的 preview（`.claude/launch.json` 已有 `tinyml-live-dashboard` 這個設定，`port 8081`）。

展示前流程:
1. 關掉任何佔用該 COM port 的程式(Arduino IDE、Serial Monitor、燒錄工具)——同一個 port 同時只能被一個程式打開。
2. 開頁面,點「連接裝置」,選 ESP32-S3 對應的 COM port(瀏覽器會跳出裝置選擇視窗,這一步無法用程式自動略過,是 API 的設計限制)。
3. 連上後點「開始擷取 (start)」送出 `start` 指令,韌體開始連續傳送 1024 點視窗。
4. 畫面每收到一個完整視窗就更新一次波形圖與 RMS/peak-to-peak/crest factor(算法與 `../tools/analyze_baseline.py` 的 `calculate_time_features` 一致,去除各軸平均值後計算,避免垂直軸的重力偏移蓋過真實振動)。

## 這不是示波器等級的即時

韌體是「收滿 1024 點才整批送出」,不是逐點串流。115200 baud 下,三軸 1024 點的 JSON(約 20-30KB)光傳輸就要 1.7-2.6 秒,所以畫面是**每隔幾秒更新一整段波形**,不是連續捲動的即時曲線。這是韌體傳輸方式與序列埠鮑率決定的物理限制,換成其他前端技術(例如加一層 Python bridge)不會改善。

若之後方案 C 要讓畫面「感覺」更即時,可以讓韌體額外用一行低頻率(例如每 100-250ms)送出 RMS/電流/溫度/RPM 等**摘要數值**,原始 1024 點視窗仍保留給波形證據用——這個頁面目前只實作了原始視窗(因為硬體目前只有振動這一種訊號),之後要加其他感測器時再依這個方向擴充韌體與這裡的解析邏輯。

## 常見失敗與預防

- **序列埠被佔用**:展示前完全關閉 Arduino IDE / Serial Monitor。
- **重新插拔後失去連線**:頁面會顯示紅色「裝置已拔除」,重插後重新點「連接裝置」,不會自動恢復。
- **半行/壞行 JSON**:頁面會累積到換行才解析,壞行會被丟棄並計數在「無法解析的行數」,不會讓整個讀取迴圈中斷。
- **開 port 後韌體才剛開機**:頁面會等待韌體送出第一筆 `status`,連線後看到狀態列顯示 `ready`/`sensor_init_failed` 才代表韌體已就緒。

## 硬體現況(2026-08-14)

H0(基本振動擷取)**尚未在真實硬體上測試過**。第一次接上真板子時,建議先照下面順序手動驗證,不要直接期待這個頁面立刻能畫出漂亮波形:

1. 插入後 Windows 有沒有出現穩定 COM port
2. 開機後 `status` 是不是有效 JSON(`who_am_i` 應為 `113`(MPU9250)或 `112`(MPU6500 clone))
3. 送 `start` 後有沒有連續收到完整 1024 點三軸視窗
4. 手動晃動/施壓馬達,波形/RMS 有沒有可見變化
5. 拔插、重啟、停止再開始,能不能正常恢復

如果原始序列埠資料本身就有問題,先用 Arduino IDE 的 Serial Monitor 或裝 [Serial Studio](https://serial-studio.github.io/) 排除韌體/接線問題,再回頭用這個頁面——它是消費現有序列協定的前端,不會、也不應該掩蓋硬體層的問題。
