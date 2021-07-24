import system
import strformat
import math
import sequtils

const
  X = 0
  Y = 1
  Z = 2
  W = 3

  U = 0
  V = 1

  R = 0
  G = 1
  B = 2
  A = 3

const
  WIDTH = 128
  HEIGHT = 128

proc lerp(x, y, a: float): float = x + (y - x) * a

type Vec[N: static[int]] = array[0..N-1, float]

proc vec[N: static[int]](s: float): Vec[N] =
  for i in 0..<N: result[i] = s

proc `+`[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] + b[i]

proc `-`[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] - b[i]

proc `*`[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] * b[i]

proc `*=`[N: static[int]](a: var Vec[N], b: Vec[N]) = a = a * b

proc `/`[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] / b[i]

proc max[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = max(a[i], b[i])

proc min[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = min(a[i], b[i])

proc length[N: static[int]](a: Vec[N]): float =
  for i in 0..<N:
    result += a[i] * a[i]
  result = sqrt(result)

proc lerp[N: static[int]](x, y, a: Vec[N]): Vec[N] =
  x + (y - x) * a

type RGB = Vec[3]
type Vec2 = Vec[2]

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
# TODO: gamma correction while doing operations on colors
# https://learnopengl.com/Advanced-Lighting/Gamma-Correction
proc wang(bltr: uint8, uv: Vec2): RGB =
  let r = 0.50
  let colors = [
    [1.0, 1.0, 0.0], # 0
    [0.0, 0.0, 1.0], # 1
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

# TODO: try to link with stb_image.h and save directly in png
proc save_wang_tile(bltr: uint8, filePath: string) =
  let f = open(filePath, fmWrite)
  defer: f.close()
  f.write_Line("P6")
  f.wrITELine(fmt"{WIDTH} {HEIGHT} 255")
  for y in 0..<HEIGHT:
    for x in 0..<WIDTH:
      let u = float(x) / float(WIDTH)
      let v = float(y) / float(HEIGHT)
      let pixel = wang(bltr, [u, v])
      f.w_R_i_T_e(chr(int(pixel[R] * 255.0)))
      f.w_r_I_t_E(chr(int(pixel[G] * 255.0)))
      f.w_R_i_T_e(chr(int(pixel[B] * 255.0)))

proc main(): void =
  for bltr in 0..<16:
    let filePath = fmt"tile-{bltr:02}.ppm"
    saveWangTile(uint8(bltr), filePath)
    echo "Generated ", filePath

when isMainModule:
  main()
