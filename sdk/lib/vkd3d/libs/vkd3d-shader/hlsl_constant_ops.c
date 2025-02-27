/*
 * HLSL constant value operations for constant folding
 *
 * Copyright 2022 Francisco Casas for CodeWeavers
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

#include <math.h>

#include "hlsl.h"

static bool fold_abs(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = fabsf(src->value.u[k].f);
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].d = fabs(src->value.u[k].d);
                break;

            case HLSL_TYPE_INT:
                /* C's abs(INT_MIN) is undefined, but HLSL evaluates this to INT_MIN */
                if (src->value.u[k].i == INT_MIN)
                    dst->u[k].i = INT_MIN;
                else
                    dst->u[k].i = abs(src->value.u[k].i);
                break;

            case HLSL_TYPE_UINT:
                dst->u[k].u = src->value.u[k].u;
                break;

            default:
                FIXME("Fold abs() for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static uint32_t float_to_uint(float x)
{
    if (isnan(x) || x <= 0)
        return 0;

    if (x >= 4294967296.0f)
        return UINT32_MAX;

    return x;
}

static int32_t float_to_int(float x)
{
    if (isnan(x))
        return 0;

    if (x <= -2147483648.0f)
        return INT32_MIN;

    if (x >= 2147483648.0f)
        return INT32_MAX;

    return x;
}

static uint32_t double_to_uint(double x)
{
    if (isnan(x) || x <= 0)
        return 0;

    if (x >= 4294967296.0)
        return UINT32_MAX;

    return x;
}

static int32_t double_to_int(double x)
{
    if (isnan(x))
        return 0;

    if (x <= -2147483648.0)
        return INT32_MIN;

    if (x >= 2147483648.0)
        return INT32_MAX;

    return x;
}

static bool fold_bit_not(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                dst->u[k].u = ~src->value.u[k].u;
                break;

            default:
                vkd3d_unreachable();
        }
    }

    return true;
}

static bool fold_cast(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    unsigned int k;
    uint32_t u;
    int32_t i;
    double d;
    float f;

    if (dst_type->dimx != src->node.data_type->dimx
            || dst_type->dimy != src->node.data_type->dimy)
    {
        FIXME("Cast from %s to %s.\n", debug_hlsl_type(ctx, src->node.data_type),
                debug_hlsl_type(ctx, dst_type));
        return false;
    }

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (src->node.data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                u = float_to_uint(src->value.u[k].f);
                i = float_to_int(src->value.u[k].f);
                f = src->value.u[k].f;
                d = src->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                u = double_to_uint(src->value.u[k].d);
                i = double_to_int(src->value.u[k].d);
                f = src->value.u[k].d;
                d = src->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
                u = src->value.u[k].i;
                i = src->value.u[k].i;
                f = src->value.u[k].i;
                d = src->value.u[k].i;
                break;

            case HLSL_TYPE_UINT:
                u = src->value.u[k].u;
                i = src->value.u[k].u;
                f = src->value.u[k].u;
                d = src->value.u[k].u;
                break;

            case HLSL_TYPE_BOOL:
                u = !!src->value.u[k].u;
                i = !!src->value.u[k].u;
                f = !!src->value.u[k].u;
                d = !!src->value.u[k].u;
                break;

            default:
                vkd3d_unreachable();
        }

        switch (dst_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].d = d;
                break;

            case HLSL_TYPE_INT:
                dst->u[k].i = i;
                break;

            case HLSL_TYPE_UINT:
                dst->u[k].u = u;
                break;

            case HLSL_TYPE_BOOL:
                /* Casts to bool should have already been lowered. */
            default:
                vkd3d_unreachable();
        }
    }
    return true;
}

