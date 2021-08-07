#define _POSIX_C_SOURCE 199309L
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>

#include <X11/Xlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define FLAG_IMPLEMENTATION
#include "./flag.h"

#define X 0
#define Y 1
#define Z 2
#define W 3

#define U 0
#define V 1

#define R 0
#define G 1
#define B 2
#define A 3

#define LA_IMPLEMENTATION
#include "./la.h"

typedef V3f RGB;
typedef V2f UV;

#include "./prof.c"

#define DEFAULT_TILE_WIDTH_PX 64
#define DEFAULT_TILE_HEIGHT_PX 64
#define MAX_TILE_WIDTH_PX (32 * 1024)
#define MAX_TILE_HEIGHT_PX (32 * 1024)

#define ATLAS_WIDTH_TL 4
#define ATLAS_HEIGHT_TL 4
static_assert(ATLAS_WIDTH_TL * ATLAS_HEIGHT_TL == 16, "The amout of tiles in the Atlas should be equal to 16 since that's the size of the full Wang Tile set");

#define DEFAULT_GRID_WIDTH_TL 13
#define DEFAULT_GRID_HEIGHT_TL 10
#define MAX_GRID_WIDTH_TL (32 * 1024)
#define MAX_GRID_HEIGHT_TL (32 * 1024)

#define POTATO_SIZE (2 * 1000 * 1000 * 1000)

typedef uint32_t BLTR;
typedef RGB (*Frag_Shader)(float time_uniform, BLTR bltr, UV uv);
typedef uint32_t RGBA32;

RGBA32 make_rgba32(float r, float g, float b)
{
    return (((uint32_t) (b * 255.0)) << 16) |
           (((uint32_t) (g * 255.0)) << 8) |
           (uint32_t) (r * 255.0) |
           0xFF000000;
}

// TODO: colors as runtime parameters @cli
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

// TODO: more wang tile ideas: @stream
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
// TODO: try to speed up the shader with SIMD instructions @stream
RGB wang_blobs(float time_uniform, BLTR bltr, UV uv)
{
    float r = lerpf(0.0f, 1.0f, (sinf(time_uniform * 2.0f) + 1.0f) / 2.0f);

    static const V2f sides[4] = {
        {{1.0, 0.5}}, // r
        {{0.5, 0.0}}, // t
        {{0.0, 0.5}}, // l
        {{0.5, 1.0}}, // b
    };

    RGB result = v3fs(0.0f);
    for (size_t i = 0; i < 4; ++i) {
        V2f p = sides[i];
        float t = 1.0f - fminf(v2f_len(v2f_sub(p, uv)) / r, 1.0f);
        result = v3f_lerp(result, colors[bltr & 1], v3fs(t));
        bltr = bltr >> 1;
    }
    return v3f_pow(result, v3fs(1.0f / 2.2f));
}

RGB wang_digits(float time_uniform, BLTR bltr, UV uv)
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
        V3f result = v3f_lerp(colors[0], colors[1], v3fs(t));
        return v3f_pow(result, v3fs(1.0f / 2.2f));
    }

    while (index-- > 0) {
        bltr >>= 1;
    }

    return colors[bltr & 1];
}

typedef struct {
    void *memory;               // the beginning of the renderer memory
    size_t memory_size;         // the size of the whole renderer memory

    size_t tile_width_px;       // provided by the user
    size_t tile_height_px;      // provided by the user

    RGBA32 *atlas;              // must be within the memory region defined by `memory` and `memory_size`
    size_t atlas_width_px;      // must be equal to (ATLAS_WIDTH_TL * tile_width_px)
    size_t atlas_height_px;     // must be equal to (ATLAS_HEIGHT_TL * tile_height_px)

    BLTR *grid_tl;              // must be within the memory region defined by `memory` and `memory_size`
    size_t grid_width_tl;       // provided by the user
    size_t grid_height_tl;      // provided by the user

    RGBA32 *grid_px;            // must be within the memory region defined by `memory` and `memory_size`
    size_t grid_width_px;       // must be equal to (grid_width_tl * tile_width_px)
    size_t grid_height_px;      // must be equal to (grid_height_tl * tile_height_px)

    float time_uniform;         // current time in seconds, used for animation in shaders
} Renderer;

// TODO: pull the renderer out of the global scope @cleanup
static Renderer renderer;

