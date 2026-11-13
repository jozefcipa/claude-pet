// Sums Claude Code token usage from local transcripts and pushes it to the pet.
// Usage: node usage.ts [--dry]
import { lookup } from 'node:dns/promises'
import { readdirSync, readFileSync, statSync } from 'node:fs'
import { homedir } from 'node:os'
import { join } from 'node:path'

const HOST = process.env.CLAUDE_PET_HOST ?? 'claude-pet.local'
const DAYS = 7
const root = join(homedir(), '.claude', 'projects')

const dayKey = (d: Date) => d.toLocaleDateString('sv') // YYYY-MM-DD, local time
const now = new Date()
const days = Array.from({ length: DAYS }, (_, i) => new Date(now.getFullYear(), now.getMonth(), now.getDate() - (DAYS - 1 - i)))
const keys = days.map(dayKey)
const today = keys.at(-1)!
const cutoff = days[0].getTime()

const tokens = new Map<string, number>(keys.map(k => [k, 0]))
const sessionsToday = new Set<string>()
const seen = new Set<string>() // a response is logged once per content block, count it once

for (const rel of readdirSync(root, { recursive: true }) as string[]) {
  if (!rel.endsWith('.jsonl')) {
    continue
  }
  const file = join(root, rel)
  if (statSync(file).mtimeMs < cutoff) {
    continue
  }
  for (const line of readFileSync(file, 'utf8').split('\n')) {
    if (!line.includes('"usage"')) {
      continue
    }
    let e
    try {
      e = JSON.parse(line)
    } catch {
      continue
    }
    const u = e.message?.usage
    if (!u || !e.timestamp) {
      continue
    }
    const id = `${e.message.id}:${e.requestId}`
    if (seen.has(id)) {
      continue
    }
    seen.add(id)
    const k = dayKey(new Date(e.timestamp))
    if (!tokens.has(k)) {
      continue
    }
    const n = (u.input_tokens ?? 0) + (u.output_tokens ?? 0) + (u.cache_creation_input_tokens ?? 0) + (u.cache_read_input_tokens ?? 0)
    tokens.set(k, tokens.get(k)! + n)
    if (k === today) {
      sessionsToday.add(e.sessionId)
    }
  }
}

const params = new URLSearchParams({
  today: String(tokens.get(today)),
  sessions: String(sessionsToday.size),
  week: keys.map(k => tokens.get(k)).join(','),
})

if (process.argv.includes('--dry')) {
  console.log(Object.fromEntries(params))
} else {
  // IPv4 only: the ESP never answers the AAAA query for .local, which stalls a normal lookup ~5s
  const { address } = await lookup(HOST, { family: 4 })
  await fetch(`http://${address}/usage?${params}`, { signal: AbortSignal.timeout(3000) }).catch(() => {})
}