static bool fold_ceil(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = ceilf(src->value.u[k].f);
                break;

            default:
                FIXME("Fold 'ceil' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_exp2(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = exp2f(src->value.u[k].f);
                break;

            default:
                FIXME("Fold 'exp2' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_floor(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = floorf(src->value.u[k].f);
                break;

            default:
                FIXME("Fold 'floor' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_fract(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;
    float i;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = modff(src->value.u[k].f, &i);
                break;

            default:
                FIXME("Fold 'fract' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_log2(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                if (ctx->profile->major_version >= 4 && src->value.u[k].f < 0.0f)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_NON_FINITE_RESULT,
                            "Indefinite logarithm result.");
                }
                dst->u[k].f = log2f(src->value.u[k].f);
                if (ctx->profile->major_version < 4 && !isfinite(dst->u[k].f))
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NON_FINITE_RESULT,
                            "Infinities and NaNs are not allowed by the shader model.");
                }
                break;

            case HLSL_TYPE_DOUBLE:
                if (src->value.u[k].d < 0.0)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_NON_FINITE_RESULT,
                            "Indefinite logarithm result.");
                }
                dst->u[k].d = log2(src->value.u[k].d);
                break;

            default:
                FIXME("Fold 'log2' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_neg(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = -src->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].d = -src->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
                dst->u[k].u = -src->value.u[k].u;
                break;

            default:
                FIXME("Fold negation for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_not(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst,
        const struct hlsl_type *dst_type, const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_BOOL:
                dst->u[k].u = ~src->value.u[k].u;
                break;

            default:
                FIXME("Fold logic 'not' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_rcp(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                if (ctx->profile->major_version >= 4 && src->value.u[k].f == 0.0f)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_DIVISION_BY_ZERO,
                            "Floating point division by zero.");
                }
                dst->u[k].f = 1.0f / src->value.u[k].f;
                if (ctx->profile->major_version < 4 && !isfinite(dst->u[k].f))
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_DIVISION_BY_ZERO,
                            "Infinities and NaNs are not allowed by the shader model.");
                }
                break;

            case HLSL_TYPE_DOUBLE:
                if (src->value.u[k].d == 0.0)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_DIVISION_BY_ZERO,
                            "Floating point division by zero.");
                }
                dst->u[k].d = 1.0 / src->value.u[k].d;
                break;

            default:
                FIXME("Fold 'rcp' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_rsq(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                if (ctx->profile->major_version >= 4)
                {
                    if (src->value.u[k].f < 0.0f)
                        hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_IMAGINARY_NUMERIC_RESULT,
                                "Imaginary square root result.");
                    else if (src->value.u[k].f == 0.0f)
                        hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_IMAGINARY_NUMERIC_RESULT,
                                "Floating point division by zero.");
                }
                dst->u[k].f = 1.0f / sqrtf(src->value.u[k].f);
                if (ctx->profile->major_version < 4 && !isfinite(dst->u[k].f))
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NON_FINITE_RESULT,
                            "Infinities and NaNs are not allowed by the shader model.");
                }
                break;

            default:
                FIXME("Fold 'rsq' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_sat(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = min(max(0.0f, src->value.u[k].f), 1.0f);
                break;

            default:
                FIXME("Fold 'sat' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_sqrt(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                if (ctx->profile->major_version >= 4 && src->value.u[k].f < 0.0f)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_IMAGINARY_NUMERIC_RESULT,
                            "Imaginary square root result.");
                }
                dst->u[k].f = sqrtf(src->value.u[k].f);
                if (ctx->profile->major_version < 4 && !isfinite(dst->u[k].f))
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NON_FINITE_RESULT,
                            "Infinities and NaNs are not allowed by the shader model.");
                }
                break;

            case HLSL_TYPE_DOUBLE:
                if (src->value.u[k].d < 0.0)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_IMAGINARY_NUMERIC_RESULT,
                            "Imaginary square root result.");
                }
                dst->u[k].d = sqrt(src->value.u[k].d);
                break;

            default:
                FIXME("Fold 'sqrt' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_add(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = src1->value.u[k].f + src2->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].d = src1->value.u[k].d + src2->value.u[k].d;
                break;

            /* Handling HLSL_TYPE_INT through the unsigned field to avoid
             * undefined behavior with signed integers in C. */
            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
                dst->u[k].u = src1->value.u[k].u + src2->value.u[k].u;
                break;

            default:
                FIXME("Fold addition for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_and(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                dst->u[k].u = src1->value.u[k].u & src2->value.u[k].u;
                break;

            default:
                FIXME("Fold bit/logic and for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_or(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                dst->u[k].u = src1->value.u[k].u | src2->value.u[k].u;
                break;

            default:
                FIXME("Fold bit/logic or for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_bit_xor(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
                dst->u[k].u = src1->value.u[k].u ^ src2->value.u[k].u;
                break;

            default:
                FIXME("Fold bit xor for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_dot(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);
    VKD3D_ASSERT(src1->node.data_type->dimx == src2->node.data_type->dimx);

    dst->u[0].f = 0.0f;
    for (k = 0; k < src1->node.data_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[0].f += src1->value.u[k].f * src2->value.u[k].f;
                break;
            default:
                FIXME("Fold 'dot' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_dp2add(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2, const struct hlsl_ir_constant *src3)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src3->node.data_type->e.numeric.type);
    VKD3D_ASSERT(src1->node.data_type->dimx == src2->node.data_type->dimx);
    VKD3D_ASSERT(src3->node.data_type->dimx == 1);

    dst->u[0].f = src3->value.u[0].f;
    for (k = 0; k < src1->node.data_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[0].f += src1->value.u[k].f * src2->value.u[k].f;
                break;
            default:
                FIXME("Fold 'dp2add' for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }

    return true;
}

static bool fold_div(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2,
        const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                if (ctx->profile->major_version >= 4 && src2->value.u[k].f == 0)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_DIVISION_BY_ZERO,
                            "Floating point division by zero.");
                }
                dst->u[k].f = src1->value.u[k].f / src2->value.u[k].f;
                if (ctx->profile->major_version < 4 && !isfinite(dst->u[k].f))
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_DIVISION_BY_ZERO,
                            "Infinities and NaNs are not allowed by the shader model.");
                }
                break;

            case HLSL_TYPE_DOUBLE:
                if (src2->value.u[k].d == 0)
                {
                    hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_DIVISION_BY_ZERO,
                            "Floating point division by zero.");
                }
                dst->u[k].d = src1->value.u[k].d / src2->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
                if (src2->value.u[k].i == 0)
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_DIVISION_BY_ZERO,
                            "Division by zero.");
                    return false;
                }
                if (src1->value.u[k].i == INT_MIN && src2->value.u[k].i == -1)
                    dst->u[k].i = INT_MIN;
                else
                    dst->u[k].i = src1->value.u[k].i / src2->value.u[k].i;
                break;

            case HLSL_TYPE_UINT:
                if (src2->value.u[k].u == 0)
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_DIVISION_BY_ZERO,
                            "Division by zero.");
                    return false;
                }
                dst->u[k].u = src1->value.u[k].u / src2->value.u[k].u;
                break;

            default:
                FIXME("Fold division for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_equal(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    unsigned int k;

    VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
    VKD3D_ASSERT(src1->node.data_type->e.numeric.type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (src1->node.data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].u = src1->value.u[k].f == src2->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].u = src1->value.u[k].d == src2->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                dst->u[k].u = src1->value.u[k].u == src2->value.u[k].u;
                break;

            default:
                vkd3d_unreachable();
        }

        dst->u[k].u *= ~0u;
    }
    return true;
}

