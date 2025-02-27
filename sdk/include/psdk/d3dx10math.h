/*
 * Copyright (C) 2016 Alistair Leslie-Hughes
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

#include "d3dx10.h"

/* This guard is the same as D3DX9 to prevent double-inclusion */
#ifndef __D3DX9MATH_H__
#define __D3DX9MATH_H__

#include <math.h>

#ifndef D3DVECTOR_DEFINED
#define D3DVECTOR_DEFINED
typedef struct _D3DVECTOR
{
    float x;
    float y;
    float z;
} D3DVECTOR;
#endif

#ifndef D3DMATRIX_DEFINED
#define D3DMATRIX_DEFINED
typedef struct _D3DMATRIX
{
    union
    {
        struct
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DMATRIX;
#endif

typedef enum _D3DX_CPU_OPTIMIZATION
{
    D3DX_NOT_OPTIMIZED,
    D3DX_3DNOW_OPTIMIZED,
    D3DX_SSE2_OPTIMIZED,
    D3DX_SSE_OPTIMIZED
} D3DX_CPU_OPTIMIZATION;

typedef struct D3DXFLOAT16
{
#ifdef __cplusplus
    D3DXFLOAT16();
    D3DXFLOAT16(float f);
    D3DXFLOAT16(const D3DXFLOAT16 &f);

    operator float();

    BOOL operator==(const D3DXFLOAT16 &f) const;
    BOOL operator!=(const D3DXFLOAT16 &f) const;
#endif
    WORD value;
} D3DXFLOAT16, *LPD3DXFLOAT16;

typedef struct D3DXCOLOR
{
#ifdef __cplusplus
public:
    D3DXCOLOR(){};
    D3DXCOLOR(UINT color);
    D3DXCOLOR(const float *color);
    D3DXCOLOR(const D3DXFLOAT16 *color);
    D3DXCOLOR(float r, float g, float b, float a);

    operator UINT() const;
    operator float *();
    operator const float *() const;

    D3DXCOLOR & operator+=(const D3DXCOLOR &color);
    D3DXCOLOR & operator-=(const D3DXCOLOR &color);
    D3DXCOLOR & operator*=(float n);
    D3DXCOLOR & operator/=(float n);

    D3DXCOLOR operator+() const;
    D3DXCOLOR operator-() const;

    D3DXCOLOR operator+(const D3DXCOLOR &color) const;
    D3DXCOLOR operator-(const D3DXCOLOR &color) const;
    D3DXCOLOR operator*(float n) const;
    D3DXCOLOR operator/(float n) const;

    friend D3DXCOLOR operator*(float n, const D3DXCOLOR &color);

    BOOL operator==(const D3DXCOLOR &color) const;
    BOOL operator!=(const D3DXCOLOR &color) const;
#endif
    float r, g, b, a;
} D3DXCOLOR, *LPD3DXCOLOR;

typedef struct D3DXVECTOR2
{
#ifdef __cplusplus
    D3DXVECTOR2() {};
    D3DXVECTOR2(const float *pf);
    D3DXVECTOR2(float fx, float fy);

    operator float* ();
    operator const float* () const;

    D3DXVECTOR2& operator += (const D3DXVECTOR2&);
    D3DXVECTOR2& operator -= (const D3DXVECTOR2&);
    D3DXVECTOR2& operator *= (float);
    D3DXVECTOR2& operator /= (float);

    D3DXVECTOR2 operator + () const;
    D3DXVECTOR2 operator - () const;

    D3DXVECTOR2 operator + (const D3DXVECTOR2&) const;
    D3DXVECTOR2 operator - (const D3DXVECTOR2&) const;
    D3DXVECTOR2 operator * (float) const;
    D3DXVECTOR2 operator / (float) const;

    friend D3DXVECTOR2 operator * (float, const D3DXVECTOR2&);

    BOOL operator == (const D3DXVECTOR2&) const;
    BOOL operator != (const D3DXVECTOR2&) const;
#endif /* __cplusplus */
    float x, y;
} D3DXVECTOR2, *LPD3DXVECTOR2;

#ifdef __cplusplus
typedef struct D3DXVECTOR3 : public D3DVECTOR
{
    D3DXVECTOR3() {};
    D3DXVECTOR3(const float *pf);
    D3DXVECTOR3(const D3DVECTOR& v);
    D3DXVECTOR3(float fx, float fy, float fz);

    operator float* ();
    operator const float* () const;

    D3DXVECTOR3& operator += (const D3DXVECTOR3&);
    D3DXVECTOR3& operator -= (const D3DXVECTOR3&);
    D3DXVECTOR3& operator *= (float);
    D3DXVECTOR3& operator /= (float);

    D3DXVECTOR3 operator + () const;
    D3DXVECTOR3 operator - () const;

    D3DXVECTOR3 operator + (const D3DXVECTOR3&) const;
    D3DXVECTOR3 operator - (const D3DXVECTOR3&) const;
    D3DXVECTOR3 operator * (float) const;
    D3DXVECTOR3 operator / (float) const;

    friend D3DXVECTOR3 operator * (float, const struct D3DXVECTOR3&);

    BOOL operator == (const D3DXVECTOR3&) const;
    BOOL operator != (const D3DXVECTOR3&) const;
} D3DXVECTOR3, *LPD3DXVECTOR3;
#else /* !__cplusplus */
typedef struct _D3DVECTOR D3DXVECTOR3, *LPD3DXVECTOR3;
#endif /* !__cplusplus */

typedef struct D3DXVECTOR4
{
#ifdef __cplusplus
    D3DXVECTOR4() {};
    D3DXVECTOR4(const float *pf);
    D3DXVECTOR4(float fx, float fy, float fz, float fw);

    operator float* ();
    operator const float* () const;

    D3DXVECTOR4& operator += (const D3DXVECTOR4&);
    D3DXVECTOR4& operator -= (const D3DXVECTOR4&);
    D3DXVECTOR4& operator *= (float);
    D3DXVECTOR4& operator /= (float);

    D3DXVECTOR4 operator + () const;
    D3DXVECTOR4 operator - () const;

    D3DXVECTOR4 operator + (const D3DXVECTOR4&) const;
    D3DXVECTOR4 operator - (const D3DXVECTOR4&) const;
    D3DXVECTOR4 operator * (float) const;
    D3DXVECTOR4 operator / (float) const;

    friend D3DXVECTOR4 operator * (float, const D3DXVECTOR4&);

    BOOL operator == (const D3DXVECTOR4&) const;
    BOOL operator != (const D3DXVECTOR4&) const;
#endif /* __cplusplus */
    float x, y, z, w;
} D3DXVECTOR4, *LPD3DXVECTOR4;

#ifdef __cplusplus
typedef struct D3DXMATRIX : public D3DMATRIX
{
    D3DXMATRIX() {};
    D3DXMATRIX(const float *pf);
    D3DXMATRIX(const D3DMATRIX& mat);
    D3DXMATRIX(float f11, float f12, float f13, float f14,
               float f21, float f22, float f23, float f24,
               float f31, float f32, float f33, float f34,
               float f41, float f42, float f43, float f44);

    float& operator () (UINT row, UINT col);
    float operator () (UINT row, UINT col) const;

    operator float* ();
    operator const float* () const;

    D3DXMATRIX& operator *= (const D3DXMATRIX&);
    D3DXMATRIX& operator += (const D3DXMATRIX&);
    D3DXMATRIX& operator -= (const D3DXMATRIX&);
    D3DXMATRIX& operator *= (float);
    D3DXMATRIX& operator /= (float);

    D3DXMATRIX operator + () const;
    D3DXMATRIX operator - () const;

    D3DXMATRIX operator * (const D3DXMATRIX&) const;
    D3DXMATRIX operator + (const D3DXMATRIX&) const;
    D3DXMATRIX operator - (const D3DXMATRIX&) const;
    D3DXMATRIX operator * (float) const;
    D3DXMATRIX operator / (float) const;

    friend D3DXMATRIX operator * (float, const D3DXMATRIX&);

    BOOL operator == (const D3DXMATRIX&) const;
    BOOL operator != (const D3DXMATRIX&) const;
} D3DXMATRIX, *LPD3DXMATRIX;
#else /* !__cplusplus */
typedef struct _D3DMATRIX D3DXMATRIX, *LPD3DXMATRIX;
#endif /* !__cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

D3DXCOLOR *D3DXColorAdd(D3DXCOLOR *out, D3DXCOLOR c1, D3DXCOLOR c2);
D3DXCOLOR *D3DXColorLerp(D3DXCOLOR *out, D3DXCOLOR c1, D3DXCOLOR c2, float s);
D3DXCOLOR *D3DXColorModulate(D3DXCOLOR *out, D3DXCOLOR c1, D3DXCOLOR c2);
D3DXCOLOR *D3DXColorScale(D3DXCOLOR *out, D3DXCOLOR c, float s);
D3DXCOLOR *D3DXColorSubtract(D3DXCOLOR *out, D3DXCOLOR c1, D3DXCOLOR c2);
D3DX_CPU_OPTIMIZATION WINAPI D3DXCpuOptimizations(BOOL enable);

#ifdef __cplusplus
}
#endif

#endif /* __D3DX9MATH_H__ */
