import React from 'react';
import {
  Home,
  Settings,
  Scissors,
  BarChart3,
  Wrench,
  Activity,
  Archive,
  Bell,
  FileText,
  Layers3,
  Target,
  Gauge,
  Database,
  Wifi,
  Shield
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
    description: "System overview and controls"
  },
  {
    title: "Wire Tracks",
    url: "/tracks",
    icon: Layers3,
    description: "MMU track management"
  },
  {
    title: "Job Queue",
    url: "/queue",
    icon: Archive,
    description: "Production planning"
  },
  {
    title: "Manual Control",
    url: "/control",
    icon: Target,
    description: "Direct machine control"
  },
  {
    title: "Production Stats",
    url: "/stats",
    icon: BarChart3,
    description: "Analytics and reports"
  }
]

const systemItems = [
  {
    title: "Calibration",
    url: "/calibration",
    icon: Gauge,
    description: "MMU and blade calibration"
  },
  {
    title: "Maintenance",
    url: "/maintenance",
    icon: Wrench,
    description: "System maintenance"
  },
  {
    title: "Activity Log",
    url: "/logs",
    icon: Activity,
    description: "System events"
  },
  {
    title: "Settings",
    url: "/settings",
    icon: Settings,
    description: "Configuration"
  }
]

const adminItems = [
  {
    title: "Database",
    url: "/database",
    icon: Database,
    description: "SQLite management"
  },
  {
    title: "Network",
    url: "/network",
    icon: Wifi,
    description: "Pi Zero connectivity"
  },
  {
    title: "Security",
    url: "/security",
    icon: Shield,
    description: "Access control"
  },
  {
    title: "Alerts",
    url: "/alerts",
    icon: Bell,
    description: "System notifications"
  },
  {
    title: "Documentation",
    url: "/docs",
    icon: FileText,
    description: "User manual"
  }
]

export function AppSidebar({ ...props }) {
  return (
    <Sidebar 
      variant="inset" 
      className="bg-gray-950 border-gray-800 text-gray-100"
      {...props}
    >
      <SidebarHeader className="border-b border-gray-800 p-4">
        <div className="flex items-center gap-3">
          <div className="relative">
            <div className="w-10 h-10 rounded-lg bg-gradient-to-br from-orange-500 to-orange-600 flex items-center justify-center">
              <Scissors className="w-6 h-6 text-white" />
            </div>
            <div className="absolute -top-1 -right-1 w-3 h-3 rounded-full bg-green-500 border-2 border-gray-950 animate-pulse" />
          </div>
          <div>
            <h2 className="font-bold text-gray-100 text-lg">WireBot</h2>
            <p className="text-xs text-orange-400 uppercase tracking-wider font-semibold">
              Multi-Track System
            </p>
          </div>
        </div>
      </SidebarHeader>
      
      <SidebarContent className="px-2">
        <SidebarGroup>
          <SidebarGroupLabel className="text-orange-400 font-semibold uppercase tracking-wider text-xs px-2">
            Operations
          </SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              {operationsItems.map((item) => (
                <SidebarMenuItem key={item.title}>
                  <SidebarMenuButton 
                    asChild
                    className="text-gray-300 hover:text-gray-100 hover:bg-gray-800 data-[state=open]:bg-gray-800 group"
                    tooltip={item.description}
                  >
                    <a href={item.url} className="flex items-center gap-3 px-3 py-2">
                      <item.icon className="w-4 h-4 text-gray-400 group-hover:text-orange-400 transition-colors" />
                      <div className="flex-1">
                        <span className="font-medium">{item.title}</span>
                        <div className="text-xs text-gray-500 group-hover:text-gray-400 transition-colors">
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
        
        <SidebarGroup>
          <SidebarGroupLabel className="text-orange-400 font-semibold uppercase tracking-wider text-xs px-2">
            System
          </SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              {systemItems.map((item) => (
                <SidebarMenuItem key={item.title}>
                  <SidebarMenuButton 
                    asChild
                    className="text-gray-300 hover:text-gray-100 hover:bg-gray-800 data-[state=open]:bg-gray-800 group"
                    tooltip={item.description}
                  >
                    <a href={item.url} className="flex items-center gap-3 px-3 py-2">
                      <item.icon className="w-4 h-4 text-gray-400 group-hover:text-blue-400 transition-colors" />
                      <div className="flex-1">
                        <span className="font-medium">{item.title}</span>
                        <div className="text-xs text-gray-500 group-hover:text-gray-400 transition-colors">
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
        
        <SidebarGroup>
          <SidebarGroupLabel className="text-orange-400 font-semibold uppercase tracking-wider text-xs px-2">
            Administration
          </SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              {adminItems.map((item) => (
                <SidebarMenuItem key={item.title}>
                  <SidebarMenuButton 
                    asChild
                    className="text-gray-300 hover:text-gray-100 hover:bg-gray-800 data-[state=open]:bg-gray-800 group"
                    tooltip={item.description}
                  >
                    <a href={item.url} className="flex items-center gap-3 px-3 py-2">
                      <item.icon className="w-4 h-4 text-gray-400 group-hover:text-purple-400 transition-colors" />
                      <div className="flex-1">
                        <span className="font-medium">{item.title}</span>
                        <div className="text-xs text-gray-500 group-hover:text-gray-400 transition-colors">
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
      
      <SidebarFooter className="border-t border-gray-800 p-4">
        <div className="space-y-2">
          <div className="flex items-center justify-between text-xs">
            <div className="text-gray-500">System Status</div>
            <div className="text-green-400 font-mono font-semibold">ONLINE</div>
          </div>
          
          <div className="grid grid-cols-2 gap-2 text-xs">
            <div className="text-center">
              <div className="text-gray-500">Pi Zero 2W</div>
              <div className="text-blue-400 font-semibold">Connected</div>
            </div>
            <div className="text-center">
              <div className="text-gray-500">Pico A+B</div>
              <div className="text-green-400 font-semibold">Ready</div>
            </div>
          </div>
          
          <div className="pt-2 border-t border-gray-800">
            <div className="flex justify-between text-xs text-gray-500">
              <span>v1.0.0</span>
              <span>5 Tracks</span>
            </div>
            <div className="text-center text-orange-400 font-mono text-xs mt-1">
              WireBot Control System
            </div>
          </div>
        </div>
      </SidebarFooter>
    </Sidebar>
  )
}