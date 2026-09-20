import { execSync } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'
import { modRoot, outputHl, stageModDir, stagePluginsDir, stageDriverDir, hashlinkDir, modType, ModType } from './paths.mts'

function buildHaxeSide(modName: string): void {
  const output = outputHl(modName)
  console.log(`Building ${modName}.hl...`)
  fs.mkdirSync(path.dirname(output), { recursive: true })
  execSync('haxe compile.hxml', { cwd: modRoot(modName), stdio: 'inherit' })
  if (!fs.existsSync(output)) throw new Error(`Build did not produce ${output}`)

  const modDir = stageModDir(modName)
  fs.mkdirSync(modDir, { recursive: true })
  fs.copyFileSync(output, path.join(modDir, path.basename(output)))

  // Optional minGameVersion/maxGameVersion gate hlx-boot itself reads at load time (Boot.hx,
  // via Native.modInfoAllowsVersion), so it has to land in the same hlx/mods/<name>/ folder
  // as the .hl, not just the dist zip.
  const modInfoPath = path.join(modRoot(modName), 'mod.info')
  if (fs.existsSync(modInfoPath)) fs.copyFileSync(modInfoPath, path.join(modDir, 'mod.info'))

  console.log(`${modName}.hl -> built`)
}

// Every native plugin in this repo follows the same recipe, so there's nothing mod-specific left
// to script: cmake-configure against the HashLink SDK (hashlinkDir(), written by `npm run setup`),
// build, then `cmake --install` straight into this mod's staged plugins/ dir - each mod's own
// native/CMakeLists.txt already names its .hdll output and `install(TARGETS ... DESTINATION .)`s
// it there. Platform gating (native plugins are Windows-only) lives in each CMakeLists.txt itself
// (a `message(FATAL_ERROR ...)` if not WIN32), not duplicated here.
function buildNativePlugin(modName: string): void {
  const nativeDir = path.join(modRoot(modName), 'native')
  const nativeBuildDir = path.join(nativeDir, 'build')
  const pluginsDir = stagePluginsDir(modName)
  fs.mkdirSync(pluginsDir, { recursive: true })

  console.log(`Building ${modName}'s native plugin...`)
  execSync(`cmake -S "${nativeDir}" -B "${nativeBuildDir}" -DHASHLINK_DIR="${hashlinkDir()}"`, { stdio: 'inherit' })
  execSync(`cmake --build "${nativeBuildDir}" --config Release`, { stdio: 'inherit' })
  execSync(`cmake --install "${nativeBuildDir}" --config Release --prefix "${pluginsDir}"`, { stdio: 'inherit' })
}

// A driver has no HashLink dependency at all by design (hlx-boot LoadLibrary/GetProcAddress's it
// directly, never through HL's own native resolution - see hlx-core/hlx-boot/include/driver.h),
// so unlike buildNativePlugin this never passes -DHASHLINK_DIR.
function buildDriver(modName: string): void {
  const nativeDir = path.join(modRoot(modName), 'native')
  const nativeBuildDir = path.join(nativeDir, 'build')
  const driverDir = stageDriverDir(modName)
  fs.mkdirSync(driverDir, { recursive: true })

  console.log(`Building ${modName}'s driver...`)
  execSync(`cmake -S "${nativeDir}" -B "${nativeBuildDir}"`, { stdio: 'inherit' })
  execSync(`cmake --build "${nativeBuildDir}" --config Release`, { stdio: 'inherit' })
  execSync(`cmake --install "${nativeBuildDir}" --config Release --prefix "${driverDir}"`, { stdio: 'inherit' })
}

export async function runBuild(modName: string): Promise<void> {
  const type = modType(modName)
  if (type & ModType.MOD) buildHaxeSide(modName)
  if (type & ModType.PLUGIN) buildNativePlugin(modName)
  if (type & ModType.DRIVER) buildDriver(modName)
}
