import fs from 'node:fs'
import path from 'node:path'
import { runBuild } from './lib/build.mts'
import { outputHl, gameDir, discoverMods } from './lib/paths.mts'

// `npm run deploy` -> all mods, `npm run deploy -- <mod-name>` -> just that one.
const requested = process.argv[2]
const mods = requested ? [requested] : discoverMods()

if (mods.length === 0) throw new Error('No mods found (looked for compile.hxml in top-level folders).')

const dest = gameDir()
if (!fs.existsSync(dest)) {
  throw new Error(
    `Game folder not found: ${dest}\nCheck hlx-core/.tools/user-config.json, or re-run: pnpm run setup (in hlx-core)`,
  )
}

// hlx-core's own `pnpm deploy` is what normally creates hlx/mods/ (alongside
// hlx/loader/ and hlx/logs/) - mkdir -p here anyway rather than requiring
// that to have already run, since a missing mods/ folder is easy to recover
// from and there's no reason to hard-fail over ordering.
const modsDir = path.join(dest, 'hlx', 'mods')

for (const mod of mods) {
  runBuild(mod)
  const output = outputHl(mod)
  // hlx-loader expects each mod in its own hlx/mods/<name>/<name>.hl subfolder
  // (room for resources alongside the .hl without colliding with other mods).
  const modDir = path.join(modsDir, mod)
  fs.mkdirSync(modDir, { recursive: true })

  const target = path.join(modDir, path.basename(output))
  fs.copyFileSync(output, target)
  console.log(`${mod}.hl -> ${target}`)
}

console.log('\nDeployed. Restart the game to have hlx-loader pick it up from hlx/mods/.')