static bool fold_gequal(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    unsigned int k;

    VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
    VKD3D_ASSERT(src1->node.data_type->e.numeric.type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (src1->node.data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].u = src1->value.u[k].f >= src2->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].u = src1->value.u[k].d >= src2->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
                dst->u[k].u = src1->value.u[k].i >= src2->value.u[k].i;
                break;

            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                dst->u[k].u = src1->value.u[k].u >= src2->value.u[k].u;
                break;

            default:
                vkd3d_unreachable();
        }

        dst->u[k].u *= ~0u;
    }
    return true;
}

static bool fold_less(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    unsigned int k;

    VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
    VKD3D_ASSERT(src1->node.data_type->e.numeric.type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (src1->node.data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].u = src1->value.u[k].f < src2->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].u = src1->value.u[k].d < src2->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
                dst->u[k].u = src1->value.u[k].i < src2->value.u[k].i;
                break;

            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                dst->u[k].u = src1->value.u[k].u < src2->value.u[k].u;
                break;

            default:
                vkd3d_unreachable();
        }

        dst->u[k].u *= ~0u;
    }
    return true;
}

static bool fold_lshift(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    unsigned int k;

    VKD3D_ASSERT(dst_type->e.numeric.type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(src2->node.data_type->e.numeric.type == HLSL_TYPE_INT);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        unsigned int shift = src2->value.u[k].u % 32;

        switch (src1->node.data_type->e.numeric.type)
        {
            case HLSL_TYPE_INT:
                dst->u[k].i = src1->value.u[k].i << shift;
                break;

            case HLSL_TYPE_UINT:
                dst->u[k].u = src1->value.u[k].u << shift;
                break;

            default:
                vkd3d_unreachable();
        }
    }

    return true;
}

