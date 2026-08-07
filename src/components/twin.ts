import factoryMapSvg from '../assets/factory-map.svg?raw'
import { escapeHtml } from '../app/html'
import type { StageStatus } from '../app/schema'

export type TwinStationStatus = StageStatus | 'unavailable'

const STATUS_COLOR: Record<TwinStationStatus, string> = {
  normal: 'var(--normal)',
  warning: 'var(--warning)',
  critical: 'var(--critical)',
  resolved: 'var(--resolved)',
  unavailable: 'var(--muted)',
}

const STATUS_LABEL: Record<TwinStationStatus, string> = {
  normal: '正常',
  warning: '警戒',
  critical: '異常',
  resolved: '已處置',
  unavailable: '無感測資料',
}

export interface TwinStation {
  dotId: 'dot-feed' | 'dot-machine' | 'dot-qc' | 'dot-pack'
  name: string
  status: TwinStationStatus
  detail: string
}

const UNINSTRUMENTED_DETAIL = '無感測資料:本情境未針對此站點模擬感測數據,實務上應比照三號機台接上規則評分。'

// 目前只有三號機台有情境資料;其餘站點先給誠實的「未模擬」說明,
// 而不是假裝全站都在監控中(呼應專案一貫的資料來源誠實標示原則)。
export function uninstrumentedStations(): TwinStation[] {
  return [
    { dotId: 'dot-feed', name: '原料/換料', status: 'unavailable', detail: UNINSTRUMENTED_DETAIL },
    { dotId: 'dot-qc', name: '品質檢測', status: 'unavailable', detail: UNINSTRUMENTED_DETAIL },
    { dotId: 'dot-pack', name: '包裝', status: 'unavailable', detail: UNINSTRUMENTED_DETAIL },
  ]
}

// 四頁共用元件:注入 factory-map.svg,依各站點 status 換圓點顏色;
// 點擊/鍵盤 Enter 站點會在下方顯示該站訊號(REQ-02:點站點看訊號),
// 狀態同時補文字標籤,不只靠顏色傳達(390×844 與色弱使用者可讀)。
export function renderTwin(container: HTMLElement, stations: TwinStation[], options?: { mini?: boolean }): void {
  container.innerHTML = factoryMapSvg
  container.querySelector('svg')?.removeAttribute('role')
  container.classList.toggle('twin-mini', Boolean(options?.mini))

  const detail = document.createElement('div')
  detail.className = 'twin-detail'
  detail.setAttribute('aria-live', 'polite')
  detail.innerHTML = '<p class="twin-hint">點選站點圓點可查看目前訊號</p>'
  container.appendChild(detail)

  const showDetail = (station: TwinStation) => {
    detail.innerHTML = `
      <strong>${escapeHtml(station.name)}</strong>
      <span class="twin-status-tag status-${station.status}">${STATUS_LABEL[station.status]}</span>
      <p>${escapeHtml(station.detail)}</p>`
  }

  stations.forEach((station) => {
    const dot = container.querySelector<SVGCircleElement>(`#${station.dotId}`)
    if (!dot) return
    dot.style.fill = STATUS_COLOR[station.status] ?? STATUS_COLOR.normal
    dot.setAttribute('tabindex', '0')
    dot.setAttribute('role', 'button')
    dot.setAttribute('aria-label', `${station.name}:${STATUS_LABEL[station.status]},點選查看訊號`)
    dot.addEventListener('click', () => showDetail(station))
    dot.addEventListener('keydown', (event) => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault()
        showDetail(station)
      }
    })
  })
}
