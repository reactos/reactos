/*
 * Copyright 2015 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "d2d1_private.h"
#include <float.h>

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

#define D2D_FIGURE_FLAG_CLOSED          0x00000001u
#define D2D_FIGURE_FLAG_HOLLOW          0x00000002u

#define D2D_CDT_EDGE_FLAG_FREED         0x80000000u
#define D2D_CDT_EDGE_FLAG_VISITED(r)    (1u << (r))

#define D2D_FP_EPS (1.0f / (1 << FLT_MANT_DIG))

static const D2D1_MATRIX_3X2_F identity =
{{{
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,
}}};

enum d2d_cdt_edge_next
{
    D2D_EDGE_NEXT_ORIGIN = 0,
    D2D_EDGE_NEXT_ROT = 1,
    D2D_EDGE_NEXT_SYM = 2,
    D2D_EDGE_NEXT_TOR = 3,
};

enum d2d_vertex_type
{
    D2D_VERTEX_TYPE_NONE,
    D2D_VERTEX_TYPE_LINE,
    D2D_VERTEX_TYPE_BEZIER,
    D2D_VERTEX_TYPE_SPLIT_BEZIER,
    D2D_VERTEX_TYPE_END,
};

struct d2d_segment_idx
{
    size_t figure_idx;
    size_t vertex_idx;
    size_t control_idx;
};

struct d2d_figure
{
    D2D1_POINT_2F *vertices;
    size_t vertices_size;
    enum d2d_vertex_type *vertex_types;
    size_t vertex_types_size;
    size_t vertex_count;

    D2D1_POINT_2F *bezier_controls;
    size_t bezier_controls_size;
    size_t bezier_control_count;

    D2D1_POINT_2F *original_bezier_controls;
    size_t original_bezier_controls_size;
    size_t original_bezier_control_count;

    D2D1_RECT_F bounds;
    unsigned int flags;
};

struct d2d_cdt_edge_ref
{
    size_t idx;
    enum d2d_cdt_edge_next r;
};

struct d2d_cdt_edge
{
    struct d2d_cdt_edge_ref next[4];
    size_t vertex[2];
    unsigned int flags;
};

struct d2d_cdt
{
    struct d2d_cdt_edge *edges;
    size_t edges_size;
    size_t edge_count;
    size_t free_edge;

    const D2D1_POINT_2F *vertices;
};

struct d2d_geometry_intersection
{
    size_t figure_idx;
    size_t vertex_idx;
    size_t control_idx;
    float t;
    D2D1_POINT_2F p;
};

struct d2d_geometry_intersections
{
    struct d2d_geometry_intersection *intersections;
    size_t intersections_size;
    size_t intersection_count;
};

struct d2d_fp_two_vec2
{
    float x[2];
    float y[2];
};

struct d2d_fp_fin
{
    float *now, *other;
    size_t length;
};

static void d2d_curve_vertex_set(struct d2d_curve_vertex *b,
        const D2D1_POINT_2F *p, float u, float v, float sign)
{
    b->position = *p;
    b->texcoord.u = u;
    b->texcoord.v = v;
    b->texcoord.sign = sign;
}

static void d2d_face_set(struct d2d_face *f, UINT16 v0, UINT16 v1, UINT16 v2)
{
    f->v[0] = v0;
    f->v[1] = v1;
    f->v[2] = v2;
}

static void d2d_outline_vertex_set(struct d2d_outline_vertex *v, float x, float y,
        float prev_x, float prev_y, float next_x, float next_y)
{
    d2d_point_set(&v->position, x, y);
    d2d_point_set(&v->prev, prev_x, prev_y);
    d2d_point_set(&v->next, next_x, next_y);
}

static void d2d_curve_outline_vertex_set(struct d2d_curve_outline_vertex *a, const D2D1_POINT_2F *position,
        const D2D1_POINT_2F *p0, const D2D1_POINT_2F *p1, const D2D1_POINT_2F *p2,
        float prev_x, float prev_y, float next_x, float next_y)
{
    a->position = *position;
    a->p0 = *p0;
    a->p1 = *p1;
    a->p2 = *p2;
    d2d_point_set(&a->prev, prev_x, prev_y);
    d2d_point_set(&a->next, next_x, next_y);
}

static void d2d_fp_two_sum(float *out, float a, float b)
{
    float a_virt, a_round, b_virt, b_round;

    out[1] = a + b;
    b_virt = out[1] - a;
    a_virt = out[1] - b_virt;
    b_round = b - b_virt;
    a_round = a - a_virt;
    out[0] = a_round + b_round;
}

static void d2d_fp_fast_two_sum(float *out, float a, float b)
{
    float b_virt;

    out[1] = a + b;
    b_virt = out[1] - a;
    out[0] = b - b_virt;
}

static void d2d_fp_two_two_sum(float *out, const float *a, const float *b)
{
    float sum[2];

    d2d_fp_two_sum(out, a[0], b[0]);
    d2d_fp_two_sum(sum, a[1], out[1]);
    d2d_fp_two_sum(&out[1], sum[0], b[1]);
    d2d_fp_two_sum(&out[2], sum[1], out[2]);
}

static void d2d_fp_two_diff_tail(float *out, float a, float b, float x)
{
    float a_virt, a_round, b_virt, b_round;

    b_virt = a - x;
    a_virt = x + b_virt;
    b_round = b_virt - b;
    a_round = a - a_virt;
    *out = a_round + b_round;
}

static void d2d_fp_two_two_diff(float *out, const float *a, const float *b)
{
    float sum[2], diff;

    diff = a[0] - b[0];
    d2d_fp_two_diff_tail(out, a[0], b[0], diff);
    d2d_fp_two_sum(sum, a[1], diff);
    diff = sum[0] - b[1];
    d2d_fp_two_diff_tail(&out[1], sum[0], b[1], diff);
    d2d_fp_two_sum(&out[2], sum[1], diff);
}

static void d2d_fp_split(float *out, float a)
{
    float a_big, c;

    c = a * ((1 << (FLT_MANT_DIG / 2)) + 1.0f);
    a_big = c - a;
    out[1] = c - a_big;
    out[0] = a - out[1];
}

static void d2d_fp_two_product_presplit(float *out, float a, float b, const float *b_split)
{
    float a_split[2], err1, err2, err3;

    out[1] = a * b;
    d2d_fp_split(a_split, a);
    err1 = out[1] - (a_split[1] * b_split[1]);
    err2 = err1 - (a_split[0] * b_split[1]);
    err3 = err2 - (a_split[1] * b_split[0]);
    out[0] = (a_split[0] * b_split[0]) - err3;
}

static void d2d_fp_two_product(float *out, float a, float b)
{
    float b_split[2];

    d2d_fp_split(b_split, b);
    d2d_fp_two_product_presplit(out, a, b, b_split);
}

static void d2d_fp_square(float *out, float a)
{
    float a_split[2], err1, err2;

    out[1] = a * a;
    d2d_fp_split(a_split, a);
    err1 = out[1] - (a_split[1] * a_split[1]);
    err2 = err1 - ((a_split[1] + a_split[1]) * a_split[0]);
    out[0] = (a_split[0] * a_split[0]) - err2;
}

static float d2d_fp_estimate(float *a, size_t len)
{
    float out = a[0];
    size_t idx = 1;

    while (idx < len)
        out += a[idx++];

    return out;
}

static void d2d_fp_fast_expansion_sum_zeroelim(float *out, size_t *out_len,
        const float *a, size_t a_len, const float *b, size_t b_len)
{
    float sum[2], q, a_curr, b_curr;
    size_t a_idx, b_idx, out_idx;

    a_curr = a[0];
    b_curr = b[0];
    a_idx = b_idx = 0;
    if ((b_curr > a_curr) == (b_curr > -a_curr))
    {
        q = a_curr;
        a_curr = a[++a_idx];
    }
    else
    {
        q = b_curr;
        b_curr = b[++b_idx];
    }
    out_idx = 0;
    if (a_idx < a_len && b_idx < b_len)
    {
        if ((b_curr > a_curr) == (b_curr > -a_curr))
        {
            d2d_fp_fast_two_sum(sum, a_curr, q);
            a_curr = a[++a_idx];
        }
        else
        {
            d2d_fp_fast_two_sum(sum, b_curr, q);
            b_curr = b[++b_idx];
        }
        if (sum[0] != 0.0f)
            out[out_idx++] = sum[0];
        q = sum[1];
        while (a_idx < a_len && b_idx < b_len)
        {
            if ((b_curr > a_curr) == (b_curr > -a_curr))
            {
                d2d_fp_two_sum(sum, q, a_curr);
                a_curr = a[++a_idx];
            }
            else
            {
                d2d_fp_two_sum(sum, q, b_curr);
                b_curr = b[++b_idx];
            }
            if (sum[0] != 0.0f)
                out[out_idx++] = sum[0];
            q = sum[1];
        }
    }
    while (a_idx < a_len)
    {
        d2d_fp_two_sum(sum, q, a_curr);
        a_curr = a[++a_idx];
        if (sum[0] != 0.0f)
            out[out_idx++] = sum[0];
        q = sum[1];
    }
    while (b_idx < b_len)
    {
        d2d_fp_two_sum(sum, q, b_curr);
        b_curr = b[++b_idx];
        if (sum[0] != 0.0f)
            out[out_idx++] = sum[0];
        q = sum[1];
    }
    if (q != 0.0f || !out_idx)
        out[out_idx++] = q;

    *out_len = out_idx;
}

static void d2d_fp_scale_expansion_zeroelim(float *out, size_t *out_len, const float *a, size_t a_len, float b)
{
    float product[2], sum[2], b_split[2], q[2], a_curr;
    size_t a_idx, out_idx;

    d2d_fp_split(b_split, b);
    d2d_fp_two_product_presplit(q, a[0], b, b_split);
    out_idx = 0;
    if (q[0] != 0.0f)
        out[out_idx++] = q[0];
    for (a_idx = 1; a_idx < a_len; ++a_idx)
    {
        a_curr = a[a_idx];
        d2d_fp_two_product_presplit(product, a_curr, b, b_split);
        d2d_fp_two_sum(sum, q[1], product[0]);
        if (sum[0] != 0.0f)
            out[out_idx++] = sum[0];
        d2d_fp_fast_two_sum(q, product[1], sum[1]);
        if (q[0] != 0.0f)
            out[out_idx++] = q[0];
    }
    if (q[1] != 0.0f || !out_idx)
        out[out_idx++] = q[1];

    *out_len = out_idx;
}

static void d2d_point_subtract(D2D1_POINT_2F *out,
        const D2D1_POINT_2F *a, const D2D1_POINT_2F *b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
}

static void d2d_point_scale(D2D1_POINT_2F *p, float scale)
{
    p->x *= scale;
    p->y *= scale;
}

static void d2d_point_lerp(D2D1_POINT_2F *out,
        const D2D1_POINT_2F *a, const D2D1_POINT_2F *b, float t)
{
    out->x = a->x * (1.0f - t) + b->x * t;
    out->y = a->y * (1.0f - t) + b->y * t;
}

static void d2d_point_calculate_bezier(D2D1_POINT_2F *out, const D2D1_POINT_2F *p0,
        const D2D1_POINT_2F *p1, const D2D1_POINT_2F *p2, float t)
{
    float t_c = 1.0f - t;

    out->x = t_c * (t_c * p0->x + t * p1->x) + t * (t_c * p1->x + t * p2->x);
    out->y = t_c * (t_c * p0->y + t * p1->y) + t * (t_c * p1->y + t * p2->y);
}

static float d2d_point_length(const D2D1_POINT_2F *p)
{
    return sqrtf(d2d_point_dot(p, p));
}

static void d2d_point_normalise(D2D1_POINT_2F *p)
{
    float l;

    if ((l = d2d_point_length(p)) != 0.0f)
        d2d_point_scale(p, 1.0f / l);
}

static BOOL d2d_vertex_type_is_bezier(enum d2d_vertex_type t)
{
    return (t == D2D_VERTEX_TYPE_BEZIER || t == D2D_VERTEX_TYPE_SPLIT_BEZIER);
}

static BOOL d2d_vertex_type_is_split_bezier(enum d2d_vertex_type t)
{
    return t == D2D_VERTEX_TYPE_SPLIT_BEZIER;
}

/* This implementation is based on the paper "Adaptive Precision
 * Floating-Point Arithmetic and Fast Robust Geometric Predicates" and
 * associated (Public Domain) code by Jonathan Richard Shewchuk. */
static float d2d_point_ccw(const D2D1_POINT_2F *a, const D2D1_POINT_2F *b, const D2D1_POINT_2F *c)
{
    static const float err_bound_result = (3.0f + 8.0f * D2D_FP_EPS) * D2D_FP_EPS;
    static const float err_bound_a = (3.0f + 16.0f * D2D_FP_EPS) * D2D_FP_EPS;
    static const float err_bound_b = (2.0f + 12.0f * D2D_FP_EPS) * D2D_FP_EPS;
    static const float err_bound_c = (9.0f + 64.0f * D2D_FP_EPS) * D2D_FP_EPS * D2D_FP_EPS;
    float det_d[16], det_c2[12], det_c1[8], det_b[4], temp4[4], temp2a[2], temp2b[2], abxacy[2], abyacx[2];
    size_t det_d_len, det_c2_len, det_c1_len;
    float det, det_sum, err_bound;
    struct d2d_fp_two_vec2 ab, ac;

    ab.x[1] = b->x - a->x;
    ab.y[1] = b->y - a->y;
    ac.x[1] = c->x - a->x;
    ac.y[1] = c->y - a->y;

    abxacy[1] = ab.x[1] * ac.y[1];
    abyacx[1] = ab.y[1] * ac.x[1];
    det = abxacy[1] - abyacx[1];

    if (abxacy[1] > 0.0f)
    {
        if (abyacx[1] <= 0.0f)
            return det;
        det_sum = abxacy[1] + abyacx[1];
    }
    else if (abxacy[1] < 0.0f)
    {
        if (abyacx[1] >= 0.0f)
            return det;
        det_sum = -abxacy[1] - abyacx[1];
    }
    else
    {
        return det;
    }

    err_bound = err_bound_a * det_sum;
    if (det >= err_bound || -det >= err_bound)
        return det;

    d2d_fp_two_product(abxacy, ab.x[1], ac.y[1]);
    d2d_fp_two_product(abyacx, ab.y[1], ac.x[1]);
    d2d_fp_two_two_diff(det_b, abxacy, abyacx);

    det = d2d_fp_estimate(det_b, 4);
    err_bound = err_bound_b * det_sum;
    if (det >= err_bound || -det >= err_bound)
        return det;

    d2d_fp_two_diff_tail(&ab.x[0], b->x, a->x, ab.x[1]);
    d2d_fp_two_diff_tail(&ab.y[0], b->y, a->y, ab.y[1]);
    d2d_fp_two_diff_tail(&ac.x[0], c->x, a->x, ac.x[1]);
    d2d_fp_two_diff_tail(&ac.y[0], c->y, a->y, ac.y[1]);

    if (ab.x[0] == 0.0f && ab.y[0] == 0.0f && ac.x[0] == 0.0f && ac.y[0] == 0.0f)
        return det;

    err_bound = err_bound_c * det_sum + err_bound_result * fabsf(det);
    det += (ab.x[1] * ac.y[0] + ac.y[1] * ab.x[0]) - (ab.y[1] * ac.x[0] + ac.x[1] * ab.y[0]);
    if (det >= err_bound || -det >= err_bound)
        return det;

    d2d_fp_two_product(temp2a, ab.x[0], ac.y[1]);
    d2d_fp_two_product(temp2b, ab.y[0], ac.x[1]);
    d2d_fp_two_two_diff(temp4, temp2a, temp2b);
    d2d_fp_fast_expansion_sum_zeroelim(det_c1, &det_c1_len, det_b, 4, temp4, 4);

    d2d_fp_two_product(temp2a, ab.x[1], ac.y[0]);
    d2d_fp_two_product(temp2b, ab.y[1], ac.x[0]);
    d2d_fp_two_two_diff(temp4, temp2a, temp2b);
    d2d_fp_fast_expansion_sum_zeroelim(det_c2, &det_c2_len, det_c1, det_c1_len, temp4, 4);

    d2d_fp_two_product(temp2a, ab.x[0], ac.y[0]);
    d2d_fp_two_product(temp2b, ab.y[0], ac.x[0]);
    d2d_fp_two_two_diff(temp4, temp2a, temp2b);
    d2d_fp_fast_expansion_sum_zeroelim(det_d, &det_d_len, det_c2, det_c2_len, temp4, 4);

    return det_d[det_d_len - 1];
}

/* Determine whether the point q is within the given tolerance of the line
 * segment defined by p0 and p1, with the given stroke width and transform.
 * Note that we don't care about the tolerance with respect to end-points or
 * joins here; those are handled separately. */
static BOOL d2d_point_on_line_segment(const D2D1_POINT_2F *q, const D2D1_POINT_2F *p0,
        const D2D1_POINT_2F *p1, const D2D1_MATRIX_3X2_F *transform, float stroke_width, float tolerance)
{
    D2D1_POINT_2F v_n, v_p, v_q, v_r;
    float l;

    d2d_point_subtract(&v_p, p1, p0);
    if ((l = d2d_point_length(&v_p)) == 0.0f)
        return FALSE;

    /* After (shear) transformation, the line segment is a parallelogram
     * defined by p⃑' and n⃑':
     *
     *   p⃑ = P₁ - P₀
     *   n⃑ = wp̂⟂
     *   p⃑' = p⃑T
     *   n⃑' = n⃑T */
    l = stroke_width / l;
    d2d_point_set(&v_r, transform->_31, transform->_32);
    d2d_point_transform(&v_n, transform, -v_p.y * l, v_p.x * l);
    d2d_point_subtract(&v_n, &v_n, &v_r);
    d2d_point_transform(&v_p, transform, v_p.x, v_p.y);
    d2d_point_subtract(&v_p, &v_p, &v_r);

    /* Decompose the vector q⃑ = Q - P₀T into a linear combination of
     * p⃑' and n⃑':
     *
     *   lq⃑ = xp⃑' + yn⃑' */
    d2d_point_transform(&v_q, transform, p0->x, p0->y);
    d2d_point_subtract(&v_q, q, &v_q);
    l = v_p.x * v_n.y - v_p.y * v_n.x;
    v_r.x = v_q.x * v_n.y - v_q.y * v_n.x;
    v_r.y = v_q.x * v_p.y - v_q.y * v_p.x;

    if (l < 0.0f)
    {
        l *= -1.0f;
        v_r.x *= -1.0f;
    }

    /* Check where Q projects onto p⃑'. */
    if (v_r.x < 0.0f || v_r.x > l)
        return FALSE;

    /* Check where Q projects onto n⃑'. */
    if (fabs(v_r.y) < l)
        return TRUE;

    /* Q lies outside the segment. Check whether the distance to the edge is
     * within the tolerance.
     *
     *   P₀' = P₀T + n⃑'
     *   q⃑' = Q - P₀'
     *      = q⃑ - n⃑'
     *
     * The distance is then q⃑' · p̂'⟂. */

    if (v_r.y > 0.0f)
        d2d_point_scale(&v_n, -1.0f);
    d2d_point_subtract(&v_q, &v_q, &v_n);

    /* Check where Q projects onto p⃑' + n⃑'. */
    l = d2d_point_dot(&v_q, &v_p);
    if (l < 0.0f || l > d2d_point_dot(&v_p, &v_p))
        return FALSE;

    v_n.x = -v_p.y;
    v_n.y = v_p.x;
    d2d_point_normalise(&v_n);

    return fabsf(d2d_point_dot(&v_q, &v_n)) < tolerance;
}

/* Approximate the Bézier segment with a (wide) line segment. If the point
 * lies outside the approximation, we're done. If the width of the
 * approximation is less than the tolerance and the point lies inside, we're
 * also done. If neither of those is the case, we subdivide the Bézier segment
 * and try again. */
static BOOL d2d_point_on_bezier_segment(const D2D1_POINT_2F *q, const D2D1_POINT_2F *p0,
        const D2D1_BEZIER_SEGMENT *b, const D2D1_MATRIX_3X2_F *transform, float stroke_width, float tolerance)
{
    float d1, d2, d3, d4, d, l, m, w, w2;
    D2D1_POINT_2F t[7], start, end, v_p;
    D2D1_BEZIER_SEGMENT b0, b1;

    m = 1.0f;
    w = stroke_width * 0.5f;

    d2d_point_subtract(&v_p, &b->point3, p0);
    /* If the endpoints coincide, use the line through the control points as
     * the direction vector. That choice is somewhat arbitrary; other choices
     * with tighter error bounds exist. */
    if ((l = d2d_point_dot(&v_p, &v_p)) == 0.0f)
    {
        d2d_point_subtract(&v_p, &b->point2, &b->point1);
        /* If the control points also coincide, the curve is in fact a line. */
        if ((l = d2d_point_dot(&v_p, &v_p)) == 0.0f)
        {
            d2d_point_subtract(&v_p, &b->point1, p0);
            end.x = p0->x + 0.75f * v_p.x;
            end.y = p0->y + 0.75f * v_p.y;

            return d2d_point_on_line_segment(q, p0, &end, transform, w, tolerance);
        }
        m = 0.0f;
    }
    l = sqrtf(l);
    d2d_point_scale(&v_p, 1.0f / l);
    m *= l;

    /* Calculate the width w2 of the approximation. */

    end.x = p0->x + v_p.x;
    end.y = p0->y + v_p.y;
    /* Here, d1 and d2 are the maximum (signed) distance of the control points
     * from the line through the start and end points. */
    d1 = d2d_point_ccw(p0, &end, &b->point1);
    d2 = d2d_point_ccw(p0, &end, &b->point2);
    /* It can be shown that if the control points of a cubic Bézier curve lie
     * on the same side of the line through the endpoints, the distance of the
     * curve itself to that line will be within 3/4 of the distance of the
     * control points to that line; if the control points lie on opposite
     * sides, that distance will be within 4/9 of the distance of the
     * corresponding control point. We're taking that as a given here. */
    if (d1 * d2 > 0.0f)
    {
        d1 *= 0.75f;
        d2 *= 0.75f;
    }
    else
    {
        d1 = (d1 * 4.0f) / 9.0f;
        d2 = (d2 * 4.0f) / 9.0f;
    }
    w2 = max(fabsf(d1), fabsf(d2));

    /* Project the control points onto the line through the endpoints of the
     * curve. We will use these to calculate the endpoints of the
     * approximation. */
    d2d_point_subtract(&t[1], &b->point1, p0);
    d1 = d2d_point_dot(&v_p, &t[1]);
    d2d_point_subtract(&t[2], &b->point2, p0);
    d2 = d2d_point_dot(&v_p, &t[2]);

    /* Calculate the start point of the approximation. Like further above, the
     * actual curve is somewhat closer to the endpoints than the control
     * points are. */
    d = min(min(d1, d2), 0);
    if (d1 * d2 > 0.0f)
        d *= 0.75f;
    else
        d = (d * 4.0f) / 9.0f;
    /* Account for the stroke width and tolerance around the endpoints by
     * adjusting the endpoints here. This matters because there are no joins
     * in the original geometry for the places where we subdivide the original
     * curve. We do this here because it's easy; alternatively we could
     * explicitly test for this when subdividing the curve further below. */
    d -= min(w + tolerance, w2);
    start.x = p0->x + d * v_p.x;
    start.y = p0->y + d * v_p.y;

    /* Calculate the end point of the approximation. */
    d1 -= m;
    d2 -= m;
    d = max(max(d1, d2), 0);
    if (d1 * d2 > 0.0f)
        d = m + d * 0.75f;
    else
        d = m + (d * 4.0f) / 9.0f;
    d += min(w2, w + tolerance);
    end.x = p0->x + d * v_p.x;
    end.y = p0->y + d * v_p.y;

    /* Calculate the error bounds of the approximation. We do this in
     * transformed space because we need these to be relative to the given
     * tolerance. */

    d2d_point_transform(&t[0], transform, p0->x, p0->y);
    d2d_point_transform(&t[1], transform, b->point1.x, b->point1.y);
    d2d_point_transform(&t[2], transform, b->point2.x, b->point2.y);
    d2d_point_transform(&t[3], transform, b->point3.x, b->point3.y);
    d2d_point_transform(&t[4], transform, start.x, start.y);
    d2d_point_transform(&t[5], transform, end.x, end.y);

    d2d_point_subtract(&t[6], &t[5], &t[4]);
    l = d2d_point_length(&t[6]);
    /* Here, d1 and d2 are the maximum (signed) distance of the control points
     * from the line through the start and end points. */
    d1 = d2d_point_ccw(&t[4], &t[5], &t[1]) / l;
    d2 = d2d_point_ccw(&t[4], &t[5], &t[2]) / l;
    if (d1 * d2 > 0.0f)
    {
        d1 *= 0.75f;
        d2 *= 0.75f;
    }
    else
    {
        d1 = (d1 * 4.0f) / 9.0f;
        d2 = (d2 * 4.0f) / 9.0f;
    }
    l = max(max(d1, d2), 0) - min(min(d1, d2), 0);

    /* d3 and d4 are the (unsigned) distance of the endpoints of the
     * approximation from the original endpoints. */
    d2d_point_subtract(&t[6], &t[4], &t[0]);
    d3 = d2d_point_length(&t[6]);
    d2d_point_subtract(&t[6], &t[5], &t[3]);
    d4 = d2d_point_length(&t[6]);
    l = max(max(d3, d4), l);

    /* If the error of the approximation is less than the tolerance, and Q
     * lies on the approximation, the distance of Q to the stroked curve is
     * definitely within the tolerance. */
    if (l <= tolerance && d2d_point_on_line_segment(q, &start, &end, transform, w, tolerance - l))
        return TRUE;
    /* On the other hand, if the distance of Q to the stroked curve is more
     * than the sum of the tolerance and d, the distance of Q to the stroked
     * curve can't possibly be within the tolerance. */
    if (!d2d_point_on_line_segment(q, &start, &end, transform, w + w2, tolerance))
        return FALSE;

    /* Subdivide the curve. Note that simply splitting the segment in half
     * here works and is easy, but may not be optimal. We could potentially
     * reduce the number of iterations we need to do by splitting based on
     * curvature or segment length. */
    d2d_point_lerp(&t[0], &b->point1, &b->point2, 0.5f);

    b1.point3 = b->point3;
    d2d_point_lerp(&b1.point2, &b->point3, &b->point2, 0.5f);
    d2d_point_lerp(&b1.point1, &t[0], &b1.point2, 0.5f);

    d2d_point_lerp(&b0.point1, p0, &b->point1, 0.5f);
    d2d_point_lerp(&b0.point2, &t[0], &b0.point1, 0.5f);
    d2d_point_lerp(&b0.point3, &b0.point2, &b1.point1, 0.5f);

    return d2d_point_on_bezier_segment(q, p0, &b0, transform, stroke_width, tolerance)
            || d2d_point_on_bezier_segment(q, &b0.point3, &b1, transform, stroke_width, tolerance);
}

