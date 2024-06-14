#include "maths.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>

uint32_t vec3_t_mov_mask[] = {
    0x80000000, 0x80000000, 0x80000000, 0x00000000
};

uint32_t xor_mask[] = {
    0x80000000, 0x80000000, 0x80000000, 0x80000000
};

uint32_t vec3_t_clear_w[] = {
    0xffffffff, 0xffffffff, 0xffffffff, 0x00000000
};

/* test it for now */
#define VEC3_T_FAST

uint32_t bitscan_forward64(uint64_t value)
{
    uint32_t index = 0;

    __asm__ volatile
    (
        "bsf rax, %1\n"
        "jz _gtfo\n"
            "inc rax\n"
            "mov %0, eax\n"
        "_gtfo:\n"
        : "=rm" (index)           
        : "rm" (value)            
        : "rax"
    );

    return index;
}

/*================== vec3_t ================== */

void vec2_t_add(vec2_t *r, vec2_t *a, vec2_t *b)
{
    *r = (vec2_t){a->x + b->x, a->y + b->y};
}

void vec2_t_sub(vec2_t *r, vec2_t *a, vec2_t *b)
{
    *r = (vec2_t){a->x - b->x, a->y - b->y};
}

void vec2_t_negate(vec2_t *r, vec2_t *v)
{
    *r = (vec2_t){-v->x, -v->y};
}

void vec2_t_scale(vec2_t *r, vec2_t *v, float s)
{
    *r = (vec2_t){v->x * s, v->y * s};
}

float vec2_t_dot(vec2_t *a,  vec2_t *b)
{
    return a->x * b->x + a->y * b->y;
}

float vec2_t_length(vec2_t *v)
{
    float s = v->x * v->x + v->y * v->y;

    if(s != 0)
    {
        s = sqrtf(s);
    }

    return s;
}

void vec2_t_lerp(vec2_t *r, vec2_t *a, vec2_t *b, float t)
{
    float one_minus_t = 1.0f - t;
    *r = (vec2_t){a->x * one_minus_t + b->x * t, a->y * one_minus_t + b->y * t};
}

float vec2_t_angle(vec2_t *v)
{
    
}

/*================== vec3_t ================== */
void vec3_t_add(vec3_t *r, vec3_t *a, vec3_t *b)
{
    #ifdef VEC3_T_FAST
        // __asm__ volatile
        // (
        //     "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
        //     "movups xmm0, xmmword ptr [%0]\n"
        //     "movups xmm1, xmmword ptr [%1]\n"
        //     "addps xmm0, xmm1\n"
        //     "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
        //     : "+rm" (a), "+rm" (b), "+rm" (r)
        //     : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
        //     : "xmm0", "xmm1", "xmm2", "memory"
        // );
        __m128 va = _mm_loadu_ps(a->comps);
        __m128 vb = _mm_loadu_ps(b->comps);
        __m128i store_mask = _mm_castps_si128(_mm_loadu_ps((const float *)vec3_t_mov_mask));
        va = _mm_add_ps(va, vb);
        _mm_maskstore_ps(r->comps, store_mask, va);
    #else
        *r = (vec3_t){a->x + b->x, a->y + b->y, a->z + b->z};
    #endif
}

void vec3_t_sub(vec3_t *r, vec3_t *a, vec3_t *b)
{
    #ifdef VEC3_T_FAST
        // __asm__ volatile
        // (
        //     "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
        //     "movups xmm0, xmmword ptr [%0]\n"
        //     "movups xmm1, xmmword ptr [%1]\n"
        //     "subps xmm0, xmm1\n"
        //     "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
        //     : "+rm" (a), "+rm" (b), "+rm" (r)
        //     : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
        //     : "xmm0", "xmm1", "xmm2", "memory"
        // );
        __m128 va = _mm_loadu_ps(a->comps);
        __m128 vb = _mm_loadu_ps(b->comps);
        __m128i store_mask = _mm_castps_si128(_mm_loadu_ps((const float *)vec3_t_mov_mask));
        va = _mm_sub_ps(va, vb);
        _mm_maskstore_ps(r->comps, store_mask, va);
    #else
        *r = (vec3_t){a->x - b->x, a->y - b->y, a->z - b->z};
    #endif
}

// void vec3_t_norm(vec3_t *r, vec3_t *v)
// {
//     float length = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);

//     if(length != 0)
//     {
//         *r = (vec3_t){v->x / length, v->y / length, v->z / length};
//     }
// }

void vec3_t_negate(vec3_t *r, vec3_t *v)
{
    #ifdef VEC3_T_FAST
        // __asm__ volatile
        // (
        //     "mov r9, %[v]\n"
        //     "mov rax, %[r]\n"
        //     "movups xmm0, %[xor_mask]\n"
        //     "movups xmm1, xmmword ptr [r9]\n"
        //     "movups xmm2, %[vec3_t_mov_mask]\n"
        //     "xorps xmm1, xmm0\n"
        //     "vmaskmovps xmmword ptr [rax], xmm2, xmm1\n"
        //     : [r] "+rm" (r), [v] "+rm" (v)
        //     : [xor_mask] "m" (xor_mask), [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
        //     : "rax", "r9", "xmm0", "xmm1", "xmm2", "memory"
        // );
        __m128 vv = _mm_loadu_ps(v->comps);
        __m128 flip_sign_mask = _mm_loadu_ps((const float *)xor_mask);
        __m128i store_mask = _mm_castps_si128(_mm_loadu_ps((const float *)vec3_t_mov_mask));
        vv = _mm_xor_ps(vv, flip_sign_mask);
        _mm_maskstore_ps(r->comps, store_mask, vv);
    #else
        *r = (vec3_t){-v->x, -v->y, -v->z};
    #endif
}