void renderer_free(Renderer *r)
{
    if (r->memory) {
        free(r->memory);
    }
    memset(r, 0, sizeof(*r));
}

void renderer_realloc(Renderer *r,
                      int not_potato,
                      size_t tile_width_px, size_t tile_height_px,
                      size_t grid_width_tl, size_t grid_height_tl)
{
    assert(0 < tile_width_px);
    assert(tile_width_px <= MAX_TILE_WIDTH_PX);
    assert(0 < tile_height_px);
    assert(tile_height_px <= MAX_TILE_HEIGHT_PX);

    assert(0 < grid_width_tl);
    assert(grid_width_tl <= MAX_GRID_WIDTH_TL);
    assert(0 < grid_height_tl);
    assert(grid_height_tl <= MAX_GRID_HEIGHT_TL);

    // The maximum values are carefully picked so the final memory_size
    // does not overflow 64 bit unsigned integer.
    //
    // Remove the static_asserts below to confirm that you know what
    // you are doing. :)
    static_assert(MAX_TILE_WIDTH_PX <= 1024 * 32, "MAX_TILE_WIDTH_PX is too big");
    static_assert(MAX_TILE_HEIGHT_PX <= 1024 * 32, "MAX_TILE_HEIGHT_PX is too big");
    static_assert(MAX_GRID_WIDTH_TL <= 1024 * 32, "MAX_GRID_WIDTH_TL is too big");
    static_assert(MAX_GRID_HEIGHT_TL <= 1024 * 32, "MAX_GRID_HEIGHT_TL is too big");

    renderer_free(r);

    r->tile_width_px = tile_width_px;
    r->tile_height_px = tile_height_px;

    r->atlas_width_px = ATLAS_WIDTH_TL * r->tile_width_px;
    r->atlas_height_px = ATLAS_HEIGHT_TL * r->tile_height_px;
    size_t atlas_size_bytes = r->atlas_width_px * r->atlas_height_px * sizeof(RGBA32);

    r->grid_width_tl = grid_width_tl;
    r->grid_height_tl = grid_height_tl;
    size_t grid_tl_size_bytes = r->grid_width_tl * r->grid_height_tl * sizeof(BLTR);
    static_assert(sizeof(RGBA32) == sizeof(BLTR), "The renderer memory management model expects RGBA32 and BLTR to have the same size");

    r->grid_width_px = r->grid_width_tl * r->tile_width_px;
    r->grid_height_px = r->grid_height_tl * r->tile_height_px;
    size_t grid_px_size_bytes = r->grid_width_px * r->grid_height_px * sizeof(RGBA32);

    r->memory_size = atlas_size_bytes + grid_tl_size_bytes + grid_px_size_bytes;
    if (!not_potato && r->memory_size >= POTATO_SIZE) {
        fprintf(stderr, "ERROR: you are trying to allocate more than %d bytes of memory.\n", POTATO_SIZE);
        fprintf(stderr, "  The laptop of the original author of this software is a potato\n");
        fprintf(stderr, "  and can't handle such big allocations without killing their entire system.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "  Please confirm that your machine is not a potato by providing -not-potato flag.\n");
        exit(1);
    }

    // TODO: make sure the memory alignment is right @cleanup
    // malloc(), according to the docs, already returns the memory that is
    // "suitably aligned for any built-in type".
    // I think the only thing we need to check if the regions sizes are divisible by 8
    // so their starts don't end up on the boundry of a word. If they are not we should pad them.
    r->memory = malloc(r->memory_size);
    if (r->memory == NULL) {
        fprintf(stderr, "ERROR: could not allocate the memory for the renderer: %s\n", strerror(errno));
        exit(1);
    }

    r->atlas   = r->memory;
    r->grid_tl = (BLTR*) ((char*) r->memory + atlas_size_bytes);
    r->grid_px = (RGBA32*) ((char*) r->memory + atlas_size_bytes + grid_tl_size_bytes);
}

void generate_tile32(uint32_t *pixels, size_t width, size_t height, size_t stride,
                     BLTR bltr, Frag_Shader shader)
{
    Renderer *r = &renderer;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            float u = (float) x / (float) width;
            float v = (float) y / (float) height;
            RGB p = shader(r->time_uniform, bltr, v2f(u, v));
            pixels[y * stride + x] = make_rgba32(p.c[R], p.c[G], p.c[B]);
        }
    }
}

