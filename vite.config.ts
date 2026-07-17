import { defineConfig } from 'vite'
import { viteSingleFile } from 'vite-plugin-singlefile'

// OFFLINE=1 時把所有 JS/CSS/資源內嵌成單一 HTML(展示日用,可 file:// 直接開)
// 平時 build 供 GitHub Pages(base './' 讓相對路徑在任何子路徑都能載入)
export default defineConfig({
  base: './',
  plugins: process.env.OFFLINE ? [viteSingleFile()] : [],
})
