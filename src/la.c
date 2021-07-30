// Linear Algebra (LA) Module
//
//   Right now we support only 2, 3 and 4 dimensional floating point vectors.
//
// Type Naming Conventions
//
//   Types always start with a capital letter. 
//   The name of the type has the following format:
//     [object][size][component type]
//   where
//     [object] is a letter that denotes the LA object. Since we only support
//       vectors at the moment, it is always "V". But in the future we plan
//       to add "M" for "matrix"
//     [size] is the size of the object
//       For vectors it's usually 2, 3 or 4 denoting the amount of the components.
//     [component type] is a letter that denotes the type of the component of the object.
//       f - float
//       d - double
//       i - int
//   examples:
//       V2f, V3d, V4i, etc
//
// Function Naming Conventions
//   Each function name has the following format:
//     [type prefix]_[operation]
//                  ↑
//                  underscore symbol
//   where
//     [type prefix] is the name of the type of the object the function operates on.
//       The name follows the same naming conventions as described by 
//       "Type Naming Conventions" except the first letter is always small.
//     [operation] is the name of the operation.
//   examples:
//     V2f v2f_sum(V2f a, V2f b);
//     V3i v3i_sub(V3i a, V3i b);
//     etc.
//
// Vector Constructors
//
//   There are two types of vector constructors:
//   1. Just a regular one that is basically `[type prefix]` without `_[operation]`.
//      It accepts `[size]` arguments and constructs the corresponding vector. 
//      Examples: 
//        Vec2f vec2f(float x, float y);
//        Vec3f vec3f(float x, float y, float z);
//        Vec4f vec4f(float x, float y, float z, float w);
//   2. The Scalar Constructor. It has the name of `[type prefix]s` (with "s" at the end).
//      It accepts always 1 argument and initializes all the components of the vector
//      with that argument.
//      Examples:
//        Vec2f vec2fs(float s);
//        Vec3f vec3fs(float s);
//        Vec4f vec4fs(float s);
//
// Interoperation between Vectors and Scalars 
//   
//   Imagine that you have the following vector:
//
//     V2f v = v2f(69.0f, 420.0f);
//
//   and you want to scale it by 2.0f. You do it like this
//
//     v = v2f_mul(v, v2fs(2.0f));
//
//   v2f_mul() performs componentswise multiplication
//   v2fs() turns a scalar into a vector
//   the whole expression multiplies each component of v by 2.0f
//
//   This approach enables commutativity of the scalar operations without
//   introducing unnecessary functions:
//
//      v2f_mul(v, v2fs(2.0f)) == v2f_mul(v2fs(2.0f), v)
//
#include <stdlib.h>
#include <math.h>

// This one is kinda out of place, but this is because math.h 
// does not really provide any lerp operation.
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
