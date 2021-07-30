// TODO: document how to use the Linear Algebra module
#include <stdlib.h>
#include <math.h>

float lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

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

typedef struct { float c[2]; } V2f;
typedef struct { float c[3]; } V3f;
typedef struct { float c[4]; } V4f;

typedef V2f UV;
typedef V3f RGB;
typedef V4f RGBA;

V2f v2f(float x, float y)
{
    V2f v = {{x, y}};
    return v;
}

V2f v2fs(float x)
{
    return v2f(x, x);
}

V2f v2f_sum(V2f a, V2f b)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] += b.c[i];
    return a;
}

V2f v2f_sub(V2f a, V2f b)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] -= b.c[i];
    return a;
}

V2f v2f_mul(V2f a, V2f b)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] *= b.c[i];
    return a;
}

V2f v2f_div(V2f a, V2f b)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] /= b.c[i];
    return a;
}

V2f v2f_max(V2f a, V2f b)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] = fmaxf(a.c[i], b.c[i]);
    return a;
}

V2f v2f_min(V2f a, V2f b)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] = fminf(a.c[i], b.c[i]);
    return a;
}

float v2f_sqrlen(V2f a)
{
    float sqrlen = 0.0f;
    for (size_t i = 0; i < 2; ++i) sqrlen += a.c[i] * a.c[i];
    return sqrlen;
}

float v2f_len(V2f a)
{
    return sqrtf(v2f_sqrlen(a));
}

V2f v2f_lerp(V2f a, V2f b, V2f t)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] = lerpf(a.c[i], b.c[i], t.c[i]);
    return a;
}

V2f v2f_sqrt(V2f a)
{
    for (size_t i = 0; i < 2; ++i) a.c[i] = sqrtf(a.c[i]);
    return a;
}

V2f v2f_pow(V2f b, V2f e)
{
    for (size_t i = 0; i < 2; ++i) b.c[i] = powf(b.c[i], e.c[i]);
    return b;
}

//////////////////////////////

V3f v3f(float x, float y, float z)
{
    V3f v = {{x, y, z}};
    return v;
}

V3f v3fs(float x)
{
    return v3f(x, x, x);
}

V3f v3f_sum(V3f a, V3f b)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] += b.c[i];
    return a;
}

V3f v3f_sub(V3f a, V3f b)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] -= b.c[i];
    return a;
}

V3f v3f_mul(V3f a, V3f b)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] *= b.c[i];
    return a;
}

V3f v3f_div(V3f a, V3f b)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] /= b.c[i];
    return a;
}

V3f v3f_max(V3f a, V3f b)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] = fmaxf(a.c[i], b.c[i]);
    return a;
}

V3f v3f_min(V3f a, V3f b)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] = fminf(a.c[i], b.c[i]);
    return a;
}

float v3f_sqrlen(V3f a)
{
    float sqrlen = 0.0f;
    for (size_t i = 0; i < 3; ++i) sqrlen += a.c[i] * a.c[i];
    return sqrlen;
}

float v3f_len(V3f a)
{
    return sqrtf(v3f_sqrlen(a));
}

V3f v3f_lerp(V3f a, V3f b, V3f t)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] = lerpf(a.c[i], b.c[i], t.c[i]);
    return a;
}

V3f v3f_sqrt(V3f a)
{
    for (size_t i = 0; i < 3; ++i) a.c[i] = sqrtf(a.c[i]);
    return a;
}

V3f v3f_pow(V3f b, V3f e)
{
    for (size_t i = 0; i < 3; ++i) b.c[i] = powf(b.c[i], e.c[i]);
    return b;
}

//////////////////////////////

V4f v4f(float x, float y, float z, float w)
{
    V4f v = {{x, y, z, w}};
    return v;
}

V4f v4fs(float x)
{
    return v4f(x, x, x, x);
}

V4f v4f_sum(V4f a, V4f b)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] += b.c[i];
    return a;
}

V4f v4f_sub(V4f a, V4f b)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] -= b.c[i];
    return a;
}

V4f v4f_mul(V4f a, V4f b)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] *= b.c[i];
    return a;
}

V4f v4f_div(V4f a, V4f b)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] /= b.c[i];
    return a;
}

V4f v4f_max(V4f a, V4f b)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] = fmaxf(a.c[i], b.c[i]);
    return a;
}

V4f v4f_min(V4f a, V4f b)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] = fminf(a.c[i], b.c[i]);
    return a;
}

float v4f_sqrlen(V4f a)
{
    float sqrlen = 0.0f;
    for (size_t i = 0; i < 4; ++i) sqrlen += a.c[i] * a.c[i];
    return sqrlen;
}

float v4f_len(V4f a)
{
    return sqrtf(v4f_sqrlen(a));
}

V4f v4f_lerp(V4f a, V4f b, V4f t)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] = lerpf(a.c[i], b.c[i], t.c[i]);
    return a;
}

V4f v4f_sqrt(V4f a)
{
    for (size_t i = 0; i < 4; ++i) a.c[i] = sqrtf(a.c[i]);
    return a;
}

V4f v4f_pow(V4f b, V4f e)
{
    for (size_t i = 0; i < 4; ++i) b.c[i] = powf(b.c[i], e.c[i]);
    return b;
}
