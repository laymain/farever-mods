import fs from 'node:fs'
import path from 'node:path'
import archiver from 'archiver'
import { buildDir, distDir } from './paths.mts'

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
    // false = zip entries keep whatever path they have under build/ - that's
    // already build/<modName>/<modName>.hl, so no extra nesting needed.
    archive.directory(buildDir(modName), false)
    archive.finalize()
  })
}
