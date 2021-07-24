import system
import strformat
import math

import la

const
  TILE_WIDTH: uint = 128
  TILE_HEIGHT: uint = 128
  TILESET_WIDTH: uint = TILE_WIDTH * 4
  TILESET_HEIGHT: uint = TILE_HEIGHT * 4

proc stripes(uv: Vec2): RGB =
  let n = 20.0
  [(sin(uv[U] * n) + 1.0) * 0.5,
   (sin((uv[U] + uv[V]) * n) + 1.0) * 0.5,
   (cos(uv[V] * n) + 1.0) * 0.5]

proc japan(uv: Vec2): RGB =
  let a = (vec[2](0.5) - uv).length > 0.25
  [1.0, float(a), float(a)]

# TODO: more wang tile ideas:
# - Metaballs: https://en.wikipedia.org/wiki/Metaballs

#     t
#   +---+
# l |   | r
#   +---+
#     b
#
#   bltr
#   0000 = 00
#   0001 = 01
#   0010 = 02
#   ...
#   1111 = 15
#
proc wang(bltr: uint8, uv: Vec2): RGB =
  let r = 0.50
  let colors = [
    [1.0, 0.0, 0.0], # 0
    [0.0, 1.0, 1.0], # 1

    # [1.0, 1.0, 0.0], # 0
    # [0.0, 0.0, 1.0], # 1

    # [0.0, 1.0, 0.0], # 0
    # [1.0, 0.0, 1.0], # 1

    # [0.0, 0.0, 0.0], # 0
    # [1.0, 1.0, 1.0], # 1
  ]
  let sides = [
    [1.0, 0.5], # r
    [0.5, 0.0], # t
    [0.0, 0.5], # l
    [0.5, 1.0], # b
  ]
  var mask = bltr
  for p in sides:
    let t = 1.0 - min((p - uv).length / r, 1.0)
    let c = colors[mask and 1]
    result = lerp(result, c, vec[3](t))
    mask = mask shr 1
  result = pow(result, vec[3](1.0 / 2.2))

type RGBA32 = uint32
type Tile32 = array[TILE_HEIGHT, array[TILE_WIDTH, RGBA32]]
type TileSet32 = array[TILESET_HEIGHT, array[TILESET_WIDTH, RGBA32]]

proc makeRGBA32(r, g, b: float): RGBA32 =
  (uint32(b * 255.0) shl 16) or
  (uint32(g * 255.0) shl 8) or
  uint32(r * 255.0) or
  uint32(0xFF000000)

proc putTileIntoTileSet(bltr: uint8, tileSet: var TileSet32) =
  let x0 = (bltr mod 4) * TILE_WIDTH
  let y0 = (bltr div 4) * TILE_HEIGHT
  for dy in 0..<TILE_HEIGHT:
    for dx in 0..<TILE_WIDTH:
      let u = float(dx) / float(TILE_WIDTH)
      let v = float(dy) / float(TILE_HEIGHT)
      let pixel = wang(bltr, [u, v])
      let x = x0 + dx
      let y = y0 + dy
      tileSet[y][x] = makeRGBA32(pixel[R], pixel[G], pixel[B])

proc generateTileSet(tileSet: var TileSet32) =
  for bltr in 0..<16:
    echo "Generating tile ", bltr
    putTileIntoTileSet(uint8(bltr), tileSet)

# TODO: try to link with stb_image.h and save directly in png
proc saveTileSet32AsPPM(tileSet: TileSet32, filePath: string) =
  let f = open(filePath, fmWrite)
  defer: f.close()
  f.writeLine("P6")
  f.writeLine(fmt"{TILESET_WIDTH} {TILESET_HEIGHT} 255")
  for y in 0..<TILESET_HEIGHT:
    for x in 0..<TILESET_WIDTH:
      f.write(chr(tileSet[y][x]          and 0xFF))
      f.write(chr((tileSet[y][x] shr 8)  and 0xFF))
      f.write(chr((tileSet[y][x] shr 16) and 0xFF))

var tileSet: TileSet32

# TODO: generate the random Wang Tile Grid
proc main(): void =
  generateTileSet(tileSet)
  let filePath = "tile-set.ppm"
  echo "Saving tile set to ", filePath
  saveTileSet32AsPPM(tileSet, filePath)

when isMainModule:
  main()