static void d2d_rect_union(D2D1_RECT_F *l, const D2D1_RECT_F *r)
{
    l->left   = min(l->left, r->left);
    l->top    = min(l->top, r->top);
    l->right  = max(l->right, r->right);
    l->bottom = max(l->bottom, r->bottom);
}

static BOOL d2d_rect_check_overlap(const D2D_RECT_F *p, const D2D_RECT_F *q)
{
    return p->left < q->right && p->top < q->bottom && p->right > q->left && p->bottom > q->top;
}

static void d2d_rect_get_bezier_bounds(D2D_RECT_F *bounds, const D2D1_POINT_2F *p0,
        const D2D1_POINT_2F *p1, const D2D1_POINT_2F *p2)
{
    D2D1_POINT_2F p;
    float root;

    bounds->left = p0->x;
    bounds->top = p0->y;
    bounds->right = p0->x;
    bounds->bottom = p0->y;

    d2d_rect_expand(bounds, p2);

    /* f(t) = (1 - t)²P₀ + 2(1 - t)tP₁ + t²P₂
     * f'(t) = 2(1 - t)(P₁ - P₀) + 2t(P₂ - P₁)
     *       = 2(P₂ - 2P₁ + P₀)t + 2(P₁ - P₀)
     *
     * f'(t) = 0
     * t = (P₀ - P₁) / (P₂ - 2P₁ + P₀) */
    root = (p0->x - p1->x) / (p2->x - 2.0f * p1->x + p0->x);
    if (root > 0.0f && root < 1.0f)
    {
        d2d_point_calculate_bezier(&p, p0, p1, p2, root);
        d2d_rect_expand(bounds, &p);
    }

    root = (p0->y - p1->y) / (p2->y - 2.0f * p1->y + p0->y);
    if (root > 0.0f && root < 1.0f)
    {
        d2d_point_calculate_bezier(&p, p0, p1, p2, root);
        d2d_rect_expand(bounds, &p);
    }
}

static void d2d_rect_get_bezier_segment_bounds(D2D_RECT_F *bounds, const D2D1_POINT_2F *p0,
        const D2D1_POINT_2F *p1, const D2D1_POINT_2F *p2, float start, float end)
{
    D2D1_POINT_2F q[3], r[2];

    d2d_point_lerp(&r[0], p0, p1, start);
    d2d_point_lerp(&r[1], p1, p2, start);
    d2d_point_lerp(&q[0], &r[0], &r[1], start);

    end = (end - start) / (1.0f - start);
    d2d_point_lerp(&q[1], &q[0], &r[1], end);
    d2d_point_lerp(&r[0], &r[1], p2, end);
    d2d_point_lerp(&q[2], &q[1], &r[0], end);

    d2d_rect_get_bezier_bounds(bounds, &q[0], &q[1], &q[2]);
}

static BOOL d2d_figure_insert_vertex(struct d2d_figure *figure, size_t idx, D2D1_POINT_2F vertex)
{
    if (!d2d_array_reserve((void **)&figure->vertices, &figure->vertices_size,
            figure->vertex_count + 1, sizeof(*figure->vertices)))
    {
        ERR("Failed to grow vertices array.\n");
        return FALSE;
    }

    if (!d2d_array_reserve((void **)&figure->vertex_types, &figure->vertex_types_size,
            figure->vertex_count + 1, sizeof(*figure->vertex_types)))
    {
        ERR("Failed to grow vertex types array.\n");
        return FALSE;
    }

    memmove(&figure->vertices[idx + 1], &figure->vertices[idx],
            (figure->vertex_count - idx) * sizeof(*figure->vertices));
    memmove(&figure->vertex_types[idx + 1], &figure->vertex_types[idx],
            (figure->vertex_count - idx) * sizeof(*figure->vertex_types));
    figure->vertices[idx] = vertex;
    figure->vertex_types[idx] = D2D_VERTEX_TYPE_NONE;
    d2d_rect_expand(&figure->bounds, &vertex);
    ++figure->vertex_count;
    return TRUE;
}

static BOOL d2d_figure_add_vertex(struct d2d_figure *figure, D2D1_POINT_2F vertex)
{
    size_t last = figure->vertex_count - 1;

    if (figure->vertex_count && figure->vertex_types[last] == D2D_VERTEX_TYPE_LINE
            && !memcmp(&figure->vertices[last], &vertex, sizeof(vertex)))
        return TRUE;

    if (!d2d_array_reserve((void **)&figure->vertices, &figure->vertices_size,
            figure->vertex_count + 1, sizeof(*figure->vertices)))
    {
        ERR("Failed to grow vertices array.\n");
        return FALSE;
    }

    if (!d2d_array_reserve((void **)&figure->vertex_types, &figure->vertex_types_size,
            figure->vertex_count + 1, sizeof(*figure->vertex_types)))
    {
        ERR("Failed to grow vertex types array.\n");
        return FALSE;
    }

    figure->vertices[figure->vertex_count] = vertex;
    figure->vertex_types[figure->vertex_count] = D2D_VERTEX_TYPE_NONE;
    d2d_rect_expand(&figure->bounds, &vertex);
    ++figure->vertex_count;
    return TRUE;
}

static BOOL d2d_figure_insert_bezier_controls(struct d2d_figure *figure,
        size_t idx, size_t count, const D2D1_POINT_2F *p)
{
    if (!d2d_array_reserve((void **)&figure->bezier_controls, &figure->bezier_controls_size,
            figure->bezier_control_count + count, sizeof(*figure->bezier_controls)))
    {
        ERR("Failed to grow bezier controls array.\n");
        return FALSE;
    }

    memmove(&figure->bezier_controls[idx + count], &figure->bezier_controls[idx],
            (figure->bezier_control_count - idx) * sizeof(*figure->bezier_controls));
    memcpy(&figure->bezier_controls[idx], p, count * sizeof(*figure->bezier_controls));
    figure->bezier_control_count += count;

    return TRUE;
}

static BOOL d2d_figure_add_bezier_controls(struct d2d_figure *figure, size_t count, const D2D1_POINT_2F *p)
{
    if (!d2d_array_reserve((void **)&figure->bezier_controls, &figure->bezier_controls_size,
            figure->bezier_control_count + count, sizeof(*figure->bezier_controls)))
    {
        ERR("Failed to grow bezier controls array.\n");
        return FALSE;
    }

    memcpy(&figure->bezier_controls[figure->bezier_control_count], p, count * sizeof(*figure->bezier_controls));
    figure->bezier_control_count += count;

    return TRUE;
}

static BOOL d2d_figure_add_original_bezier_controls(struct d2d_figure *figure, size_t count, const D2D1_POINT_2F *p)
{
    if (!d2d_array_reserve((void **)&figure->original_bezier_controls, &figure->original_bezier_controls_size,
            figure->original_bezier_control_count + count, sizeof(*figure->original_bezier_controls)))
    {
        ERR("Failed to grow cubic Bézier controls array.\n");
        return FALSE;
    }

    memcpy(&figure->original_bezier_controls[figure->original_bezier_control_count],
            p, count * sizeof(*figure->original_bezier_controls));
    figure->original_bezier_control_count += count;

    return TRUE;
}

static void d2d_cdt_edge_rot(struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    dst->idx = src->idx;
    dst->r = (src->r + D2D_EDGE_NEXT_ROT) & 3;
}

static void d2d_cdt_edge_sym(struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    dst->idx = src->idx;
    dst->r = (src->r + D2D_EDGE_NEXT_SYM) & 3;
}

static void d2d_cdt_edge_tor(struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    dst->idx = src->idx;
    dst->r = (src->r + D2D_EDGE_NEXT_TOR) & 3;
}

static void d2d_cdt_edge_next_left(const struct d2d_cdt *cdt,
        struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    d2d_cdt_edge_rot(dst, &cdt->edges[src->idx].next[(src->r + D2D_EDGE_NEXT_TOR) & 3]);
}

static void d2d_cdt_edge_next_origin(const struct d2d_cdt *cdt,
        struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    *dst = cdt->edges[src->idx].next[src->r];
}

static void d2d_cdt_edge_prev_origin(const struct d2d_cdt *cdt,
        struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    d2d_cdt_edge_rot(dst, &cdt->edges[src->idx].next[(src->r + D2D_EDGE_NEXT_ROT) & 3]);
}

static size_t d2d_cdt_edge_origin(const struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *e)
{
    return cdt->edges[e->idx].vertex[e->r >> 1];
}

static size_t d2d_cdt_edge_destination(const struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *e)
{
    return cdt->edges[e->idx].vertex[!(e->r >> 1)];
}

static void d2d_cdt_edge_set_origin(const struct d2d_cdt *cdt,
        const struct d2d_cdt_edge_ref *e, size_t vertex)
{
    cdt->edges[e->idx].vertex[e->r >> 1] = vertex;
}

static void d2d_cdt_edge_set_destination(const struct d2d_cdt *cdt,
        const struct d2d_cdt_edge_ref *e, size_t vertex)
{
    cdt->edges[e->idx].vertex[!(e->r >> 1)] = vertex;
}

static float d2d_cdt_ccw(const struct d2d_cdt *cdt, size_t a, size_t b, size_t c)
{
    return d2d_point_ccw(&cdt->vertices[a], &cdt->vertices[b], &cdt->vertices[c]);
}

static BOOL d2d_cdt_rightof(const struct d2d_cdt *cdt, size_t p, const struct d2d_cdt_edge_ref *e)
{
    return d2d_cdt_ccw(cdt, p, d2d_cdt_edge_destination(cdt, e), d2d_cdt_edge_origin(cdt, e)) > 0.0f;
}

static BOOL d2d_cdt_leftof(const struct d2d_cdt *cdt, size_t p, const struct d2d_cdt_edge_ref *e)
{
    return d2d_cdt_ccw(cdt, p, d2d_cdt_edge_origin(cdt, e), d2d_cdt_edge_destination(cdt, e)) > 0.0f;
}

/* |ax ay|
 * |bx by| */
static void d2d_fp_four_det2x2(float *out, float ax, float ay, float bx, float by)
{
    float axby[2], aybx[2];

    d2d_fp_two_product(axby, ax, by);
    d2d_fp_two_product(aybx, ay, bx);
    d2d_fp_two_two_diff(out, axby, aybx);
}

/* (a->x² + a->y²) * det2x2 */
static void d2d_fp_sub_det3x3(float *out, size_t *out_len, const struct d2d_fp_two_vec2 *a, const float *det2x2)
{
    size_t axd_len, ayd_len, axxd_len, ayyd_len;
    float axd[8], ayd[8], axxd[16], ayyd[16];

    d2d_fp_scale_expansion_zeroelim(axd, &axd_len, det2x2, 4, a->x[1]);
    d2d_fp_scale_expansion_zeroelim(axxd, &axxd_len, axd, axd_len, a->x[1]);
    d2d_fp_scale_expansion_zeroelim(ayd, &ayd_len, det2x2, 4, a->y[1]);
    d2d_fp_scale_expansion_zeroelim(ayyd, &ayyd_len, ayd, ayd_len, a->y[1]);
    d2d_fp_fast_expansion_sum_zeroelim(out, out_len, axxd, axxd_len, ayyd, ayyd_len);
}

/* det_abt = det_ab * c[0]
 * fin += c[0] * (az * b - bz * a + c[1] * det_ab * 2.0f) */
static void d2d_cdt_incircle_refine1(struct d2d_fp_fin *fin, float *det_abt, size_t *det_abt_len,
        const float *det_ab, float a, const float *az, float b, const float *bz, const float *c)
{
    size_t temp48_len, temp32_len, temp16a_len, temp16b_len, temp16c_len, temp8_len;
    float temp48[48], temp32[32], temp16a[16], temp16b[16], temp16c[16], temp8[8];
    float *swap;

    d2d_fp_scale_expansion_zeroelim(det_abt, det_abt_len, det_ab, 4, c[0]);
    d2d_fp_scale_expansion_zeroelim(temp16a, &temp16a_len, det_abt, *det_abt_len, 2.0f * c[1]);
    d2d_fp_scale_expansion_zeroelim(temp8, &temp8_len, az, 4, c[0]);
    d2d_fp_scale_expansion_zeroelim(temp16b, &temp16b_len, temp8, temp8_len, b);
    d2d_fp_scale_expansion_zeroelim(temp8, &temp8_len, bz, 4, c[0]);
    d2d_fp_scale_expansion_zeroelim(temp16c, &temp16c_len, temp8, temp8_len, -a);
    d2d_fp_fast_expansion_sum_zeroelim(temp32, &temp32_len, temp16a, temp16a_len, temp16b, temp16b_len);
    d2d_fp_fast_expansion_sum_zeroelim(temp48, &temp48_len, temp16c, temp16c_len, temp32, temp32_len);
    d2d_fp_fast_expansion_sum_zeroelim(fin->other, &fin->length, fin->now, fin->length, temp48, temp48_len);
    swap = fin->now; fin->now = fin->other; fin->other = swap;
}

static void d2d_cdt_incircle_refine2(struct d2d_fp_fin *fin, const struct d2d_fp_two_vec2 *a,
        const struct d2d_fp_two_vec2 *b, const float *bz, const struct d2d_fp_two_vec2 *c, const float *cz,
        const float *axt_det_bc, size_t axt_det_bc_len, const float *ayt_det_bc, size_t ayt_det_bc_len)
{
    size_t temp64_len, temp48_len, temp32a_len, temp32b_len, temp16a_len, temp16b_len, temp8_len;
    float temp64[64], temp48[48], temp32a[32], temp32b[32], temp16a[16], temp16b[16], temp8[8];
    float bct[8], bctt[4], temp4a[4], temp4b[4], temp2a[2], temp2b[2];
    size_t bct_len, bctt_len;
    float *swap;

    /* bct = (b->x[0] * c->y[1] + b->x[1] * c->y[0]) - (c->x[0] * b->y[1] + c->x[1] * b->y[0]) */
    /* bctt = b->x[0] * c->y[0] + c->x[0] * b->y[0] */
    if (b->x[0] != 0.0f || b->y[0] != 0.0f || c->x[0] != 0.0f || c->y[0] != 0.0f)
    {
        d2d_fp_two_product(temp2a, b->x[0], c->y[1]);
        d2d_fp_two_product(temp2b, b->x[1], c->y[0]);
        d2d_fp_two_two_sum(temp4a, temp2a, temp2b);
        d2d_fp_two_product(temp2a, c->x[0], -b->y[1]);
        d2d_fp_two_product(temp2b, c->x[1], -b->y[0]);
        d2d_fp_two_two_sum(temp4b, temp2a, temp2b);
        d2d_fp_fast_expansion_sum_zeroelim(bct, &bct_len, temp4a, 4, temp4b, 4);

        d2d_fp_two_product(temp2a, b->x[0], c->y[0]);
        d2d_fp_two_product(temp2b, c->x[0], b->y[0]);
        d2d_fp_two_two_diff(bctt, temp2a, temp2b);
        bctt_len = 4;
    }
    else
    {
        bct[0] = 0.0f;
        bct_len = 1;
        bctt[0] = 0.0f;
        bctt_len = 1;
    }

    if (a->x[0] != 0.0f)
    {
        size_t axt_bct_len, axt_bctt_len;
        float axt_bct[16], axt_bctt[8];

        /* fin += a->x[0] * (axt_det_bc + bct * 2.0f * a->x[1]) */
        d2d_fp_scale_expansion_zeroelim(temp16a, &temp16a_len, axt_det_bc, axt_det_bc_len, a->x[0]);
        d2d_fp_scale_expansion_zeroelim(axt_bct, &axt_bct_len, bct, bct_len, a->x[0]);
        d2d_fp_scale_expansion_zeroelim(temp32a, &temp32a_len, axt_bct, axt_bct_len, 2.0f * a->x[1]);
        d2d_fp_fast_expansion_sum_zeroelim(temp48, &temp48_len, temp16a, temp16a_len, temp32a, temp32a_len);
        d2d_fp_fast_expansion_sum_zeroelim(fin->other, &fin->length, fin->now, fin->length, temp48, temp48_len);
        swap = fin->now; fin->now = fin->other; fin->other = swap;

        if (b->y[0] != 0.0f)
        {
            /* fin += a->x[0] * cz * b->y[0] */
            d2d_fp_scale_expansion_zeroelim(temp8, &temp8_len, cz, 4, a->x[0]);
            d2d_fp_scale_expansion_zeroelim(temp16a, &temp16a_len, temp8, temp8_len, b->y[0]);
            d2d_fp_fast_expansion_sum_zeroelim(fin->other, &fin->length, fin->now, fin->length, temp16a, temp16a_len);
            swap = fin->now; fin->now = fin->other; fin->other = swap;
        }

        if (c->y[0] != 0.0f)
        {
            /* fin -= a->x[0] * bz * c->y[0] */
            d2d_fp_scale_expansion_zeroelim(temp8, &temp8_len, bz, 4, -a->x[0]);
            d2d_fp_scale_expansion_zeroelim(temp16a, &temp16a_len, temp8, temp8_len, c->y[0]);
            d2d_fp_fast_expansion_sum_zeroelim(fin->other, &fin->length, fin->now, fin->length, temp16a, temp16a_len);
            swap = fin->now; fin->now = fin->other; fin->other = swap;
        }

        /* fin += a->x[0] * (bct * a->x[0] + bctt * (2.0f * a->x[1] + a->x[0])) */
        d2d_fp_scale_expansion_zeroelim(temp32a, &temp32a_len, axt_bct, axt_bct_len, a->x[0]);
        d2d_fp_scale_expansion_zeroelim(axt_bctt, &axt_bctt_len, bctt, bctt_len, a->x[0]);
        d2d_fp_scale_expansion_zeroelim(temp16a, &temp16a_len, axt_bctt, axt_bctt_len, 2.0f * a->x[1]);
        d2d_fp_scale_expansion_zeroelim(temp16b, &temp16b_len, axt_bctt, axt_bctt_len, a->x[0]);
        d2d_fp_fast_expansion_sum_zeroelim(temp32b, &temp32b_len, temp16a, temp16a_len, temp16b, temp16b_len);
        d2d_fp_fast_expansion_sum_zeroelim(temp64, &temp64_len, temp32a, temp32a_len, temp32b, temp32b_len);
        d2d_fp_fast_expansion_sum_zeroelim(fin->other, &fin->length, fin->now, fin->length, temp64, temp64_len);
        swap = fin->now; fin->now = fin->other; fin->other = swap;
    }

    if (a->y[0] != 0.0f)
    {
        size_t ayt_bct_len, ayt_bctt_len;
        float ayt_bct[16], ayt_bctt[8];

        /* fin += a->y[0] * (ayt_det_bc + bct * 2.0f * a->y[1]) */
        d2d_fp_scale_expansion_zeroelim(temp16a, &temp16a_len, ayt_det_bc, ayt_det_bc_len, a->y[0]);
        d2d_fp_scale_expansion_zeroelim(ayt_bct, &ayt_bct_len, bct, bct_len, a->y[0]);
        d2d_fp_scale_expansion_zeroelim(temp32a, &temp32a_len, ayt_bct, ayt_bct_len, 2.0f * a->y[1]);
        d2d_fp_fast_expansion_sum_zeroelim(temp48, &temp48_len, temp16a, temp16a_len, temp32a, temp32a_len);
        d2d_fp_fast_expansion_sum_zeroelim(fin->other, &fin->length, fin->now, fin->length, temp48, temp48_len);
        swap = fin->now; fin->now = fin->other; fin->other = swap;

        /* fin += a->y[0] * (bct * a->y[0] + bctt * (2.0f * a->y[1] + a->y[0])) */
        d2d_fp_scale_expansion_zeroelim(temp32a, &temp32a_len, ayt_bct, ayt_bct_len, a->y[0]);
        d2d_fp_scale_expansion_zeroelim(ayt_bctt, &ayt_bctt_len, bctt, bctt_len, a->y[0]);
        d2d_fp_scale_expansion_zeroelim(temp16a, &temp16a_len, ayt_bctt, ayt_bctt_len, 2.0f * a->y[1]);
        d2d_fp_scale_expansion_zeroelim(temp16b, &temp16b_len, ayt_bctt, ayt_bctt_len, a->y[0]);
        d2d_fp_fast_expansion_sum_zeroelim(temp32b, &temp32b_len, temp16a, temp16a_len, temp16b, temp16b_len);
        d2d_fp_fast_expansion_sum_zeroelim(temp64, &temp64_len, temp32a, temp32a_len, temp32b, temp32b_len);
        d2d_fp_fast_expansion_sum_zeroelim(fin->other, &fin->length, fin->now, fin->length, temp64, temp64_len);
        swap = fin->now; fin->now = fin->other; fin->other = swap;
    }
}