void vec3_t_scale(vec3_t *r, vec3_t *v, float s)
{
    *r = (vec3_t){v->x * s, v->y * s, v->z * s};
}

float vec3_t_dot(vec3_t *a,  vec3_t *b)
{
    #ifdef VEC3_T_FAST
        float result;
        __m128 va = _mm_loadu_ps(a->comps);
        __m128 vb = _mm_loadu_ps(b->comps);
        __m128 clear_w_mask = _mm_loadu_ps((const float *)vec3_t_clear_w);
        va = _mm_and_ps(va, clear_w_mask);
        vb = _mm_and_ps(vb, clear_w_mask);
        va = _mm_mul_ps(va, vb);
        va = _mm_hadd_ps(va, va);
        va = _mm_hadd_ps(va, va);
        _mm_store_ss(&result, va);
    return result;

    #else
        return a->x * b->x + a->y * b->y + a->z * b->z;
    #endif
}

float vec3_t_length(vec3_t *v)
{
    return sqrtf(vec3_t_dot(v, v));
}

void vec3_t_cross(vec3_t *r, vec3_t *a, vec3_t *b)
{
    *r = (vec3_t){{.x = a->y * b->z - a->z * b->y, .y = a->z * b->x - a->x * b->z, .z = a->x * b->y - a->y * b->x}};
}

float vec3_t_triple(vec3_t *a, vec3_t *b, vec3_t *c)
{
    vec3_t t;
    vec3_t_cross(&t, a, b);
    return vec3_t_dot(&t, c);
}

void vec3_t_max(vec3_t *r, vec3_t *a, vec3_t *b)
{ 
    #ifdef VEC3_T_FAST
        // __asm__ volatile
        // (
        //     "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
        //     "movups xmm0, xmmword ptr [%0]\n"
        //     "movups xmm1, xmmword ptr [%1]\n"
        //     "maxps xmm0, xmm1\n"
        //     "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
        //     : "+rm" (a), "+rm" (b), "+rm" (r)
        //     : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
        //     : "xmm0", "xmm1", "xmm2", "memory"
        // );
        __m128 va = _mm_loadu_ps(a->comps);
        __m128 vb = _mm_loadu_ps(b->comps);
        __m128i store_mask = _mm_castps_si128(_mm_loadu_ps((const float *)vec3_t_mov_mask));
        va = _mm_max_ps(va, vb);
        _mm_maskstore_ps(r->comps, store_mask, va);
    #else
        *r = (vec3_t){fmaxf(a->x, b->x), fmaxf(a->y, b->y), fmaxf(a->z, b->z)};
    #endif
}

void vec3_t_min(vec3_t *r, vec3_t *a, vec3_t *b)
{
    #ifdef VEC3_T_FAST
        // __asm__ volatile
        // (
        //     "movups xmm2, xmmword ptr %[vec3_t_mov_mask]\n"
        //     "movups xmm0, xmmword ptr [%0]\n"
        //     "movups xmm1, xmmword ptr [%1]\n"
        //     "minps xmm0, xmm1\n"
        //     "vmaskmovps xmmword ptr [%2], xmm2, xmm0\n"
        //     : "+rm" (a), "+rm" (b), "+rm" (r)
        //     : [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
        //     : "xmm0", "xmm1", "xmm2", "memory"
        // );
        __m128 va = _mm_loadu_ps(a->comps);
        __m128 vb = _mm_loadu_ps(b->comps);
        __m128i store_mask = _mm_castps_si128(_mm_loadu_ps((const float *)vec3_t_mov_mask));
        va = _mm_min_ps(va, vb);
        _mm_maskstore_ps(r->comps, store_mask, va);
    #else
        *r = (vec3_t){fminf(a->x, b->x), fminf(a->y, b->y), fminf(a->z, b->z)};
    #endif
}

void vec3_t_fabs(vec3_t *r, vec3_t *v)
{
    *r = (vec3_t){fabsf(v->x), fabsf(v->y), fabsf(v->z)};
}

void vec3_t_normalize(vec3_t *r, vec3_t *v)
{
    float length = vec3_t_length(v);

    if(length != 0.0f)
    {
        *r = (vec3_t){v->x / length, v->y / length, v->z / length};
    }
}

void vec3_t_fmadd(vec3_t *r, vec3_t *a, vec3_t *b, float s)
{
    #ifdef VEC3_T_FAST
        // __asm__ volatile
        // (
        //     "mov r9,  %[a]\n"
        //     "mov r10, %[b]\n"
        //     "mov rax, %[r]\n"
        //     "movss  xmm3, dword ptr %[s]\n"
        //     "movups xmm0, xmmword ptr %[vec3_t_mov_mask]\n"
        //     "shufps xmm3, xmm3, 0x00\n"
        //     "movups xmm1, xmmword ptr [r9]\n"
        //     "movups xmm2, xmmword ptr [r10]\n"
        //     "vfmadd231ps xmm1, xmm2, xmm3\n"
        //     "vmaskmovps xmmword ptr [rax], xmm0, xmm1\n"
        //     : [a] "+rm" (a), [b] "+rm" (b), [r] "+rm" (r)
        //     : [vec3_t_mov_mask] "m" (vec3_t_mov_mask), [s] "m" (s)
        //     : "rax", "r9", "r10", "xmm0", "xmm1", "xmm2", "memory"
        // );

        __asm__ volatile
        (
            "movss  xmm3, dword ptr [%3]\n"
            "movups xmm0, xmmword ptr %[vec3_t_mov_mask]\n"
            "shufps xmm3, xmm3, 0x00\n"
            "movups xmm1, xmmword ptr [%0]\n"
            "movups xmm2, xmmword ptr [%1]\n"
            "vfmadd231ps xmm1, xmm2, xmm3\n"
            "vmaskmovps xmmword ptr [%2], xmm0, xmm1\n"
            : "+rm" (a), "+rm" (b), "+rm" (r)
            : "m" (s), [vec3_t_mov_mask] "m" (vec3_t_mov_mask)
            : "xmm0", "xmm1", "xmm2", "memory"
        );
    #else 
        *r = (vec3_t){fmaf(b->x, s, a->x), fmaf(b->y, s, a->y), fmaf(b->z, s, a->z)};
    #endif
}

