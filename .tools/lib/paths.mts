import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'

export const REPO_ROOT = path.resolve(import.meta.dirname, '../..')

// A mod's kind is a bitfield inferred from what's actually in its folder, not declared anywhere -
// MOD for a Haxe side (compile.hxml, compiled to .hl and staged under hlx/mods/<name>/), PLUGIN
// for a native side (native/CMakeLists.txt, cmake-built and staged under hlx/plugins/<name>/). A
// mod can be either or both: shader-cache is MOD|PLUGIN (Haxe hook + native .hdll),
// shader-persistent-cache is PLUGIN only (no Haxe side at all), everything else is MOD only.
export const ModType = {
  MOD: 1 << 0,
  PLUGIN: 1 << 1,
} as const

export function modType(modName: string): number {
  const root = path.join(REPO_ROOT, modName)
  let type = 0
  if (fs.existsSync(path.join(root, 'compile.hxml'))) type |= ModType.MOD
  if (fs.existsSync(path.join(root, 'native', 'CMakeLists.txt'))) type |= ModType.PLUGIN
  return type
}

// A "mod" is any top-level folder with a non-zero modType() - one of its two markers
// (compile.hxml, native/CMakeLists.txt) is something every mod has and nothing else at repo root does.
export function discoverMods(): string[] {
  return fs
    .readdirSync(REPO_ROOT, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .filter((name) => modType(name) !== 0)
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

// Raw haxe compiler output, per each mod's own compile.hxml `-hl` line - not
// shaped like anything in particular, just where the compiler is told to put it.
export function outputHl(modName: string): string {
  return path.join(buildDir(modName), modName, `${modName}.hl`)
}

// The assembled tree, shaped exactly like the game's own `hlx/` folder
// (`mods/<name>/<name>.hl`, `plugins/<name>/*.hdll` - each mod gets its own plugins subfolder,
// matching hlx-boot's own EagerLoadPluginHdlls scan, hlx-core/hlx-boot/src/boot.c) - runZip zips
// this as-is, and deploy copies it as-is, so both stay in sync with hlx-boot/hlx-loader's own
// folder conventions by construction rather than by duplicated logic.
export function stageDir(modName: string): string {
  return path.join(buildDir(modName), 'stage')
}

export function stageModDir(modName: string): string {
  return path.join(stageDir(modName), 'mods', modName)
}

export function stagePluginsDir(modName: string): string {
  return path.join(stageDir(modName), 'plugins', modName)
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

// This project's own config (distinct from HLX_CORE_CONFIG above, which is read-only and
// belongs to hlx-core) - currently just the HashLink SDK/checkout path every PLUGIN-type mod
// (see ModType above) needs to cmake-build its native side against. Gitignored (machine-specific
// path); populate via `npm run setup`.
const USER_CONFIG_PATH = path.join(REPO_ROOT, '.tools', 'user-config.json')

interface UserConfig {
  hashlinkDir: string
}

export function readUserConfig(): UserConfig {
  if (!fs.existsSync(USER_CONFIG_PATH)) throw new Error('.tools/user-config.json not found - run: npm run setup')
  return JSON.parse(fs.readFileSync(USER_CONFIG_PATH, 'utf8'))
}

export function writeUserConfig(config: UserConfig): void {
  fs.writeFileSync(USER_CONFIG_PATH, JSON.stringify(config, null, 2) + '\n', 'utf8')
}

export function hashlinkDir(): string {
  return toNativePath(readUserConfig().hashlinkDir)
}