/* Determine if point D is inside or outside the circle defined by points A,
 * B, C. As explained in the paper by Guibas and Stolfi, this is equivalent to
 * calculating the signed volume of the tetrahedron defined by projecting the
 * points onto the paraboloid of revolution x = x² + y²,
 * λ:(x, y) → (x, y, x² + y²). I.e., D is inside the cirlce if
 *
 * |λ(A) 1|
 * |λ(B) 1| > 0
 * |λ(C) 1|
 * |λ(D) 1|
 *
 * After translating D to the origin, that becomes:
 *
 * |λ(A-D)|
 * |λ(B-D)| > 0
 * |λ(C-D)|
 *
 * This implementation is based on the paper "Adaptive Precision
 * Floating-Point Arithmetic and Fast Robust Geometric Predicates" and
 * associated (Public Domain) code by Jonathan Richard Shewchuk. */
static BOOL d2d_cdt_incircle(const struct d2d_cdt *cdt, size_t a, size_t b, size_t c, size_t d)
{
    static const float err_bound_result = (3.0f + 8.0f * D2D_FP_EPS) * D2D_FP_EPS;
    static const float err_bound_a = (10.0f + 96.0f * D2D_FP_EPS) * D2D_FP_EPS;
    static const float err_bound_b = (4.0f + 48.0f * D2D_FP_EPS) * D2D_FP_EPS;
    static const float err_bound_c = (44.0f + 576.0f * D2D_FP_EPS) * D2D_FP_EPS * D2D_FP_EPS;

    size_t axt_det_bc_len, ayt_det_bc_len, bxt_det_ca_len, byt_det_ca_len, cxt_det_ab_len, cyt_det_ab_len;
    float axt_det_bc[8], ayt_det_bc[8], bxt_det_ca[8], byt_det_ca[8], cxt_det_ab[8], cyt_det_ab[8];
    float fin1[1152], fin2[1152], temp64[64], sub_det_a[32], sub_det_b[32], sub_det_c[32];
    float det_bc[4], det_ca[4], det_ab[4], daz[4], dbz[4], dcz[4], temp2a[2], temp2b[2];
    size_t temp64_len, sub_det_a_len, sub_det_b_len, sub_det_c_len;
    float dbxdcy, dbydcx, dcxday, dcydax, daxdby, daydbx;
    const D2D1_POINT_2F *p = cdt->vertices;
    struct d2d_fp_two_vec2 da, db, dc;
    float permanent, err_bound, det;
    struct d2d_fp_fin fin;

    da.x[1] = p[a].x - p[d].x;
    da.y[1] = p[a].y - p[d].y;
    db.x[1] = p[b].x - p[d].x;
    db.y[1] = p[b].y - p[d].y;
    dc.x[1] = p[c].x - p[d].x;
    dc.y[1] = p[c].y - p[d].y;

    daz[3] = da.x[1] * da.x[1] + da.y[1] * da.y[1];
    dbxdcy = db.x[1] * dc.y[1];
    dbydcx = db.y[1] * dc.x[1];

    dbz[3] = db.x[1] * db.x[1] + db.y[1] * db.y[1];
    dcxday = dc.x[1] * da.y[1];
    dcydax = dc.y[1] * da.x[1];

    dcz[3] = dc.x[1] * dc.x[1] + dc.y[1] * dc.y[1];
    daxdby = da.x[1] * db.y[1];
    daydbx = da.y[1] * db.x[1];

    det = daz[3] * (dbxdcy - dbydcx) + dbz[3] * (dcxday - dcydax) + dcz[3] * (daxdby - daydbx);
    permanent = daz[3] * (fabsf(dbxdcy) + fabsf(dbydcx))
            + dbz[3] * (fabsf(dcxday) + fabsf(dcydax))
            + dcz[3] * (fabsf(daxdby) + fabsf(daydbx));
    err_bound = err_bound_a * permanent;
    if (det > err_bound || -det > err_bound)
        return det > 0.0f;

    fin.now = fin1;
    fin.other = fin2;

    d2d_fp_four_det2x2(det_bc, db.x[1], db.y[1], dc.x[1], dc.y[1]);
    d2d_fp_sub_det3x3(sub_det_a, &sub_det_a_len, &da, det_bc);

    d2d_fp_four_det2x2(det_ca, dc.x[1], dc.y[1], da.x[1], da.y[1]);
    d2d_fp_sub_det3x3(sub_det_b, &sub_det_b_len, &db, det_ca);

    d2d_fp_four_det2x2(det_ab, da.x[1], da.y[1], db.x[1], db.y[1]);
    d2d_fp_sub_det3x3(sub_det_c, &sub_det_c_len, &dc, det_ab);

    d2d_fp_fast_expansion_sum_zeroelim(temp64, &temp64_len, sub_det_a, sub_det_a_len, sub_det_b, sub_det_b_len);
    d2d_fp_fast_expansion_sum_zeroelim(fin.now, &fin.length, temp64, temp64_len, sub_det_c, sub_det_c_len);
    det = d2d_fp_estimate(fin.now, fin.length);
    err_bound = err_bound_b * permanent;
    if (det >= err_bound || -det >= err_bound)
        return det > 0.0f;

    d2d_fp_two_diff_tail(&da.x[0], p[a].x, p[d].x, da.x[1]);
    d2d_fp_two_diff_tail(&da.y[0], p[a].y, p[d].y, da.y[1]);
    d2d_fp_two_diff_tail(&db.x[0], p[b].x, p[d].x, db.x[1]);
    d2d_fp_two_diff_tail(&db.y[0], p[b].y, p[d].y, db.y[1]);
    d2d_fp_two_diff_tail(&dc.x[0], p[c].x, p[d].x, dc.x[1]);
    d2d_fp_two_diff_tail(&dc.y[0], p[c].y, p[d].y, dc.y[1]);
    if (da.x[0] == 0.0f && db.x[0] == 0.0f && dc.x[0] == 0.0f
            && da.y[0] == 0.0f && db.y[0] == 0.0f && dc.y[0] == 0.0f)
        return det > 0.0f;

    err_bound = err_bound_c * permanent + err_bound_result * fabsf(det);
    det += (daz[3] * ((db.x[1] * dc.y[0] + dc.y[1] * db.x[0]) - (db.y[1] * dc.x[0] + dc.x[1] * db.y[0]))
            + 2.0f * (da.x[1] * da.x[0] + da.y[1] * da.y[0]) * (db.x[1] * dc.y[1] - db.y[1] * dc.x[1]))
            + (dbz[3] * ((dc.x[1] * da.y[0] + da.y[1] * dc.x[0]) - (dc.y[1] * da.x[0] + da.x[1] * dc.y[0]))
            + 2.0f * (db.x[1] * db.x[0] + db.y[1] * db.y[0]) * (dc.x[1] * da.y[1] - dc.y[1] * da.x[1]))
            + (dcz[3] * ((da.x[1] * db.y[0] + db.y[1] * da.x[0]) - (da.y[1] * db.x[0] + db.x[1] * da.y[0]))
            + 2.0f * (dc.x[1] * dc.x[0] + dc.y[1] * dc.y[0]) * (da.x[1] * db.y[1] - da.y[1] * db.x[1]));
    if (det >= err_bound || -det >= err_bound)
        return det > 0.0f;

    if (db.x[0] != 0.0f || db.y[0] != 0.0f || dc.x[0] != 0.0f || dc.y[0] != 0.0f)
    {
        d2d_fp_square(temp2a, da.x[1]);
        d2d_fp_square(temp2b, da.y[1]);
        d2d_fp_two_two_sum(daz, temp2a, temp2b);
    }
    if (dc.x[0] != 0.0f || dc.y[0] != 0.0f || da.x[0] != 0.0f || da.y[0] != 0.0f)
    {
        d2d_fp_square(temp2a, db.x[1]);
        d2d_fp_square(temp2b, db.y[1]);
        d2d_fp_two_two_sum(dbz, temp2a, temp2b);
    }
    if (da.x[0] != 0.0f || da.y[0] != 0.0f || db.x[0] != 0.0f || db.y[0] != 0.0f)
    {
        d2d_fp_square(temp2a, dc.x[1]);
        d2d_fp_square(temp2b, dc.y[1]);
        d2d_fp_two_two_sum(dcz, temp2a, temp2b);
    }

    if (da.x[0] != 0.0f)
        d2d_cdt_incircle_refine1(&fin, axt_det_bc, &axt_det_bc_len, det_bc, dc.y[1], dcz, db.y[1], dbz, da.x);
    if (da.y[0] != 0.0f)
        d2d_cdt_incircle_refine1(&fin, ayt_det_bc, &ayt_det_bc_len, det_bc, db.x[1], dbz, dc.x[1], dcz, da.y);
    if (db.x[0] != 0.0f)
        d2d_cdt_incircle_refine1(&fin, bxt_det_ca, &bxt_det_ca_len, det_ca, da.y[1], daz, dc.y[1], dcz, db.x);
    if (db.y[0] != 0.0f)
        d2d_cdt_incircle_refine1(&fin, byt_det_ca, &byt_det_ca_len, det_ca, dc.x[1], dcz, da.x[1], daz, db.y);
    if (dc.x[0] != 0.0f)
        d2d_cdt_incircle_refine1(&fin, cxt_det_ab, &cxt_det_ab_len, det_ab, db.y[1], dbz, da.y[1], daz, dc.x);
    if (dc.y[0] != 0.0f)
        d2d_cdt_incircle_refine1(&fin, cyt_det_ab, &cyt_det_ab_len, det_ab, da.x[1], daz, db.x[1], dbz, dc.y);

    if (da.x[0] != 0.0f || da.y[0] != 0.0f)
        d2d_cdt_incircle_refine2(&fin, &da, &db, dbz, &dc, dcz,
                axt_det_bc, axt_det_bc_len, ayt_det_bc, ayt_det_bc_len);
    if (db.x[0] != 0.0f || db.y[0] != 0.0f)
        d2d_cdt_incircle_refine2(&fin, &db, &dc, dcz, &da, daz,
                bxt_det_ca, bxt_det_ca_len, byt_det_ca, byt_det_ca_len);
    if (dc.x[0] != 0.0f || dc.y[0] != 0.0f)
        d2d_cdt_incircle_refine2(&fin, &dc, &da, daz, &db, dbz,
                cxt_det_ab, cxt_det_ab_len, cyt_det_ab, cyt_det_ab_len);

    return fin.now[fin.length - 1] > 0.0f;
}

static void d2d_cdt_splice(const struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *a,
        const struct d2d_cdt_edge_ref *b)
{
    struct d2d_cdt_edge_ref ta, tb, alpha, beta;

    ta = cdt->edges[a->idx].next[a->r];
    tb = cdt->edges[b->idx].next[b->r];
    cdt->edges[a->idx].next[a->r] = tb;
    cdt->edges[b->idx].next[b->r] = ta;

    d2d_cdt_edge_rot(&alpha, &ta);
    d2d_cdt_edge_rot(&beta, &tb);

    ta = cdt->edges[alpha.idx].next[alpha.r];
    tb = cdt->edges[beta.idx].next[beta.r];
    cdt->edges[alpha.idx].next[alpha.r] = tb;
    cdt->edges[beta.idx].next[beta.r] = ta;
}

static BOOL d2d_cdt_create_edge(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *e)
{
    struct d2d_cdt_edge *edge;

    if (cdt->free_edge != ~0u)
    {
        e->idx = cdt->free_edge;
        cdt->free_edge = cdt->edges[e->idx].next[D2D_EDGE_NEXT_ORIGIN].idx;
    }
    else
    {
        if (!d2d_array_reserve((void **)&cdt->edges, &cdt->edges_size, cdt->edge_count + 1, sizeof(*cdt->edges)))
        {
            ERR("Failed to grow edges array.\n");
            return FALSE;
        }
        e->idx = cdt->edge_count++;
    }
    e->r = 0;

    edge = &cdt->edges[e->idx];
    edge->next[D2D_EDGE_NEXT_ORIGIN] = *e;
    d2d_cdt_edge_tor(&edge->next[D2D_EDGE_NEXT_ROT], e);
    d2d_cdt_edge_sym(&edge->next[D2D_EDGE_NEXT_SYM], e);
    d2d_cdt_edge_rot(&edge->next[D2D_EDGE_NEXT_TOR], e);
    edge->flags = 0;

    return TRUE;
}

static void d2d_cdt_destroy_edge(struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *e)
{
    struct d2d_cdt_edge_ref next, sym, prev;

    d2d_cdt_edge_next_origin(cdt, &next, e);
    if (next.idx != e->idx || next.r != e->r)
    {
        d2d_cdt_edge_prev_origin(cdt, &prev, e);
        d2d_cdt_splice(cdt, e, &prev);
    }

    d2d_cdt_edge_sym(&sym, e);

    d2d_cdt_edge_next_origin(cdt, &next, &sym);
    if (next.idx != sym.idx || next.r != sym.r)
    {
        d2d_cdt_edge_prev_origin(cdt, &prev, &sym);
        d2d_cdt_splice(cdt, &sym, &prev);
    }

    cdt->edges[e->idx].flags |= D2D_CDT_EDGE_FLAG_FREED;
    cdt->edges[e->idx].next[D2D_EDGE_NEXT_ORIGIN].idx = cdt->free_edge;
    cdt->free_edge = e->idx;
}

static BOOL d2d_cdt_connect(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *e,
        const struct d2d_cdt_edge_ref *a, const struct d2d_cdt_edge_ref *b)
{
    struct d2d_cdt_edge_ref tmp;

    if (!d2d_cdt_create_edge(cdt, e))
        return FALSE;
    d2d_cdt_edge_set_origin(cdt, e, d2d_cdt_edge_destination(cdt, a));
    d2d_cdt_edge_set_destination(cdt, e, d2d_cdt_edge_origin(cdt, b));
    d2d_cdt_edge_next_left(cdt, &tmp, a);
    d2d_cdt_splice(cdt, e, &tmp);
    d2d_cdt_edge_sym(&tmp, e);
    d2d_cdt_splice(cdt, &tmp, b);

    return TRUE;
}

static BOOL d2d_cdt_merge(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *left_outer,
        struct d2d_cdt_edge_ref *left_inner, struct d2d_cdt_edge_ref *right_inner,
        struct d2d_cdt_edge_ref *right_outer)
{
    struct d2d_cdt_edge_ref base_edge, tmp;

    /* Create the base edge between both parts. */
    for (;;)
    {
        if (d2d_cdt_leftof(cdt, d2d_cdt_edge_origin(cdt, right_inner), left_inner))
        {
            d2d_cdt_edge_next_left(cdt, left_inner, left_inner);
        }
        else if (d2d_cdt_rightof(cdt, d2d_cdt_edge_origin(cdt, left_inner), right_inner))
        {
            d2d_cdt_edge_sym(&tmp, right_inner);
            d2d_cdt_edge_next_origin(cdt, right_inner, &tmp);
        }
        else
        {
            break;
        }
    }

    d2d_cdt_edge_sym(&tmp, right_inner);
    if (!d2d_cdt_connect(cdt, &base_edge, &tmp, left_inner))
        return FALSE;
    if (d2d_cdt_edge_origin(cdt, left_inner) == d2d_cdt_edge_origin(cdt, left_outer))
        d2d_cdt_edge_sym(left_outer, &base_edge);
    if (d2d_cdt_edge_origin(cdt, right_inner) == d2d_cdt_edge_origin(cdt, right_outer))
        *right_outer = base_edge;

    for (;;)
    {
        struct d2d_cdt_edge_ref left_candidate, right_candidate, sym_base_edge;
        BOOL left_valid, right_valid;

        /* Find the left candidate. */
        d2d_cdt_edge_sym(&sym_base_edge, &base_edge);
        d2d_cdt_edge_next_origin(cdt, &left_candidate, &sym_base_edge);
        if ((left_valid = d2d_cdt_leftof(cdt, d2d_cdt_edge_destination(cdt, &left_candidate), &sym_base_edge)))
        {
            d2d_cdt_edge_next_origin(cdt, &tmp, &left_candidate);
            while (d2d_cdt_edge_destination(cdt, &tmp) != d2d_cdt_edge_destination(cdt, &sym_base_edge)
                    && d2d_cdt_incircle(cdt,
                    d2d_cdt_edge_origin(cdt, &sym_base_edge), d2d_cdt_edge_destination(cdt, &sym_base_edge),
                    d2d_cdt_edge_destination(cdt, &left_candidate), d2d_cdt_edge_destination(cdt, &tmp)))
            {
                d2d_cdt_destroy_edge(cdt, &left_candidate);
                left_candidate = tmp;
                d2d_cdt_edge_next_origin(cdt, &tmp, &left_candidate);
            }
        }
        d2d_cdt_edge_sym(&left_candidate, &left_candidate);

        /* Find the right candidate. */
        d2d_cdt_edge_prev_origin(cdt, &right_candidate, &base_edge);
        if ((right_valid = d2d_cdt_rightof(cdt, d2d_cdt_edge_destination(cdt, &right_candidate), &base_edge)))
        {
            d2d_cdt_edge_prev_origin(cdt, &tmp, &right_candidate);
            while (d2d_cdt_edge_destination(cdt, &tmp) != d2d_cdt_edge_destination(cdt, &base_edge)
                    && d2d_cdt_incircle(cdt,
                    d2d_cdt_edge_origin(cdt, &sym_base_edge), d2d_cdt_edge_destination(cdt, &sym_base_edge),
                    d2d_cdt_edge_destination(cdt, &right_candidate), d2d_cdt_edge_destination(cdt, &tmp)))
            {
                d2d_cdt_destroy_edge(cdt, &right_candidate);
                right_candidate = tmp;
                d2d_cdt_edge_prev_origin(cdt, &tmp, &right_candidate);
            }
        }

        if (!left_valid && !right_valid)
            break;

        /* Connect the appropriate candidate with the base edge. */
        if (!left_valid || (right_valid && d2d_cdt_incircle(cdt,
                d2d_cdt_edge_origin(cdt, &left_candidate), d2d_cdt_edge_destination(cdt, &left_candidate),
                d2d_cdt_edge_origin(cdt, &right_candidate), d2d_cdt_edge_destination(cdt, &right_candidate))))
        {
            if (!d2d_cdt_connect(cdt, &base_edge, &right_candidate, &sym_base_edge))
                return FALSE;
        }
        else
        {
            if (!d2d_cdt_connect(cdt, &base_edge, &sym_base_edge, &left_candidate))
                return FALSE;
        }
    }

    return TRUE;
}

/* Create a Delaunay triangulation from a set of vertices. This is an
 * implementation of the divide-and-conquer algorithm described by Guibas and
 * Stolfi. Should be called with at least two vertices. */
static BOOL d2d_cdt_triangulate(struct d2d_cdt *cdt, size_t start_vertex, size_t vertex_count,
        struct d2d_cdt_edge_ref *left_edge, struct d2d_cdt_edge_ref *right_edge)
{
    struct d2d_cdt_edge_ref left_inner, left_outer, right_inner, right_outer, tmp;
    size_t cut;

    /* Only two vertices, create a single edge. */
    if (vertex_count == 2)
    {
        struct d2d_cdt_edge_ref a;

        if (!d2d_cdt_create_edge(cdt, &a))
            return FALSE;
        d2d_cdt_edge_set_origin(cdt, &a, start_vertex);
        d2d_cdt_edge_set_destination(cdt, &a, start_vertex + 1);

        *left_edge = a;
        d2d_cdt_edge_sym(right_edge, &a);

        return TRUE;
    }

    /* Three vertices, create a triangle. */
    if (vertex_count == 3)
    {
        struct d2d_cdt_edge_ref a, b, c;
        float det;

        if (!d2d_cdt_create_edge(cdt, &a))
            return FALSE;
        if (!d2d_cdt_create_edge(cdt, &b))
            return FALSE;
        d2d_cdt_edge_sym(&tmp, &a);
        d2d_cdt_splice(cdt, &tmp, &b);

        d2d_cdt_edge_set_origin(cdt, &a, start_vertex);
        d2d_cdt_edge_set_destination(cdt, &a, start_vertex + 1);
        d2d_cdt_edge_set_origin(cdt, &b, start_vertex + 1);
        d2d_cdt_edge_set_destination(cdt, &b, start_vertex + 2);

        det = d2d_cdt_ccw(cdt, start_vertex, start_vertex + 1, start_vertex + 2);
        if (det != 0.0f && !d2d_cdt_connect(cdt, &c, &b, &a))
            return FALSE;

        if (det < 0.0f)
        {
            d2d_cdt_edge_sym(left_edge, &c);
            *right_edge = c;
        }
        else
        {
            *left_edge = a;
            d2d_cdt_edge_sym(right_edge, &b);
        }

        return TRUE;
    }

    /* More than three vertices, divide. */
    cut = vertex_count / 2;
    if (!d2d_cdt_triangulate(cdt, start_vertex, cut, &left_outer, &left_inner))
        return FALSE;
    if (!d2d_cdt_triangulate(cdt, start_vertex + cut, vertex_count - cut, &right_inner, &right_outer))
        return FALSE;
    /* Merge the left and right parts. */
    if (!d2d_cdt_merge(cdt, &left_outer, &left_inner, &right_inner, &right_outer))
        return FALSE;

    *left_edge = left_outer;
    *right_edge = right_outer;
    return TRUE;
}

static int __cdecl d2d_cdt_compare_vertices(const void *a, const void *b)
{
    const D2D1_POINT_2F *p0 = a;
    const D2D1_POINT_2F *p1 = b;
    float diff = p0->x - p1->x;

    if (diff == 0.0f)
        diff = p0->y - p1->y;

    return diff == 0.0f ? 0 : (diff > 0.0f ? 1 : -1);
}

/* Determine whether a given point is inside the geometry, using the current
 * fill mode rule. */
static BOOL d2d_path_geometry_point_inside(const struct d2d_geometry *geometry,
        const D2D1_POINT_2F *probe, BOOL triangles_only)
{
    const D2D1_POINT_2F *p0, *p1;
    D2D1_POINT_2F v_p, v_probe;
    unsigned int score;
    size_t i, j, last;

    for (i = 0, score = 0; i < geometry->u.path.figure_count; ++i)
    {
        const struct d2d_figure *figure = &geometry->u.path.figures[i];

        if (probe->x < figure->bounds.left || probe->x > figure->bounds.right
                || probe->y < figure->bounds.top || probe->y > figure->bounds.bottom)
            continue;

        last = figure->vertex_count - 1;
        if (!triangles_only)
        {
            while (last && figure->vertex_types[last] == D2D_VERTEX_TYPE_NONE)
                --last;
        }
        p0 = &figure->vertices[last];
        for (j = 0; j <= last; ++j)
        {
            if (!triangles_only && figure->vertex_types[j] == D2D_VERTEX_TYPE_NONE)
                continue;

            p1 = &figure->vertices[j];
            d2d_point_subtract(&v_p, p1, p0);
            d2d_point_subtract(&v_probe, probe, p0);

            if ((probe->y < p0->y) != (probe->y < p1->y) && v_probe.x < v_p.x * (v_probe.y / v_p.y))
            {
                if (geometry->u.path.fill_mode == D2D1_FILL_MODE_ALTERNATE || (probe->y < p0->y))
                    ++score;
                else
                    --score;
            }

            p0 = p1;
        }
    }

    return geometry->u.path.fill_mode == D2D1_FILL_MODE_ALTERNATE ? score & 1 : score;
}