void vec3_t_lerp(vec3_t *r, vec3_t *a, vec3_t *b, float s)
{
    float s1 = 1.0f - s;
    *r = (vec3_t){a->x * s1 + b->x * s, a->y * s1 + b->y * s, a->z * s1 + b->z * s};
}

void vec3_t_snap(vec3_t *r, vec3_t *v, float snap_value)
{
    if(snap_value > 0.0f)
    {
        for(uint32_t index = 0; index < 3; index++)
        {
            float fract = fmodf(v->comps[index], snap_value);
        
            if(fabsf(fract / snap_value) >= 0.5f)
            {
                if(v->comps[index] < 0.0f)
                {
                    r->comps[index] = v->comps[index] - snap_value;
                }
                else
                {
                    r->comps[index] = v->comps[index] + snap_value;
                }
            }

            r->comps[index] -= fract;
        }
    }
    else
    {
        *r = *v;
    }
}

void vec3_t_snap_on_plane(vec3_t *r, vec3_t *v, mat3_t *plane_orientation, float snap_value)
{
    if(snap_value > 0.0f)
    {
        vec3_t plane_point;
        vec3_t plane_vec;
        vec3_t_scale(&plane_point, &plane_orientation->rows[1], vec3_t_dot(v, &plane_orientation->rows[1]));
        vec3_t_sub(&plane_vec, v, &plane_point);

        *r = (vec3_t){};

        for(uint32_t index = 0; index < 3; index++)
        {
            float proj = vec3_t_dot(&plane_vec, &plane_orientation->rows[index]);
            float fract = fmodf(proj, snap_value);

            if(fabsf(fract / snap_value) >= 0.5f)
            {
                if(proj < 0.0f)
                {
                    proj -= snap_value;
                }
                else
                {
                    proj += snap_value;
                }
            }

            proj -= fract;
            vec3_t_fmadd(r, r, &plane_orientation->rows[index], proj);
        }

        vec3_t_add(r, r, &plane_point);
    }
    else
    {
        *r = *v;
    }
}

void vec3_t_snap_oriented(vec3_t *r, vec3_t *v, mat3_t *orientation, float snap_value)
{
    if(snap_value > 0.0f)
    {
        vec3_t plane_vec = *v;

        *r = (vec3_t){};

        for(uint32_t index = 0; index < 3; index++)
        {
            float proj = vec3_t_dot(&plane_vec, &orientation->rows[index]);
            float fract = fmodf(proj, snap_value);

            if(fabsf(fract / snap_value) >= 0.5f)
            {
                if(proj < 0.0f)
                {
                    proj -= snap_value;
                }
                else
                {
                    proj += snap_value;
                }
            }

            proj -= fract;
            vec3_t_fmadd(r, r, &orientation->rows[index], proj);
        }
    }
    else
    {
        *r = *v;
    }
}

uint32_t vec3_t_box_overlap(vec3_t *min_a, vec3_t *max_a, vec3_t *min_b, vec3_t *max_b)
{
    uint32_t overlap = 0;

    __asm__ volatile
    (
        "movups xmm0, xmmword ptr [%1]\n"
        "movups xmm1, xmmword ptr [%4]\n"
        "movups xmm3, xmmword ptr [%5]\n"
        /* test if min_a is less than max_b */
        "cmpps xmm0, xmm1, 0x1\n"
        "movups xmm1, xmmword ptr [%2]\n"
        "movups xmm2, xmmword ptr [%3]\n"
        /* test if max_a is greater than min_b */
        "cmpps xmm1, xmm2, 0x6\n"
        /* this will zero out the first three components of xmm0 if
        both comparisons returned true */
        "pxor xmm0, xmm1\n"
        /* clear the bits of the last component, since we don't care about it */
        "pand xmm0, xmm3\n"
        /* test if all bits in xmm0 are zero */
        "ptest xmm0, xmm0\n"
        "setz byte ptr [%0]\n" 
        : "+m" (overlap)
        : "p" (min_a), "p" (max_a), "p" (min_b), "p" (max_b), "m" (vec3_t_clear_w)
        : "xmm0", "xmm1", "xmm2", "xmm3", "memory"
    );

    return overlap;
} 

uint32_t vec3_t_cmp(vec3_t *a, vec3_t *b)
{
    return a->x == b->x && a->y == b->y && a->z == b->z;
}

/*================== vec4_t ================== */

// void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b)
// {
//     // *r = (vec4_t){a->x + b->x, a->y + b->y, a->z + b->z, a->w + b->w};
//     // __asm__ volatile
//     // (
//     //     "mov rax, %[a]\n"
//     //     "mov r9,  %[b]\n"
//     //     "mov r10, %[r]\n"
//     //     "movups xmm0, xmmword ptr [rax]\n"
//     //     "movups xmm1, xmmword ptr [r9]\n"
//     //     "addps xmm0, xmm1\n"
//     //     "movups xmmword ptr [r10], xmm0\n"
//     //     : [r] "+r" (r), [a] "+r" (a), [b] "+r" (b) :
//     //     : "rax", "r10", "r11", "xmm0", "xmm1", "memory"
//     // );
//     // __m128 va = _mm_loadu_ps(a->comps);
//     // __m128 vb = _mm_loadu_ps(b->comps);
//     // va = _mm_add_ps(va, vb);
//     // _mm_storeu_ps(r->comps, va);
// }

