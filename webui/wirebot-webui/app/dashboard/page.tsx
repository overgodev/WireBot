"use client"

import React, { useState } from "react"
import {
  Settings,
  Power,
  Play,
  Pause,
  Square,
  Home,
  Thermometer,
  Fan,
  AlertTriangle,
  CheckCircle,
  Clock,
  Layers,
  Zap,
  WifiOff,
  Wifi,
  Activity,
  BarChart3,
  Scissors,
} from "lucide-react"

import { AppSidebar } from "@/components/app-sidebar"
import {
  SidebarInset,
  SidebarProvider,
  SidebarTrigger,
} from "@/components/ui/sidebar"
import { Separator } from "@/components/ui/separator"

export default function DashboardPage() {
  return (
    <SidebarProvider>
      <AppSidebar />

      <SidebarInset>
        {/* Top bar (old layout) */}
        <header className="flex h-16 shrink-0 items-center gap-2 px-4 border-b border-border bg-background/80 backdrop-blur">
          <SidebarTrigger className="-ml-1" />
          <Separator orientation="vertical" className="h-4" />
          <h1 className="text-sm font-medium">Dashboard</h1>
        </header>

        {/* Main content – new WireBot dashboard inside old shell */}
        <main className="flex flex-1 flex-col gap-4 p-4 pt-2">
          <WireBotDashboard />
        </main>
      </SidebarInset>
    </SidebarProvider>
  )
}

// -------------------------------
// WireBot dashboard content
// -------------------------------