static BOOL d2d_path_geometry_add_fill_face(struct d2d_geometry *geometry, const struct d2d_cdt *cdt,
        const struct d2d_cdt_edge_ref *base_edge)
{
    struct d2d_cdt_edge_ref tmp;
    struct d2d_face *face;
    D2D1_POINT_2F probe;

    if (cdt->edges[base_edge->idx].flags & D2D_CDT_EDGE_FLAG_VISITED(base_edge->r))
        return TRUE;

    if (!d2d_array_reserve((void **)&geometry->fill.faces, &geometry->fill.faces_size,
            geometry->fill.face_count + 1, sizeof(*geometry->fill.faces)))
    {
        ERR("Failed to grow faces array.\n");
        return FALSE;
    }

    face = &geometry->fill.faces[geometry->fill.face_count];

    /* It may seem tempting to use the center of the face as probe origin, but
     * multiplying by powers of two works much better for preserving accuracy. */

    tmp = *base_edge;
    cdt->edges[tmp.idx].flags |= D2D_CDT_EDGE_FLAG_VISITED(tmp.r);
    face->v[0] = d2d_cdt_edge_origin(cdt, &tmp);
    probe.x = cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].x * 0.25f;
    probe.y = cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].y * 0.25f;

    d2d_cdt_edge_next_left(cdt, &tmp, &tmp);
    cdt->edges[tmp.idx].flags |= D2D_CDT_EDGE_FLAG_VISITED(tmp.r);
    face->v[1] = d2d_cdt_edge_origin(cdt, &tmp);
    probe.x += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].x * 0.25f;
    probe.y += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].y * 0.25f;

    d2d_cdt_edge_next_left(cdt, &tmp, &tmp);
    cdt->edges[tmp.idx].flags |= D2D_CDT_EDGE_FLAG_VISITED(tmp.r);
    face->v[2] = d2d_cdt_edge_origin(cdt, &tmp);
    probe.x += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].x * 0.50f;
    probe.y += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].y * 0.50f;

    if (d2d_cdt_leftof(cdt, face->v[2], base_edge) && d2d_path_geometry_point_inside(geometry, &probe, TRUE))
        ++geometry->fill.face_count;

    return TRUE;
}

static BOOL d2d_cdt_generate_faces(const struct d2d_cdt *cdt, struct d2d_geometry *geometry)
{
    struct d2d_cdt_edge_ref base_edge;
    size_t i;

    for (i = 0; i < cdt->edge_count; ++i)
    {
        if (cdt->edges[i].flags & D2D_CDT_EDGE_FLAG_FREED)
            continue;

        base_edge.idx = i;
        base_edge.r = 0;
        if (!d2d_path_geometry_add_fill_face(geometry, cdt, &base_edge))
            goto fail;
        d2d_cdt_edge_sym(&base_edge, &base_edge);
        if (!d2d_path_geometry_add_fill_face(geometry, cdt, &base_edge))
            goto fail;
    }

    return TRUE;

fail:
    free(geometry->fill.faces);
    geometry->fill.faces = NULL;
    geometry->fill.faces_size = 0;
    geometry->fill.face_count = 0;
    return FALSE;
}

static BOOL d2d_cdt_fixup(struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *base_edge)
{
    struct d2d_cdt_edge_ref candidate, next, new_base;
    unsigned int count = 0;

    d2d_cdt_edge_next_left(cdt, &next, base_edge);
    if (next.idx == base_edge->idx)
    {
        ERR("Degenerate face.\n");
        return FALSE;
    }

    candidate = next;
    while (d2d_cdt_edge_destination(cdt, &next) != d2d_cdt_edge_origin(cdt, base_edge))
    {
        if (d2d_cdt_incircle(cdt, d2d_cdt_edge_origin(cdt, base_edge), d2d_cdt_edge_destination(cdt, base_edge),
                d2d_cdt_edge_destination(cdt, &candidate), d2d_cdt_edge_destination(cdt, &next)))
            candidate = next;
        d2d_cdt_edge_next_left(cdt, &next, &next);
        ++count;
    }

    if (count > 1)
    {
        d2d_cdt_edge_next_left(cdt, &next, &candidate);
        if (d2d_cdt_edge_destination(cdt, &next) == d2d_cdt_edge_origin(cdt, base_edge))
            d2d_cdt_edge_next_left(cdt, &next, base_edge);
        else
            next = *base_edge;
        if (!d2d_cdt_connect(cdt, &new_base, &candidate, &next))
            return FALSE;
        if (!d2d_cdt_fixup(cdt, &new_base))
            return FALSE;
        d2d_cdt_edge_sym(&new_base, &new_base);
        if (!d2d_cdt_fixup(cdt, &new_base))
            return FALSE;
    }

    return TRUE;
}

static void d2d_cdt_cut_edges(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *end_edge,
        const struct d2d_cdt_edge_ref *base_edge, size_t start_vertex, size_t end_vertex)
{
    struct d2d_cdt_edge_ref next;
    float ccw;

    d2d_cdt_edge_next_left(cdt, &next, base_edge);
    if (d2d_cdt_edge_destination(cdt, &next) == end_vertex)
    {
        *end_edge = next;
        return;
    }

    ccw = d2d_cdt_ccw(cdt, d2d_cdt_edge_destination(cdt, &next), end_vertex, start_vertex);
    if (ccw == 0.0f)
    {
        *end_edge = next;
        return;
    }

    if (ccw > 0.0f)
        d2d_cdt_edge_next_left(cdt, &next, &next);

    d2d_cdt_edge_sym(&next, &next);
    d2d_cdt_cut_edges(cdt, end_edge, &next, start_vertex, end_vertex);
    d2d_cdt_destroy_edge(cdt, &next);
}

static BOOL d2d_cdt_insert_segment(struct d2d_cdt *cdt, struct d2d_geometry *geometry,
        const struct d2d_cdt_edge_ref *origin, struct d2d_cdt_edge_ref *edge, size_t end_vertex)
{
    struct d2d_cdt_edge_ref base_edge, current, new_origin, next, target;
    size_t current_destination, current_origin;

    for (current = *origin;; current = next)
    {
        d2d_cdt_edge_next_origin(cdt, &next, &current);

        current_destination = d2d_cdt_edge_destination(cdt, &current);
        if (current_destination == end_vertex)
        {
            d2d_cdt_edge_sym(edge, &current);
            return TRUE;
        }

        current_origin = d2d_cdt_edge_origin(cdt, &current);
        if (d2d_cdt_ccw(cdt, end_vertex, current_origin, current_destination) == 0.0f
                && (cdt->vertices[current_destination].x > cdt->vertices[current_origin].x)
                == (cdt->vertices[end_vertex].x > cdt->vertices[current_origin].x)
                && (cdt->vertices[current_destination].y > cdt->vertices[current_origin].y)
                == (cdt->vertices[end_vertex].y > cdt->vertices[current_origin].y))
        {
            d2d_cdt_edge_sym(&new_origin, &current);
            return d2d_cdt_insert_segment(cdt, geometry, &new_origin, edge, end_vertex);
        }

        if (d2d_cdt_rightof(cdt, end_vertex, &next) && d2d_cdt_leftof(cdt, end_vertex, &current))
        {
            d2d_cdt_edge_next_left(cdt, &base_edge, &current);

            d2d_cdt_edge_sym(&base_edge, &base_edge);
            d2d_cdt_cut_edges(cdt, &target, &base_edge, d2d_cdt_edge_origin(cdt, origin), end_vertex);
            d2d_cdt_destroy_edge(cdt, &base_edge);

            if (!d2d_cdt_connect(cdt, &base_edge, &target, &current))
                return FALSE;
            *edge = base_edge;
            if (!d2d_cdt_fixup(cdt, &base_edge))
                return FALSE;
            d2d_cdt_edge_sym(&base_edge, &base_edge);
            if (!d2d_cdt_fixup(cdt, &base_edge))
                return FALSE;

            if (d2d_cdt_edge_origin(cdt, edge) == end_vertex)
                return TRUE;
            new_origin = *edge;
            return d2d_cdt_insert_segment(cdt, geometry, &new_origin, edge, end_vertex);
        }

        if (next.idx == origin->idx)
        {
            ERR("Triangle not found.\n");
            return FALSE;
        }
    }
}

static BOOL d2d_cdt_insert_segments(struct d2d_cdt *cdt, struct d2d_geometry *geometry)
{
    size_t start_vertex, end_vertex, i, j, k;
    struct d2d_cdt_edge_ref edge, new_edge;
    const struct d2d_figure *figure;
    const D2D1_POINT_2F *p;
    BOOL found;

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        figure = &geometry->u.path.figures[i];

        if (figure->flags & D2D_FIGURE_FLAG_HOLLOW)
            continue;

        /* Degenerate figure. */
        if (figure->vertex_count < 2)
            continue;

        p = bsearch(&figure->vertices[figure->vertex_count - 1], cdt->vertices,
                geometry->fill.vertex_count, sizeof(*p), d2d_cdt_compare_vertices);
        start_vertex = p - cdt->vertices;

        for (k = 0, found = FALSE; k < cdt->edge_count; ++k)
        {
            if (cdt->edges[k].flags & D2D_CDT_EDGE_FLAG_FREED)
                continue;

            edge.idx = k;
            edge.r = 0;

            if (d2d_cdt_edge_origin(cdt, &edge) == start_vertex)
            {
                found = TRUE;
                break;
            }
            d2d_cdt_edge_sym(&edge, &edge);
            if (d2d_cdt_edge_origin(cdt, &edge) == start_vertex)
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            ERR("Edge not found.\n");
            return FALSE;
        }

        for (j = 0; j < figure->vertex_count; start_vertex = end_vertex, ++j)
        {
            p = bsearch(&figure->vertices[j], cdt->vertices,
                    geometry->fill.vertex_count, sizeof(*p), d2d_cdt_compare_vertices);
            end_vertex = p - cdt->vertices;

            if (start_vertex == end_vertex)
                continue;

            if (!d2d_cdt_insert_segment(cdt, geometry, &edge, &new_edge, end_vertex))
                return FALSE;
            edge = new_edge;
        }
    }

    return TRUE;
}

static BOOL d2d_geometry_intersections_add(struct d2d_geometry_intersections *i,
        const struct d2d_segment_idx *segment_idx, float t, D2D1_POINT_2F p)
{
    struct d2d_geometry_intersection *intersection;

    if (!d2d_array_reserve((void **)&i->intersections, &i->intersections_size,
            i->intersection_count + 1, sizeof(*i->intersections)))
    {
        ERR("Failed to grow intersections array.\n");
        return FALSE;
    }

    intersection = &i->intersections[i->intersection_count++];
    intersection->figure_idx = segment_idx->figure_idx;
    intersection->vertex_idx = segment_idx->vertex_idx;
    intersection->control_idx = segment_idx->control_idx;
    intersection->t = t;
    intersection->p = p;

    return TRUE;
}

static int __cdecl d2d_geometry_intersections_compare(const void *a, const void *b)
{
    const struct d2d_geometry_intersection *i0 = a;
    const struct d2d_geometry_intersection *i1 = b;

    if (i0->figure_idx != i1->figure_idx)
        return i0->figure_idx - i1->figure_idx;
    if (i0->vertex_idx != i1->vertex_idx)
        return i0->vertex_idx - i1->vertex_idx;
    if (i0->t != i1->t)
        return i0->t > i1->t ? 1 : -1;
    return 0;
}

static BOOL d2d_geometry_intersect_line_line(struct d2d_geometry *geometry,
        struct d2d_geometry_intersections *intersections, const struct d2d_segment_idx *idx_p,
        const struct d2d_segment_idx *idx_q)
{
    D2D1_POINT_2F v_p, v_q, v_qp, intersection;
    const D2D1_POINT_2F *p[2], *q[2];
    const struct d2d_figure *figure;
    float s, t, det;
    size_t next;

    figure = &geometry->u.path.figures[idx_p->figure_idx];
    p[0] = &figure->vertices[idx_p->vertex_idx];
    next = idx_p->vertex_idx + 1;
    if (next == figure->vertex_count)
        next = 0;
    p[1] = &figure->vertices[next];

    figure = &geometry->u.path.figures[idx_q->figure_idx];
    q[0] = &figure->vertices[idx_q->vertex_idx];
    next = idx_q->vertex_idx + 1;
    if (next == figure->vertex_count)
        next = 0;
    q[1] = &figure->vertices[next];

    d2d_point_subtract(&v_p, p[1], p[0]);
    d2d_point_subtract(&v_q, q[1], q[0]);
    d2d_point_subtract(&v_qp, p[0], q[0]);

    det = v_p.x * v_q.y - v_p.y * v_q.x;
    if (det == 0.0f)
        return TRUE;

    s = (v_q.x * v_qp.y - v_q.y * v_qp.x) / det;
    t = (v_p.x * v_qp.y - v_p.y * v_qp.x) / det;

    if (s < 0.0f || s > 1.0f || t < 0.0f || t > 1.0f)
        return TRUE;

    intersection.x = p[0]->x + v_p.x * s;
    intersection.y = p[0]->y + v_p.y * s;

    if (s > 0.0f && s < 1.0f && !d2d_geometry_intersections_add(intersections, idx_p, s, intersection))
        return FALSE;

    if (t > 0.0f && t < 1.0f && !d2d_geometry_intersections_add(intersections, idx_q, t, intersection))
        return FALSE;

    return TRUE;
}

static BOOL d2d_geometry_add_bezier_line_intersections(struct d2d_geometry *geometry,
        struct d2d_geometry_intersections *intersections, const struct d2d_segment_idx *idx_p,
        const D2D1_POINT_2F **p, const struct d2d_segment_idx *idx_q, const D2D1_POINT_2F **q, float s)
{
    D2D1_POINT_2F intersection;
    float t;

    d2d_point_calculate_bezier(&intersection, p[0], p[1], p[2], s);
    if (fabsf(q[1]->x - q[0]->x) > fabsf(q[1]->y - q[0]->y))
        t = (intersection.x - q[0]->x) / (q[1]->x - q[0]->x);
    else
        t = (intersection.y - q[0]->y) / (q[1]->y - q[0]->y);
    if (t < 0.0f || t > 1.0f)
        return TRUE;

    d2d_point_lerp(&intersection, q[0], q[1], t);

    if (s > 0.0f && s < 1.0f && !d2d_geometry_intersections_add(intersections, idx_p, s, intersection))
        return FALSE;

    if (t > 0.0f && t < 1.0f && !d2d_geometry_intersections_add(intersections, idx_q, t, intersection))
        return FALSE;

    return TRUE;
}

static BOOL d2d_geometry_intersect_bezier_line(struct d2d_geometry *geometry,
        struct d2d_geometry_intersections *intersections,
        const struct d2d_segment_idx *idx_p, const struct d2d_segment_idx *idx_q)
{
    const D2D1_POINT_2F *p[3], *q[2];
    const struct d2d_figure *figure;
    float y[3], root, theta, d, e;
    size_t next;

    figure = &geometry->u.path.figures[idx_p->figure_idx];
    p[0] = &figure->vertices[idx_p->vertex_idx];
    p[1] = &figure->bezier_controls[idx_p->control_idx];
    next = idx_p->vertex_idx + 1;
    p[2] = &figure->vertices[next];

    figure = &geometry->u.path.figures[idx_q->figure_idx];
    q[0] = &figure->vertices[idx_q->vertex_idx];
    next = idx_q->vertex_idx + 1;
    if (next == figure->vertex_count)
        next = 0;
    q[1] = &figure->vertices[next];

    /* Align the line with x-axis. */
    theta = -atan2f(q[1]->y - q[0]->y, q[1]->x - q[0]->x);
    y[0] = (p[0]->x - q[0]->x) * sinf(theta) + (p[0]->y - q[0]->y) * cosf(theta);
    y[1] = (p[1]->x - q[0]->x) * sinf(theta) + (p[1]->y - q[0]->y) * cosf(theta);
    y[2] = (p[2]->x - q[0]->x) * sinf(theta) + (p[2]->y - q[0]->y) * cosf(theta);

    /* Intersect the transformed curve with the x-axis.
     *
     * f(t) = (1 - t)²P₀ + 2(1 - t)tP₁ + t²P₂
     *      = (P₀ - 2P₁ + P₂)t² + 2(P₁ - P₀)t + P₀
     *
     * a = P₀ - 2P₁ + P₂
     * b = 2(P₁ - P₀)
     * c = P₀
     *
     * f(t) = 0
     * t = (-b ± √(b² - 4ac)) / 2a
     *   = (-2(P₁ - P₀) ± √((2(P₁ - P₀))² - 4((P₀ - 2P₁ + P₂)P₀))) / 2(P₀ - 2P₁ + P₂)
     *   = (2P₀ - 2P₁ ± √(4P₀² + 4P₁² - 8P₀P₁ - 4P₀² + 8P₀P₁ - 4P₀P₂)) / (2P₀ - 4P₁ + 2P₂)
     *   = (P₀ - P₁ ± √(P₁² - P₀P₂)) / (P₀ - 2P₁ + P₂) */

    d = y[0] - 2 * y[1] + y[2];
    if (d == 0.0f)
    {
        /* P₀ - 2P₁ + P₂ = 0
         * f(t) = (P₀ - 2P₁ + P₂)t² + 2(P₁ - P₀)t + P₀ = 0
         * t = -P₀ / 2(P₁ - P₀) */
        root = -y[0] / (2.0f * (y[1] - y[0]));
        if (root < 0.0f || root > 1.0f)
            return TRUE;

        return d2d_geometry_add_bezier_line_intersections(geometry, intersections, idx_p, p, idx_q, q, root);
    }

    e = y[1] * y[1] - y[0] * y[2];
    if (e < 0.0f)
        return TRUE;

    root = (y[0] - y[1] + sqrtf(e)) / d;
    if (root >= 0.0f && root <= 1.0f && !d2d_geometry_add_bezier_line_intersections(geometry,
            intersections, idx_p, p, idx_q, q, root))
        return FALSE;

    root = (y[0] - y[1] - sqrtf(e)) / d;
    if (root >= 0.0f && root <= 1.0f && !d2d_geometry_add_bezier_line_intersections(geometry,
            intersections, idx_p, p, idx_q, q, root))
        return FALSE;

    return TRUE;
}

static BOOL d2d_geometry_intersect_bezier_bezier(struct d2d_geometry *geometry,
        struct d2d_geometry_intersections *intersections,
        const struct d2d_segment_idx *idx_p, float start_p, float end_p,
        const struct d2d_segment_idx *idx_q, float start_q, float end_q)
{
    const D2D1_POINT_2F *p[3], *q[3];
    const struct d2d_figure *figure;
    D2D_RECT_F p_bounds, q_bounds;
    D2D1_POINT_2F intersection;
    float centre_p, centre_q;
    size_t next;

    figure = &geometry->u.path.figures[idx_p->figure_idx];
    p[0] = &figure->vertices[idx_p->vertex_idx];
    p[1] = &figure->bezier_controls[idx_p->control_idx];
    next = idx_p->vertex_idx + 1;
    p[2] = &figure->vertices[next];

    figure = &geometry->u.path.figures[idx_q->figure_idx];
    q[0] = &figure->vertices[idx_q->vertex_idx];
    q[1] = &figure->bezier_controls[idx_q->control_idx];
    next = idx_q->vertex_idx + 1;
    q[2] = &figure->vertices[next];

    d2d_rect_get_bezier_segment_bounds(&p_bounds, p[0], p[1], p[2], start_p, end_p);
    d2d_rect_get_bezier_segment_bounds(&q_bounds, q[0], q[1], q[2], start_q, end_q);

    if (!d2d_rect_check_overlap(&p_bounds, &q_bounds))
        return TRUE;

    centre_p = (start_p + end_p) / 2.0f;
    centre_q = (start_q + end_q) / 2.0f;

    if (end_p - start_p < 1e-3f)
    {
        d2d_point_calculate_bezier(&intersection, p[0], p[1], p[2], centre_p);
        if (start_p > 0.0f && end_p < 1.0f && !d2d_geometry_intersections_add(intersections,
                idx_p, centre_p, intersection))
            return FALSE;
        if (start_q > 0.0f && end_q < 1.0f && !d2d_geometry_intersections_add(intersections,
                idx_q, centre_q, intersection))
            return FALSE;
        return TRUE;
    }

    if (!d2d_geometry_intersect_bezier_bezier(geometry, intersections,
            idx_p, start_p, centre_p, idx_q, start_q, centre_q))
        return FALSE;
    if (!d2d_geometry_intersect_bezier_bezier(geometry, intersections,
            idx_p, start_p, centre_p, idx_q, centre_q, end_q))
        return FALSE;
    if (!d2d_geometry_intersect_bezier_bezier(geometry, intersections,
            idx_p, centre_p, end_p, idx_q, start_q, centre_q))
        return FALSE;
    if (!d2d_geometry_intersect_bezier_bezier(geometry, intersections,
            idx_p, centre_p, end_p, idx_q, centre_q, end_q))
        return FALSE;

    return TRUE;
}

static BOOL d2d_geometry_apply_intersections(struct d2d_geometry *geometry,
        struct d2d_geometry_intersections *intersections)
{
    size_t vertex_offset, control_offset, next, i;
    struct d2d_geometry_intersection *inter;
    enum d2d_vertex_type vertex_type;
    const D2D1_POINT_2F *p[3];
    struct d2d_figure *figure;
    D2D1_POINT_2F q[2];
    float t, t_prev;

    for (i = 0; i < intersections->intersection_count; ++i)
    {
        inter = &intersections->intersections[i];
        if (!i || inter->figure_idx != intersections->intersections[i - 1].figure_idx)
            vertex_offset = control_offset = 0;

        figure = &geometry->u.path.figures[inter->figure_idx];
        vertex_type = figure->vertex_types[inter->vertex_idx + vertex_offset];
        if (!d2d_vertex_type_is_bezier(vertex_type))
        {
            if (!d2d_figure_insert_vertex(&geometry->u.path.figures[inter->figure_idx],
                    inter->vertex_idx + vertex_offset + 1, inter->p))
                return FALSE;
            ++vertex_offset;
            continue;
        }

        t = inter->t;
        if (i && inter->figure_idx == intersections->intersections[i - 1].figure_idx
                && inter->vertex_idx == intersections->intersections[i - 1].vertex_idx)
        {
            t_prev = intersections->intersections[i - 1].t;
            if (t - t_prev < 1e-3f)
            {
                inter->t = intersections->intersections[i - 1].t;
                continue;
            }
            t = (t - t_prev) / (1.0f - t_prev);
        }

        p[0] = &figure->vertices[inter->vertex_idx + vertex_offset];
        p[1] = &figure->bezier_controls[inter->control_idx + control_offset];
        next = inter->vertex_idx + vertex_offset + 1;
        p[2] = &figure->vertices[next];

        d2d_point_lerp(&q[0], p[0], p[1], t);
        d2d_point_lerp(&q[1], p[1], p[2], t);

        figure->bezier_controls[inter->control_idx + control_offset] = q[0];
        if (!(d2d_figure_insert_bezier_controls(figure, inter->control_idx + control_offset + 1, 1, &q[1])))
            return FALSE;
        ++control_offset;

        if (!(d2d_figure_insert_vertex(figure, inter->vertex_idx + vertex_offset + 1, inter->p)))
            return FALSE;
        figure->vertex_types[inter->vertex_idx + vertex_offset + 1] = D2D_VERTEX_TYPE_SPLIT_BEZIER;
        ++vertex_offset;
    }

