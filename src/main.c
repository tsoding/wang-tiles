#define _POSIX_C_SOURCE 199309L
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include <X11/Xlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "./la.c"
#include "./prof.c"

// TODO: make <TILE_WIDTH_PX>x<TILE_HEIGHT_PX> and <GRID_WIDTH_TL>x<GRID_HEIGHT_TL> runtime parameters

#define TILE_WIDTH_PX 64
#define TILE_HEIGHT_PX 64

#define ATLAS_WIDTH_TL 4
#define ATLAS_HEIGHT_TL 4
static_assert(ATLAS_WIDTH_TL * ATLAS_HEIGHT_TL == 16, "The amout of tiles in the Atlas should be equal to 16 since that's the size of the full Wang Tile set");
#define ATLAS_WIDTH_PX (TILE_WIDTH_PX * ATLAS_WIDTH_TL)
#define ATLAS_HEIGHT_PX (TILE_HEIGHT_PX * ATLAS_HEIGHT_TL)

#define GRID_WIDTH_TL 13
#define GRID_HEIGHT_TL 10
#define GRID_WIDTH_PX (GRID_WIDTH_TL * TILE_WIDTH_PX)
#define GRID_HEIGHT_PX (GRID_HEIGHT_TL * TILE_HEIGHT_PX)

typedef uint32_t BLTR;
typedef RGB (*Frag_Shader)(BLTR bltr, UV uv);
typedef uint32_t RGBA32;

RGBA32 make_rgba32(float r, float g, float b)
{
    return (((uint32_t) (b * 255.0)) << 16) |
           (((uint32_t) (g * 255.0)) << 8) |
           (uint32_t) (r * 255.0) |
           0xFF000000;
}