void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b)
{
    // *r = (vec4_t){a->x - b->x, a->y - b->y, a->z - b->z, a->w - b->w};
    // __asm__ volatile
    // (
    //     "mov rax, %[a]\n"
    //     "mov r9,  %[b]\n"
    //     "mov r10, %[r]\n"
    //     "movups xmm0, xmmword ptr [rax]\n"
    //     "movups xmm1, xmmword ptr [r9]\n"
    //     "subps xmm0, xmm1\n"
    //     "movups xmmword ptr [r10], xmm0\n"
    //     : [r] "+r" (r), [a] "+r" (a), [b] "+r" (b) :
    //     : "rax", "r10", "r11", "xmm0", "xmm1", "memory"
    // );
    __m128 va = _mm_loadu_ps(a->comps);
    __m128 vb = _mm_loadu_ps(b->comps);
    va = _mm_sub_ps(va, vb);
    _mm_storeu_ps(r->comps, va);
}

void vec4_t_normalize(vec4_t *r, vec4_t *v)
{
    float length = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);

    if(length != 0)
    {
        *r = (vec4_t){v->x / length, v->y / length, v->z / length, v->w / length};
    }
}

// void vec4_t_scale(vec4_t *r, vec4_t *v, float s)
// {
//     // *r = (vec4_t){v->x * s, v->y * s, v->z * s, v->w * s};
//     // __asm__ volatile
//     // (
//     //     "movss xmm0, dword ptr %[float_s]\n"
//     //     "mov r9, %[vec_v]\n"
//     //     "movups xmm1, xmmword ptr [r9]\n"
//     //     "shufps xmm0, xmm0, 0x00\n"
//     //     "mov r8, %[vec_r]\n"
//     //     "mulps xmm1, xmm0\n"
//     //     "movups xmmword ptr [r8], xmm1\n"
//     //     : [vec_r] "+rm" (r), [vec_v] "+rm" (v)
//     //     : [float_s] "m" (s)
//     //     : "r8", "r9", "xmm0", "xmm1", "memory"
//     // );

//     r->vec = _mm_mul_ps(v->vec, _mm_broadcast_ss(&s));
// }

float vec4_t_dot(vec4_t *a, vec4_t *b)
{
    // return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
    float result;
    __m128 va = _mm_loadu_ps(a->comps);
    __m128 vb = _mm_loadu_ps(b->comps);
    va = _mm_mul_ps(va, vb);
    va = _mm_hadd_ps(va, va);
    va = _mm_hadd_ps(va, va);
    _mm_store_ss(&result, va);
    return result;
}

void vec4_t_max(vec4_t *r, vec4_t *a, vec4_t *b)
{
    // __asm__ volatile
    // (
    //     "mov r8, %[vec_a]\n"
    //     "mov r9, %[vec_b]\n"
    //     "movaps xmm0, xmmword ptr [r8]\n"
    //     "movaps xmm1, xmmword ptr [r9]\n"
    //     "mov r8, %[vec_r]\n"
    //     "maxps xmm0, xmm1\n"
    //     "movaps xmmword ptr [r8], xmm0\n"
    //     : [vec_a] "+rm" (a), [vec_b] "+rm" (b), [vec_r] "+rm" (r) :
    //     : "r8", "r9", "xmm0", "xmm1", "memory"
    // );
    __m128 va = _mm_loadu_ps(a->comps);
    __m128 vb = _mm_loadu_ps(b->comps);
    va = _mm_max_ps(va, vb);
    _mm_storeu_ps(r->comps, va);
}

void vec4_t_min(vec4_t *r, vec4_t *a, vec4_t *b)
{
    // __asm__ volatile
    // (
    //     "mov r8, %[vec_a]\n"
    //     "mov r9, %[vec_b]\n"
    //     "movaps xmm0, xmmword ptr [r8]\n"
    //     "movaps xmm1, xmmword ptr [r9]\n"
    //     "mov r8, %[vec_r]\n"
    //     "minps xmm0, xmm1\n"
    //     "movaps xmmword ptr [r8], xmm0\n"
    //     : [vec_a] "+rm" (a), [vec_b] "+rm" (b), [vec_r] "+rm" (r) :
    //     : "r8", "r9", "xmm0", "xmm1", "memory"
    // );
    __m128 va = _mm_loadu_ps(a->comps);
    __m128 vb = _mm_loadu_ps(b->comps);
    va = _mm_min_ps(va, vb);
    _mm_storeu_ps(r->comps, va);
}

// void vec4_t_fmadd(vec4_t *r, vec4_t *a, vec4_t *b, float s)
// {
//     __asm__ volatile
//     (
//         "movss  xmm3, dword ptr [%3]\n"
//         "shufps xmm3, xmm3, 0x00\n"
//         "movups xmm1, xmmword ptr [%0]\n"
//         "movups xmm2, xmmword ptr [%1]\n"
//         "vfmadd231ps xmm1, xmm2, xmm3\n"
//         "movups xmmword ptr [%2], xmm1\n"
//         : "+rm" (a), "+rm" (b), "+rm" (r)
//         : "m" (s)
//         : "xmm1", "xmm2", "xmm3", "memory"
//     );
// }

void quat_t_vec3_t_rotate(vec3_t *r, quat_t *quat, vec3_t *v)
{
    quat_t inverse;
    quat_t_conj(&inverse, quat);
    quat_t result = {.vec3 = *v, .s = 0.0f};
    quat_t_mult(&result, &result, &inverse);
    quat_t_mult(&result, quat, &result);
    *r = result.vec3;
}

