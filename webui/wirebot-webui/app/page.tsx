import { WireBotConnectForm } from "@/components/wirebot-connect-form"

export default function Page() {
  return (
    <div className="bg-muted flex min-h-svh flex-col items-center justify-center p-6 md:p-10">
      <div className="w-full max-w-sm md:max-w-4xl">
        <WireBotConnectForm />
      </div>

      <p className="mt-6 px-6 text-center text-xs text-muted-foreground">
        By connecting, you agree to your local safety rules. Always keep an E-Stop within reach.
      </p>
    </div>
  )
}
