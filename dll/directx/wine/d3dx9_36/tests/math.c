/*
 * Copyright 2008 David Adam
 * Copyright 2008 Luis Busquets
 * Copyright 2008 Philip Nilsson
 * Copyright 2008 Henri Verbeet
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

#define COBJMACROS
#include "wine/test.h"
#include "d3dx9.h"
#include <math.h>
#include <stdint.h>

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

static BOOL compare_vec2(const D3DXVECTOR2 *v1, const D3DXVECTOR2 *v2, unsigned int ulps)
{
    return compare_float(v1->x, v2->x, ulps) && compare_float(v1->y, v2->y, ulps);
}

static BOOL compare_vec3(const D3DXVECTOR3 *v1, const D3DXVECTOR3 *v2, unsigned int ulps)
{
    return compare_float(v1->x, v2->x, ulps)
            && compare_float(v1->y, v2->y, ulps)
            && compare_float(v1->z, v2->z, ulps);
}

static BOOL compare_vec4(const D3DXVECTOR4 *v1, const D3DXVECTOR4 *v2, unsigned int ulps)
{
    return compare_float(v1->x, v2->x, ulps)
            && compare_float(v1->y, v2->y, ulps)
            && compare_float(v1->z, v2->z, ulps)
            && compare_float(v1->w, v2->w, ulps);
}

static BOOL compare_color(const D3DXCOLOR *c1, const D3DXCOLOR *c2, unsigned int ulps)
{
    return compare_float(c1->r, c2->r, ulps)
            && compare_float(c1->g, c2->g, ulps)
            && compare_float(c1->b, c2->b, ulps)
            && compare_float(c1->a, c2->a, ulps);
}

static BOOL compare_plane(const D3DXPLANE *p1, const D3DXPLANE *p2, unsigned int ulps)
{
    return compare_float(p1->a, p2->a, ulps)
            && compare_float(p1->b, p2->b, ulps)
            && compare_float(p1->c, p2->c, ulps)
            && compare_float(p1->d, p2->d, ulps);
}

static BOOL compare_quaternion(const D3DXQUATERNION *q1, const D3DXQUATERNION *q2, unsigned int ulps)
{
    return compare_float(q1->x, q2->x, ulps)
            && compare_float(q1->y, q2->y, ulps)
            && compare_float(q1->z, q2->z, ulps)
            && compare_float(q1->w, q2->w, ulps);
}

static BOOL compare_matrix(const D3DXMATRIX *m1, const D3DXMATRIX *m2, unsigned int ulps)
{
    unsigned int i, j;

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if (!compare_float(m1->m[i][j], m2->m[i][j], ulps))
                return FALSE;
        }
    }

    return TRUE;
}

#define expect_vec2(expected, vector, ulps) expect_vec2_(__LINE__, expected, vector, ulps)
static void expect_vec2_(unsigned int line, const D3DXVECTOR2 *expected, const D3DXVECTOR2 *vector, unsigned int ulps)
{
    BOOL equal = compare_vec2(expected, vector, ulps);
    ok_(__FILE__, line)(equal,
            "Got unexpected vector {%.8e, %.8e}, expected {%.8e, %.8e}.\n",
            vector->x, vector->y, expected->x, expected->y);
}

#define expect_vec3(expected, vector, ulps) expect_vec3_(__LINE__, expected, vector, ulps)
static void expect_vec3_(unsigned int line, const D3DXVECTOR3 *expected, const D3DXVECTOR3 *vector, unsigned int ulps)
{
    BOOL equal = compare_vec3(expected, vector, ulps);
    ok_(__FILE__, line)(equal,
            "Got unexpected vector {%.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e}.\n",
            vector->x, vector->y, vector->z, expected->x, expected->y, expected->z);
}

#define expect_vec4(expected, vector, ulps) expect_vec4_(__LINE__, expected, vector, ulps)
static void expect_vec4_(unsigned int line, const D3DXVECTOR4 *expected, const D3DXVECTOR4 *vector, unsigned int ulps)
{
    BOOL equal = compare_vec4(expected, vector, ulps);
    ok_(__FILE__, line)(equal,
            "Got unexpected vector {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
            vector->x, vector->y, vector->z, vector->w, expected->x, expected->y, expected->z, expected->w);
}

#define expect_color(expected, color, ulps) expect_color_(__LINE__, expected, color, ulps)
static void expect_color_(unsigned int line, const D3DXCOLOR *expected, const D3DXCOLOR *color, unsigned int ulps)
{
    BOOL equal = compare_color(expected, color, ulps);
    ok_(__FILE__, line)(equal,
            "Got unexpected color {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
            color->r, color->g, color->b, color->a, expected->r, expected->g, expected->b, expected->a);
}

#define expect_plane(expected, plane, ulps) expect_plane_(__LINE__, expected, plane, ulps)
static void expect_plane_(unsigned int line, const D3DXPLANE *expected, const D3DXPLANE *plane, unsigned int ulps)
{
    BOOL equal = compare_plane(expected, plane, ulps);
    ok_(__FILE__, line)(equal,
            "Got unexpected plane {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
            plane->a, plane->b, plane->c, plane->d, expected->a, expected->b, expected->c, expected->d);
}

#define expect_quaternion(expected, quaternion, ulps) expect_quaternion_(__LINE__, expected, quaternion, ulps)
static void expect_quaternion_(unsigned int line, const D3DXQUATERNION *expected,
        const D3DXQUATERNION *quaternion, unsigned int ulps)
{
    BOOL equal = compare_quaternion(expected, quaternion, ulps);
    ok_(__FILE__, line)(equal,
            "Got unexpected quaternion {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
            quaternion->x, quaternion->y, quaternion->z, quaternion->w,
            expected->x, expected->y, expected->z, expected->w);
}

#define expect_matrix(expected, matrix, ulps) expect_matrix_(__LINE__, expected, matrix, ulps)
static void expect_matrix_(unsigned int line, const D3DXMATRIX *expected, const D3DXMATRIX *matrix, unsigned int ulps)
{
    BOOL equal = compare_matrix(expected, matrix, ulps);
    ok_(__FILE__, line)(equal,
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, "
            "%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e}, "
            "expected {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, "
            "%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            matrix->m[0][0], matrix->m[0][1], matrix->m[0][2], matrix->m[0][3],
            matrix->m[1][0], matrix->m[1][1], matrix->m[1][2], matrix->m[1][3],
            matrix->m[2][0], matrix->m[2][1], matrix->m[2][2], matrix->m[2][3],
            matrix->m[3][0], matrix->m[3][1], matrix->m[3][2], matrix->m[3][3],
            expected->m[0][0], expected->m[0][1], expected->m[0][2], expected->m[0][3],
            expected->m[1][0], expected->m[1][1], expected->m[1][2], expected->m[1][3],
            expected->m[2][0], expected->m[2][1], expected->m[2][2], expected->m[2][3],
            expected->m[3][0], expected->m[3][1], expected->m[3][2], expected->m[3][3]);
}

#define expect_vec4_array(count, expected, vector, ulps) expect_vec4_array_(__LINE__, count, expected, vector, ulps)
static void expect_vec4_array_(unsigned int line, unsigned int count, const D3DXVECTOR4 *expected,
        const D3DXVECTOR4 *vector, unsigned int ulps)
{
    BOOL equal;
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        equal = compare_vec4(&expected[i], &vector[i], ulps);
        ok_(__FILE__, line)(equal,
                "Got unexpected vector {%.8e, %.8e, %.8e, %.8e} at index %u, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                vector[i].x, vector[i].y, vector[i].z, vector[i].w, i,
                expected[i].x, expected[i].y, expected[i].z, expected[i].w);
        if (!equal)
            break;
    }
}

static void set_matrix(D3DXMATRIX* mat,
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33)
{
    mat->m[0][0] = m00; mat->m[0][1] = m01; mat->m[0][2] = m02; mat->m[0][3] = m03;
    mat->m[1][0] = m10; mat->m[1][1] = m11; mat->m[1][2] = m12; mat->m[1][3] = m13;
    mat->m[2][0] = m20; mat->m[2][1] = m21; mat->m[2][2] = m22; mat->m[2][3] = m23;
    mat->m[3][0] = m30; mat->m[3][1] = m31; mat->m[3][2] = m32; mat->m[3][3] = m33;
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    unsigned int adapter_ordinal;
    IDirect3DDevice9 *device;
    DWORD behavior_flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    adapter_ordinal = D3DADAPTER_DEFAULT;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, adapter_ordinal, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, &device)))
        return device;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, adapter_ordinal, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, &device)))
        return device;

    behavior_flags = (behavior_flags
            & ~(D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING))
            | D3DCREATE_HARDWARE_VERTEXPROCESSING;

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, adapter_ordinal, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, &device)))
        return device;

    return NULL;
}

static void D3DXColorTest(void)
{
    D3DXCOLOR color, color1, color2, expected, got;
    LPD3DXCOLOR funcpointer;
    FLOAT scale;

    color.r = 0.2f; color.g = 0.75f; color.b = 0.41f; color.a = 0.93f;
    color1.r = 0.6f; color1.g = 0.55f; color1.b = 0.23f; color1.a = 0.82f;
    color2.r = 0.3f; color2.g = 0.5f; color2.b = 0.76f; color2.a = 0.11f;

    scale = 0.3f;

/*_______________D3DXColorAdd________________*/
    expected.r = 0.9f; expected.g = 1.05f; expected.b = 0.99f; expected.a = 0.93f;
    D3DXColorAdd(&got,&color1,&color2);
    expect_color(&expected, &got, 1);
    /* Test the NULL case */
    funcpointer = D3DXColorAdd(&got,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorAdd(NULL,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorAdd(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorAdjustContrast______*/
    expected.r = 0.41f; expected.g = 0.575f; expected.b = 0.473f; expected.a = 0.93f;
    D3DXColorAdjustContrast(&got,&color,scale);
    expect_color(&expected, &got, 0);

/*_______________D3DXColorAdjustSaturation______*/
    expected.r = 0.486028f; expected.g = 0.651028f; expected.b = 0.549028f; expected.a = 0.93f;
    D3DXColorAdjustSaturation(&got,&color,scale);
    expect_color(&expected, &got, 16);

/*_______________D3DXColorLerp________________*/
    expected.r = 0.32f; expected.g = 0.69f; expected.b = 0.356f; expected.a = 0.897f;
    D3DXColorLerp(&got,&color,&color1,scale);
    expect_color(&expected, &got, 0);
    /* Test the NULL case */
    funcpointer = D3DXColorLerp(&got,NULL,&color1,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorLerp(NULL,NULL,&color1,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorLerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorModulate________________*/
    expected.r = 0.18f; expected.g = 0.275f; expected.b = 0.1748f; expected.a = 0.0902f;
    D3DXColorModulate(&got,&color1,&color2);
    expect_color(&expected, &got, 0);
    /* Test the NULL case */
    funcpointer = D3DXColorModulate(&got,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorModulate(NULL,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorModulate(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorNegative________________*/
    expected.r = 0.8f; expected.g = 0.25f; expected.b = 0.59f; expected.a = 0.93f;
    D3DXColorNegative(&got,&color);
    expect_color(&expected, &got, 1);
    /* Test the greater than 1 case */
    color1.r = 0.2f; color1.g = 1.75f; color1.b = 0.41f; color1.a = 0.93f;
    expected.r = 0.8f; expected.g = -0.75f; expected.b = 0.59f; expected.a = 0.93f;
    D3DXColorNegative(&got,&color1);
    expect_color(&expected, &got, 1);
    /* Test the negative case */
    color1.r = 0.2f; color1.g = -0.75f; color1.b = 0.41f; color1.a = 0.93f;
    expected.r = 0.8f; expected.g = 1.75f; expected.b = 0.59f; expected.a = 0.93f;
    D3DXColorNegative(&got,&color1);
    expect_color(&expected, &got, 1);
    /* Test the NULL case */
    funcpointer = D3DXColorNegative(&got,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorNegative(NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorScale________________*/
    expected.r = 0.06f; expected.g = 0.225f; expected.b = 0.123f; expected.a = 0.279f;
    D3DXColorScale(&got,&color,scale);
    expect_color(&expected, &got, 1);
    /* Test the NULL case */
    funcpointer = D3DXColorScale(&got,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorScale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorSubtract_______________*/
    expected.r = -0.1f; expected.g = 0.25f; expected.b = -0.35f; expected.a = 0.82f;
    D3DXColorSubtract(&got,&color,&color2);
    expect_color(&expected, &got, 1);
    /* Test the NULL case */
    funcpointer = D3DXColorSubtract(&got,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorSubtract(NULL,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorSubtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
}

static void D3DXFresnelTest(void)
{
    float fresnel;
    BOOL equal;

    fresnel = D3DXFresnelTerm(0.5f, 1.5f);
    equal = compare_float(fresnel, 8.91867128e-02f, 1);
    ok(equal, "Got unexpected Fresnel term %.8e.\n", fresnel);
}

static void D3DXMatrixTest(void)
{
    D3DXMATRIX expectedmat, gotmat, mat, mat2, mat3;
    BOOL expected, got, equal;
    float angle, determinant;
    D3DXPLANE plane;
    D3DXQUATERNION q, r;
    D3DXVECTOR3 at, axis, eye, last;
    D3DXVECTOR4 light;
    D3DXMATRIX *ret;

    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = 10.0f; mat.m[1][1] = 20.0f; mat.m[2][2] = 30.0f;
    mat.m[3][3] = -40.0f;

    mat2.m[0][0] = 1.0f; mat2.m[1][0] = 2.0f; mat2.m[2][0] = 3.0f;
    mat2.m[3][0] = 4.0f; mat2.m[0][1] = 5.0f; mat2.m[1][1] = 6.0f;
    mat2.m[2][1] = 7.0f; mat2.m[3][1] = 8.0f; mat2.m[0][2] = -8.0f;
    mat2.m[1][2] = -7.0f; mat2.m[2][2] = -6.0f; mat2.m[3][2] = -5.0f;
    mat2.m[0][3] = -4.0f; mat2.m[1][3] = -3.0f; mat2.m[2][3] = -2.0f;
    mat2.m[3][3] = -1.0f;

    plane.a = -3.0f; plane.b = -1.0f; plane.c = 4.0f; plane.d = 7.0f;

    q.x = 1.0f; q.y = -4.0f; q.z =7.0f; q.w = -11.0f;
    r.x = 0.87f; r.y = 0.65f; r.z =0.43f; r.w= 0.21f;

    at.x = -2.0f; at.y = 13.0f; at.z = -9.0f;
    axis.x = 1.0f; axis.y = -3.0f; axis.z = 7.0f;
    eye.x = 8.0f; eye.y = -5.0f; eye.z = 5.75f;
    last.x = 9.7f; last.y = -8.6; last.z = 1.3f;

    light.x = 9.6f; light.y = 8.5f; light.z = 7.4; light.w = 6.3;

    angle = D3DX_PI/3.0f;

/*____________D3DXMatrixAffineTransformation______*/
    set_matrix(&expectedmat,
            -459.239990f, -576.719971f, -263.440002f, 0.0f,
            519.760010f, -352.440002f, -277.679993f, 0.0f,
            363.119995f, -121.040001f, -117.479996f, 0.0f,
            -1239.0f, 667.0f, 567.0f, 1.0f);
    D3DXMatrixAffineTransformation(&gotmat, 3.56f, &at, &q, &axis);
    expect_matrix(&expectedmat, &gotmat, 0);

    /* Test the NULL case */
    expectedmat.m[3][0] = 1.0f; expectedmat.m[3][1] = -3.0f; expectedmat.m[3][2] = 7.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixAffineTransformation(&gotmat, 3.56f, NULL, &q, &axis);
    expect_matrix(&expectedmat, &gotmat, 0);

    expectedmat.m[3][0] = -1240.0f; expectedmat.m[3][1] = 670.0f; expectedmat.m[3][2] = 560.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixAffineTransformation(&gotmat, 3.56f, &at, &q, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixAffineTransformation(&gotmat, 3.56f, NULL, &q, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            3.56f, 0.0f, 0.0f, 0.0f,
            0.0f, 3.56f, 0.0f, 0.0f,
            0.0f, 0.0f, 3.56f, 0.0f,
            1.0f, -3.0f, 7.0f, 1.0f);
    D3DXMatrixAffineTransformation(&gotmat, 3.56f, NULL, NULL, &axis);
    expect_matrix(&expectedmat, &gotmat, 0);

    D3DXMatrixAffineTransformation(&gotmat, 3.56f, &at, NULL, &axis);
    expect_matrix(&expectedmat, &gotmat, 0);

    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixAffineTransformation(&gotmat, 3.56f, &at, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    D3DXMatrixAffineTransformation(&gotmat, 3.56f, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

/*____________D3DXMatrixfDeterminant_____________*/
    determinant = D3DXMatrixDeterminant(&mat);
    equal = compare_float(determinant, -147888.0f, 0);
    ok(equal, "Got unexpected determinant %.8e.\n", determinant);

/*____________D3DXMatrixInverse______________*/
    set_matrix(&expectedmat,
            16067.0f/73944.0f, -10165.0f/147888.0f, -2729.0f/147888.0f, -1631.0f/49296.0f,
            -565.0f/36972.0f, 2723.0f/73944.0f, -1073.0f/73944.0f, 289.0f/24648.0f,
            -389.0f/2054.0f, 337.0f/4108.0f, 181.0f/4108.0f, 317.0f/4108.0f,
            163.0f/5688.0f, -101.0f/11376.0f, -73.0f/11376.0f, -127.0f/3792.0f);
    D3DXMatrixInverse(&gotmat, &determinant, &mat);
    expect_matrix(&expectedmat, &gotmat, 1);
    equal = compare_float(determinant, -147888.0f, 0);
    ok(equal, "Got unexpected determinant %.8e.\n", determinant);
    determinant = 5.0f;
    ret = D3DXMatrixInverse(&gotmat, &determinant, &mat2);
    ok(!ret, "Unexpected return value %p.\n", ret);
    expect_matrix(&expectedmat, &gotmat, 1);
    ok(compare_float(determinant, 5.0f, 0) || broken(!determinant), /* Vista 64 bit testbot */
            "Unexpected determinant %.8e.\n", determinant);

/*____________D3DXMatrixIsIdentity______________*/
    expected = FALSE;
    memset(&mat3, 0, sizeof(mat3));
    got = D3DXMatrixIsIdentity(&mat3);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);
    D3DXMatrixIdentity(&mat3);
    expected = TRUE;
    got = D3DXMatrixIsIdentity(&mat3);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);
    mat3.m[0][0] = 0.000009f;
    expected = FALSE;
    got = D3DXMatrixIsIdentity(&mat3);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);
    /* Test the NULL case */
    expected = FALSE;
    got = D3DXMatrixIsIdentity(NULL);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);

/*____________D3DXMatrixLookAtLH_______________*/
    set_matrix(&expectedmat,
            -0.82246518f, -0.40948939f, -0.39480308f, 0.0f,
            -0.55585691f, 0.43128574f, 0.71064550f, 0.0f,
            -0.12072885f, 0.80393475f, -0.58233452f, 0.0f,
            4.4946337f, 0.80971903f, 10.060076f, 1.0f);
    D3DXMatrixLookAtLH(&gotmat, &eye, &at, &axis);
    expect_matrix(&expectedmat, &gotmat, 32);

/*____________D3DXMatrixLookAtRH_______________*/
    set_matrix(&expectedmat,
            0.82246518f, -0.40948939f, 0.39480308f, 0.0f,
            0.55585691f, 0.43128574f, -0.71064550f, 0.0f,
            0.12072885f, 0.80393475f, 0.58233452f, 0.0f,
            -4.4946337f, 0.80971903f, -10.060076f, 1.0f);
    D3DXMatrixLookAtRH(&gotmat, &eye, &at, &axis);
    expect_matrix(&expectedmat, &gotmat, 32);

/*____________D3DXMatrixMultiply______________*/
    set_matrix(&expectedmat,
            73.0f, 193.0f, -197.0f, -77.0f,
            231.0f, 551.0f, -489.0f, -169.0f,
            239.0f, 523.0f, -400.0f, -116.0f,
            -164.0f, -320.0f, 187.0f, 31.0f);
    D3DXMatrixMultiply(&gotmat, &mat, &mat2);
    expect_matrix(&expectedmat, &gotmat, 0);

/*____________D3DXMatrixMultiplyTranspose____*/
    set_matrix(&expectedmat,
            73.0f, 231.0f, 239.0f, -164.0f,
            193.0f, 551.0f, 523.0f, -320.0f,
            -197.0f, -489.0f, -400.0f, 187.0f,
            -77.0f, -169.0f, -116.0f, 31.0f);
    D3DXMatrixMultiplyTranspose(&gotmat, &mat, &mat2);
    expect_matrix(&expectedmat, &gotmat, 0);

/*____________D3DXMatrixOrthoLH_______________*/
    set_matrix(&expectedmat,
            0.8f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.27027027f, 0.0f, 0.0f,
            0.0f, 0.0f, -0.15151515f, 0.0f,
            0.0f, 0.0f, -0.48484848f, 1.0f);
    D3DXMatrixOrthoLH(&gotmat, 2.5f, 7.4f, -3.2f, -9.8f);
    expect_matrix(&expectedmat, &gotmat, 16);

/*____________D3DXMatrixOrthoOffCenterLH_______________*/
    set_matrix(&expectedmat,
            3.6363636f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.18018018f, 0.0f, 0.0f,
            0.0f, 0.0f, -0.045662100f, 0.0f,
            -1.7272727f, -0.56756757f, 0.42465753f, 1.0f);
    D3DXMatrixOrthoOffCenterLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 9.3, -12.6);
    expect_matrix(&expectedmat, &gotmat, 32);

/*____________D3DXMatrixOrthoOffCenterRH_______________*/
    set_matrix(&expectedmat,
            3.6363636f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.18018018f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.045662100f, 0.0f,
            -1.7272727f, -0.56756757f, 0.42465753f, 1.0f);
    D3DXMatrixOrthoOffCenterRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 9.3, -12.6);
    expect_matrix(&expectedmat, &gotmat, 32);

/*____________D3DXMatrixOrthoRH_______________*/
    set_matrix(&expectedmat,
            0.8f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.27027027f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.15151515f, 0.0f,
            0.0f, 0.0f, -0.48484848f, 1.0f);
    D3DXMatrixOrthoRH(&gotmat, 2.5f, 7.4f, -3.2f, -9.8f);
    expect_matrix(&expectedmat, &gotmat, 16);

/*____________D3DXMatrixPerspectiveFovLH_______________*/
    set_matrix(&expectedmat,
            13.288858f, 0.0f, 0.0f, 0.0f,
            0.0f, 9.9666444f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.78378378f, 1.0f,
            0.0f, 0.0f, 1.8810811f, 0.0f);
    D3DXMatrixPerspectiveFovLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_matrix(&expectedmat, &gotmat, 4);

/*____________D3DXMatrixPerspectiveFovRH_______________*/
    set_matrix(&expectedmat,
            13.288858f, 0.0f, 0.0f, 0.0f,
            0.0f, 9.9666444f, 0.0f, 0.0f,
            0.0f, 0.0f, -0.78378378f, -1.0f,
            0.0f, 0.0f, 1.8810811f, 0.0f);
    D3DXMatrixPerspectiveFovRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_matrix(&expectedmat, &gotmat, 4);

/*____________D3DXMatrixPerspectiveLH_______________*/
    set_matrix(&expectedmat,
            -24.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -6.4f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.78378378f, 1.0f,
            0.0f, 0.0f, 1.8810811f, 0.0f);
    D3DXMatrixPerspectiveLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_matrix(&expectedmat, &gotmat, 4);

/*____________D3DXMatrixPerspectiveOffCenterLH_______________*/
    set_matrix(&expectedmat,
            11.636364f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.57657658f, 0.0f, 0.0f,
            -1.7272727f, -0.56756757f, 0.84079602f, 1.0f,
            0.0f, 0.0f, -2.6905473f, 0.0f);
    D3DXMatrixPerspectiveOffCenterLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 3.2f, -16.9f);
    expect_matrix(&expectedmat, &gotmat, 8);

/*____________D3DXMatrixPerspectiveOffCenterRH_______________*/
    set_matrix(&expectedmat,
            11.636364f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.57657658f, 0.0f, 0.0f,
            1.7272727f, 0.56756757f, -0.84079602f, -1.0f,
            0.0f, 0.0f, -2.6905473f, 0.0f);
    D3DXMatrixPerspectiveOffCenterRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 3.2f, -16.9f);
    expect_matrix(&expectedmat, &gotmat, 8);

/*____________D3DXMatrixPerspectiveRH_______________*/
    set_matrix(&expectedmat,
            -24.0f, -0.0f, 0.0f, 0.0f,
            0.0f, -6.4f, 0.0f, 0.0f,
            0.0f, 0.0f, -0.78378378f, -1.0f,
            0.0f, 0.0f, 1.8810811f, 0.0f);
    D3DXMatrixPerspectiveRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_matrix(&expectedmat, &gotmat, 4);

/*____________D3DXMatrixReflect______________*/
    set_matrix(&expectedmat,
            0.30769235f, -0.23076922f, 0.92307687f, 0.0f,
            -0.23076922, 0.92307693f, 0.30769232f, 0.0f,
            0.92307687f, 0.30769232f, -0.23076922f, 0.0f,
            1.6153846f, 0.53846157f, -2.1538463f, 1.0f);
    D3DXMatrixReflect(&gotmat, &plane);
    expect_matrix(&expectedmat, &gotmat, 32);

/*____________D3DXMatrixRotationAxis_____*/
    set_matrix(&expectedmat,
            0.50847453f, 0.76380461f, 0.39756277f, 0.0f,
            -0.81465209f, 0.57627118f, -0.065219201f, 0.0f,
            -0.27891868f, -0.29071301f, 0.91525424f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixRotationAxis(&gotmat, &axis, angle);
    expect_matrix(&expectedmat, &gotmat, 32);

/*____________D3DXMatrixRotationQuaternion______________*/
    set_matrix(&expectedmat,
            -129.0f, -162.0f, -74.0f, 0.0f,
            146.0f, -99.0f, -78.0f, 0.0f,
            102.0f, -34.0f, -33.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixRotationQuaternion(&gotmat, &q);
    expect_matrix(&expectedmat, &gotmat, 0);

/*____________D3DXMatrixRotationX______________*/
    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, sqrt(3.0f)/2.0f, 0.0f,
            0.0f, -sqrt(3.0f)/2.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixRotationX(&gotmat, angle);
    expect_matrix(&expectedmat, &gotmat, 1);

/*____________D3DXMatrixRotationY______________*/
    set_matrix(&expectedmat,
            0.5f, 0.0f, -sqrt(3.0f)/2.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            sqrt(3.0f)/2.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixRotationY(&gotmat, angle);
    expect_matrix(&expectedmat, &gotmat, 1);

/*____________D3DXMatrixRotationYawPitchRoll____*/
    set_matrix(&expectedmat,
            0.88877726f, 0.091874748f, -0.44903678f, 0.0f,
            0.35171318f, 0.49148652f, 0.79670501f, 0.0f,
            0.29389259f, -0.86602545f, 0.40450847f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixRotationYawPitchRoll(&gotmat, 3.0f * angle / 5.0f, angle, 3.0f * angle / 17.0f);
    expect_matrix(&expectedmat, &gotmat, 64);

/*____________D3DXMatrixRotationZ______________*/
    set_matrix(&expectedmat,
            0.5f, sqrt(3.0f)/2.0f, 0.0f, 0.0f,
            -sqrt(3.0f)/2.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixRotationZ(&gotmat, angle);
    expect_matrix(&expectedmat, &gotmat, 1);

/*____________D3DXMatrixScaling______________*/
    set_matrix(&expectedmat,
            0.69f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.53f, 0.0f, 0.0f,
            0.0f, 0.0f, 4.11f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixScaling(&gotmat, 0.69f, 0.53f, 4.11f);
    expect_matrix(&expectedmat, &gotmat, 0);

/*____________D3DXMatrixShadow______________*/
    set_matrix(&expectedmat,
            12.786773f, 5.0009613f, 4.3537784f, 3.7065949f,
            1.8827150f, 8.8056154f, 1.4512594f, 1.2355317f,
            -7.5308599f, -6.6679487f, 1.3335901f, -4.9421268f,
            -13.179006f, -11.668910f, -10.158816f, -1.5100943f);
    D3DXMatrixShadow(&gotmat, &light, &plane);
    expect_matrix(&expectedmat, &gotmat, 8);

/*____________D3DXMatrixTransformation______________*/
    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            8.5985f, -21.024f, 14.383499, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            0.0f, 52.0f, 54.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            18.2985f, -29.624001f, 15.683499f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, NULL, &eye, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            9.7f, 43.400002f, 55.299999f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            8.5985f, -21.024f, 14.383499, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            8.5985f, -21.024f, 14.383499, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            8.5985f, -21.024f, 14.383499, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            53094.015625f, 2044.133789f, 21711.687500f, 0.0f,
            -7294.705078f, 47440.683594f, 28077.113281, 0.0f,
            -12749.161133f, 28365.580078f, 13503.520508f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 32);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            104.565598f, -35.492798f, -25.306400f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            0.0f, 52.0f, 54.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            -287420.0f, -14064.0f, 37122.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            18.2985f, -29.624001f, 15.683499f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, NULL, &axis, &eye, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            18.2985f, -29.624001f, 15.683499f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, NULL, &eye, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            18.2985f, -29.624001f, 15.683499f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, NULL, &eye, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            53094.015625f, 2044.133789f, 21711.687500f, 0.0f,
            -7294.705078f, 47440.683594f, 28077.113281, 0.0f,
            -12749.161133f, 28365.580078f, 13503.520508f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 32);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            114.265594f, -44.092796f, -24.006401f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -3.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 7.0f, 0.0f,
            9.7f, 43.400002f, 55.299999f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            9.7f, -8.6f, 1.3f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            -287410.3125f, -14072.599609f, 37123.300781f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, &axis, NULL, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            53094.015625f, 2044.133789f, 21711.687500f, 0.0f,
            -7294.705078f, 47440.683594f, 28077.113281, 0.0f,
            -12749.161133f, 28365.580078f, 13503.520508f, 0.0f,
            8.598499f, -21.024f, 14.383499f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 32);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            113.164093f, -56.5168f, -10.922897f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            0.9504f, -0.8836f, 0.9244f, 0.0f,
            1.0212f, 0.1936f, -1.3588f, 0.0f,
            8.5985f, -21.024f, 14.383499, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            53094.015625f, 2044.133789f, 21711.687500f, 0.0f,
            -7294.705078f, 47440.683594f, 28077.113281, 0.0f,
            -12749.161133f, 28365.580078f, 13503.520508f, 0.0f,
            86280.34375f, -357366.3125f, -200024.125f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, &axis, NULL, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 32);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            -287410.3125f, -14064.0f, 37122.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, &axis, &eye, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 512);

    set_matrix(&expectedmat,
            53094.015625f, 2044.133789f, 21711.687500f, 0.0f,
            -7294.705078f, 47440.683594f, 28077.113281, 0.0f,
            -12749.161133f, 28365.580078f, 13503.520508f, 0.0f,
            86280.34375f, -357366.3125f, -200009.75f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, &axis, &eye, &r, NULL);
    expect_matrix(&expectedmat, &gotmat, 2048);

    set_matrix(&expectedmat,
            25521.0f, 39984.0f, 20148.0f, 0.0f,
            39984.0f, 4933.0f, -3324.0f, 0.0f,
            20148.0f, -3324.0f, -5153.0f, 0.0f,
            -287410.3125f, -14072.599609f, 37123.300781f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, &axis, &eye, NULL, &last);
    expect_matrix(&expectedmat, &gotmat, 0);

    set_matrix(&expectedmat,
            53094.015625f, 2044.133789f, 21711.687500f, 0.0f,
            -7294.705078f, 47440.683594f, 28077.113281, 0.0f,
            -12749.161133f, 28365.580078f, 13503.520508f, 0.0f,
            86290.046875f, -357374.90625f, -200022.828125f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, &axis, NULL, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 32);

    set_matrix(&expectedmat,
            -0.21480007f, 1.3116000f, 0.47520003f, 0.0f,
            0.95040143f, -0.88360137f, 0.92439979f, 0.0f,
            1.0212044f, 0.19359307f, -1.3588026f, 0.0f,
            18.298532f, -29.624001f, 15.683499f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, &q, NULL, &eye, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 512);

    set_matrix(&expectedmat,
            -0.2148f, 1.3116f, 0.4752f, 0.0f,
            -2.8512f, 2.6508f, -2.7732f, 0.0f,
            7.148399f, 1.3552f, -9.5116f, 0.0f,
            122.86409f, -65.116798f, -9.622897f, 1.0f);
    D3DXMatrixTransformation(&gotmat, &at, NULL, &axis, &eye, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 8);

    set_matrix(&expectedmat,
            53094.015625f, 2044.133789f, 21711.687500f, 0.0f,
            -7294.705078f, 47440.683594f, 28077.113281, 0.0f,
            -12749.161133f, 28365.580078f, 13503.520508f, 0.0f,
            18.2985f, -29.624001f, 15.683499f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, &eye, &r, &last);
    expect_matrix(&expectedmat, &gotmat, 32);

    q.x = 1.0f; q.y = 1.0f; q.z = 1.0f; q.w = 1.0f;
    axis.x = 1.0f; axis.y = 1.0f; axis.z = 2.0f;

    set_matrix(&expectedmat,
            41.0f, -12.0f, -24.0f, 0.0f,
            -12.0f, 25.0f, -12.0f, 0.0f,
            -24.0f, -12.0f, 34.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 1.0f; q.z = 1.0f; q.w = 1.0f;
    axis.x = 1.0f; axis.y = 1.0f; axis.z = 3.0f;

    set_matrix(&expectedmat,
            57.0f, -12.0f, -36.0f, 0.0f,
            -12.0f, 25.0f, -12.0f, 0.0f,
            -36.0f, -12.0f, 43.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 1.0f; q.z = 1.0f; q.w = 0.0f;
    axis.x = 1.0f; axis.y = 1.0f; axis.z = 3.0f;

    set_matrix(&expectedmat,
            25.0f, 0.0f, -20.0f, 0.0f,
            0.0f, 25.0f, -20.0f, 0.0f,
            -20.0f, -20.0f, 35.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 1.0f; q.z = 0.0f; q.w = 0.0f;
    axis.x = 1.0f; axis.y = 1.0f; axis.z = 3.0f;

    set_matrix(&expectedmat,
            5.0f, -4.0f, 0.0f, 0.0f,
            -4.0f, 5.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 27.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 0.0f; q.z = 0.0f; q.w = 0.0f;
    axis.x = 5.0f; axis.y = 2.0f; axis.z = 1.0f;

    set_matrix(&expectedmat,
            5.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 0.0f; q.z = 0.0f; q.w = 0.0f;
    axis.x = 1.0f; axis.y = 4.0f; axis.z = 1.0f;

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 4.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 0.0f; q.y = 1.0f; q.z = 0.0f; q.w = 0.0f;
    axis.x = 1.0f; axis.y = 4.0f; axis.z = 1.0f;

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 4.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 0.0f; q.z = 0.0f; q.w = 1.0f;
    axis.x = 1.0f; axis.y = 4.0f; axis.z = 1.0f;

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 8.0f, -6.0f, 0.0f,
            0.0f, -6.0f, 17.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 0.0f; q.z = 0.0f; q.w = 1.0f;
    axis.x = 0.0f; axis.y = 4.0f; axis.z = 0.0f;

    set_matrix(&expectedmat,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 4.0f, -8.0f, 0.0f,
            0.0f, -8.0f, 16.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 0.0f; q.y = 1.0f; q.z = 0.0f; q.w = 1.0f;
    axis.x = 1.0f; axis.y = 4.0f; axis.z = 1.0f;

    set_matrix(&expectedmat,
            5.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 4.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 5.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 1.0f; q.y = 0.0f; q.z = 0.0f; q.w = 0.0f;
    axis.x = 1.0f; axis.y = 1.0f; axis.z = 3.0f;

    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 3.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 3.0f; axis.y = 3.0f; axis.z = 3.0f;

    set_matrix(&expectedmat,
            3796587.0f, -1377948.0f, -1589940.0f, 0.0f,
            -1377948.0f, 3334059.0f, -1879020.0f, 0.0f,
            -1589940.0f, -1879020.0f, 2794443.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 1.0f; axis.y = 1.0f; axis.z = 1.0f;

    set_matrix(&expectedmat,
            1265529.0f, -459316.0f, -529980.0f, 0.0f,
            -459316.0f, 1111353.0f, -626340.0f, 0.0f,
            -529980.0f, -626340.0f, 931481.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 1.0f; axis.y = 1.0f; axis.z = 3.0f;

    set_matrix(&expectedmat,
            2457497.0f, -434612.0f, -1423956.0f, 0.0f,
            -434612.0f, 1111865.0f, -644868.0f, 0.0f,
            -1423956.0f, -644868.0f, 1601963.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 0.0f; axis.y = 0.0f; axis.z = 3.0f;

    set_matrix(&expectedmat,
            1787952.0f, 37056.0f, -1340964.0f, 0.0f,
            37056.0f, 768.0f, -27792.0f, 0.0f,
            -1340964.0f, -27792.0f, 1005723.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 0.0f; axis.y = 0.0f; axis.z = 1.0f;

    set_matrix(&expectedmat,
            595984.0f, 12352.0f, -446988.0f, 0.0f,
            12352.0f, 256.0f, -9264.0f, 0.0f,
            -446988.0f, -9264.0f, 335241.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 0.0f; axis.y = 3.0f; axis.z = 0.0f;

    set_matrix(&expectedmat,
            150528.0f, 464352.0f, -513408.0f, 0.0f,
            464352.0f, 1432443.0f, -1583772.0f, 0.0f,
            -513408.0f, -1583772.0f, 1751088.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 0.0f; axis.y = 1.0f; axis.z = 0.0f;

    set_matrix(&expectedmat,
            50176.0f, 154784.0f, -171136.0f, 0.0f,
            154784.0f, 477481.0f, -527924.0f, 0.0f,
            -171136.0f, -527924.0f, 583696.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

    q.x = 11.0f; q.y = 13.0f; q.z = 15.0f; q.w = 17.0f;
    axis.x = 1.0f; axis.y = 0.0f; axis.z = 0.0f;

    set_matrix(&expectedmat,
            619369.0f, -626452.0f, 88144.0f, 0.0f,
            -626452.0f, 633616.0f, -89152.0f, 0.0f,
            88144.0f, -89152, 12544.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    D3DXMatrixTransformation(&gotmat, NULL, &q, &axis, NULL, NULL, NULL);
    expect_matrix(&expectedmat, &gotmat, 0);

/*____________D3DXMatrixTranslation______________*/
    set_matrix(&expectedmat,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.69f, 0.53f, 4.11f, 1.0f);
    D3DXMatrixTranslation(&gotmat, 0.69f, 0.53f, 4.11f);
    expect_matrix(&expectedmat, &gotmat, 0);

/*____________D3DXMatrixTranspose______________*/
    set_matrix(&expectedmat,
            10.0f, 11.0f, 19.0f, 2.0f,
            5.0f, 20.0f, -21.0f, 3.0f,
            7.0f, 16.0f, 30.f, -4.0f,
            8.0f, 33.0f, 43.0f, -40.0f);
    D3DXMatrixTranspose(&gotmat, &mat);
    expect_matrix(&expectedmat, &gotmat, 0);
}

static void D3DXPlaneTest(void)
{
    D3DXMATRIX mat;
    D3DXPLANE expectedplane, gotplane, nulplane, plane;
    D3DXVECTOR3 expectedvec, gotvec, vec1, vec2, vec3;
    LPD3DXVECTOR3 funcpointer;
    D3DXVECTOR4 vec;
    FLOAT expected, got;

    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = 10.0f; mat.m[1][1] = 20.0f; mat.m[2][2] = 30.0f;
    mat.m[3][3] = -40.0f;

    plane.a = -3.0f; plane.b = -1.0f; plane.c = 4.0f; plane.d = 7.0f;

    vec.x = 2.0f; vec.y = 5.0f; vec.z = -6.0f; vec.w = 11.0f;

/*_______________D3DXPlaneDot________________*/
    expected = 42.0f;
    got = D3DXPlaneDot(&plane, &vec);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDot(NULL, &vec);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDot(NULL, NULL);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);

/*_______________D3DXPlaneDotCoord________________*/
    expected = -28.0f;
    got = D3DXPlaneDotCoord(&plane, &vec);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotCoord(NULL, &vec);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotCoord(NULL, NULL);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);

/*_______________D3DXPlaneDotNormal______________*/
    expected = -35.0f;
    got = D3DXPlaneDotNormal(&plane, &vec);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotNormal(NULL, &vec);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotNormal(NULL, NULL);
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);

/*_______________D3DXPlaneFromPointNormal_______*/
    vec1.x = 11.0f; vec1.y = 13.0f; vec1.z = 15.0f;
    vec2.x = 17.0f; vec2.y = 31.0f; vec2.z = 24.0f;
    expectedplane.a = 17.0f; expectedplane.b = 31.0f; expectedplane.c = 24.0f; expectedplane.d = -950.0f;
    D3DXPlaneFromPointNormal(&gotplane, &vec1, &vec2);
    expect_plane(&expectedplane, &gotplane, 0);
    gotplane.a = vec2.x; gotplane.b = vec2.y; gotplane.c = vec2.z;
    D3DXPlaneFromPointNormal(&gotplane, &vec1, (D3DXVECTOR3 *)&gotplane);
    expect_plane(&expectedplane, &gotplane, 0);
    gotplane.a = vec1.x; gotplane.b = vec1.y; gotplane.c = vec1.z;
    expectedplane.d = -1826.0f;
    D3DXPlaneFromPointNormal(&gotplane, (D3DXVECTOR3 *)&gotplane, &vec2);
    expect_plane(&expectedplane, &gotplane, 0);

/*_______________D3DXPlaneFromPoints_______*/
    vec1.x = 1.0f; vec1.y = 2.0f; vec1.z = 3.0f;
    vec2.x = 1.0f; vec2.y = -6.0f; vec2.z = -5.0f;
    vec3.x = 83.0f; vec3.y = 74.0f; vec3.z = 65.0f;
    expectedplane.a = 0.085914f; expectedplane.b = -0.704492f; expectedplane.c = 0.704492f; expectedplane.d = -0.790406f;
    D3DXPlaneFromPoints(&gotplane,&vec1,&vec2,&vec3);
    expect_plane(&expectedplane, &gotplane, 64);

/*_______________D3DXPlaneIntersectLine___________*/
    vec1.x = 9.0f; vec1.y = 6.0f; vec1.z = 3.0f;
    vec2.x = 2.0f; vec2.y = 5.0f; vec2.z = 8.0f;
    expectedvec.x = 20.0f/3.0f; expectedvec.y = 17.0f/3.0f; expectedvec.z = 14.0f/3.0f;
    D3DXPlaneIntersectLine(&gotvec,&plane,&vec1,&vec2);
    expect_vec3(&expectedvec, &gotvec, 1);
    /* Test a parallel line */
    vec1.x = 11.0f; vec1.y = 13.0f; vec1.z = 15.0f;
    vec2.x = 17.0f; vec2.y = 31.0f; vec2.z = 24.0f;
    expectedvec.x = 20.0f/3.0f; expectedvec.y = 17.0f/3.0f; expectedvec.z = 14.0f/3.0f;
    funcpointer = D3DXPlaneIntersectLine(&gotvec,&plane,&vec1,&vec2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXPlaneNormalize______________*/
    expectedplane.a = -3.0f/sqrt(26.0f); expectedplane.b = -1.0f/sqrt(26.0f); expectedplane.c = 4.0f/sqrt(26.0f); expectedplane.d = 7.0/sqrt(26.0f);
    D3DXPlaneNormalize(&gotplane, &plane);
    expect_plane(&expectedplane, &gotplane, 2);
    nulplane.a = 0.0; nulplane.b = 0.0f; nulplane.c = 0.0f; nulplane.d = 0.0f;
    expectedplane.a = 0.0f; expectedplane.b = 0.0f; expectedplane.c = 0.0f; expectedplane.d = 0.0f;
    D3DXPlaneNormalize(&gotplane, &nulplane);
    expect_plane(&expectedplane, &gotplane, 0);

/*_______________D3DXPlaneTransform____________*/
    expectedplane.a = 49.0f; expectedplane.b = -98.0f; expectedplane.c = 55.0f; expectedplane.d = -165.0f;
    D3DXPlaneTransform(&gotplane,&plane,&mat);
    expect_plane(&expectedplane, &gotplane, 0);
}

static void D3DXQuaternionTest(void)
{
    D3DXMATRIX mat;
    D3DXQUATERNION expectedquat, gotquat, Nq, Nq1, nul, smallq, smallr, q, r, s, t, u;
    BOOL expectedbool, gotbool, equal;
    float angle, got, scale, scale2;
    LPD3DXQUATERNION funcpointer;
    D3DXVECTOR3 axis, expectedvec;

    nul.x = 0.0f; nul.y = 0.0f; nul.z = 0.0f; nul.w = 0.0f;
    q.x = 1.0f; q.y = 2.0f; q.z = 4.0f; q.w = 10.0f;
    r.x = -3.0f; r.y = 4.0f; r.z = -5.0f; r.w = 7.0;
    t.x = -1111.0f; t.y = 111.0f; t.z = -11.0f; t.w = 1.0f;
    u.x = 91.0f; u.y = - 82.0f; u.z = 7.3f; u.w = -6.4f;
    smallq.x = 0.1f; smallq.y = 0.2f; smallq.z= 0.3f; smallq.w = 0.4f;
    smallr.x = 0.5f; smallr.y = 0.6f; smallr.z= 0.7f; smallr.w = 0.8f;

    scale = 0.3f;
    scale2 = 0.78f;

/*_______________D3DXQuaternionBaryCentric________________________*/
    expectedquat.x = -867.444458; expectedquat.y = 87.851111f; expectedquat.z = -9.937778f; expectedquat.w = 3.235555f;
    D3DXQuaternionBaryCentric(&gotquat,&q,&r,&t,scale,scale2);
    expect_quaternion(&expectedquat, &gotquat, 1);

/*_______________D3DXQuaternionConjugate________________*/
    expectedquat.x = -1.0f; expectedquat.y = -2.0f; expectedquat.z = -4.0f; expectedquat.w = 10.0f;
    D3DXQuaternionConjugate(&gotquat,&q);
    expect_quaternion(&expectedquat, &gotquat, 0);
    /* Test the NULL case */
    funcpointer = D3DXQuaternionConjugate(&gotquat,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXQuaternionConjugate(NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXQuaternionDot______________________*/
    got = D3DXQuaternionDot(&q,&r);
    equal = compare_float(got, 55.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    /* Tests the case NULL */
    got = D3DXQuaternionDot(NULL,&r);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    got = D3DXQuaternionDot(NULL,NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);

/*_______________D3DXQuaternionExp______________________________*/
    expectedquat.x = -0.216382f; expectedquat.y = -0.432764f; expectedquat.z = -0.8655270f; expectedquat.w = -0.129449f;
    D3DXQuaternionExp(&gotquat,&q);
    expect_quaternion(&expectedquat, &gotquat, 16);
    /* Test the null quaternion */
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 1.0f;
    D3DXQuaternionExp(&gotquat,&nul);
    expect_quaternion(&expectedquat, &gotquat, 0);
    /* Test the case where the norm of the quaternion is <1 */
    Nq1.x = 0.2f; Nq1.y = 0.1f; Nq1.z = 0.3; Nq1.w= 0.9f;
    expectedquat.x = 0.195366; expectedquat.y = 0.097683f; expectedquat.z = 0.293049f; expectedquat.w = 0.930813f;
    D3DXQuaternionExp(&gotquat,&Nq1);
    expect_quaternion(&expectedquat, &gotquat, 8);

/*_______________D3DXQuaternionIdentity________________*/
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 1.0f;
    D3DXQuaternionIdentity(&gotquat);
    expect_quaternion(&expectedquat, &gotquat, 0);
    /* Test the NULL case */
    funcpointer = D3DXQuaternionIdentity(NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXQuaternionInverse________________________*/
    expectedquat.x = -1.0f/121.0f; expectedquat.y = -2.0f/121.0f; expectedquat.z = -4.0f/121.0f; expectedquat.w = 10.0f/121.0f;
    D3DXQuaternionInverse(&gotquat,&q);
    expect_quaternion(&expectedquat, &gotquat, 0);

    expectedquat.x = 1.0f; expectedquat.y = 2.0f; expectedquat.z = 4.0f; expectedquat.w = 10.0f;
    D3DXQuaternionInverse(&gotquat,&gotquat);
    expect_quaternion(&expectedquat, &gotquat, 1);


/*_______________D3DXQuaternionIsIdentity________________*/
    s.x = 0.0f; s.y = 0.0f; s.z = 0.0f; s.w = 1.0f;
    expectedbool = TRUE;
    gotbool = D3DXQuaternionIsIdentity(&s);
    ok( expectedbool == gotbool, "Expected boolean : %d, Got bool : %d\n", expectedbool, gotbool);
    s.x = 2.3f; s.y = -4.2f; s.z = 1.2f; s.w=0.2f;
    expectedbool = FALSE;
    gotbool = D3DXQuaternionIsIdentity(&q);
    ok( expectedbool == gotbool, "Expected boolean : %d, Got bool : %d\n", expectedbool, gotbool);
    /* Test the NULL case */
    gotbool = D3DXQuaternionIsIdentity(NULL);
    ok(gotbool == FALSE, "Expected boolean: %d, Got boolean: %d\n", FALSE, gotbool);

/*_______________D3DXQuaternionLength__________________________*/
    got = D3DXQuaternionLength(&q);
    equal = compare_float(got, 11.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXQuaternionLength(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXQuaternionLengthSq________________________*/
    got = D3DXQuaternionLengthSq(&q);
    equal = compare_float(got, 121.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL */
    got = D3DXQuaternionLengthSq(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXQuaternionLn______________________________*/
    expectedquat.x = 1.0f; expectedquat.y = 2.0f; expectedquat.z = 4.0f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&q);
    expect_quaternion(&expectedquat, &gotquat, 0);
    expectedquat.x = -3.0f; expectedquat.y = 4.0f; expectedquat.z = -5.0f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&r);
    expect_quaternion(&expectedquat, &gotquat, 0);
    Nq.x = 1.0f/11.0f; Nq.y = 2.0f/11.0f; Nq.z = 4.0f/11.0f; Nq.w=10.0f/11.0f;
    expectedquat.x = 0.093768f; expectedquat.y = 0.187536f; expectedquat.z = 0.375073f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq);
    expect_quaternion(&expectedquat, &gotquat, 32);
    Nq.x = 0.0f; Nq.y = 0.0f; Nq.z = 0.0f; Nq.w = 1.0f;
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq);
    expect_quaternion(&expectedquat, &gotquat, 0);
    Nq.x = 5.4f; Nq.y = 1.2f; Nq.z = -0.3f; Nq.w = -0.3f;
    expectedquat.x = 10.616652f; expectedquat.y = 2.359256f; expectedquat.z = -0.589814f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq);
    expect_quaternion(&expectedquat, &gotquat, 1);
    /* Test the case where the norm of the quaternion is <1 */
    Nq1.x = 0.2f; Nq1.y = 0.1f; Nq1.z = 0.3; Nq1.w = 0.9f;
    expectedquat.x = 0.206945f; expectedquat.y = 0.103473f; expectedquat.z = 0.310418f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq1);
    expect_quaternion(&expectedquat, &gotquat, 64);
    /* Test the case where the real part of the quaternion is -1.0f */
    Nq1.x = 0.2f; Nq1.y = 0.1f; Nq1.z = 0.3; Nq1.w = -1.0f;
    expectedquat.x = 0.2f; expectedquat.y = 0.1f; expectedquat.z = 0.3f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq1);
    expect_quaternion(&expectedquat, &gotquat, 0);

/*_______________D3DXQuaternionMultiply________________________*/
    expectedquat.x = 3.0f; expectedquat.y = 61.0f; expectedquat.z = -32.0f; expectedquat.w = 85.0f;
    D3DXQuaternionMultiply(&gotquat,&q,&r);
    expect_quaternion(&expectedquat, &gotquat, 0);

/*_______________D3DXQuaternionNormalize________________________*/
    expectedquat.x = 1.0f/11.0f; expectedquat.y = 2.0f/11.0f; expectedquat.z = 4.0f/11.0f; expectedquat.w = 10.0f/11.0f;
    D3DXQuaternionNormalize(&gotquat,&q);
    expect_quaternion(&expectedquat, &gotquat, 1);

/*_______________D3DXQuaternionRotationAxis___________________*/
    axis.x = 2.0f; axis.y = 7.0; axis.z = 13.0f;
    angle = D3DX_PI/3.0f;
    expectedquat.x = 0.067116; expectedquat.y = 0.234905f; expectedquat.z = 0.436251f; expectedquat.w = 0.866025f;
    D3DXQuaternionRotationAxis(&gotquat,&axis,angle);
    expect_quaternion(&expectedquat, &gotquat, 64);
 /* Test the nul quaternion */
    axis.x = 0.0f; axis.y = 0.0; axis.z = 0.0f;
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.866025f;
    D3DXQuaternionRotationAxis(&gotquat,&axis,angle);
    expect_quaternion(&expectedquat, &gotquat, 8);

/*_______________D3DXQuaternionRotationMatrix___________________*/
    /* test when the trace is >0 */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = 10.0f; mat.m[1][1] = 20.0f; mat.m[2][2] = 30.0f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 2.368682f; expectedquat.y = 0.768221f; expectedquat.z = -0.384111f; expectedquat.w = 3.905125f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 16);
    /* test the case when the greater element is (2,2) */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = -60.0f; mat.m[2][2] = 40.0f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 1.233905f; expectedquat.y = -0.237290f; expectedquat.z = 5.267827f; expectedquat.w = -0.284747f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 64);
    /* test the case when the greater element is (1,1) */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 60.0f; mat.m[2][2] = -80.0f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 0.651031f; expectedquat.y = 6.144103f; expectedquat.z = -0.203447f; expectedquat.w = 0.488273f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 8);
    /* test the case when the trace is near 0 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = -0.9f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 1.709495f; expectedquat.y = 2.339872f; expectedquat.z = -0.534217f; expectedquat.w = 1.282122f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 8);
    /* test the case when the trace is 0.49 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = -0.51f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 1.724923f; expectedquat.y = 2.318944f; expectedquat.z = -0.539039f; expectedquat.w = 1.293692f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 8);
    /* test the case when the trace is 0.51 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = -0.49f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 1.725726f; expectedquat.y = 2.317865f; expectedquat.z = -0.539289f; expectedquat.w = 1.294294f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 8);
    /* test the case when the trace is 0.99 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = -0.01f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 1.745328f; expectedquat.y = 2.291833f; expectedquat.z = -0.545415f; expectedquat.w = 1.308996f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 4);
    /* test the case when the trace is 1.0 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = 0.0f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 1.745743f; expectedquat.y = 2.291288f; expectedquat.z = -0.545545f; expectedquat.w = 1.309307f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 8);
    /* test the case when the trace is 1.01 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = 0.01f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 18.408188f; expectedquat.y = 5.970223f; expectedquat.z = -2.985111f; expectedquat.w = 0.502494f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 4);
    /* test the case when the trace is 1.5 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = 0.5f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 15.105186f; expectedquat.y = 4.898980f; expectedquat.z = -2.449490f; expectedquat.w = 0.612372f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 8);
    /* test the case when the trace is 1.7 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = 0.70f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 14.188852f; expectedquat.y = 4.601790f; expectedquat.z = -2.300895f; expectedquat.w = 0.651920f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 4);
    /* test the case when the trace is 1.99 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = 0.99f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 13.114303f; expectedquat.y = 4.253287f; expectedquat.z = -2.126644f; expectedquat.w = 0.705337f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 4);
    /* test the case when the trace is 2.0 in a matrix which is not a rotation */
    mat.m[0][1] = 5.0f; mat.m[0][2] = 7.0f; mat.m[0][3] = 8.0f;
    mat.m[1][0] = 11.0f; mat.m[1][2] = 16.0f; mat.m[1][3] = 33.0f;
    mat.m[2][0] = 19.0f; mat.m[2][1] = -21.0f; mat.m[2][3] = 43.0f;
    mat.m[3][0] = 2.0f; mat.m[3][1] = 3.0f; mat.m[3][2] = -4.0f;
    mat.m[0][0] = -10.0f; mat.m[1][1] = 10.0f; mat.m[2][2] = 2.0f;
    mat.m[3][3] = 48.0f;
    expectedquat.x = 10.680980f; expectedquat.y = 3.464102f; expectedquat.z = -1.732051f; expectedquat.w = 0.866025f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_quaternion(&expectedquat, &gotquat, 8);

/*_______________D3DXQuaternionRotationYawPitchRoll__________*/
    expectedquat.x = 0.303261f; expectedquat.y = 0.262299f; expectedquat.z = 0.410073f; expectedquat.w = 0.819190f;
    D3DXQuaternionRotationYawPitchRoll(&gotquat,D3DX_PI/4.0f,D3DX_PI/11.0f,D3DX_PI/3.0f);
    expect_quaternion(&expectedquat, &gotquat, 16);

/*_______________D3DXQuaternionSlerp________________________*/
    expectedquat.x = -0.2f; expectedquat.y = 2.6f; expectedquat.z = 1.3f; expectedquat.w = 9.1f;
    D3DXQuaternionSlerp(&gotquat,&q,&r,scale);
    expect_quaternion(&expectedquat, &gotquat, 4);
    expectedquat.x = 334.0f; expectedquat.y = -31.9f; expectedquat.z = 6.1f; expectedquat.w = 6.7f;
    D3DXQuaternionSlerp(&gotquat,&q,&t,scale);
    expect_quaternion(&expectedquat, &gotquat, 2);
    expectedquat.x = 0.239485f; expectedquat.y = 0.346580f; expectedquat.z = 0.453676f; expectedquat.w = 0.560772f;
    D3DXQuaternionSlerp(&gotquat,&smallq,&smallr,scale);
    expect_quaternion(&expectedquat, &gotquat, 32);

/*_______________D3DXQuaternionSquad________________________*/
    expectedquat.x = -156.296f; expectedquat.y = 30.242f; expectedquat.z = -2.5022f; expectedquat.w = 7.3576f;
    D3DXQuaternionSquad(&gotquat,&q,&r,&t,&u,scale);
    expect_quaternion(&expectedquat, &gotquat, 2);

/*_______________D3DXQuaternionSquadSetup___________________*/
    r.x = 1.0f; r.y = 2.0f; r.z = 4.0f; r.w = 10.0f;
    s.x = -3.0f; s.y = 4.0f; s.z = -5.0f; s.w = 7.0;
    t.x = -1111.0f; t.y = 111.0f; t.z = -11.0f; t.w = 1.0f;
    u.x = 91.0f; u.y = - 82.0f; u.z = 7.3f; u.w = -6.4f;
    D3DXQuaternionSquadSetup(&gotquat, &Nq, &Nq1, &r, &s, &t, &u);
    expectedquat.x = 7.121285f; expectedquat.y = 2.159964f; expectedquat.z = -3.855094f; expectedquat.w = 5.362844f;
    expect_quaternion(&expectedquat, &gotquat, 16);
    expectedquat.x = -1113.492920f; expectedquat.y = 82.679260f; expectedquat.z = -6.696645f; expectedquat.w = -4.090050f;
    expect_quaternion(&expectedquat, &Nq, 4);
    expectedquat.x = -1111.0f; expectedquat.y = 111.0f; expectedquat.z = -11.0f; expectedquat.w = 1.0f;
    expect_quaternion(&expectedquat, &Nq1, 0);
    gotquat = s;
    D3DXQuaternionSquadSetup(&gotquat, &Nq, &Nq1, &r, &gotquat, &t, &u);
    expectedquat.x = -1113.492920f; expectedquat.y = 82.679260f; expectedquat.z = -6.696645f; expectedquat.w = -4.090050f;
    expect_quaternion(&expectedquat, &Nq, 4);
    Nq1 = u;
    D3DXQuaternionSquadSetup(&gotquat, &Nq, &Nq1, &r, &s, &t, &Nq1);
    expect_quaternion(&expectedquat, &Nq, 4);
    r.x = 0.2f; r.y = 0.3f; r.z = 1.3f; r.w = -0.6f;
    s.x = -3.0f; s.y =-2.0f; s.z = 4.0f; s.w = 0.2f;
    t.x = 0.4f; t.y = 8.3f; t.z = -3.1f; t.w = -2.7f;
    u.x = 1.1f; u.y = -0.7f; u.z = 9.2f; u.w = 0.0f;
    D3DXQuaternionSquadSetup(&gotquat, &Nq, &Nq1, &r, &s, &u, &t);
    expectedquat.x = -4.139569f; expectedquat.y = -2.469115f; expectedquat.z = 2.364477f; expectedquat.w = 0.465494f;
    expect_quaternion(&expectedquat, &gotquat, 16);
    expectedquat.x = 2.342533f; expectedquat.y = 2.365127f; expectedquat.z = 8.628538f; expectedquat.w = -0.898356f;
    expect_quaternion(&expectedquat, &Nq, 16);
    expectedquat.x = 1.1f; expectedquat.y = -0.7f; expectedquat.z = 9.2f; expectedquat.w = 0.0f;
    expect_quaternion(&expectedquat, &Nq1, 0);
    D3DXQuaternionSquadSetup(&gotquat, &Nq, &Nq1, &r, &s, &t, &u);
    expectedquat.x = -3.754567f; expectedquat.y = -0.586085f; expectedquat.z = 3.815818f; expectedquat.w = -0.198150f;
    expect_quaternion(&expectedquat, &gotquat, 32);
    expectedquat.x = 0.140773f; expectedquat.y = -8.737090f; expectedquat.z = -0.516593f; expectedquat.w = 3.053942f;
    expect_quaternion(&expectedquat, &Nq, 16);
    expectedquat.x = -0.4f; expectedquat.y = -8.3f; expectedquat.z = 3.1f; expectedquat.w = 2.7f;
    expect_quaternion(&expectedquat, &Nq1, 0);
    r.x = -1.0f; r.y = 0.0f; r.z = 0.0f; r.w = 0.0f;
    s.x = 1.0f; s.y =0.0f; s.z = 0.0f; s.w = 0.0f;
    t.x = 1.0f; t.y = 0.0f; t.z = 0.0f; t.w = 0.0f;
    u.x = -1.0f; u.y = 0.0f; u.z = 0.0f; u.w = 0.0f;
    D3DXQuaternionSquadSetup(&gotquat, &Nq, &Nq1, &r, &s, &t, &u);
    expectedquat.x = 1.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    expect_quaternion(&expectedquat, &gotquat, 0);
    expectedquat.x = 1.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    expect_quaternion(&expectedquat, &Nq, 0);
    expectedquat.x = 1.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    expect_quaternion(&expectedquat, &Nq1, 0);

/*_______________D3DXQuaternionToAxisAngle__________________*/
    Nq.x = 1.0f/22.0f; Nq.y = 2.0f/22.0f; Nq.z = 4.0f/22.0f; Nq.w = 10.0f/22.0f;
    expectedvec.x = 1.0f/22.0f; expectedvec.y = 2.0f/22.0f; expectedvec.z = 4.0f/22.0f;
    D3DXQuaternionToAxisAngle(&Nq,&axis,&angle);
    expect_vec3(&expectedvec, &axis, 0);
    equal = compare_float(angle, 2.197869f, 1);
    ok(equal, "Got unexpected angle %.8e.\n", angle);
    /* Test if |w|>1.0f */
    expectedvec.x = 1.0f; expectedvec.y = 2.0f; expectedvec.z = 4.0f;
    D3DXQuaternionToAxisAngle(&q,&axis,&angle);
    expect_vec3(&expectedvec, &axis, 0);
    /* Test the null quaternion */
    expectedvec.x = 0.0f; expectedvec.y = 0.0f; expectedvec.z = 0.0f;
    D3DXQuaternionToAxisAngle(&nul, &axis, &angle);
    expect_vec3(&expectedvec, &axis, 0);
    equal = compare_float(angle, 3.14159274e+00f, 0);
    ok(equal, "Got unexpected angle %.8e.\n", angle);

    D3DXQuaternionToAxisAngle(&nul, &axis, NULL);
    D3DXQuaternionToAxisAngle(&nul, NULL, &angle);
    expect_vec3(&expectedvec, &axis, 0);
    equal = compare_float(angle, 3.14159274e+00f, 0);
    ok(equal, "Got unexpected angle %.8e.\n", angle);
}

static void D3DXVector2Test(void)
{
    D3DXVECTOR2 expectedvec, gotvec, nul, u, v, w, x;
    LPD3DXVECTOR2 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    float coeff1, coeff2, got, scale;
    D3DXMATRIX mat;
    BOOL equal;

    nul.x = 0.0f; nul.y = 0.0f;
    u.x = 3.0f; u.y = 4.0f;
    v.x = -7.0f; v.y = 9.0f;
    w.x = 4.0f; w.y = -3.0f;
    x.x = 2.0f; x.y = -11.0f;

    set_matrix(&mat,
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f);

    coeff1 = 2.0f; coeff2 = 5.0f;
    scale = -6.5f;

/*_______________D3DXVec2Add__________________________*/
    expectedvec.x = -4.0f; expectedvec.y = 13.0f;
    D3DXVec2Add(&gotvec,&u,&v);
    expect_vec2(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec2Add(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Add(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2BaryCentric___________________*/
    expectedvec.x = -12.0f; expectedvec.y = -21.0f;
    D3DXVec2BaryCentric(&gotvec,&u,&v,&w,coeff1,coeff2);
    expect_vec2(&expectedvec, &gotvec, 0);

/*_______________D3DXVec2CatmullRom____________________*/
    expectedvec.x = 5820.25f; expectedvec.y = -3654.5625f;
    D3DXVec2CatmullRom(&gotvec,&u,&v,&w,&x,scale);
    expect_vec2(&expectedvec, &gotvec, 0);

/*_______________D3DXVec2CCW__________________________*/
    got = D3DXVec2CCW(&u, &v);
    equal = compare_float(got, 55.0f, 0);
    ok(equal, "Got unexpected ccw %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec2CCW(NULL, &v);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected ccw %.8e.\n", got);
    got = D3DXVec2CCW(NULL, NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected ccw %.8e.\n", got);

/*_______________D3DXVec2Dot__________________________*/
    got = D3DXVec2Dot(&u, &v);
    equal = compare_float(got, 15.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    /* Tests the case NULL */
    got = D3DXVec2Dot(NULL, &v);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    got = D3DXVec2Dot(NULL, NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);

/*_______________D3DXVec2Hermite__________________________*/
    expectedvec.x = 2604.625f; expectedvec.y = -4533.0f;
    D3DXVec2Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec2(&expectedvec, &gotvec, 0);

/*_______________D3DXVec2Length__________________________*/
    got = D3DXVec2Length(&u);
    equal = compare_float(got, 5.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec2Length(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXVec2LengthSq________________________*/
    got = D3DXVec2LengthSq(&u);
    equal = compare_float(got, 25.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec2LengthSq(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXVec2Lerp__________________________*/
    expectedvec.x = 68.0f; expectedvec.y = -28.5f;
    D3DXVec2Lerp(&gotvec, &u, &v, scale);
    expect_vec2(&expectedvec, &gotvec, 0);
    /* Tests the case NULL. */
    funcpointer = D3DXVec2Lerp(&gotvec,NULL,&v,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Lerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Maximize__________________________*/
    expectedvec.x = 3.0f; expectedvec.y = 9.0f;
    D3DXVec2Maximize(&gotvec, &u, &v);
    expect_vec2(&expectedvec, &gotvec, 0);
    /* Tests the case NULL. */
    funcpointer = D3DXVec2Maximize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Maximize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Minimize__________________________*/
    expectedvec.x = -7.0f; expectedvec.y = 4.0f;
    D3DXVec2Minimize(&gotvec,&u,&v);
    expect_vec2(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec2Minimize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Minimize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Normalize_________________________*/
    expectedvec.x = 0.6f; expectedvec.y = 0.8f;
    D3DXVec2Normalize(&gotvec,&u);
    expect_vec2(&expectedvec, &gotvec, 0);
    /* Test the nul vector */
    expectedvec.x = 0.0f; expectedvec.y = 0.0f;
    D3DXVec2Normalize(&gotvec,&nul);
    expect_vec2(&expectedvec, &gotvec, 0);

/*_______________D3DXVec2Scale____________________________*/
    expectedvec.x = -19.5f; expectedvec.y = -26.0f;
    D3DXVec2Scale(&gotvec,&u,scale);
    expect_vec2(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec2Scale(&gotvec,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Scale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Subtract__________________________*/
    expectedvec.x = 10.0f; expectedvec.y = -5.0f;
    D3DXVec2Subtract(&gotvec, &u, &v);
    expect_vec2(&expectedvec, &gotvec, 0);
    /* Tests the case NULL. */
    funcpointer = D3DXVec2Subtract(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Subtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Transform_______________________*/
    expectedtrans.x = 36.0f; expectedtrans.y = 44.0f; expectedtrans.z = 52.0f; expectedtrans.w = 60.0f;
    D3DXVec2Transform(&gottrans, &u, &mat);
    expect_vec4(&expectedtrans, &gottrans, 0);
    gottrans.x = u.x; gottrans.y = u.y;
    D3DXVec2Transform(&gottrans, (D3DXVECTOR2 *)&gottrans, &mat);
    expect_vec4(&expectedtrans, &gottrans, 0);

/*_______________D3DXVec2TransformCoord_______________________*/
    expectedvec.x = 0.6f; expectedvec.y = 11.0f/15.0f;
    D3DXVec2TransformCoord(&gotvec, &u, &mat);
    expect_vec2(&expectedvec, &gotvec, 1);
    gotvec.x = u.x; gotvec.y = u.y;
    D3DXVec2TransformCoord(&gotvec, &gotvec, &mat);
    expect_vec2(&expectedvec, &gotvec, 1);

 /*_______________D3DXVec2TransformNormal______________________*/
    expectedvec.x = 23.0f; expectedvec.y = 30.0f;
    D3DXVec2TransformNormal(&gotvec,&u,&mat);
    expect_vec2(&expectedvec, &gotvec, 0);
}

static void D3DXVector3Test(void)
{
    D3DVIEWPORT9 viewport;
    D3DXVECTOR3 expectedvec, gotvec, nul, u, v, w, x;
    LPD3DXVECTOR3 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    D3DXMATRIX mat, projection, view, world;
    float coeff1, coeff2, got, scale;
    BOOL equal;

    nul.x = 0.0f; nul.y = 0.0f; nul.z = 0.0f;
    u.x = 9.0f; u.y = 6.0f; u.z = 2.0f;
    v.x = 2.0f; v.y = -3.0f; v.z = -4.0;
    w.x = 3.0f; w.y = -5.0f; w.z = 7.0f;
    x.x = 4.0f; x.y = 1.0f; x.z = 11.0f;

    viewport.Width = 800; viewport.MinZ = 0.2f; viewport.X = 10;
    viewport.Height = 680; viewport.MaxZ = 0.9f; viewport.Y = 5;

    set_matrix(&mat,
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f);

    view.m[0][1] = 5.0f; view.m[0][2] = 7.0f; view.m[0][3] = 8.0f;
    view.m[1][0] = 11.0f; view.m[1][2] = 16.0f; view.m[1][3] = 33.0f;
    view.m[2][0] = 19.0f; view.m[2][1] = -21.0f; view.m[2][3] = 43.0f;
    view.m[3][0] = 2.0f; view.m[3][1] = 3.0f; view.m[3][2] = -4.0f;
    view.m[0][0] = 10.0f; view.m[1][1] = 20.0f; view.m[2][2] = 30.0f;
    view.m[3][3] = -40.0f;

    set_matrix(&world,
            21.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 23.0f, 7.0f, 8.0f,
            -8.0f, -7.0f, 25.0f, -5.0f,
            -4.0f, -3.0f, -2.0f, 27.0f);

    coeff1 = 2.0f; coeff2 = 5.0f;
    scale = -6.5f;

/*_______________D3DXVec3Add__________________________*/
    expectedvec.x = 11.0f; expectedvec.y = 3.0f; expectedvec.z = -2.0f;
    D3DXVec3Add(&gotvec,&u,&v);
    expect_vec3(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Add(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Add(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3BaryCentric___________________*/
    expectedvec.x = -35.0f; expectedvec.y = -67.0; expectedvec.z = 15.0f;
    D3DXVec3BaryCentric(&gotvec,&u,&v,&w,coeff1,coeff2);
    expect_vec3(&expectedvec, &gotvec, 0);

/*_______________D3DXVec3CatmullRom____________________*/
    expectedvec.x = 1458.0f; expectedvec.y = 22.1875f; expectedvec.z = 4141.375f;
    D3DXVec3CatmullRom(&gotvec,&u,&v,&w,&x,scale);
    expect_vec3(&expectedvec, &gotvec, 0);

/*_______________D3DXVec3Cross________________________*/
    expectedvec.x = -18.0f; expectedvec.y = 40.0f; expectedvec.z = -39.0f;
    D3DXVec3Cross(&gotvec,&u,&v);
    expect_vec3(&expectedvec, &gotvec, 0);
    expectedvec.x = -277.0f; expectedvec.y = -150.0f; expectedvec.z = -26.0f;
    D3DXVec3Cross(&gotvec,&gotvec,&v);
    expect_vec3(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Cross(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Cross(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Dot__________________________*/
    got = D3DXVec3Dot(&u, &v);
    equal = compare_float(got, -8.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    /* Tests the case NULL */
    got = D3DXVec3Dot(NULL, &v);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    got = D3DXVec3Dot(NULL, NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);

/*_______________D3DXVec3Hermite__________________________*/
    expectedvec.x = -6045.75f; expectedvec.y = -6650.0f; expectedvec.z = 1358.875f;
    D3DXVec3Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec3(&expectedvec, &gotvec, 0);

/*_______________D3DXVec3Length__________________________*/
    got = D3DXVec3Length(&u);
    equal = compare_float(got, 11.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec3Length(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXVec3LengthSq________________________*/
    got = D3DXVec3LengthSq(&u);
    equal = compare_float(got, 121.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec3LengthSq(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXVec3Lerp__________________________*/
    expectedvec.x = 54.5f; expectedvec.y = 64.5f; expectedvec.z = 41.0f ;
    D3DXVec3Lerp(&gotvec,&u,&v,scale);
    expect_vec3(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Lerp(&gotvec,NULL,&v,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Lerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Maximize__________________________*/
    expectedvec.x = 9.0f; expectedvec.y = 6.0f; expectedvec.z = 2.0f;
    D3DXVec3Maximize(&gotvec,&u,&v);
    expect_vec3(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Maximize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Maximize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Minimize__________________________*/
    expectedvec.x = 2.0f; expectedvec.y = -3.0f; expectedvec.z = -4.0f;
    D3DXVec3Minimize(&gotvec,&u,&v);
    expect_vec3(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Minimize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Minimize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Normalize_________________________*/
    expectedvec.x = 9.0f/11.0f; expectedvec.y = 6.0f/11.0f; expectedvec.z = 2.0f/11.0f;
    D3DXVec3Normalize(&gotvec,&u);
    expect_vec3(&expectedvec, &gotvec, 1);
    /* Test the nul vector */
    expectedvec.x = 0.0f; expectedvec.y = 0.0f; expectedvec.z = 0.0f;
    D3DXVec3Normalize(&gotvec,&nul);
    expect_vec3(&expectedvec, &gotvec, 0);

/*_______________D3DXVec3Scale____________________________*/
    expectedvec.x = -58.5f; expectedvec.y = -39.0f; expectedvec.z = -13.0f;
    D3DXVec3Scale(&gotvec,&u,scale);
    expect_vec3(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Scale(&gotvec,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Scale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Subtract_______________________*/
    expectedvec.x = 7.0f; expectedvec.y = 9.0f; expectedvec.z = 6.0f;
    D3DXVec3Subtract(&gotvec,&u,&v);
    expect_vec3(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Subtract(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Subtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Transform_______________________*/
    expectedtrans.x = 70.0f; expectedtrans.y = 88.0f; expectedtrans.z = 106.0f; expectedtrans.w = 124.0f;
    D3DXVec3Transform(&gottrans, &u, &mat);
    expect_vec4(&expectedtrans, &gottrans, 0);

    gottrans.x = u.x; gottrans.y = u.y; gottrans.z = u.z;
    D3DXVec3Transform(&gottrans, (D3DXVECTOR3 *)&gottrans, &mat);
    expect_vec4(&expectedtrans, &gottrans, 0);

/*_______________D3DXVec3TransformCoord_______________________*/
    expectedvec.x = 70.0f/124.0f; expectedvec.y = 88.0f/124.0f; expectedvec.z = 106.0f/124.0f;
    D3DXVec3TransformCoord(&gotvec,&u,&mat);
    expect_vec3(&expectedvec, &gotvec, 1);

/*_______________D3DXVec3TransformNormal______________________*/
    expectedvec.x = 57.0f; expectedvec.y = 74.0f; expectedvec.z = 91.0f;
    D3DXVec3TransformNormal(&gotvec,&u,&mat);
    expect_vec3(&expectedvec, &gotvec, 0);

/*_______________D3DXVec3Project_________________________*/
    expectedvec.x = 1135.721924f; expectedvec.y = 147.086914f; expectedvec.z = 0.153412f;
    D3DXMatrixPerspectiveFovLH(&projection,D3DX_PI/4.0f,20.0f/17.0f,1.0f,1000.0f);
    D3DXVec3Project(&gotvec,&u,&viewport,&projection,&view,&world);
    expect_vec3(&expectedvec, &gotvec, 32);
    /* World matrix can be omitted */
    D3DXMatrixMultiply(&mat,&world,&view);
    D3DXVec3Project(&gotvec,&u,&viewport,&projection,&mat,NULL);
    expect_vec3(&expectedvec, &gotvec, 32);
    /* Projection matrix can be omitted */
    D3DXMatrixMultiply(&mat,&view,&projection);
    D3DXVec3Project(&gotvec,&u,&viewport,NULL,&mat,&world);
    expect_vec3(&expectedvec, &gotvec, 32);
    /* View matrix can be omitted */
    D3DXMatrixMultiply(&mat,&world,&view);
    D3DXVec3Project(&gotvec,&u,&viewport,&projection,NULL,&mat);
    expect_vec3(&expectedvec, &gotvec, 32);
    /* All matrices can be omitted */
    expectedvec.x = 4010.000000f; expectedvec.y = -1695.000000f; expectedvec.z = 1.600000f;
    D3DXVec3Project(&gotvec,&u,&viewport,NULL,NULL,NULL);
    expect_vec3(&expectedvec, &gotvec, 2);
    /* Viewport can be omitted */
    expectedvec.x = 1.814305f; expectedvec.y = 0.582097f; expectedvec.z = -0.066555f;
    D3DXVec3Project(&gotvec,&u,NULL,&projection,&view,&world);
    expect_vec3(&expectedvec, &gotvec, 64);

/*_______________D3DXVec3Unproject_________________________*/
    expectedvec.x = -2.913411f; expectedvec.y = 1.593215f; expectedvec.z = 0.380724f;
    D3DXMatrixPerspectiveFovLH(&projection,D3DX_PI/4.0f,20.0f/17.0f,1.0f,1000.0f);
    D3DXVec3Unproject(&gotvec,&u,&viewport,&projection,&view,&world);
    expect_vec3(&expectedvec, &gotvec, 16);
    /* World matrix can be omitted */
    D3DXMatrixMultiply(&mat,&world,&view);
    D3DXVec3Unproject(&gotvec,&u,&viewport,&projection,&mat,NULL);
    expect_vec3(&expectedvec, &gotvec, 16);
    /* Projection matrix can be omitted */
    D3DXMatrixMultiply(&mat,&view,&projection);
    D3DXVec3Unproject(&gotvec,&u,&viewport,NULL,&mat,&world);
    expect_vec3(&expectedvec, &gotvec, 32);
    /* View matrix can be omitted */
    D3DXMatrixMultiply(&mat,&world,&view);
    D3DXVec3Unproject(&gotvec,&u,&viewport,&projection,NULL,&mat);
    expect_vec3(&expectedvec, &gotvec, 16);
    /* All matrices can be omitted */
    expectedvec.x = -1.002500f; expectedvec.y = 0.997059f; expectedvec.z = 2.571429f;
    D3DXVec3Unproject(&gotvec,&u,&viewport,NULL,NULL,NULL);
    expect_vec3(&expectedvec, &gotvec, 4);
    /* Viewport can be omitted */
    expectedvec.x = -11.018396f; expectedvec.y = 3.218991f; expectedvec.z = 1.380329f;
    D3DXVec3Unproject(&gotvec,&u,NULL,&projection,&view,&world);
    expect_vec3(&expectedvec, &gotvec, 8);
}

static void D3DXVector4Test(void)
{
    D3DXVECTOR4 expectedvec, gotvec, u, v, w, x;
    LPD3DXVECTOR4 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    float coeff1, coeff2, got, scale;
    D3DXMATRIX mat;
    BOOL equal;

    u.x = 1.0f; u.y = 2.0f; u.z = 4.0f; u.w = 10.0;
    v.x = -3.0f; v.y = 4.0f; v.z = -5.0f; v.w = 7.0;
    w.x = 4.0f; w.y =6.0f; w.z = -2.0f; w.w = 1.0f;
    x.x = 6.0f; x.y = -7.0f; x.z =8.0f; x.w = -9.0f;

    set_matrix(&mat,
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f);

    coeff1 = 2.0f; coeff2 = 5.0;
    scale = -6.5f;

/*_______________D3DXVec4Add__________________________*/
    expectedvec.x = -2.0f; expectedvec.y = 6.0f; expectedvec.z = -1.0f; expectedvec.w = 17.0f;
    D3DXVec4Add(&gotvec,&u,&v);
    expect_vec4(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Add(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Add(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4BaryCentric____________________*/
    expectedvec.x = 8.0f; expectedvec.y = 26.0; expectedvec.z =  -44.0f; expectedvec.w = -41.0f;
    D3DXVec4BaryCentric(&gotvec,&u,&v,&w,coeff1,coeff2);
    expect_vec4(&expectedvec, &gotvec, 0);

/*_______________D3DXVec4CatmullRom____________________*/
    expectedvec.x = 2754.625f; expectedvec.y = 2367.5625f; expectedvec.z = 1060.1875f; expectedvec.w = 131.3125f;
    D3DXVec4CatmullRom(&gotvec,&u,&v,&w,&x,scale);
    expect_vec4(&expectedvec, &gotvec, 0);

/*_______________D3DXVec4Cross_________________________*/
    expectedvec.x = 390.0f; expectedvec.y = -393.0f; expectedvec.z = -316.0f; expectedvec.w = 166.0f;
    D3DXVec4Cross(&gotvec,&u,&v,&w);
    expect_vec4(&expectedvec, &gotvec, 0);

/*_______________D3DXVec4Dot__________________________*/
    got = D3DXVec4Dot(&u, &v);
    equal = compare_float(got, 55.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec4Dot(NULL, &v);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);
    got = D3DXVec4Dot(NULL, NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected dot %.8e.\n", got);

/*_______________D3DXVec4Hermite_________________________*/
    expectedvec.x = 1224.625f; expectedvec.y = 3461.625f; expectedvec.z = -4758.875f; expectedvec.w = -5781.5f;
    D3DXVec4Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec4(&expectedvec, &gotvec, 0);

/*_______________D3DXVec4Length__________________________*/
    got = D3DXVec4Length(&u);
    equal = compare_float(got, 11.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec4Length(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXVec4LengthSq________________________*/
    got = D3DXVec4LengthSq(&u);
    equal = compare_float(got, 121.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);
    /* Tests the case NULL. */
    got = D3DXVec4LengthSq(NULL);
    equal = compare_float(got, 0.0f, 0);
    ok(equal, "Got unexpected length %.8e.\n", got);

/*_______________D3DXVec4Lerp__________________________*/
    expectedvec.x = 27.0f; expectedvec.y = -11.0f; expectedvec.z = 62.5;  expectedvec.w = 29.5;
    D3DXVec4Lerp(&gotvec,&u,&v,scale);
    expect_vec4(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Lerp(&gotvec,NULL,&v,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Lerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Maximize__________________________*/
    expectedvec.x = 1.0f; expectedvec.y = 4.0f; expectedvec.z = 4.0f; expectedvec.w = 10.0;
    D3DXVec4Maximize(&gotvec,&u,&v);
    expect_vec4(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Maximize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Maximize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Minimize__________________________*/
    expectedvec.x = -3.0f; expectedvec.y = 2.0f; expectedvec.z = -5.0f; expectedvec.w = 7.0;
    D3DXVec4Minimize(&gotvec,&u,&v);
    expect_vec4(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Minimize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Minimize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Normalize_________________________*/
    expectedvec.x = 1.0f/11.0f; expectedvec.y = 2.0f/11.0f; expectedvec.z = 4.0f/11.0f; expectedvec.w = 10.0f/11.0f;
    D3DXVec4Normalize(&gotvec,&u);
    expect_vec4(&expectedvec, &gotvec, 1);

/*_______________D3DXVec4Scale____________________________*/
    expectedvec.x = -6.5f; expectedvec.y = -13.0f; expectedvec.z = -26.0f; expectedvec.w = -65.0f;
    D3DXVec4Scale(&gotvec,&u,scale);
    expect_vec4(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Scale(&gotvec,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Scale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Subtract__________________________*/
    expectedvec.x = 4.0f; expectedvec.y = -2.0f; expectedvec.z = 9.0f; expectedvec.w = 3.0f;
    D3DXVec4Subtract(&gotvec,&u,&v);
    expect_vec4(&expectedvec, &gotvec, 0);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Subtract(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Subtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Transform_______________________*/
    expectedtrans.x = 177.0f; expectedtrans.y = 194.0f; expectedtrans.z = 211.0f; expectedtrans.w = 228.0f;
    D3DXVec4Transform(&gottrans,&u,&mat);
    expect_vec4(&expectedtrans, &gottrans, 0);
}

static void test_matrix_stack(void)
{
    ID3DXMatrixStack *stack;
    ULONG refcount;
    HRESULT hr;

    const D3DXMATRIX mat1 = {{{
         1.0f,  2.0f,  3.0f,  4.0f,
         5.0f,  6.0f,  7.0f,  8.0f,
         9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    }}};

    const D3DXMATRIX mat2 = {{{
        17.0f, 18.0f, 19.0f, 20.0f,
        21.0f, 22.0f, 23.0f, 24.0f,
        25.0f, 26.0f, 27.0f, 28.0f,
        29.0f, 30.0f, 31.0f, 32.0f
    }}};

    hr = D3DXCreateMatrixStack(0, &stack);
    ok(SUCCEEDED(hr), "Failed to create a matrix stack, hr %#lx\n", hr);
    if (FAILED(hr)) return;

    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)),
            "The top of an empty matrix stack should be an identity matrix\n");

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#lx\n", hr);

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#lx\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#lx\n", hr);

    hr = ID3DXMatrixStack_LoadMatrix(stack, &mat1);
    ok(SUCCEEDED(hr), "LoadMatrix failed, hr %#lx\n", hr);
    expect_matrix(&mat1, ID3DXMatrixStack_GetTop(stack), 0);

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#lx\n", hr);
    expect_matrix(&mat1, ID3DXMatrixStack_GetTop(stack), 0);

    hr = ID3DXMatrixStack_LoadMatrix(stack, &mat2);
    ok(SUCCEEDED(hr), "LoadMatrix failed, hr %#lx\n", hr);
    expect_matrix(&mat2, ID3DXMatrixStack_GetTop(stack), 0);

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#lx\n", hr);
    expect_matrix(&mat2, ID3DXMatrixStack_GetTop(stack), 0);

    hr = ID3DXMatrixStack_LoadIdentity(stack);
    ok(SUCCEEDED(hr), "LoadIdentity failed, hr %#lx\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#lx\n", hr);
    expect_matrix(&mat2, ID3DXMatrixStack_GetTop(stack), 0);

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#lx\n", hr);
    expect_matrix(&mat1, ID3DXMatrixStack_GetTop(stack), 0);

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#lx\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#lx\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    refcount = ID3DXMatrixStack_Release(stack);
    ok(!refcount, "Matrix stack has %lu references left.\n", refcount);
}

static void test_Matrix_AffineTransformation2D(void)
{
    D3DXMATRIX exp_mat, got_mat;
    D3DXVECTOR2 center, position;
    FLOAT angle, scale;

    center.x = 3.0f;
    center.y = 4.0f;

    position.x = -6.0f;
    position.y = 7.0f;

    angle = D3DX_PI/3.0f;

    scale = 20.0f;

    exp_mat.m[0][0] = 10.0f;
    exp_mat.m[1][0] = -17.320507f;
    exp_mat.m[2][0] = 0.0f;
    exp_mat.m[3][0] = -1.035898f;
    exp_mat.m[0][1] = 17.320507f;
    exp_mat.m[1][1] = 10.0f;
    exp_mat.m[2][1] = 0.0f;
    exp_mat.m[3][1] = 6.401924f;
    exp_mat.m[0][2] = 0.0f;
    exp_mat.m[1][2] = 0.0f;
    exp_mat.m[2][2] = 1.0f;
    exp_mat.m[3][2] = 0.0f;
    exp_mat.m[0][3] = 0.0f;
    exp_mat.m[1][3] = 0.0f;
    exp_mat.m[2][3] = 0.0f;
    exp_mat.m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, &center, angle, &position);
    expect_matrix(&exp_mat, &got_mat, 2);

/*______________*/

    center.x = 3.0f;
    center.y = 4.0f;

    angle = D3DX_PI/3.0f;

    scale = 20.0f;

    exp_mat.m[0][0] = 10.0f;
    exp_mat.m[1][0] = -17.320507f;
    exp_mat.m[2][0] = 0.0f;
    exp_mat.m[3][0] = 4.964102f;
    exp_mat.m[0][1] = 17.320507f;
    exp_mat.m[1][1] = 10.0f;
    exp_mat.m[2][1] = 0.0f;
    exp_mat.m[3][1] = -0.598076f;
    exp_mat.m[0][2] = 0.0f;
    exp_mat.m[1][2] = 0.0f;
    exp_mat.m[2][2] = 1.0f;
    exp_mat.m[3][2] = 0.0f;
    exp_mat.m[0][3] = 0.0f;
    exp_mat.m[1][3] = 0.0f;
    exp_mat.m[2][3] = 0.0f;
    exp_mat.m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, &center, angle, NULL);
    expect_matrix(&exp_mat, &got_mat, 8);

/*______________*/

    position.x = -6.0f;
    position.y = 7.0f;

    angle = D3DX_PI/3.0f;

    scale = 20.0f;

    exp_mat.m[0][0] = 10.0f;
    exp_mat.m[1][0] = -17.320507f;
    exp_mat.m[2][0] = 0.0f;
    exp_mat.m[3][0] = -6.0f;
    exp_mat.m[0][1] = 17.320507f;
    exp_mat.m[1][1] = 10.0f;
    exp_mat.m[2][1] = 0.0f;
    exp_mat.m[3][1] = 7.0f;
    exp_mat.m[0][2] = 0.0f;
    exp_mat.m[1][2] = 0.0f;
    exp_mat.m[2][2] = 1.0f;
    exp_mat.m[3][2] = 0.0f;
    exp_mat.m[0][3] = 0.0f;
    exp_mat.m[1][3] = 0.0f;
    exp_mat.m[2][3] = 0.0f;
    exp_mat.m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, NULL, angle, &position);
    expect_matrix(&exp_mat, &got_mat, 1);

/*______________*/

    angle = 5.0f * D3DX_PI/4.0f;

    scale = -20.0f;

    exp_mat.m[0][0] = 14.142133f;
    exp_mat.m[1][0] = -14.142133f;
    exp_mat.m[2][0] = 0.0f;
    exp_mat.m[3][0] = 0.0f;
    exp_mat.m[0][1] = 14.142133;
    exp_mat.m[1][1] = 14.142133f;
    exp_mat.m[2][1] = 0.0f;
    exp_mat.m[3][1] = 0.0f;
    exp_mat.m[0][2] = 0.0f;
    exp_mat.m[1][2] = 0.0f;
    exp_mat.m[2][2] = 1.0f;
    exp_mat.m[3][2] = 0.0f;
    exp_mat.m[0][3] = 0.0f;
    exp_mat.m[1][3] = 0.0f;
    exp_mat.m[2][3] = 0.0f;
    exp_mat.m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, NULL, angle, NULL);
    expect_matrix(&exp_mat, &got_mat, 8);
}

static void test_Matrix_Decompose(void)
{
    D3DXMATRIX pm;
    D3DXQUATERNION exp_rotation, got_rotation;
    D3DXVECTOR3 exp_scale, got_scale, exp_translation, got_translation;
    HRESULT hr;
    BOOL equal;

/*___________*/

    pm.m[0][0] = -9.23879206e-01f;
    pm.m[1][0] = -2.70598412e-01f;
    pm.m[2][0] =  2.70598441e-01f;
    pm.m[3][0] = -5.00000000e+00f;
    pm.m[0][1] =  2.70598471e-01f;
    pm.m[1][1] =  3.80604863e-02f;
    pm.m[2][1] =  9.61939573e-01f;
    pm.m[3][1] =  0.00000000e+00f;
    pm.m[0][2] = -2.70598441e-01f;
    pm.m[1][2] =  9.61939573e-01f;
    pm.m[2][2] =  3.80603075e-02f;
    pm.m[3][2] =  1.00000000e+01f;
    pm.m[0][3] =  0.00000000e+00f;
    pm.m[1][3] =  0.00000000e+00f;
    pm.m[2][3] =  0.00000000e+00f;
    pm.m[3][3] =  1.00000000e+00f;

    exp_scale.x = 9.99999881e-01f;
    exp_scale.y = 9.99999881e-01f;
    exp_scale.z = 9.99999881e-01f;

    exp_rotation.x = 2.14862776e-08f;
    exp_rotation.y = 6.93519890e-01f;
    exp_rotation.z = 6.93519890e-01f;
    exp_rotation.w = 1.95090637e-01f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 1);
    expect_quaternion(&exp_rotation, &got_rotation, 1);
    expect_vec3(&exp_translation, &got_translation, 0);

/*_________*/

    pm.m[0][0] = -2.255813f;
    pm.m[1][0] = 1.302324f;
    pm.m[2][0] = 1.488373f;
    pm.m[3][0] = 1.0f;
    pm.m[0][1] = 1.302327f;
    pm.m[1][1] = -0.7209296f;
    pm.m[2][1] = 2.60465f;
    pm.m[3][1] = 2.0f;
    pm.m[0][2] = 1.488371f;
    pm.m[1][2] = 2.604651f;
    pm.m[2][2] = -0.02325551f;
    pm.m[3][2] = 3.0f;
    pm.m[0][3] = 0.0f;
    pm.m[1][3] = 0.0f;
    pm.m[2][3] = 0.0f;
    pm.m[3][3] = 1.0f;

    exp_scale.x = 2.99999928e+00f;
    exp_scale.y = 2.99999905e+00f;
    exp_scale.z = 2.99999952e+00f;

    exp_rotation.x = 3.52180451e-01f;
    exp_rotation.y = 6.16315663e-01f;
    exp_rotation.z = 7.04360664e-01f;
    exp_rotation.w = 3.38489343e-07f;

    exp_translation.x = 1.0f;
    exp_translation.y = 2.0f;
    exp_translation.z = 3.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 0);
    expect_quaternion(&exp_rotation, &got_rotation, 2);
    expect_vec3(&exp_translation, &got_translation, 0);

/*_____________*/

    pm.m[0][0] = 2.427051f;
    pm.m[1][0] = 0.0f;
    pm.m[2][0] = 1.763355f;
    pm.m[3][0] = 5.0f;
    pm.m[0][1] = 0.0f;
    pm.m[1][1] = 3.0f;
    pm.m[2][1] = 0.0f;
    pm.m[3][1] = 5.0f;
    pm.m[0][2] = -1.763355f;
    pm.m[1][2] = 0.0f;
    pm.m[2][2] = 2.427051f;
    pm.m[3][2] = 5.0f;
    pm.m[0][3] = 0.0f;
    pm.m[1][3] = 0.0f;
    pm.m[2][3] = 0.0f;
    pm.m[3][3] = 1.0f;

    exp_scale.x = 2.99999976e+00f;
    exp_scale.y = 3.00000000e+00f;
    exp_scale.z = 2.99999976e+00f;

    exp_rotation.x = 0.00000000e+00f;
    exp_rotation.y = 3.09016883e-01f;
    exp_rotation.z = 0.00000000e+00f;
    exp_rotation.w = 9.51056540e-01f;

    exp_translation.x = 5.0f;
    exp_translation.y = 5.0f;
    exp_translation.z = 5.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 1);
    expect_quaternion(&exp_rotation, &got_rotation, 1);
    expect_vec3(&exp_translation, &got_translation, 0);

/*_____________*/

    pm.m[0][0] = -9.23879206e-01f;
    pm.m[1][0] = -2.70598412e-01f;
    pm.m[2][0] =  2.70598441e-01f;
    pm.m[3][0] = -5.00000000e+00f;
    pm.m[0][1] =  2.70598471e-01f;
    pm.m[1][1] =  3.80604863e-02f;
    pm.m[2][1] =  9.61939573e-01f;
    pm.m[3][1] =  0.00000000e+00f;
    pm.m[0][2] = -2.70598441e-01f;
    pm.m[1][2] =  9.61939573e-01f;
    pm.m[2][2] =  3.80603075e-02f;
    pm.m[3][2] =  1.00000000e+01f;
    pm.m[0][3] =  0.00000000e+00f;
    pm.m[1][3] =  0.00000000e+00f;
    pm.m[2][3] =  0.00000000e+00f;
    pm.m[3][3] =  1.00000000e+00f;

    exp_scale.x = 9.99999881e-01f;
    exp_scale.y = 9.99999881e-01f;
    exp_scale.z = 9.99999881e-01f;

    exp_rotation.x = 2.14862776e-08f;
    exp_rotation.y = 6.93519890e-01f;
    exp_rotation.z = 6.93519890e-01f;
    exp_rotation.w = 1.95090637e-01f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 1);
    expect_quaternion(&exp_rotation, &got_rotation, 1);
    expect_vec3(&exp_translation, &got_translation, 0);

/*__________*/

    pm.m[0][0] = -9.23878908e-01f;
    pm.m[1][0] = -5.41196704e-01f;
    pm.m[2][0] =  8.11795175e-01f;
    pm.m[3][0] = -5.00000000e+00f;
    pm.m[0][1] =  2.70598322e-01f;
    pm.m[1][1] =  7.61209577e-02f;
    pm.m[2][1] =  2.88581824e+00f;
    pm.m[3][1] =  0.00000000e+00f;
    pm.m[0][2] = -2.70598352e-01f;
    pm.m[1][2] =  1.92387879e+00f;
    pm.m[2][2] =  1.14180908e-01f;
    pm.m[3][2] =  1.00000000e+01f;
    pm.m[0][3] =  0.00000000e+00f;
    pm.m[1][3] =  0.00000000e+00f;
    pm.m[2][3] =  0.00000000e+00f;
    pm.m[3][3] =  1.00000000e+00f;

    exp_scale.x = 9.99999583e-01f;
    exp_scale.y = 1.99999940e+00f;
    exp_scale.z = 2.99999928e+00f;

    exp_rotation.x = 1.07431388e-08f;
    exp_rotation.y = 6.93519890e-01f;
    exp_rotation.z = 6.93519831e-01f;
    exp_rotation.w = 1.95090622e-01f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 1);
    equal = compare_quaternion(&exp_rotation, &got_rotation, 1);
    exp_rotation.x = 0.0f;
    equal |= compare_quaternion(&exp_rotation, &got_rotation, 2);
    ok(equal, "Got unexpected quaternion {%.8e, %.8e, %.8e, %.8e}.\n",
            got_rotation.x, got_rotation.y, got_rotation.z, got_rotation.w);
    expect_vec3(&exp_translation, &got_translation, 0);

/*__________*/

    pm.m[0][0] = 0.7156004f;
    pm.m[1][0] = -0.5098283f;
    pm.m[2][0] = -0.4774843f;
    pm.m[3][0] = -5.0f;
    pm.m[0][1] = -0.6612288f;
    pm.m[1][1] = -0.7147621f;
    pm.m[2][1] = -0.2277977f;
    pm.m[3][1] = 0.0f;
    pm.m[0][2] = -0.2251499f;
    pm.m[1][2] = 0.4787385f;
    pm.m[2][2] = -0.8485972f;
    pm.m[3][2] = 10.0f;
    pm.m[0][3] = 0.0f;
    pm.m[1][3] = 0.0f;
    pm.m[2][3] = 0.0f;
    pm.m[3][3] = 1.0f;

    exp_scale.x = 9.99999940e-01f;
    exp_scale.y = 1.00000012e+00f;
    exp_scale.z = 1.00000012e+00f;

    exp_rotation.x = 9.05394852e-01f;
    exp_rotation.y = -3.23355347e-01f;
    exp_rotation.z = -1.94013178e-01f;
    exp_rotation.w = 1.95090592e-01f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 0);
    expect_quaternion(&exp_rotation, &got_rotation, 1);
    expect_vec3(&exp_translation, &got_translation, 0);

/*_____________*/

    pm.m[0][0] = 0.06554436f;
    pm.m[1][0] = -0.6873012f;
    pm.m[2][0] = 0.7234092f;
    pm.m[3][0] = -5.0f;
    pm.m[0][1] = -0.9617381f;
    pm.m[1][1] = -0.2367795f;
    pm.m[2][1] = -0.1378230f;
    pm.m[3][1] = 0.0f;
    pm.m[0][2] = 0.2660144f;
    pm.m[1][2] = -0.6866967f;
    pm.m[2][2] = -0.6765233f;
    pm.m[3][2] = 10.0f;
    pm.m[0][3] = 0.0f;
    pm.m[1][3] = 0.0f;
    pm.m[2][3] = 0.0f;
    pm.m[3][3] = 1.0f;

    exp_scale.x = 9.99999940e-01f;
    exp_scale.y = 9.99999940e-01f;
    exp_scale.z = 9.99999881e-01f;

    exp_rotation.x = 7.03357518e-01f;
    exp_rotation.y = -5.86131275e-01f;
    exp_rotation.z = 3.51678789e-01f;
    exp_rotation.w = -1.95090577e-01f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 1);
    expect_quaternion(&exp_rotation, &got_rotation, 2);
    expect_vec3(&exp_translation, &got_translation, 0);

/*_________*/

    pm.m[0][0] =  7.12104797e+00f;
    pm.m[1][0] = -5.88348627e+00f;
    pm.m[2][0] =  1.18184204e+01f;
    pm.m[3][0] = -5.00000000e+00f;
    pm.m[0][1] =  5.88348627e+00f;
    pm.m[1][1] = -1.06065865e+01f;
    pm.m[2][1] = -8.82523251e+00f;
    pm.m[3][1] =  0.00000000e+00f;
    pm.m[0][2] =  1.18184204e+01f;
    pm.m[1][2] =  8.82523155e+00f;
    pm.m[2][2] = -2.72764111e+00f;
    pm.m[3][2] =  2.00000000e+00f;
    pm.m[0][3] =  0.00000000e+00f;
    pm.m[1][3] =  0.00000000e+00f;
    pm.m[2][3] =  0.00000000e+00f;
    pm.m[3][3] =  1.00000000e+00f;

    exp_scale.x = 1.49999933e+01f;
    exp_scale.y = 1.49999933e+01f;
    exp_scale.z = 1.49999943e+01f;

    exp_rotation.x = 7.68714130e-01f;
    exp_rotation.y = 0.00000000e+00f;
    exp_rotation.z = 5.12475967e-01f;
    exp_rotation.w = 3.82683903e-01f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 2.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    expect_vec3(&exp_scale, &got_scale, 0);
    expect_quaternion(&exp_rotation, &got_rotation, 1);
    expect_vec3(&exp_translation, &got_translation, 0);

/*__________*/

    pm.m[0][0] = 0.0f;
    pm.m[1][0] = 4.0f;
    pm.m[2][0] = 5.0f;
    pm.m[3][0] = -5.0f;
    pm.m[0][1] = 0.0f;
    pm.m[1][1] = -10.60660f;
    pm.m[2][1] = -8.825232f;
    pm.m[3][1] = 6.0f;
    pm.m[0][2] = 0.0f;
    pm.m[1][2] = 8.8252320f;
    pm.m[2][2] = 2.727645;
    pm.m[3][2] = 3.0f;
    pm.m[0][3] = 0.0f;
    pm.m[1][3] = 0.0f;
    pm.m[2][3] = 0.0f;
    pm.m[3][3] = 1.0f;

    hr = D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %lx\n", hr);
}

static void test_Matrix_Transformation2D(void)
{
    D3DXMATRIX exp_mat, got_mat;
    D3DXVECTOR2 rot_center, sca, sca_center, trans;
    FLOAT rot, sca_rot;

    rot_center.x = 3.0f;
    rot_center.y = 4.0f;

    sca.x = 12.0f;
    sca.y = -3.0f;

    sca_center.x = 9.0f;
    sca_center.y = -5.0f;

    trans.x = -6.0f;
    trans.y = 7.0f;

    rot = D3DX_PI/3.0f;

    sca_rot = 5.0f*D3DX_PI/4.0f;

    exp_mat.m[0][0] = -4.245192f;
    exp_mat.m[1][0] = -0.147116f;
    exp_mat.m[2][0] = 0.0f;
    exp_mat.m[3][0] = 45.265373f;
    exp_mat.m[0][1] = 7.647113f;
    exp_mat.m[1][1] = 8.745192f;
    exp_mat.m[2][1] = 0.0f;
    exp_mat.m[3][1] = -13.401899f;
    exp_mat.m[0][2] = 0.0f;
    exp_mat.m[1][2] = 0.0f;
    exp_mat.m[2][2] = 1.0f;
    exp_mat.m[3][2] = 0.0f;
    exp_mat.m[0][3] = 0.0f;
    exp_mat.m[1][3] = 0.0f;
    exp_mat.m[2][3] = 0.0f;
    exp_mat.m[3][3] = 1.0f;

    D3DXMatrixTransformation2D(&got_mat, &sca_center, sca_rot, &sca, &rot_center, rot, &trans);
    expect_matrix(&exp_mat, &got_mat, 64);

/*_________*/

    sca_center.x = 9.0f;
    sca_center.y = -5.0f;

    trans.x = -6.0f;
    trans.y = 7.0f;

    rot = D3DX_PI/3.0f;

    sca_rot = 5.0f*D3DX_PI/4.0f;

    exp_mat.m[0][0] = 0.50f;
    exp_mat.m[1][0] = -0.866025f;
    exp_mat.m[2][0] = 0.0f;
    exp_mat.m[3][0] = -6.0f;
    exp_mat.m[0][1] = 0.866025f;
    exp_mat.m[1][1] = 0.50f;
    exp_mat.m[2][1] = 0.0f;
    exp_mat.m[3][1] = 7.0f;
    exp_mat.m[0][2] = 0.0f;
    exp_mat.m[1][2] = 0.0f;
    exp_mat.m[2][2] = 1.0f;
    exp_mat.m[3][2] = 0.0f;
    exp_mat.m[0][3] = 0.0f;
    exp_mat.m[1][3] = 0.0f;
    exp_mat.m[2][3] = 0.0f;
    exp_mat.m[3][3] = 1.0f;

    D3DXMatrixTransformation2D(&got_mat, &sca_center, sca_rot, NULL, NULL, rot, &trans);
    expect_matrix(&exp_mat, &got_mat, 8);

/*_________*/

    exp_mat.m[0][0] = 0.50f;
    exp_mat.m[1][0] = -0.866025f;
    exp_mat.m[2][0] = 0.0f;
    exp_mat.m[3][0] = 0.0f;
    exp_mat.m[0][1] = 0.866025f;
    exp_mat.m[1][1] = 0.50f;
    exp_mat.m[2][1] = 0.0f;
    exp_mat.m[3][1] = 0.0f;
    exp_mat.m[0][2] = 0.0f;
    exp_mat.m[1][2] = 0.0f;
    exp_mat.m[2][2] = 1.0f;
    exp_mat.m[3][2] = 0.0f;
    exp_mat.m[0][3] = 0.0f;
    exp_mat.m[1][3] = 0.0f;
    exp_mat.m[2][3] = 0.0f;
    exp_mat.m[3][3] = 1.0f;

    D3DXMatrixTransformation2D(&got_mat, NULL, sca_rot, NULL, NULL, rot, NULL);
    expect_matrix(&exp_mat, &got_mat, 8);
}

static void test_D3DXVec_Array(void)
{
    D3DXPLANE inp_plane[5], out_plane[7], exp_plane[7];
    D3DXVECTOR4 inp_vec[5], out_vec[7], exp_vec[7];
    D3DXMATRIX mat, projection, view, world;
    D3DVIEWPORT9 viewport;
    unsigned int i;

    viewport.Width = 800; viewport.MinZ = 0.2f; viewport.X = 10;
    viewport.Height = 680; viewport.MaxZ = 0.9f; viewport.Y = 5;

    memset(out_vec, 0, sizeof(out_vec));
    memset(exp_vec, 0, sizeof(exp_vec));
    memset(out_plane, 0, sizeof(out_plane));
    memset(exp_plane, 0, sizeof(exp_plane));

    for (i = 0; i < ARRAY_SIZE(inp_vec); ++i)
    {
        inp_plane[i].a = inp_plane[i].c = inp_vec[i].x = inp_vec[i].z = i;
        inp_plane[i].b = inp_plane[i].d = inp_vec[i].y = inp_vec[i].w = ARRAY_SIZE(inp_vec) - i;
    }

    set_matrix(&mat,
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f);

    D3DXMatrixPerspectiveFovLH(&projection, D3DX_PI / 4.0f, 20.0f / 17.0f, 1.0f, 1000.0f);

    view.m[0][1] = 5.0f; view.m[0][2] = 7.0f; view.m[0][3] = 8.0f;
    view.m[1][0] = 11.0f; view.m[1][2] = 16.0f; view.m[1][3] = 33.0f;
    view.m[2][0] = 19.0f; view.m[2][1] = -21.0f; view.m[2][3] = 43.0f;
    view.m[3][0] = 2.0f; view.m[3][1] = 3.0f; view.m[3][2] = -4.0f;
    view.m[0][0] = 10.0f; view.m[1][1] = 20.0f; view.m[2][2] = 30.0f;
    view.m[3][3] = -40.0f;

    set_matrix(&world,
            21.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 23.0f, 7.0f, 8.0f,
            -8.0f, -7.0f, 25.0f, -5.0f,
            -4.0f, -3.0f, -2.0f, 27.0f);

    /* D3DXVec2TransformCoordArray */
    exp_vec[1].x = 6.78571403e-01f; exp_vec[1].y = 7.85714269e-01f;
    exp_vec[2].x = 6.53846204e-01f; exp_vec[2].y = 7.69230783e-01f;
    exp_vec[3].x = 6.25000000e-01f; exp_vec[3].y = 7.50000000e-01f;
    exp_vec[4].x = 5.90909123e-01f; exp_vec[4].y = 7.27272749e-01f;
    exp_vec[5].x = 5.49999952e-01f; exp_vec[5].y = 6.99999928e-01f;
    D3DXVec2TransformCoordArray((D3DXVECTOR2 *)&out_vec[1], sizeof(*out_vec),
            (D3DXVECTOR2 *)inp_vec, sizeof(*inp_vec), &mat, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 1);

    /* D3DXVec2TransformNormalArray */
    exp_vec[1].x = 25.0f; exp_vec[1].y = 30.0f;
    exp_vec[2].x = 21.0f; exp_vec[2].y = 26.0f;
    exp_vec[3].x = 17.0f; exp_vec[3].y = 22.0f;
    exp_vec[4].x = 13.0f; exp_vec[4].y = 18.0f;
    exp_vec[5].x =  9.0f; exp_vec[5].y = 14.0f;
    D3DXVec2TransformNormalArray((D3DXVECTOR2 *)&out_vec[1], sizeof(*out_vec),
            (D3DXVECTOR2 *)inp_vec, sizeof(*inp_vec), &mat, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 0);

    /* D3DXVec3TransformCoordArray */
    exp_vec[1].x = 6.78571403e-01f; exp_vec[1].y = 7.85714269e-01f; exp_vec[1].z = 8.92857075e-01f;
    exp_vec[2].x = 6.71874940e-01f; exp_vec[2].y = 7.81249940e-01f; exp_vec[2].z = 8.90624940e-01f;
    exp_vec[3].x = 6.66666627e-01f; exp_vec[3].y = 7.77777731e-01f; exp_vec[3].z = 8.88888836e-01f;
    exp_vec[4].x = 6.62499964e-01f; exp_vec[4].y = 7.74999976e-01f; exp_vec[4].z = 8.87499928e-01f;
    exp_vec[5].x = 6.59090877e-01f; exp_vec[5].y = 7.72727251e-01f; exp_vec[5].z = 8.86363566e-01f;
    D3DXVec3TransformCoordArray((D3DXVECTOR3 *)&out_vec[1], sizeof(*out_vec),
            (D3DXVECTOR3 *)inp_vec, sizeof(*inp_vec), &mat, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 1);

    /* D3DXVec3TransformNormalArray */
    exp_vec[1].x = 25.0f; exp_vec[1].y = 30.0f; exp_vec[1].z = 35.0f;
    exp_vec[2].x = 30.0f; exp_vec[2].y = 36.0f; exp_vec[2].z = 42.0f;
    exp_vec[3].x = 35.0f; exp_vec[3].y = 42.0f; exp_vec[3].z = 49.0f;
    exp_vec[4].x = 40.0f; exp_vec[4].y = 48.0f; exp_vec[4].z = 56.0f;
    exp_vec[5].x = 45.0f; exp_vec[5].y = 54.0f; exp_vec[5].z = 63.0f;
    D3DXVec3TransformNormalArray((D3DXVECTOR3 *)&out_vec[1], sizeof(*out_vec),
            (D3DXVECTOR3 *)inp_vec, sizeof(*inp_vec), &mat, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 0);

    /* D3DXVec3ProjectArray */
    exp_vec[1].x = 1.08955420e+03f; exp_vec[1].y = -2.26590622e+02f; exp_vec[1].z = 2.15272754e-01f;
    exp_vec[2].x = 1.06890344e+03f; exp_vec[2].y =  1.03085144e+02f; exp_vec[2].z = 1.83049560e-01f;
    exp_vec[3].x = 1.05177905e+03f; exp_vec[3].y =  3.76462280e+02f; exp_vec[3].z = 1.56329080e-01f;
    exp_vec[4].x = 1.03734888e+03f; exp_vec[4].y =  6.06827393e+02f; exp_vec[4].z = 1.33812696e-01f;
    exp_vec[5].x = 1.02502356e+03f; exp_vec[5].y =  8.03591248e+02f; exp_vec[5].z = 1.14580572e-01f;
    D3DXVec3ProjectArray((D3DXVECTOR3 *)&out_vec[1], sizeof(*out_vec), (D3DXVECTOR3 *)inp_vec,
            sizeof(*inp_vec), &viewport, &projection, &view, &world, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 8);

    /* D3DXVec3UnprojectArray */
    exp_vec[1].x = -6.12403107e+00f; exp_vec[1].y = 3.22536016e+00f; exp_vec[1].z = 6.20571136e-01f;
    exp_vec[2].x = -3.80710936e+00f; exp_vec[2].y = 2.04657936e+00f; exp_vec[2].z = 4.46894377e-01f;
    exp_vec[3].x = -2.92283988e+00f; exp_vec[3].y = 1.59668946e+00f; exp_vec[3].z = 3.80609393e-01f;
    exp_vec[4].x = -2.45622563e+00f; exp_vec[4].y = 1.35928988e+00f; exp_vec[4].z = 3.45631927e-01f;
    exp_vec[5].x = -2.16789746e+00f; exp_vec[5].y = 1.21259713e+00f; exp_vec[5].z = 3.24018806e-01f;
    D3DXVec3UnprojectArray((D3DXVECTOR3 *)&out_vec[1], sizeof(*out_vec), (D3DXVECTOR3 *)inp_vec,
            sizeof(*inp_vec), &viewport, &projection, &view, &world, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 4);

    /* D3DXVec2TransformArray */
    exp_vec[1].x = 38.0f; exp_vec[1].y = 44.0f; exp_vec[1].z = 50.0f; exp_vec[1].w = 56.0f;
    exp_vec[2].x = 34.0f; exp_vec[2].y = 40.0f; exp_vec[2].z = 46.0f; exp_vec[2].w = 52.0f;
    exp_vec[3].x = 30.0f; exp_vec[3].y = 36.0f; exp_vec[3].z = 42.0f; exp_vec[3].w = 48.0f;
    exp_vec[4].x = 26.0f; exp_vec[4].y = 32.0f; exp_vec[4].z = 38.0f; exp_vec[4].w = 44.0f;
    exp_vec[5].x = 22.0f; exp_vec[5].y = 28.0f; exp_vec[5].z = 34.0f; exp_vec[5].w = 40.0f;
    D3DXVec2TransformArray(&out_vec[1], sizeof(*out_vec), (D3DXVECTOR2 *)inp_vec,
            sizeof(*inp_vec), &mat, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 0);

    /* D3DXVec3TransformArray */
    exp_vec[1].x = 38.0f; exp_vec[1].y = 44.0f; exp_vec[1].z = 50.0f; exp_vec[1].w = 56.0f;
    exp_vec[2].x = 43.0f; exp_vec[2].y = 50.0f; exp_vec[2].z = 57.0f; exp_vec[2].w = 64.0f;
    exp_vec[3].x = 48.0f; exp_vec[3].y = 56.0f; exp_vec[3].z = 64.0f; exp_vec[3].w = 72.0f;
    exp_vec[4].x = 53.0f; exp_vec[4].y = 62.0f; exp_vec[4].z = 71.0f; exp_vec[4].w = 80.0f;
    exp_vec[5].x = 58.0f; exp_vec[5].y = 68.0f; exp_vec[5].z = 78.0f; exp_vec[5].w = 88.0f;
    D3DXVec3TransformArray(&out_vec[1], sizeof(*out_vec), (D3DXVECTOR3 *)inp_vec,
            sizeof(*inp_vec), &mat, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 0);

    /* D3DXVec4TransformArray */
    exp_vec[1].x = 90.0f; exp_vec[1].y = 100.0f; exp_vec[1].z = 110.0f; exp_vec[1].w = 120.0f;
    exp_vec[2].x = 82.0f; exp_vec[2].y = 92.0f;  exp_vec[2].z = 102.0f; exp_vec[2].w = 112.0f;
    exp_vec[3].x = 74.0f; exp_vec[3].y = 84.0f;  exp_vec[3].z = 94.0f;  exp_vec[3].w = 104.0f;
    exp_vec[4].x = 66.0f; exp_vec[4].y = 76.0f;  exp_vec[4].z = 86.0f;  exp_vec[4].w = 96.0f;
    exp_vec[5].x = 58.0f; exp_vec[5].y = 68.0f;  exp_vec[5].z = 78.0f;  exp_vec[5].w = 88.0f;
    D3DXVec4TransformArray(&out_vec[1], sizeof(*out_vec), inp_vec, sizeof(*inp_vec), &mat, ARRAY_SIZE(inp_vec));
    expect_vec4_array(ARRAY_SIZE(exp_vec), exp_vec, out_vec, 0);

    /* D3DXPlaneTransformArray */
    exp_plane[1].a = 90.0f; exp_plane[1].b = 100.0f; exp_plane[1].c = 110.0f; exp_plane[1].d = 120.0f;
    exp_plane[2].a = 82.0f; exp_plane[2].b = 92.0f;  exp_plane[2].c = 102.0f; exp_plane[2].d = 112.0f;
    exp_plane[3].a = 74.0f; exp_plane[3].b = 84.0f;  exp_plane[3].c = 94.0f;  exp_plane[3].d = 104.0f;
    exp_plane[4].a = 66.0f; exp_plane[4].b = 76.0f;  exp_plane[4].c = 86.0f;  exp_plane[4].d = 96.0f;
    exp_plane[5].a = 58.0f; exp_plane[5].b = 68.0f;  exp_plane[5].c = 78.0f;  exp_plane[5].d = 88.0f;
    D3DXPlaneTransformArray(&out_plane[1], sizeof(*out_plane), inp_plane,
            sizeof(*inp_plane), &mat, ARRAY_SIZE(inp_plane));
    for (i = 0; i < ARRAY_SIZE(exp_plane); ++i)
    {
        BOOL equal = compare_plane(&exp_plane[i], &out_plane[i], 0);
        ok(equal, "Got unexpected plane {%.8e, %.8e, %.8e, %.8e} at index %u, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                out_plane[i].a, out_plane[i].b, out_plane[i].c, out_plane[i].d, i,
                exp_plane[i].a, exp_plane[i].b, exp_plane[i].c, exp_plane[i].d);
        if (!equal)
            break;
    }
}

static void test_D3DXFloat_Array(void)
{
    unsigned int i;
    void *out = NULL;
    D3DXFLOAT16 half;
    BOOL equal;

    /* Input floats through bit patterns because compilers do not generate reliable INF and NaN values. */
    union convert
    {
        DWORD d;
        float f;
    } single;

    struct
    {
        union convert single_in;

        /* half_ver2 occurs on WXPPROSP3 (32 bit math), WVISTAADM (32/64 bit math), W7PRO (32 bit math) */
        WORD half_ver1, half_ver2;

        /* single_out_ver2 confirms that half -> single conversion is consistent across platforms */
        union convert single_out_ver1, single_out_ver2;
    }
    testdata[] =
    {
        { { 0x479c4000 }, 0x7c00, 0x7ce2, { 0x47800000 }, { 0x479c4000 } }, /* 80000.0f */
        { { 0x477fdf00 }, 0x7bff, 0x7bff, { 0x477fe000 }, { 0x477fe000 } }, /* 65503.0f */
        { { 0x477fe000 }, 0x7bff, 0x7bff, { 0x477fe000 }, { 0x477fe000 } }, /* 65504.0f */
        { { 0x477ff000 }, 0x7bff, 0x7c00, { 0x477fe000 }, { 0x47800000 } }, /* 65520.0f */
        { { 0x477ff100 }, 0x7c00, 0x7c00, { 0x47800000 }, { 0x47800000 } }, /* 65521.0f */

        { { 0x477ffe00 }, 0x7c00, 0x7c00, { 0x47800000 }, { 0x47800000 } }, /* 65534.0f */
        { { 0x477fff00 }, 0x7c00, 0x7c00, { 0x47800000 }, { 0x47800000 } }, /* 65535.0f */
        { { 0x47800000 }, 0x7c00, 0x7c00, { 0x47800000 }, { 0x47800000 } }, /* 65536.0f */
        { { 0xc79c4000 }, 0xfc00, 0xfce2, { 0xc7800000 }, { 0xc79c4000 } }, /* -80000.0f */
        { { 0xc77fdf00 }, 0xfbff, 0xfbff, { 0xc77fe000 }, { 0xc77fe000 } }, /* -65503.0f */

        { { 0xc77fe000 }, 0xfbff, 0xfbff, { 0xc77fe000 }, { 0xc77fe000 } }, /* -65504.0f */
        { { 0xc77ff000 }, 0xfbff, 0xfc00, { 0xc77fe000 }, { 0xc7800000 } }, /* -65520.0f */
        { { 0xc77ff100 }, 0xfc00, 0xfc00, { 0xc7800000 }, { 0xc7800000 } }, /* -65521.0f */
        { { 0xc77ffe00 }, 0xfc00, 0xfc00, { 0xc7800000 }, { 0xc7800000 } }, /* -65534.0f */
        { { 0xc77fff00 }, 0xfc00, 0xfc00, { 0xc7800000 }, { 0xc7800000 } }, /* -65535.0f */

        { { 0xc7800000 }, 0xfc00, 0xfc00, { 0xc7800000 }, { 0xc7800000 } }, /* -65536.0f */
        { { 0x7f800000 }, 0x7c00, 0x7fff, { 0x47800000 }, { 0x47ffe000 } }, /* INF */
        { { 0xff800000 }, 0xffff, 0xffff, { 0xc7ffe000 }, { 0xc7ffe000 } }, /* -INF */
        { { 0x7fc00000 }, 0x7fff, 0xffff, { 0x47ffe000 }, { 0xc7ffe000 } }, /* NaN */
        { { 0xffc00000 }, 0xffff, 0xffff, { 0xc7ffe000 }, { 0xc7ffe000 } }, /* -NaN */

        { { 0x00000000 }, 0x0000, 0x0000, { 0x00000000 }, { 0x00000000 } }, /* 0.0f */
        { { 0x80000000 }, 0x8000, 0x8000, { 0x80000000 }, { 0x80000000 } }, /* -0.0f */
        { { 0x330007ff }, 0x0000, 0x0000, { 0x00000000 }, { 0x00000000 } }, /* 2.9809595e-08f */
        { { 0xb30007ff }, 0x8000, 0x8000, { 0x80000000 }, { 0x80000000 } }, /* -2.9809595e-08f */
        { { 0x33000800 }, 0x0001, 0x0000, { 0x33800000 }, { 0x00000000 } }, /* 2.9809598e-08f */

        { { 0xb3000800 }, 0x8001, 0x8000, { 0xb3800000 }, { 0x80000000 } }, /* -2.9809598e-08f */
        { { 0x33c00000 }, 0x0002, 0x0001, { 0x34000000 }, { 0x33800000 } }, /* 8.9406967e-08f */
    };

    /* exception on NULL out or in parameter */
    single.f = 0.0f;
    out = D3DXFloat32To16Array(&half, &single.f, 0);
    ok(out == &half, "Got %p, expected %p.\n", out, &half);

    out = D3DXFloat16To32Array(&single.f, &half, 0);
    ok(out == &single.f, "Got %p, expected %p.\n", out, &single.f);

    for (i = 0; i < ARRAY_SIZE(testdata); ++i)
    {
        out = D3DXFloat32To16Array(&half, &testdata[i].single_in.f, 1);
        ok(out == &half, "Got %p, expected %p.\n", out, &half);
        ok(half.value == testdata[i].half_ver1 || half.value == testdata[i].half_ver2,
           "Got %x, expected %x or %x for index %d.\n", half.value, testdata[i].half_ver1,
           testdata[i].half_ver2, i);

        out = D3DXFloat16To32Array(&single.f, (D3DXFLOAT16 *)&testdata[i].half_ver1, 1);
        ok(out == &single.f, "Got %p, expected %p.\n", out, &single.f);
        equal = compare_float(single.f, testdata[i].single_out_ver1.f, 0);
        ok(equal, "Got %#lx, expected %#lx at index %u.\n", single.d, testdata[i].single_out_ver1.d, i);

        out = D3DXFloat16To32Array(&single.f, (D3DXFLOAT16 *)&testdata[i].half_ver2, 1);
        ok(out == &single.f, "Got %p, expected %p.\n", out, &single.f);
        equal = compare_float(single.f, testdata[i].single_out_ver2.f, 0);
        ok(equal, "Got %#lx, expected %#lx at index %u.\n", single.d, testdata[i].single_out_ver2.d, i);
    }
}

static void test_D3DXSHAdd(void)
{
    float out[50] = {0.0f};
    unsigned int i, k;
    float *ret;
    BOOL equal;

    static const float in1[50] =
    {
        1.11f, 1.12f, 1.13f, 1.14f, 1.15f, 1.16f, 1.17f, 1.18f,
        1.19f, 1.20f, 1.21f, 1.22f, 1.23f, 1.24f, 1.25f, 1.26f,
        1.27f, 1.28f, 1.29f, 1.30f, 1.31f, 1.32f, 1.33f, 1.34f,
        1.35f, 1.36f, 1.37f, 1.38f, 1.39f, 1.40f, 1.41f, 1.42f,
        1.43f, 1.44f, 1.45f, 1.46f, 1.47f, 1.48f, 1.49f, 1.50f,
        1.51f, 1.52f, 1.53f, 1.54f, 1.55f, 1.56f, 1.57f, 1.58f,
        1.59f, 1.60f,
    };
    static const float in2[50] =
    {
        2.11f, 2.12f, 2.13f, 2.14f, 2.15f, 2.16f, 2.17f, 2.18f,
        2.19f, 2.20f, 2.21f, 2.22f, 2.23f, 2.24f, 2.25f, 2.26f,
        2.27f, 2.28f, 2.29f, 2.30f, 2.31f, 2.32f, 2.33f, 2.34f,
        2.35f, 2.36f, 2.37f, 2.38f, 2.39f, 2.40f, 2.41f, 2.42f,
        2.43f, 2.44f, 2.45f, 2.46f, 2.47f, 2.48f, 2.49f, 2.50f,
        2.51f, 2.52f, 2.53f, 2.54f, 2.55f, 2.56f, 2.57f, 2.58f,
        2.59f, 2.60f,
    };

    /*
     * Order is not limited by D3DXSH_MINORDER and D3DXSH_MAXORDER!
     * All values will work, test from 0-7 [D3DXSH_MINORDER = 2, D3DXSH_MAXORDER = 6]
     * Exceptions will show up when out, in1 or in2 is NULL
     */
    for (k = 0; k <= D3DXSH_MAXORDER + 1; k++)
    {
        UINT count = k * k;

        ret = D3DXSHAdd(&out[0], k, &in1[0], &in2[0]);
        ok(ret == out, "%u: D3DXSHAdd() failed, got %p, expected %p\n", k, out, ret);

        for (i = 0; i < count; ++i)
        {
            equal = compare_float(in1[i] + in2[i], out[i], 0);
            ok(equal, "%u-%u: Got %.8e, expected %.8e.\n", k, i, out[i], in1[i] + in2[i]);
        }
        equal = compare_float(out[count], 0.0f, 0);
        ok(equal, "%u-%u: Got %.8e, expected 0.0.\n", k, k * k, out[count]);
    }
}

static void test_D3DXSHDot(void)
{
    float a[49], b[49], got;
    unsigned int i;
    BOOL equal;

    static const float expected[] = {0.5f, 0.5f, 25.0f, 262.5f, 1428.0f, 5362.5f, 15873.0f, 39812.5f};

    for (i = 0; i < ARRAY_SIZE(a); ++i)
    {
        a[i] = i + 1.0f;
        b[i] = i + 0.5f;
    }

    /* D3DXSHDot computes by using order * order elements */
    for (i = 0; i <= D3DXSH_MAXORDER + 1; i++)
    {
        got = D3DXSHDot(i, a, b);
        equal = compare_float(got, expected[i], 0);
        ok(equal, "order %u: Got %.8e, expected %.8e.\n", i, got, expected[i]);
    }
}

static void test_D3DXSHEvalConeLight(void)
{
    float bout[49], expected, gout[49], rout[49];
    unsigned int j, l, order;
    D3DXVECTOR3 dir;
    HRESULT hr;
    BOOL equal;

    static const float table[] =
    {
    /* Red colour */
        1.604815f, -3.131381f, 7.202175f, -2.870432f, 6.759296f, -16.959688f,
        32.303082f, -15.546381f, -0.588878f, -5.902123f, 40.084042f, -77.423569f,
        137.556320f, -70.971603f, -3.492171f, 7.683092f, -2.129311f, -35.971344f,
        183.086548f, -312.414948f, 535.091064f, -286.380371f, -15.950727f, 46.825714f,
        -12.127637f, 11.289261f, -12.417809f, -155.039566f, 681.182556f, -1079.733643f,
        1807.650513f, -989.755798f, -59.345467f, 201.822815f, -70.726486f, 7.206529f,

        3.101155f, -3.128710f, 7.196033f, -2.867984f, -0.224708f, 0.563814f,
        -1.073895f, 0.516829f, 0.019577f, 2.059788f, -13.988971f, 27.020128f,
        -48.005917f, 24.768450f, 1.218736f, -2.681329f, -0.088639f, -1.497410f,
        7.621501f, -13.005165f, 22.274696f, -11.921401f, -0.663995f, 1.949254f,
        -0.504848f, 4.168484f, -4.585193f, -57.247314f, 251.522095f, -398.684387f,
        667.462891f, -365.460693f, -21.912912f, 74.521721f, -26.115280f, 2.660963f,
    /* Green colour */
        2.454422f, -4.789170f, 11.015091f, -4.390072f, 10.337747f, -25.938347f,
        49.404713f, -23.776817f, -0.900637f, -9.026776f, 61.305000f, -118.412514f,
        210.380249f, -108.544792f, -5.340967f, 11.750610f, -3.256593f, -55.014996f,
        280.014709f, -477.811066f, 818.374512f, -437.993469f, -24.395227f, 71.615799f,
        -18.548151f, 17.265928f, -18.991943f, -237.119324f, 1041.808594f, -1651.357300f,
        2764.642090f, -1513.744141f, -90.763657f, 308.670197f, -108.169922f, 11.021750f,

        4.742942f, -4.785086f, 11.005697f, -4.386329f, -0.343672f, 0.862303f,
        -1.642427f, 0.790444f, 0.029941f, 3.150264f, -21.394896f, 41.324898f,
        -73.420807f, 37.881153f, 1.863950f, -4.100857f, -0.135565f, -2.290156f,
        11.656413f, -19.890251f, 34.067181f, -18.232729f, -1.015521f, 2.981212f,
        -0.772120f, 6.375328f, -7.012648f, -87.554710f, 384.680817f, -609.752563f,
        1020.825500f, -558.939819f, -33.513863f, 113.974388f, -39.941013f, 4.069707f,
    /* Blue colour */
        3.304030f, -6.446959f, 14.828006f, -5.909713f, 13.916198f, -34.917004f,
        66.506340f, -32.007256f, -1.212396f, -12.151429f, 82.525963f, -159.401459f,
        283.204193f, -146.117996f, -7.189764f, 15.818130f, -4.383876f, -74.058655f,
        376.942871f, -643.207214f, 1101.658081f, -589.606628f, -32.839729f, 96.405884f,
        -24.968664f, 23.242596f, -25.566080f, -319.199097f, 1402.434692f, -2222.980957f,
        3721.633545f, -2037.732544f, -122.181847f, 415.517578f, -145.613358f, 14.836972f,

        6.384730f, -6.441462f, 14.815362f, -5.904673f, -0.462635f, 1.160793f,
        -2.210959f, 1.064060f, 0.040305f, 4.240739f, -28.800821f, 55.629673f,
        -98.835709f, 50.993862f, 2.509163f, -5.520384f, -0.182491f, -3.082903f,
        15.691326f, -26.775339f, 45.859665f, -24.544060f, -1.367048f, 4.013170f,
        -1.039392f, 8.582172f, -9.440103f, -117.862114f, 517.839600f, -820.820740f,
        1374.188232f, -752.419067f, -45.114819f, 153.427063f, -53.766754f, 5.478452f, };
    const struct
    {
        float *red_received, *green_received, *blue_received;
        const float *red_expected, *green_expected, *blue_expected;
        float radius, roffset, goffset, boffset;
    }
    test[] =
    {
        { rout, gout, bout, table, &table[72], &table[144], 0.5f, 1.01f, 1.02f, 1.03f, },
        { rout, gout, bout, &table[36], &table[108], &table[180], 1.6f, 1.01f, 1.02f, 1.03f, },
        { rout, rout, rout, &table[144], &table[144], &table[144], 0.5f, 1.03f, 1.03f, 1.03f, },
        { rout, rout, bout, &table[72], &table[72], &table[144], 0.5, 1.02f, 1.02f, 1.03f, },
        { rout, gout, gout, table, &table[144], &table[144], 0.5f, 1.01f, 1.03f, 1.03f, },
        { rout, gout, rout, &table[144], &table[72], &table[144], 0.5f, 1.03f, 1.02f, 1.03f, },
    /* D3DXSHEvalConeLight accepts NULL green or blue colour. */
        { rout, NULL, bout, table, NULL, &table[144], 0.5f, 1.01f, 0.0f, 1.03f, },
        { rout, gout, NULL, table, &table[72], NULL, 0.5f, 1.01f, 1.02f, 0.0f, },
        { rout, NULL, NULL, table, NULL, NULL, 0.5f, 1.01f, 0.0f, 0.0f, },
    };

    dir.x = 1.1f; dir.y = 1.2f; dir.z = 2.76f;

    for (l = 0; l < ARRAY_SIZE(test); ++l)
    {
        for (order = D3DXSH_MINORDER; order <= D3DXSH_MAXORDER; order++)
        {
            for (j = 0; j < 49; j++)
            {
                test[l].red_received[j] = 1.01f + j;
                if (test[l].green_received)
                    test[l].green_received[j] = 1.02f + j;
                if (test[l].blue_received)
                    test[l].blue_received[j] = 1.03f + j;
            }

            hr = D3DXSHEvalConeLight(order, &dir, test[l].radius, 1.7f, 2.6f, 3.5f, test[l].red_received, test[l].green_received, test[l].blue_received);
            ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);

            for (j = 0; j < 49; j++)
            {
                if (j >= order * order)
                    expected = j + test[l].roffset;
                else
                    expected = test[l].red_expected[j];
                equal = compare_float(test[l].red_received[j], expected, 128);
                ok(equal, "Red: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                        l, order, j, expected, test[l].red_received[j]);

                if (test[l].green_received)
                {
                    if (j >= order * order)
                        expected = j + test[l].goffset;
                    else
                        expected = test[l].green_expected[j];
                    equal = compare_float(test[l].green_received[j], expected, 64);
                    ok(equal, "Green: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, test[l].green_received[j]);
                }

                if (test[l].blue_received)
                {
                    if (j >= order * order)
                        expected = j + test[l].boffset;
                    else
                        expected = test[l].blue_expected[j];
                    equal = compare_float(test[l].blue_received[j], expected, 128);
                    ok(equal, "Blue: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, test[l].blue_received[j]);
                }
            }
        }
    }

    /* Cone light with radius <= 0.0f behaves as a directional light */
    for (order = D3DXSH_MINORDER; order <= D3DXSH_MAXORDER; order++)
    {
        FLOAT blue[49], green[49], red[49];

        for (j = 0; j < 49; j++)
        {
            rout[j] = 1.01f + j;
            gout[j] = 1.02f + j;
            bout[j] = 1.03f + j;
            red[j] = 1.01f + j;
            green[j] = 1.02f + j;
            blue[j] = 1.03f + j;
        }

        hr = D3DXSHEvalConeLight(order, &dir, -0.1f, 1.7f, 2.6f, 3.5f, rout, gout, bout);
        ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
        D3DXSHEvalDirectionalLight(order, &dir, 1.7f, 2.6f, 3.5f, red, green, blue);

        for (j = 0; j < 49; j++)
        {
            equal = compare_float(red[j], rout[j], 0);
            ok(equal, "Red: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                    l, order, j, red[j], rout[j]);

            equal = compare_float(green[j], gout[j], 0);
            ok(equal, "Green: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                    l, order, j, green[j], gout[j]);

            equal = compare_float(blue[j], bout[j], 0);
            ok(equal, "Blue: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                    l, order, j, blue[j], bout[j]);
        }
    }

    /* D3DXSHEvalConeLight accepts order < D3DXSH_MINORDER or order > D3DXSH_MAXORDER. But tests in native windows show that the colour outputs are not set */
    hr = D3DXSHEvalConeLight(7, &dir, 0.5f, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
    hr = D3DXSHEvalConeLight(0, &dir, 0.5f, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
    hr = D3DXSHEvalConeLight(1, &dir, 0.5f, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
}

static void test_D3DXSHEvalDirection(void)
{
    float a[49], expected, *received_ptr;
    unsigned int i, order;
    D3DXVECTOR3 d;
    BOOL equal;

    static const float table[36] =
    {
         2.82094806e-01f, -9.77205038e-01f,  1.46580756e+00f, -4.88602519e-01f,  2.18509698e+00f, -6.55529118e+00f,
         8.20018101e+00f, -3.27764559e-00f, -1.63882279e+00f,  1.18008721e+00f,  1.73436680e+01f, -4.02200317e+01f,
         4.70202179e+01f, -2.01100159e+01f, -1.30077515e+01f,  6.49047947e+00f, -1.50200577e+01f,  1.06207848e+01f,
         1.17325661e+02f, -2.40856750e+02f,  2.71657288e+02f, -1.20428375e+02f, -8.79942474e+01f,  5.84143143e+01f,
        -4.38084984e+00f,  2.49425201e+01f, -1.49447693e+02f,  7.82781296e+01f,  7.47791748e+02f, -1.42768787e+03f,
         1.57461914e+03f, -7.13843933e+02f, -5.60843811e+02f,  4.30529724e+02f, -4.35889091e+01f, -2.69116650e+01f,
    };

    d.x = 1.0; d.y = 2.0f; d.z = 3.0f;

    for (order = 0; order <= D3DXSH_MAXORDER + 1; order++)
    {
        for (i = 0; i < ARRAY_SIZE(a); ++i)
            a[i] = 1.5f + i;

        received_ptr = D3DXSHEvalDirection(a, order, &d);
        ok(received_ptr == a, "Expected %p, received %p\n", a, received_ptr);

        for (i = 0; i < ARRAY_SIZE(a); ++i)
        {
            /* if the order is < D3DXSH_MINORDER or order > D3DXSH_MAXORDER or
             * the index of the element is greater than order * order - 1,
             * D3DXSHEvalDirection() does not modify the output. */
            if ((order < D3DXSH_MINORDER) || (order > D3DXSH_MAXORDER) || (i >= order * order))
                expected = 1.5f + i;
            else
                expected = table[i];

            equal = compare_float(a[i], expected, 2);
            ok(equal, "order %u, index %u: Got unexpected result %.8e, expected %.8e.\n",
                    order, i, a[i], expected);
        }
    }
}

static void test_D3DXSHEvalDirectionalLight(void)
{
    float *blue_out, bout[49], expected, gout[49], *green_out, *red_out, rout[49];
    unsigned int j, l, order, startindex;
    D3DXVECTOR3 dir;
    HRESULT hr;
    BOOL equal;

    static const float table[] =
    {
    /* Red colour */
      2.008781f, -4.175174f, 9.602900f, -3.827243f, 1.417963f, -2.947181f,
      6.778517f, -2.701583f, 7.249108f, -18.188671f, 34.643921f, -16.672949f,
      -0.631551f, 1.417963f, -2.947181f, 6.778517f, -2.701583f, 7.249108f,
      -18.188671f, 34.643921f, -16.672949f, -0.631551f, -7.794341f, 52.934967f,
      -102.245529f, 181.656815f, -93.725060f, -4.611760f, 10.146287f, 1.555186f,
      -3.232392f, 7.434503f, -2.963026f, 7.950634f, -19.948866f, 37.996559f,
      -18.286459f, -0.692669f, -8.548632f, 58.057705f, -112.140251f, 199.236496f,
      -102.795227f, -5.058059f, 11.128186f, -4.189955f, -70.782669f, 360.268829f,
      -614.755005f, 1052.926270f, -563.525391f, -31.387066f, 92.141365f, -23.864176f,
      1.555186f, -3.232392f, 7.434503f, -2.963026f, 7.950634f, -19.948866f,
      37.996559f, -18.286459f, -0.692669f, -8.548632f, 58.057705f, -112.140251f,
      199.236496f, -102.795227f, -5.058059f, 11.128186f, -4.189955f, -70.782669f,
      360.268829f, -614.755005f, 1052.926270f, -563.525391f, -31.387066f, 92.141365f,
      -23.864176f, 34.868664f, -38.354366f, -478.864166f, 2103.939941f, -3334.927734f,
      5583.213867f, -3057.017090f, -183.297836f, 623.361633f, -218.449921f, 22.258503f,
    /* Green colour */
      3.072254f, -6.385560f, 14.686787f, -5.853429f, 2.168650f, -4.507453f,
      10.367143f, -4.131832f, 11.086870f, -27.817965f, 52.984818f, -25.499800f,
      -0.965902f, 2.168650f, -4.507453f, 10.367143f, -4.131832f, 11.086870f,
      -27.817965f, 52.984818f, -25.499800f, -0.965902f, -11.920755f, 80.959351f,
      -156.375488f, 277.828033f, -143.344193f, -7.053278f, 15.517849f, 2.378519f,
      -4.943659f, 11.370415f, -4.531687f, 12.159794f, -30.510029f, 58.112385f,
      -27.967525f, -1.059376f, -13.074378f, 88.794136f, -171.508621f, 304.714630f,
      -157.216217f, -7.735855f, 17.019577f, -6.408166f, -108.255844f, 550.999390f,
      -940.213501f, 1610.357788f, -861.862305f, -48.003746f, 140.922089f, -36.498150f,
      2.378519f, -4.943659f, 11.370415f, -4.531687f, 12.159794f, -30.510029f,
      58.112385f, -27.967525f, -1.059376f, -13.074378f, 88.794136f, -171.508621f,
      304.714630f, -157.216217f, -7.735855f, 17.019577f, -6.408166f, -108.255844f,
      550.999390f, -940.213501f, 1610.357788f, -861.862305f, -48.003746f, 140.922089f,
      -36.498150f, 53.328545f, -58.659618f, -732.380493f, 3217.790283f, -5100.477539f,
      8539.033203f, -4675.437500f, -280.337860f, 953.376587f, -334.099884f, 34.042416f,
    /* Blue colour */
      4.135726f, -8.595945f, 19.770674f, -7.879617f, 2.919336f, -6.067726f,
      13.955770f, -5.562082f, 14.924634f, -37.447262f, 71.325722f, -34.326656f,
      -1.300252f, 2.919336f, -6.067726f, 13.955770f, -5.562082f, 14.924634f,
      -37.447262f, 71.325722f, -34.326656f, -1.300252f, -16.047173f, 108.983749f,
      -210.505493f, 373.999298f, -192.963348f, -9.494799f, 20.889414f, 3.201853f,
      -6.654925f, 15.306328f, -6.100348f, 16.368954f, -41.071194f, 78.228210f,
      -37.648590f, -1.426083f, -17.600124f, 119.530563f, -230.876984f, 410.192780f,
      -211.637222f, -10.413651f, 22.910971f, -8.626378f, -145.729019f, 741.729919f,
      -1265.671997f, 2167.789307f, -1160.199219f, -64.620430f, 189.702820f, -49.132126f,
      3.201853f, -6.654925f, 15.306328f, -6.100348f, 16.368954f, -41.071194f,
      78.228210f, -37.648590f, -1.426083f, -17.600124f, 119.530563f, -230.876984f,
      410.192780f, -211.637222f, -10.413651f, 22.910971f, -8.626378f, -145.729019f,
      741.729919f, -1265.671997f, 2167.789307f, -1160.199219f, -64.620430f, 189.702820f,
      -49.132126f, 71.788422f, -78.964867f, -985.896790f, 4331.640625f, -6866.027344f,
      11494.852539f, -6293.858398f, -377.377899f, 1283.391479f, -449.749817f, 45.826328f, };
    const struct
    {
        float *red_in, *green_in, *blue_in;
        const float *red_out, *green_out, *blue_out;
        float roffset, goffset, boffset;
    }
    test[] =
    {
      { rout, gout, bout, table, &table[90], &table[180], 1.01f, 1.02f, 1.03f, },
      { rout, rout, rout, &table[180], &table[180], &table[180], 1.03f, 1.03f, 1.03f, },
      { rout, rout, bout, &table[90], &table[90], &table[180], 1.02f, 1.02f, 1.03f, },
      { rout, gout, gout, table, &table[180], &table[180], 1.01f, 1.03f, 1.03f, },
      { rout, gout, rout, &table[180], &table[90], &table[180], 1.03f, 1.02f, 1.03f, },
    /* D3DXSHEvalDirectionaLight accepts NULL green or blue colour. */
      { rout, NULL, bout, table, NULL, &table[180], 1.01f, 0.0f, 1.03f, },
      { rout, gout, NULL, table, &table[90], NULL, 1.01f, 1.02f, 0.0f, },
      { rout, NULL, NULL, table, NULL, NULL, 1.01f, 0.0f, 0.0f, },
    };

    dir.x = 1.1f; dir.y= 1.2f; dir.z = 2.76f;

    for (l = 0; l < ARRAY_SIZE(test); ++l)
    {
        startindex = 0;

        for (order = D3DXSH_MINORDER; order <= D3DXSH_MAXORDER; order++)
        {
            red_out = test[l].red_in;
            green_out = test[l].green_in;
            blue_out = test[l].blue_in;

            for (j = 0; j < ARRAY_SIZE(rout); ++j)
            {
                red_out[j] = 1.01f + j;
                if ( green_out )
                    green_out[j] = 1.02f + j;
                if ( blue_out )
                    blue_out[j] = 1.03f + j;
            }

            hr = D3DXSHEvalDirectionalLight(order, &dir, 1.7f, 2.6f, 3.5f, red_out, green_out, blue_out);
            ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);

            for (j = 0; j < ARRAY_SIZE(rout); ++j)
            {
                if ( j >= order * order )
                    expected = j + test[l].roffset;
                else
                    expected = test[l].red_out[startindex + j];
                equal = compare_float(expected, red_out[j], 8);
                ok(equal, "Red: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                        l, order, j, expected, red_out[j]);

                if ( green_out )
                {
                    if ( j >= order * order )
                        expected = j + test[l].goffset;
                    else
                        expected = test[l].green_out[startindex + j];
                    equal = compare_float(expected, green_out[j], 8);
                    ok(equal, "Green: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, green_out[j]);
                }

                if ( blue_out )
                {
                    if ( j >= order * order )
                        expected = j + test[l].boffset;
                    else
                        expected = test[l].blue_out[startindex + j];
                    equal = compare_float(expected, blue_out[j], 4);
                    ok(equal, "Blue: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, blue_out[j]);
                }
            }

            startindex += order * order;
        }
    }

    /* D3DXSHEvalDirectionalLight accepts order < D3DXSH_MINORDER or order > D3DXSH_MAXORDER. But tests in native windows show that the colour outputs are not set*/
    hr = D3DXSHEvalDirectionalLight(7, &dir, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
    hr = D3DXSHEvalDirectionalLight(0, &dir, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
    hr = D3DXSHEvalDirectionalLight(1, &dir, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
}

static void test_D3DXSHEvalHemisphereLight(void)
{
    float bout[49], expected, gout[49], rout[49];
    unsigned int j, l, order;
    D3DXCOLOR bottom, top;
    D3DXVECTOR3 dir;
    HRESULT hr;
    BOOL equal;

    static const float table[] =
    {
        /* Red colour. */
        23.422981f, 15.859521f, -36.476898f, 14.537894f,
        /* Green colour. */
        19.966694f,  6.096982f, -14.023058f,  5.588900f,
        /* Blue colour. */
        24.566214f,  8.546826f, -19.657701f,  7.834591f,
    };
    const struct
    {
        float *red_received, *green_received, *blue_received;
        const float *red_expected, *green_expected, *blue_expected;
        const float roffset, goffset, boffset;
    }
    test[] =
    {
        { rout, gout, bout, table, &table[4], &table[8], 1.01f, 1.02f, 1.03f, },
        { rout, rout, rout, &table[8], &table[8], &table[8], 1.03f, 1.03f, 1.03f, },
        { rout, rout, bout, &table[4], &table[4], &table[8], 1.02f, 1.02f, 1.03f, },
        { rout, gout, gout, table, &table[8], &table[8], 1.01f, 1.03f, 1.03f, },
        { rout, gout, rout, &table[8], &table[4], &table[8], 1.03f, 1.02f, 1.03f, },
    /* D3DXSHEvalHemisphereLight accepts NULL green or blue colour. */
        { rout, NULL, bout, table, NULL, &table[8], 1.01f, 1.02f, 1.03f, },
        { rout, gout, NULL, table, &table[4], NULL, 1.01f, 1.02f, 1.03f, },
        { rout, NULL, NULL, table, NULL, NULL, 1.01f, 1.02f, 1.03f, },
    };

    dir.x = 1.1f; dir.y = 1.2f; dir.z = 2.76f;
    top.r = 0.1f; top.g = 2.1f; top.b = 2.3f; top.a = 4.3f;
    bottom.r = 8.71f; bottom.g = 5.41f; bottom.b = 6.94f; bottom.a = 8.43f;

    for (l = 0; l < ARRAY_SIZE(test); ++l)
        for (order = D3DXSH_MINORDER; order <= D3DXSH_MAXORDER + 1; order++)
        {
            for (j = 0; j < 49; j++)
            {
                test[l].red_received[j] = 1.01f + j;
                if (test[l].green_received)
                    test[l].green_received[j] = 1.02f + j;
                if (test[l].blue_received)
                    test[l].blue_received[j] = 1.03f + j;
            }

            hr = D3DXSHEvalHemisphereLight(order, &dir, top, bottom, test[l].red_received, test[l].green_received, test[l].blue_received);
            ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);

            for (j = 0; j < 49; j++)
            {
                if (j < 4)
                    expected = test[l].red_expected[j];
                else if (j < order * order)
                     expected = 0.0f;
                else
                    expected = test[l].roffset + j;
                equal = compare_float(test[l].red_received[j], expected, 4);
                ok(equal, "Red: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                        l, order, j, expected, test[l].red_received[j]);

                if (test[l].green_received)
                {
                    if (j < 4)
                        expected = test[l].green_expected[j];
                    else if (j < order * order)
                         expected = 0.0f;
                    else
                         expected = test[l].goffset + j;
                    equal = compare_float(expected, test[l].green_received[j], 4);
                    ok(equal, "Green: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, test[l].green_received[j]);
                }

                if (test[l].blue_received)
                {
                    if (j < 4)
                        expected = test[l].blue_expected[j];
                    else if (j < order * order)
                        expected = 0.0f;
                    else
                        expected = test[l].boffset + j;
                    equal = compare_float(expected, test[l].blue_received[j], 4);
                    ok(equal, "Blue: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, test[l].blue_received[j]);
                }
            }
        }
}

static void test_D3DXSHEvalSphericalLight(void)
{
    float bout[49], expected, gout[49], rout[49];
    unsigned int j, l, order;
    D3DXVECTOR3 dir;
    HRESULT hr;
    BOOL equal;

    static const float table[] =
    {
        /* Red colour. */
         3.01317163e+00f, -9.77240128e-01f,  2.24765220e+00f, -8.95803434e-01f,  3.25255224e-35f, -8.16094904e-35f,
         8.95199460e-35f, -7.48086982e-35f, -2.83366352e-36f,  6.29281376e-02f, -4.27374053e-01f,  6.19212543e-01f,
        -3.04508915e-01f,  5.67611487e-01f,  3.72333533e-02f, -8.19167317e-02f,  1.25205729e-36f,  2.11515287e-35f,
        -8.85884025e-35f,  8.22100105e-35f, -1.41290744e-37f,  7.53591749e-35f,  7.71793061e-36f, -2.75340121e-35f,
         7.13117824e-36f,  1.24992691e-02f, -1.37487792e-02f, -1.48109290e-01f,  4.34345843e-01f, -2.45986100e-01f,
        -1.51757946e-01f, -2.25487254e-01f, -3.78407442e-02f,  1.92801335e-01f, -7.83071154e-02f,  7.97894137e-03f,

         4.02519645e-01f, -2.43653315e-01f,  5.60402600e-01f, -2.23348868e-01f,  1.62046875e-01f, -4.06590330e-01f,
         4.46001368e-01f, -3.72707796e-01f, -1.41177231e-02f, -4.31995198e-02f,  2.93387896e-01f, -4.25083048e-01f,
         2.09042241e-01f, -3.89659453e-01f, -2.55603144e-02f,  5.62349945e-02f, -4.68822967e-03f, -7.92002290e-02f,
         3.31712278e-01f, -3.07828893e-01f,  5.29052032e-04f, -2.82176480e-01f, -2.88991817e-02f,  1.03098934e-01f,
        -2.67021338e-02f,  7.24339502e-03f, -7.96749298e-03f, -8.58301461e-02f,  2.51705799e-01f, -1.42550295e-01f,
        -8.79445626e-02f, -1.30671101e-01f, -2.19289189e-02f,  1.11729432e-01f, -4.53794030e-02f,  4.62384030e-03f,

         1.95445306e+00f, -8.56593659e-01f,  1.97016533e+00f, -7.85210840e-01f,  2.31033385e-01f, -5.79683751e-01f,
         6.35872835e-01f, -5.31376762e-01f, -2.01279127e-02f,  2.11104646e-02f, -1.43370917e-01f,  2.07726860e-01f,
        -1.02153423e-01f,  1.90416285e-01f,  1.24906507e-02f, -2.74805568e-02f,  6.33162467e-03f,  1.06962790e-01f,
        -4.47989495e-01f,  4.15734115e-01f, -7.14504011e-04f,  3.81089599e-01f,  3.90293960e-02f, -1.39238860e-01f,
         3.60622028e-02f, -4.47359268e-03f,  4.92080277e-03f,  5.30095505e-02f, -1.55456001e-01f,  8.80404774e-02f,
         5.43154350e-02f,  8.07037695e-02f,  1.35435180e-02f, -6.90052063e-02f,  2.80267699e-02f, -2.85572968e-03f,
        /* Green colour. */
         4.60837984e+00f, -1.49460245e+00f,  3.43758549e+00f, -1.37005222e+00f,  4.97449134e-35f, -1.24814507e-34f,
         1.36912850e-34f, -1.14413296e-34f, -4.33383805e-36f,  9.62430278e-02f, -6.53630863e-01f,  9.47030887e-01f,
        -4.65719486e-01f,  8.68111630e-01f,  5.69451249e-02f, -1.25284405e-01f,  1.91491103e-36f,  3.23493947e-35f,
        -1.35488136e-34f,  1.25732949e-34f, -2.16091711e-37f,  1.15255201e-34f,  1.18038931e-35f, -4.21108392e-35f,
         1.09065072e-35f,  1.91165280e-02f, -2.10275433e-02f, -2.26520076e-01f,  6.64293599e-01f, -3.76214011e-01f,
        -2.32100374e-01f, -3.44862837e-01f, -5.78740756e-02f,  2.94872611e-01f, -1.19763816e-01f,  1.22030860e-02f,

         6.15618240e-01f, -3.72646222e-01f,  8.57086273e-01f, -3.41592364e-01f,  2.47836381e-01f, -6.21843994e-01f,
         6.82119695e-01f, -5.70023651e-01f, -2.15918104e-02f, -6.60698496e-02f,  4.48710870e-01f, -6.50126972e-01f,
         3.19711642e-01f, -5.95949713e-01f, -3.90922430e-02f,  8.60064566e-02f, -7.17023314e-03f, -1.21129754e-01f,
         5.07324627e-01f, -4.70797100e-01f,  8.09138350e-04f, -4.31564000e-01f, -4.41987457e-02f,  1.57680712e-01f,
        -4.08385549e-02f,  1.10781328e-02f, -1.21855767e-02f, -1.31269627e-01f,  3.84961785e-01f, -2.18018084e-01f,
        -1.34503440e-01f, -1.99849906e-01f, -3.35383443e-02f,  1.70880296e-01f, -6.94037884e-02f,  7.07175529e-03f,

         2.98916331e+00f, -1.31008433e+00f,  3.01319384e+00f, -1.20091062e+00f,  3.53345154e-01f, -8.86575090e-01f,
         9.72511332e-01f, -8.12693818e-01f, -3.07838645e-02f,  3.22865908e-02f, -2.19273153e-01f,  3.17699883e-01f,
        -1.56234637e-01f,  2.91224888e-01f,  1.91033469e-02f, -4.20290842e-02f,  9.68366064e-03f,  1.63590138e-01f,
        -6.85160360e-01f,  6.35828606e-01f, -1.09277077e-03f,  5.82842878e-01f,  5.96920135e-02f, -2.12953537e-01f,
         5.51539537e-02f, -6.84196484e-03f,  7.52593316e-03f,  8.10734249e-02f, -2.37756221e-01f,  1.34650133e-01f,
         8.30706600e-02f,  1.23429287e-01f,  2.07136145e-02f, -1.05537368e-01f,  4.28644688e-02f, -4.36758628e-03f,
        /* Blue colour. */
         6.20358848e+00f, -2.01196491e+00f,  4.62751910e+00f, -1.84430114e+00f,  6.69643089e-35f, -1.68019534e-34f,
         1.84305766e-34f, -1.54017904e-34f, -5.83401297e-36f,  1.29557927e-01f, -8.79887732e-01f,  1.27484932e+00f,
        -6.26930101e-01f,  1.16861185e+00f,  7.66569017e-02f, -1.68652090e-01f,  2.57776494e-36f,  4.35472637e-35f,
        -1.82387882e-34f,  1.69255899e-34f, -2.90892699e-37f,  1.55151238e-34f,  1.58898567e-35f, -5.66876703e-35f,
         1.46818371e-35f,  2.57337886e-02f, -2.83063093e-02f, -3.04930882e-01f,  8.94241416e-01f, -5.06441957e-01f,
        -3.12442822e-01f, -4.64238452e-01f, -7.79074123e-02f,  3.96943914e-01f, -1.61220527e-01f,  1.64272318e-02f,

         8.28716892e-01f, -5.01639163e-01f,  1.15377003e+00f, -4.59835891e-01f,  3.33625909e-01f, -8.37097715e-01f,
         9.18238085e-01f, -7.67339558e-01f, -2.90658997e-02f, -8.89401854e-02f,  6.04033886e-01f, -8.75170956e-01f,
         4.30381072e-01f, -8.02240028e-01f, -5.26241753e-02f,  1.15777927e-01f, -9.65223728e-03f, -1.63059290e-01f,
         6.82937023e-01f, -6.33765350e-01f,  1.08922474e-03f, -5.80951560e-01f, -5.94983136e-02f,  2.12262505e-01f,
        -5.49749798e-02f,  1.49128717e-02f, -1.64036616e-02f, -1.76709119e-01f,  5.18217807e-01f, -2.93485893e-01f,
        -1.81062330e-01f, -2.69028730e-01f, -4.51477729e-02f,  2.30031176e-01f, -9.34281801e-02f,  9.51967094e-03f,

         4.02387383e+00f, -1.76357513e+00f,  4.05622263e+00f, -1.61661051e+00f,  4.75656955e-01f, -1.19346651e+00f,
         1.30914992e+00f, -1.09401095e+00f, -4.14398191e-02f,  4.34627200e-02f, -2.95175409e-01f,  4.27672935e-01f,
        -2.10315865e-01f,  3.92033517e-01f,  2.57160448e-02f, -5.65776154e-02f,  1.30356975e-02f,  2.20217502e-01f,
        -9.22331288e-01f,  8.55923154e-01f, -1.47103763e-03f,  7.84596211e-01f,  8.03546365e-02f, -2.86668233e-01f,
         7.42457096e-02f, -9.21033762e-03f,  1.01310642e-02f,  1.09137307e-01f, -3.20056463e-01f,  1.81259801e-01f,
         1.11825893e-01f,  1.66154815e-01f,  2.78837128e-02f, -1.42069538e-01f,  5.77021717e-02f, -5.87944329e-03f,
    };
    const struct
    {
        float *red_received, *green_received, *blue_received;
        const float *red_expected, *green_expected, *blue_expected;
        float radius, roffset, goffset, boffset;
    }
    test[] =
    {
        { rout, gout, bout, table, &table[108], &table[216], 17.4f, 1.01f, 1.02f, 1.03f, },
        { rout, gout, bout, &table[36], &table[144], &table[252], 1.6f, 1.01f, 1.02f, 1.03f, },
        { rout, gout, bout, &table[72], &table[180], &table[288], -3.0f, 1.01f, 1.02f, 1.03f, },
        { rout, rout, rout, &table[216], &table[216], &table[216], 17.4f, 1.03f, 1.03f, 1.03f, },
        { rout, rout, bout, &table[108], &table[108], &table[216], 17.4, 1.02f, 1.02f, 1.03f, },
        { rout, gout, gout, table, &table[216], &table[216], 17.4f, 1.01f, 1.03f, 1.03f, },
        { rout, gout, rout, &table[216], &table[108], &table[216], 17.4f, 1.03f, 1.02f, 1.03f, },
    /* D3DXSHEvalSphericalLight accepts NULL green or blue colour. */
        { rout, NULL, bout, table, NULL, &table[216], 17.4f, 1.01f, 0.0f, 1.03f, },
        { rout, gout, NULL, table, &table[108], NULL, 17.4f, 1.01f, 1.02f, 0.0f, },
        { rout, NULL, NULL, table, NULL, NULL, 17.4f, 1.01f, 0.0f, 0.0f, },
    };

    dir.x = 1.1f; dir.y = 1.2f; dir.z = 2.76f;

    for (l = 0; l < ARRAY_SIZE(test); ++l)
    {
        for (order = D3DXSH_MINORDER; order <= D3DXSH_MAXORDER; order++)
        {
            for (j = 0; j < 49; j++)
            {
                test[l].red_received[j] = 1.01f + j;
                if (test[l].green_received)
                    test[l].green_received[j] = 1.02f + j;
                if (test[l].blue_received)
                    test[l].blue_received[j] = 1.03f + j;
            }

            hr = D3DXSHEvalSphericalLight(order, &dir, test[l].radius, 1.7f, 2.6f, 3.5f, test[l].red_received, test[l].green_received, test[l].blue_received);
            ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);

            for (j = 0; j < 49; j++)
            {
                if (j >= order * order)
                    expected = j + test[l].roffset;
                else
                    expected = test[l].red_expected[j];
                equal = compare_float(expected, test[l].red_received[j], 4096);
                ok(equal || (fabs(expected) < 1.0e-6f && fabs(test[l].red_received[j]) < 1.0e-6f),
                        "Red: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                        l, order, j, expected, test[l].red_received[j]);

                if (test[l].green_received)
                {
                    if (j >= order * order)
                        expected = j + test[l].goffset;
                    else
                        expected = test[l].green_expected[j];
                    equal = compare_float(expected, test[l].green_received[j], 4096);
                    ok(equal || (fabs(expected) < 1.0e-6f && fabs(test[l].green_received[j]) < 1.0e-6f),
                            "Green: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, test[l].green_received[j]);
                }

                if (test[l].blue_received)
                {
                    if (j >= order * order)
                        expected = j + test[l].boffset;
                    else
                        expected = test[l].blue_expected[j];
                    equal = compare_float(expected, test[l].blue_received[j], 4096);
                    ok(equal || (fabs(expected) < 1.0e-6f && fabs(test[l].blue_received[j]) < 1.0e-6f),
                            "Blue: case %u, order %u: expected[%u] = %.8e, received %.8e.\n",
                            l, order, j, expected, test[l].blue_received[j]);
                }
            }
        }
    }

    /* D3DXSHEvalSphericalLight accepts order < D3DXSH_MINORDER or order > D3DXSH_MAXORDER. But tests in native windows show that the colour outputs are not set */
    hr = D3DXSHEvalSphericalLight(7, &dir, 17.4f, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
    hr = D3DXSHEvalSphericalLight(0, &dir, 17.4f, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
    hr = D3DXSHEvalSphericalLight(1, &dir, 17.4f, 1.0f, 2.0f, 3.0f, rout, gout, bout);
    ok(hr == D3D_OK, "Expected %#lx, got %#lx\n", D3D_OK, hr);
}

static void test_D3DXSHMultiply2(void)
{
    float a[20], b[20], c[20];
    unsigned int i;
    BOOL equal;

    /* D3DXSHMultiply2() only modifies the first 4 elements of the array. */
    static const float expected[20] =
    {
         3.41859412f,  1.69821072f,  1.70385253f,  1.70949447f,  4.0f,  5.0f,  6.0f,
                7.0f,         8.0f,         9.0f,        10.0f, 11.0f, 12.0f, 13.0f,
               14.0f,        15.0f,        16.0f,        17.0f, 18.0f, 19.0f,
    };

    for (i = 0; i < ARRAY_SIZE(a); ++i)
    {
        a[i] = 1.0f + i / 100.0f;
        b[i] = 3.0f - i / 100.0f;
        c[i] = i;
    }

    D3DXSHMultiply2(c, a, b);
    for (i = 0; i < ARRAY_SIZE(expected); ++i)
    {
        equal = compare_float(c[i], expected[i], 2);
        ok(equal, "Expected[%u] = %.8e, received = %.8e.\n", i, expected[i], c[i]);
    }
}

static void test_D3DXSHMultiply3(void)
{
    float a[20], b[20], c[20];
    unsigned int i;
    BOOL equal;

    /* D3DXSHMultiply3() only modifies the first 9 elements of the array. */
    static const float expected[20] =
    {
        7.81391382e+00f, 2.25605774e+00f, 5.94839954e+00f, 4.97089481e+00f, 2.89985824e+00f, 3.59894633e+00f,
        1.72657156e+00f, 5.57353783e+00f, 6.22063160e-01f, 9.00000000e+00f, 1.00000000e+01f, 1.10000000e+01f,
        1.20000000e+01f, 1.30000000e+01f, 1.40000000e+01f, 1.50000000e+01f, 1.60000000e+01f, 1.70000000e+01f,
        1.80000000e+01f, 1.90000000e+01f,
    };
    static const float expected_aliased[20] =
    {
        4.54092499e+02f, 2.12640405e+00f, 5.57040071e+00f, 1.53303785e+01f, 2.27960873e+01f, 4.36041260e+01f,
        4.27384138e+00f, 1.75772034e+02f, 2.37672729e+02f, 1.09000003e+00f, 1.10000002e+00f, 1.11000001e+00f,
        1.12000000e+00f, 1.13000000e+00f, 1.13999999e+00f, 1.14999998e+00f, 1.15999997e+00f, 1.16999996e+00f,
        1.17999995e+00f, 1.19000006e+00f,
    };

    for (i = 0; i < ARRAY_SIZE(a); ++i)
    {
        a[i] = 1.0f + i / 100.0f;
        b[i] = 3.0f - i / 100.0f;
        c[i] = i;
    }

    D3DXSHMultiply3(c, a, b);
    for (i = 0; i < ARRAY_SIZE(expected); ++i)
    {
        equal = compare_float(c[i], expected[i], 4);
        ok(equal, "Expected[%u] = %.8e, received = %.8e.\n", i, expected[i], c[i]);
    }

    memcpy(c, a, sizeof(c));
    D3DXSHMultiply3(c, c, b);
    for (i = 0; i < ARRAY_SIZE(expected_aliased); ++i)
    {
        equal = compare_float(c[i], expected_aliased[i], 64);
        ok(equal, "Expected[%u] = %.8e, received = %.8e.\n", i, expected_aliased[i], c[i]);
    }
}

static void test_D3DXSHMultiply4(void)
{
    float a[20], b[20], c[20];
    unsigned int i;
    BOOL equal;

    /* D3DXSHMultiply4() only modifies the first 16 elements of the array. */
    static const float expected[] =
    {
        /* c, a, b */
         1.41825991e+01f,  2.61570334e+00f,  1.28286009e+01f,  9.82059574e+00f,  3.03969646e+00f,  4.53044176e+00f,
         5.82058382e+00f,  1.22498465e+01f,  2.19434643e+00f,  3.90015244e+00f,  5.41660881e+00f,  5.60181284e+00f,
         9.59981740e-01f,  7.03754997e+00f,  3.62523031e+00f,  4.63601470e-01f,  1.60000000e+01f,  1.70000000e+01f,
         1.80000000e+01f,  1.90000000e+01f,
        /* c, c, b */
        -2.11441266e+05f, -2.52915771e+03f, -1.00233936e+04f, -4.41277191e+02f, -1.63994385e+02f, -5.26305115e+02f,
         2.96361875e+04f, -3.93183081e+03f, -1.35771113e+04f, -3.97897388e+03f, -1.03303418e+04f, -1.37797871e+04f,
        -1.66851094e+04f, -4.49813750e+04f, -7.32697422e+04f, -9.52373359e+04f,  1.60000000e+01f,  1.70000000e+01f,
         1.80000000e+01f,  1.90000000e+01f,
        /* c, c, c */
         2.36682415e-01f, -7.17648506e-01f, -1.80499524e-01f, -7.71235973e-02f,  1.44830629e-01f,  5.73285699e-01f,
        -3.37959230e-01f,  5.56938872e-02f, -4.42100227e-01f,  1.47701755e-01f, -5.51566519e-02f,  8.43374059e-02f,
         1.79876596e-01f,  9.09878965e-03f,  2.32199892e-01f,  7.41420984e-02f,  1.60000002e+00f,  1.70000005e+00f,
         1.80000007e+00f,  1.89999998e+00f,
    };

    for (i = 0; i < ARRAY_SIZE(a); ++i)
    {
        a[i] = 1.0f + i / 100.0f;
        b[i] = 3.0f - i / 100.0f;
        c[i] = i;
    }

    D3DXSHMultiply4(c, a, b);
    for (i = 0; i < ARRAY_SIZE(c); ++i)
    {
        equal = compare_float(c[i], expected[i], 16);
        ok(equal, "Expected[%u] = %.8e, received = %.8e.\n", i, expected[i], c[i]);
    }

    for (i = 0; i < ARRAY_SIZE(b); ++i)
    {
        b[i] = 3.0f - i / 100.0f;
        c[i] = i;
    }

    D3DXSHMultiply4(c, c, b);
    for (i = 0; i < ARRAY_SIZE(c); ++i)
    {
        equal = compare_float(c[i], expected[20 + i], 32);
        ok(equal, "Expected[%u] = %.8e, received = %.8e.\n", i, expected[20 + i], c[i]);
    }

    for (i = 0; i < ARRAY_SIZE(c); ++i)
        c[i] = 0.1f * i;

    D3DXSHMultiply4(c, c, c);
    for (i = 0; i < ARRAY_SIZE(c); ++i)
    {
        equal = compare_float(c[i], expected[40 + i], 8);
        ok(equal, "Expected[%u] = %.8e, received = %.8e.\n", i, expected[40 + i], c[i]);
    }
}

static void test_D3DXSHRotate(void)
{
    float expected, in[49], out[49], *out_temp, *received_ptr;
    unsigned int i, j, l, order;
    D3DXMATRIX m[4];
    BOOL equal;

    static const float table[]=
    {
        /* Rotation around the x-axis, π/2. */
         1.00999999e+00f, -3.00999999e+00f,  2.00999975e+00f,  4.01000023e+00f, -8.01000023e+00f, -6.00999928e+00f,
        -1.13078899e+01f,  5.00999975e+00f, -1.56583869e+00f,  1.09359801e+00f, -1.10099983e+01f,  1.98334141e+01f,
        -1.52681913e+01f, -1.90041180e+01f, -3.36488891e+00f, -9.56262684e+00f,  1.20996542e+01f, -2.72131383e-01f,
         3.02410126e+01f,  2.69199905e+01f,  3.92368774e+01f, -2.26324463e+01f,  6.70738792e+00f, -1.17682819e+01f,
         3.44367194e+00f, -6.07445812e+00f,  1.16183939e+01f,  1.52756083e+00f,  3.78963356e+01f, -5.69012184e+01f,
         4.74228935e+01f,  5.03915329e+01f,  1.06181908e+01f,  2.55010109e+01f,  4.92456071e-02f,  1.69833069e+01f,

         1.00999999e+00f, -3.00999999e+00f, -3.01000023e+00f,  4.01000023e+00f, -8.01000023e+00f, -6.00999928e+00f,
        -1.13078890e+01f, -8.01000023e+00f,  1.42979193e+01f,
        /* Rotation around the x-axis, -π/2. */
         1.00999999e+00f,  3.00999999e+00f, -2.01000023e+00f,  4.01000023e+00f,  8.01000023e+00f, -6.01000118e+00f,
        -1.13078880e+01f, -5.01000071e+00f, -1.56583774e+00f, -1.09359753e+00f, -1.10100021e+01f, -1.98334103e+01f,
         1.52681961e+01f, -1.90041142e+01f,  3.36489248e+00f, -9.56262398e+00f, -1.20996523e+01f, -2.72129118e-01f,
        -3.02410049e+01f,  2.69200020e+01f,  3.92368736e+01f,  2.26324520e+01f,  6.70738268e+00f,  1.17682877e+01f,
         3.44367099e+00f,  6.07445717e+00f,  1.16183996e+01f, -1.52756333e+00f,  3.78963509e+01f,  5.69011993e+01f,
        -4.74229126e+01f,  5.03915253e+01f, -1.06182041e+01f,  2.55009995e+01f, -4.92481887e-02f,  1.69833050e+01f,

         1.00999999e+00f,  3.00999999e+00f, -3.01000023e+00f,  4.01000023e+00f,  8.01000023e+00f, -6.01000118e+00f,
        -1.13078899e+01f, -8.01000023e+00f,  1.42979193e+01f,
        /* Yaw π/3, pitch π/4, roll π/5. */
         1.00999999e+00f,  4.94489908e+00f,  1.44230127e+00f,  1.62728095e+00f,  2.19220325e-01f,  1.05408239e+01f,
        -9.13690281e+00f,  2.76374960e+00f, -7.30904531e+00f, -5.87572050e+00f,  5.30312395e+00f, -8.68215370e+00f,
        -2.56833839e+01f,  1.68027866e+00f, -1.88083878e+01f,  7.65365601e+00f,  1.69391327e+01f, -1.73280182e+01f,
         1.46297951e+01f, -5.44671021e+01f, -1.22310352e+01f, -4.08985710e+00f, -9.44422245e+00f,  3.05603528e+00f,
         1.79257303e-01f, -1.00418749e+01f,  2.30900917e+01f, -2.31887093e+01f,  1.17270985e+01f, -6.51830902e+01f,
         4.86715775e+01f, -1.50732088e+01f,  3.87931709e+01f, -2.60395355e+01f,  6.19276857e+00f, -1.76722469e+01f,

         1.00999999e+00f,  4.94489908e+00f, -8.91142070e-01f,  4.60769463e+00f,  2.19218358e-01f,  1.07733250e+01f,
        -8.20476913e+00f,  1.35638294e+01f, -1.20077667e+01f,
        /* Rotation around the z-axis, π/6. */
         1.00999999e+00f,  3.74571109e+00f,  3.00999999e+00f,  2.46776199e+00f,  1.03078890e+01f,  9.20981312e+00f,
         7.01000023e+00f,  3.93186355e+00f,  1.66212186e-01f,  1.60099983e+01f,  1.85040417e+01f,  1.74059658e+01f,
         1.30100002e+01f,  6.12801647e+00f, -2.02994061e+00f, -1.00100012e+01f,  1.31542921e+01f,  2.40099964e+01f,
         2.94322453e+01f,  2.83341675e+01f,  2.10100021e+01f,  9.05622101e+00f, -4.95814323e+00f, -1.80100002e+01f,
        -2.72360935e+01f, -4.52033186e+00f,  1.68145428e+01f,  3.40099945e+01f,  4.30924950e+01f,  4.19944229e+01f,
         3.10100002e+01f,  1.27164707e+01f, -8.61839962e+00f, -2.80100021e+01f, -4.08963470e+01f, -4.41905708e+01f,

         1.00999999e+00f,  3.74571109e+00f,  3.00999999e+00f,  1.59990644e+00f,  1.03078890e+01f,  9.20981312e+00f,
         7.01000023e+00f,  2.33195710e+00f, -4.42189360e+00f,
    };

    D3DXMatrixRotationX(&m[0], -D3DX_PI / 2.0f);
    D3DXMatrixRotationX(&m[1], D3DX_PI / 2.0f);
    D3DXMatrixRotationYawPitchRoll(&m[2], D3DX_PI / 3.0f, D3DX_PI / 4.0f, D3DX_PI / 5.0f);
    D3DXMatrixRotationZ(&m[3], D3DX_PI / 6.0f);

    for (l = 0; l < 2; l++)
    {
        if (l == 0)
            out_temp = out;
        else
            out_temp = in;

        for (j = 0; j < ARRAY_SIZE(m); ++j)
        {
            for (order = 0; order <= D3DXSH_MAXORDER; order++)
            {
                for (i = 0; i < ARRAY_SIZE(out); ++i)
                {
                    out[i] = ( i + 1.0f ) * ( i + 1.0f );
                    in[i] = i + 1.01f;
                }

                received_ptr = D3DXSHRotate(out_temp, order, &m[j], in);
                ok(received_ptr == out_temp, "Order %u, expected %p, received %p.\n",
                        order, out, received_ptr);

                for (i = 0; i < ARRAY_SIZE(out); ++i)
                {
                    if ((i > 0) && ((i >= order * order) || (order > D3DXSH_MAXORDER)))
                    {
                        if (l == 0)
                            expected = ( i + 1.0f ) * ( i + 1.0f );
                        else
                            expected = i + 1.01f;
                    }
                    else if ((l == 0) || (order > 3))
                        expected = table[45 * j + i];
                    else
                        expected = table[45 * j + 36 +i];
                    equal = compare_float(out_temp[i], expected, 4096);
                    ok(equal, "Order %u index %u, expected %.8e, received %.8e.\n",
                            order, i, expected, out_temp[i]);
                }
            }
        }
    }
}

static void test_D3DXSHRotateZ(void)
{
    float expected, in[49], out[49], *out_temp, *received_ptr;
    unsigned int end, i, j, l, order, square;
    BOOL equal;

    static const float angle[] = {D3DX_PI / 3.0f, -D3DX_PI / 3.0f, 4.0f * D3DX_PI / 3.0f};
    static const float table[] =
    {
        /* Angle π/3. */
         1.00999999e+00f,  4.47776222e+00f,  3.00999999e+00f,  2.64288902e-01f,  5.29788828e+00f,  9.94186401e+00f,
         7.01000023e+00f, -1.19981313e+00f, -8.84378910e+00f, -1.00100021e+01f,  7.49403954e+00f,  1.81380157e+01f,
         1.30100002e+01f, -3.39596605e+00f, -1.70399418e+01f, -1.60099983e+01f, -3.01642971e+01f, -1.80100040e+01f,
         1.04222422e+01f,  2.90662193e+01f,  2.10100002e+01f, -6.32417059e+00f, -2.79681454e+01f, -2.40099983e+01f,
         2.22609901e+00f, -1.81805649e+01f, -4.38245506e+01f, -2.80100040e+01f,  1.40824928e+01f,  4.27264709e+01f,
         3.10100002e+01f, -9.98442554e+00f, -4.16283989e+01f, -3.40099945e+01f,  5.88635778e+00f,  4.05303307e+01f,

         1.00999999e+00f,  4.47776222e+00f,  0.00000000e+00f, -5.81678391e+00f,  5.29788828e+00f,  6.93686390e+00f,
         0.00000000e+00f, -9.01125050e+00f, -2.29405236e+00f, -1.00100021e+01f,  1.29990416e+01f,  1.21330166e+01f,
         0.00000000e+00f, -1.57612505e+01f, -5.62874842e+00f,  0.00000000e+00f, -3.01642971e+01f, -3.29017075e-06f,
         1.99272442e+01f,  1.90612202e+01f,  0.00000000e+00f, -2.47612514e+01f, -8.62874794e+00f,  0.00000000e+00f,
        -1.30615301e+01f, -1.81805649e+01f, -3.03195534e+01f, -4.66050415e-06f,  2.85874958e+01f,  2.77214737e+01f,
         0.00000000e+00f, -3.60112534e+01f, -1.23787460e+01f,  0.00000000e+00f, -1.31287584e+01f, -2.36172504e+01f,

         1.00999999e+00f,  3.97776222e+00f,  3.97776222e+00f,  1.11419535e+00f,  7.24579096e+00f,  1.05597591e+01f,
         1.05597591e+01f, -9.95159924e-01f, -4.67341393e-01f,  4.67339337e-01f,  1.27653713e+01f,  1.85157013e+01f,
         1.85157013e+01f, -1.79728663e+00f,  4.93915796e-01f, -4.93915856e-01f, -2.14123421e+01f,  2.14123383e+01f,
         9.22107220e+00f,  2.36717567e+01f,  2.36717567e+01f,  3.85019469e+00f, -2.04687271e+01f,  2.04687233e+01f,
        -1.06621027e+01f, -3.65166283e+01f, -1.20612450e+01f,  1.20612402e+01f,  2.25568752e+01f,  3.89999084e+01f,
         3.89999084e+01f, -3.48751247e-02f, -1.04279022e+01f,  1.04279003e+01f, -3.68382835e+01f, -2.76528034e+01f,
        /* Angle -π/3. */
         1.00999999e+00f, -2.46776247e+00f,  3.00999999e+00f,  3.74571109e+00f, -1.03078899e+01f, -3.93186426e+00f,
         7.01000023e+00f,  9.20981312e+00f, -1.66213632e-01f, -1.00099983e+01f, -1.85040436e+01f, -6.12801695e+00f,
         1.30100002e+01f,  1.74059658e+01f,  2.02993774e+00f, -1.60100021e+01f,  1.31543026e+01f, -1.80099964e+01f,
        -2.94322472e+01f, -9.05622101e+00f,  2.10100002e+01f,  2.83341694e+01f,  4.95813942e+00f, -2.40100021e+01f,
        -2.72360916e+01f,  4.41905823e+01f,  1.68145580e+01f, -2.80099964e+01f, -4.30924988e+01f, -1.27164736e+01f,
         3.10100002e+01f,  4.19944229e+01f,  8.61839294e+00f, -3.40100021e+01f, -4.08963470e+01f, -4.52030993e+00f,

         1.00999999e+00f, -2.46776247e+00f,  0.00000000e+00f, -3.20571756e+00f, -1.03078899e+01f, -6.93686390e+00f,
         0.00000000e+00f, -9.01125050e+00f, -4.46344614e+00f, -1.00099983e+01f, -1.29990416e+01f, -1.21330166e+01f,
         0.00000000e+00f, -1.57612505e+01f, -5.62874842e+00f,  0.00000000e+00f,  1.31543026e+01f,  3.29017075e-06f,
        -1.99272442e+01f, -1.90612202e+01f,  0.00000000e+00f, -2.47612514e+01f, -8.62874794e+00f,  0.00000000e+00f,
        -5.69598293e+00f,  4.41905823e+01f,  3.03195534e+01f,  4.66050415e-06f, -2.85874958e+01f, -2.77214737e+01f,
         0.00000000e+00f, -3.60112534e+01f, -1.23787460e+01f,  0.00000000e+00f, -1.31287584e+01f, -5.74052582e+01f,

         1.00999999e+00f, -2.96776223e+00f, -2.96776223e+00f, -6.09195352e-01f, -7.49829102e+00f, -1.06860094e+01f,
        -1.06860094e+01f, -1.18367157e+01f,  5.39078045e+00f, -5.39077854e+00f, -1.03036509e+01f, -1.72848415e+01f,
        -1.72848415e+01f, -1.75656433e+01f,  4.11427259e+00f, -4.11427307e+00f,  2.37164364e+01f, -2.37164326e+01f,
        -8.06902504e+00f, -2.30957317e+01f, -2.30957317e+01f, -1.85358467e+01f, -1.12711067e+01f,  1.12711039e+01f,
        -2.07248449e+00f,  3.01493301e+01f,  1.52448931e+01f, -1.52448883e+01f, -2.09650497e+01f, -3.82039986e+01f,
        -3.82039986e+01f, -3.72582664e+01f,  5.42667723e+00f, -5.42667913e+00f, -2.33967514e+01f, -9.90355873e+00f,
        /* Angle 4π/3. */
         1.00999999e+00f, -4.47776222e+00f,  3.00999999e+00f, -2.64288664e-01f,  5.29788685e+00f, -9.94186401e+00f,
         7.01000023e+00f,  1.19981360e+00f, -8.84378815e+00f,  1.00100040e+01f,  7.49403811e+00f, -1.81380157e+01f,
         1.30100002e+01f,  3.39596677e+00f, -1.70399399e+01f,  1.60099964e+01f, -3.01642933e+01f,  1.80100060e+01f,
         1.04222393e+01f, -2.90662193e+01f,  2.10100002e+01f,  6.32417202e+00f, -2.79681435e+01f,  2.40099926e+01f,
         2.22610497e+00f,  1.81805515e+01f, -4.38245430e+01f,  2.80100079e+01f,  1.40824890e+01f, -4.27264709e+01f,
         3.10100002e+01f,  9.98442745e+00f, -4.16283989e+01f,  3.40099869e+01f,  5.88636589e+00f, -4.05303268e+01f,

         1.00999999e+00f, -4.47776222e+00f,  0.00000000e+00f, -1.93892837e+00f,  5.29788685e+00f, -6.93686390e+00f,
         0.00000000e+00f, -3.00375080e+00f, -2.29405141e+00f,  1.00100040e+01f,  1.29990396e+01f, -1.21330166e+01f,
         0.00000000e+00f, -5.25375128e+00f, -5.62874699e+00f, -5.68378528e-06f, -3.01642933e+01f,  7.00829787e-06f,
         1.99272423e+01f, -1.90612202e+01f,  0.00000000e+00f, -8.25375271e+00f, -8.62874603e+00f, -4.09131496e-12f,
        -1.30615349e+01f,  1.81805515e+01f, -3.03195534e+01f,  9.92720470e-06f,  2.85874920e+01f, -2.77214737e+01f,
         0.00000000e+00f, -1.20037527e+01f, -1.23787422e+01f, -5.79531909e-12f, -1.31287651e+01f, -7.87240028e+00f,

         1.00999999e+00f, -3.97776222e+00f, -3.97776222e+00f,  2.86356640e+00f,  6.37110424e+00f, -1.01224155e+01f,
        -1.01224155e+01f,  1.05787458e+01f, -7.76929522e+00f, -7.76928997e+00f,  1.68836861e+01f, -2.05748577e+01f,
        -2.05748577e+01f,  2.49091301e+01f, -5.72616625e+00f, -5.72616386e+00f, -1.87962208e+01f, -1.87962112e+01f,
         2.93253498e+01f, -3.37238922e+01f, -3.37238922e+01f,  4.22584419e+01f, -4.85123205e+00f, -4.85122633e+00f,
        -2.53339314e+00f,  3.24522591e+01f, -4.65456696e+01f, -4.65456543e+01f,  5.18603249e+01f, -5.36516304e+01f,
        -5.36516304e+01f,  7.17381744e+01f,  4.44061565e+00f,  4.44062901e+00f,  2.58841743e+01f, -1.07481155e+01f,
    };

    for (l = 0; l < 3; l++)
    {
        if (l == 0)
            out_temp = out;
        else
            out_temp = &in[l - 1];

        if (l < 2)
            end = 49;
        else
            end = 48;

        for (j = 0; j < ARRAY_SIZE(angle); ++j)
        {
            for (order = 0; order <= D3DXSH_MAXORDER + 1; order++)
            {
                for (i = 0; i < ARRAY_SIZE(out); ++i)
                {
                    out[i] = ( i + 1.0f ) * ( i + 1.0f );
                    in[i] = i + 1.01f;
                }

                received_ptr = D3DXSHRotateZ(out_temp, order, angle[j], in);
                ok(received_ptr == out_temp, "angle %f, order %u, expected %p, received %p\n", angle[j], order, out_temp, received_ptr);

                for (i = 0; i < end; i++)
                {
                    /* order = 0 or order = 1 behaves like order = D3DXSH_MINORDER */
                    square = (order <= D3DXSH_MINORDER) ? D3DXSH_MINORDER * D3DXSH_MINORDER : order * order;
                    if (i >= square || ((order >= D3DXSH_MAXORDER) && (i >= D3DXSH_MAXORDER * D3DXSH_MAXORDER)))
                        if (l > 0)
                            expected = i + l + 0.01f;
                        else
                            expected = ( i + 1.0f ) * ( i + 1.0f );
                    else
                        expected = table[36 * (l + 3 * j) + i];
                    equal = compare_float(expected, out_temp[i], 512);
                    ok(equal || (fabs(expected) < 2.0e-5f && fabs(out_temp[i]) < 2.0e-5f),
                            "angle %.8e, order %u index %u, expected %.8e, received %.8e.\n",
                            angle[j], order, i, expected, out_temp[i]);
                }
            }
        }
    }
}

static void test_D3DXSHScale(void)
{
    float a[49], b[49], expected, *received_array;
    unsigned int i, order;
    BOOL equal;

    for (i = 0; i < ARRAY_SIZE(a); ++i)
    {
        a[i] = i;
        b[i] = i;
    }

    for (order = 0; order <= D3DXSH_MAXORDER + 1; order++)
    {
        received_array = D3DXSHScale(b, order, a, 5.0f);
        ok(received_array == b, "Expected %p, received %p\n", b, received_array);

        for (i = 0; i < ARRAY_SIZE(b); ++i)
        {
            if (i < order * order)
                expected = 5.0f * a[i];
            /* D3DXSHScale does not modify the elements of the array after the order * order-th element */
            else
                expected = a[i];
            equal = compare_float(b[i], expected, 0);
            ok(equal, "order %u, element %u, expected %.8e, received %.8e.\n", order, i, expected, b[i]);
        }
    }
}

static void test_D3DXSHProjectCubeMap(void)
{
    unsigned int i, j, level, face, x, y;
    float red[4], green[4], blue[4];
    IDirect3DCubeTexture9 *texture;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT map_desc;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        D3DFORMAT format;
        float red[4];
        float green[4];
        float blue[4];
    }
    tests[] =
    {
        {D3DFMT_A8R8G8B8,
            {1.77656245f, -1.11197047e-2f,  2.08763797e-2f, -2.10229922e-2f},
            {1.75811982f, -4.82511893e-2f,  1.67397819e-2f, -1.71497762e-2f},
            {1.75515056f, -4.07523997e-2f,  1.05397226e-2f, -2.46812664e-2f}},
        {D3DFMT_X8R8G8B8,
            {1.77656245f, -1.11197047e-2f,  2.08763797e-2f, -2.10229922e-2f},
            {1.75811982f, -4.82511893e-2f,  1.67397819e-2f, -1.71497762e-2f},
            {1.75515056f, -4.07523997e-2f,  1.05397226e-2f, -2.46812664e-2f}},
        {D3DFMT_A8B8G8R8,
            {1.75515056f, -4.07523997e-2f,  1.05397226e-2f, -2.46812664e-2f},
            {1.75811982f, -4.82511893e-2f,  1.67397819e-2f, -1.71497762e-2f},
            {1.77656245f, -1.11197047e-2f,  2.08763797e-2f, -2.10229922e-2f}},
        {D3DFMT_R5G6B5,
            {1.77099848f, -3.88867334e-2f,  6.73775524e-2f, -1.26888147e-2f},
            {1.77244151f, -5.64723741e-4f, -2.77878426e-4f, -9.10691451e-3f},
            {1.78902030f,  2.79005636e-2f,  1.62461456e-2f,  2.21668324e-3f}},
        {D3DFMT_A1R5G5B5,
            {1.78022826f,  1.46923587e-2f,  3.58058624e-2f,  2.51076911e-2f},
            {1.77233493f, -7.58088892e-4f, -2.03727093e-3f, -1.34809706e-2f},
            {1.78902030f,  2.79005636e-2f,  1.62461456e-2f,  2.21668324e-3f}},
        {D3DFMT_X1R5G5B5,
            {1.78022826f,  1.46923587e-2f,  3.58058624e-2f,  2.51076911e-2f},
            {1.77233493f, -7.58088892e-4f, -2.03727093e-3f, -1.34809706e-2f},
            {1.78902030f,  2.79005636e-2f,  1.62461456e-2f,  2.21668324e-3f}},
        {D3DFMT_A2R10G10B10,
            {1.79359019f, -7.74506712e-4f,  8.65613017e-3f,  5.75336441e-3f},
            {1.77067971f,  6.42523961e-3f,  1.35379164e-2f,  2.24088971e-3f},
            {1.76601243f, -4.94002625e-2f,  1.28124524e-2f, -7.69229094e-3f}},
        {D3DFMT_A2B10G10R10,
            {1.76601243f, -4.94002625e-2f,  1.28124524e-2f, -7.69229094e-3f},
            {1.77067971f,  6.42523961e-3f,  1.35379164e-2f,  2.24088971e-3f},
            {1.79359019f, -7.74506712e-4f,  8.65613017e-3f,  5.75336441e-3f}},
        {D3DFMT_A16B16G16R16,
            {1.75979614f,  1.44450525e-2f, -3.25212209e-3f,  2.98178056e-3f},
            {1.78080165f, -2.63770130e-2f,  6.31967233e-3f,  3.66022950e-3f},
            {1.77588308f, -1.93727610e-3f, -3.22831096e-3f, -6.18841546e-3f}},
        {D3DFMT_A16B16G16R16F,
            { 5.17193642e+1f, -3.41681671e+2f, -8.82221741e+2f,  7.77049316e+2f},
            {-2.08198950e+3f,  5.24323584e+3f, -3.42663379e+3f,  3.80999243e+3f},
            {-1.10743945e+3f, -9.43649292e+2f,  5.48424316e+2f,  1.65352710e+3f}},
    };

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Failed to create a D3D object.\n");
        return;
    }

    window = create_window();
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Format %#x", tests[i].format);

        hr = IDirect3DDevice9_CreateCubeTexture(device, 8, 4, D3DUSAGE_DYNAMIC,
                tests[i].format, D3DPOOL_DEFAULT, &texture, NULL);
        if (FAILED(hr))
        {
            skip("Failed to create cube texture.\n");
            winetest_pop_context();
            continue;
        }
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        for (face = 0; face < 6; ++face)
        {
            hr = IDirect3DCubeTexture9_LockRect(texture, face, 0, &map_desc, NULL, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            for (y = 0; y < 8; ++y)
            {
                uint8_t *row = (uint8_t *)map_desc.pBits + y * map_desc.Pitch;

                for (x = 0; x < map_desc.Pitch; ++x)
                    row[x] = face * 111 + y * 39 + x * 7;
            }

            hr = IDirect3DCubeTexture9_UnlockRect(texture, face, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            for (level = 1; level < 4; ++level)
            {
                hr = IDirect3DCubeTexture9_LockRect(texture, face, level, &map_desc, NULL, 0);
                ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
                memset(map_desc.pBits, 0xcc, (8 >> level) * map_desc.Pitch);
                hr = IDirect3DCubeTexture9_UnlockRect(texture, face, level);
                ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            }
        }

        hr = D3DXSHProjectCubeMap(1, texture, red, green, blue);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

        hr = D3DXSHProjectCubeMap(7, texture, red, green, blue);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

        hr = D3DXSHProjectCubeMap(2, NULL, red, green, blue);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

        memset(red, 0, sizeof(red));
        memset(green, 0, sizeof(green));
        memset(blue, 0, sizeof(blue));
        hr = D3DXSHProjectCubeMap(2, texture, red, green, blue);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        for (j = 0; j < 4; ++j)
        {
            ok(compare_float(red[j], tests[i].red[j], 1024),
                    "Got unexpected value %.8e for red coefficient %u.\n", red[j], j);
            ok(compare_float(green[j], tests[i].green[j], 1024),
                    "Got unexpected value %.8e for green coefficient %u.\n", green[j], j);
            ok(compare_float(blue[j], tests[i].blue[j], 1024),
                    "Got unexpected value %.8e for blue coefficient %u.\n", blue[j], j);
        }

        hr = D3DXSHProjectCubeMap(2, texture, red, NULL, NULL);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = D3DXSHProjectCubeMap(2, texture, NULL, green, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

        hr = D3DXSHProjectCubeMap(2, texture, NULL, NULL, blue);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

        IDirect3DCubeTexture9_Release(texture);

        winetest_pop_context();
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(math)
{
    D3DXColorTest();
    D3DXFresnelTest();
    D3DXMatrixTest();
    D3DXPlaneTest();
    D3DXQuaternionTest();
    D3DXVector2Test();
    D3DXVector3Test();
    D3DXVector4Test();
    test_matrix_stack();
    test_Matrix_AffineTransformation2D();
    test_Matrix_Decompose();
    test_Matrix_Transformation2D();
    test_D3DXVec_Array();
    test_D3DXFloat_Array();
    test_D3DXSHAdd();
    test_D3DXSHDot();
    test_D3DXSHEvalConeLight();
    test_D3DXSHEvalDirection();
    test_D3DXSHEvalDirectionalLight();
    test_D3DXSHEvalHemisphereLight();
    test_D3DXSHEvalSphericalLight();
    test_D3DXSHMultiply2();
    test_D3DXSHMultiply3();
    test_D3DXSHMultiply4();
    test_D3DXSHRotate();
    test_D3DXSHRotateZ();
    test_D3DXSHScale();
    test_D3DXSHProjectCubeMap();
}
