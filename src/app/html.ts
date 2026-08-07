// 情境資料來自 JSON,未來若改接 MES/知識服務等外部來源就不再可信。
// 所有拼進 innerHTML 的資料一律先跑這個函式,避免變成 XSS 邊界。
export function escapeHtml(value: unknown): string {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;')
}