// TODO: colors as runtime parameters
static const RGB colors[] = {
    // {{1.0f, 0.0f, 0.0f}}, // 0
    // {{0.0f, 1.0f, 1.0f}}, // 1

    // {{1.0f, 1.0f, 0.0f}}, // 0
    // {{0.0f, 0.0f, 1.0f}}, // 1

    // {{0.99f, 0.99f, 0.01f}}, // 0
    // {{0.01f, 0.01f, 0.99f}}, // 1

    // {{0.0f, 1.0f, 0.0f}}, // 0
    // {{1.0f, 0.0f, 1.0f}}, // 1

    {{0.0f, 0.0f, 0.0f}}, // 0
    {{1.0f, 1.0f, 1.0f}}, // 1
};
static_assert(sizeof(colors) / sizeof(colors[0]) == 2, "colors array must have exactly 2 elements");
float time_uniform = 0.0f;

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
// TODO: try to speed up the shader with SIMD instructions
RGB wang_blobs(BLTR bltr, UV uv)
{
    float r = lerpf(0.0f, 1.0f, (sinf(time_uniform * 2.0f) + 1.0f) / 2.0f);

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

RGB wang_digits(BLTR bltr, UV uv)
{
    float ds[4] = {
        1.0f - uv.c[X], // r
        uv.c[Y],        // t
        uv.c[X],        // l
        1.0f - uv.c[Y], // b
    };

    int index = -1;
    for (int i = 0; i < 4; ++i) {
        if (index < 0 || ds[index] > ds[i]) {
            index = i;
        }
    }

    float r = lerpf(0.0f, 0.5f, (sinf(time_uniform * 4.0f) + 1.0f) / 2.0f);

    if (ds[index] > r) {
        float t = 1.0f - (float) bltr / 16.0f;
        Vec3f result = vec3f_lerp(colors[0], colors[1], vec3fs(t));
        return vec3f_pow(result, vec3fs(1.0f / 2.2f));
    }

    while (index-- > 0) {
        bltr >>= 1;
    }

    return colors[bltr & 1];
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

RGBA32 atlas[ATLAS_WIDTH_PX * ATLAS_HEIGHT_PX];
BLTR grid_tl[GRID_WIDTH_TL * GRID_HEIGHT_TL];
RGBA32 grid_px[GRID_WIDTH_PX * GRID_HEIGHT_PX];

void *generate_tile_thread(void *arg)
{
    size_t bltr = (size_t) arg;
    size_t y = (bltr / ATLAS_WIDTH_TL) * TILE_WIDTH_PX;
    size_t x = (bltr % ATLAS_WIDTH_TL) * TILE_WIDTH_PX;

    // TODO: the tile shader as the runtime parameter
    generate_tile32(
        &atlas[y * ATLAS_WIDTH_PX + x],
        TILE_WIDTH_PX, TILE_HEIGHT_PX, ATLAS_WIDTH_PX,
        bltr, wang_digits);

    return NULL;
}

// TODO: a runtime parameter to limit the amount of created threads
// ./wang -j5
void render_atlas(void)
{
    // TODO: it would be nice to figure out how to not recreate the threads on each render_atlas()
    pthread_t threads[16] = {0};

    for (size_t i = 0; i < 16; ++i) {
        // TODO: can we get rid of pthread dependency on Linux completely and just use clone(2) directly
        pthread_create(&threads[i], NULL, generate_tile_thread, (void*) i);
    }

    for (size_t i = 0; i < 16; ++i) {
        pthread_join(threads[i], NULL);
    }
}

// rand_tile(-a-b)
//
// v     = abcd   // value mask
// p     = 0101   // position mask
// c     = xyzw   // candidate tile
//
// v & p = 0b0d
// c & p = 0y0w
//
// fits(v, p, c) = c & p = v & p
//
// Then just bruteforce c checking if it fits() the constraints vp
// c = 0, 1, 2, ..., 15

BLTR rand_tile(BLTR v, BLTR p)
{
    BLTR cs[16] = {0};
    size_t n = 0;

    for (BLTR c = 0; c < 16; ++c) {
        if ((c & p) == (v & p)) {
            cs[n++] = c;
        }
    }

    return cs[rand() % n];
}

void copy_pixels32(RGBA32 *dst, size_t dst_stride,
                   RGBA32 *src, size_t src_stride,
                   size_t width, size_t height)
{
    while (height-- > 0) {
        memcpy(dst, src, width * sizeof(RGBA32));
        dst += dst_stride;
        src += src_stride;
    }
}

void generate_grid(void)
{
    // +---+---+---+
    // | m | l | l
    // +---+---+---+
    // | t |tl |tl
    // +---+---+---+
    // | t |tl |tl
    // +   +   +   +
    //

    // First Top Left Corner
    grid_tl[0] = rand_tile(0, 0);

    // First Top Row
    for (size_t x = 1; x < GRID_WIDTH_TL; ++x) {
        // p = bltr == 0b0100 == 1 << 2
        //
        //                             bltr
        // grid[x - 1]               = abcd
        // grid[x - 1] & 0b0001      = 000d
        // grid[x - 1] & 0b0001 << 2 = 0d00
        BLTR lp = 1 << 2;
        BLTR lv = (grid_tl[x - 1] & 1) << 2;
        grid_tl[x] = rand_tile(lv, lp);
    }

    // First Left Column
    for (size_t y = 1; y < GRID_HEIGHT_TL; ++y) {
        // p = bltr == 0b0010 == 1 << 1
        // g = grid[(y - 1) * GRID_WIDTH_TL]
        //
        //                     bltr
        // g                 = abcd
        // g & 0b1000        = a000
        // (g & 0b1000) >> 2 = 00a0
        //
        // v = (g & 8) >> 2

        BLTR tp = 1 << 1;
        BLTR tv = (grid_tl[(y - 1) * GRID_WIDTH_TL] & 8) >> 2;
        grid_tl[y * GRID_WIDTH_TL] = rand_tile(tv, tp);
    }

    // The Rest of the Tiles
    for (size_t y = 1; y < GRID_HEIGHT_TL; ++y) {
        for (size_t x = 1; x < GRID_WIDTH_TL; ++x) {
            //     +---+
            //     | t |
            // +---+---+
            // | l | g |
            // +---+---+
            //
            BLTR lp = 1 << 2;
            BLTR lv = (grid_tl[y * GRID_WIDTH_TL + x - 1] & 1) << 2;
            BLTR tp = 1 << 1;
            BLTR tv = (grid_tl[(y - 1) * GRID_WIDTH_TL + x] & 8) >> 2;
            grid_tl[y * GRID_WIDTH_TL + x] = rand_tile(lv | tv, lp | tp);
        }
    }
}

void render_grid(void)
{
    // TODO: parallelize grid rendering
    for (size_t gy_tl = 0; gy_tl < GRID_HEIGHT_TL; ++gy_tl) {
        for (size_t gx_tl = 0; gx_tl < GRID_WIDTH_TL; ++gx_tl) {
            BLTR bltr = grid_tl[gy_tl * GRID_WIDTH_TL + gx_tl];
            size_t ax_tl = bltr % ATLAS_WIDTH_TL;
            size_t ay_tl = bltr / ATLAS_WIDTH_TL;

            size_t gx_px = gx_tl * TILE_WIDTH_PX;
            size_t gy_px = gy_tl * TILE_HEIGHT_PX;
            size_t ax_px = ax_tl * TILE_WIDTH_PX;
            size_t ay_px = ay_tl * TILE_HEIGHT_PX;

            copy_pixels32(&grid_px[gy_px * GRID_WIDTH_PX + gx_px], GRID_WIDTH_PX,
                          &atlas[ay_px * ATLAS_WIDTH_PX + ax_px], ATLAS_WIDTH_PX,
                          TILE_WIDTH_PX, TILE_HEIGHT_PX);
        }
    }
}

void live_rendering_with_xlib(void)
{
    generate_grid();

    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: could not open the default display\n");
        exit(1);
    }

    // TODO: More control over the Visual that is used by live_rendering_with_xlib()
    // Right now we use whatever Visual that is picked by XCreateSimpleWindow
    // Since our render only works with 32bit RGBA pixels we need to be more explicit about
    // what we expect from X11. Probably need to write some code that grabs the list of available
    // Visuals and use the most suited one.
    Window window = XCreateSimpleWindow(
                        display,
                        XDefaultRootWindow(display),
                        0, 0,
                        800, 600,
                        0,
                        0,
                        0);

    XWindowAttributes wa = {0};
    XGetWindowAttributes(display, window, &wa);

    // TODO: the byte order of the pixels differ between the X11 and our renderer
    XImage *image = XCreateImage(display,
                                 wa.visual,
                                 wa.depth,
                                 ZPixmap,
                                 0,
                                 (char*) grid_px,
                                 GRID_WIDTH_PX,
                                 GRID_HEIGHT_PX,
                                 32,
                                 GRID_WIDTH_PX * sizeof(RGBA32));

    GC gc = XCreateGC(display, window, 0, NULL);

    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    XSelectInput(display, window, 0);

    XMapWindow(display, window);

    int quit = 0;
    while (!quit) {
        while (XPending(display) > 0) {
            XEvent event = {0};
            XNextEvent(display, &event);
            switch (event.type) {
            // TODO: animation controls in live rendering
            case ClientMessage: {
                if ((Atom) event.xclient.data.l[0] == wm_delete_window) {
                    quit = 1;
                }
            }
            break;
            }
        }

        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
            fprintf(stderr, "ERROR: could not get current monotonic time: %s\n",
                    strerror(errno));
            exit(1);
        }
        time_uniform = (float) now.tv_sec + (now.tv_nsec / 1000) * 0.000001;

        // TODO: live rendering animation that transitions between different grids
        render_atlas();
        render_grid();
        XPutImage(display, window, gc, image,
                  0, 0,
                  0, 0,
                  GRID_WIDTH_PX,
                  GRID_HEIGHT_PX);
    }

    XCloseDisplay(display);
}

