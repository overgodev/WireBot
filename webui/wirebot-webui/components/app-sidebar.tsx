"use client"

import React from "react"
import {
  Home,
  Settings,
  Scissors,
  BarChart3,
  Wrench,
  Activity,
  Bell,
  FileText,
  Layers3,
  Target,
  Gauge,
  Database,
  Wifi,
  Shield,
} from "lucide-react"

import {
  Sidebar,
  SidebarContent,
  SidebarFooter,
  SidebarGroup,
  SidebarGroupContent,
  SidebarGroupLabel,
  SidebarHeader,
  SidebarMenu,
  SidebarMenuButton,
  SidebarMenuItem,
} from "@/components/ui/sidebar"

const operationsItems = [
  {
    title: "Dashboard",
    url: "/dashboard",
    icon: Home,
    description: "System overview and controls",
  },
  {
    title: "Wire Tracks",
    url: "/tracks",
    icon: Layers3,
    description: "MMU track management",
  },
  {
    title: "Job Queue",
    url: "/queue",
    icon: Target,
    description: "Production planning",
  },
  {
    title: "Manual Control",
    url: "/control",
    icon: Gauge,
    description: "Direct machine control",
  },
  {
    title: "Production Stats",
    url: "/stats",
    icon: BarChart3,
    description: "Analytics and reports",
  },
]

const systemItems = [
  {
    title: "Calibration",
    url: "/calibration",
    icon: Gauge,
    description: "MMU and blade calibration",
  },
  {
    title: "Maintenance",
    url: "/maintenance",
    icon: Wrench,
    description: "System maintenance",
  },
  {
    title: "Activity Log",
    url: "/logs",
    icon: Activity,
    description: "System events",
  },
  {
    title: "Settings",
    url: "/settings",
    icon: Settings,
    description: "Configuration",
  },
]

const adminItems = [
  {
    title: "Database",
    url: "/database",
    icon: Database,
    description: "SQLite management",
  },
  {
    title: "Network",
    url: "/network",
    icon: Wifi,
    description: "Pi Zero connectivity",
  },
  {
    title: "Security",
    url: "/security",
    icon: Shield,
    description: "Access control",
  },
  {
    title: "Alerts",
    url: "/alerts",
    icon: Bell,
    description: "System notifications",
  },
  {
    title: "Documentation",
    url: "/docs",
    icon: FileText,
    description: "User manual",
  },
]

