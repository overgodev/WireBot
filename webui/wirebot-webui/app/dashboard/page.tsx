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
        {/* Top bar */}
        <header className="flex h-16 shrink-0 items-center gap-2 px-4">
          <SidebarTrigger className="-ml-1" />
          <Separator orientation="vertical" className="h-4" />
          <h1 className="text-sm font-medium">Dashboard</h1>
        </header>

        {/* Main content */}
        <main className="flex flex-1 flex-col gap-4 p-4 pt-0">
          <div className="grid auto-rows-min gap-4 md:grid-cols-3">
            <div className="rounded-xl border bg-muted/50 p-4">
              <div className="text-sm font-medium">Controller</div>
              <div className="mt-1 text-xs text-muted-foreground">
                Status: Disconnected
              </div>
            </div>

            <div className="rounded-xl border bg-muted/50 p-4">
              <div className="text-sm font-medium">Pico A</div>
              <div className="mt-1 text-xs text-muted-foreground">
                Not detected
              </div>
            </div>

            <div className="rounded-xl border bg-muted/50 p-4">
              <div className="text-sm font-medium">Pico B</div>
              <div className="mt-1 text-xs text-muted-foreground">
                Not detected
              </div>
            </div>
          </div>

          <div className="flex-1 rounded-xl border bg-muted/50 p-4">
            <div className="text-sm font-medium">Activity</div>
            <div className="mt-2 text-xs text-muted-foreground">
              Waiting for controller connection…
            </div>
          </div>
        </main>
      </SidebarInset>
    </SidebarProvider>
  )
}