void offline_rendering_into_png_files(const char *atlas_png_path, const char *grid_png_path)
{
    begin_clock("TOTAL");
    {
        begin_clock("RENDERING");
        {
            begin_clock("ATLAS RENDERING");
            {
                render_atlas();
            }
            end_clock();

            // TODO: Could we do atlas rendering and grid generation in parallel?
            // Grid generation and atlas rendering are completely independant.
            begin_clock("GRID GENERATION");
            {
                generate_grid();
            }
            end_clock();

            begin_clock("GRID RENDERING");
            {
                render_grid();
            }
            end_clock();
        }
        end_clock();

        begin_clock("ATLAS PNG OUTPUT");
        {
            if (!stbi_write_png(atlas_png_path, ATLAS_WIDTH_PX, ATLAS_HEIGHT_PX, 4, atlas, ATLAS_WIDTH_PX * sizeof(RGBA32))) {
                fprintf(stderr, "ERROR: could not save file %s: %s\n", atlas_png_path,
                        strerror(errno));
                exit(1);
            }
        }
        end_clock();
        printf("Atlas saved to %s\n", atlas_png_path);

        begin_clock("GRID PNG OUTPUT");
        {
            if (!stbi_write_png(grid_png_path, GRID_WIDTH_PX, GRID_HEIGHT_PX, 4, grid_px, GRID_WIDTH_PX * sizeof(RGBA32))) {
                fprintf(stderr, "ERROR: could not save file %s: %s\n", grid_png_path,
                        strerror(errno));
                exit(1);
            }
        }
        end_clock();
        printf("Grid saved to %s\n", grid_png_path);
    }
    end_clock();

    printf("Performance Summary:\n");
    dump_summary(stdout);
}

