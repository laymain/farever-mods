import { execSync } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'
import { modRoot, outputHl } from './paths.mts'

export function runBuild(modName: string): void {
  const output = outputHl(modName)
  console.log(`Building ${modName}.hl...`)
  fs.mkdirSync(path.dirname(output), { recursive: true })
  execSync('haxe compile.hxml', { cwd: modRoot(modName), stdio: 'inherit' })

  if (!fs.existsSync(output)) throw new Error(`Build did not produce ${output}`)
  console.log(`${modName}.hl -> built`)
}
