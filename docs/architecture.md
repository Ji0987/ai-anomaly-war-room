# 架構文件

> 對應評分項 1。實線 = 已實作;虛線 = 未來藍圖(簡報用,不在本月範圍)。

## 系統定位

「場域感知 → 知識治理 → 戰情室追因 → 人員決策 → 損失量化」決策閉環的離線 PoC。

## 技術架構(已實作,實線)

```
Figma 產線底圖 ──匯出 SVG──▶ src/assets/
Dify 批次生成診斷卡 ──人工審核──▶ src/data/scenarios/*.json(版本化)
                                        │
                                        ▼
                    Vite + Vanilla TS 靜態 SPA(hash router)
                    ├─ 建置時內嵌 JSON/SVG(無 runtime fetch)
                    ├─ npm run build ──▶ GitHub Pages(課程部署證據)
                    └─ npm run build:offline ──▶ 單檔 HTML(展示日)
```

## 未來藍圖(虛線,僅簡報敘事)

- 邊緣感測裝置只上傳事件特徵(資料最小化)
- 地端/雲地混合的企業知識服務接口
- 與既有 MES/告警系統的標準化事件契約

## 關鍵設計決策

| 決策 | 理由 |
|------|------|
| 靜態 SPA 而非前後端分離 | 展示零依賴、評分表無後端要求、時程 1 個月 |
| JSON 建置時內嵌 | 斷網可跑;file:// 下 fetch 會被 CORS 擋 |
| Dify 預生成而非即時呼叫 | 展示穩定、輸出可審核、API Key 不進前端 |
| hash router | GitHub Pages 子路徑與 file:// 皆可用 |