void quat_t_vec3_t_rotate_inverse(vec3_t *r, quat_t *quat, vec3_t *v)
{
    quat_t inverse;
    quat_t_conj(&inverse, quat);
    quat_t result = {.vec3 = *v, .s = 0.0f};
    quat_t_mult(&result, &result, quat);
    quat_t_mult(&result, &inverse, &result);
    *r = result.vec3;
}

void quat_t_mult(quat_t *r, quat_t *a, quat_t *b)
{
    *r = (quat_t){
        .x = a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y,
        .y = a->w * b->y + a->y * b->w + a->z * b->x - a->x * b->z,
        .z = a->w * b->z + a->z * b->w + a->x * b->y - a->y * b->x,
        .w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z
    };
}

void quat_t_conj(quat_t *r, quat_t *q)
{
    *r = (quat_t){
        .x = -q->x,
        .y = -q->y,
        .z = -q->z,
        .w =  q->w,
    };
}

void quat_slerp(vec4_t *r, vec4_t *a, vec4_t *b, float t)
{
    /*
        implementation copied from: http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
    */
    float cos_half_theta = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
    vec4_t tb;

    if(cos_half_theta < 0.0)
    {
        tb.x = -b->x;
        tb.y = -b->y;
        tb.z = -b->z;
        tb.w = -b->w;
        b = &tb;

        cos_half_theta = -cos_half_theta;
    }

    if(t <= 0.0f)
    {
        *r = *a;
        return;
    }
    else if(t >= 1.0f || fabs(cos_half_theta) >= 1.0)
    {
        *r = *b;
        return;
    }

    // if(fabs(cos_half_theta) >= 1.0)
    // {
    //     *r = *b;
    //     return;
    // }

    float half_theta = acosf(cos_half_theta);
    float sin_half_theta = sqrtf(1.0 - cos_half_theta * cos_half_theta);

    if(fabs(sin_half_theta) < 0.001)
    {
        r->x = a->x * 0.5 + b->x * 0.5;
        r->y = a->y * 0.5 + b->y * 0.5;
        r->z = a->z * 0.5 + b->z * 0.5;
        r->w = a->w * 0.5 + b->w * 0.5;
        return;
    }

    float ratio_a = sinf((1.0 - t) * half_theta) / sin_half_theta;
    float ratio_b = sinf(t * half_theta) / sin_half_theta;

    r->x = a->x * ratio_a + b->x * ratio_b;
    r->y = a->y * ratio_a + b->y * ratio_b;
    r->z = a->z * ratio_a + b->z * ratio_b;
    r->w = a->w * ratio_a + b->w * ratio_b;

    return;
}

/*================== mat2_t ================== */

void mat2_t_mul(mat2_t *r, mat2_t *a, mat2_t *b)
{
    *r = (mat2_t){
        .rows[0] = {
            .x = a->rows[0].x * b->rows[0].x + a->rows[0].y * b->rows[1].x,
            .y = a->rows[0].x * b->rows[0].y + a->rows[0].y * b->rows[1].y,
        },
        .rows[1] = {
            .x = a->rows[1].x * b->rows[0].x + a->rows[1].y * b->rows[1].x,
            .y = a->rows[1].x * b->rows[0].y + a->rows[1].y * b->rows[1].y,
        },
    };
}

void mat2_t_vec2_t_mul(vec2_t *r, vec2_t *v, mat2_t *m)
{
    *r = (vec2_t){
        .x = v->x * m->rows[0].x + v->y * m->rows[1].x,
        .y = v->x * m->rows[0].y + v->y * m->rows[1].y,
    };
}

/*================== mat3_t ================== */

// void mat3_t_id(mat3_t *m)
// {
//     m->rows[0] = (vec3_t){1.0f, 0.0f, 0.0f};
//     m->rows[1] = (vec3_t){0.0f, 1.0f, 0.0f};
//     m->rows[2] = (vec3_t){0.0f, 0.0f, 1.0f};
// }

void mat3_t_transpose(mat3_t *r, mat3_t *m)
{
    float temp = m->rows[1].x;
    m->rows[1].x = m->rows[0].y;
    m->rows[0].y = temp;

    temp = m->rows[2].x;
    m->rows[2].x = m->rows[0].z;
    m->rows[0].z = temp;

    temp = m->rows[2].y;
    m->rows[2].y = m->rows[1].z;
    m->rows[1].z = temp;
}

void mat3_t_mul(mat3_t *r, mat3_t *a, mat3_t *b)
{
    *r = (mat3_t){
        .rows[0] = (vec3_t){a->rows[0].x * b->rows[0].x + a->rows[0].y * b->rows[1].x + a->rows[0].z * b->rows[2].x,
                            a->rows[0].x * b->rows[0].y + a->rows[0].y * b->rows[1].y + a->rows[0].z * b->rows[2].y,
                            a->rows[0].x * b->rows[0].z + a->rows[0].y * b->rows[1].z + a->rows[0].z * b->rows[2].z},

        .rows[1] = (vec3_t){a->rows[1].x * b->rows[0].x + a->rows[1].y * b->rows[1].x + a->rows[2].z * b->rows[2].x,
                            a->rows[1].x * b->rows[0].y + a->rows[1].y * b->rows[1].y + a->rows[2].z * b->rows[2].y,
                            a->rows[1].x * b->rows[0].z + a->rows[1].y * b->rows[1].z + a->rows[2].z * b->rows[2].z},

        .rows[2] = (vec3_t){a->rows[2].x * b->rows[0].x + a->rows[2].y * b->rows[1].x + a->rows[2].z * b->rows[2].x,
                            a->rows[2].x * b->rows[0].y + a->rows[2].y * b->rows[1].y + a->rows[2].z * b->rows[2].y,
                            a->rows[2].x * b->rows[0].z + a->rows[2].y * b->rows[1].z + a->rows[2].z * b->rows[2].z}
    };
}

