"use client"

import * as React from "react"
import { useRouter } from "next/navigation"
import { Button } from "@/components/ui/button"
import { Card, CardContent } from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"

export function WireBotConnectForm() {
  const router = useRouter()

  const [host, setHost] = React.useState("http://wirebot.local:8000")
  const [key, setKey] = React.useState("")

  function onSubmit(e: React.FormEvent) {
    e.preventDefault()

    // Persist for dashboard + future API client usage
    localStorage.setItem("wirebot_host", host.trim())
    localStorage.setItem("wirebot_key", key.trim())

    // Navigate to dashboard
    router.push("/dashboard")
  }

  return (
    <Card className="overflow-hidden p-0">
      <CardContent className="grid p-0 md:grid-cols-2 md:items-stretch">
        {/* LEFT: Connect / Auth */}
        <form className="p-6 md:p-8" onSubmit={onSubmit}>
          <div className="flex h-full min-h-[520px] flex-col">
            {/* Header with logo */}
            <div className="flex flex-col items-center gap-4 text-center">
              <img
                src="/wirebot-logo.svg"
                alt="WireBot"
                className="h-14 w-14"
              />
              <div className="text-3xl font-bold tracking-tight">WireBot</div>
              <p className="text-muted-foreground text-balance">
                Connect to your controller to start.
              </p>
            </div>

            {/* Inputs */}
            <div className="mt-10 grid gap-6">
              <div className="grid gap-2">
                <Label htmlFor="host">Controller Host</Label>
                <Input
                  id="host"
                  value={host}
                  onChange={(e) => setHost(e.target.value)}
                  placeholder="http://wirebot.local:8000"
                  required
                />
                <p className="text-muted-foreground text-xs">
                  Use an IP like{" "}
                  <span className="font-mono">http://192.168.1.50:8000</span>
                </p>
              </div>

              <div className="grid gap-2">
                <Label htmlFor="key">Access Key (optional)</Label>
                <Input
                  id="key"
                  value={key}
                  onChange={(e) => setKey(e.target.value)}
                  placeholder="leave blank if none"
                />
                <p className="text-muted-foreground text-xs">
                  If you enable auth later, paste the key here.
                </p>
              </div>

              <Button type="submit" className="w-full">
                Connect
              </Button>
            </div>
          </div>
        </form>

        {/* RIGHT: Visual / Hero */}
        <div className="relative hidden md:block">
          <div className="absolute inset-0 bg-muted" />
          <div
            className="absolute inset-0 opacity-20
            [background-image:radial-gradient(circle_at_1px_1px,rgba(255,255,255,0.18)_1px,transparent_0)]
            [background-size:24px_24px]"
          />

          <div className="relative h-full w-full">
            <img
              src="/wirebot-hero.png"
              alt="WireBot"
              className="absolute inset-0 h-full w-full object-cover dark:brightness-[0.7] dark:contrast-125"
            />
            <div className="absolute inset-0 bg-gradient-to-t from-background/60 via-background/10 to-transparent" />
          </div>
        </div>
      </CardContent>
    </Card>
  )
}