    return TRUE;
}

/* Intersect the geometry's segments with themselves. This uses the
 * straightforward approach of testing everything against everything, but
 * there certainly exist more scalable algorithms for this. */
static BOOL d2d_geometry_intersect_self(struct d2d_geometry *geometry)
{
    struct d2d_geometry_intersections intersections = {0};
    const struct d2d_figure *figure_p, *figure_q;
    struct d2d_segment_idx idx_p, idx_q;
    enum d2d_vertex_type type_p, type_q;
    BOOL ret = FALSE;
    size_t max_q;

    if (!geometry->u.path.figure_count)
        return TRUE;

    for (idx_p.figure_idx = 0; idx_p.figure_idx < geometry->u.path.figure_count; ++idx_p.figure_idx)
    {
        figure_p = &geometry->u.path.figures[idx_p.figure_idx];
        idx_p.control_idx = 0;
        for (idx_p.vertex_idx = 0; idx_p.vertex_idx < figure_p->vertex_count; ++idx_p.vertex_idx)
        {
            if ((type_p = figure_p->vertex_types[idx_p.vertex_idx]) == D2D_VERTEX_TYPE_END)
                continue;

            for (idx_q.figure_idx = 0; idx_q.figure_idx <= idx_p.figure_idx; ++idx_q.figure_idx)
            {
                figure_q = &geometry->u.path.figures[idx_q.figure_idx];
                if (idx_q.figure_idx != idx_p.figure_idx)
                {
                    if (!d2d_rect_check_overlap(&figure_p->bounds, &figure_q->bounds))
                        continue;
                    if ((max_q = figure_q->vertex_count)
                            && figure_q->vertex_types[max_q - 1] == D2D_VERTEX_TYPE_END)
                        --max_q;
                }
                else
                {
                    max_q = idx_p.vertex_idx;
                }

                idx_q.control_idx = 0;
                for (idx_q.vertex_idx = 0; idx_q.vertex_idx < max_q; ++idx_q.vertex_idx)
                {
                    type_q = figure_q->vertex_types[idx_q.vertex_idx];
                    if (d2d_vertex_type_is_bezier(type_q))
                    {
                        if (d2d_vertex_type_is_bezier(type_p))
                        {
                            if (!d2d_geometry_intersect_bezier_bezier(geometry, &intersections,
                                    &idx_p, 0.0f, 1.0f, &idx_q, 0.0f, 1.0f))
                                goto done;
                        }
                        else
                        {
                            if (!d2d_geometry_intersect_bezier_line(geometry, &intersections, &idx_q, &idx_p))
                                goto done;
                        }
                        ++idx_q.control_idx;
                    }
                    else
                    {
                        if (d2d_vertex_type_is_bezier(type_p))
                        {
                            if (!d2d_geometry_intersect_bezier_line(geometry, &intersections, &idx_p, &idx_q))
                                goto done;
                        }
                        else
                        {
                            if (!d2d_geometry_intersect_line_line(geometry, &intersections, &idx_p, &idx_q))
                                goto done;
                        }
                    }
                }
            }
            if (d2d_vertex_type_is_bezier(type_p))
                ++idx_p.control_idx;
        }
    }

    qsort(intersections.intersections, intersections.intersection_count,
            sizeof(*intersections.intersections), d2d_geometry_intersections_compare);
    ret = d2d_geometry_apply_intersections(geometry, &intersections);

done:
    free(intersections.intersections);
    return ret;
}

static HRESULT d2d_path_geometry_triangulate(struct d2d_geometry *geometry)
{
    struct d2d_cdt_edge_ref left_edge, right_edge;
    size_t vertex_count, i, j;
    struct d2d_cdt cdt = {0};
    D2D1_POINT_2F *vertices;
#ifdef __i386__
    unsigned int control_word_x87, mask = 0;
#endif

    for (i = 0, vertex_count = 0; i < geometry->u.path.figure_count; ++i)
    {
        if (geometry->u.path.figures[i].flags & D2D_FIGURE_FLAG_HOLLOW)
            continue;
        vertex_count += geometry->u.path.figures[i].vertex_count;
    }

    if (vertex_count < 3)
    {
        WARN("Geometry has %lu vertices.\n", (long)vertex_count);
        return S_OK;
    }

    if (!(vertices = calloc(vertex_count, sizeof(*vertices))))
        return E_OUTOFMEMORY;

    for (i = 0, j = 0; i < geometry->u.path.figure_count; ++i)
    {
        if (geometry->u.path.figures[i].flags & D2D_FIGURE_FLAG_HOLLOW)
            continue;
        memcpy(&vertices[j], geometry->u.path.figures[i].vertices,
                geometry->u.path.figures[i].vertex_count * sizeof(*vertices));
        j += geometry->u.path.figures[i].vertex_count;
    }

    /* Sort vertices, eliminate duplicates. */
    qsort(vertices, vertex_count, sizeof(*vertices), d2d_cdt_compare_vertices);
    for (i = 1; i < vertex_count; ++i)
    {
        if (!memcmp(&vertices[i - 1], &vertices[i], sizeof(*vertices)))
        {
            --vertex_count;
            memmove(&vertices[i], &vertices[i + 1], (vertex_count - i) * sizeof(*vertices));
            --i;
        }
    }

    if (vertex_count < 3)
    {
        WARN("Geometry has %lu vertices after eliminating duplicates.\n", (long)vertex_count);
        free(vertices);
        return S_OK;
    }

    geometry->fill.vertices = vertices;
    geometry->fill.vertex_count = vertex_count;

    cdt.free_edge = ~0u;
    cdt.vertices = vertices;

#ifdef __i386__
    control_word_x87 = _controlfp(0, 0);
    _controlfp(_PC_24, mask = _MCW_PC);
#endif
    if (!d2d_cdt_triangulate(&cdt, 0, vertex_count, &left_edge, &right_edge))
        goto fail;
    if (!d2d_cdt_insert_segments(&cdt, geometry))
        goto fail;
#ifdef __i386__
    _controlfp(control_word_x87, _MCW_PC);
    mask = 0;
#endif

    if (!d2d_cdt_generate_faces(&cdt, geometry))
        goto fail;

    free(cdt.edges);
    return S_OK;

fail:
    geometry->fill.vertices = NULL;
    geometry->fill.vertex_count = 0;
    free(vertices);
    free(cdt.edges);
#ifdef __i386__
    if (mask) _controlfp(control_word_x87, mask);
#endif
    return E_FAIL;
}

static BOOL d2d_path_geometry_add_figure(struct d2d_geometry *geometry)
{
    struct d2d_figure *figure;

    if (!d2d_array_reserve((void **)&geometry->u.path.figures, &geometry->u.path.figures_size,
            geometry->u.path.figure_count + 1, sizeof(*geometry->u.path.figures)))
    {
        ERR("Failed to grow figures array.\n");
        return FALSE;
    }

    figure = &geometry->u.path.figures[geometry->u.path.figure_count];
    memset(figure, 0, sizeof(*figure));
    figure->bounds.left = FLT_MAX;
    figure->bounds.top = FLT_MAX;
    figure->bounds.right = -FLT_MAX;
    figure->bounds.bottom = -FLT_MAX;

    ++geometry->u.path.figure_count;
    return TRUE;
}

static BOOL d2d_geometry_outline_add_join(struct d2d_geometry *geometry,
        const D2D1_POINT_2F *prev, const D2D1_POINT_2F *p0, const D2D1_POINT_2F *next)
{
    static const D2D1_POINT_2F origin = {0.0f, 0.0f};
    struct d2d_outline_vertex *v;
    struct d2d_face *f;
    size_t base_idx;
    float ccw;

    if (!d2d_array_reserve((void **)&geometry->outline.vertices, &geometry->outline.vertices_size,
            geometry->outline.vertex_count + 4, sizeof(*geometry->outline.vertices)))
    {
        ERR("Failed to grow outline vertices array.\n");
        return FALSE;
    }
    base_idx = geometry->outline.vertex_count;
    v = &geometry->outline.vertices[base_idx];

    if (!d2d_array_reserve((void **)&geometry->outline.faces, &geometry->outline.faces_size,
            geometry->outline.face_count + 2, sizeof(*geometry->outline.faces)))
    {
        ERR("Failed to grow outline faces array.\n");
        return FALSE;
    }
    f = &geometry->outline.faces[geometry->outline.face_count];

    ccw = d2d_point_ccw(&origin, prev, next);
    if (ccw == 0.0f)
    {
        d2d_outline_vertex_set(&v[0], p0->x, p0->y, -prev->x, -prev->y, -prev->x, -prev->y);
        d2d_outline_vertex_set(&v[1], p0->x, p0->y,  prev->x,  prev->y,  prev->x,  prev->y);
        d2d_outline_vertex_set(&v[2], p0->x + 25.0f * -prev->x, p0->y + 25.0f * -prev->y,
                 prev->x,  prev->y,  prev->x,  prev->y);
        d2d_outline_vertex_set(&v[3], p0->x + 25.0f * -prev->x, p0->y + 25.0f * -prev->y,
                -prev->x, -prev->y, -prev->x, -prev->y);
    }
    else if (ccw < 0.0f)
    {
        d2d_outline_vertex_set(&v[0], p0->x, p0->y,  next->x,  next->y, -prev->x, -prev->y);
        d2d_outline_vertex_set(&v[1], p0->x, p0->y, -next->x, -next->y, -next->x, -next->y);
        d2d_outline_vertex_set(&v[2], p0->x, p0->y, -next->x, -next->y,  prev->x,  prev->y);
        d2d_outline_vertex_set(&v[3], p0->x, p0->y,  prev->x,  prev->y,  prev->x,  prev->y);
    }
    else
    {
        d2d_outline_vertex_set(&v[0], p0->x, p0->y,  prev->x,  prev->y, -next->x, -next->y);
        d2d_outline_vertex_set(&v[1], p0->x, p0->y, -prev->x, -prev->y, -prev->x, -prev->y);
        d2d_outline_vertex_set(&v[2], p0->x, p0->y, -prev->x, -prev->y,  next->x,  next->y);
        d2d_outline_vertex_set(&v[3], p0->x, p0->y,  next->x,  next->y,  next->x,  next->y);
    }
    geometry->outline.vertex_count += 4;

    d2d_face_set(&f[0], base_idx + 1, base_idx + 0, base_idx + 2);
    d2d_face_set(&f[1], base_idx + 2, base_idx + 0, base_idx + 3);
    geometry->outline.face_count += 2;

    return TRUE;
}

static BOOL d2d_geometry_outline_add_line_segment(struct d2d_geometry *geometry,
        const D2D1_POINT_2F *p0, const D2D1_POINT_2F *next)
{
    struct d2d_outline_vertex *v;
    D2D1_POINT_2F q_next;
    struct d2d_face *f;
    size_t base_idx;

    if (!d2d_array_reserve((void **)&geometry->outline.vertices, &geometry->outline.vertices_size,
            geometry->outline.vertex_count + 4, sizeof(*geometry->outline.vertices)))
    {
        ERR("Failed to grow outline vertices array.\n");
        return FALSE;
    }
    base_idx = geometry->outline.vertex_count;
    v = &geometry->outline.vertices[base_idx];

    if (!d2d_array_reserve((void **)&geometry->outline.faces, &geometry->outline.faces_size,
            geometry->outline.face_count + 2, sizeof(*geometry->outline.faces)))
    {
        ERR("Failed to grow outline faces array.\n");
        return FALSE;
    }
    f = &geometry->outline.faces[geometry->outline.face_count];

    d2d_point_subtract(&q_next, next, p0);
    d2d_point_normalise(&q_next);

    d2d_outline_vertex_set(&v[0], p0->x, p0->y,  q_next.x,  q_next.y,  q_next.x,  q_next.y);
    d2d_outline_vertex_set(&v[1], p0->x, p0->y, -q_next.x, -q_next.y, -q_next.x, -q_next.y);
    d2d_outline_vertex_set(&v[2], next->x, next->y,  q_next.x,  q_next.y,  q_next.x,  q_next.y);
    d2d_outline_vertex_set(&v[3], next->x, next->y, -q_next.x, -q_next.y, -q_next.x, -q_next.y);
    geometry->outline.vertex_count += 4;

    d2d_face_set(&f[0], base_idx + 0, base_idx + 1, base_idx + 2);
    d2d_face_set(&f[1], base_idx + 2, base_idx + 1, base_idx + 3);
    geometry->outline.face_count += 2;

    return TRUE;
}

static BOOL d2d_geometry_outline_add_bezier_segment(struct d2d_geometry *geometry,
        const D2D1_POINT_2F *p0, const D2D1_POINT_2F *p1, const D2D1_POINT_2F *p2)
{
    struct d2d_curve_outline_vertex *b;
    D2D1_POINT_2F r0, r1, r2;
    D2D1_POINT_2F q0, q1, q2;
    struct d2d_face *f;
    size_t base_idx;

    if (!d2d_array_reserve((void **)&geometry->outline.beziers, &geometry->outline.beziers_size,
            geometry->outline.bezier_count + 7, sizeof(*geometry->outline.beziers)))
    {
        ERR("Failed to grow outline beziers array.\n");
        return FALSE;
    }
    base_idx = geometry->outline.bezier_count;
    b = &geometry->outline.beziers[base_idx];

    if (!d2d_array_reserve((void **)&geometry->outline.bezier_faces, &geometry->outline.bezier_faces_size,
            geometry->outline.bezier_face_count + 5, sizeof(*geometry->outline.bezier_faces)))
    {
        ERR("Failed to grow outline faces array.\n");
        return FALSE;
    }
    f = &geometry->outline.bezier_faces[geometry->outline.bezier_face_count];

    d2d_point_lerp(&q0, p0, p1, 0.5f);
    d2d_point_lerp(&q1, p1, p2, 0.5f);
    d2d_point_lerp(&q2, &q0, &q1, 0.5f);

    d2d_point_subtract(&r0, &q0, p0);
    d2d_point_subtract(&r1, &q1, &q0);
    d2d_point_subtract(&r2, p2, &q1);

    d2d_point_normalise(&r0);
    d2d_point_normalise(&r1);
    d2d_point_normalise(&r2);

    if (d2d_point_ccw(p0, p1, p2) > 0.0f)
    {
        d2d_point_scale(&r0, -1.0f);
        d2d_point_scale(&r1, -1.0f);
        d2d_point_scale(&r2, -1.0f);
    }

    d2d_curve_outline_vertex_set(&b[0],  p0, p0, p1, p2,  r0.x,  r0.y,  r0.x,  r0.y);
    d2d_curve_outline_vertex_set(&b[1],  p0, p0, p1, p2, -r0.x, -r0.y, -r0.x, -r0.y);
    d2d_curve_outline_vertex_set(&b[2], &q0, p0, p1, p2,  r0.x,  r0.y,  r1.x,  r1.y);
    d2d_curve_outline_vertex_set(&b[3], &q2, p0, p1, p2, -r1.x, -r1.y, -r1.x, -r1.y);
    d2d_curve_outline_vertex_set(&b[4], &q1, p0, p1, p2,  r1.x,  r1.y,  r2.x,  r2.y);
    d2d_curve_outline_vertex_set(&b[5],  p2, p0, p1, p2, -r2.x, -r2.y, -r2.x, -r2.y);
    d2d_curve_outline_vertex_set(&b[6],  p2, p0, p1, p2,  r2.x,  r2.y,  r2.x,  r2.y);
    geometry->outline.bezier_count += 7;

    d2d_face_set(&f[0], base_idx + 0, base_idx + 1, base_idx + 2);
    d2d_face_set(&f[1], base_idx + 2, base_idx + 1, base_idx + 3);
    d2d_face_set(&f[2], base_idx + 3, base_idx + 4, base_idx + 2);
    d2d_face_set(&f[3], base_idx + 5, base_idx + 4, base_idx + 3);
    d2d_face_set(&f[4], base_idx + 5, base_idx + 6, base_idx + 4);
    geometry->outline.bezier_face_count += 5;

    return TRUE;
}

static BOOL d2d_geometry_outline_add_arc_quadrant(struct d2d_geometry *geometry,
        const D2D1_POINT_2F *p0, const D2D1_POINT_2F *p1, const D2D1_POINT_2F *p2)
{
    struct d2d_curve_outline_vertex *a;
    D2D1_POINT_2F r0, r1;
    struct d2d_face *f;
    size_t base_idx;

    if (!d2d_array_reserve((void **)&geometry->outline.arcs, &geometry->outline.arcs_size,
            geometry->outline.arc_count + 5, sizeof(*geometry->outline.arcs)))
    {
        ERR("Failed to grow outline arcs array.\n");
        return FALSE;
    }
    base_idx = geometry->outline.arc_count;
    a = &geometry->outline.arcs[base_idx];

    if (!d2d_array_reserve((void **)&geometry->outline.arc_faces, &geometry->outline.arc_faces_size,
            geometry->outline.arc_face_count + 3, sizeof(*geometry->outline.arc_faces)))
    {
        ERR("Failed to grow outline faces array.\n");
        return FALSE;
    }
    f = &geometry->outline.arc_faces[geometry->outline.arc_face_count];

    d2d_point_subtract(&r0, p1, p0);
    d2d_point_subtract(&r1, p2, p1);

    d2d_point_normalise(&r0);
    d2d_point_normalise(&r1);

    if (d2d_point_ccw(p0, p1, p2) > 0.0f)
    {
        d2d_point_scale(&r0, -1.0f);
        d2d_point_scale(&r1, -1.0f);
    }

    d2d_curve_outline_vertex_set(&a[0],  p0, p0, p1, p2,  r0.x,  r0.y,  r0.x,  r0.y);
    d2d_curve_outline_vertex_set(&a[1],  p0, p0, p1, p2, -r0.x, -r0.y, -r0.x, -r0.y);
    d2d_curve_outline_vertex_set(&a[2],  p1, p0, p1, p2,  r0.x,  r0.y,  r1.x,  r1.y);
    d2d_curve_outline_vertex_set(&a[3],  p2, p0, p1, p2, -r1.x, -r1.y, -r1.x, -r1.y);
    d2d_curve_outline_vertex_set(&a[4],  p2, p0, p1, p2,  r1.x,  r1.y,  r1.x,  r1.y);
    geometry->outline.arc_count += 5;

    d2d_face_set(&f[0], base_idx + 0, base_idx + 1, base_idx + 2);
    d2d_face_set(&f[1], base_idx + 2, base_idx + 1, base_idx + 3);
    d2d_face_set(&f[2], base_idx + 2, base_idx + 4, base_idx + 3);
    geometry->outline.arc_face_count += 3;

    return TRUE;
}

static BOOL d2d_geometry_add_figure_outline(struct d2d_geometry *geometry,
        struct d2d_figure *figure, D2D1_FIGURE_END figure_end)
{
    const D2D1_POINT_2F *prev, *p0, *p1, *next, *next_prev;
    size_t bezier_idx, i, vertex_count;
    enum d2d_vertex_type type;

    if (!(vertex_count = figure->vertex_count))
        return TRUE;

    p0 = &figure->vertices[0];
    if (figure_end == D2D1_FIGURE_END_CLOSED)
    {
        if (figure->vertex_types[vertex_count - 1] == D2D_VERTEX_TYPE_END && !--vertex_count)
            return TRUE;

        /* In case of a CLOSED path, a join between first and last vertex is
         * required. */
        if (d2d_vertex_type_is_bezier(figure->vertex_types[vertex_count - 1]))
            prev = &figure->bezier_controls[figure->bezier_control_count - 1];
        else
            prev = &figure->vertices[vertex_count - 1];
    }
    else
    {
        if (!--vertex_count)
            return TRUE;
        prev = p0;
    }

    for (i = 0, bezier_idx = 0; i < vertex_count; ++i)
    {
        if ((type = figure->vertex_types[i]) == D2D_VERTEX_TYPE_NONE)
        {
            prev = next_prev = &figure->vertices[i];
            continue;
        }

        /* next: tangent along next segment, at p0.
         * p1: next vertex. */
        if (d2d_vertex_type_is_bezier(type))
        {
            next_prev = next = &figure->bezier_controls[bezier_idx++];
            /* type BEZIER implies i + 1 < figure->vertex_count. */
            p1 = &figure->vertices[i + 1];

            if (!d2d_geometry_outline_add_bezier_segment(geometry, p0, next, p1))
            {
                ERR("Failed to add bezier segment.\n");
                return FALSE;
            }
        }
        else
        {
            if (i + 1 == figure->vertex_count)
                next = p1 = &figure->vertices[0];
            else
                next = p1 = &figure->vertices[i + 1];
            next_prev = p0;

            if (!d2d_geometry_outline_add_line_segment(geometry, p0, p1))
            {
                ERR("Failed to add line segment.\n");
                return FALSE;
            }
        }

        if (i || figure_end == D2D1_FIGURE_END_CLOSED)
        {
            D2D1_POINT_2F q_next, q_prev;

            d2d_point_subtract(&q_prev, prev, p0);
            d2d_point_subtract(&q_next, next, p0);

            d2d_point_normalise(&q_prev);
            d2d_point_normalise(&q_next);

            if (!d2d_geometry_outline_add_join(geometry, &q_prev, p0, &q_next))
            {
                ERR("Failed to add join.\n");
                return FALSE;
            }
        }

        p0 = p1;
        prev = next_prev;
    }

    return TRUE;
}

static BOOL d2d_geometry_fill_add_arc_triangle(struct d2d_geometry *geometry,
        const D2D1_POINT_2F *p0, const D2D1_POINT_2F *p1, const D2D1_POINT_2F *p2)
{
    struct d2d_curve_vertex *a;

    if (!d2d_array_reserve((void **)&geometry->fill.arc_vertices, &geometry->fill.arc_vertices_size,
            geometry->fill.arc_vertex_count + 3, sizeof(*geometry->fill.arc_vertices)))
        return FALSE;

    a = &geometry->fill.arc_vertices[geometry->fill.arc_vertex_count];
    d2d_curve_vertex_set(&a[0], p0, 0.0f, 1.0f, -1.0f);
    d2d_curve_vertex_set(&a[1], p1, 1.0f, 1.0f, -1.0f);
    d2d_curve_vertex_set(&a[2], p2, 1.0f, 0.0f, -1.0f);
    geometry->fill.arc_vertex_count += 3;

    return TRUE;
}

static void d2d_geometry_cleanup(struct d2d_geometry *geometry)
{
    free(geometry->outline.arc_faces);
    free(geometry->outline.arcs);
    free(geometry->outline.bezier_faces);
    free(geometry->outline.beziers);
    free(geometry->outline.faces);
    free(geometry->outline.vertices);
    free(geometry->fill.arc_vertices);
    free(geometry->fill.bezier_vertices);
    free(geometry->fill.faces);
    free(geometry->fill.vertices);
    ID2D1Factory_Release(geometry->factory);
}

static void d2d_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        const D2D1_MATRIX_3X2_F *transform, const struct ID2D1GeometryVtbl *vtbl)
{
    geometry->ID2D1Geometry_iface.lpVtbl = vtbl;
    geometry->refcount = 1;
    ID2D1Factory_AddRef(geometry->factory = factory);
    geometry->transform = *transform;
}

