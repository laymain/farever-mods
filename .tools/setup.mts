import readline from 'node:readline'
import { readUserConfig, writeUserConfig } from './lib/paths.mts'

const DEFAULT_HASHLINK_DIR = 'D:\\Projects\\haxe\\hashlink-a94f831-win64'

function readStoredHashlinkDir(): string | null {
  try {
    return readUserConfig().hashlinkDir
  } catch {
    return null
  }
}

async function promptHashlinkDir(): Promise<string> {
  const stored = readStoredHashlinkDir()
  const defaultDir = stored ?? DEFAULT_HASHLINK_DIR
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout })
  return new Promise((resolve) => {
    const label = stored ? `Enter to keep: ${stored}` : `Enter for default: ${DEFAULT_HASHLINK_DIR}`
    rl.question(`HashLink checkout/SDK path, needs include/hl.h + libhl.lib (${label}): `, (answer: string) => {
      rl.close()
      resolve(answer.trim() || defaultDir)
    })
  })
}

console.log('=== farever-mods Setup ===')
const hashlinkDir = await promptHashlinkDir()
writeUserConfig({ hashlinkDir })
console.log(`Wrote .tools/user-config.json (hashlinkDir=${hashlinkDir})`)
console.log('\nSetup complete. Next: npm run build -- <mod-name>')