void *generate_tile_thread(void *arg)
{
    Renderer *r = &renderer;

    size_t bltr = (size_t) arg;
    size_t y = (bltr / ATLAS_WIDTH_TL) * r->tile_width_px;
    size_t x = (bltr % ATLAS_WIDTH_TL) * r->tile_width_px;


    // TODO: the tile shader as the runtime parameter @cli
    generate_tile32(
        &r->atlas[y * r->atlas_width_px + x],
        r->tile_width_px, r->tile_height_px, r->atlas_width_px,
        bltr, wang_digits);

    return NULL;
}

// TODO: a runtime parameter to limit the amount of created threads @cli
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
    Renderer *r = &renderer;

    // +---+---+---+
    // | m | l | l
    // +---+---+---+
    // | t |tl |tl
    // +---+---+---+
    // | t |tl |tl
    // +   +   +   +
    //

    // First Top Left Corner
    r->grid_tl[0] = rand_tile(0, 0);

    // First Top Row
    for (size_t x = 1; x < r->grid_width_tl; ++x) {
        // p = bltr == 0b0100 == 1 << 2
        //
        //                             bltr
        // grid[x - 1]               = abcd
        // grid[x - 1] & 0b0001      = 000d
        // grid[x - 1] & 0b0001 << 2 = 0d00
        BLTR lp = 1 << 2;
        BLTR lv = (r->grid_tl[x - 1] & 1) << 2;
        r->grid_tl[x] = rand_tile(lv, lp);
    }

    // First Left Column
    for (size_t y = 1; y < r->grid_height_tl; ++y) {
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
        BLTR tv = (r->grid_tl[(y - 1) * r->grid_width_tl] & 8) >> 2;
        r->grid_tl[y * r->grid_width_tl] = rand_tile(tv, tp);
    }

    // The Rest of the Tiles
    for (size_t y = 1; y < r->grid_height_tl; ++y) {
        for (size_t x = 1; x < r->grid_width_tl; ++x) {
            //     +---+
            //     | t |
            // +---+---+
            // | l | g |
            // +---+---+
            //
            BLTR lp = 1 << 2;
            BLTR lv = (r->grid_tl[y * r->grid_width_tl + x - 1] & 1) << 2;
            BLTR tp = 1 << 1;
            BLTR tv = (r->grid_tl[(y - 1) * r->grid_width_tl + x] & 8) >> 2;
            r->grid_tl[y * r->grid_width_tl + x] = rand_tile(lv | tv, lp | tp);
        }
    }
}

void render_grid(void)
{
    Renderer *r = &renderer;
    // TODO: parallelize grid rendering
    for (size_t gy_tl = 0; gy_tl < r->grid_height_tl; ++gy_tl) {
        for (size_t gx_tl = 0; gx_tl < r->grid_width_tl; ++gx_tl) {
            BLTR bltr = r->grid_tl[gy_tl * r->grid_width_tl + gx_tl];
            size_t ax_tl = bltr % ATLAS_WIDTH_TL;
            size_t ay_tl = bltr / ATLAS_WIDTH_TL;

            size_t gx_px = gx_tl * r->tile_width_px;
            size_t gy_px = gy_tl * r->tile_height_px;
            size_t ax_px = ax_tl * r->tile_width_px;
            size_t ay_px = ay_tl * r->tile_height_px;

            copy_pixels32(&r->grid_px[gy_px * r->grid_width_px + gx_px], r->grid_width_px,
                          &r->atlas[ay_px * r->atlas_width_px + ax_px], r->atlas_width_px,
                          r->tile_width_px, r->tile_height_px);
        }
    }
}

void live_rendering_with_xlib(void)
{
    Renderer *r = &renderer;

    generate_grid();

    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: could not open the default display\n");
        exit(1);
    }

    // TODO: More control over the Visual that is used by live_rendering_with_xlib() @cleanup
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

    // TODO: the byte order of the pixels differ between the X11 and our renderer @cleanup
    XImage *image = XCreateImage(display,
                                 wa.visual,
                                 wa.depth,
                                 ZPixmap,
                                 0,
                                 (char*) r->grid_px,
                                 r->grid_width_px,
                                 r->grid_height_px,
                                 32,
                                 r->grid_width_px * sizeof(RGBA32));

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
        r->time_uniform = (float) now.tv_sec + (now.tv_nsec / 1000) * 0.000001;

        // TODO: live rendering animation that transitions between different grids @stream
        render_atlas();
        render_grid();
        // TODO: XPutImage is slow, try to use XCopyArea instead @stream
        // https://www.x.org/releases/X11R7.5/doc/man/man3/XCopyArea.3.html
        XPutImage(display, window, gc, image,
                  0, 0,
                  0, 0,
                  r->grid_width_px,
                  r->grid_height_px);
    }

    XCloseDisplay(display);
}