void mat3_t_rotx(mat3_t *r, float turns)
{
    float s = sinf(turns * M_PI * 2.0f);
    float c = cosf(turns * M_PI * 2.0f);
    *r = (mat3_t){
        1.0f, 0.0f, 0.0f,
        0.0f,    c,    s,
        0.0f,   -s,    c,
    };
}

void mat3_t_rotx_mul(mat3_t *r, float turns)
{
    mat3_t rot;
    mat3_t_rotx(&rot, turns);
    mat3_t_mul(r, r, &rot);
}

void mat3_t_roty(mat3_t *r, float turns)
{
    float s = sinf(turns * M_PI * 2.0f);
    float c = cosf(turns * M_PI * 2.0f);
    *r = (mat3_t){
        c,    0.0f,   -s,
        0.0f, 1.0f, 0.0f,
        s,    0.0f,    c,
    };
}

void mat3_t_roty_mul(mat3_t *r, float turns)
{
    mat3_t rot;
    mat3_t_roty(&rot, turns);
    mat3_t_mul(r, r, &rot);
}

void mat3_t_rotz(mat3_t *r, float turns)
{
    float s = sinf(turns * M_PI * 2.0f);
    float c = cosf(turns * M_PI * 2.0f);
    *r = (mat3_t){
        c,       s,    0.0f,
       -s,       c,    0.0f,
        0.0f,  0.0,    1.0f,
    };
}

void mat3_t_rotz_mul(mat3_t *r, float turns)
{
    mat3_t rot;
    mat3_t_rotz(&rot, turns);
    mat3_t_mul(r, r, &rot);
}

void mat3_t_vec3_t_mul(vec3_t *r, vec3_t *v, mat3_t *m)
{
    *r = (vec3_t){
        v->x * m->rows[0].x + v->y * m->rows[1].x + v->z * m->rows[2].x,
        v->x * m->rows[0].y + v->y * m->rows[1].y + v->z * m->rows[2].y,
        v->x * m->rows[0].z + v->y * m->rows[1].z + v->z * m->rows[2].z
    };
}

void mat3_t_from_quat_t(mat3_t *r, quat_t *q)
{
    r->rows[0] = (vec3_t){
        .x = 1.0f - 2.0f * (q->z * q->z + q->y * q->y),
        .y = 2.0f * (q->x * q->y + q->w * q->z),
        .z = 2.0f * (q->z * q->x - q->w * q->y)  
    };

    r->rows[1] = (vec3_t){
        .x = 2.0f * (q->x * q->y - q->w * q->z),
        .y = 1.0f - 2.0f * (q->x * q->x + q->z * q->z),
        .z = 2.0f * (q->y * q->z + q->w * q->x)
    };

    r->rows[2] = (vec3_t){
        .x = 2.0f * (q->z * q->x + q->w * q->y),
        .y = 2.0f * (q->y * q->z - q->w * q->x),
        .z = 1.0f - 2.0f * (q->x * q->x + q->y * q->y)
    };
}

/*================== mat4_t ================== */
void mat4_t_id(mat4_t *m)
{
    m->rows[0] = (vec4_t){1.0f, 0.0f, 0.0f, 0.0f};
    m->rows[1] = (vec4_t){0.0f, 1.0f, 0.0f, 0.0f};
    m->rows[2] = (vec4_t){0.0f, 0.0f, 1.0f, 0.0f};
    m->rows[3] = (vec4_t){0.0f, 0.0f, 0.0f, 1.0f};
}

void mat4_t_transpose(mat4_t *r, mat4_t *m)
{
    if(r != m)
    {
        *r = *m;
    }

    float temp = r->rows[1].x;
    r->rows[1].x = r->rows[0].y;
    r->rows[0].y = temp;

    temp = r->rows[2].x;
    r->rows[2].x = r->rows[0].z;
    r->rows[0].z = temp;

    temp = r->rows[2].y;
    r->rows[2].y = r->rows[1].z;
    r->rows[1].z = temp;

    temp = r->rows[3].x;
    r->rows[3].x = r->rows[0].w;
    r->rows[0].w = temp;

    temp = r->rows[3].y;
    r->rows[3].y = r->rows[1].w;
    r->rows[1].w = temp;

    temp = r->rows[3].z;
    r->rows[3].z = r->rows[2].w;
    r->rows[2].w = temp;
}

