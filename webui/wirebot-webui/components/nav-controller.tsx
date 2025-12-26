"use client"

import * as React from "react"
import { ChevronsUpDown, Link2, LogOut, RefreshCw } from "lucide-react"

import { Avatar, AvatarFallback, AvatarImage } from "@/components/ui/avatar"
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuLabel,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu"
import {
  SidebarMenu,
  SidebarMenuButton,
  SidebarMenuItem,
  useSidebar,
} from "@/components/ui/sidebar"

export function NavController({
  controller,
}: {
  controller: {
    name: string
    host: string
    avatar: string
  }
}) {
  const { isMobile } = useSidebar()
  const [host, setHost] = React.useState(controller.host)

  React.useEffect(() => {
    const saved = localStorage.getItem("wirebot_host")
    if (saved) setHost(saved)

    // keep updated if other tabs change it
    function onStorage(e: StorageEvent) {
      if (e.key === "wirebot_host") setHost(e.newValue || "not connected")
    }
    window.addEventListener("storage", onStorage)
    return () => window.removeEventListener("storage", onStorage)
  }, [])

  function clearConnection() {
    localStorage.removeItem("wirebot_host")
    localStorage.removeItem("wirebot_key")
    setHost("not connected")
  }

  return (
    <SidebarMenu>
      <SidebarMenuItem>
        <DropdownMenu>
          <DropdownMenuTrigger asChild>
            <SidebarMenuButton
              size="lg"
              className="data-[state=open]:bg-sidebar-accent data-[state=open]:text-sidebar-accent-foreground"
            >
              <Avatar className="h-8 w-8 rounded-lg">
                <AvatarImage src={controller.avatar} alt={controller.name} />
                <AvatarFallback className="rounded-lg">WB</AvatarFallback>
              </Avatar>

              <div className="grid flex-1 text-left text-sm leading-tight">
                <span className="truncate font-medium">{controller.name}</span>
                <span className="truncate text-xs">{host}</span>
              </div>

              <ChevronsUpDown className="ml-auto size-4" />
            </SidebarMenuButton>
          </DropdownMenuTrigger>

          <DropdownMenuContent
            className="w-(--radix-dropdown-menu-trigger-width) min-w-56 rounded-lg"
            side={isMobile ? "bottom" : "right"}
            align="end"
            sideOffset={4}
          >
            <DropdownMenuLabel className="p-0 font-normal">
              <div className="flex items-center gap-2 px-1 py-1.5 text-left text-sm">
                <Avatar className="h-8 w-8 rounded-lg">
                  <AvatarImage src={controller.avatar} alt={controller.name} />
                  <AvatarFallback className="rounded-lg">WB</AvatarFallback>
                </Avatar>
                <div className="grid flex-1 text-left text-sm leading-tight">
                  <span className="truncate font-medium">{controller.name}</span>
                  <span className="truncate text-xs">{host}</span>
                </div>
              </div>
            </DropdownMenuLabel>

            <DropdownMenuSeparator />

            <DropdownMenuItem onClick={() => setHost(localStorage.getItem("wirebot_host") || "not connected")}>
              <RefreshCw className="mr-2 size-4" />
              Refresh host
            </DropdownMenuItem>

            <DropdownMenuItem onClick={() => navigator.clipboard.writeText(host)}>
              <Link2 className="mr-2 size-4" />
              Copy host
            </DropdownMenuItem>

            <DropdownMenuSeparator />

            <DropdownMenuItem onClick={clearConnection}>
              <LogOut className="mr-2 size-4" />
              Disconnect
            </DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu>
      </SidebarMenuItem>
    </SidebarMenu>
  )
}
