import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

export default defineConfig({
  plugins: [
    react(),
    tailwindcss(),
  ],
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:7800',
        changeOrigin: true,
        // Port is dynamic; dev mode always proxies to 7800 first
        // In production the frontend is served by the C++ server itself
      }
    }
  },
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
  }
})