export function AppSidebar(props: React.ComponentProps<typeof Sidebar>) {
  return (
    <Sidebar
      // remove "inset" so it behaves like a normal left-rail
      className="bg-gray-950 text-gray-100 border-r border-gray-800"
      {...props}
    >
      {/* HEADER */}
      <SidebarHeader className="border-b border-gray-800 px-4 py-3">
        <div className="flex items-center gap-3">
          <div className="relative">
            <div className="w-10 h-10 rounded-lg bg-gradient-to-br from-orange-500 to-orange-600 flex items-center justify-center">
              <Scissors className="w-6 h-6 text-white" />
            </div>
            <div className="absolute -top-1 -right-1 w-3 h-3 rounded-full bg-green-500 border-2 border-gray-950 animate-pulse" />
          </div>
          <div className="flex flex-col">
            <h2 className="font-bold text-gray-100 text-lg leading-tight">
              WireBot
            </h2>
            <p className="text-[11px] text-orange-400 uppercase tracking-[0.18em] font-semibold">
              Multi-Track System
            </p>
          </div>
        </div>
      </SidebarHeader>

      {/* CONTENT */}
      <SidebarContent className="px-0 py-2">
        {/* OPERATIONS */}
        <SidebarGroup>
          <SidebarGroupLabel className="text-orange-400 font-semibold uppercase tracking-wider text-[11px] px-4 pb-1">
            Operations
          </SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              {operationsItems.map((item) => (
                <SidebarMenuItem key={item.title}>
                  <SidebarMenuButton
                    asChild
                    className="text-gray-300 hover:text-gray-100 hover:bg-gray-800/80 data-[state=open]:bg-gray-800/80 group"
                    tooltip={item.description}
                  >
                    <a href={item.url} className="flex items-start gap-3 px-4 py-2.5">
                      <item.icon className="mt-0.5 w-4 h-4 text-gray-400 group-hover:text-orange-400 transition-colors" />
                      <div className="flex-1">
                        <span className="font-medium text-sm leading-tight">
                          {item.title}
                        </span>
                        <div className="text-[11px] text-gray-500 group-hover:text-gray-400 transition-colors">
                          {item.description}
                        </div>
                      </div>
                    </a>
                  </SidebarMenuButton>
                </SidebarMenuItem>
              ))}
            </SidebarMenu>
          </SidebarGroupContent>
        </SidebarGroup>

        {/* SYSTEM */}
        <SidebarGroup>
          <SidebarGroupLabel className="text-orange-400 font-semibold uppercase tracking-wider text-[11px] px-4 pb-1 pt-3">
            System
          </SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              {systemItems.map((item) => (
                <SidebarMenuItem key={item.title}>
                  <SidebarMenuButton
                    asChild
                    className="text-gray-300 hover:text-gray-100 hover:bg-gray-800/80 data-[state=open]:bg-gray-800/80 group"
                    tooltip={item.description}
                  >
                    <a href={item.url} className="flex items-start gap-3 px-4 py-2.5">
                      <item.icon className="mt-0.5 w-4 h-4 text-gray-400 group-hover:text-blue-400 transition-colors" />
                      <div className="flex-1">
                        <span className="font-medium text-sm leading-tight">
                          {item.title}
                        </span>
                        <div className="text-[11px] text-gray-500 group-hover:text-gray-400 transition-colors">
                          {item.description}
                        </div>
                      </div>
                    </a>
                  </SidebarMenuButton>
                </SidebarMenuItem>
              ))}
            </SidebarMenu>
          </SidebarGroupContent>
        </SidebarGroup>

        {/* ADMIN */}
        <SidebarGroup>
          <SidebarGroupLabel className="text-orange-400 font-semibold uppercase tracking-wider text-[11px] px-4 pb-1 pt-3">
            Administration
          </SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              {adminItems.map((item) => (
                <SidebarMenuItem key={item.title}>
                  <SidebarMenuButton
                    asChild
                    className="text-gray-300 hover:text-gray-100 hover:bg-gray-800/80 data-[state=open]:bg-gray-800/80 group"
                    tooltip={item.description}
                  >
                    <a href={item.url} className="flex items-start gap-3 px-4 py-2.5">
                      <item.icon className="mt-0.5 w-4 h-4 text-gray-400 group-hover:text-purple-400 transition-colors" />
                      <div className="flex-1">
                        <span className="font-medium text-sm leading-tight">
                          {item.title}
                        </span>
                        <div className="text-[11px] text-gray-500 group-hover:text-gray-400 transition-colors">
                          {item.description}
                        </div>
                      </div>
                    </a>
                  </SidebarMenuButton>
                </SidebarMenuItem>
              ))}
            </SidebarMenu>
          </SidebarGroupContent>
        </SidebarGroup>
      </SidebarContent>

      {/* FOOTER */}
      <SidebarFooter className="border-t border-gray-800 px-4 py-3">
        <div className="space-y-2 text-xs">
          <div className="flex items-center justify-between">
            <span className="text-gray-500">System Status</span>
            <span className="text-green-400 font-mono font-semibold">ONLINE</span>
          </div>

          <div className="grid grid-cols-2 gap-2">
            <div className="text-left">
              <div className="text-gray-500">Pi Zero 2W</div>
              <div className="text-blue-400 font-semibold">Connected</div>
            </div>
            <div className="text-right">
              <div className="text-gray-500">Pico A+B</div>
              <div className="text-green-400 font-semibold">Ready</div>
            </div>
          </div>

          <div className="pt-2 border-t border-gray-800">
            <div className="flex justify-between text-gray-500">
              <span>v1.0.0</span>
              <span>5 Tracks</span>
            </div>
            <div className="text-center text-orange-400 font-mono text-[11px] mt-1">
              WireBot Control System
            </div>
          </div>
        </div>
      </SidebarFooter>
    </Sidebar>
  )
}
