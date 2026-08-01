import { escapeHtml } from './html'

// 情境資料缺欄位或格式不正確時的共用防呆畫面(REQ-07/AC-07a):
// 四頁共用同一份實作,不在各頁重複手刻檢查邏輯。
export function renderDataError(root: HTMLElement, message: string): void {
  root.innerHTML = `
    <section class="data-error" role="alert">
      <h1>此階段資料無法顯示</h1>
      <p>${escapeHtml(message)}</p>
      <p class="note">請確認情境 JSON 是否缺欄位或格式錯誤;其餘頁面不受影響,可由上方導覽切換。</p>
    </section>`
}
