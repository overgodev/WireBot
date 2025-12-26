"use client"

import {
  AudioWaveform,
  BookOpen,
  Bot,
  Command,
  Frame,
  GalleryVerticalEnd,
  Map,
  PieChart,
  Settings2,
  SquareTerminal,
} from "lucide-react"

import { NavMain } from "@/components/nav-main"
import { NavProjects } from "@/components/nav-projects"
import { NavController } from "@/components/nav-controller"
import { TeamSwitcher } from "@/components/team-switcher"

import {
  Sidebar,
  SidebarContent,
  SidebarFooter,
  SidebarHeader,
  SidebarRail,
} from "@/components/ui/sidebar"

const data = {
  controller: {
    name: "WireBot Controller",
    host: "not connected",
    avatar: "/wirebot-logo.svg",
  },
  teams: [
    {
      name: "WireBot",
      logo: GalleryVerticalEnd,
      plan: "Local",
    },
  ],
  navMain: [
    {
      title: "Dashboard",
      url: "/dashboard",
      icon: SquareTerminal,
      isActive: true,
    },
    {
      title: "Control",
      url: "/dashboard/control",
      icon: Bot,
    },
    {
      title: "Diagnostics",
      url: "/dashboard/diagnostics",
      icon: Settings2,
    },
  ],
  projects: [
    {
      name: "Jobs",
      url: "/dashboard/jobs",
      icon: Frame,
    },
    {
      name: "Logs",
      url: "/dashboard/logs",
      icon: PieChart,
    },
  ],
}

export function AppSidebar() {
  return (
    <Sidebar collapsible="icon">
      <SidebarHeader>
        <TeamSwitcher teams={data.teams} />
      </SidebarHeader>

      <SidebarContent>
        <NavMain items={data.navMain} />
        <NavProjects projects={data.projects} />
      </SidebarContent>

      <SidebarFooter>
        <NavController controller={data.controller} />
      </SidebarFooter>

      <SidebarRail />
    </Sidebar>
  )
}