static bool fold_max(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = fmaxf(src1->value.u[k].f, src2->value.u[k].f);
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].d = fmax(src1->value.u[k].d, src2->value.u[k].d);
                break;

            case HLSL_TYPE_INT:
                dst->u[k].i = max(src1->value.u[k].i, src2->value.u[k].i);
                break;

            case HLSL_TYPE_UINT:
                dst->u[k].u = max(src1->value.u[k].u, src2->value.u[k].u);
                break;

            default:
                FIXME("Fold max for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_min(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = fminf(src1->value.u[k].f, src2->value.u[k].f);
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].d = fmin(src1->value.u[k].d, src2->value.u[k].d);
                break;

            case HLSL_TYPE_INT:
                dst->u[k].i = min(src1->value.u[k].i, src2->value.u[k].i);
                break;

            case HLSL_TYPE_UINT:
                dst->u[k].u = min(src1->value.u[k].u, src2->value.u[k].u);
                break;

            default:
                FIXME("Fold min for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_mod(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2,
        const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_INT:
                if (src2->value.u[k].i == 0)
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_DIVISION_BY_ZERO, "Division by zero.");
                    return false;
                }
                if (src1->value.u[k].i == INT_MIN && src2->value.u[k].i == -1)
                    dst->u[k].i = 0;
                else
                    dst->u[k].i = src1->value.u[k].i % src2->value.u[k].i;
                break;

            case HLSL_TYPE_UINT:
                if (src2->value.u[k].u == 0)
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_DIVISION_BY_ZERO, "Division by zero.");
                    return false;
                }
                dst->u[k].u = src1->value.u[k].u % src2->value.u[k].u;
                break;

            default:
                FIXME("Fold modulus for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_mul(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    enum hlsl_base_type type = dst_type->e.numeric.type;
    unsigned int k;

    VKD3D_ASSERT(type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].f = src1->value.u[k].f * src2->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].d = src1->value.u[k].d * src2->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
                dst->u[k].u = src1->value.u[k].u * src2->value.u[k].u;
                break;

            default:
                FIXME("Fold multiplication for type %s.\n", debug_hlsl_type(ctx, dst_type));
                return false;
        }
    }
    return true;
}

static bool fold_nequal(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    unsigned int k;

    VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
    VKD3D_ASSERT(src1->node.data_type->e.numeric.type == src2->node.data_type->e.numeric.type);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        switch (src1->node.data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                dst->u[k].u = src1->value.u[k].f != src2->value.u[k].f;
                break;

            case HLSL_TYPE_DOUBLE:
                dst->u[k].u = src1->value.u[k].d != src2->value.u[k].d;
                break;

            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                dst->u[k].u = src1->value.u[k].u != src2->value.u[k].u;
                break;

            default:
                vkd3d_unreachable();
        }

        dst->u[k].u *= ~0u;
    }
    return true;
}

static bool fold_ternary(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2, const struct hlsl_ir_constant *src3)
{
    unsigned int k;

    VKD3D_ASSERT(dst_type->e.numeric.type == src2->node.data_type->e.numeric.type);
    VKD3D_ASSERT(dst_type->e.numeric.type == src3->node.data_type->e.numeric.type);
    VKD3D_ASSERT(src1->node.data_type->e.numeric.type == HLSL_TYPE_BOOL);

    for (k = 0; k < dst_type->dimx; ++k)
        dst->u[k] = src1->value.u[k].u ? src2->value.u[k] : src3->value.u[k];

    return true;
}

static bool fold_rshift(struct hlsl_ctx *ctx, struct hlsl_constant_value *dst, const struct hlsl_type *dst_type,
        const struct hlsl_ir_constant *src1, const struct hlsl_ir_constant *src2)
{
    unsigned int k;

    VKD3D_ASSERT(dst_type->e.numeric.type == src1->node.data_type->e.numeric.type);
    VKD3D_ASSERT(src2->node.data_type->e.numeric.type == HLSL_TYPE_INT);

    for (k = 0; k < dst_type->dimx; ++k)
    {
        unsigned int shift = src2->value.u[k].u % 32;

        switch (src1->node.data_type->e.numeric.type)
        {
            case HLSL_TYPE_INT:
                dst->u[k].i = src1->value.u[k].i >> shift;
                break;

            case HLSL_TYPE_UINT:
                dst->u[k].u = src1->value.u[k].u >> shift;
                break;

            default:
                vkd3d_unreachable();
        }
    }

    return true;
}

