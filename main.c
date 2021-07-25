#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "./la.c"

#define TILE_WIDTH_PX 256
#define TILE_HEIGHT_PX 256

#define ATLAS_WIDTH_TL 4
#define ATLAS_HEIGHT_TL 4
#define ATLAS_WIDTH_PX (TILE_WIDTH_PX * ATLAS_WIDTH_TL)
#define ATLAS_HEIGHT_PX (TILE_HEIGHT_PX * ATLAS_HEIGHT_TL)

typedef uint8_t BLTR;
typedef RGB (*Frag_Shader)(BLTR bltr, UV uv);
typedef uint32_t RGBA32;

RGBA32 make_rgba32(float r, float g, float b)
{
    return (((uint32_t) (b * 255.0)) << 16) |
           (((uint32_t) (g * 255.0)) << 8) |
           (uint32_t) (r * 255.0) |
           0xFF000000;
}

RGB stripes(UV uv)
{
    float n = 20.0f;
    Vec3f v = vec3f(sinf(uv.c[U] * n),
                    sinf((uv.c[U] + uv.c[V]) * n),
                    cosf(uv.c[V] * n));
    return vec3f_mul(vec3f_sum(v, vec3fs(1.0f)),
                     vec3fs(0.5f));
}

RGB japan(UV uv)
{
    float r = 0.25;
    int a = vec2f_sqrlen(vec2f_sub(vec2fs(0.5f), uv)) > r * r;
    return vec3f(1.0f, (float) a, (float) a);
}

// TODO: more wang tile ideas:
// - Metaballs: https://en.wikipedia.org/wiki/Metaballs

//     t
//   +---+
// l |   | r
//   +---+
//     b
//
//   bltr
//   0000 = 00
//   0001 = 01
//   0010 = 02
//   ...
//   1111 = 15
//
RGB wang(BLTR bltr, UV uv)
{
    float r = 0.50;
    static const RGB colors[] = {
        // {{1.0f, 0.0f, 0.0f}}, // 0
        // {{0.0f, 1.0f, 1.0f}}, // 1

        {{1.0f, 1.0f, 0.0f}}, // 0
        {{0.0f, 0.0f, 1.0f}}, // 1

        // {{0.0f, 1.0f, 0.0f}}, // 0
        // {{1.0f, 0.0f, 1.0f}}, // 1

        // {{0.0f, 0.0f, 0.0f}}, // 0
        // {{1.0f, 1.0f, 1.0f}}, // 1
    };
    static_assert(sizeof(colors) / sizeof(colors[0]) == 2, "colors array must have exactly 2 elements");

    static const Vec2f sides[4] = {
        {{1.0, 0.5}}, // r
        {{0.5, 0.0}}, // t
        {{0.0, 0.5}}, // l
        {{0.5, 1.0}}, // b
    };

    RGB result = vec3fs(0.0f);
    for (size_t i = 0; i < 4; ++i) {
        Vec2f p = sides[i];
        float t = 1.0f - fminf(vec2f_len(vec2f_sub(p, uv)) / r, 1.0f);
        result = vec3f_lerp(result, colors[bltr & 1], vec3fs(t));
        bltr = bltr >> 1;
    }
    return vec3f_pow(result, vec3fs(1.0f / 2.2f));
}

void generate_tile32(uint32_t *pixels, size_t width, size_t height, size_t stride,
                     BLTR bltr, Frag_Shader shader)
{
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            float u = (float) x / (float) width;
            float v = (float) y / (float) height;
            RGB p = shader(bltr, vec2f(u, v));
            pixels[y * stride + x] = make_rgba32(p.c[R], p.c[G], p.c[B]);
        }
    }
}

RGBA32 tile[TILE_WIDTH_PX * TILE_HEIGHT_PX];
RGBA32 atlas[ATLAS_WIDTH_PX * ATLAS_HEIGHT_PX];

// TODO: generate the random Wang Tile Grid
int main()
{
    const char *output_file_path = "output.png";

    // TODO: multi-threaded atlas generation
    for (BLTR bltr = 0; bltr < 16; ++bltr) {
        size_t y = (bltr / ATLAS_WIDTH_TL) * TILE_WIDTH_PX;
        size_t x = (bltr % ATLAS_WIDTH_TL) * TILE_WIDTH_PX;

        printf("Generating God Seed %.0f%%...\n", (float) bltr / 15.0f * 100.0f);

        generate_tile32(
            &atlas[y * ATLAS_WIDTH_PX + x],
            TILE_WIDTH_PX, TILE_HEIGHT_PX, ATLAS_WIDTH_PX,
            bltr, wang);
    }

    if (!stbi_write_png(output_file_path, ATLAS_WIDTH_PX, ATLAS_HEIGHT_PX, 4, tile, ATLAS_WIDTH_PX * sizeof(RGBA32))) {
        fprintf(stderr, "ERROR: could not save file %s: %s\n", output_file_path,
                strerror(errno));
        exit(1);
    }

    printf("Generated Dream Seed forsenCD\n");

    return 0;
}