void offline_rendering_into_png_files(bool no_png, const char *atlas_png_path, const char *grid_png_path)
{
    Renderer *r = &renderer;

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

        if (!no_png) {
            begin_clock("ATLAS PNG OUTPUT");
            {
                if (!stbi_write_png(atlas_png_path, r->atlas_width_px, r->atlas_height_px, 4, r->atlas, r->atlas_width_px * sizeof(RGBA32))) {
                    fprintf(stderr, "ERROR: could not save file %s: %s\n", atlas_png_path,
                            strerror(errno));
                    exit(1);
                }
            }
            end_clock();
            printf("Atlas saved to %s\n", atlas_png_path);

            begin_clock("GRID PNG OUTPUT");
            {
                if (!stbi_write_png(grid_png_path, r->grid_width_px, r->grid_height_px, 4, r->grid_px, r->grid_width_px * sizeof(RGBA32))) {
                    fprintf(stderr, "ERROR: could not save file %s: %s\n", grid_png_path,
                            strerror(errno));
                    exit(1);
                }
            }
            end_clock();
            printf("Grid saved to %s\n", grid_png_path);
        }
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

void usage(const char *program, FILE *stream)
{
    fprintf(stream, "Usage: %s [OPTIONS]\n", program);
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

void check_flag_range(const char *program, uint64_t *flag, uint64_t min, uint64_t max)
{
    if (!(min <= *flag && *flag <= max)) {
        usage(program, stderr);
        fprintf(stderr, "ERROR: -%s: outside of the range [%"PRIu64"..%"PRIu64"]\n",
                flag_name(flag), min, max);
        exit(1);
    }
}

int main(int argc, char **argv)
{
    Renderer *r = &renderer;

    srand(time(0));

    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    bool *live = flag_bool("live", false, "Animate and render the Wang Tiles in \"real time\" in a separate X11 window");
    bool *no_png = flag_bool("no-png", false, "Don't output any png files in the offline mode. Just test the performance of the renderer");
    bool *not_potato = flag_bool("not_potato", false, "Confirm that your machine is not a potato");
    char **atlas_png_path = flag_str("atlas-png", "atlas.png", "Path to the output atlas.png file");
    char **grid_png_path = flag_str("grid-png", "grid.png", "Path to the output grid.png file");
    uint64_t *tw = flag_uint64("tw", DEFAULT_TILE_WIDTH_PX, "The width of the tile in PIXELS");
    uint64_t *th = flag_uint64("th", DEFAULT_TILE_HEIGHT_PX, "The height of the tile in PIXELS");
    uint64_t *gw = flag_uint64("gw", DEFAULT_GRID_WIDTH_TL, "The width of the grid in TILES");
    uint64_t *gh = flag_uint64("gh", DEFAULT_GRID_HEIGHT_TL, "The height of the grid in TILES");

    assert(argc >= 1);
    const char *program = argv[0];

    if (!flag_parse(argc, argv)) {
        usage(program, stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help) {
        usage(program, stdout);
        exit(0);
    }

    check_flag_range(program, tw, 1, MAX_TILE_WIDTH_PX);
    check_flag_range(program, th, 1, MAX_TILE_HEIGHT_PX);
    check_flag_range(program, gw, 1, MAX_GRID_WIDTH_TL);
    check_flag_range(program, gh, 1, MAX_GRID_HEIGHT_TL);

    renderer_realloc(r, *not_potato, *tw, *th, *gw, *gh);

    printf("Tile Size (px):      %zux%zu\n", r->tile_width_px, r->tile_height_px);
    printf("Grid Size (tl):      %zux%zu\n", r->grid_width_tl, r->grid_height_tl);
    printf("Grid Size (px):      %zux%zu\n", r->grid_width_px, r->grid_height_px);
    printf("Memory Size (bytes): %zu\n", r->memory_size);

    if (*live) {
        live_rendering_with_xlib();
    } else {
        offline_rendering_into_png_files(*no_png, *atlas_png_path, *grid_png_path);
    }

    renderer_free(r);

    return 0;
}
