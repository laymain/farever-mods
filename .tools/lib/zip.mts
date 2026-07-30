import fs from 'node:fs'
import path from 'node:path'
import archiver from 'archiver'
import { stageDir, distDir } from './paths.mts'

export function runZip(modName: string): Promise<void> {
  const outFile = path.join(distDir(modName), `${modName}.zip`)
  fs.mkdirSync(distDir(modName), { recursive: true })
  if (fs.existsSync(outFile)) fs.rmSync(outFile)

  return new Promise((resolve, reject) => {
    const output = fs.createWriteStream(outFile)
    const archive = archiver('zip', { zlib: { level: 9 } })

    output.on('close', () => {
      console.log(`${modName}/dist/${modName}.zip -> ${archive.pointer()} bytes`)
      resolve()
    })
    archive.on('error', reject)

    archive.pipe(output)
    // Nests the staged tree under an `hlx/` prefix inside the archive - so the entries read
    // hlx/mods/<modName>/<modName>.hl (+ hlx/plugins/*.hdll if any), and the zip can be extracted
    // straight into the game's own install root (not its hlx/ subfolder) for any mod, whether it
    // has a Haxe side, a native plugin, or both.
    archive.directory(stageDir(modName), 'hlx')
    archive.finalize()
  })
}