void mat4_t_mul(mat4_t *r, mat4_t *a, mat4_t *b)
{
    __asm__ volatile
    (
        "mov r8, %[mat_b]\n"

        "movaps xmm1, xmmword ptr [r8 + 0 ]\n"
        "movaps xmm2, xmmword ptr [r8 + 16]\n"
        "movaps xmm3, xmmword ptr [r8 + 32]\n"
        "movaps xmm4, xmmword ptr [r8 + 48]\n"
        
        "mov r8, %[mat_a]\n"
        "mov r9, %[mat_r]\n"

        "movaps xmm0, xmmword ptr [r8]\n"
        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0x00\n"
        "shufps xmm7, xmm7, 0x55\n"
        "mulps xmm5, xmm1\n"
        "mulps xmm7, xmm2\n"
        "movaps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"
        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0xaa\n"
        "shufps xmm7, xmm7, 0xff\n"
        "movaps xmm0, xmmword ptr [r8 + 16]\n"
        "mulps xmm5, xmm3\n"
        "mulps xmm7, xmm4\n"
        "addps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"

        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0x00\n"
        "movaps xmmword ptr [r9], xmm6\n"
        "shufps xmm7, xmm7, 0x55\n"
        "mulps xmm5, xmm1\n"
        "mulps xmm7, xmm2\n"
        "movaps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"
        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0xaa\n"
        "shufps xmm7, xmm7, 0xff\n"
        "movaps xmm0, xmmword ptr [r8 + 32]\n"
        "mulps xmm5, xmm3\n"
        "mulps xmm7, xmm4\n"
        "addps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"

        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0x00\n"
        "movaps xmmword ptr [r9 + 16], xmm6\n"
        "shufps xmm7, xmm7, 0x55\n"
        "mulps xmm5, xmm1\n"
        "mulps xmm7, xmm2\n"
        "movaps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"
        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0xaa\n"
        "shufps xmm7, xmm7, 0xff\n"
        "movaps xmm0, xmmword ptr [r8 + 48]\n"
        "mulps xmm5, xmm3\n"
        "mulps xmm7, xmm4\n"
        "addps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"

        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0x00\n"
        "movaps xmmword ptr [r9 + 32], xmm6\n"
        "shufps xmm7, xmm7, 0x55\n"
        "mulps xmm5, xmm1\n"
        "mulps xmm7, xmm2\n"
        "movaps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"
        "movaps xmm5, xmm0\n"
        "movaps xmm7, xmm0\n"
        "shufps xmm5, xmm5, 0xaa\n"
        "shufps xmm7, xmm7, 0xff\n"
        "mulps xmm5, xmm3\n"
        "mulps xmm7, xmm4\n"
        "addps xmm6, xmm5\n"
        "addps xmm6, xmm7\n"
        "movaps xmmword ptr [r9 + 48], xmm6\n"       

        : [mat_r] "+rm" (r), [mat_a] "+rm" (a), [mat_b] "+rm" (b) :
        : "r8", "r9", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "memory"
    );
}

