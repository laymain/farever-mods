import { runBuild } from './lib/build.mts'
import { runZip } from './lib/zip.mts'
import { discoverMods } from './lib/paths.mts'

// `npm run build` -> all mods, `npm run build -- <mod-name>` -> just that one.
const requested = process.argv[2]
const mods = requested ? [requested] : discoverMods()

if (mods.length === 0) throw new Error('No mods found (looked for compile.hxml in top-level folders).')

for (const mod of mods) {
  runBuild(mod)
  await runZip(mod)
}
console.log(`\nBuild complete (${mods.join(', ')}).`)
