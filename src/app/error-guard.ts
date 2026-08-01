export function renderDataError(root: HTMLElement, message: string): void {
  root.innerHTML = `
    <section class="data-error" role="alert">
      <h1>情境資料無法顯示</h1>
      <p>${message}</p>
    </section>`
}

export function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null
}