bool hlsl_fold_constant_exprs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_constant *arg1, *arg2 = NULL, *arg3 = NULL;
    struct hlsl_constant_value res = {0};
    struct hlsl_ir_node *res_node;
    struct hlsl_ir_expr *expr;
    unsigned int i;
    bool success;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (!expr->operands[0].node)
        return false;

    if (instr->data_type->class > HLSL_CLASS_VECTOR)
        return false;

    for (i = 0; i < ARRAY_SIZE(expr->operands); ++i)
    {
        if (expr->operands[i].node)
        {
            if (expr->operands[i].node->type != HLSL_IR_CONSTANT)
                return false;
            VKD3D_ASSERT(expr->operands[i].node->data_type->class <= HLSL_CLASS_VECTOR);
        }
    }
    arg1 = hlsl_ir_constant(expr->operands[0].node);
    if (expr->operands[1].node)
        arg2 = hlsl_ir_constant(expr->operands[1].node);
    if (expr->operands[2].node)
        arg3 = hlsl_ir_constant(expr->operands[2].node);

    switch (expr->op)
    {
        case HLSL_OP1_ABS:
            success = fold_abs(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_BIT_NOT:
            success = fold_bit_not(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_CAST:
            success = fold_cast(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_CEIL:
            success = fold_ceil(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_EXP2:
            success = fold_exp2(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_FLOOR:
            success = fold_floor(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_FRACT:
            success = fold_fract(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_LOG2:
            success = fold_log2(ctx, &res, instr->data_type, arg1, &instr->loc);
            break;

        case HLSL_OP1_NEG:
            success = fold_neg(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_LOGIC_NOT:
            success = fold_not(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_RCP:
            success = fold_rcp(ctx, &res, instr->data_type, arg1, &instr->loc);
            break;

        case HLSL_OP1_RSQ:
            success = fold_rsq(ctx, &res, instr->data_type, arg1, &instr->loc);
            break;

        case HLSL_OP1_SAT:
            success = fold_sat(ctx, &res, instr->data_type, arg1);
            break;

        case HLSL_OP1_SQRT:
            success = fold_sqrt(ctx, &res, instr->data_type, arg1, &instr->loc);
            break;

        case HLSL_OP2_ADD:
            success = fold_add(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_BIT_AND:
        case HLSL_OP2_LOGIC_AND:
            success = fold_and(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_BIT_OR:
        case HLSL_OP2_LOGIC_OR:
            success = fold_or(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_BIT_XOR:
            success = fold_bit_xor(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_DOT:
            success = fold_dot(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_DIV:
            success = fold_div(ctx, &res, instr->data_type, arg1, arg2, &instr->loc);
            break;

        case HLSL_OP2_EQUAL:
            success = fold_equal(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_GEQUAL:
            success = fold_gequal(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_LESS:
            success = fold_less(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_LSHIFT:
            success = fold_lshift(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_MAX:
            success = fold_max(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_MIN:
            success = fold_min(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_MOD:
            success = fold_mod(ctx, &res, instr->data_type, arg1, arg2, &instr->loc);
            break;

        case HLSL_OP2_MUL:
            success = fold_mul(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_NEQUAL:
            success = fold_nequal(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP2_RSHIFT:
            success = fold_rshift(ctx, &res, instr->data_type, arg1, arg2);
            break;

        case HLSL_OP3_DP2ADD:
            success = fold_dp2add(ctx, &res, instr->data_type, arg1, arg2, arg3);
            break;

        case HLSL_OP3_TERNARY:
            success = fold_ternary(ctx, &res, instr->data_type, arg1, arg2, arg3);
            break;

        default:
            FIXME("Fold \"%s\" expression.\n", debug_hlsl_expr_op(expr->op));
            success = false;
            break;
    }

    if (success)
    {
        if (!(res_node = hlsl_new_constant(ctx, instr->data_type, &res, &instr->loc)))
            return false;
        list_add_before(&expr->node.entry, &res_node->entry);
        hlsl_replace_node(&expr->node, res_node);
    }
    return success;
}

static bool constant_is_zero(struct hlsl_ir_constant *const_arg)
{
    struct hlsl_type *data_type = const_arg->node.data_type;
    unsigned int k;

    for (k = 0; k < data_type->dimx; ++k)
    {
        switch (data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                if (const_arg->value.u[k].f != 0.0f)
                    return false;
                break;

            case HLSL_TYPE_DOUBLE:
                if (const_arg->value.u[k].d != 0.0)
                    return false;
                break;

            case HLSL_TYPE_UINT:
            case HLSL_TYPE_INT:
            case HLSL_TYPE_BOOL:
                if (const_arg->value.u[k].u != 0)
                    return false;
                break;

            default:
                return false;
        }
    }
    return true;
}

static bool constant_is_one(struct hlsl_ir_constant *const_arg)
{
    struct hlsl_type *data_type = const_arg->node.data_type;
    unsigned int k;

    for (k = 0; k < data_type->dimx; ++k)
    {
        switch (data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                if (const_arg->value.u[k].f != 1.0f)
                    return false;
                break;

            case HLSL_TYPE_DOUBLE:
                if (const_arg->value.u[k].d != 1.0)
                    return false;
                break;

            case HLSL_TYPE_UINT:
            case HLSL_TYPE_INT:
                if (const_arg->value.u[k].u != 1)
                    return false;
                break;

            case HLSL_TYPE_BOOL:
                if (const_arg->value.u[k].u != ~0)
                    return false;
                break;

            default:
                return false;
        }
    }
    return true;
}

bool hlsl_fold_constant_identities(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_constant *const_arg = NULL;
    struct hlsl_ir_node *mut_arg = NULL;
    struct hlsl_ir_node *res_node;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (instr->data_type->class > HLSL_CLASS_VECTOR)
        return false;

    /* Verify that the expression has two operands. */
    for (i = 0; i < ARRAY_SIZE(expr->operands); ++i)
    {
        if (!!expr->operands[i].node != (i < 2))
            return false;
    }

    if (expr->operands[0].node->type == HLSL_IR_CONSTANT)
    {
        const_arg = hlsl_ir_constant(expr->operands[0].node);
        mut_arg = expr->operands[1].node;
    }
    else if (expr->operands[1].node->type == HLSL_IR_CONSTANT)
    {
        mut_arg = expr->operands[0].node;
        const_arg = hlsl_ir_constant(expr->operands[1].node);
    }
    else
    {
        return false;
    }

    res_node = NULL;
    switch (expr->op)
    {
        case HLSL_OP2_ADD:
            if (constant_is_zero(const_arg))
                res_node = mut_arg;
            break;

        case HLSL_OP2_MUL:
            if (constant_is_one(const_arg))
                res_node = mut_arg;
            break;

        case HLSL_OP2_LOGIC_AND:
            if (constant_is_zero(const_arg))
                res_node = &const_arg->node;
            else if (constant_is_one(const_arg))
                res_node = mut_arg;
            break;

        case HLSL_OP2_LOGIC_OR:
            if (constant_is_zero(const_arg))
                res_node = mut_arg;
            else if (constant_is_one(const_arg))
                res_node = &const_arg->node;
            break;

        default:
            break;
    }

    if (res_node)
    {
        hlsl_replace_node(&expr->node, res_node);
        return true;
    }
    return false;
}

bool hlsl_fold_constant_swizzles(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_constant_value value;
    struct hlsl_ir_swizzle *swizzle;
    struct hlsl_ir_constant *src;
    struct hlsl_ir_node *dst;
    unsigned int i;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);
    if (swizzle->val.node->type != HLSL_IR_CONSTANT)
        return false;
    src = hlsl_ir_constant(swizzle->val.node);

    for (i = 0; i < swizzle->node.data_type->dimx; ++i)
        value.u[i] = src->value.u[hlsl_swizzle_get_component(swizzle->swizzle, i)];

    if (!(dst = hlsl_new_constant(ctx, instr->data_type, &value, &instr->loc)))
        return false;

    list_add_before(&swizzle->node.entry, &dst->entry);
    hlsl_replace_node(&swizzle->node, dst);
    return true;
}
