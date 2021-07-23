import system
import strformat
import math

const WIDTH = 512
const HEIGHT = 512

proc frag(u, v: float): tuple[r: float, g: float, b: float] =
  let n = 20.0
  ((sin(u * n) + 1.0) * 0.5,
   (sin((u + v) * n) + 1.0) * 0.5,
   (cos(v * n) + 1.0) * 0.5)

proc main(): void =
  let f = open("output.ppm", fmWrite)
  defer: f.close()
  f.writeLine("P6")
  f.writeLine(fmt"{WIDTH} {HEIGHT} 255")
  for y in 0..<HEIGHT:
    for x in 0..<WIDTH:
      let u = float(x) / float(WIDTH)
      let v = float(y) / float(HEIGHT)
      let (r, g, b) = frag(u, v)
      f.w_r_i_t_e(chr(int(r * 255.0)))
      f.w_r_i_t_e(chr(int(g * 255.0)))
      f.w_r_i_t_e(chr(int(b * 255.0)))

when isMainModule:
  main()
