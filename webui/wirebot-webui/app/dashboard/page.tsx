'use client'

import React, { useState, useEffect } from 'react';
import { AppSidebar } from "@/components/app-sidebar"
import {
  SidebarInset,
  SidebarProvider,
  SidebarTrigger,
} from "@/components/ui/sidebar"
import { Separator } from "@/components/ui/separator"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Badge } from "@/components/ui/badge"
import { Progress } from "@/components/ui/progress"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { Textarea } from "@/components/ui/textarea"
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
  Zap,
  WifiOff,
  Wifi,
  Activity,
  BarChart3,
  Scissors,
  Cable,
  Layers3,
  ChevronRight,
  Edit3,
  Plus,
  Trash2,
  RotateCcw,
  Target,
  Move3D,
  Gauge
} from 'lucide-react';

interface LogEntry {
  time: string;
  message: string;
  type: 'info' | 'success' | 'warning' | 'error';
}

interface WireTrack {
  id: number;
  name: string;
  wireType: string;
  gauge: string;
  color: string;
  material: string;
  stockLength: number;
  remainingLength: number;
  isActive: boolean;
  position: number; // MMU position
  lastUsed: string;
}

interface JobConfig {
  selectedTrack: number | null;
  wireLength: number;
  quantity: number;
  feedSpeed: number;
  acceleration: number;
  stripLength: number;
  motionProfile: string;
  notes: string;
}

type MachineStatus = 'IDLE' | 'RUNNING' | 'ERROR' | 'HOMING' | 'SELECTING' | 'PRELOADING' | 'FEEDING' | 'CUTTING' | 'STRIPPING';
type WorkflowStep = 'track_select' | 'preload' | 'feed' | 'strip' | 'cut' | 'complete';

