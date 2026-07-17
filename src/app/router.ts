import { STAGE_IDS } from './state'

export type RenderFn = (root: HTMLElement) => void

const routes = new Map<string, RenderFn>()

export function register(stageId: string, render: RenderFn): void {
  routes.set(stageId, render)
}

export function currentStageId(): string {
  const hash = location.hash.replace(/^#\/?/, '')
  return STAGE_IDS.includes(hash) ? hash : STAGE_IDS[0]
}

export function navigate(stageId: string): void {
  location.hash = `#/${stageId}`
}

export function startRouter(root: HTMLElement, onChange?: (id: string) => void): void {
  const renderCurrent = () => {
    const id = currentStageId()
    const render = routes.get(id)
    root.innerHTML = ''
    if (render) {
      render(root)
    } else {
      root.textContent = '找不到頁面,請按「重設」回到正常基線。'
    }
    onChange?.(id)
  }
  window.addEventListener('hashchange', renderCurrent)
  renderCurrent()
}
