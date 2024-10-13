#ifndef MATHS_H
#define MATHS_H

#include <immintrin.h>
#include <float.h>
#include <tgmath.h>
#include <stdint.h>
#include <stdalign.h>

#undef  M_PI
#define M_PI 3.14159265f


#ifdef __cplusplus
extern "C"
{
#endif

extern uint32_t vec3_t_mov_mask[];
extern uint32_t xor_mask[];
extern uint32_t vec3_t_clear_w[];

#define clampf(min, max, value) fmaxf(fminf((value), (max)), (min))
#define array_count(array) (sizeof((array)) / sizeof((array)[0]))
#define align_to(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))
#define suitable_align(value) align_to(value, alignof(max_align_t))
#define cvt_ivec2_t_vec2_t(iv2) ((vec2_t){(float)(iv2).x, (float)(iv2).y})
#define cvt_vec2_t_ivec2_t(v2) ((ivec2_t){(int32_t)(v2).x, (int32_t)(v2).y})

uint32_t bitscan_forward64(uint64_t value);

/*================== ivec2_t ================== */

typedef union
{
    struct { int32_t x, y; };
    int32_t  comps[2];
} ivec2_t;

void ivec2_t_add(ivec2_t *r, ivec2_t *a, ivec2_t *b);

void ivec2_t_sub(ivec2_t *r, ivec2_t *a, ivec2_t *b);

/*================== vec2_t ================== */
typedef union
{
    struct { float x, y; };
    struct { float r, g; };
    struct { float u, v; };
    float  comps[2];
} vec2_t;

void vec2_t_add(vec2_t *r, vec2_t *a, vec2_t *b);

void vec2_t_sub(vec2_t *r, vec2_t *a, vec2_t *b);

void vec2_t_negate(vec2_t *r, vec2_t *v);

void vec2_t_scale(vec2_t *r, vec2_t *v, float s);

float vec2_t_dot(vec2_t *a,  vec2_t *b);

float vec2_t_length(vec2_t *v);

void vec2_t_lerp(vec2_t *r, vec2_t *a, vec2_t *b, float t);

float vec2_t_angle(vec2_t *v);

/*================== vec3_t ================== */
typedef union
{
    struct { float x, y, z; };
    struct { float r, g, b; };
    struct { vec2_t xy; float s; };
    float  comps[3];
} vec3_t;

// __always_inline void vec3_t_add(vec3_t *r, vec3_t *a, vec3_t *b)
// {
//     __asm__ volatile
//     (
//         "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
//         "movups xmm0, xmmword ptr [%0]\n"
//         "movups xmm1, xmmword ptr [%1]\n"
//         "addps xmm0, xmm1\n"
//         "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r)
//         : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
//         : "xmm0", "xmm1", "xmm2", "memory"
//     );
// }

// __always_inline void vec3_t_sub(vec3_t *r, vec3_t *a, vec3_t *b)
// {
//     __asm__ volatile
//     (
//         "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
//         "movups xmm0, xmmword ptr [%0]\n"
//         "movups xmm1, xmmword ptr [%1]\n"
//         "subps xmm0, xmm1\n"
//         "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r)
//         : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
//         : "xmm0", "xmm1", "xmm2", "memory"
//     );
// }

/* forward declaration */
typedef union mat3_t mat3_t;

void vec3_t_add(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_sub(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_norm(vec3_t *r, vec3_t *v);

void vec3_t_negate(vec3_t *r, vec3_t *v);

void vec3_t_scale(vec3_t *r, vec3_t *v, float s);

float vec3_t_dot(vec3_t *a,  vec3_t *b);

float vec3_t_length(vec3_t *v);

void vec3_t_cross(vec3_t *r, vec3_t *a, vec3_t *b);

float vec3_t_triple(vec3_t *a, vec3_t *b, vec3_t *c);

// __always_inline void vec3_t_max(vec3_t *r, vec3_t *a, vec3_t *b)
// {
//     __asm__ volatile
//     (
//         "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
//         "movups xmm0, xmmword ptr [%0]\n"
//         "movups xmm1, xmmword ptr [%1]\n"
//         "maxps xmm0, xmm1\n"
//         "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r)
//         : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
//         : "xmm0", "xmm1", "xmm2", "memory"
//     );
// }

// __always_inline void vec3_t_min(vec3_t *r, vec3_t *a, vec3_t *b)
// {
//     __asm__ volatile
//     (
//         "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
//         "movups xmm0, xmmword ptr [%0]\n"
//         "movups xmm1, xmmword ptr [%1]\n"
//         "minps xmm0, xmm1\n"
//         "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r)
//         : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
//         : "xmm0", "xmm1", "xmm2", "memory"
//     );
// }

void vec3_t_max(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_min(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_fabs(vec3_t *r, vec3_t *v);

void vec3_t_normalize(vec3_t *r, vec3_t *v);

// __always_inline void vec3_t_fmadd(vec3_t *r, vec3_t *a, vec3_t *b, float s)
// {
//     __asm__ volatile
//     (
//         "movss  xmm3, dword ptr [%3]\n"
//         "movups xmm0, xmmword ptr %[vec3_t_mov_mask]\n"
//         "shufps xmm3, xmm3, 0x00\n"
//         "movups xmm1, xmmword ptr [%0]\n"
//         "movups xmm2, xmmword ptr [%1]\n"
//         "vfmadd231ps xmm1, xmm2, xmm3\n"
//         "vmaskmovps xmmword ptr [%2], xmm0, xmm1\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r)
//         : "m" (s), [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
//         : "xmm0", "xmm1", "xmm2", "xmm3", "memory"
//     );
// }
void vec3_t_fmadd(vec3_t *r, vec3_t *a, vec3_t *b, float s);

void vec3_t_lerp(vec3_t *r, vec3_t *a, vec3_t *b, float s); 

void vec3_t_snap(vec3_t *r, vec3_t *v, float snap_value);

void vec3_t_snap_on_plane(vec3_t *r, vec3_t *v, mat3_t *plane_orientation, float snap_value);

void vec3_t_snap_oriented(vec3_t *r, vec3_t *v, mat3_t *orientation, float snap_value);

// __always_inline uint32_t vec3_t_box_overlap(vec3_t *min_a, vec3_t *max_a, vec3_t *min_b, vec3_t *max_b)
// {
//     uint32_t overlap = 0;

//     __asm__ volatile
//     (
//         "movups xmm0, xmmword ptr [%1]\n"
//         "movups xmm1, xmmword ptr [%4]\n"
//         "movups xmm3, xmmword ptr [%5]\n"
//         /* test if min_a is less than max_b */
//         "cmpps xmm0, xmm1, 0x1\n"
//         "movups xmm1, xmmword ptr [%2]\n"
//         "movups xmm2, xmmword ptr [%3]\n"
//         /* test if max_a is greater than min_b */
//         "cmpps xmm1, xmm2, 0x6\n"
//         /* this will zero out the first three components of xmm0 if
//         both comparisons returned true */
//         "pxor xmm0, xmm1\n"
//         /* clear the bits of the last component, since we don't care about it */
//         "pand xmm0, xmm3\n"
//         /* test if xmm0 is filled with ones */
//         "ptest xmm0, xmm0\n"
//         "setz byte ptr [%0]\n" 
//         : "+m" (overlap)
//         : "rm" (min_a), "rm" (max_a), "rm" (min_b), "rm" (max_b), "m" (vec3_t_clear_w)
//         : "xmm0", "xmm1", "xmm2", "xmm3", "memory"
//     );

//     return overlap;
// }

uint32_t vec3_t_box_overlap(vec3_t *min_a, vec3_t *max_a, vec3_t *min_b, vec3_t *max_b);

uint32_t vec3_t_cmp(vec3_t *a, vec3_t *b);

/*================== vec4_t ================== */
typedef union
{
    float   comps[4];
    struct { vec3_t vec3; float s; };
    struct { vec3_t normal; float dist; };
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    __m128  vec;
} vec4_t, quat_t;

#define QUAT_T_ID (quat_t){0, 0, 0, 1}

// __always_inline void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b)
// {
//     __asm__ volatile
//     (
//         "movups xmm0, xmmword ptr [%0]\n"
//         "movups xmm1, xmmword ptr [%1]\n"
//         "addps xmm0, xmm1\n"
//         "movups xmmword ptr [%2], xmm0\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r) :
//         : "xmm0", "xmm1", "xmm2", "memory"
//     );
// }

// __always_inline void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b)
// {
//     __asm__ volatile
//     (
//         "movups xmm0, xmmword ptr [%0]\n"
//         "movups xmm1, xmmword ptr [%1]\n"
//         "subps xmm0, xmm1\n"
//         "movups xmmword ptr [%2], xmm0\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r) :
//         : "xmm0", "xmm1", "xmm2", "memory"
//     );
// }

__always_inline void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b)
{
    r->vec = _mm_add_ps(a->vec, b->vec);
}

void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_normalize(vec4_t *r, vec4_t *v);

__always_inline void vec4_t_scale(vec4_t *r, vec4_t *v, float s)
{
    r->vec = _mm_mul_ps(v->vec, _mm_broadcast_ss(&s));
}

float vec4_t_dot(vec4_t *a, vec4_t *b);

float vec4_t_length(vec4_t *v);

void vec4_t_max(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_min(vec4_t *r, vec4_t *a, vec4_t *b);

__always_inline void vec4_t_fmadd(vec4_t *r, vec4_t *a, vec4_t *b, float s)
{
    // r->vec = _mm_fmadd_ps(_mm_broadcast_ss(&s), b->vec, a->vec);

    // __m128 scale = _mm_load_ss(&s);
    // r->vec = _mm_fmadd_ps(_mm_shuffle_ps(scale, scale, 0x00), b->vec, a->vec);

    // __asm__ volatile
    // (
    //     "movss  xmm3, dword ptr [%3]\n"
    //     "shufps xmm3, xmm3, 0x00\n"
    //     "movups xmm1, xmmword ptr [%0]\n"
    //     "movups xmm2, xmmword ptr [%1]\n"
    //     "vfmadd231ps xmm1, xmm2, xmm3\n"
    //     "movups xmmword ptr [%2], xmm1\n"
    //     : "+rm" (a), "+rm" (b), "+rm" (r)
    //     : "m" (s)
    //     : "xmm1", "xmm2", "xmm3", "memory"
    // );

    // __asm__ volatile
    // (
    //     // "movss  xmm3, dword ptr [%3]\n"
    //     // "shufps xmm3, xmm3, 0x00\n"
    //     "vbroadcastss xmm3, dword ptr [%3]\n"
    //     "movups xmm1, xmmword ptr [%0]\n"
    //     "movups xmm2, xmmword ptr [%1]\n"
    //     "vfmadd231ps xmm1, xmm2, xmm3\n"
    //     "movups xmmword ptr [%2], xmm1\n"
    //     : "+rm" (a), "+rm" (b), "+rm" (r)
    //     : "m" (s)
    //     : "xmm1", "xmm2", "xmm3", "memory"
    // );

    __m128 xmm3 = _mm_load_ss(&s);
    xmm3 = _mm_shuffle_ps(xmm3, xmm3, 0x00);
    r->vec = _mm_fmadd_ps(xmm3, b->vec, a->vec);
    // __m128 xmm1 = _mm_load_ps(a->comps);
}

void quat_t_vec3_t_rotate(vec3_t *r, quat_t *quat, vec3_t *v);

void quat_t_vec3_t_rotate_inverse(vec3_t *r, quat_t *quat, vec3_t *v);

void quat_t_mult(quat_t *r, quat_t *a, quat_t *b);

void quat_t_conj(quat_t *r, quat_t *q);

void quat_slerp(vec4_t *r, vec4_t *a, vec4_t *b, float t);

/*================== vec4_t ================== */

/* this mostly exists to define __m256i constants */
typedef union
{
    int32_t comps[8];
    __m256i vec;
}ivec8_t;

/*================== mat2_t ================== */

typedef union mat2_t
{
    float   comps[4];
    vec2_t  rows[2];
    struct 
    { 
        float c00, c01;
        float c10, c11;
    };
} mat2_t;

void mat2_t_mul(mat2_t *r, mat2_t *a, mat2_t *b);

void mat2_t_vec2_t_mul(vec2_t *r, vec2_t *v, mat2_t *m);

/*================== mat3_t ================== */
typedef union mat3_t
{
    float   comps[9];
    vec3_t  rows[3];
    struct 
    { 
        float c00, c01, c02;
        float c10, c11, c12;
        float c20, c21, c22; 
    };
} mat3_t;

// void mat3_t_id(mat3_t *m);

#define mat3_t_id ((mat3_t){1, 0, 0, 0, 1, 0, 0, 0, 1})

void mat3_t_transpose(mat3_t *r, mat3_t *m);

void mat3_t_mul(mat3_t *r, mat3_t *a, mat3_t *b);

void mat3_t_rotx(mat3_t *r, float turns);

void mat3_t_rotx_mul(mat3_t *r, float turns);

void mat3_t_roty(mat3_t *r, float turns);

void mat3_t_roty_mul(mat3_t *r, float turns);

void mat3_t_rotz(mat3_t *r, float turns);

void mat3_t_rotz_mul(mat3_t *r, float turns);

void mat3_t_vec3_t_mul(vec3_t *r, vec3_t *v, mat3_t *m);

void mat3_t_from_quat_t(mat3_t *r, quat_t *q);

/*================== mat4_t ================== */
typedef union
{
    float   comps[16];
    vec4_t  rows[4];
    struct 
    { 
        float c00, c01, c02, c03;
        float c10, c11, c12, c13;
        float c20, c21, c22, c23;
        float c30, c31, c32, c33; 
    };
} mat4_t;

void mat4_t_id(mat4_t *m);

void mat4_t_transpose(mat4_t *r, mat4_t *m);

void mat4_t_mul(mat4_t *r, mat4_t *a, mat4_t *b);

void mat4_t_rotx(mat4_t *r, float turns);

void mat4_t_rotx_mul(mat4_t *r, float turns);

void mat4_t_roty(mat4_t *r, float turns);

void mat4_t_roty_mul(mat4_t *r, float turns);

void mat4_t_rotz(mat4_t *r, float turns);

void mat4_t_rotz_mul(mat4_t *r, float turns);

void mat4_t_invvm(mat4_t *r, mat4_t *m);

void mat4_t_ortho(mat4_t *r, float width, float height, float z_near, float z_far);

void mat4_t_persp(mat4_t* m, float fov_y, float aspect, float z_near, float z_far);

void mat4_t_proj_planes(mat4_t *m, vec4_t *planes);

__always_inline void mat4_t_vec4_t_mul(vec4_t *r, vec4_t *v, mat4_t *m)
{
    // r->vec = _mm_add_ps(
    //     _mm_add_ps(
    //         _mm_mul_ps(m->rows[0].vec, _mm_shuffle_ps(v->vec, v->vec, 0x00)),
    //         _mm_mul_ps(m->rows[1].vec, _mm_shuffle_ps(v->vec, v->vec, 0x55))
    //     ), 
    //     _mm_add_ps(
    //         _mm_mul_ps(m->rows[2].vec, _mm_shuffle_ps(v->vec, v->vec, 0xaa)),
    //         _mm_mul_ps(m->rows[3].vec, _mm_shuffle_ps(v->vec, v->vec, 0xff))
    //     )
    // );

    // __m128 row0 = _mm_shuffle_ps(v->vec, v->vec, 0x00);
    // __m128 row1 = _mm_shuffle_ps(v->vec, v->vec, 0x55);
    // __m128 row2 = _mm_shuffle_ps(v->vec, v->vec, 0xaa);
    // __m128 row3 = _mm_shuffle_ps(v->vec, v->vec, 0xff);

    __m128 row0 = _mm_broadcast_ss(&v->x);
    __m128 row1 = _mm_broadcast_ss(&v->y);
    __m128 row2 = _mm_broadcast_ss(&v->z);
    __m128 row3 = _mm_broadcast_ss(&v->w);

    // __m128 sum0 = _mm_add_ps(_mm_mul_ps(m->rows[0].vec, row0), _mm_mul_ps(m->rows[1].vec, row1));
    // __m128 sum1 = _mm_add_ps(_mm_mul_ps(m->rows[2].vec, row2), _mm_mul_ps(m->rows[3].vec, row3));

    r->vec = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(m->rows[0].vec, row0), _mm_mul_ps(m->rows[1].vec, row1)), 
        _mm_add_ps(_mm_mul_ps(m->rows[2].vec, row2), _mm_mul_ps(m->rows[3].vec, row3))
    );
}

// __always_inline void mat4_t_vec4_t_mul(vec4_t *r, vec4_t *v, mat4_t *m)
// {
//     // r->vec = _mm_add_ps(
//     //     _mm_add_ps(
//     //         _mm_mul_ps(m->rows[0].vec, _mm_shuffle_ps(v->vec, v->vec, 0x00)),
//     //         _mm_mul_ps(m->rows[1].vec, _mm_shuffle_ps(v->vec, v->vec, 0x55))
//     //     ), 
//     //     _mm_add_ps(
//     //         _mm_mul_ps(m->rows[2].vec, _mm_shuffle_ps(v->vec, v->vec, 0xaa)),
//     //         _mm_mul_ps(m->rows[3].vec, _mm_shuffle_ps(v->vec, v->vec, 0xff))
//     //     )
//     // );

//     __m128 row0 = _mm_shuffle_ps(v->vec, v->vec, 0x00);
//     __m128 row1 = _mm_shuffle_ps(v->vec, v->vec, 0x55);
//     __m128 row2 = _mm_shuffle_ps(v->vec, v->vec, 0xaa);
//     __m128 row3 = _mm_shuffle_ps(v->vec, v->vec, 0xff);

//     // __m128 sum0 = _mm_add_ps(_mm_mul_ps(m->rows[0].vec, row0), _mm_mul_ps(m->rows[1].vec, row1));
//     // __m128 sum1 = _mm_add_ps(_mm_mul_ps(m->rows[2].vec, row2), _mm_mul_ps(m->rows[3].vec, row3));

//     r->vec = _mm_add_ps(
//         _mm_add_ps(_mm_mul_ps(m->rows[0].vec, row0), _mm_mul_ps(m->rows[1].vec, row1)), 
//         _mm_add_ps(_mm_mul_ps(m->rows[2].vec, row2), _mm_mul_ps(m->rows[3].vec, row3))
//     );
// }

void mat4_t_comp(mat4_t *r, mat3_t *m, vec3_t *v);

#ifdef __cplusplus
}
#endif

#endif