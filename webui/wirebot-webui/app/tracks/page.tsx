'use client'

import React, { useState } from 'react';
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
import { Textarea } from "@/components/ui/textarea"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from "@/components/ui/dialog"
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table"
import { 
  Plus, 
  Edit2, 
  Trash2, 
  Cable,
  Layers3,
  Settings,
  CheckCircle,
  AlertTriangle,
  RotateCcw,
  Save,
  X,
  Target,
  Gauge,
  Palette
} from 'lucide-react';

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
  position: number;
  lastUsed: string;
  notes?: string;
  supplier?: string;
  partNumber?: string;
  costPerMeter?: number;
}

interface WireSpec {
  gauge: string;
  type: string;
  material: string;
}

export default function TrackManagementPage() {
  const [wireTracks, setWireTracks] = useState<WireTrack[]>([
    {
      id: 1,
      name: "Red 18AWG Stranded",
      wireType: "Stranded",
      gauge: "18 AWG",
      color: "#ef4444",
      material: "Copper",
      stockLength: 1000,
      remainingLength: 847,
      isActive: true,
      position: 1,
      lastUsed: "2024-12-26 13:45",
      notes: "Primary power wire for automotive applications",
      supplier: "Alpha Wire",
      partNumber: "3051 RD005",
      costPerMeter: 0.85
    },
    {
      id: 2,
      name: "Black 18AWG Stranded", 
      wireType: "Stranded",
      gauge: "18 AWG",
      color: "#1f2937",
      material: "Copper",
      stockLength: 1000,
      remainingLength: 692,
      isActive: true,
      position: 2,
      lastUsed: "2024-12-26 14:12",
      notes: "Ground wire for automotive applications",
      supplier: "Alpha Wire",
      partNumber: "3051 BK005",
      costPerMeter: 0.85
    },
    {
      id: 3,
      name: "Blue 22AWG Solid",
      wireType: "Solid", 
      gauge: "22 AWG",
      color: "#3b82f6",
      material: "Copper",
      stockLength: 500,
      remainingLength: 234,
      isActive: true,
      position: 3,
      lastUsed: "2024-12-25 16:30",
      notes: "Signal wire for low current applications",
      supplier: "Belden",
      partNumber: "8022-22-1000",
      costPerMeter: 0.45
    },
    {
      id: 4,
      name: "Green 20AWG Stranded",
      wireType: "Stranded",
      gauge: "20 AWG", 
      color: "#10b981",
      material: "Copper",
      stockLength: 750,
      remainingLength: 0,
      isActive: false,
      position: 4,
      lastUsed: "2024-12-24 09:15",
      notes: "Control wire - needs restocking",
      supplier: "Southwire",
      partNumber: "THHN-20-GRN",
      costPerMeter: 0.65
    },
    {
      id: 5,
      name: "White 16AWG Stranded",
      wireType: "Stranded",
      gauge: "16 AWG",
      color: "#f3f4f6", 
      material: "Copper",
      stockLength: 1500,
      remainingLength: 1340,
      isActive: true,
      position: 5,
      lastUsed: "2024-12-26 10:22",
      notes: "Heavy duty power wire",
      supplier: "Remington Industries",
      partNumber: "16UL1007STRWHT",
      costPerMeter: 1.20
    }
  ]);

  const [editingTrack, setEditingTrack] = useState<WireTrack | null>(null);
  const [showAddDialog, setShowAddDialog] = useState(false);
  const [showEditDialog, setShowEditDialog] = useState(false);

  const wireGauges = ["10 AWG", "12 AWG", "14 AWG", "16 AWG", "18 AWG", "20 AWG", "22 AWG", "24 AWG", "26 AWG", "28 AWG", "30 AWG"];
  const wireTypes = ["Stranded", "Solid", "Tinned Stranded", "Litz"];
  const wireMaterials = ["Copper", "Aluminum", "Silver-Plated Copper", "Tinned Copper"];
  const wireColors = [
    { name: "Red", value: "#ef4444" },
    { name: "Black", value: "#1f2937" },
    { name: "Blue", value: "#3b82f6" },
    { name: "Green", value: "#10b981" },
    { name: "White", value: "#f3f4f6" },
    { name: "Yellow", value: "#eab308" },
    { name: "Orange", value: "#f97316" },
    { name: "Purple", value: "#a855f7" },
    { name: "Gray", value: "#6b7280" },
    { name: "Brown", value: "#a16207" }
  ];

  const [newTrack, setNewTrack] = useState<Partial<WireTrack>>({
    name: "",
    wireType: "Stranded",
    gauge: "18 AWG",
    color: "#ef4444",
    material: "Copper",
    stockLength: 1000,
    remainingLength: 1000,
    isActive: true,
    position: 6,
    notes: "",
    supplier: "",
    partNumber: "",
    costPerMeter: 0
  });

  const getStockStatus = (track: WireTrack) => {
    const percentage = (track.remainingLength / track.stockLength) * 100;
    if (percentage === 0) return { status: 'empty', color: 'destructive' };
    if (percentage < 20) return { status: 'low', color: 'destructive' };
    if (percentage < 50) return { status: 'medium', color: 'warning' };
    return { status: 'good', color: 'success' };
  };

  const handleAddTrack = () => {
    if (!newTrack.name) return;
    
    const track: WireTrack = {
      id: Math.max(...wireTracks.map(t => t.id)) + 1,
      name: newTrack.name,
      wireType: newTrack.wireType || "Stranded",
      gauge: newTrack.gauge || "18 AWG",
      color: newTrack.color || "#ef4444",
      material: newTrack.material || "Copper",
      stockLength: newTrack.stockLength || 1000,
      remainingLength: newTrack.remainingLength || 1000,
      isActive: newTrack.isActive ?? true,
      position: newTrack.position || wireTracks.length + 1,
      lastUsed: new Date().toISOString().slice(0, 16),
      notes: newTrack.notes || "",
      supplier: newTrack.supplier || "",
      partNumber: newTrack.partNumber || "",
      costPerMeter: newTrack.costPerMeter || 0
    };

    setWireTracks([...wireTracks, track]);
    setNewTrack({
      name: "",
      wireType: "Stranded",
      gauge: "18 AWG",
      color: "#ef4444",
      material: "Copper",
      stockLength: 1000,
      remainingLength: 1000,
      isActive: true,
      position: wireTracks.length + 2,
      notes: "",
      supplier: "",
      partNumber: "",
      costPerMeter: 0
    });
    setShowAddDialog(false);
  };

  const handleEditTrack = () => {
    if (!editingTrack) return;
    
    setWireTracks(prev => 
      prev.map(track => 
        track.id === editingTrack.id ? editingTrack : track
      )
    );
    setEditingTrack(null);
    setShowEditDialog(false);
  };

  const handleDeleteTrack = (id: number) => {
    setWireTracks(prev => prev.filter(track => track.id !== id));
  };

  const handleRestockTrack = (id: number) => {
    setWireTracks(prev => 
      prev.map(track => 
        track.id === id 
          ? { ...track, remainingLength: track.stockLength, isActive: true }
          : track
      )
    );
  };

  const calibratePosition = (trackId: number) => {
    // This would trigger MMU calibration sequence
    console.log(`Calibrating position for track ${trackId}`);
  };

  return (
    <SidebarProvider>
      <AppSidebar />
      <SidebarInset>
        <header className="flex h-16 shrink-0 items-center gap-2 px-4 border-b">
          <SidebarTrigger className="-ml-1" />
          <Separator orientation="vertical" className="h-4" />
          <div className="flex items-center gap-4 flex-1">
            <div className="flex items-center gap-2">
              <Layers3 className="w-5 h-5 text-orange-500" />
              <h1 className="text-lg font-semibold">Wire Track Management</h1>
            </div>
            <Dialog open={showAddDialog} onOpenChange={setShowAddDialog}>
              <DialogTrigger asChild>
                <Button className="ml-auto">
                  <Plus className="w-4 h-4 mr-2" />
                  Add Track
                </Button>
              </DialogTrigger>
              <DialogContent className="max-w-2xl">
                <DialogHeader>
                  <DialogTitle>Add New Wire Track</DialogTitle>
                  <DialogDescription>
                    Configure a new wire track for the MMU system
                  </DialogDescription>
                </DialogHeader>
                
                <div className="grid gap-4">
                  <div className="grid grid-cols-2 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="trackName">Track Name</Label>
                      <Input
                        id="trackName"
                        value={newTrack.name}
                        onChange={(e) => setNewTrack(prev => ({ ...prev, name: e.target.value }))}
                        placeholder="e.g., Red 18AWG Stranded"
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="position">MMU Position</Label>
                      <Input
                        id="position"
                        type="number"
                        value={newTrack.position}
                        onChange={(e) => setNewTrack(prev => ({ ...prev, position: Number(e.target.value) }))}
                        min={1}
                        max={8}
                      />
                    </div>
                  </div>
                  
                  <div className="grid grid-cols-3 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="gauge">Wire Gauge</Label>
                      <Select 
                        value={newTrack.gauge} 
                        onValueChange={(value) => setNewTrack(prev => ({ ...prev, gauge: value }))}
                      >
                        <SelectTrigger>
                          <SelectValue />
                        </SelectTrigger>
                        <SelectContent>
                          {wireGauges.map(gauge => (
                            <SelectItem key={gauge} value={gauge}>{gauge}</SelectItem>
                          ))}
                        </SelectContent>
                      </Select>
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="wireType">Wire Type</Label>
                      <Select 
                        value={newTrack.wireType} 
                        onValueChange={(value) => setNewTrack(prev => ({ ...prev, wireType: value }))}
                      >
                        <SelectTrigger>
                          <SelectValue />
                        </SelectTrigger>
                        <SelectContent>
                          {wireTypes.map(type => (
                            <SelectItem key={type} value={type}>{type}</SelectItem>
                          ))}
                        </SelectContent>
                      </Select>
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="material">Material</Label>
                      <Select 
                        value={newTrack.material} 
                        onValueChange={(value) => setNewTrack(prev => ({ ...prev, material: value }))}
                      >
                        <SelectTrigger>
                          <SelectValue />
                        </SelectTrigger>
                        <SelectContent>
                          {wireMaterials.map(material => (
                            <SelectItem key={material} value={material}>{material}</SelectItem>
                          ))}
                        </SelectContent>
                      </Select>
                    </div>
                  </div>
                  
                  <div className="space-y-2">
                    <Label>Wire Color</Label>
                    <div className="flex gap-2 flex-wrap">
                      {wireColors.map(colorOption => (
                        <button
                          key={colorOption.value}
                          type="button"
                          className={`w-8 h-8 rounded border-2 ${
                            newTrack.color === colorOption.value 
                              ? 'border-orange-500 ring-2 ring-orange-200' 
                              : 'border-gray-300'
                          }`}
                          style={{ backgroundColor: colorOption.value }}
                          onClick={() => setNewTrack(prev => ({ ...prev, color: colorOption.value }))}
                          title={colorOption.name}
                        />
                      ))}
                    </div>
                  </div>
                  
                  <div className="grid grid-cols-2 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="stockLength">Stock Length (m)</Label>
                      <Input
                        id="stockLength"
                        type="number"
                        value={newTrack.stockLength}
                        onChange={(e) => setNewTrack(prev => ({ ...prev, stockLength: Number(e.target.value) }))}
                        min={1}
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="remainingLength">Remaining Length (m)</Label>
                      <Input
                        id="remainingLength"
                        type="number"
                        value={newTrack.remainingLength}
                        onChange={(e) => setNewTrack(prev => ({ ...prev, remainingLength: Number(e.target.value) }))}
                        min={0}
                      />
                    </div>
                  </div>
                  
                  <div className="grid grid-cols-3 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="supplier">Supplier</Label>
                      <Input
                        id="supplier"
                        value={newTrack.supplier}
                        onChange={(e) => setNewTrack(prev => ({ ...prev, supplier: e.target.value }))}
                        placeholder="e.g., Alpha Wire"
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="partNumber">Part Number</Label>
                      <Input
                        id="partNumber"
                        value={newTrack.partNumber}
                        onChange={(e) => setNewTrack(prev => ({ ...prev, partNumber: e.target.value }))}
                        placeholder="e.g., 3051 RD005"
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="costPerMeter">Cost per Meter ($)</Label>
                      <Input
                        id="costPerMeter"
                        type="number"
                        step="0.01"
                        value={newTrack.costPerMeter}
                        onChange={(e) => setNewTrack(prev => ({ ...prev, costPerMeter: Number(e.target.value) }))}
                        min={0}
                      />
                    </div>
                  </div>
                  
                  <div className="space-y-2">
                    <Label htmlFor="notes">Notes</Label>
                    <Textarea
                      id="notes"
                      value={newTrack.notes}
                      onChange={(e) => setNewTrack(prev => ({ ...prev, notes: e.target.value }))}
                      placeholder="Add any notes about this wire track..."
                      rows={3}
                    />
                  </div>
                </div>
                
                <DialogFooter>
                  <Button variant="outline" onClick={() => setShowAddDialog(false)}>
                    Cancel
                  </Button>
                  <Button onClick={handleAddTrack} disabled={!newTrack.name}>
                    <Save className="w-4 h-4 mr-2" />
                    Add Track
                  </Button>
                </DialogFooter>
              </DialogContent>
            </Dialog>
          </div>
        </header>

        <main className="flex flex-1 flex-col gap-6 p-6">
          {/* Track Overview */}
          <div className="grid auto-rows-min gap-4 md:grid-cols-4">
            <Card>
              <CardHeader className="pb-2">
                <CardTitle className="text-sm">Total Tracks</CardTitle>
              </CardHeader>
              <CardContent>
                <div className="text-2xl font-bold">{wireTracks.length}</div>
                <p className="text-xs text-muted-foreground">
                  Configured
                </p>
              </CardContent>
            </Card>
            
            <Card>
              <CardHeader className="pb-2">
                <CardTitle className="text-sm">Active Tracks</CardTitle>
              </CardHeader>
              <CardContent>
                <div className="text-2xl font-bold text-green-600">
                  {wireTracks.filter(t => t.isActive).length}
                </div>
                <p className="text-xs text-muted-foreground">
                  Ready to use
                </p>
              </CardContent>
            </Card>
            
            <Card>
              <CardHeader className="pb-2">
                <CardTitle className="text-sm">Low Stock</CardTitle>
              </CardHeader>
              <CardContent>
                <div className="text-2xl font-bold text-yellow-600">
                  {wireTracks.filter(t => (t.remainingLength / t.stockLength) < 0.2 && t.remainingLength > 0).length}
                </div>
                <p className="text-xs text-muted-foreground">
                  Need restocking
                </p>
              </CardContent>
            </Card>
            
            <Card>
              <CardHeader className="pb-2">
                <CardTitle className="text-sm">Empty Tracks</CardTitle>
              </CardHeader>
              <CardContent>
                <div className="text-2xl font-bold text-red-600">
                  {wireTracks.filter(t => t.remainingLength === 0).length}
                </div>
                <p className="text-xs text-muted-foreground">
                  Out of stock
                </p>
              </CardContent>
            </Card>
          </div>

          {/* Track Table */}
          <Card>
            <CardHeader>
              <CardTitle>Wire Track Configuration</CardTitle>
              <CardDescription>
                Manage wire tracks, positions, and inventory levels
              </CardDescription>
            </CardHeader>
            <CardContent>
              <Table>
                <TableHeader>
                  <TableRow>
                    <TableHead className="w-[60px]">Pos</TableHead>
                    <TableHead>Track Name</TableHead>
                    <TableHead>Specifications</TableHead>
                    <TableHead>Stock Level</TableHead>
                    <TableHead>Supplier Info</TableHead>
                    <TableHead>Status</TableHead>
                    <TableHead className="text-right">Actions</TableHead>
                  </TableRow>
                </TableHeader>
                <TableBody>
                  {wireTracks.map((track) => {
                    const stockStatus = getStockStatus(track);
                    return (
                      <TableRow key={track.id}>
                        <TableCell className="font-bold">
                          <Badge variant="outline">T{track.position}</Badge>
                        </TableCell>
                        <TableCell>
                          <div className="flex items-center gap-2">
                            <div 
                              className="w-4 h-4 rounded border"
                              style={{ backgroundColor: track.color }}
                            />
                            <div>
                              <div className="font-medium">{track.name}</div>
                              <div className="text-sm text-muted-foreground">
                                Last used: {new Date(track.lastUsed).toLocaleString()}
                              </div>
                            </div>
                          </div>
                        </TableCell>
                        <TableCell>
                          <div className="text-sm">
                            <div className="font-medium">{track.gauge} {track.wireType}</div>
                            <div className="text-muted-foreground">{track.material}</div>
                          </div>
                        </TableCell>
                        <TableCell>
                          <div className="space-y-2">
                            <div className="flex justify-between text-sm">
                              <span>{track.remainingLength}m</span>
                              <span className="text-muted-foreground">/ {track.stockLength}m</span>
                            </div>
                            <Progress 
                              value={(track.remainingLength / track.stockLength) * 100} 
                              className="h-2"
                            />
                            <div className="text-xs text-muted-foreground">
                              {Math.round((track.remainingLength / track.stockLength) * 100)}% remaining
                            </div>
                          </div>
                        </TableCell>
                        <TableCell>
                          <div className="text-sm">
                            <div className="font-medium">{track.supplier || 'N/A'}</div>
                            <div className="text-muted-foreground">{track.partNumber || 'N/A'}</div>
                            {track.costPerMeter && (
                              <div className="text-xs text-green-600">${track.costPerMeter.toFixed(2)}/m</div>
                            )}
                          </div>
                        </TableCell>
                        <TableCell>
                          <Badge variant={stockStatus.color}>
                            {stockStatus.status === 'empty' && <AlertTriangle className="w-3 h-3 mr-1" />}
                            {stockStatus.status === 'good' && <CheckCircle className="w-3 h-3 mr-1" />}
                            {track.isActive ? 
                              (stockStatus.status === 'empty' ? 'Empty' : 
                               stockStatus.status === 'low' ? 'Low Stock' : 
                               'Active') 
                              : 'Inactive'}
                          </Badge>
                        </TableCell>
                        <TableCell>
                          <div className="flex items-center gap-1 justify-end">
                            <Button
                              variant="ghost"
                              size="sm"
                              onClick={() => calibratePosition(track.id)}
                              title="Calibrate MMU Position"
                            >
                              <Target className="w-4 h-4" />
                            </Button>
                            <Button
                              variant="ghost"
                              size="sm"
                              onClick={() => {
                                setEditingTrack(track);
                                setShowEditDialog(true);
                              }}
                            >
                              <Edit2 className="w-4 h-4" />
                            </Button>
                            {track.remainingLength === 0 && (
                              <Button
                                variant="ghost"
                                size="sm"
                                onClick={() => handleRestockTrack(track.id)}
                                title="Restock Track"
                              >
                                <RotateCcw className="w-4 h-4" />
                              </Button>
                            )}
                            <Button
                              variant="ghost"
                              size="sm"
                              onClick={() => handleDeleteTrack(track.id)}
                            >
                              <Trash2 className="w-4 h-4" />
                            </Button>
                          </div>
                        </TableCell>
                      </TableRow>
                    );
                  })}
                </TableBody>
              </Table>
            </CardContent>
          </Card>

          {/* Edit Dialog */}
          <Dialog open={showEditDialog} onOpenChange={setShowEditDialog}>
            <DialogContent className="max-w-2xl">
              <DialogHeader>
                <DialogTitle>Edit Wire Track</DialogTitle>
                <DialogDescription>
                  Modify track configuration and parameters
                </DialogDescription>
              </DialogHeader>
              
              {editingTrack && (
                <div className="grid gap-4">
                  <div className="grid grid-cols-2 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="editTrackName">Track Name</Label>
                      <Input
                        id="editTrackName"
                        value={editingTrack.name}
                        onChange={(e) => setEditingTrack(prev => prev ? { ...prev, name: e.target.value } : null)}
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="editPosition">MMU Position</Label>
                      <Input
                        id="editPosition"
                        type="number"
                        value={editingTrack.position}
                        onChange={(e) => setEditingTrack(prev => prev ? { ...prev, position: Number(e.target.value) } : null)}
                        min={1}
                        max={8}
                      />
                    </div>
                  </div>
                  
                  <div className="grid grid-cols-2 gap-4">
                    <div className="space-y-2">
                      <Label htmlFor="editStockLength">Stock Length (m)</Label>
                      <Input
                        id="editStockLength"
                        type="number"
                        value={editingTrack.stockLength}
                        onChange={(e) => setEditingTrack(prev => prev ? { ...prev, stockLength: Number(e.target.value) } : null)}
                        min={1}
                      />
                    </div>
                    <div className="space-y-2">
                      <Label htmlFor="editRemainingLength">Remaining Length (m)</Label>
                      <Input
                        id="editRemainingLength"
                        type="number"
                        value={editingTrack.remainingLength}
                        onChange={(e) => setEditingTrack(prev => prev ? { ...prev, remainingLength: Number(e.target.value) } : null)}
                        min={0}
                      />
                    </div>
                  </div>
                  
                  <div className="space-y-2">
                    <Label htmlFor="editNotes">Notes</Label>
                    <Textarea
                      id="editNotes"
                      value={editingTrack.notes || ''}
                      onChange={(e) => setEditingTrack(prev => prev ? { ...prev, notes: e.target.value } : null)}
                      rows={3}
                    />
                  </div>
                </div>
              )}
              
              <DialogFooter>
                <Button variant="outline" onClick={() => setShowEditDialog(false)}>
                  Cancel
                </Button>
                <Button onClick={handleEditTrack}>
                  <Save className="w-4 h-4 mr-2" />
                  Save Changes
                </Button>
              </DialogFooter>
            </DialogContent>
          </Dialog>
        </main>
      </SidebarInset>
    </SidebarProvider>
  );
}