export default function WireBotDashboard() {
  const [isConnected, setIsConnected] = useState<boolean>(true);
  const [machineStatus, setMachineStatus] = useState<MachineStatus>('IDLE');
  const [picoAStatus, setPicoAStatus] = useState<string>('Connected');
  const [picoBStatus, setPicoBStatus] = useState<string>('Connected');
  const [temperature, setTemperature] = useState<number>(42);
  const [fanSpeed, setFanSpeed] = useState<number>(65);
  const [completedPieces, setCompletedPieces] = useState<number>(23);
  const [totalPieces, setTotalPieces] = useState<number>(100);
  const [currentWorkflowStep, setCurrentWorkflowStep] = useState<WorkflowStep>('track_select');
  
  // Wire tracks from SQLite
  const [wireTracks, setWireTracks] = useState<WireTrack[]>([
    {
      id: 1,
      name: "Red 18AWG Stranded",
      wireType: "Stranded Copper",
      gauge: "18 AWG",
      color: "#ef4444",
      material: "Copper",
      stockLength: 1000,
      remainingLength: 847,
      isActive: true,
      position: 1,
      lastUsed: "2024-12-26 13:45"
    },
    {
      id: 2,
      name: "Black 18AWG Stranded", 
      wireType: "Stranded Copper",
      gauge: "18 AWG",
      color: "#1f2937",
      material: "Copper",
      stockLength: 1000,
      remainingLength: 692,
      isActive: true,
      position: 2,
      lastUsed: "2024-12-26 14:12"
    },
    {
      id: 3,
      name: "Blue 22AWG Solid",
      wireType: "Solid Copper", 
      gauge: "22 AWG",
      color: "#3b82f6",
      material: "Copper",
      stockLength: 500,
      remainingLength: 234,
      isActive: true,
      position: 3,
      lastUsed: "2024-12-25 16:30"
    },
    {
      id: 4,
      name: "Green 20AWG Stranded",
      wireType: "Stranded Copper",
      gauge: "20 AWG", 
      color: "#10b981",
      material: "Copper",
      stockLength: 750,
      remainingLength: 0,
      isActive: false,
      position: 4,
      lastUsed: "2024-12-24 09:15"
    },
    {
      id: 5,
      name: "White 16AWG Stranded",
      wireType: "Stranded Copper",
      gauge: "16 AWG",
      color: "#f3f4f6", 
      material: "Copper",
      stockLength: 1500,
      remainingLength: 1340,
      isActive: true,
      position: 5,
      lastUsed: "2024-12-26 10:22"
    }
  ]);

  const [jobConfig, setJobConfig] = useState<JobConfig>({
    selectedTrack: null,
    wireLength: 150,
    quantity: 100,
    feedSpeed: 500,
    acceleration: 200,
    stripLength: 10,
    motionProfile: 'standard',
    notes: ''
  });
  
  const [logs, setLogs] = useState<LogEntry[]>([
    { time: '14:23:45', message: 'System initialized successfully', type: 'info' },
    { time: '14:23:46', message: 'Pico A motion controller detected', type: 'success' },
    { time: '14:23:46', message: 'Pico B I/O controller detected', type: 'success' },
    { time: '14:23:47', message: 'MMU belt homed to position', type: 'success' },
    { time: '14:23:48', message: 'Blade axis homed to closed position', type: 'success' },
    { time: '14:24:12', message: '5 wire tracks loaded from database', type: 'info' },
    { time: '14:24:13', message: 'Ready for operation', type: 'success' },
  ]);

  const getStatusVariant = (status: MachineStatus) => {
    switch(status) {
      case 'IDLE': return 'secondary' as const;
      case 'RUNNING': return 'default' as const;
      case 'SELECTING': return 'outline' as const;
      case 'PRELOADING': return 'outline' as const;
      case 'FEEDING': return 'outline' as const;
      case 'CUTTING': return 'outline' as const;
      case 'STRIPPING': return 'outline' as const;
      case 'ERROR': return 'destructive' as const;
      case 'HOMING': return 'outline' as const;
      default: return 'secondary' as const;
    }
  };

  const getStatusIcon = () => {
    switch(machineStatus) {
      case 'RUNNING': return <Activity className="w-4 h-4" />;
      case 'ERROR': return <AlertTriangle className="w-4 h-4" />;
      case 'IDLE': return <CheckCircle className="w-4 h-4" />;
      case 'HOMING': return <Home className="w-4 h-4" />;
      case 'SELECTING': return <Target className="w-4 h-4" />;
      case 'PRELOADING': return <Move3D className="w-4 h-4" />;
      case 'FEEDING': return <ChevronRight className="w-4 h-4" />;
      case 'CUTTING': return <Scissors className="w-4 h-4" />;
      case 'STRIPPING': return <Cable className="w-4 h-4" />;
      default: return <Clock className="w-4 h-4" />;
    }
  };

  const getWorkflowStepStatus = (step: WorkflowStep) => {
    const stepOrder: WorkflowStep[] = ['track_select', 'preload', 'feed', 'strip', 'cut', 'complete'];
    const currentIndex = stepOrder.indexOf(currentWorkflowStep);
    const stepIndex = stepOrder.indexOf(step);
    
    if (stepIndex < currentIndex) return 'complete';
    if (stepIndex === currentIndex) return 'active';
    return 'pending';
  };

  const addLog = (message: string, type: LogEntry['type'] = 'info') => {
    const newLog: LogEntry = {
      time: new Date().toLocaleTimeString(),
      message,
      type
    };
    setLogs(prev => [newLog, ...prev.slice(0, 9)]);
  };

  const selectTrack = (trackId: number) => {
    const track = wireTracks.find(t => t.id === trackId);
    if (!track) return;
    
    setJobConfig(prev => ({ ...prev, selectedTrack: trackId }));
    addLog(`Selected track ${track.position}: ${track.name}`, 'info');
  };

  const handleStartJob = () => {
    if (!jobConfig.selectedTrack) {
      addLog('No wire track selected', 'error');
      return;
    }
    
    const selectedTrack = wireTracks.find(t => t.id === jobConfig.selectedTrack);
    if (!selectedTrack?.isActive) {
      addLog('Selected track is not active or out of wire', 'error');
      return;
    }

    setMachineStatus('SELECTING');
    setCurrentWorkflowStep('track_select');
    addLog(`Starting job: ${jobConfig.quantity} pieces @ ${jobConfig.wireLength}mm`, 'success');
    addLog(`Using track ${selectedTrack.position}: ${selectedTrack.name}`, 'info');
    
    // Simulate workflow progression
    setTimeout(() => {
      setMachineStatus('PRELOADING');
      setCurrentWorkflowStep('preload');
      addLog(`Moving MMU to track ${selectedTrack.position}`, 'info');
    }, 2000);
    
    setTimeout(() => {
      setMachineStatus('FEEDING');
      setCurrentWorkflowStep('feed');
      addLog('Preload complete, wire ready for feeding', 'success');
    }, 4000);
    
    setTimeout(() => {
      setMachineStatus('RUNNING');
      addLog('Wire feeding started', 'info');
    }, 6000);
  };

  const handleStopJob = () => {
    setMachineStatus('IDLE');
    setCurrentWorkflowStep('track_select');
    addLog('Job stopped by user', 'warning');
  };

  const handleHomeAxes = () => {
    setMachineStatus('HOMING');
    addLog('Homing all axes...', 'info');
    setTimeout(() => {
      setMachineStatus('IDLE');
      addLog('MMU belt homed, blade axis homed to closed position', 'success');
    }, 3000);
  };

  const TrackSelector = () => (
    <Card>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Layers3 className="w-5 h-5" />
          Wire Track Selection
        </CardTitle>
        <CardDescription>
          Choose which wire track to use for this job
        </CardDescription>
      </CardHeader>
      <CardContent>
        <div className="grid gap-3">
          {wireTracks.map((track) => (
            <div
              key={track.id}
              className={`
                border rounded-lg p-3 cursor-pointer transition-all duration-200
                ${jobConfig.selectedTrack === track.id 
                  ? 'border-orange-500 bg-orange-500/5 ring-1 ring-orange-500/20' 
                  : 'border-gray-200 dark:border-gray-700 hover:border-gray-300 dark:hover:border-gray-600'
                }
                ${!track.isActive ? 'opacity-50 cursor-not-allowed' : ''}
              `}
              onClick={() => track.isActive && selectTrack(track.id)}
            >
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-3">
                  <div className="flex items-center gap-2">
                    <div 
                      className="w-4 h-4 rounded border-2 border-gray-300"
                      style={{ backgroundColor: track.color }}
                    />
                    <Badge variant="outline" className="text-xs">
                      T{track.position}
                    </Badge>
                  </div>
                  <div>
                    <div className="font-semibold text-sm">{track.name}</div>
                    <div className="text-xs text-muted-foreground">
                      {track.gauge} • {track.wireType} • {track.material}
                    </div>
                  </div>
                </div>
                <div className="text-right">
                  <div className="text-sm font-medium">
                    {track.remainingLength}m
                  </div>
                  <div className="text-xs text-muted-foreground">
                    {track.isActive ? 'Available' : 'Empty'}
                  </div>
                </div>
              </div>
              
              {/* Stock level indicator */}
              <div className="mt-2">
                <div className="flex justify-between text-xs mb-1">
                  <span>Stock Level</span>
                  <span>{Math.round((track.remainingLength / track.stockLength) * 100)}%</span>
                </div>
                <Progress 
                  value={(track.remainingLength / track.stockLength) * 100} 
                  className="h-1"
                />
              </div>
            </div>
          ))}
        </div>
        
        <div className="flex gap-2 mt-4">
          <Button variant="outline" size="sm" className="flex-1">
            <Edit3 className="w-4 h-4 mr-2" />
            Manage Tracks
          </Button>
          <Button variant="outline" size="sm" className="flex-1">
            <Plus className="w-4 h-4 mr-2" />
            Add Track
          </Button>
        </div>
      </CardContent>
    </Card>
  );

  const WorkflowProgress = () => {
    const steps = [
      { key: 'track_select', label: 'Track Select', icon: Target },
      { key: 'preload', label: 'Preload', icon: Move3D },
      { key: 'feed', label: 'Feed', icon: ChevronRight },
      { key: 'strip', label: 'Strip', icon: Cable },
      { key: 'cut', label: 'Cut', icon: Scissors },
      { key: 'complete', label: 'Complete', icon: CheckCircle }
    ] as const;

    return (
      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <BarChart3 className="w-5 h-5" />
            Workflow Progress
          </CardTitle>
          <CardDescription>
            Current stage in the wire processing workflow
          </CardDescription>
        </CardHeader>
        <CardContent>
          <div className="space-y-3">
            {steps.map((step, index) => {
              const status = getWorkflowStepStatus(step.key);
              const Icon = step.icon;
              
              return (
                <div key={step.key} className="flex items-center gap-3">
                  <div className={`
                    w-8 h-8 rounded-full flex items-center justify-center text-sm font-medium
                    ${status === 'complete' ? 'bg-green-500 text-white' : ''}
                    ${status === 'active' ? 'bg-orange-500 text-white' : ''}
                    ${status === 'pending' ? 'bg-gray-200 text-gray-500 dark:bg-gray-700 dark:text-gray-400' : ''}
                  `}>
                    <Icon className="w-4 h-4" />
                  </div>
                  <div className="flex-1">
                    <div className={`text-sm font-medium ${
                      status === 'active' ? 'text-orange-500' : 
                      status === 'complete' ? 'text-green-600' : 
                      'text-muted-foreground'
                    }`}>
                      {step.label}
                    </div>
                  </div>
                  <div>
                    {status === 'complete' && (
                      <CheckCircle className="w-4 h-4 text-green-500" />
                    )}
                    {status === 'active' && (
                      <div className="w-2 h-2 rounded-full bg-orange-500 animate-pulse" />
                    )}
                  </div>
                </div>
              );
            })}
          </div>
        </CardContent>
      </Card>
    );
  };

  return (
    <SidebarProvider>
      <AppSidebar />
      <SidebarInset>
        {/* Top bar */}
        <header className="flex h-16 shrink-0 items-center gap-2 px-4 border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60 sticky top-0 z-50">
          <SidebarTrigger className="-ml-1" />
          <Separator orientation="vertical" className="h-4" />
          <div className="flex items-center gap-4 flex-1">
            <div className="flex items-center gap-2">
              <div className="relative">
                <Scissors className="w-6 h-6 text-orange-500" />
                <div className="absolute -top-1 -right-1 w-3 h-3 rounded-full bg-green-500 border-2 border-background animate-pulse" />
              </div>
              <div>
                <h1 className="text-lg font-bold">WireBot</h1>
                <p className="text-xs text-muted-foreground">Multi-Track Wire Processing</p>
              </div>
            </div>
            
            <div className="flex items-center gap-4 ml-auto">
              <Badge variant={getStatusVariant(machineStatus)} className="flex items-center gap-1 font-mono">
                {getStatusIcon()}
                {machineStatus}
              </Badge>
              <div className="flex items-center gap-1">
                {isConnected ? (
                  <Wifi className="w-4 h-4 text-green-500" />
                ) : (
                  <WifiOff className="w-4 h-4 text-red-500" />
                )}
                <span className="text-sm text-muted-foreground font-mono">
                  {isConnected ? 'ONLINE' : 'OFFLINE'}
                </span>
              </div>
            </div>
          </div>
        </header>

        {/* Main content */}
        <main className="flex flex-1 flex-col gap-6 p-6">
          {/* System Status Cards */}
          <div className="grid auto-rows-min gap-4 md:grid-cols-4">
            <Card className="relative overflow-hidden">
              <div className="absolute top-0 left-0 right-0 h-0.5 bg-gradient-to-r from-orange-500 to-orange-600" />
              <CardHeader className="pb-2">
                <CardTitle className="text-sm flex items-center gap-2">
                  <Power className="w-4 h-4" />
                  Pi Zero 2W
                </CardTitle>
              </CardHeader>
              <CardContent>
                <div className="text-xl font-bold">
                  {isConnected ? 'Online' : 'Offline'}
                </div>
                <p className="text-xs text-muted-foreground">
                  Web UI + Job Logic
                </p>
              </CardContent>
            </Card>

            <Card className="relative overflow-hidden">
              <div className="absolute top-0 left-0 right-0 h-0.5 bg-gradient-to-r from-blue-500 to-blue-600" />
              <CardHeader className="pb-2">
                <CardTitle className="text-sm flex items-center gap-2">
                  <Zap className="w-4 h-4" />
                  Pico A (Motion)
                </CardTitle>
              </CardHeader>
              <CardContent>
                <div className="text-xl font-bold">{picoAStatus}</div>
                <p className="text-xs text-muted-foreground">
                  5× TMC2209 Steppers
                </p>
              </CardContent>
            </Card>

            <Card className="relative overflow-hidden">
              <div className="absolute top-0 left-0 right-0 h-0.5 bg-gradient-to-r from-green-500 to-green-600" />
              <CardHeader className="pb-2">
                <CardTitle className="text-sm flex items-center gap-2">
                  <Settings className="w-4 h-4" />
                  Pico B (I/O)
                </CardTitle>
              </CardHeader>
              <CardContent>
                <div className="text-xl font-bold">{picoBStatus}</div>
                <p className="text-xs text-muted-foreground">
                  Servos + Sensors
                </p>
              </CardContent>
            </Card>

            <Card className="relative overflow-hidden">
              <div className="absolute top-0 left-0 right-0 h-0.5 bg-gradient-to-r from-purple-500 to-purple-600" />
              <CardHeader className="pb-2">
                <CardTitle className="text-sm flex items-center gap-2">
                  <Gauge className="w-4 h-4" />
                  Environment
                </CardTitle>
              </CardHeader>
              <CardContent>
                <div className="flex items-center justify-between">
                  <div>
                    <div className="text-lg font-bold">{temperature}°C</div>
                    <div className="text-xs text-muted-foreground">Fan: {fanSpeed}%</div>
                  </div>
                  <Fan className={`w-6 h-6 text-muted-foreground ${fanSpeed > 50 ? 'animate-spin' : ''}`} />
                </div>
              </CardContent>
            </Card>
          </div>

          <div className="grid gap-6 xl:grid-cols-3">
            <div className="xl:col-span-2 space-y-6">
              {/* Track Selection */}
              <TrackSelector />
              
              {/* Job Configuration */}
              <Card>
                <CardHeader>
                  <CardTitle className="flex items-center gap-2">
                    <Settings className="w-5 h-5" />
                    Job Configuration
                  </CardTitle>
                  <CardDescription>
                    Configure wire processing parameters
                  </CardDescription>
                </CardHeader>
                <CardContent className="space-y-4">
                  <div className="grid grid-cols-3 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="wireLength">Wire Length (mm)</Label>
                      <Input
                        id="wireLength"
                        type="number"
                        value={jobConfig.wireLength}
                        onChange={(e) => setJobConfig(prev => ({ ...prev, wireLength: Number(e.target.value) }))}
                        min={1}
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="quantity">Quantity</Label>
                      <Input
                        id="quantity"
                        type="number"
                        value={jobConfig.quantity}
                        onChange={(e) => setJobConfig(prev => ({ ...prev, quantity: Number(e.target.value) }))}
                        min={1}
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="stripLength">Strip Length (mm)</Label>
                      <Input
                        id="stripLength"
                        type="number"
                        value={jobConfig.stripLength}
                        onChange={(e) => setJobConfig(prev => ({ ...prev, stripLength: Number(e.target.value) }))}
                        min={0}
                      />
                    </div>
                  </div>
                  
                  <div className="grid grid-cols-3 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="feedSpeed">Feed Speed (mm/min)</Label>
                      <Input
                        id="feedSpeed"
                        type="number"
                        value={jobConfig.feedSpeed}
                        onChange={(e) => setJobConfig(prev => ({ ...prev, feedSpeed: Number(e.target.value) }))}
                        min={1}
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="acceleration">Acceleration (mm/s²)</Label>
                      <Input
                        id="acceleration"
                        type="number"
                        value={jobConfig.acceleration}
                        onChange={(e) => setJobConfig(prev => ({ ...prev, acceleration: Number(e.target.value) }))}
                        min={1}
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="motionProfile">Motion Profile</Label>
                      <Select 
                        value={jobConfig.motionProfile} 
                        onValueChange={(value) => setJobConfig(prev => ({ ...prev, motionProfile: value }))}
                      >
                        <SelectTrigger>
                          <SelectValue />
                        </SelectTrigger>
                        <SelectContent>
                          <SelectItem value="standard">Standard</SelectItem>
                          <SelectItem value="precision">Precision</SelectItem>
                          <SelectItem value="fast">Fast</SelectItem>
                          <SelectItem value="custom">Custom</SelectItem>
                        </SelectContent>
                      </Select>
                    </div>
                  </div>

                  <div className="space-y-2">
                    <Label htmlFor="notes">Job Notes</Label>
                    <Textarea
                      id="notes"
                      value={jobConfig.notes}
                      onChange={(e) => setJobConfig(prev => ({ ...prev, notes: e.target.value }))}
                      placeholder="Add job notes, special instructions, or customer information..."
                      rows={2}
                    />
                  </div>

                  <div className="flex gap-2 pt-4">
                    <Button 
                      onClick={handleStartJob} 
                      disabled={machineStatus !== 'IDLE' || !jobConfig.selectedTrack}
                      className="flex-1"
                    >
                      <Play className="w-4 h-4 mr-2" />
                      Start Production
                    </Button>
                    <Button 
                      variant="outline" 
                      disabled={machineStatus !== 'IDLE' || !jobConfig.selectedTrack}
                    >
                      <BarChart3 className="w-4 h-4 mr-2" />
                      Test Run
                    </Button>
                    <Button 
                      variant="outline"
                      onClick={handleHomeAxes}
                      disabled={machineStatus !== 'IDLE'}
                    >
                      <Home className="w-4 h-4 mr-2" />
                      Home
                    </Button>
                  </div>
                </CardContent>
              </Card>
            </div>

            <div className="space-y-6">
              {/* Workflow Progress */}
              <WorkflowProgress />
              
              {/* Production Stats */}
              <Card>
                <CardHeader>
                  <CardTitle className="flex items-center gap-2">
                    <Activity className="w-5 h-5" />
                    Production Status
                  </CardTitle>
                </CardHeader>
                <CardContent className="space-y-4">
                  <div>
                    <div className="flex justify-between items-center mb-2">
                      <span className="text-sm font-medium">Job Progress</span>
                      <span className="text-sm text-muted-foreground">
                        {completedPieces} / {totalPieces} pieces
                      </span>
                    </div>
                    <Progress value={(completedPieces / totalPieces) * 100} className="h-2" />
                    <div className="text-xs text-muted-foreground mt-1">
                      {((completedPieces / totalPieces) * 100).toFixed(1)}% Complete
                    </div>
                  </div>

                  <div className="grid grid-cols-2 gap-4 text-sm">
                    <div>
                      <div className="text-muted-foreground">Wire Length</div>
                      <div className="font-semibold">{jobConfig.wireLength}mm</div>
                    </div>
                    <div>
                      <div className="text-muted-foreground">Strip Length</div>
                      <div className="font-semibold">{jobConfig.stripLength}mm</div>
                    </div>
                    <div>
                      <div className="text-muted-foreground">Feed Speed</div>
                      <div className="font-semibold">{jobConfig.feedSpeed}mm/min</div>
                    </div>
                    <div>
                      <div className="text-muted-foreground">Estimated Time</div>
                      <div className="font-semibold">42m</div>
                    </div>
                  </div>

                  <div className="space-y-2">
                    <h4 className="text-sm font-medium">Emergency Controls</h4>
                    <div className="grid grid-cols-1 gap-2">
                      <Button
                        variant="outline"
                        size="sm"
                        disabled={machineStatus !== 'RUNNING'}
                      >
                        <Pause className="w-4 h-4 mr-2" />
                        Pause Job
                      </Button>
                      <Button
                        variant="destructive"
                        size="sm"
                        onClick={handleStopJob}
                        disabled={machineStatus === 'IDLE'}
                      >
                        <Square className="w-4 h-4 mr-2" />
                        Emergency Stop
                      </Button>
                    </div>
                  </div>
                </CardContent>
              </Card>
            </div>
          </div>

          {/* Activity Log */}
          <Card>
            <CardHeader>
              <CardTitle className="flex items-center gap-2">
                <Activity className="w-5 h-5" />
                System Activity Log
              </CardTitle>
              <CardDescription>
                Real-time system events, MMU operations, and job progress
              </CardDescription>
            </CardHeader>
            <CardContent>
              <div className="bg-black/90 rounded-lg p-4 max-h-64 overflow-y-auto font-mono text-sm border border-gray-800">
                <div className="space-y-1">
                  {logs.map((log, i) => (
                    <div key={i} className="flex items-start gap-2">
                      <span className="text-gray-500 text-xs w-20 shrink-0 mt-0.5">
                        {log.time}
                      </span>
                      <span className={`flex-1 ${
                        log.type === 'success' ? 'text-green-400' : 
                        log.type === 'warning' ? 'text-yellow-400' : 
                        log.type === 'error' ? 'text-red-400' : 
                        'text-blue-400'
                      }`}>
                        {log.message}
                      </span>
                    </div>
                  ))}
                </div>
              </div>
            </CardContent>
          </Card>
        </main>
      </SidebarInset>
    </SidebarProvider>
  );
}