char *shift_args(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

void help(const char *program, FILE *stream)
{
    fprintf(stream, "Usage: %s [OPTIONS]\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -help                     Print this help message to stdout and exit with 0 code\n");
    fprintf(stream, "    -live                     Animate and render the Wang Tiles in \"real time\"\n");
    fprintf(stream, "                              in a separate X11 window\n");
    fprintf(stream, "    -atlas-png <atlas.png>    Path to the output atlas.png file (default: \"atlas.png\")\n");
    fprintf(stream, "    -grid-png <grid.png>      Path to the output grid.png file (default: \"grid.png\")\n");
}

int main(int argc, char **argv)
{
    const char *program = shift_args(&argc, &argv);

    int live = 0;
    const char *atlas_png_path = "atlas.png";
    const char *grid_png_path = "grid.png";

    // TODO: implement Go flag-like module for parsing parameters
    while (argc > 0) {
        const char *param = shift_args(&argc, &argv);

        if (strcmp(param, "-live") == 0) {
            live = 1;
        } else if (strcmp(param, "-help") == 0) {
            help(program, stdout);
            exit(0);
        } else if (strcmp(param, "-atlas-png") == 0) {
            if (argc <= 0) {
                help(program, stderr);
                fprintf(stderr, "ERROR: %s: no argument is provided", param);
                exit(1);
            }
            atlas_png_path = shift_args(&argc, &argv);
        } else if (strcmp(param, "-grid-png") == 0) {
            if (argc <= 0) {
                help(program, stderr);
                fprintf(stderr, "ERROR: %s: no argument is provided", param);
                exit(1);
            }
            grid_png_path = shift_args(&argc, &argv);
        } else {
            help(program, stderr);
            fprintf(stderr, "ERROR: %s: unknown parameter\n", param);
            exit(1);
        }
    }

    srand(time(0));

    printf("Tile Size (px): %dx%d\n", TILE_WIDTH_PX, TILE_HEIGHT_PX);
    printf("Grid Size (tl): %dx%d\n", GRID_WIDTH_TL, GRID_HEIGHT_TL);
    printf("Grid Size (px): %dx%d\n", GRID_WIDTH_PX, GRID_HEIGHT_PX);

    if (live) {
        live_rendering_with_xlib();
    } else {
        offline_rendering_into_png_files(atlas_png_path, grid_png_path);
    }

    return 0;
}