static inline struct d2d_geometry *impl_from_ID2D1GeometrySink(ID2D1GeometrySink *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, u.path.ID2D1GeometrySink_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_sink_QueryInterface(ID2D1GeometrySink *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1GeometrySink)
            || IsEqualGUID(iid, &IID_ID2D1SimplifiedGeometrySink)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1GeometrySink_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_geometry_sink_AddRef(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1Geometry_AddRef(&geometry->ID2D1Geometry_iface);
}

static ULONG STDMETHODCALLTYPE d2d_geometry_sink_Release(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1Geometry_Release(&geometry->ID2D1Geometry_iface);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_SetFillMode(ID2D1GeometrySink *iface, D2D1_FILL_MODE mode)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    if (geometry->u.path.state == D2D_GEOMETRY_STATE_CLOSED)
        return;
    geometry->u.path.fill_mode = mode;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_SetSegmentFlags(ID2D1GeometrySink *iface, D2D1_PATH_SEGMENT flags)
{
    TRACE("iface %p, flags %#x.\n", iface, flags);

    if (flags != D2D1_PATH_SEGMENT_NONE)
        FIXME("Ignoring flags %#x.\n", flags);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_BeginFigure(ID2D1GeometrySink *iface,
        D2D1_POINT_2F start_point, D2D1_FIGURE_BEGIN figure_begin)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    struct d2d_figure *figure;

    TRACE("iface %p, start_point %s, figure_begin %#x.\n",
            iface, debug_d2d_point_2f(&start_point), figure_begin);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_OPEN)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    if (!d2d_path_geometry_add_figure(geometry))
    {
        ERR("Failed to add figure.\n");
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    figure = &geometry->u.path.figures[geometry->u.path.figure_count - 1];
    if (figure_begin == D2D1_FIGURE_BEGIN_HOLLOW)
        figure->flags |= D2D_FIGURE_FLAG_HOLLOW;

    if (!d2d_figure_add_vertex(figure, start_point))
    {
        ERR("Failed to add vertex.\n");
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    geometry->u.path.state = D2D_GEOMETRY_STATE_FIGURE;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddLines(ID2D1GeometrySink *iface,
        const D2D1_POINT_2F *points, UINT32 count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    struct d2d_figure *figure = &geometry->u.path.figures[geometry->u.path.figure_count - 1];
    unsigned int i;

    TRACE("iface %p, points %p, count %u.\n", iface, points, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    for (i = 0; i < count; ++i)
    {
        figure->vertex_types[figure->vertex_count - 1] = D2D_VERTEX_TYPE_LINE;
        if (!d2d_figure_add_vertex(figure, points[i]))
        {
            ERR("Failed to add vertex.\n");
            return;
        }
    }

    geometry->u.path.segment_count += count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddBeziers(ID2D1GeometrySink *iface,
        const D2D1_BEZIER_SEGMENT *beziers, UINT32 count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    struct d2d_figure *figure = &geometry->u.path.figures[geometry->u.path.figure_count - 1];
    D2D1_POINT_2F p;
    unsigned int i;

    TRACE("iface %p, beziers %p, count %u.\n", iface, beziers, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    for (i = 0; i < count; ++i)
    {
        D2D1_RECT_F bezier_bounds;

        if (!d2d_figure_add_original_bezier_controls(figure, 1, &beziers[i].point1)
                || !d2d_figure_add_original_bezier_controls(figure, 1, &beziers[i].point2))
        {
            ERR("Failed to add cubic Bézier controls.\n");
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
            return;
        }

        /* FIXME: This tries to approximate a cubic Bézier with a quadratic one. */
        p.x = (beziers[i].point1.x + beziers[i].point2.x) * 0.75f;
        p.y = (beziers[i].point1.y + beziers[i].point2.y) * 0.75f;
        p.x -= (figure->vertices[figure->vertex_count - 1].x + beziers[i].point3.x) * 0.25f;
        p.y -= (figure->vertices[figure->vertex_count - 1].y + beziers[i].point3.y) * 0.25f;
        figure->vertex_types[figure->vertex_count - 1] = D2D_VERTEX_TYPE_BEZIER;

        d2d_rect_get_bezier_bounds(&bezier_bounds, &figure->vertices[figure->vertex_count - 1],
                &p, &beziers[i].point3);

        if (!d2d_figure_add_bezier_controls(figure, 1, &p))
        {
            ERR("Failed to add bezier control.\n");
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
            return;
        }

        if (!d2d_figure_add_vertex(figure, beziers[i].point3))
        {
            ERR("Failed to add bezier vertex.\n");
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
            return;
        }

        d2d_rect_union(&figure->bounds, &bezier_bounds);
    }

    geometry->u.path.segment_count += count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_EndFigure(ID2D1GeometrySink *iface, D2D1_FIGURE_END figure_end)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    struct d2d_figure *figure;

    TRACE("iface %p, figure_end %#x.\n", iface, figure_end);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    figure = &geometry->u.path.figures[geometry->u.path.figure_count - 1];
    if (memcmp(&figure->vertices[0], &figure->vertices[figure->vertex_count - 1], sizeof(*figure->vertices)))
        figure->vertex_types[figure->vertex_count - 1] = D2D_VERTEX_TYPE_LINE;
    else
        figure->vertex_types[figure->vertex_count - 1] = D2D_VERTEX_TYPE_END;
    if (figure_end == D2D1_FIGURE_END_CLOSED)
    {
        ++geometry->u.path.segment_count;
        figure->flags |= D2D_FIGURE_FLAG_CLOSED;
    }

    if (!d2d_geometry_add_figure_outline(geometry, figure, figure_end))
    {
        ERR("Failed to add figure outline.\n");
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    geometry->u.path.state = D2D_GEOMETRY_STATE_OPEN;
}

static void d2d_path_geometry_free_figures(struct d2d_geometry *geometry)
{
    size_t i;

    if (!geometry->u.path.figures)
        return;

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        free(geometry->u.path.figures[i].original_bezier_controls);
        free(geometry->u.path.figures[i].bezier_controls);
        free(geometry->u.path.figures[i].vertices);
    }
    free(geometry->u.path.figures);
    geometry->u.path.figures = NULL;
    geometry->u.path.figures_size = 0;
}

static BOOL d2d_geometry_get_bezier_segment_idx(struct d2d_geometry *geometry, struct d2d_segment_idx *idx, BOOL next)
{
    if (next)
    {
        ++idx->vertex_idx;
        ++idx->control_idx;
    }

    for (; idx->figure_idx < geometry->u.path.figure_count; ++idx->figure_idx, idx->vertex_idx = idx->control_idx = 0)
    {
        struct d2d_figure *figure = &geometry->u.path.figures[idx->figure_idx];

        if (!figure->bezier_control_count || figure->flags & D2D_FIGURE_FLAG_HOLLOW)
            continue;

        for (; idx->vertex_idx < figure->vertex_count; ++idx->vertex_idx)
        {
            if (d2d_vertex_type_is_bezier(figure->vertex_types[idx->vertex_idx]))
                return TRUE;
        }
    }

    return FALSE;
}

static BOOL d2d_geometry_get_first_bezier_segment_idx(struct d2d_geometry *geometry, struct d2d_segment_idx *idx)
{
    memset(idx, 0, sizeof(*idx));

    return d2d_geometry_get_bezier_segment_idx(geometry, idx, FALSE);
}

static BOOL d2d_geometry_get_next_bezier_segment_idx(struct d2d_geometry *geometry, struct d2d_segment_idx *idx)
{
    return d2d_geometry_get_bezier_segment_idx(geometry, idx, TRUE);
}

static BOOL d2d_geometry_check_bezier_overlap(struct d2d_geometry *geometry,
        const struct d2d_segment_idx *idx_p, const struct d2d_segment_idx *idx_q)
{
    const D2D1_POINT_2F *a[3], *b[3], *p[2], *q;
    const struct d2d_figure *figure;
    D2D1_POINT_2F v_q[3], v_p, v_qp;
    unsigned int i, j, score;
    float det, t;

    figure = &geometry->u.path.figures[idx_p->figure_idx];
    a[0] = &figure->vertices[idx_p->vertex_idx];
    a[1] = &figure->bezier_controls[idx_p->control_idx];
    a[2] = &figure->vertices[idx_p->vertex_idx + 1];

    figure = &geometry->u.path.figures[idx_q->figure_idx];
    b[0] = &figure->vertices[idx_q->vertex_idx];
    b[1] = &figure->bezier_controls[idx_q->control_idx];
    b[2] = &figure->vertices[idx_q->vertex_idx + 1];

    if (d2d_point_ccw(a[0], a[1], a[2]) == 0.0f || d2d_point_ccw(b[0], b[1], b[2]) == 0.0f)
        return FALSE;

    d2d_point_subtract(&v_q[0], b[1], b[0]);
    d2d_point_subtract(&v_q[1], b[2], b[0]);
    d2d_point_subtract(&v_q[2], b[1], b[2]);

    /* Check for intersections between the edges. Strictly speaking we'd only
     * need to check 8 of the 9 possible intersections, since if there's any
     * intersection there has to be a second intersection as well. */
    for (i = 0; i < 3; ++i)
    {
        d2d_point_subtract(&v_p, a[(i & 1) + 1], a[i & 2]);
        for (j = 0; j < 3; ++j)
        {
            det = v_p.x * v_q[j].y - v_p.y * v_q[j].x;
            if (det == 0.0f)
                continue;

            d2d_point_subtract(&v_qp, a[i & 2], b[j & 2]);
            t = (v_q[j].x * v_qp.y - v_q[j].y * v_qp.x) / det;
            if (t <= 0.0f || t >= 1.0f)
                continue;

            t = (v_p.x * v_qp.y - v_p.y * v_qp.x) / det;
            if (t <= 0.0f || t >= 1.0f)
                continue;

            return TRUE;
        }
    }

    /* Check if one triangle is contained within the other. */
    for (j = 0, score = 0, q = a[1], p[0] = b[2]; j < 3; ++j)
    {
        p[1] = b[j];
        d2d_point_subtract(&v_p, p[1], p[0]);
        d2d_point_subtract(&v_qp, q, p[0]);

        if ((q->y < p[0]->y) != (q->y < p[1]->y) && v_qp.x < v_p.x * (v_qp.y / v_p.y))
            ++score;

        p[0] = p[1];
    }

    if (score & 1)
        return TRUE;

    for (j = 0, score = 0, q = b[1], p[0] = a[2]; j < 3; ++j)
    {
        p[1] = a[j];
        d2d_point_subtract(&v_p, p[1], p[0]);
        d2d_point_subtract(&v_qp, q, p[0]);

        if ((q->y < p[0]->y) != (q->y < p[1]->y) && v_qp.x < v_p.x * (v_qp.y / v_p.y))
            ++score;

        p[0] = p[1];
    }

    return score & 1;
}

static float d2d_geometry_bezier_ccw(struct d2d_geometry *geometry, const struct d2d_segment_idx *idx)
{
    const struct d2d_figure *figure = &geometry->u.path.figures[idx->figure_idx];
    size_t next = idx->vertex_idx + 1;

    return d2d_point_ccw(&figure->vertices[idx->vertex_idx],
            &figure->bezier_controls[idx->control_idx], &figure->vertices[next]);
}

static BOOL d2d_geometry_split_bezier(struct d2d_geometry *geometry, const struct d2d_segment_idx *idx)
{
    const D2D1_POINT_2F *p[3];
    struct d2d_figure *figure;
    D2D1_POINT_2F q[3];
    size_t next;

    figure = &geometry->u.path.figures[idx->figure_idx];
    p[0] = &figure->vertices[idx->vertex_idx];
    p[1] = &figure->bezier_controls[idx->control_idx];
    next = idx->vertex_idx + 1;
    p[2] = &figure->vertices[next];

    d2d_point_lerp(&q[0], p[0], p[1], 0.5f);
    d2d_point_lerp(&q[1], p[1], p[2], 0.5f);
    d2d_point_lerp(&q[2], &q[0], &q[1], 0.5f);

    figure->bezier_controls[idx->control_idx] = q[0];
    if (!(d2d_figure_insert_bezier_controls(figure, idx->control_idx + 1, 1, &q[1])))
        return FALSE;
    if (!(d2d_figure_insert_vertex(figure, idx->vertex_idx + 1, q[2])))
        return FALSE;
    figure->vertex_types[idx->vertex_idx + 1] = D2D_VERTEX_TYPE_SPLIT_BEZIER;

    return TRUE;
}

static HRESULT d2d_geometry_resolve_beziers(struct d2d_geometry *geometry)
{
    struct d2d_segment_idx idx_p, idx_q;
    struct d2d_curve_vertex *b;
    const D2D1_POINT_2F *p[3];
    struct d2d_figure *figure;
    size_t bezier_idx, i;

    if (!d2d_geometry_get_first_bezier_segment_idx(geometry, &idx_p))
        return S_OK;

    /* Split overlapping bezier control triangles. */
    while (d2d_geometry_get_next_bezier_segment_idx(geometry, &idx_p))
    {
        d2d_geometry_get_first_bezier_segment_idx(geometry, &idx_q);
        while (idx_q.figure_idx < idx_p.figure_idx || idx_q.vertex_idx < idx_p.vertex_idx)
        {
            while (d2d_geometry_check_bezier_overlap(geometry, &idx_p, &idx_q))
            {
                if (fabsf(d2d_geometry_bezier_ccw(geometry, &idx_q)) > fabsf(d2d_geometry_bezier_ccw(geometry, &idx_p)))
                {
                    if (!d2d_geometry_split_bezier(geometry, &idx_q))
                        return E_OUTOFMEMORY;
                    if (idx_p.figure_idx == idx_q.figure_idx)
                    {
                        ++idx_p.vertex_idx;
                        ++idx_p.control_idx;
                    }
                }
                else
                {
                    if (!d2d_geometry_split_bezier(geometry, &idx_p))
                        return E_OUTOFMEMORY;
                }
            }
            d2d_geometry_get_next_bezier_segment_idx(geometry, &idx_q);
        }
    }

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        if (geometry->u.path.figures[i].flags & D2D_FIGURE_FLAG_HOLLOW)
            continue;
        geometry->fill.bezier_vertex_count += 3 * geometry->u.path.figures[i].bezier_control_count;
    }

    if (!(geometry->fill.bezier_vertices = calloc(geometry->fill.bezier_vertex_count,
            sizeof(*geometry->fill.bezier_vertices))))
    {
        ERR("Failed to allocate bezier vertices array.\n");
        geometry->fill.bezier_vertex_count = 0;
        return E_OUTOFMEMORY;
    }

    bezier_idx = 0;
    d2d_geometry_get_first_bezier_segment_idx(geometry, &idx_p);
    for (;;)
    {
        float sign = -1.0f;

        figure = &geometry->u.path.figures[idx_p.figure_idx];
        p[0] = &figure->vertices[idx_p.vertex_idx];
        p[1] = &figure->bezier_controls[idx_p.control_idx];

        i = idx_p.vertex_idx + 1;
        if (d2d_path_geometry_point_inside(geometry, p[1], FALSE))
        {
            sign = 1.0f;
            d2d_figure_insert_vertex(figure, i, *p[1]);
            /* Inserting a vertex potentially invalidates p[0]. */
            p[0] = &figure->vertices[idx_p.vertex_idx];
            ++i;
        }

        if (i == figure->vertex_count)
            i = 0;
        p[2] = &figure->vertices[i];

        b = &geometry->fill.bezier_vertices[bezier_idx * 3];
        d2d_curve_vertex_set(&b[0], p[0], 0.0f, 0.0f, sign);
        d2d_curve_vertex_set(&b[1], p[1], 0.5f, 0.0f, sign);
        d2d_curve_vertex_set(&b[2], p[2], 1.0f, 1.0f, sign);

        if (!d2d_geometry_get_next_bezier_segment_idx(geometry, &idx_p))
            break;
        ++bezier_idx;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_sink_Close(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    HRESULT hr = E_FAIL;

    TRACE("iface %p.\n", iface);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_OPEN)
    {
        if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return D2DERR_WRONG_STATE;
    }
    geometry->u.path.state = D2D_GEOMETRY_STATE_CLOSED;

    if (!d2d_geometry_intersect_self(geometry))
        goto done;
    if (FAILED(hr = d2d_geometry_resolve_beziers(geometry)))
        goto done;
    if (FAILED(hr = d2d_path_geometry_triangulate(geometry)))
        goto done;

done:
    if (FAILED(hr))
    {
        free(geometry->fill.bezier_vertices);
        geometry->fill.bezier_vertices = NULL;
        geometry->fill.bezier_vertex_count = 0;
        d2d_path_geometry_free_figures(geometry);
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
    }
    return hr;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddLine(ID2D1GeometrySink *iface, D2D1_POINT_2F point)
{
    TRACE("iface %p, point %s.\n", iface, debug_d2d_point_2f(&point));

    d2d_geometry_sink_AddLines(iface, &point, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddBezier(ID2D1GeometrySink *iface, const D2D1_BEZIER_SEGMENT *bezier)
{
    TRACE("iface %p, bezier %p.\n", iface, bezier);

    d2d_geometry_sink_AddBeziers(iface, bezier, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddQuadraticBezier(ID2D1GeometrySink *iface,
        const D2D1_QUADRATIC_BEZIER_SEGMENT *bezier)
{
    TRACE("iface %p, bezier %p.\n", iface, bezier);

    ID2D1GeometrySink_AddQuadraticBeziers(iface, bezier, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddQuadraticBeziers(ID2D1GeometrySink *iface,
        const D2D1_QUADRATIC_BEZIER_SEGMENT *beziers, UINT32 bezier_count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    struct d2d_figure *figure = &geometry->u.path.figures[geometry->u.path.figure_count - 1];
    unsigned int i;

    TRACE("iface %p, beziers %p, bezier_count %u.\n", iface, beziers, bezier_count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    for (i = 0; i < bezier_count; ++i)
    {
        D2D1_RECT_F bezier_bounds;
        D2D1_POINT_2F p[2];

        /* Construct a cubic curve. */
        d2d_point_lerp(&p[0], &figure->vertices[figure->vertex_count - 1], &beziers[i].point1, 2.0f / 3.0f);
        d2d_point_lerp(&p[1], &beziers[i].point2, &beziers[i].point1, 2.0f / 3.0f);
        if (!d2d_figure_add_original_bezier_controls(figure, 2, p))
        {
            ERR("Failed to add cubic Bézier controls.\n");
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
            return;
        }

        d2d_rect_get_bezier_bounds(&bezier_bounds, &figure->vertices[figure->vertex_count - 1],
                &beziers[i].point1, &beziers[i].point2);

        figure->vertex_types[figure->vertex_count - 1] = D2D_VERTEX_TYPE_BEZIER;
        if (!d2d_figure_add_bezier_controls(figure, 1, &beziers[i].point1))
        {
            ERR("Failed to add bezier.\n");
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
            return;
        }

        if (!d2d_figure_add_vertex(figure, beziers[i].point2))
        {
            ERR("Failed to add bezier vertex.\n");
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
            return;
        }

        d2d_rect_union(&figure->bounds, &bezier_bounds);
    }

    geometry->u.path.segment_count += bezier_count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddArc(ID2D1GeometrySink *iface, const D2D1_ARC_SEGMENT *arc)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, arc %p stub!\n", iface, arc);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    if (!d2d_figure_add_vertex(&geometry->u.path.figures[geometry->u.path.figure_count - 1], arc->point))
    {
        ERR("Failed to add vertex.\n");
        return;
    }

    ++geometry->u.path.segment_count;
}

static const struct ID2D1GeometrySinkVtbl d2d_geometry_sink_vtbl =
{
    d2d_geometry_sink_QueryInterface,
    d2d_geometry_sink_AddRef,
    d2d_geometry_sink_Release,
    d2d_geometry_sink_SetFillMode,
    d2d_geometry_sink_SetSegmentFlags,
    d2d_geometry_sink_BeginFigure,
    d2d_geometry_sink_AddLines,
    d2d_geometry_sink_AddBeziers,
    d2d_geometry_sink_EndFigure,
    d2d_geometry_sink_Close,
    d2d_geometry_sink_AddLine,
    d2d_geometry_sink_AddBezier,
    d2d_geometry_sink_AddQuadraticBezier,
    d2d_geometry_sink_AddQuadraticBeziers,
    d2d_geometry_sink_AddArc,
};

static inline struct d2d_geometry *impl_from_ID2D1PathGeometry1(ID2D1PathGeometry1 *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_QueryInterface(ID2D1PathGeometry1 *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1PathGeometry1)
            || IsEqualGUID(iid, &IID_ID2D1PathGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1PathGeometry1_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_path_geometry_AddRef(ID2D1PathGeometry1 *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_path_geometry_Release(ID2D1PathGeometry1 *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_path_geometry_free_figures(geometry);
        d2d_geometry_cleanup(geometry);
        free(geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_path_geometry_GetFactory(ID2D1PathGeometry1 *iface, ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetBounds(ID2D1PathGeometry1 *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);
    size_t i;

    TRACE("iface %p, transform %p, bounds %p.\n", iface, transform, bounds);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
        return D2DERR_WRONG_STATE;

    bounds->left = FLT_MAX;
    bounds->top = FLT_MAX;
    bounds->right = -FLT_MAX;
    bounds->bottom = -FLT_MAX;

    if (!transform)
    {
        if (geometry->u.path.bounds.left > geometry->u.path.bounds.right
                && !isinf(geometry->u.path.bounds.left))
        {
            for (i = 0; i < geometry->u.path.figure_count; ++i)
            {
                if (geometry->u.path.figures[i].flags & D2D_FIGURE_FLAG_HOLLOW)
                    continue;
                d2d_rect_union(&geometry->u.path.bounds, &geometry->u.path.figures[i].bounds);
            }
            if (geometry->u.path.bounds.left > geometry->u.path.bounds.right)
            {
                geometry->u.path.bounds.left = INFINITY;
                geometry->u.path.bounds.right = FLT_MAX;
                geometry->u.path.bounds.top = INFINITY;
                geometry->u.path.bounds.bottom = FLT_MAX;
            }
        }

        *bounds = geometry->u.path.bounds;
        return S_OK;
    }

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        const struct d2d_figure *figure = &geometry->u.path.figures[i];
        enum d2d_vertex_type type = D2D_VERTEX_TYPE_NONE;
        D2D1_RECT_F bezier_bounds;
        D2D1_POINT_2F p, p1, p2;
        size_t j, bezier_idx;

        if (figure->flags & D2D_FIGURE_FLAG_HOLLOW)
            continue;

        for (j = 0; j < figure->vertex_count; ++j)
        {
            if (figure->vertex_types[j] == D2D_VERTEX_TYPE_NONE)
                continue;

            p = figure->vertices[j];
            type = figure->vertex_types[j];
            d2d_point_transform(&p, transform, p.x, p.y);
            d2d_rect_expand(bounds, &p);
            break;
        }

        for (bezier_idx = 0, ++j; j < figure->vertex_count; ++j)
        {
            enum d2d_vertex_type next_type;

            if ((next_type = figure->vertex_types[j]) == D2D_VERTEX_TYPE_NONE
                    || d2d_vertex_type_is_split_bezier(next_type))
                continue;

            switch (type)
            {
                case D2D_VERTEX_TYPE_LINE:
                    p = figure->vertices[j];
                    d2d_point_transform(&p, transform, p.x, p.y);
                    d2d_rect_expand(bounds, &p);
                    break;

                case D2D_VERTEX_TYPE_BEZIER:
                    /* FIXME: This attempts to approximate a cubic Bézier with
                     * a quadratic one. */
                    p1 = figure->original_bezier_controls[bezier_idx++];
                    d2d_point_transform(&p1, transform, p1.x, p1.y);
                    p2 = figure->original_bezier_controls[bezier_idx++];
                    d2d_point_transform(&p2, transform, p2.x, p2.y);
                    p1.x = (p1.x + p2.x) * 0.75f;
                    p1.y = (p1.y + p2.y) * 0.75f;
                    p2 = figure->vertices[j];
                    d2d_point_transform(&p2, transform, p2.x, p2.y);
                    p1.x -= (p.x + p2.x) * 0.25f;
                    p1.y -= (p.y + p2.y) * 0.25f;

                    d2d_rect_get_bezier_bounds(&bezier_bounds, &p, &p1, &p2);
                    d2d_rect_union(bounds, &bezier_bounds);
                    p = p2;
                    break;

                default:
                    FIXME("Unhandled vertex type %#x.\n", type);
                    p = figure->vertices[j];
                    d2d_point_transform(&p, transform, p.x, p.y);
                    d2d_rect_expand(bounds, &p);
                    break;
            }

            type = next_type;
        }
    }

    if (bounds->left > bounds->right)
    {
        bounds->left = INFINITY;
        bounds->right = FLT_MAX;
        bounds->top = INFINITY;
        bounds->bottom = FLT_MAX;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetWidenedBounds(ID2D1PathGeometry1 *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_StrokeContainsPoint(ID2D1PathGeometry1 *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);
    enum d2d_vertex_type type = D2D_VERTEX_TYPE_NONE;
    unsigned int i, j, bezier_idx;
    D2D1_BEZIER_SEGMENT b;
    D2D1_POINT_2F p, p1;

    TRACE("iface %p, point %s, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, contains %p.\n",
            iface, debug_d2d_point_2f(&point), stroke_width, stroke_style, transform, tolerance, contains);

    if (stroke_style)
        FIXME("Ignoring stroke style %p.\n", stroke_style);

    if (!transform)
        transform = &identity;

    if (tolerance <= 0.0f)
        tolerance = D2D1_DEFAULT_FLATTENING_TOLERANCE;

    *contains = FALSE;
    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        const struct d2d_figure *figure = &geometry->u.path.figures[i];

        for (j = 0; j < figure->vertex_count; ++j)
        {
            if (figure->vertex_types[j] == D2D_VERTEX_TYPE_NONE)
                continue;

            p = figure->vertices[j];
            type = figure->vertex_types[j];
            break;
        }

        for (bezier_idx = 0, ++j; j < figure->vertex_count; ++j)
        {
            enum d2d_vertex_type next_type;

            if ((next_type = figure->vertex_types[j]) == D2D_VERTEX_TYPE_NONE
                    || d2d_vertex_type_is_split_bezier(next_type))
                continue;

            switch (type)
            {
                case D2D_VERTEX_TYPE_LINE:
                    p1 = figure->vertices[j];
                    *contains = d2d_point_on_line_segment(&point, &p, &p1, transform, stroke_width * 0.5f, tolerance);
                    p = p1;
                    break;

                case D2D_VERTEX_TYPE_BEZIER:
                    b.point1 = figure->original_bezier_controls[bezier_idx++];
                    b.point2 = figure->original_bezier_controls[bezier_idx++];
                    b.point3 = figure->vertices[j];
                    *contains = d2d_point_on_bezier_segment(&point, &p, &b, transform, stroke_width, tolerance);
                    p = b.point3;
                    break;

                default:
                    FIXME("Unhandled vertex type %#x.\n", type);
                    p = figure->vertices[j];
                    break;
            }
            if (*contains)
                return S_OK;
            type = next_type;
        }

        if (type == D2D_VERTEX_TYPE_LINE)
        {
            p1 = figure->vertices[0];
            if (figure->flags & D2D_FIGURE_FLAG_CLOSED)
                *contains = d2d_point_on_line_segment(&point, &p, &p1, transform, stroke_width * 0.5f, tolerance);
        }

        if (*contains)
            return S_OK;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_FillContainsPoint(ID2D1PathGeometry1 *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);
    D2D1_MATRIX_3X2_F g_i;

    TRACE("iface %p, point %s, transform %p, tolerance %.8e, contains %p.\n",
            iface, debug_d2d_point_2f(&point), transform, tolerance, contains);

    if (transform)
    {
        if (!d2d_matrix_invert(&g_i, transform))
            return D2DERR_UNSUPPORTED_OPERATION;
        d2d_point_transform(&point, &g_i, point.x, point.y);
    }

    *contains = !!d2d_path_geometry_point_inside(geometry, &point, FALSE);

    TRACE("-> %#x.\n", *contains);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_CompareWithGeometry(ID2D1PathGeometry1 *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static void d2d_geometry_flatten_cubic(ID2D1SimplifiedGeometrySink *sink, const D2D1_POINT_2F *p0,
        const D2D1_BEZIER_SEGMENT *b, float tolerance)
{
    D2D1_BEZIER_SEGMENT b0, b1;
    D2D1_POINT_2F q;
    float d;

    /* It's certainly possible to calculate the maximum deviation of the
     * approximation from the curve, but it's a little involved. Instead, note
     * that if the control points were evenly spaced and collinear, p1 would
     * be exactly between p0 and p2, and p2 would be exactly between p1 and
     * p3. The deviation is a decent enough approximation, and much easier to
     * calculate.
     *
     * p1' = (p0 + p2) / 2
     * p2' = (p1 + p3) / 2
     *   d = ‖p1 - p1'‖₁ + ‖p2 - p2'‖₁ */
    d2d_point_lerp(&q, p0, &b->point2, 0.5f);
    d2d_point_subtract(&q, &b->point1, &q);
    d = fabsf(q.x) + fabsf(q.y);
    d2d_point_lerp(&q, &b->point1, &b->point3, 0.5f);
    d2d_point_subtract(&q, &b->point2, &q);
    d += fabsf(q.x) + fabsf(q.y);
    if (d < tolerance)
    {
        ID2D1SimplifiedGeometrySink_AddLines(sink, &b->point3, 1);
        return;
    }

    d2d_point_lerp(&q, &b->point1, &b->point2, 0.5f);

    b1.point3 = b->point3;
    d2d_point_lerp(&b1.point2, &b1.point3, &b->point2, 0.5f);
    d2d_point_lerp(&b1.point1, &b1.point2, &q, 0.5f);

    d2d_point_lerp(&b0.point1, p0, &b->point1, 0.5f);
    d2d_point_lerp(&b0.point2, &b0.point1, &q, 0.5f);
    d2d_point_lerp(&b0.point3, &b0.point2, &b1.point1, 0.5f);

    d2d_geometry_flatten_cubic(sink, p0, &b0, tolerance);
    ID2D1SimplifiedGeometrySink_SetSegmentFlags(sink, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN);
    d2d_geometry_flatten_cubic(sink, &b0.point3, &b1, tolerance);
    ID2D1SimplifiedGeometrySink_SetSegmentFlags(sink, D2D1_PATH_SEGMENT_NONE);
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Simplify(ID2D1PathGeometry1 *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);
    enum d2d_vertex_type type = D2D_VERTEX_TYPE_NONE;
    unsigned int i, j, bezier_idx;
    D2D1_FIGURE_BEGIN begin;
    D2D1_BEZIER_SEGMENT b;
    D2D1_FIGURE_END end;
    D2D1_POINT_2F p;

    TRACE("iface %p, option %#x, transform %p, tolerance %.8e, sink %p.\n",
            iface, option, transform, tolerance, sink);

    ID2D1SimplifiedGeometrySink_SetFillMode(sink, geometry->u.path.fill_mode);
    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        const struct d2d_figure *figure = &geometry->u.path.figures[i];

        for (j = 0; j < figure->vertex_count; ++j)
        {
            if (figure->vertex_types[j] == D2D_VERTEX_TYPE_NONE)
                continue;

            p = figure->vertices[j];
            if (transform)
                d2d_point_transform(&p, transform, p.x, p.y);
            begin = figure->flags & D2D_FIGURE_FLAG_HOLLOW ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
            ID2D1SimplifiedGeometrySink_BeginFigure(sink, p, begin);
            type = figure->vertex_types[j];
            break;
        }

        for (bezier_idx = 0, ++j; j < figure->vertex_count; ++j)
        {
            enum d2d_vertex_type next_type;

            if ((next_type = figure->vertex_types[j]) == D2D_VERTEX_TYPE_NONE
                    || d2d_vertex_type_is_split_bezier(next_type))
                continue;

            switch (type)
            {
                case D2D_VERTEX_TYPE_LINE:
                    p = figure->vertices[j];
                    if (transform)
                        d2d_point_transform(&p, transform, p.x, p.y);
                    ID2D1SimplifiedGeometrySink_AddLines(sink, &p, 1);
                    break;

                case D2D_VERTEX_TYPE_BEZIER:
                    b.point1 = figure->original_bezier_controls[bezier_idx++];
                    b.point2 = figure->original_bezier_controls[bezier_idx++];
                    b.point3 = figure->vertices[j];
                    if (transform)
                    {
                        d2d_point_transform(&b.point1, transform, b.point1.x, b.point1.y);
                        d2d_point_transform(&b.point2, transform, b.point2.x, b.point2.y);
                        d2d_point_transform(&b.point3, transform, b.point3.x, b.point3.y);
                    }

                    if (option == D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES)
                        d2d_geometry_flatten_cubic(sink, &p, &b, tolerance);
                    else
                        ID2D1SimplifiedGeometrySink_AddBeziers(sink, &b, 1);
                    p = b.point3;
                    break;

                default:
                    FIXME("Unhandled vertex type %#x.\n", type);
                    p = figure->vertices[j];
                    if (transform)
                        d2d_point_transform(&p, transform, p.x, p.y);
                    ID2D1SimplifiedGeometrySink_AddLines(sink, &p, 1);
                    break;
            }

            type = next_type;
        }

        end = figure->flags & D2D_FIGURE_FLAG_CLOSED ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN;
        ID2D1SimplifiedGeometrySink_EndFigure(sink, end);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Tessellate(ID2D1PathGeometry1 *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_CombineWithGeometry(ID2D1PathGeometry1 *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Outline(ID2D1PathGeometry1 *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputeArea(ID2D1PathGeometry1 *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputeLength(ID2D1PathGeometry1 *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputePointAtLength(ID2D1PathGeometry1 *iface, float length,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point, D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Widen(ID2D1PathGeometry1 *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Open(ID2D1PathGeometry1 *iface, ID2D1GeometrySink **sink)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);

    TRACE("iface %p, sink %p.\n", iface, sink);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_INITIAL)
        return D2DERR_WRONG_STATE;

    *sink = &geometry->u.path.ID2D1GeometrySink_iface;
    ID2D1GeometrySink_AddRef(*sink);

    geometry->u.path.state = D2D_GEOMETRY_STATE_OPEN;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Stream(ID2D1PathGeometry1 *iface, ID2D1GeometrySink *sink)
{
    FIXME("iface %p, sink %p stub!\n", iface, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetSegmentCount(ID2D1PathGeometry1 *iface, UINT32 *count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
        return D2DERR_WRONG_STATE;

    *count = geometry->u.path.segment_count;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetFigureCount(ID2D1PathGeometry1 *iface, UINT32 *count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry1(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
        return D2DERR_WRONG_STATE;

    *count = geometry->u.path.figure_count;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry1_ComputePointAndSegmentAtLength(ID2D1PathGeometry1 *iface,
        float length, UINT32 start_segment, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        D2D1_POINT_DESCRIPTION *point_desc)
{
    FIXME("iface %p, length %.8e, start_segment %u, transform %p, tolerance %.8e, point_desc %p.\n",
            iface, length, start_segment, transform, tolerance, point_desc);

    return E_NOTIMPL;
}

static const struct ID2D1PathGeometry1Vtbl d2d_path_geometry_vtbl =
{
    d2d_path_geometry_QueryInterface,
    d2d_path_geometry_AddRef,
    d2d_path_geometry_Release,
    d2d_path_geometry_GetFactory,
    d2d_path_geometry_GetBounds,
    d2d_path_geometry_GetWidenedBounds,
    d2d_path_geometry_StrokeContainsPoint,
    d2d_path_geometry_FillContainsPoint,
    d2d_path_geometry_CompareWithGeometry,
    d2d_path_geometry_Simplify,
    d2d_path_geometry_Tessellate,
    d2d_path_geometry_CombineWithGeometry,
    d2d_path_geometry_Outline,
    d2d_path_geometry_ComputeArea,
    d2d_path_geometry_ComputeLength,
    d2d_path_geometry_ComputePointAtLength,
    d2d_path_geometry_Widen,
    d2d_path_geometry_Open,
    d2d_path_geometry_Stream,
    d2d_path_geometry_GetSegmentCount,
    d2d_path_geometry_GetFigureCount,
    d2d_path_geometry1_ComputePointAndSegmentAtLength,
};

void d2d_path_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory)
{
    d2d_geometry_init(geometry, factory, &identity, (ID2D1GeometryVtbl *)&d2d_path_geometry_vtbl);
    geometry->u.path.ID2D1GeometrySink_iface.lpVtbl = &d2d_geometry_sink_vtbl;
    geometry->u.path.bounds.left = FLT_MAX;
    geometry->u.path.bounds.right = -FLT_MAX;
    geometry->u.path.bounds.top = FLT_MAX;
    geometry->u.path.bounds.bottom = -FLT_MAX;
}

static inline struct d2d_geometry *impl_from_ID2D1EllipseGeometry(ID2D1EllipseGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_QueryInterface(ID2D1EllipseGeometry *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1EllipseGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1EllipseGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_ellipse_geometry_AddRef(ID2D1EllipseGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1EllipseGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_ellipse_geometry_Release(ID2D1EllipseGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1EllipseGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_geometry_cleanup(geometry);
        free(geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_ellipse_geometry_GetFactory(ID2D1EllipseGeometry *iface, ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1EllipseGeometry(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_GetBounds(ID2D1EllipseGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, transform %p, bounds %p stub!\n", iface, transform, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_GetWidenedBounds(ID2D1EllipseGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_StrokeContainsPoint(ID2D1EllipseGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    FIXME("iface %p, point %s, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, debug_d2d_point_2f(&point), stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_FillContainsPoint(ID2D1EllipseGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point %s, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, debug_d2d_point_2f(&point), transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_CompareWithGeometry(ID2D1EllipseGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_Simplify(ID2D1EllipseGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_Tessellate(ID2D1EllipseGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_CombineWithGeometry(ID2D1EllipseGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_Outline(ID2D1EllipseGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_ComputeArea(ID2D1EllipseGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_ComputeLength(ID2D1EllipseGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_ComputePointAtLength(ID2D1EllipseGeometry *iface,
        float length, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point,
        D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_ellipse_geometry_Widen(ID2D1EllipseGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_ellipse_geometry_GetEllipse(ID2D1EllipseGeometry *iface, D2D1_ELLIPSE *ellipse)
{
    struct d2d_geometry *geometry = impl_from_ID2D1EllipseGeometry(iface);

    TRACE("iface %p, ellipse %p.\n", iface, ellipse);

    *ellipse = geometry->u.ellipse.ellipse;
}

static const struct ID2D1EllipseGeometryVtbl d2d_ellipse_geometry_vtbl =
{
    d2d_ellipse_geometry_QueryInterface,
    d2d_ellipse_geometry_AddRef,
    d2d_ellipse_geometry_Release,
    d2d_ellipse_geometry_GetFactory,
    d2d_ellipse_geometry_GetBounds,
    d2d_ellipse_geometry_GetWidenedBounds,
    d2d_ellipse_geometry_StrokeContainsPoint,
    d2d_ellipse_geometry_FillContainsPoint,
    d2d_ellipse_geometry_CompareWithGeometry,
    d2d_ellipse_geometry_Simplify,
    d2d_ellipse_geometry_Tessellate,
    d2d_ellipse_geometry_CombineWithGeometry,
    d2d_ellipse_geometry_Outline,
    d2d_ellipse_geometry_ComputeArea,
    d2d_ellipse_geometry_ComputeLength,
    d2d_ellipse_geometry_ComputePointAtLength,
    d2d_ellipse_geometry_Widen,
    d2d_ellipse_geometry_GetEllipse,
};

HRESULT d2d_ellipse_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory, const D2D1_ELLIPSE *ellipse)
{
    D2D1_POINT_2F *v, v1, v2, v3, v4;
    struct d2d_face *f;
    float l, r, t, b;

    d2d_geometry_init(geometry, factory, &identity, (ID2D1GeometryVtbl *)&d2d_ellipse_geometry_vtbl);
    geometry->u.ellipse.ellipse = *ellipse;

    if (!(geometry->fill.vertices = malloc(4 * sizeof(*geometry->fill.vertices))))
        goto fail;
    if (!d2d_array_reserve((void **)&geometry->fill.faces,
            &geometry->fill.faces_size, 2, sizeof(*geometry->fill.faces)))
        goto fail;

    l = ellipse->point.x - ellipse->radiusX;
    r = ellipse->point.x + ellipse->radiusX;
    t = ellipse->point.y - ellipse->radiusY;
    b = ellipse->point.y + ellipse->radiusY;

    d2d_point_set(&v1, r, t);
    d2d_point_set(&v2, r, b);
    d2d_point_set(&v3, l, b);
    d2d_point_set(&v4, l, t);

    v = geometry->fill.vertices;
    d2d_point_set(&v[0], ellipse->point.x, t);
    d2d_point_set(&v[1], r, ellipse->point.y);
    d2d_point_set(&v[2], ellipse->point.x, b);
    d2d_point_set(&v[3], l, ellipse->point.y);
    geometry->fill.vertex_count = 4;

    f = geometry->fill.faces;
    d2d_face_set(&f[0], 0, 3, 2);
    d2d_face_set(&f[1], 0, 2, 1);
    geometry->fill.face_count = 2;

    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[0], &v1, &v[1]))
        goto fail;
    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[1], &v2, &v[2]))
        goto fail;
    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[2], &v3, &v[3]))
        goto fail;
    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[3], &v4, &v[0]))
        goto fail;

    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[0], &v1, &v[1]))
        goto fail;
    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[1], &v2, &v[2]))
        goto fail;
    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[2], &v3, &v[3]))
        goto fail;
    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[3], &v4, &v[0]))
        goto fail;

    return S_OK;

fail:
    d2d_geometry_cleanup(geometry);
    return E_OUTOFMEMORY;
}

static inline struct d2d_geometry *impl_from_ID2D1RectangleGeometry(ID2D1RectangleGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_QueryInterface(ID2D1RectangleGeometry *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1RectangleGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1RectangleGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_rectangle_geometry_AddRef(ID2D1RectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_rectangle_geometry_Release(ID2D1RectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_geometry_cleanup(geometry);
        free(geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_rectangle_geometry_GetFactory(ID2D1RectangleGeometry *iface, ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_GetBounds(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    D2D1_RECT_F *rect;
    D2D1_POINT_2F p;

    TRACE("iface %p, transform %p, bounds %p.\n", iface, transform, bounds);

    rect = &geometry->u.rectangle.rect;
    if (!transform)
    {
        *bounds = *rect;
        return S_OK;
    }

    bounds->left = FLT_MAX;
    bounds->top = FLT_MAX;
    bounds->right = -FLT_MAX;
    bounds->bottom = -FLT_MAX;

    d2d_point_transform(&p, transform, rect->left, rect->top);
    d2d_rect_expand(bounds, &p);
    d2d_point_transform(&p, transform, rect->left, rect->bottom);
    d2d_rect_expand(bounds, &p);
    d2d_point_transform(&p, transform, rect->right, rect->bottom);
    d2d_rect_expand(bounds, &p);
    d2d_point_transform(&p, transform, rect->right, rect->top);
    d2d_rect_expand(bounds, &p);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_GetWidenedBounds(ID2D1RectangleGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_StrokeContainsPoint(ID2D1RectangleGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    const struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    const D2D1_RECT_F *rect = &geometry->u.rectangle.rect;
    unsigned int i;
    struct
    {
        D2D1_POINT_2F s, e;
    }
    segments[4];

    TRACE("iface %p, point %s, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, contains %p.\n",
            iface, debug_d2d_point_2f(&point), stroke_width, stroke_style, transform, tolerance, contains);

    if (stroke_style)
        FIXME("Ignoring stroke style %p.\n", stroke_style);

    tolerance = fabsf(tolerance);

    if (!transform)
    {
        D2D1_POINT_2F d, s;

        s.x = rect->right - rect->left;
        s.y = rect->bottom - rect->top;
        d.x = fabsf((rect->right + rect->left) * 0.5f - point.x);
        d.y = fabsf((rect->bottom + rect->top) * 0.5f - point.y);

        /* Inside test. */
        if (d.x <= (s.x - stroke_width) * 0.5f - tolerance && d.y <= (s.y - stroke_width) * 0.5f - tolerance)
        {
            *contains = FALSE;
            return S_OK;
        }

        if (tolerance == 0.0f)
        {
            *contains = d.x < (s.x + stroke_width) * 0.5f && d.y < (s.y + stroke_width) * 0.5f;
        }
        else
        {
            d.x = max(d.x - (s.x + stroke_width) * 0.5f, 0.0f);
            d.y = max(d.y - (s.y + stroke_width) * 0.5f, 0.0f);

            *contains = d2d_point_dot(&d, &d) < tolerance * tolerance;
        }

        return S_OK;
    }

    stroke_width *= 0.5f;

    d2d_point_set(&segments[0].s, rect->left - stroke_width, rect->bottom);
    d2d_point_set(&segments[0].e, rect->right + stroke_width, rect->bottom);
    d2d_point_set(&segments[1].s, rect->right, rect->bottom + stroke_width);
    d2d_point_set(&segments[1].e, rect->right, rect->top - stroke_width);
    d2d_point_set(&segments[2].s, rect->right + stroke_width, rect->top);
    d2d_point_set(&segments[2].e, rect->left - stroke_width, rect->top);
    d2d_point_set(&segments[3].s, rect->left, rect->top - stroke_width);
    d2d_point_set(&segments[3].e, rect->left, rect->bottom + stroke_width);

    *contains = FALSE;
    for (i = 0; i < ARRAY_SIZE(segments); ++i)
    {
        if (d2d_point_on_line_segment(&point, &segments[i].s, &segments[i].e, transform, stroke_width, tolerance))
        {
            *contains = TRUE;
            break;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_FillContainsPoint(ID2D1RectangleGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    D2D1_RECT_F *rect = &geometry->u.rectangle.rect;
    float dx, dy;

    TRACE("iface %p, point %s, transform %p, tolerance %.8e, contains %p.\n",
            iface, debug_d2d_point_2f(&point), transform, tolerance, contains);

    if (transform)
    {
        D2D1_MATRIX_3X2_F g_i;

        if (!d2d_matrix_invert(&g_i, transform))
            return D2DERR_UNSUPPORTED_OPERATION;
        d2d_point_transform(&point, &g_i, point.x, point.y);
    }

    if (tolerance == 0.0f)
        tolerance = D2D1_DEFAULT_FLATTENING_TOLERANCE;

    dx = max(fabsf((rect->right  + rect->left) / 2.0f - point.x) - (rect->right  - rect->left) / 2.0f, 0.0f);
    dy = max(fabsf((rect->bottom + rect->top)  / 2.0f - point.y) - (rect->bottom - rect->top)  / 2.0f, 0.0f);

    *contains = tolerance * tolerance > (dx * dx + dy * dy);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_CompareWithGeometry(ID2D1RectangleGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Simplify(ID2D1RectangleGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    D2D1_RECT_F *rect = &geometry->u.rectangle.rect;
    D2D1_POINT_2F p[4];
    unsigned int i;

    TRACE("iface %p, option %#x, transform %p, tolerance %.8e, sink %p.\n",
            iface, option, transform, tolerance, sink);

    d2d_point_set(&p[0], rect->left, rect->top);
    d2d_point_set(&p[1], rect->right, rect->top);
    d2d_point_set(&p[2], rect->right, rect->bottom);
    d2d_point_set(&p[3], rect->left, rect->bottom);

    if (transform)
    {
        for (i = 0; i < ARRAY_SIZE(p); ++i)
        {
            d2d_point_transform(&p[i], transform, p[i].x, p[i].y);
        }
    }

    ID2D1SimplifiedGeometrySink_SetFillMode(sink, D2D1_FILL_MODE_ALTERNATE);
    ID2D1SimplifiedGeometrySink_BeginFigure(sink, p[0], D2D1_FIGURE_BEGIN_FILLED);
    ID2D1SimplifiedGeometrySink_AddLines(sink, &p[1], 3);
    ID2D1SimplifiedGeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Tessellate(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_CombineWithGeometry(ID2D1RectangleGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Outline(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputeArea(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputeLength(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputePointAtLength(ID2D1RectangleGeometry *iface,
        float length, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point,
        D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Widen(ID2D1RectangleGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_rectangle_geometry_GetRect(ID2D1RectangleGeometry *iface, D2D1_RECT_F *rect)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);

    TRACE("iface %p, rect %p.\n", iface, rect);

    *rect = geometry->u.rectangle.rect;
}

static const struct ID2D1RectangleGeometryVtbl d2d_rectangle_geometry_vtbl =
{
    d2d_rectangle_geometry_QueryInterface,
    d2d_rectangle_geometry_AddRef,
    d2d_rectangle_geometry_Release,
    d2d_rectangle_geometry_GetFactory,
    d2d_rectangle_geometry_GetBounds,
    d2d_rectangle_geometry_GetWidenedBounds,
    d2d_rectangle_geometry_StrokeContainsPoint,
    d2d_rectangle_geometry_FillContainsPoint,
    d2d_rectangle_geometry_CompareWithGeometry,
    d2d_rectangle_geometry_Simplify,
    d2d_rectangle_geometry_Tessellate,
    d2d_rectangle_geometry_CombineWithGeometry,
    d2d_rectangle_geometry_Outline,
    d2d_rectangle_geometry_ComputeArea,
    d2d_rectangle_geometry_ComputeLength,
    d2d_rectangle_geometry_ComputePointAtLength,
    d2d_rectangle_geometry_Widen,
    d2d_rectangle_geometry_GetRect,
};

HRESULT d2d_rectangle_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory, const D2D1_RECT_F *rect)
{
    struct d2d_face *f;
    D2D1_POINT_2F *v;
    float l, r, t, b;

    static const D2D1_POINT_2F prev[] =
    {
        { 1.0f,  0.0f},
        { 0.0f, -1.0f},
        {-1.0f,  0.0f},
        { 0.0f,  1.0f},
    };
    static const D2D1_POINT_2F next[] =
    {
        { 0.0f,  1.0f},
        { 1.0f,  0.0f},
        { 0.0f, -1.0f},
        {-1.0f,  0.0f},
    };

    d2d_geometry_init(geometry, factory, &identity, (ID2D1GeometryVtbl *)&d2d_rectangle_geometry_vtbl);
    geometry->u.rectangle.rect = *rect;

    if (!(geometry->fill.vertices = malloc(4 * sizeof(*geometry->fill.vertices))))
        goto fail;
    if (!d2d_array_reserve((void **)&geometry->fill.faces,
            &geometry->fill.faces_size, 2, sizeof(*geometry->fill.faces)))
        goto fail;

    l = min(rect->left, rect->right);
    r = max(rect->left, rect->right);
    t = min(rect->top, rect->bottom);
    b = max(rect->top, rect->bottom);

    v = geometry->fill.vertices;
    d2d_point_set(&v[0], l, t);
    d2d_point_set(&v[1], l, b);
    d2d_point_set(&v[2], r, b);
    d2d_point_set(&v[3], r, t);
    geometry->fill.vertex_count = 4;

    f = geometry->fill.faces;
    d2d_face_set(&f[0], 1, 2, 0);
    d2d_face_set(&f[1], 0, 2, 3);
    geometry->fill.face_count = 2;

    if (!d2d_geometry_outline_add_line_segment(geometry, &v[0], &v[1]))
        goto fail;
    if (!d2d_geometry_outline_add_line_segment(geometry, &v[1], &v[2]))
        goto fail;
    if (!d2d_geometry_outline_add_line_segment(geometry, &v[2], &v[3]))
        goto fail;
    if (!d2d_geometry_outline_add_line_segment(geometry, &v[3], &v[0]))
        goto fail;

    if (!d2d_geometry_outline_add_join(geometry, &prev[0], &v[0], &next[0]))
        goto fail;
    if (!d2d_geometry_outline_add_join(geometry, &prev[1], &v[1], &next[1]))
        goto fail;
    if (!d2d_geometry_outline_add_join(geometry, &prev[2], &v[2], &next[2]))
        goto fail;
    if (!d2d_geometry_outline_add_join(geometry, &prev[3], &v[3], &next[3]))
        goto fail;

    return S_OK;

fail:
    d2d_geometry_cleanup(geometry);
    return E_OUTOFMEMORY;
}

static inline struct d2d_geometry *impl_from_ID2D1RoundedRectangleGeometry(ID2D1RoundedRectangleGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_QueryInterface(ID2D1RoundedRectangleGeometry *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1RoundedRectangleGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1RoundedRectangleGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_AddRef(ID2D1RoundedRectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RoundedRectangleGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_Release(ID2D1RoundedRectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RoundedRectangleGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_geometry_cleanup(geometry);
        free(geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_GetFactory(ID2D1RoundedRectangleGeometry *iface,
        ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RoundedRectangleGeometry(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_GetBounds(ID2D1RoundedRectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, transform %p, bounds %p stub!\n", iface, transform, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_GetWidenedBounds(ID2D1RoundedRectangleGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_StrokeContainsPoint(
        ID2D1RoundedRectangleGeometry *iface, D2D1_POINT_2F point, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point %s, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, debug_d2d_point_2f(&point), stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_FillContainsPoint(ID2D1RoundedRectangleGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point %s, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, debug_d2d_point_2f(&point), transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_CompareWithGeometry(
        ID2D1RoundedRectangleGeometry *iface, ID2D1Geometry *geometry,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_Simplify(ID2D1RoundedRectangleGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_Tessellate(ID2D1RoundedRectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_CombineWithGeometry(
        ID2D1RoundedRectangleGeometry *iface, ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_Outline(ID2D1RoundedRectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_ComputeArea(ID2D1RoundedRectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_ComputeLength(ID2D1RoundedRectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_ComputePointAtLength(
        ID2D1RoundedRectangleGeometry *iface, float length, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_POINT_2F *point, D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_Widen(ID2D1RoundedRectangleGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_rounded_rectangle_geometry_GetRoundedRect(ID2D1RoundedRectangleGeometry *iface,
        D2D1_ROUNDED_RECT *rounded_rect)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RoundedRectangleGeometry(iface);

    TRACE("iface %p, rounded_rect %p.\n", iface, rounded_rect);

    *rounded_rect = geometry->u.rounded_rectangle.rounded_rect;
}

static const struct ID2D1RoundedRectangleGeometryVtbl d2d_rounded_rectangle_geometry_vtbl =
{
    d2d_rounded_rectangle_geometry_QueryInterface,
    d2d_rounded_rectangle_geometry_AddRef,
    d2d_rounded_rectangle_geometry_Release,
    d2d_rounded_rectangle_geometry_GetFactory,
    d2d_rounded_rectangle_geometry_GetBounds,
    d2d_rounded_rectangle_geometry_GetWidenedBounds,
    d2d_rounded_rectangle_geometry_StrokeContainsPoint,
    d2d_rounded_rectangle_geometry_FillContainsPoint,
    d2d_rounded_rectangle_geometry_CompareWithGeometry,
    d2d_rounded_rectangle_geometry_Simplify,
    d2d_rounded_rectangle_geometry_Tessellate,
    d2d_rounded_rectangle_geometry_CombineWithGeometry,
    d2d_rounded_rectangle_geometry_Outline,
    d2d_rounded_rectangle_geometry_ComputeArea,
    d2d_rounded_rectangle_geometry_ComputeLength,
    d2d_rounded_rectangle_geometry_ComputePointAtLength,
    d2d_rounded_rectangle_geometry_Widen,
    d2d_rounded_rectangle_geometry_GetRoundedRect,
};

HRESULT d2d_rounded_rectangle_geometry_init(struct d2d_geometry *geometry,
        ID2D1Factory *factory, const D2D1_ROUNDED_RECT *rounded_rect)
{
    D2D1_POINT_2F *v, v1, v2, v3, v4;
    struct d2d_face *f;
    float l, r, t, b;
    float rx, ry;

    d2d_geometry_init(geometry, factory, &identity, (ID2D1GeometryVtbl *)&d2d_rounded_rectangle_geometry_vtbl);
    geometry->u.rounded_rectangle.rounded_rect = *rounded_rect;

    if (!(geometry->fill.vertices = malloc(8 * sizeof(*geometry->fill.vertices))))
        goto fail;
    if (!d2d_array_reserve((void **)&geometry->fill.faces,
            &geometry->fill.faces_size, 6, sizeof(*geometry->fill.faces)))
        goto fail;

    l = min(rounded_rect->rect.left, rounded_rect->rect.right);
    r = max(rounded_rect->rect.left, rounded_rect->rect.right);
    t = min(rounded_rect->rect.top, rounded_rect->rect.bottom);
    b = max(rounded_rect->rect.top, rounded_rect->rect.bottom);

    rx = min(rounded_rect->radiusX, 0.5f * (r - l));
    ry = min(rounded_rect->radiusY, 0.5f * (b - t));

    d2d_point_set(&v1, r, t);
    d2d_point_set(&v2, r, b);
    d2d_point_set(&v3, l, b);
    d2d_point_set(&v4, l, t);

    v = geometry->fill.vertices;
    d2d_point_set(&v[0], l + rx, t);
    d2d_point_set(&v[1], r - rx, t);
    d2d_point_set(&v[2], r, t + ry);
    d2d_point_set(&v[3], r, b - ry);
    d2d_point_set(&v[4], r - rx, b);
    d2d_point_set(&v[5], l + rx, b);
    d2d_point_set(&v[6], l, b - ry);
    d2d_point_set(&v[7], l, t + ry);
    geometry->fill.vertex_count = 8;

    f = geometry->fill.faces;
    d2d_face_set(&f[0], 0, 7, 6);
    d2d_face_set(&f[1], 0, 6, 5);
    d2d_face_set(&f[2], 0, 5, 4);
    d2d_face_set(&f[3], 0, 4, 1);
    d2d_face_set(&f[4], 1, 4, 3);
    d2d_face_set(&f[5], 1, 3, 2);
    geometry->fill.face_count = 6;

    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[1], &v1, &v[2]))
        goto fail;
    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[3], &v2, &v[4]))
        goto fail;
    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[5], &v3, &v[6]))
        goto fail;
    if (!d2d_geometry_fill_add_arc_triangle(geometry, &v[7], &v4, &v[0]))
        goto fail;

    if (!d2d_geometry_outline_add_line_segment(geometry, &v[0], &v[1]))
        goto fail;
    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[1], &v1, &v[2]))
        goto fail;
    if (!d2d_geometry_outline_add_line_segment(geometry, &v[2], &v[3]))
        goto fail;
    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[3], &v2, &v[4]))
        goto fail;
    if (!d2d_geometry_outline_add_line_segment(geometry, &v[4], &v[5]))
        goto fail;
    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[5], &v3, &v[6]))
        goto fail;
    if (!d2d_geometry_outline_add_line_segment(geometry, &v[6], &v[7]))
        goto fail;
    if (!d2d_geometry_outline_add_arc_quadrant(geometry, &v[7], &v4, &v[0]))
        goto fail;

    return S_OK;

fail:
    d2d_geometry_cleanup(geometry);
    return E_OUTOFMEMORY;
}

static inline struct d2d_geometry *impl_from_ID2D1TransformedGeometry(ID2D1TransformedGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_QueryInterface(ID2D1TransformedGeometry *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1TransformedGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1TransformedGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_transformed_geometry_AddRef(ID2D1TransformedGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_transformed_geometry_Release(ID2D1TransformedGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        geometry->outline.arc_faces = NULL;
        geometry->outline.arcs = NULL;
        geometry->outline.bezier_faces = NULL;
        geometry->outline.beziers = NULL;
        geometry->outline.faces = NULL;
        geometry->outline.vertices = NULL;
        geometry->fill.arc_vertices = NULL;
        geometry->fill.bezier_vertices = NULL;
        geometry->fill.faces = NULL;
        geometry->fill.vertices = NULL;
        ID2D1Geometry_Release(geometry->u.transformed.src_geometry);
        d2d_geometry_cleanup(geometry);
        free(geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_transformed_geometry_GetFactory(ID2D1TransformedGeometry *iface,
        ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_GetBounds(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    D2D1_MATRIX_3X2_F g;

    TRACE("iface %p, transform %p, bounds %p.\n", iface, transform, bounds);

    g = geometry->transform;
    if (transform)
        d2d_matrix_multiply(&g, transform);

    return ID2D1Geometry_GetBounds(geometry->u.transformed.src_geometry, &g, bounds);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_GetWidenedBounds(ID2D1TransformedGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_StrokeContainsPoint(ID2D1TransformedGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    D2D1_MATRIX_3X2_F g;

    TRACE("iface %p, point %s, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, contains %p.\n",
            iface, debug_d2d_point_2f(&point), stroke_width, stroke_style, transform, tolerance, contains);

    g = geometry->transform;
    stroke_width /= g.m11;
    if (transform)
        d2d_matrix_multiply(&g, transform);

    if (tolerance <= 0.0f)
        tolerance = D2D1_DEFAULT_FLATTENING_TOLERANCE;

    return ID2D1Geometry_StrokeContainsPoint(geometry->u.transformed.src_geometry, point, stroke_width, stroke_style,
            &g, tolerance, contains);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_FillContainsPoint(ID2D1TransformedGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    D2D1_MATRIX_3X2_F g;

    TRACE("iface %p, point %s, transform %p, tolerance %.8e, contains %p.\n",
            iface, debug_d2d_point_2f(&point), transform, tolerance, contains);

    g = geometry->transform;
    if (transform)
        d2d_matrix_multiply(&g, transform);

    return ID2D1Geometry_FillContainsPoint(geometry->u.transformed.src_geometry, point, &g, tolerance, contains);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_CompareWithGeometry(ID2D1TransformedGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Simplify(ID2D1TransformedGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    D2D1_MATRIX_3X2_F g;

    TRACE("iface %p, option %#x, transform %p, tolerance %.8e, sink %p.\n",
            iface, option, transform, tolerance, sink);

    g = geometry->transform;
    if (transform)
        d2d_matrix_multiply(&g, transform);

    return ID2D1Geometry_Simplify(geometry->u.transformed.src_geometry, option, &g, tolerance, sink);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Tessellate(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_CombineWithGeometry(ID2D1TransformedGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Outline(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_ComputeArea(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_ComputeLength(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_ComputePointAtLength(ID2D1TransformedGeometry *iface,
        float length, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point,
        D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Widen(ID2D1TransformedGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_transformed_geometry_GetSourceGeometry(ID2D1TransformedGeometry *iface,
        ID2D1Geometry **src_geometry)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);

    TRACE("iface %p, src_geometry %p.\n", iface, src_geometry);

    ID2D1Geometry_AddRef(*src_geometry = geometry->u.transformed.src_geometry);
}

static void STDMETHODCALLTYPE d2d_transformed_geometry_GetTransform(ID2D1TransformedGeometry *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = geometry->u.transformed.transform;
}

static const struct ID2D1TransformedGeometryVtbl d2d_transformed_geometry_vtbl =
{
    d2d_transformed_geometry_QueryInterface,
    d2d_transformed_geometry_AddRef,
    d2d_transformed_geometry_Release,
    d2d_transformed_geometry_GetFactory,
    d2d_transformed_geometry_GetBounds,
    d2d_transformed_geometry_GetWidenedBounds,
    d2d_transformed_geometry_StrokeContainsPoint,
    d2d_transformed_geometry_FillContainsPoint,
    d2d_transformed_geometry_CompareWithGeometry,
    d2d_transformed_geometry_Simplify,
    d2d_transformed_geometry_Tessellate,
    d2d_transformed_geometry_CombineWithGeometry,
    d2d_transformed_geometry_Outline,
    d2d_transformed_geometry_ComputeArea,
    d2d_transformed_geometry_ComputeLength,
    d2d_transformed_geometry_ComputePointAtLength,
    d2d_transformed_geometry_Widen,
    d2d_transformed_geometry_GetSourceGeometry,
    d2d_transformed_geometry_GetTransform,
};

void d2d_transformed_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        ID2D1Geometry *src_geometry, const D2D_MATRIX_3X2_F *transform)
{
    struct d2d_geometry *src_impl;
    D2D_MATRIX_3X2_F g;

    src_impl = unsafe_impl_from_ID2D1Geometry(src_geometry);

    g = src_impl->transform;
    d2d_matrix_multiply(&g, transform);
    d2d_geometry_init(geometry, factory, &g, (ID2D1GeometryVtbl *)&d2d_transformed_geometry_vtbl);
    ID2D1Geometry_AddRef(geometry->u.transformed.src_geometry = src_geometry);
    geometry->u.transformed.transform = *transform;
    geometry->fill = src_impl->fill;
    geometry->outline = src_impl->outline;
}

static inline struct d2d_geometry *impl_from_ID2D1GeometryGroup(ID2D1GeometryGroup *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_QueryInterface(ID2D1GeometryGroup *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1GeometryGroup)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1GeometryGroup_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_geometry_group_AddRef(ID2D1GeometryGroup *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometryGroup(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_geometry_group_Release(ID2D1GeometryGroup *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometryGroup(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < geometry->u.group.geometry_count; ++i)
            ID2D1Geometry_Release(geometry->u.group.src_geometries[i]);
        free(geometry->u.group.src_geometries);
        d2d_geometry_cleanup(geometry);
        free(geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_geometry_group_GetFactory(ID2D1GeometryGroup *iface,
        ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometryGroup(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_GetBounds(ID2D1GeometryGroup *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometryGroup(iface);
    D2D1_RECT_F rect;
    unsigned int i;

    TRACE("iface %p, transform %p, bounds %p.\n", iface, transform, bounds);

    bounds->left = FLT_MAX;
    bounds->top = FLT_MAX;
    bounds->right = -FLT_MAX;
    bounds->bottom = -FLT_MAX;

    for (i = 0; i < geometry->u.group.geometry_count; ++i)
    {
        if (SUCCEEDED(ID2D1Geometry_GetBounds(geometry->u.group.src_geometries[i], transform, &rect)))
            d2d_rect_union(bounds, &rect);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_GetWidenedBounds(ID2D1GeometryGroup *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_StrokeContainsPoint(ID2D1GeometryGroup *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    FIXME("iface %p, point %s, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, contains %p.\n",
            iface, debug_d2d_point_2f(&point), stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_FillContainsPoint(ID2D1GeometryGroup *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point %s, transform %p, tolerance %.8e, contains %p stub!.\n",
            iface, debug_d2d_point_2f(&point), transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_CompareWithGeometry(ID2D1GeometryGroup *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_Simplify(ID2D1GeometryGroup *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!.\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_Tessellate(ID2D1GeometryGroup *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_CombineWithGeometry(ID2D1GeometryGroup *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_Outline(ID2D1GeometryGroup *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_ComputeArea(ID2D1GeometryGroup *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_ComputeLength(ID2D1GeometryGroup *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_ComputePointAtLength(ID2D1GeometryGroup *iface,
        float length, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point,
        D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_group_Widen(ID2D1GeometryGroup *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static D2D1_FILL_MODE STDMETHODCALLTYPE d2d_geometry_group_GetFillMode(ID2D1GeometryGroup *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometryGroup(iface);

    TRACE("iface %p.\n", iface);

    return geometry->u.group.fill_mode;
}

static UINT32 STDMETHODCALLTYPE d2d_geometry_group_GetSourceGeometryCount(ID2D1GeometryGroup *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometryGroup(iface);

    TRACE("iface %p.\n", iface);

    return geometry->u.group.geometry_count;
}

static void STDMETHODCALLTYPE d2d_geometry_group_GetSourceGeometries(ID2D1GeometryGroup *iface,
        ID2D1Geometry **geometries, UINT32 geometry_count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometryGroup(iface);
    unsigned int i;

    TRACE("iface %p, geometries %p, geometry_count %u.\n", iface, geometries, geometry_count);

    geometry_count = min(geometry_count, geometry->u.group.geometry_count);
    for (i = 0; i < geometry_count; ++i)
        ID2D1Geometry_AddRef(geometries[i] = geometry->u.group.src_geometries[i]);
}

static const struct ID2D1GeometryGroupVtbl d2d_geometry_group_vtbl =
{
    d2d_geometry_group_QueryInterface,
    d2d_geometry_group_AddRef,
    d2d_geometry_group_Release,
    d2d_geometry_group_GetFactory,
    d2d_geometry_group_GetBounds,
    d2d_geometry_group_GetWidenedBounds,
    d2d_geometry_group_StrokeContainsPoint,
    d2d_geometry_group_FillContainsPoint,
    d2d_geometry_group_CompareWithGeometry,
    d2d_geometry_group_Simplify,
    d2d_geometry_group_Tessellate,
    d2d_geometry_group_CombineWithGeometry,
    d2d_geometry_group_Outline,
    d2d_geometry_group_ComputeArea,
    d2d_geometry_group_ComputeLength,
    d2d_geometry_group_ComputePointAtLength,
    d2d_geometry_group_Widen,
    d2d_geometry_group_GetFillMode,
    d2d_geometry_group_GetSourceGeometryCount,
    d2d_geometry_group_GetSourceGeometries,
};

HRESULT d2d_geometry_group_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        D2D1_FILL_MODE fill_mode, ID2D1Geometry **geometries, unsigned int geometry_count)
{
    unsigned int i;

    d2d_geometry_init(geometry, factory, &identity, (ID2D1GeometryVtbl *)&d2d_geometry_group_vtbl);

    if (!(geometry->u.group.src_geometries = calloc(geometry_count, sizeof(*geometries))))
    {
        d2d_geometry_cleanup(geometry);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < geometry_count; ++i)
    {
        ID2D1Geometry_AddRef(geometry->u.group.src_geometries[i] = geometries[i]);
    }
    geometry->u.group.geometry_count = geometry_count;
    geometry->u.group.fill_mode = fill_mode;

    return S_OK;
}

struct d2d_geometry *unsafe_impl_from_ID2D1Geometry(ID2D1Geometry *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_ellipse_geometry_vtbl
            || iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_path_geometry_vtbl
            || iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_rectangle_geometry_vtbl
            || iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_rounded_rectangle_geometry_vtbl
            || iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_transformed_geometry_vtbl
            || iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_geometry_group_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}
