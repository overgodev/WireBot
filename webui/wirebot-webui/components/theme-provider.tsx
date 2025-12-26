"use client"

import * as React from "react"
import { ThemeProvider as NextThemesProvider } from "next-themes"

export function ThemeProvider({ children }: { children: React.ReactNode }) {
  return (
    <NextThemesProvider
      attribute="class"
      forcedTheme="dark"        // <- hard lock to dark
      enableSystem={false}
      disableTransitionOnChange
      storageKey="wirebot-theme" // <- new key, ignores old `theme`
    >
      {children}
    </NextThemesProvider>
  )
}
