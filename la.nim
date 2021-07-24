import math

const
  X* = 0
  Y* = 1
  Z* = 2
  W* = 3

  U* = 0
  V* = 1

  R* = 0
  G* = 1
  B* = 2
  A* = 3

proc lerp*(x, y, a: float): float = x + (y - x) * a

type Vec*[N: static[int]] = array[0..N-1, float]
type RGB* = Vec[3]
type Vec2* = Vec[2]

proc vec*[N: static[int]](s: float): Vec[N] =
  for i in 0..<N: result[i] = s

proc `+`*[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] + b[i]

proc `-`*[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] - b[i]

proc `*`*[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] * b[i]

proc `*=`*[N: static[int]](a: var Vec[N], b: Vec[N]) = a = a * b

proc `/`*[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = a[i] / b[i]

proc max*[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = max(a[i], b[i])

proc min*[N: static[int]](a, b: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = min(a[i], b[i])

proc length*[N: static[int]](a: Vec[N]): float =
  for i in 0..<N:
    result += a[i] * a[i]
  result = sqrt(result)

proc lerp*[N: static[int]](x, y, a: Vec[N]): Vec[N] =
  x + (y - x) * a

proc sqrt*[N: static[int]](x: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = sqrt(x[i])

proc pow*[N: static[int]](x, y: Vec[N]): Vec[N] =
  for i in 0..<N:
    result[i] = pow(x[i], y[i])
