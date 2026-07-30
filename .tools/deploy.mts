import fs from 'node:fs'
import path from 'node:path'
import { runBuild } from './lib/build.mts'
import { gameDir, discoverMods, stageModDir, stagePluginsDir } from './lib/paths.mts'

// `npm run deploy` -> all mods, `npm run deploy -- <mod-name>` -> just that one.
const requested = process.argv[2]
const mods = requested ? [requested] : discoverMods()

if (mods.length === 0) throw new Error('No mods found (looked for compile.hxml or native/CMakeLists.txt in top-level folders).')

const dest = gameDir()
if (!fs.existsSync(dest)) {
  throw new Error(
    `Game folder not found: ${dest}\nCheck hlx-core/.tools/user-config.json, or re-run: pnpm run setup (in hlx-core)`,
  )
}

// hlx-core's own `pnpm deploy` is what normally creates hlx/mods/ and
// hlx/plugins/ (alongside hlx/loader/ and hlx/logs/) - mkdir -p here anyway
// rather than requiring that to have already run, since missing folders are
// easy to recover from and there's no reason to hard-fail over ordering.
const modsDir = path.join(dest, 'hlx', 'mods')
const pluginsDir = path.join(dest, 'hlx', 'plugins')

for (const mod of mods) {
  await runBuild(mod)

  // A native-only mod (no compile.hxml, e.g. shader-persistent-cache) never gets a stageModDir
  // at all - it has no .hl to stage, so there's nothing to put under hlx/mods/ for it either.
  const stagedMod = stageModDir(mod)
  if (fs.existsSync(stagedMod)) {
    const modDest = path.join(modsDir, mod)
    fs.mkdirSync(modDest, { recursive: true })
    for (const file of fs.readdirSync(stagedMod)) {
      fs.copyFileSync(path.join(stagedMod, file), path.join(modDest, file))
    }
    console.log(`${mod}.hl -> ${modDest}`)
  }

  // Each mod gets its own plugins/<mod-name>/ subfolder (matches hlx-boot's own
  // EagerLoadPluginHdlls scan, hlx-core/hlx-boot/src/boot.c) rather than dumping every mod's
  // .hdll files into one shared flat folder.
  const stagedPlugins = stagePluginsDir(mod)
  if (fs.existsSync(stagedPlugins)) {
    const modPluginsDest = path.join(pluginsDir, mod)
    fs.mkdirSync(modPluginsDest, { recursive: true })
    for (const file of fs.readdirSync(stagedPlugins)) {
      const target = path.join(modPluginsDest, file)
      fs.copyFileSync(path.join(stagedPlugins, file), target)
      console.log(`${file} -> ${target}`)
    }
  }
}

console.log('\nDeployed. Restart the game to have hlx-loader pick it up from hlx/mods/ and hlx/plugins/.')