void mat4_t_rotx(mat4_t *r, float turns)
{
    float s = sinf(turns * M_PI * 2.0f);
    float c = cosf(turns * M_PI * 2.0f);
    *r = (mat4_t){
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,    c,    s, 0.0f,
        0.0f,   -s,    c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
}

void mat4_t_rotx_mul(mat4_t *r, float turns)
{
    mat4_t rot;
    mat4_t_rotx(&rot, turns);
    mat4_t_mul(r, r, &rot);
}

void mat4_t_roty(mat4_t *r, float turns)
{
    float s = sinf(turns * M_PI * 2.0f);
    float c = cosf(turns * M_PI * 2.0f);
    *r = (mat4_t){
        c,    0.0f,   -s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s,    0.0f,    c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
}

void mat4_t_roty_mul(mat4_t *r, float turns)
{
    mat4_t rot;
    mat4_t_roty(&rot, turns);
    mat4_t_mul(r, r, &rot);
}

void mat4_t_rotz(mat4_t *r, float turns)
{
    float s = sinf(turns * M_PI * 2.0f);
    float c = cosf(turns * M_PI * 2.0f);
    *r = (mat4_t){
        c,       s,    0.0f, 0.0f,
       -s,       c,    0.0f, 0.0f,
        0.0f,  0.0,    1.0f, 0.0f,
        0.0f, 0.0f,    0.0f, 1.0f,
    };
}

void mat4_t_rotz_mul(mat4_t *r, float turns)
{
    mat4_t rot;
    mat4_t_rotz(&rot, turns);
    mat4_t_mul(r, r, &rot);
}

void mat4_t_invvm(mat4_t *r, mat4_t *m)
{
    mat4_t_transpose(r, m);
    /* -translation * inverse_rotation. Matrix is transposed, so translation is in the last column. */
    r->rows[3].x = -r->rows[0].w * r->rows[0].x - r->rows[1].w * r->rows[1].x - r->rows[2].w * r->rows[2].x;
    r->rows[3].y = -r->rows[0].w * r->rows[0].y - r->rows[1].w * r->rows[1].y - r->rows[2].w * r->rows[2].y;
    r->rows[3].z = -r->rows[0].w * r->rows[0].z - r->rows[1].w * r->rows[1].z - r->rows[2].w * r->rows[2].z;

    r->rows[0].w = 0.0f;
    r->rows[1].w = 0.0f;
    r->rows[2].w = 0.0f;
}

void mat4_t_ortho(mat4_t *r, float width, float height, float z_near, float z_far)
{
    *r = (mat4_t) {
        2.0f / width,  0.0f,          0.0f,                        0.0f,
        0.0f,          2.0f / height, 0.0f,                        0.0f,
        0.0f,          0.0f,         -2.0 / (z_far - z_near),      0.0f,
        0.0f,          0.0f, -(z_far + z_near) / (z_far - z_near), 1.0f,
    };
}

void mat4_t_persp(mat4_t* m, float fov_y, float aspect, float z_near, float z_far)
{
    float t = tanf(fov_y) * z_near;
    float r = t * aspect;

    mat4_t_id(m);

    m->rows[0].x = z_near / r;
    m->rows[1].y = z_near / t;
    m->rows[2].z = (-z_far + z_near) / (z_far - z_near);
    m->rows[2].w = -1.0;
    m->rows[3].z = -(2.0 * z_near * z_far) / (z_far - z_near);
    m->rows[3].w = 0.0;
}

void mat4_t_proj_planes(mat4_t *m, vec4_t *planes)
{
    /*
        https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
    */

    /* -X */
    planes[0].x = m->rows[0].w + m->rows[0].x;
    planes[0].y = m->rows[1].w + m->rows[1].x;
    planes[0].z = m->rows[2].w + m->rows[2].x;
    planes[0].w = m->rows[3].w + m->rows[3].x;
    /* +X */
    planes[1].x = m->rows[0].w - m->rows[0].x;
    planes[1].y = m->rows[1].w - m->rows[1].x;
    planes[1].z = m->rows[2].w - m->rows[2].x;
    planes[1].w = m->rows[3].w - m->rows[3].x;
    /* +Y */
    planes[2].x = m->rows[0].w + m->rows[0].y;
    planes[2].y = m->rows[1].w + m->rows[1].y;
    planes[2].z = m->rows[2].w + m->rows[2].y;
    planes[2].w = m->rows[3].w + m->rows[3].y;
    /* -Y */
    planes[3].x = m->rows[0].w - m->rows[0].y;
    planes[3].y = m->rows[1].w - m->rows[1].y;
    planes[3].z = m->rows[2].w - m->rows[2].y;
    planes[3].w = m->rows[3].w - m->rows[3].y;
    /* near */
    planes[4].x = m->rows[0].w + m->rows[0].z;
    planes[4].y = m->rows[1].w + m->rows[1].z;
    planes[4].z = m->rows[2].w + m->rows[2].z;
    planes[4].w = m->rows[3].w + m->rows[3].z;

    for(uint32_t index = 0; index < 5; index++)
    {
        float normal_length = vec3_t_length(&planes[index].vec3);
        vec4_t_scale(&planes[index], &planes[index], 1.0f / normal_length);
    }
}

// void mat4_t_vec4_t_mul(vec4_t *r, vec4_t *v, mat4_t *m)
// {
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

// void mat4_t_vec4_t_mul(vec4_t *r, vec4_t *v, mat4_t *m)
// {
//     // __asm__ volatile
//     // (
//     //     "mov r8, %[mat_m]\n"

//     //     "movaps xmm1, xmmword ptr [r8 + 0 ]\n"
//     //     "movaps xmm2, xmmword ptr [r8 + 16]\n"
//     //     "movaps xmm3, xmmword ptr [r8 + 32]\n"
//     //     "movaps xmm4, xmmword ptr [r8 + 48]\n"
        
//     //     "mov r8, %[vec_v]\n"
//     //     "mov r9, %[vec_r]\n"

//     //     "movaps xmm0, xmmword ptr [r8]\n"
//     //     "movaps xmm5, xmm0\n"
//     //     "movaps xmm7, xmm0\n"
//     //     "shufps xmm5, xmm5, 0x00\n"
//     //     "shufps xmm7, xmm7, 0x55\n"
//     //     "mulps xmm5, xmm1\n"
//     //     "mulps xmm7, xmm2\n"
//     //     "movaps xmm6, xmm5\n"
//     //     "addps xmm6, xmm7\n"
//     //     "movaps xmm5, xmm0\n"
//     //     "movaps xmm7, xmm0\n"
//     //     "shufps xmm5, xmm5, 0xaa\n"
//     //     "shufps xmm7, xmm7, 0xff\n"
//     //     "mulps xmm5, xmm3\n"
//     //     "mulps xmm7, xmm4\n"
//     //     "addps xmm6, xmm5\n"
//     //     "addps xmm6, xmm7\n"
//     //     "movaps xmmword ptr [r9], xmm6\n"

//     //     : [vec_r] "+rm" (r), [vec_v] "+rm" (v), [mat_m] "+rm" (m) :
//     //     : "r8", "r9", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "memory"
//     // );

//     // __m128 row0 = _mm_mul_ps(m->rows[0].vec, _mm_shuffle_ps(v->vec, v->vec, 0x00));
//     // __m128 row1 = _mm_mul_ps(m->rows[1].vec, _mm_shuffle_ps(v->vec, v->vec, 0x55));
//     // __m128 row2 = _mm_mul_ps(m->rows[2].vec, _mm_shuffle_ps(v->vec, v->vec, 0xaa));
//     // row0 = _mm_add_ps(row0, row1);
//     // __m128 row3 = _mm_mul_ps(m->rows[3].vec, _mm_shuffle_ps(v->vec, v->vec, 0xff));
//     // row2 = _mm_add_ps(row2, row3);
//     // r->vec = _mm_add_ps(row0, row2);

//     // __m128 row0 = _mm_mul_ps(m->rows[0].vec, _mm_shuffle_ps(v->vec, v->vec, 0x00));
//     // __m128 row1 = _mm_mul_ps(m->rows[1].vec, _mm_shuffle_ps(v->vec, v->vec, 0x55));
//     // __m128 row2 = _mm_mul_ps(m->rows[2].vec, _mm_shuffle_ps(v->vec, v->vec, 0xaa));
//     // row0 = _mm_add_ps(row0, row1);
//     // __m128 row3 = _mm_mul_ps(m->rows[3].vec, _mm_shuffle_ps(v->vec, v->vec, 0xff));
//     // row2 = _mm_add_ps(row2, row3);

//     r->vec = _mm_add_ps(
//                 _mm_add_ps(
//                     _mm_mul_ps(m->rows[0].vec, _mm_shuffle_ps(v->vec, v->vec, 0x00)),
//                     _mm_mul_ps(m->rows[1].vec, _mm_shuffle_ps(v->vec, v->vec, 0x55))
//                 ), 
//                 _mm_add_ps(
//                     _mm_mul_ps(m->rows[2].vec, _mm_shuffle_ps(v->vec, v->vec, 0xaa)),
//                     _mm_mul_ps(m->rows[3].vec, _mm_shuffle_ps(v->vec, v->vec, 0xff))
//                 )
//             );
// }

void mat4_t_comp(mat4_t *r, mat3_t *m, vec3_t *v)
{
    r->rows[0] = (vec4_t){.vec3 = m->rows[0],   .s = 0.0f};
    r->rows[1] = (vec4_t){.vec3 = m->rows[1],   .s = 0.0f};
    r->rows[2] = (vec4_t){.vec3 = m->rows[2],   .s = 0.0f};
    r->rows[3] = (vec4_t){.vec3 = *v,           .s = 1.0f};
}