const WireBotDashboard = () => {
  const [isConnected, setIsConnected] = useState(false)
  const [machineStatus, setMachineStatus] = useState<"IDLE" | "RUNNING" | "ERROR" | "HOMING">("IDLE")
  const [picoAStatus, setPicoAStatus] = useState("Connected")
  const [picoBStatus, setPicoBStatus] = useState("Connected")
  const [temperature, setTemperature] = useState(42)
  const [fanSpeed, setFanSpeed] = useState(65)
  const [completedPieces, setCompletedPieces] = useState(0)
  const [totalPieces, setTotalPieces] = useState(100)
  const [currentLength, setCurrentLength] = useState(150)
  const [feedSpeed, setFeedSpeed] = useState(500)
  const [acceleration, setAcceleration] = useState(200)
  const [logs, setLogs] = useState<string[]>([
    "System initialized",
    "Controllers detected",
    "Ready for operation",
  ])

  // Job configuration
  const [wireLength, setWireLength] = useState(150)
  const [quantity, setQuantity] = useState(100)
  const [jobFeedSpeed, setJobFeedSpeed] = useState(500)
  const [jobAcceleration, setJobAcceleration] = useState(200)
  const [motionMode, setMotionMode] = useState("standard")

  const getStatusColor = (status: string) => {
    switch (status) {
      case "IDLE":
        return "text-yellow-400 border-yellow-400/30 bg-yellow-400/10"
      case "RUNNING":
        return "text-green-400 border-green-400/30 bg-green-400/10"
      case "ERROR":
        return "text-red-400 border-red-400/30 bg-red-400/10"
      case "HOMING":
        return "text-blue-400 border-blue-400/30 bg-blue-400/10"
      default:
        return "text-muted-foreground border-border/40 bg-muted/40"
    }
  }

  const getStatusIcon = () => {
    switch (machineStatus) {
      case "RUNNING":
        return <Activity className="w-4 h-4" />
      case "ERROR":
        return <AlertTriangle className="w-4 h-4" />
      case "IDLE":
        return <CheckCircle className="w-4 h-4" />
      case "HOMING":
        return <Home className="w-4 h-4" />
      default:
        return <Clock className="w-4 h-4" />
    }
  }

  const StatusCard = ({
    title,
    status,
    icon,
    subtitle,
  }: {
    title: string
    status: string
    icon: React.ReactNode
    subtitle?: string
  }) => (
    <div className="bg-background/70 border border-border/70 rounded-lg p-4 backdrop-blur-sm">
      <div className="flex items-center gap-2 mb-2">
        {icon}
        <span className="text-foreground font-semibold">{title}</span>
      </div>
      <div className={`text-xs px-2 py-1 rounded border ${getStatusColor(status)} font-mono`}>
        {status}
      </div>
      {subtitle && <div className="text-xs text-muted-foreground mt-1">{subtitle}</div>}
    </div>
  )

  const ControlButton = ({
    onClick,
    children,
    variant = "default",
    disabled = false,
  }: {
    onClick: () => void
    children: React.ReactNode
    variant?: "default" | "primary" | "danger" | "success"
    disabled?: boolean
  }) => {
    const baseClasses =
      "px-4 py-2 rounded-lg text-xs font-semibold transition-all duration-200 flex items-center gap-2"
    const variants: Record<string, string> = {
      default: "bg-muted hover:bg-muted/80 text-foreground border border-border",
      primary: "bg-blue-600 hover:bg-blue-500 text-white border border-blue-500",
      danger: "bg-red-600 hover:bg-red-500 text-white border border-red-500",
      success: "bg-green-600 hover:bg-green-500 text-white border border-green-500",
    }

    return (
      <button
        type="button"
        onClick={onClick}
        disabled={disabled}
        className={`${baseClasses} ${variants[variant]} ${
          disabled ? "opacity-50 cursor-not-allowed" : ""
        }`}
      >
        {children}
      </button>
    )
  }

  return (
    <div className="flex w-full rounded-xl border bg-background/80 text-foreground font-mono shadow-sm overflow-hidden">
      {/* Left sidebar - Controls */}
      <div className="w-80 border-r border-border bg-background/80 p-4 space-y-6">
        {/* Header inside dashboard */}
        <div className="flex items-center gap-3 pb-3 border-b border-border/60">
          <Scissors className="w-7 h-7 text-orange-400" />
          <div>
            <h2 className="text-lg font-bold text-foreground">WireBot</h2>
            <p className="text-[10px] text-muted-foreground uppercase tracking-widest">
              Automated Wire Cutting System
            </p>
          </div>
        </div>

        <div>
          <h3 className="text-xs font-bold mb-3 text-orange-400 uppercase tracking-wider">
            System Status
          </h3>
          <div className="space-y-3">
            <StatusCard
              title="Main Controller"
              status={isConnected ? "ONLINE" : "OFFLINE"}
              icon={<Power className="w-4 h-4" />}
            />
            <StatusCard
              title="Pico A (Motion)"
              status={picoAStatus}
              icon={<Zap className="w-4 h-4" />}
              subtitle="5-axis stepper control"
            />
            <StatusCard
              title="Pico B (I/O)"
              status={picoBStatus}
              icon={<Settings className="w-4 h-4" />}
              subtitle="Servos, fans, sensors"
            />
          </div>
        </div>

        <div>
          <h3 className="text-xs font-bold mb-3 text-orange-400 uppercase tracking-wider">
            Machine Controls
          </h3>
          <div className="space-y-2">
            <ControlButton onClick={() => {}} variant="primary" disabled={!isConnected}>
              <Home className="w-4 h-4" />
              Home All Axes
            </ControlButton>
            <ControlButton onClick={() => {}} variant="success" disabled={machineStatus !== "IDLE"}>
              <Play className="w-4 h-4" />
              Start Job
            </ControlButton>
            <ControlButton onClick={() => {}} variant="default" disabled={machineStatus !== "RUNNING"}>
              <Pause className="w-4 h-4" />
              Pause
            </ControlButton>
            <ControlButton onClick={() => {}} variant="danger" disabled={machineStatus === "IDLE"}>
              <Square className="w-4 h-4" />
              Emergency Stop
            </ControlButton>
          </div>
        </div>

        <div>
          <h3 className="text-xs font-bold mb-3 text-orange-400 uppercase tracking-wider">
            Environment
          </h3>
          <div className="space-y-3">
            <div className="bg-background/70 p-3 rounded border border-border/70">
              <div className="flex items-center justify-between mb-2">
                <div className="flex items-center gap-2">
                  <Thermometer className="w-4 h-4 text-blue-400" />
                  <span className="text-xs">Temperature</span>
                </div>
                <span className="text-base font-bold text-blue-400">{temperature}°C</span>
              </div>
              <div className="w-full bg-muted rounded-full h-2">
                <div
                  className="bg-blue-400 h-2 rounded-full transition-all duration-300"
                  style={{ width: `${Math.min((temperature / 80) * 100, 100)}%` }}
                ></div>
              </div>
            </div>

            <div className="bg-background/70 p-3 rounded border border-border/70">
              <div className="flex items-center justify-between mb-2">
                <div className="flex items-center gap-2">
                  <Fan className="w-4 h-4 text-green-400" />
                  <span className="text-xs">Fan Speed</span>
                </div>
                <span className="text-base font-bold text-green-400">{fanSpeed}%</span>
              </div>
              <div className="w-full bg-muted rounded-full h-2">
                <div
                  className="bg-green-400 h-2 rounded-full transition-all duration-300"
                  style={{ width: `${fanSpeed}%` }}
                ></div>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Main content area */}
      <div className="flex-1 p-4 space-y-4">
        {/* Top status bar from new design */}
        <div className="flex items-center justify-between rounded-lg border border-border bg-background/80 px-4 py-3">
          <div className="flex items-center gap-3">
            <div
              className={`flex items-center gap-2 px-3 py-1 rounded border text-xs ${getStatusColor(
                machineStatus,
              )}`}
            >
              {getStatusIcon()}
              <span className="font-semibold">{machineStatus}</span>
            </div>
          </div>
          <div className="flex items-center gap-3 text-xs">
            <div className="flex items-center gap-1">
              {isConnected ? (
                <Wifi className="w-4 h-4 text-green-400" />
              ) : (
                <WifiOff className="w-4 h-4 text-red-400" />
              )}
              <span>{isConnected ? "CONNECTED" : "OFFLINE"}</span>
            </div>
          </div>
        </div>

        {/* Job Configuration */}
        <div className="bg-background/80 border border-border rounded-lg p-4 backdrop-blur-sm">
          <h2 className="text-sm font-bold mb-4 text-orange-400 uppercase tracking-wider">
            Job Configuration
          </h2>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div className="space-y-3">
              <div>
                <label className="block text-xs font-semibold mb-1 text-muted-foreground">
                  Wire Length (mm)
                </label>
                <input
                  type="number"
                  value={wireLength}
                  onChange={(e) => setWireLength(Number(e.target.value))}
                  className="w-full bg-muted border border-border rounded px-3 py-2 text-xs text-foreground focus:border-orange-400 focus:outline-none"
                  min={1}
                />
              </div>

              <div>
                <label className="block text-xs font-semibold mb-1 text-muted-foreground">
                  Quantity (pieces)
                </label>
                <input
                  type="number"
                  value={quantity}
                  onChange={(e) => setQuantity(Number(e.target.value))}
                  className="w-full bg-muted border border-border rounded px-3 py-2 text-xs text-foreground focus:border-orange-400 focus:outline-none"
                  min={1}
                />
              </div>

              <div>
                <label className="block text-xs font-semibold mb-1 text-muted-foreground">
                  Motion Profile
                </label>
                <select
                  value={motionMode}
                  onChange={(e) => setMotionMode(e.target.value)}
                  className="w-full bg-muted border border-border rounded px-3 py-2 text-xs text-foreground focus:border-orange-400 focus:outline-none"
                >
                  <option value="standard">Standard</option>
                  <option value="precision">Precision</option>
                  <option value="fast">Fast</option>
                  <option value="custom">Custom</option>
                </select>
              </div>
            </div>

            <div className="space-y-3">
              <div>
                <label className="block text-xs font-semibold mb-1 text-muted-foreground">
                  Feed Speed (mm/min)
                </label>
                <input
                  type="number"
                  value={jobFeedSpeed}
                  onChange={(e) => setJobFeedSpeed(Number(e.target.value))}
                  className="w-full bg-muted border border-border rounded px-3 py-2 text-xs text-foreground focus:border-orange-400 focus:outline-none"
                  min={1}
                />
              </div>

              <div>
                <label className="block text-xs font-semibold mb-1 text-muted-foreground">
                  Acceleration (mm/s²)
                </label>
                <input
                  type="number"
                  value={jobAcceleration}
                  onChange={(e) => setJobAcceleration(Number(e.target.value))}
                  className="w-full bg-muted border border-border rounded px-3 py-2 text-xs text-foreground focus:border-orange-400 focus:outline-none"
                  min={1}
                />
              </div>

              <div className="flex gap-2 pt-1">
                <ControlButton onClick={() => {}} variant="primary" disabled={machineStatus !== "IDLE"}>
                  <Play className="w-4 h-4" />
                  Start Production
                </ControlButton>
                <ControlButton onClick={() => {}} variant="default" disabled={machineStatus !== "IDLE"}>
                  <BarChart3 className="w-4 h-4" />
                  Dry Run
                </ControlButton>
              </div>
            </div>
          </div>
        </div>

        {/* Progress and Stats */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div className="bg-background/80 border border-border rounded-lg p-4 backdrop-blur-sm">
            <div className="flex items-center gap-2 mb-3">
              <Layers className="w-4 h-4 text-blue-400" />
              <h3 className="text-xs font-bold text-blue-400 uppercase tracking-wider">
                Production Progress
              </h3>
            </div>
            <div className="space-y-2">
              <div className="flex justify-between items-end">
                <span className="text-xl font-bold text-foreground">
                  {completedPieces}
                </span>
                <span className="text-[11px] text-muted-foreground">
                  / {totalPieces} pieces
                </span>
              </div>
              <div className="w-full bg-muted rounded-full h-2.5">
                <div
                  className="bg-blue-400 h-2.5 rounded-full transition-all duration-500"
                  style={{ width: `${(completedPieces / totalPieces) * 100}%` }}
                ></div>
              </div>
              <div className="text-[11px] text-muted-foreground">
                {((completedPieces / totalPieces) * 100).toFixed(1)}% Complete
              </div>
            </div>
          </div>

          <div className="bg-background/80 border border-border rounded-lg p-4 backdrop-blur-sm">
            <div className="flex items-center gap-2 mb-3">
              <Scissors className="w-4 h-4 text-green-400" />
              <h3 className="text-xs font-bold text-green-400 uppercase tracking-wider">
                Current Job
              </h3>
            </div>
            <div className="space-y-1.5 text-xs">
              <div className="flex justify-between">
                <span className="text-muted-foreground">Length:</span>
                <span className="text-foreground font-semibold">{currentLength}mm</span>
              </div>
              <div className="flex justify-between">
                <span className="text-muted-foreground">Feed:</span>
                <span className="text-foreground font-semibold">{feedSpeed}mm/min</span>
              </div>
              <div className="flex justify-between">
                <span className="text-muted-foreground">Accel:</span>
                <span className="text-foreground font-semibold">{acceleration}mm/s²</span>
              </div>
            </div>
          </div>

          <div className="bg-background/80 border border-border rounded-lg p-4 backdrop-blur-sm">
            <div className="flex items-center gap-2 mb-3">
              <Clock className="w-4 h-4 text-yellow-400" />
              <h3 className="text-xs font-bold text-yellow-400 uppercase tracking-wider">
                Runtime Stats
              </h3>
            </div>
            <div className="space-y-1.5 text-xs">
              <div className="flex justify-between">
                <span className="text-muted-foreground">Uptime:</span>
                <span className="text-foreground font-semibold">2h 34m</span>
              </div>
              <div className="flex justify-between">
                <span className="text-muted-foreground">Efficiency:</span>
                <span className="text-foreground font-semibold">94.2%</span>
              </div>
              <div className="flex justify-between">
                <span className="text-muted-foreground">Est. Complete:</span>
                <span className="text-foreground font-semibold">14:32</span>
              </div>
            </div>
          </div>
        </div>

        {/* Live Activity Log */}
        <div className="bg-background/80 border border-border rounded-lg p-4 backdrop-blur-sm">
          <h2 className="text-xs font-bold mb-3 text-orange-400 uppercase tracking-wider flex items-center gap-2">
            <Activity className="w-4 h-4" />
            Live Activity Log
          </h2>
          <div className="bg-black/40 dark:bg-black/60 rounded border border-border/60 p-3 h-40 overflow-y-auto text-[11px]">
            <div className="space-y-1 font-mono">
              {logs.map((log, i) => (
                <div key={i} className="text-green-400">
                  <span className="text-gray-500 mr-1">
                    [{new Date().toLocaleTimeString()}]
                  </span>
                  {log}
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  )
}
