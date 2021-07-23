# Wang Tiles Grid generator in [Nim](https://nim-lang.org/)

## Goal

Trying to generate a random grid of 2-colored [Wang Tiles](http://www.cr31.co.uk/stagecast/wang/intro.html) based on a single function definition:

```nim
proc wang(bltr: uint8, uv: Vec2): RGB = ...
```

where

- `bltr` is a 4 bit mask encoding the specific Wang Tile. Each bit represents a side of the tile in the following order: Bottom, Left, Top, Right. `0` is the first color, `1` is the second one.
- `uv` is a 2D vector that represents a point on the tile. Top-Left corner is `[0.0, 0.0]`, Bottom-Right corner is `[1.0, 1.0]`. Coordinates are normalized and resolution-independant.
- The result of the function is a 3D vector that represents an RGB color of a pixel on the tile `bltr` at `uv`.

Think of it as a [Fragment Shader](https://www.khronos.org/opengl/wiki/Fragment_Shader) that generates a Texture for a Wang Tile based on its bitmask description.

Here are some tiles we generated so far:

![0000](./tiles/tile-00.png) ![0001](./tiles/tile-01.png) ![0010](./tiles/tile-02.png) ![0011](./tiles/tile-03.png)

![0100](./tiles/tile-04.png) ![0101](./tiles/tile-05.png) ![0110](./tiles/tile-06.png) ![0111](./tiles/tile-07.png)

![1000](./tiles/tile-08.png) ![1001](./tiles/tile-09.png) ![1010](./tiles/tile-10.png) ![1011](./tiles/tile-11.png)

![1100](./tiles/tile-12.png) ![1101](./tiles/tile-13.png) ![1110](./tiles/tile-14.png) ![1111](./tiles/tile-15.png)

## Quick Start

Install [Nim](https://nim-lang.org/) compiler first.

```console
$ nim r -d:release main.nim
$ feh *.ppm
```
