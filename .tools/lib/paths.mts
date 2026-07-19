import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'

export const REPO_ROOT = path.resolve(import.meta.dirname, '../..')

// A "mod" is any top-level folder with its own compile.hxml - that's the one
// marker every mod has and nothing else at repo root does.
export function discoverMods(): string[] {
  return fs
    .readdirSync(REPO_ROOT, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .filter((name) => fs.existsSync(path.join(REPO_ROOT, name, 'compile.hxml')))
}

export function modRoot(modName: string): string {
  return path.join(REPO_ROOT, modName)
}

export function buildDir(modName: string): string {
  return path.join(modRoot(modName), 'build')
}

export function distDir(modName: string): string {
  return path.join(modRoot(modName), 'dist')
}

// Wrapped in its own <modName>/ subfolder so build/ is already shaped exactly
// like a valid mod archive (matching hlx-loader's hlx/mods/<name>/<name>.hl
// scan convention) - runZip just zips build/ as-is, no extra nesting logic.
export function outputHl(modName: string): string {
  return path.join(buildDir(modName), modName, `${modName}.hl`)
}

// This project is standalone from hlx-core's own build/deploy pipeline, but
// there's only one game being modded, so re-asking the user to configure the
// same game install path a second time would just be friction. Reads
// hlx-core's config read-only rather than writing/owning any config of its
// own.
const HLX_CORE_CONFIG = path.resolve(REPO_ROOT, '../hlx-core/.tools/user-config.json')

interface HlxCoreConfig {
  gamePath: string
}

// Only matters if this ever runs from WSL/Linux against a Windows-style path
// - mirrors hlx-core/tools/lib/paths.mts's own toNativePath exactly, since
// the config file being read is the same Windows-style path either way.
export function toNativePath(windowsPath: string): string {
  if (os.platform() !== 'linux') return windowsPath
  return windowsPath
    .replace(/\\/g, '/')
    .replace(/^([A-Za-z]):/, (_, drive: string) => `/mnt/${drive.toLowerCase()}`)
}

export function gameDir(): string {
  if (!fs.existsSync(HLX_CORE_CONFIG)) {
    throw new Error(
      `${HLX_CORE_CONFIG} not found - run \`pnpm run setup\` in hlx-core first (this project reuses its game-path config, it doesn't keep its own).`,
    )
  }
  const config = JSON.parse(fs.readFileSync(HLX_CORE_CONFIG, 'utf8')) as HlxCoreConfig
  return toNativePath(config.gamePath)
}
