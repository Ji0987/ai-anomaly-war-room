import factoryMapSvg from '../assets/factory-map.svg?raw'

const STATUS_COLOR: Record<string, string> = {
  normal: 'var(--normal)',
  warning: 'var(--warning)',
  critical: 'var(--critical)',
  resolved: 'var(--resolved)',
}

// 工作包 B:四頁共用同一份 SVG,只依 stage.status 換三號機台圓點顏色,其餘站點維持正常
export function renderTwin(container: HTMLElement, status: string): void {
  container.innerHTML = factoryMapSvg
  const machineDot = container.querySelector<SVGCircleElement>('#dot-machine')
  if (machineDot) {
    machineDot.style.fill = STATUS_COLOR[status] ?? STATUS_COLOR.normal
  }
}
