/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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
#include <limits.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include "d3dcompiler.h"
#include "d2d1_3.h"
#include "d2d1effectauthor.h"
#include "d3d11.h"
#include "wincrypt.h"
#include "wine/test.h"
#include "initguid.h"
#include "dwrite.h"
#include "wincodec.h"

DEFINE_GUID(CLSID_TestEffect, 0xb9ee12e9,0x32d9,0xe659,0xac,0x61,0x2d,0x7c,0xea,0x69,0x28,0x78);
DEFINE_GUID(CLSID_TestEffect2, 0xb9ee12e9,0x32d9,0xe659,0xac,0x61,0x2d,0x7c,0xea,0x69,0x28,0x79);
DEFINE_GUID(CLSID_AuxEffect, 0xb9ee12e9,0x32d9,0xe659,0xac,0x61,0x2d,0x7c,0xea,0x69,0x28,0x79);
DEFINE_GUID(GUID_TestVertexShader, 0x5bcdcfae,0x1e92,0x4dc1,0x94,0xfa,0x3b,0x01,0xca,0x54,0x59,0x20);
DEFINE_GUID(GUID_TestPixelShader,  0x53015748,0xfc13,0x4168,0xbd,0x13,0x0f,0xcf,0x15,0x29,0x7f,0x01);
DEFINE_GUID(GUID_CustomVertexBuffer, 0x53015748,0xfc13,0x4168,0xbd,0x13,0x0f,0xcf,0x15,0x29,0x7f,0x02);

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static const WCHAR *effect_xml_a =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string' value='TestEffectA'/>      \
        <Property name='Author'      type='string' value='The Wine Project'/> \
        <Property name='Category'    type='string' value='Test'/>             \
        <Property name='Description' type='string' value='Test effect.'/>     \
        <Inputs>                                                              \
            <Input name='Source'/>                                            \
        </Inputs>                                                             \
        <Property name='Integer' type='uint32'>                               \
            <Property name='DisplayName' type='string' value='Integer'/>      \
            <Property name='Min'         type='uint32' value='0'/>            \
            <Property name='Max'         type='uint32' value='100'/>          \
            <Property name='Default'     type='uint32' value='10'/>           \
        </Property>                                                           \
        <Property name='Int32Prop' type='int32' value='-2'>                   \
            <Property name='DisplayName' type='string' value='Int32 prop'/>   \
            <Property name='Default' type='int32' value='10'/>                \
        </Property>                                                           \
        <Property name='Int32PropHex' type='int32' value='0xffff0001'>        \
            <Property name='DisplayName' type='string' value='Int32 prop hex'/> \
        </Property>                                                           \
        <Property name='Int32PropOct' type='int32' value='012'>               \
            <Property name='DisplayName' type='string' value='Int32 prop oct'/> \
        </Property>                                                           \
        <Property name='UInt32Prop' type='uint32' value='-3'>                 \
            <Property name='DisplayName' type='string' value='UInt32 prop'/>  \
            <Property name='Default' type='uint32' value='10'/>               \
        </Property>                                                           \
        <Property name='UInt32PropHex' type='uint32' value='0xfff'>           \
            <Property name='DisplayName' type='string' value='UInt32 prop hex'/> \
        </Property>                                                           \
        <Property name='UInt32PropOct' type='uint32' value='013'>           \
            <Property name='DisplayName' type='string' value='UInt32 prop oct'/> \
        </Property>                                                           \
        <Property name='Bool' type='bool'>                                     \
            <Property name='DisplayName' type='string' value='Bool property'/> \
            <Property name='Default'     type='bool' value='false'/>           \
        </Property>                                                            \
        <Property name='Vec2Prop' type='vector2' value='( 3.0,  4.0)'>         \
            <Property name='DisplayName' type='string' value='Vec2 prop'/>    \
            <Property name='Default'     type='vector2' value='(1.0, 2.0)'/>  \
        </Property>                                                           \
        <Property name='Vec3Prop' type='vector3' value='(5.0, 6.0, 7.0)'>     \
            <Property name='DisplayName' type='string' value='Vec3 prop'/>    \
            <Property name='Default' type='vector3' value='(0.1, 0.2, 0.3)'/> \
        </Property>                                                           \
        <Property name='Vec4Prop' type='vector4' value='(8.0,9.0,10.0,11.0)'>   \
            <Property name='DisplayName' type='string' value='Vec4 prop'/>      \
            <Property name='Default' type='vector4' value='(0.8,0.9,1.0,1.1)'/> \
        </Property>                                                             \
        <Property name='Mat3x2Prop' type='matrix3x2'                          \
            value='(1.0,2.0,3.0,4.0,5.0,6.0)'>                                \
            <Property name='DisplayName' type='string' value='Mat3x2 prop'/>  \
            <Property name='Default' type='matrix3x2'                         \
                value='(0.1,0.2,0.3,0.4,0.5,0.6)'/>                           \
        </Property>                                                           \
        <Property name='Mat4x3Prop' type='matrix4x3'                          \
            value='(1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12)'>       \
            <Property name='DisplayName' type='string' value='Mat4x3 prop'/>  \
            <Property name='Default' type='matrix4x3'                         \
                value='(0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.2)'/>   \
        </Property>                                                           \
        <Property name='Mat4x4Prop' type='matrix4x4'                          \
            value='(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)'>                 \
            <Property name='DisplayName' type='string' value='Mat4x4 prop'/>  \
            <Property name='Default' type='matrix4x4'                         \
                value='(16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)'/>            \
        </Property>                                                           \
        <Property name='Mat5x4Prop' type='matrix5x4'                          \
            value='(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)'>     \
            <Property name='DisplayName' type='string' value='Mat5x4 prop'/>  \
            <Property name='Default' type='matrix5x4'                         \
                value='(20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)'/>\
        </Property>                                                           \
        <Property name='BlobProp' type='blob' >                               \
            <Property name='DisplayName' type='string' value='Blob prop'/>    \
        </Property>                                                           \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_b =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string' value='TestEffectB'/>      \
        <Property name='Author'      type='string' value='The Wine Project'/> \
        <Property name='Category'    type='string' value='Test'/>             \
        <Property name='Description' type='string' value='Test effect.'/>     \
        <Inputs>                                                              \
            <Input name='Source'/>                                            \
        </Inputs>                                                             \
        <Property name='Context' type='iunknown'>                             \
            <Property name='DisplayName' type='string' value='Context'/>      \
        </Property>                                                           \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_c =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string' value='TestEffectC'/>      \
        <Property name='Author'      type='string' value='The Wine Project'/> \
        <Property name='Category'    type='string' value='Test'/>             \
        <Property name='Description' type='string' value='Test effect.'/>     \
        <Inputs>                                                              \
            <Input name='Source'/>                                            \
        </Inputs>                                                             \
        <Property name='Context' type='iunknown'>                             \
            <Property name='DisplayName' type='string' value='Context'/>      \
        </Property>                                                           \
        <Property name='Integer' type='uint32'>                               \
            <Property name='DisplayName' type='string' value='Integer'/>      \
        </Property>                                                           \
        <Property name='Graph' type='iunknown'>                               \
            <Property name='DisplayName' type='string' value='Graph'/>        \
        </Property>                                                           \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_minimum =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string'/>                          \
        <Property name='Author'      type='string'/>                          \
        <Property name='Category'    type='string'/>                          \
        <Property name='Description' type='string'/>                          \
        <Inputs/>                                                             \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_without_version =
L"<Effect>                                                                    \
        <Property name='DisplayName' type='string'/>                          \
        <Property name='Author'      type='string'/>                          \
        <Property name='Category'    type='string'/>                          \
        <Property name='Description' type='string'/>                          \
        <Inputs/>                                                             \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_without_inputs =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string'/>                          \
        <Property name='Author'      type='string'/>                          \
        <Property name='Category'    type='string'/>                          \
        <Property name='Description' type='string'/>                          \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_without_name =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='Author'      type='string'/>                          \
        <Property name='Category'    type='string'/>                          \
        <Property name='Description' type='string'/>                          \
        <Inputs/>                                                             \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_without_author =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string'/>                          \
        <Property name='Category'    type='string'/>                          \
        <Property name='Description' type='string'/>                          \
        <Inputs/>                                                             \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_without_category =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string'/>                          \
        <Property name='Author'      type='string'/>                          \
        <Property name='Description' type='string'/>                          \
        <Inputs/>                                                             \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_without_description =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string'/>                          \
        <Property name='Author'      type='string'/>                          \
        <Property name='Category'    type='string'/>                          \
        <Inputs/>                                                             \
    </Effect>                                                                 \
";

static const WCHAR *effect_xml_without_type =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string'/>                          \
        <Property name='Author'      type='string'/>                          \
        <Property name='Category'    type='string'/>                          \
        <Property name='Description'/>                                        \
        <Inputs/>                                                             \
    </Effect>                                                                 \
";

static const WCHAR *effect_variable_input_count =
L"<?xml version='1.0'?>                                                       \
    <Effect>                                                                  \
        <Property name='DisplayName' type='string' value='VariableInputs'/>   \
        <Property name='Author'      type='string' value='The Wine Project'/> \
        <Property name='Category'    type='string' value='Test'/>             \
        <Property name='Description' type='string' value='Test effect.'/>     \
        <Inputs minimum='2' maximum='5' >                                     \
            <Input name='Source1'/>                                           \
            <Input name='Source2'/>                                           \
            <Input name='Source3'/>                                           \
        </Inputs>                                                             \
        <Property name='Graph' type='iunknown'>                               \
            <Property name='DisplayName' type='string' value='Graph'/>        \
        </Property>                                                           \
    </Effect>                                                                 \
";

static const char test_vs_code[] =
    "void main(float4 pos : Position, out float4 output : SV_Position)\n"
    "{\n"
    "    output = pos;\n"
    "}";

static const char test_ps_code[] =
    "float4 main() : SV_Target\n"
    "{\n"
    "    return float4(0.1, 0.2, 0.3, 0.4);\n"
    "}";

static HRESULT (WINAPI *pD2D1CreateDevice)(IDXGIDevice *dxgi_device,
        const D2D1_CREATION_PROPERTIES *properties, ID2D1Device **device);
static void (WINAPI *pD2D1SinCos)(float angle, float *s, float *c);
static float (WINAPI *pD2D1Tan)(float angle);
static float (WINAPI *pD2D1Vec3Length)(float x, float y, float z);
static D2D1_COLOR_F (WINAPI *pD2D1ConvertColorSpace)(D2D1_COLOR_SPACE src_colour_space,
        D2D1_COLOR_SPACE dst_colour_space, const D2D1_COLOR_F *colour);

static BOOL use_mt = TRUE;

static struct test_entry
{
    void (*test)(BOOL d3d11);
    BOOL d3d11;
} *mt_tests;
size_t mt_tests_size, mt_test_count;

struct d2d1_test_context
{
    BOOL d3d11;
    IDXGIDevice *device;
    HWND window;
    IDXGISwapChain *swapchain;
    IDXGISurface *surface;
    ID2D1RenderTarget *rt;
    ID2D1DeviceContext *context;
    ID2D1Factory *factory;
    ID2D1Factory1 *factory1;
    ID2D1Factory2 *factory2;
    ID2D1Factory3 *factory3;
};

struct resource_readback
{
    BOOL d3d11;
    union
    {
        ID3D10Resource *d3d10_resource;
        ID3D11Resource *d3d11_resource;
    } u;
    unsigned int pitch, width, height;
    void *data;
};

struct figure
{
    unsigned int *spans;
    unsigned int spans_size;
    unsigned int span_count;
};

struct geometry_sink
{
    ID2D1SimplifiedGeometrySink ID2D1SimplifiedGeometrySink_iface;

    struct geometry_figure
    {
        D2D1_FIGURE_BEGIN begin;
        D2D1_FIGURE_END end;
        D2D1_POINT_2F start_point;
        struct geometry_segment
        {
            enum
            {
                SEGMENT_BEZIER,
                SEGMENT_LINE,
            } type;
            union
            {
                D2D1_BEZIER_SEGMENT bezier;
                D2D1_POINT_2F line;
            } u;
            DWORD flags;
        } *segments;
        unsigned int segments_size;
        unsigned int segment_count;
    } *figures;
    unsigned int figures_size;
    unsigned int figure_count;

    D2D1_FILL_MODE fill_mode;
    DWORD segment_flags;
    BOOL closed;
};

struct expected_geometry_figure
{
    D2D1_FIGURE_BEGIN begin;
    D2D1_FIGURE_END end;
    D2D1_POINT_2F start_point;
    unsigned int segment_count;
    const struct geometry_segment *segments;
};

struct effect_impl
{
    ID2D1EffectImpl ID2D1EffectImpl_iface;
    ID2D1DrawTransform ID2D1DrawTransform_iface;
    LONG refcount;
    UINT integer;
    ID2D1EffectContext *effect_context;
    ID2D1TransformGraph *transform_graph;
    ID2D1DrawInfo *draw_info;
    struct
    {
        BYTE *data;
        ULONG size;
    } blob;

    const GUID *vertex_buffer;
    const GUID *vertex_shader;
    const GUID *pixel_shader;
};

static void queue_d3d1x_test(void (*test)(BOOL d3d11), BOOL d3d11)
{
    if (mt_test_count >= mt_tests_size)
    {
        mt_tests_size = max(16, mt_tests_size * 2);
        mt_tests = realloc(mt_tests, mt_tests_size * sizeof(*mt_tests));
    }
    mt_tests[mt_test_count].test = test;
    mt_tests[mt_test_count++].d3d11 = d3d11;
}

static void queue_d3d10_test(void (*test)(BOOL d3d11))
{
    queue_d3d1x_test(test, FALSE);
}

static void queue_test(void (*test)(BOOL d3d11))
{
    queue_d3d1x_test(test, FALSE);
    queue_d3d1x_test(test, TRUE);
}

static DWORD WINAPI thread_func(void *ctx)
{
    LONG *i = ctx, j;

    while (*i < mt_test_count)
    {
        j = *i;
        if (InterlockedCompareExchange(i, j + 1, j) == j)
            mt_tests[j].test(mt_tests[j].d3d11);
    }

    return 0;
}

static void run_queued_tests(void)
{
    unsigned int thread_count, i;
    HANDLE *threads;
    SYSTEM_INFO si;
    LONG test_idx;

    if (!use_mt)
    {
        for (i = 0; i < mt_test_count; ++i)
        {
            mt_tests[i].test(mt_tests[i].d3d11);
        }

        return;
    }

    GetSystemInfo(&si);
    thread_count = si.dwNumberOfProcessors;
    threads = calloc(thread_count, sizeof(*threads));
    for (i = 0, test_idx = 0; i < thread_count; ++i)
    {
        threads[i] = CreateThread(NULL, 0, thread_func, &test_idx, 0, NULL);
        ok(!!threads[i], "Failed to create thread %u.\n", i);
    }
    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
    for (i = 0; i < thread_count; ++i)
    {
        CloseHandle(threads[i]);
    }
    free(threads);
}

static void set_point(D2D1_POINT_2F *point, float x, float y)
{
    point->x = x;
    point->y = y;
}

static void set_quadratic(D2D1_QUADRATIC_BEZIER_SEGMENT *quadratic, float x1, float y1, float x2, float y2)
{
    quadratic->point1.x = x1;
    quadratic->point1.y = y1;
    quadratic->point2.x = x2;
    quadratic->point2.y = y2;
}

static void set_rect(D2D1_RECT_F *rect, float left, float top, float right, float bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static void set_vec4(D2D_VECTOR_4F *vec, float x, float y, float z, float w)
{
    vec->x = x;
    vec->y = y;
    vec->z = z;
    vec->w = w;
}

static void set_rounded_rect(D2D1_ROUNDED_RECT *rect, float left, float top, float right, float bottom,
        float radius_x, float radius_y)
{
    set_rect(&rect->rect, left, top, right, bottom);
    rect->radiusX = radius_x;
    rect->radiusY = radius_y;
}

static void set_rect_u(D2D1_RECT_U *rect, UINT32 left, UINT32 top, UINT32 right, UINT32 bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static void set_rect_l(D2D1_RECT_L *rect, LONG left, LONG top, LONG right, LONG bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static void set_ellipse(D2D1_ELLIPSE *ellipse, float x, float y, float rx, float ry)
{
    set_point(&ellipse->point, x, y);
    ellipse->radiusX = rx;
    ellipse->radiusY = ry;
}

static void set_color(D2D1_COLOR_F *color, float r, float g, float b, float a)
{
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
}

static void set_size_u(D2D1_SIZE_U *size, unsigned int w, unsigned int h)
{
    size->width = w;
    size->height = h;
}

static void set_size_f(D2D1_SIZE_F *size, float w, float h)
{
    size->width = w;
    size->height = h;
}

static void set_matrix_identity(D2D1_MATRIX_3X2_F *matrix)
{
    matrix->_11 = 1.0f;
    matrix->_12 = 0.0f;
    matrix->_21 = 0.0f;
    matrix->_22 = 1.0f;
    matrix->_31 = 0.0f;
    matrix->_32 = 0.0f;
}

static void rotate_matrix(D2D1_MATRIX_3X2_F *matrix, float theta)
{
    float sin_theta, cos_theta, tmp_11, tmp_12;

    sin_theta = sinf(theta);
    cos_theta = cosf(theta);
    tmp_11 = matrix->_11;
    tmp_12 = matrix->_12;

    matrix->_11 = cos_theta * tmp_11 + sin_theta * matrix->_21;
    matrix->_12 = cos_theta * tmp_12 + sin_theta * matrix->_22;
    matrix->_21 = -sin_theta * tmp_11 + cos_theta * matrix->_21;
    matrix->_22 = -sin_theta * tmp_12 + cos_theta * matrix->_22;
}

static void skew_matrix(D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    float tmp_11, tmp_12;

    tmp_11 = matrix->_11;
    tmp_12 = matrix->_12;

    matrix->_11 += y * matrix->_21;
    matrix->_12 += y * matrix->_22;
    matrix->_21 += x * tmp_11;
    matrix->_22 += x * tmp_12;
}

static void scale_matrix(D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    matrix->_11 *= x;
    matrix->_12 *= x;
    matrix->_21 *= y;
    matrix->_22 *= y;
}

static void translate_matrix(D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    matrix->_31 += x * matrix->_11 + y * matrix->_21;
    matrix->_32 += x * matrix->_12 + y * matrix->_22;
}

static void line_to(ID2D1GeometrySink *sink, float x, float y)
{
    D2D1_POINT_2F point;

    set_point(&point, x,  y);
    ID2D1GeometrySink_AddLine(sink, point);
}

static void quadratic_to(ID2D1GeometrySink *sink, float x1, float y1, float x2, float y2)
{
    D2D1_QUADRATIC_BEZIER_SEGMENT quadratic;

    set_quadratic(&quadratic, x1, y1, x2, y2);
    ID2D1GeometrySink_AddQuadraticBezier(sink, &quadratic);
}

static void cubic_to(ID2D1GeometrySink *sink, float x1, float y1, float x2, float y2, float x3, float y3)
{
    D2D1_BEZIER_SEGMENT b;

    b.point1.x = x1;
    b.point1.y = y1;
    b.point2.x = x2;
    b.point2.y = y2;
    b.point3.x = x3;
    b.point3.y = y3;
    ID2D1GeometrySink_AddBezier(sink, &b);
}

static void get_d3d10_surface_readback(IDXGISurface *surface, struct resource_readback *rb)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    D3D10_MAPPED_TEXTURE2D map_desc;
    DXGI_SURFACE_DESC surface_desc;
    ID3D10Resource *src_resource;
    ID3D10Device *device;
    HRESULT hr;

    hr = IDXGISurface_GetDevice(surface, &IID_ID3D10Device, (void **)&device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISurface_QueryInterface(surface, &IID_ID3D10Resource, (void **)&src_resource);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    texture_desc.Width = surface_desc.Width;
    texture_desc.Height = surface_desc.Height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = surface_desc.Format;
    texture_desc.SampleDesc = surface_desc.SampleDesc;
    texture_desc.Usage = D3D10_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D10Texture2D **)&rb->u.d3d10_resource);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    rb->width = texture_desc.Width;
    rb->height = texture_desc.Height;

    ID3D10Device_CopyResource(device, rb->u.d3d10_resource, src_resource);
    ID3D10Resource_Release(src_resource);
    ID3D10Device_Release(device);

    hr = ID3D10Texture2D_Map((ID3D10Texture2D *)rb->u.d3d10_resource, 0, D3D10_MAP_READ, 0, &map_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    rb->pitch = map_desc.RowPitch;
    rb->data = map_desc.pData;
}

static void get_d3d11_surface_readback(IDXGISurface *surface, struct resource_readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_MAPPED_SUBRESOURCE map_desc;
    DXGI_SURFACE_DESC surface_desc;
    ID3D11Resource *src_resource;
    ID3D11DeviceContext *context;
    ID3D11Device *device;
    HRESULT hr;

    hr = IDXGISurface_GetDevice(surface, &IID_ID3D11Device, (void **)&device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISurface_QueryInterface(surface, &IID_ID3D11Resource, (void **)&src_resource);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    texture_desc.Width = surface_desc.Width;
    texture_desc.Height = surface_desc.Height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = surface_desc.Format;
    texture_desc.SampleDesc = surface_desc.SampleDesc;
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D11Texture2D **)&rb->u.d3d11_resource);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    rb->width = texture_desc.Width;
    rb->height = texture_desc.Height;

    ID3D11Device_GetImmediateContext(device, &context);
    ID3D11DeviceContext_CopyResource(context, rb->u.d3d11_resource, src_resource);
    ID3D11Resource_Release(src_resource);
    ID3D11Device_Release(device);

    hr = ID3D11DeviceContext_Map(context, rb->u.d3d11_resource, 0, D3D11_MAP_READ, 0, &map_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D11DeviceContext_Release(context);

    rb->pitch = map_desc.RowPitch;
    rb->data = map_desc.pData;
}

static void get_surface_readback(struct d2d1_test_context *ctx, struct resource_readback *rb)
{
    if ((rb->d3d11 = ctx->d3d11))
        get_d3d11_surface_readback(ctx->surface, rb);
    else
        get_d3d10_surface_readback(ctx->surface, rb);
}

static void release_d3d10_resource_readback(struct resource_readback *rb)
{
    ID3D10Texture2D_Unmap((ID3D10Texture2D *)rb->u.d3d10_resource, 0);
    ID3D10Resource_Release(rb->u.d3d10_resource);
}

static void release_d3d11_resource_readback(struct resource_readback *rb)
{
    ID3D11DeviceContext *context;
    ID3D11Device *device;

    ID3D11Resource_GetDevice(rb->u.d3d11_resource, &device);
    ID3D11Device_GetImmediateContext(device, &context);
    ID3D11Device_Release(device);

    ID3D11DeviceContext_Unmap(context, rb->u.d3d11_resource, 0);
    ID3D11Resource_Release(rb->u.d3d11_resource);
    ID3D11DeviceContext_Release(context);
}

static void release_resource_readback(struct resource_readback *rb)
{
    if (rb->d3d11)
        release_d3d11_resource_readback(rb);
    else
        release_d3d10_resource_readback(rb);
}

static DWORD get_readback_colour(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return ((DWORD *)((BYTE *)rb->data + y * rb->pitch))[x];
}

static float clamp_float(float f, float lower, float upper)
{
    return f < lower ? lower : f > upper ? upper : f;
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_colour(DWORD c1, DWORD c2, BYTE max_diff)
{
    return compare_uint(c1 & 0xff, c2 & 0xff, max_diff)
            && compare_uint((c1 >> 8) & 0xff, (c2 >> 8) & 0xff, max_diff)
            && compare_uint((c1 >> 16) & 0xff, (c2 >> 16) & 0xff, max_diff)
            && compare_uint((c1 >> 24) & 0xff, (c2 >> 24) & 0xff, max_diff);
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

static BOOL compare_colour_f(const D2D1_COLOR_F *colour, float r, float g, float b, float a, unsigned int ulps)
{
    return compare_float(colour->r, r, ulps)
            && compare_float(colour->g, g, ulps)
            && compare_float(colour->b, b, ulps)
            && compare_float(colour->a, a, ulps);
}

static BOOL compare_point(const D2D1_POINT_2F *point, float x, float y, unsigned int ulps)
{
    return compare_float(point->x, x, ulps)
            && compare_float(point->y, y, ulps);
}

static BOOL compare_rect(const D2D1_RECT_F *rect, float left, float top, float right, float bottom, unsigned int ulps)
{
    return compare_float(rect->left, left, ulps)
            && compare_float(rect->top, top, ulps)
            && compare_float(rect->right, right, ulps)
            && compare_float(rect->bottom, bottom, ulps);
}

static BOOL compare_bezier_segment(const D2D1_BEZIER_SEGMENT *b, float x1, float y1,
        float x2, float y2, float x3, float y3, unsigned int ulps)
{
    return compare_point(&b->point1, x1, y1, ulps)
            && compare_point(&b->point2, x2, y2, ulps)
            && compare_point(&b->point3, x3, y3, ulps);
}

static BOOL compare_sha1(void *data, unsigned int pitch, unsigned int bpp,
        unsigned int w, unsigned int h, const char *ref_sha1)
{
    static const char hex_chars[] = "0123456789abcdef";
    HCRYPTPROV provider;
    BYTE hash_data[20];
    HCRYPTHASH hash;
    DWORD hash_size;
    unsigned int i;
    char sha1[41];
    BOOL ret;

    ret = CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(ret, "Failed to acquire crypt context.\n");
    ret = CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash);
    ok(ret, "Failed to create hash.\n");

    for (i = 0; i < h; ++i)
    {
        if (!(ret = CryptHashData(hash, (BYTE *)data + pitch * i, w * bpp, 0)))
            break;
    }
    ok(ret, "Failed to hash data.\n");

    hash_size = sizeof(hash_data);
    ret = CryptGetHashParam(hash, HP_HASHVAL, hash_data, &hash_size, 0);
    ok(ret, "Failed to get hash value.\n");
    ok(hash_size == sizeof(hash_data), "Got unexpected hash size %lu.\n", hash_size);

    ret = CryptDestroyHash(hash);
    ok(ret, "Failed to destroy hash.\n");
    ret = CryptReleaseContext(provider, 0);
    ok(ret, "Failed to release crypt context.\n");

    for (i = 0; i < 20; ++i)
    {
        sha1[i * 2] = hex_chars[hash_data[i] >> 4];
        sha1[i * 2 + 1] = hex_chars[hash_data[i] & 0xf];
    }
    sha1[40] = 0;

    return !strcmp(ref_sha1, (char *)sha1);
}

static BOOL compare_surface(struct d2d1_test_context *ctx, const char *ref_sha1)
{
    struct resource_readback rb;
    BOOL ret;

    get_surface_readback(ctx, &rb);
    ret = compare_sha1(rb.data, rb.pitch, 4, rb.width, rb.height, ref_sha1);
    release_resource_readback(&rb);

    return ret;
}

static BOOL compare_wic_bitmap(IWICBitmap *bitmap, const char *ref_sha1)
{
    UINT stride, width, height, buffer_size;
    IWICBitmapLock *lock;
    BYTE *data;
    HRESULT hr;
    BOOL ret;

    hr = IWICBitmap_Lock(bitmap, NULL, WICBitmapLockRead, &lock);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IWICBitmapLock_GetDataPointer(lock, &buffer_size, &data);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IWICBitmapLock_GetStride(lock, &stride);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IWICBitmapLock_GetSize(lock, &width, &height);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = compare_sha1(data, stride, 4, width, height, ref_sha1);

    IWICBitmapLock_Release(lock);

    return ret;
}

static void serialize_figure(struct figure *figure)
{
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int i, j, k, span;
    char output[76];
    char t[3];
    char *p;

    for (i = 0, j = 0, k = 0, p = output; i < figure->span_count; ++i)
    {
        span = figure->spans[i];
        while (span)
        {
            t[j] = span & 0x7f;
            if (span > 0x7f)
                t[j] |= 0x80;
            span >>= 7;
            if (++j == 3)
            {
                p[0] = lookup[(t[0] & 0xfc) >> 2];
                p[1] = lookup[((t[0] & 0x03) << 4) | ((t[1] & 0xf0) >> 4)];
                p[2] = lookup[((t[1] & 0x0f) << 2) | ((t[2] & 0xc0) >> 6)];
                p[3] = lookup[t[2] & 0x3f];
                p += 4;
                if (++k == 19)
                {
                    trace("%.76s\n", output);
                    p = output;
                    k = 0;
                }
                j = 0;
            }
        }
    }
    if (j)
    {
        for (i = j; i < 3; ++i)
            t[i] = 0;
        p[0] = lookup[(t[0] & 0xfc) >> 2];
        p[1] = lookup[((t[0] & 0x03) << 4) | ((t[1] & 0xf0) >> 4)];
        p[2] = lookup[((t[1] & 0x0f) << 2) | ((t[2] & 0xc0) >> 6)];
        p[3] = lookup[t[2] & 0x3f];
        ++k;
    }
    if (k)
        trace("%.*s\n", k * 4, output);
}

static void figure_add_span(struct figure *figure, unsigned int span)
{
    if (figure->span_count == figure->spans_size)
    {
        figure->spans_size *= 2;
        figure->spans = HeapReAlloc(GetProcessHeap(), 0, figure->spans,
                figure->spans_size * sizeof(*figure->spans));
    }

    figure->spans[figure->span_count++] = span;
}

static void deserialize_span(struct figure *figure, unsigned int *current, unsigned int *shift, unsigned int c)
{
    *current |= (c & 0x7f) << *shift;
    if (c & 0x80)
    {
        *shift += 7;
        return;
    }

    if (*current)
        figure_add_span(figure, *current);
    *current = 0;
    *shift = 0;
}

static void deserialize_figure(struct figure *figure, const BYTE *s)
{
    static const BYTE lookup[] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    unsigned int current = 0, shift = 0;
    const BYTE *ptr;
    BYTE x, y;

    figure->span_count = 0;
    figure->spans_size = 64;
    figure->spans = HeapAlloc(GetProcessHeap(), 0, figure->spans_size * sizeof(*figure->spans));

    for (ptr = s; *ptr; ptr += 4)
    {
        x = lookup[ptr[0]];
        y = lookup[ptr[1]];
        deserialize_span(figure, &current, &shift, ((x & 0x3f) << 2) | ((y & 0x3f) >> 4));
        x = lookup[ptr[2]];
        deserialize_span(figure, &current, &shift, ((y & 0x0f) << 4) | ((x & 0x3f) >> 2));
        y = lookup[ptr[3]];
        deserialize_span(figure, &current, &shift, ((x & 0x03) << 6) | (y & 0x3f));
    }
}

static void read_figure(struct figure *figure, BYTE *data, unsigned int pitch,
        unsigned int x, unsigned int y,  unsigned int w, unsigned int h, DWORD prev)
{
    unsigned int i, j, span;

    figure->span_count = 0;
    for (i = 0, span = 0; i < h; ++i)
    {
        const DWORD *row = (DWORD *)&data[(y + i) * pitch + x * 4];
        for (j = 0; j < w; ++j, ++span)
        {
            if ((i || j) && prev != row[j])
            {
                figure_add_span(figure, span);
                prev = row[j];
                span = 0;
            }
        }
    }
    if (span)
        figure_add_span(figure, span);
}

static BOOL compare_figure(struct d2d1_test_context *ctx, unsigned int x, unsigned int y,
        unsigned int w, unsigned int h, DWORD prev, unsigned int max_diff, const char *ref)
{
    struct figure ref_figure, figure;
    unsigned int i, j, span, diff;
    struct resource_readback rb;

    get_surface_readback(ctx, &rb);

    figure.span_count = 0;
    figure.spans_size = 64;
    figure.spans = HeapAlloc(GetProcessHeap(), 0, figure.spans_size * sizeof(*figure.spans));

    read_figure(&figure, rb.data, rb.pitch, x, y, w, h, prev);

    deserialize_figure(&ref_figure, (BYTE *)ref);
    span = w * h;
    for (i = 0; i < ref_figure.span_count; ++i)
    {
        span -= ref_figure.spans[i];
    }
    if (span)
        figure_add_span(&ref_figure, span);

    for (i = 0, j = 0, diff = 0; i < figure.span_count && j < ref_figure.span_count;)
    {
        if (figure.spans[i] == ref_figure.spans[j])
        {
            if ((i ^ j) & 1)
                diff += ref_figure.spans[j];
            ++i;
            ++j;
        }
        else if (figure.spans[i] > ref_figure.spans[j])
        {
            if ((i ^ j) & 1)
                diff += ref_figure.spans[j];
            figure.spans[i] -= ref_figure.spans[j];
            ++j;
        }
        else
        {
            if ((i ^ j) & 1)
                diff += figure.spans[i];
            ref_figure.spans[j] -= figure.spans[i];
            ++i;
        }
    }
    if (diff > max_diff)
    {
        trace("diff %u > max_diff %u.\n", diff, max_diff);
        read_figure(&figure, rb.data, rb.pitch, x, y, w, h, prev);
        serialize_figure(&figure);
    }

    HeapFree(GetProcessHeap(), 0, ref_figure.spans);
    HeapFree(GetProcessHeap(), 0, figure.spans);
    release_resource_readback(&rb);

    return diff <= max_diff;
}

static ID3D10Device1 *create_d3d10_device(void)
{
    ID3D10Device1 *device;

    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_WARP, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static ID3D11Device *create_d3d11_device(void)
{
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device *device;

    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, &level, 1, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, &level, 1, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, &level, 1, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static IDXGIDevice *create_device(BOOL d3d11)
{
    ID3D10Device1 *d3d10_device;
    ID3D11Device *d3d11_device;
    IDXGIDevice *device;
    HRESULT hr;

    if (d3d11)
    {
        if (!(d3d11_device = create_d3d11_device()))
            return NULL;
        hr = ID3D11Device_QueryInterface(d3d11_device, &IID_IDXGIDevice, (void **)&device);
        ID3D11Device_Release(d3d11_device);
    }
    else
    {
        if (!(d3d10_device = create_d3d10_device()))
            return NULL;
        hr = ID3D10Device1_QueryInterface(d3d10_device, &IID_IDXGIDevice, (void **)&device);
        ID3D10Device1_Release(d3d10_device);
    }

    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    return device;
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d2d1_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDXGISwapChain *create_swapchain(IDXGIDevice *device, HWND window, BOOL windowed)
{
    IDXGISwapChain *swapchain;
    DXGI_SWAP_CHAIN_DESC desc;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    HRESULT hr;

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIAdapter_Release(adapter);

    desc.BufferDesc.Width = 640;
    desc.BufferDesc.Height = 480;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.OutputWindow = window;
    desc.Windowed = windowed;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIFactory_Release(factory);

    return swapchain;
}

static IDXGISwapChain *create_d3d10_swapchain(ID3D10Device1 *device, HWND window, BOOL windowed)
{
    IDXGISwapChain *swapchain;
    IDXGIDevice *dxgi_device;
    HRESULT hr;

    hr = ID3D10Device1_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    swapchain = create_swapchain(dxgi_device, window, windowed);
    IDXGIDevice_Release(dxgi_device);
    return swapchain;
}

static ID2D1RenderTarget *create_render_target_desc(IDXGISurface *surface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc, BOOL d3d11, D2D1_FACTORY_TYPE factory_type)
{
    ID2D1RenderTarget *render_target;
    ID2D1Factory *factory;
    HRESULT hr;

    hr = D2D1CreateFactory(factory_type, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, surface, desc, &render_target);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1Factory_Release(factory);

    return render_target;
}

static ID2D1RenderTarget *create_render_target(IDXGISurface *surface, BOOL d3d11, D2D1_FACTORY_TYPE factory_type)
{
    D2D1_RENDER_TARGET_PROPERTIES desc;

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    return create_render_target_desc(surface, &desc, d3d11, factory_type);
}

#define release_test_context(ctx) release_test_context_(__LINE__, ctx)
static void release_test_context_(unsigned int line, struct d2d1_test_context *ctx)
{
    ULONG ref;

    if (ctx->factory3)
        ID2D1Factory3_Release(ctx->factory3);
    if (ctx->factory2)
        ID2D1Factory2_Release(ctx->factory2);
    if (ctx->factory1)
        ID2D1Factory1_Release(ctx->factory1);
    ID2D1DeviceContext_Release(ctx->context);
    ID2D1RenderTarget_Release(ctx->rt);
    ref = ID2D1Factory_Release(ctx->factory);
    ok_(__FILE__, line)(!ref, "Factory has %lu references left.\n", ref);

    IDXGISurface_Release(ctx->surface);
    IDXGISwapChain_Release(ctx->swapchain);
    DestroyWindow(ctx->window);
    ref = IDXGIDevice_Release(ctx->device);
    ok_(__FILE__, line)(!ref, "Device has %lu references left.\n", ref);
}

#define init_test_context(ctx, d3d11) init_test_context_(__LINE__, ctx, d3d11, D2D1_FACTORY_TYPE_SINGLE_THREADED)
#define init_test_multithreaded_context(ctx, d3d11) init_test_context_(__LINE__, ctx, d3d11, D2D1_FACTORY_TYPE_MULTI_THREADED)
static BOOL init_test_context_(unsigned int line, struct d2d1_test_context *ctx, BOOL d3d11, D2D1_FACTORY_TYPE factory_type)
{
    HRESULT hr;

    memset(ctx, 0, sizeof(*ctx));

    ctx->d3d11 = d3d11;
    if (!(ctx->device = create_device(d3d11)))
    {
        skip_(__FILE__, line)("Failed to create device, skipping tests.\n");
        return FALSE;
    }

    ctx->window = create_window();
    ok_(__FILE__, line)(!!ctx->window, "Failed to create test window.\n");
    ctx->swapchain = create_swapchain(ctx->device, ctx->window, TRUE);
    ok_(__FILE__, line)(!!ctx->swapchain, "Failed to create swapchain.\n");
    hr = IDXGISwapChain_GetBuffer(ctx->swapchain, 0, &IID_IDXGISurface, (void **)&ctx->surface);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get buffer, hr %#lx.\n", hr);

    ctx->rt = create_render_target(ctx->surface, d3d11, factory_type);
    if (!ctx->rt && d3d11)
    {
        todo_wine win_skip_(__FILE__, line)("Skipping d3d11 tests.\n");

        IDXGISurface_Release(ctx->surface);
        IDXGISwapChain_Release(ctx->swapchain);
        DestroyWindow(ctx->window);
        IDXGIDevice_Release(ctx->device);

        return FALSE;
    }
    ok_(__FILE__, line)(!!ctx->rt, "Failed to create render target.\n");

    hr = ID2D1RenderTarget_QueryInterface(ctx->rt, &IID_ID2D1DeviceContext, (void **)&ctx->context);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get device context, hr %#lx.\n", hr);

    ID2D1RenderTarget_GetFactory(ctx->rt, &ctx->factory);
    ID2D1Factory_QueryInterface(ctx->factory, &IID_ID2D1Factory1, (void **)&ctx->factory1);
    ID2D1Factory_QueryInterface(ctx->factory, &IID_ID2D1Factory2, (void **)&ctx->factory2);
    ID2D1Factory_QueryInterface(ctx->factory, &IID_ID2D1Factory3, (void **)&ctx->factory3);

    return TRUE;
}

#define check_bitmap_options(b, o) check_bitmap_options_(__LINE__, b, o)
static void check_bitmap_options_(unsigned int line, ID2D1Bitmap *bitmap, DWORD expected_options)
{
    D2D1_BITMAP_OPTIONS options;
    ID2D1Bitmap1 *bitmap1;
    HRESULT hr;

    hr = ID2D1Bitmap_QueryInterface(bitmap, &IID_ID2D1Bitmap1, (void **)&bitmap1);
    if (FAILED(hr))
        return;

    options = ID2D1Bitmap1_GetOptions(bitmap1);
    ok_(__FILE__, line)(options == expected_options, "Got unexpected bitmap options %#x, expected %#lx.\n",
            options, expected_options);

    ID2D1Bitmap1_Release(bitmap1);
}

#define check_bitmap_surface(b, s, o) check_bitmap_surface_(__LINE__, b, s, o)
static void check_bitmap_surface_(unsigned int line, ID2D1Bitmap *bitmap, BOOL has_surface, DWORD expected_options)
{
    D2D1_BITMAP_OPTIONS options;
    IDXGISurface *surface;
    ID2D1Bitmap1 *bitmap1;
    HRESULT hr;

    hr = ID2D1Bitmap_QueryInterface(bitmap, &IID_ID2D1Bitmap1, (void **)&bitmap1);
    if (FAILED(hr))
        return;

    options = ID2D1Bitmap1_GetOptions(bitmap1);
    ok_(__FILE__, line)(options == expected_options, "Got unexpected bitmap options %#x, expected %#lx.\n",
            options, expected_options);

    surface = (void *)0xdeadbeef;
    hr = ID2D1Bitmap1_GetSurface(bitmap1, &surface);
    if (has_surface)
    {
        unsigned int bind_flags = 0, misc_flags = 0;
        D3D10_TEXTURE2D_DESC desc;
        ID3D10Texture2D *texture;
        D2D1_SIZE_U pixel_size;

        ok_(__FILE__, line)(hr == S_OK, "Failed to get bitmap surface, hr %#lx.\n", hr);
        ok_(__FILE__, line)(!!surface, "Expected surface instance.\n");

        /* Correlate with resource configuration. */
        hr = IDXGISurface_QueryInterface(surface, &IID_ID3D10Texture2D, (void **)&texture);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get texture pointer, hr %#lx.\n", hr);

        ID3D10Texture2D_GetDesc(texture, &desc);
        ok_(__FILE__, line)(desc.Usage == 0, "Unexpected usage %#x.\n", desc.Usage);

        if (options & D2D1_BITMAP_OPTIONS_TARGET)
            bind_flags |= D3D10_BIND_RENDER_TARGET;
        if (!(options & D2D1_BITMAP_OPTIONS_CANNOT_DRAW))
            bind_flags |= D3D10_BIND_SHADER_RESOURCE;
        if (options & D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE)
            misc_flags |= D3D10_RESOURCE_MISC_GDI_COMPATIBLE;

        ok_(__FILE__, line)(desc.BindFlags == bind_flags, "Unexpected bind flags %#x for bitmap options %#x.\n",
                desc.BindFlags, options);
        ok_(__FILE__, line)(!desc.CPUAccessFlags, "Unexpected cpu access flags %#x.\n", desc.CPUAccessFlags);
        ok_(__FILE__, line)(desc.MiscFlags == misc_flags, "Unexpected misc flags %#x for bitmap options %#x.\n",
                desc.MiscFlags, options);

        pixel_size = ID2D1Bitmap_GetPixelSize(bitmap);
        if (!pixel_size.width || !pixel_size.height)
            pixel_size.width = pixel_size.height = 1;
        ok_(__FILE__, line)(desc.Width == pixel_size.width, "Got width %u, expected %u.\n",
                desc.Width, pixel_size.width);
        ok_(__FILE__, line)(desc.Height == pixel_size.height, "Got height %u, expected %u.\n",
                desc.Height, pixel_size.height);

        ID3D10Texture2D_Release(texture);

        IDXGISurface_Release(surface);
    }
    else
    {
        ok_(__FILE__, line)(hr == D2DERR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        ok_(__FILE__, line)(!surface, "Unexpected surface instance.\n");
    }

    ID2D1Bitmap1_Release(bitmap1);
}

static inline struct geometry_sink *impl_from_ID2D1SimplifiedGeometrySink(ID2D1SimplifiedGeometrySink *iface)
{
    return CONTAINING_RECORD(iface, struct geometry_sink, ID2D1SimplifiedGeometrySink_iface);
}

static HRESULT STDMETHODCALLTYPE geometry_sink_QueryInterface(ID2D1SimplifiedGeometrySink *iface,
        REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_ID2D1SimplifiedGeometrySink)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE geometry_sink_AddRef(ID2D1SimplifiedGeometrySink *iface)
{
    return 0;
}

static ULONG STDMETHODCALLTYPE geometry_sink_Release(ID2D1SimplifiedGeometrySink *iface)
{
    return 0;
}

static void STDMETHODCALLTYPE geometry_sink_SetFillMode(ID2D1SimplifiedGeometrySink *iface, D2D1_FILL_MODE mode)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);

    sink->fill_mode = mode;
}

static void STDMETHODCALLTYPE geometry_sink_SetSegmentFlags(ID2D1SimplifiedGeometrySink *iface,
        D2D1_PATH_SEGMENT flags)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);

    sink->segment_flags = flags;
}

static void STDMETHODCALLTYPE geometry_sink_BeginFigure(ID2D1SimplifiedGeometrySink *iface,
        D2D1_POINT_2F start_point, D2D1_FIGURE_BEGIN figure_begin)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure;

    if (sink->figure_count == sink->figures_size)
    {
        sink->figures_size *= 2;
        sink->figures = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sink->figures,
                sink->figures_size * sizeof(*sink->figures));
    }
    figure = &sink->figures[sink->figure_count++];

    figure->begin = figure_begin;
    figure->start_point = start_point;
    figure->segments_size = 4;
    figure->segments = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            figure->segments_size * sizeof(*figure->segments));
}

static struct geometry_segment *geometry_figure_add_segment(struct geometry_figure *figure)
{
    if (figure->segment_count == figure->segments_size)
    {
        figure->segments_size *= 2;
        figure->segments = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, figure->segments,
                figure->segments_size * sizeof(*figure->segments));
    }
    return &figure->segments[figure->segment_count++];
}

static void STDMETHODCALLTYPE geometry_sink_AddLines(ID2D1SimplifiedGeometrySink *iface,
        const D2D1_POINT_2F *points, UINT32 count)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure = &sink->figures[sink->figure_count - 1];
    struct geometry_segment *segment;
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        segment = geometry_figure_add_segment(figure);
        segment->type = SEGMENT_LINE;
        segment->u.line = points[i];
        segment->flags = sink->segment_flags;
    }
}

static void STDMETHODCALLTYPE geometry_sink_AddBeziers(ID2D1SimplifiedGeometrySink *iface,
        const D2D1_BEZIER_SEGMENT *beziers, UINT32 count)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure = &sink->figures[sink->figure_count - 1];
    struct geometry_segment *segment;
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        segment = geometry_figure_add_segment(figure);
        segment->type = SEGMENT_BEZIER;
        segment->u.bezier = beziers[i];
        segment->flags = sink->segment_flags;
    }
}

static void STDMETHODCALLTYPE geometry_sink_EndFigure(ID2D1SimplifiedGeometrySink *iface,
        D2D1_FIGURE_END figure_end)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure = &sink->figures[sink->figure_count - 1];

    figure->end = figure_end;
}

static HRESULT STDMETHODCALLTYPE geometry_sink_Close(ID2D1SimplifiedGeometrySink *iface)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);

    sink->closed = TRUE;

    return S_OK;
}

static const struct ID2D1SimplifiedGeometrySinkVtbl geometry_sink_vtbl =
{
    geometry_sink_QueryInterface,
    geometry_sink_AddRef,
    geometry_sink_Release,
    geometry_sink_SetFillMode,
    geometry_sink_SetSegmentFlags,
    geometry_sink_BeginFigure,
    geometry_sink_AddLines,
    geometry_sink_AddBeziers,
    geometry_sink_EndFigure,
    geometry_sink_Close,
};

static void geometry_sink_init(struct geometry_sink *sink)
{
    memset(sink, 0, sizeof(*sink));
    sink->ID2D1SimplifiedGeometrySink_iface.lpVtbl = &geometry_sink_vtbl;
    sink->figures_size = 4;
    sink->figures = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sink->figures_size * sizeof(*sink->figures));
}

static void geometry_sink_cleanup(struct geometry_sink *sink)
{
    unsigned int i;

    for (i = 0; i < sink->figure_count; ++i)
    {
        HeapFree(GetProcessHeap(), 0, sink->figures[i].segments);
    }
    HeapFree(GetProcessHeap(), 0, sink->figures);
}

#define geometry_sink_check(a, b, c, d, e) geometry_sink_check_(__LINE__, a, b, c, d, e)
static void geometry_sink_check_(unsigned int line, const struct geometry_sink *sink, D2D1_FILL_MODE fill_mode,
        unsigned int figure_count, const struct expected_geometry_figure *expected_figures, unsigned int ulps)
{
    const struct geometry_segment *segment, *expected_segment;
    const struct expected_geometry_figure *expected_figure;
    const struct geometry_figure *figure;
    unsigned int i, j;
    unsigned int segment_count;
    BOOL match;

    ok_(__FILE__, line)(sink->fill_mode == fill_mode,
            "Got unexpected fill mode %#x.\n", sink->fill_mode);
    ok_(__FILE__, line)(sink->figure_count == figure_count,
            "Got unexpected figure count %u, expected %u.\n", sink->figure_count, figure_count);
    ok_(__FILE__, line)(!sink->closed, "Sink is closed.\n");

    for (i = 0; i < figure_count; ++i)
    {
        expected_figure = &expected_figures[i];
        figure = &sink->figures[i];

        ok_(__FILE__, line)(figure->begin == expected_figure->begin,
                "Got unexpected figure %u begin %#x, expected %#x.\n",
                i, figure->begin, expected_figure->begin);
        ok_(__FILE__, line)(figure->end == expected_figure->end,
                "Got unexpected figure %u end %#x, expected %#x.\n",
                i, figure->end, expected_figure->end);
        match = compare_point(&figure->start_point,
                expected_figure->start_point.x, expected_figure->start_point.y, ulps);
        ok_(__FILE__, line)(match, "Got unexpected figure %u start point {%.8e, %.8e}, expected {%.8e, %.8e}.\n",
                i, figure->start_point.x, figure->start_point.y,
                expected_figure->start_point.x, expected_figure->start_point.y);
        ok_(__FILE__, line)(figure->segment_count == expected_figure->segment_count,
                "Got unexpected figure %u segment count %u, expected %u.\n",
                i, figure->segment_count, expected_figure->segment_count);

        segment_count = expected_figure->segment_count < figure->segment_count ?
            expected_figure->segment_count : figure->segment_count;

        for (j = 0; j < segment_count; ++j)
        {
            expected_segment = &expected_figure->segments[j];
            segment = &figure->segments[j];
            ok_(__FILE__, line)(segment->type == expected_segment->type,
                    "Got unexpected figure %u, segment %u type %#x, expected %#x.\n",
                    i, j, segment->type, expected_segment->type);
            ok_(__FILE__, line)(segment->flags == expected_segment->flags,
                    "Got unexpected figure %u, segment %u flags %#lx, expected %#lx.\n",
                    i, j, segment->flags, expected_segment->flags);
            switch (segment->type)
            {
                case SEGMENT_LINE:
                    match = compare_point(&segment->u.line,
                            expected_segment->u.line.x, expected_segment->u.line.y, ulps);
                    ok_(__FILE__, line)(match, "Got unexpected figure %u segment %u {%.8e, %.8e}, "
                            "expected {%.8e, %.8e}.\n",
                            i, j, segment->u.line.x, segment->u.line.y,
                            expected_segment->u.line.x, expected_segment->u.line.y);
                    break;

                case SEGMENT_BEZIER:
                    match = compare_bezier_segment(&segment->u.bezier,
                            expected_segment->u.bezier.point1.x, expected_segment->u.bezier.point1.y,
                            expected_segment->u.bezier.point2.x, expected_segment->u.bezier.point2.y,
                            expected_segment->u.bezier.point3.x, expected_segment->u.bezier.point3.y,
                            ulps);
                    ok_(__FILE__, line)(match, "Got unexpected figure %u segment %u "
                            "{%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}, "
                            "expected {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
                            i, j, segment->u.bezier.point1.x, segment->u.bezier.point1.y,
                            segment->u.bezier.point2.x, segment->u.bezier.point2.y,
                            segment->u.bezier.point3.x, segment->u.bezier.point3.y,
                            expected_segment->u.bezier.point1.x, expected_segment->u.bezier.point1.y,
                            expected_segment->u.bezier.point2.x, expected_segment->u.bezier.point2.y,
                            expected_segment->u.bezier.point3.x, expected_segment->u.bezier.point3.y);
                    break;
            }
        }

        for (j = segment_count; j < expected_figure->segment_count; ++j)
        {
            segment = &expected_figure->segments[j];
            switch (segment->type)
            {
                case SEGMENT_LINE:
                    ok_(__FILE__, line)(FALSE, "Missing figure %u segment %u {%.8e, %.8e}.\n",
                            i, j, segment->u.line.x, segment->u.line.y);
                    break;
                case SEGMENT_BEZIER:
                    ok_(__FILE__, line)(FALSE, "Missing figure %u segment %u "
                            "{%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}\n",
                            i, j, segment->u.bezier.point1.x, segment->u.bezier.point1.y,
                            segment->u.bezier.point2.x, segment->u.bezier.point2.y,
                            segment->u.bezier.point3.x, segment->u.bezier.point3.y);
                    break;
            }
        }

        for (j = segment_count; j < figure->segment_count; ++j)
        {
            segment = &figure->segments[j];
            switch (segment->type)
            {
                case SEGMENT_LINE:
                    ok_(__FILE__, line)(FALSE, "Got unexpected figure %u segment %u {%.8e, %.8e}.\n",
                            i, j, segment->u.line.x, segment->u.line.y);
                    break;
                case SEGMENT_BEZIER:
                    ok_(__FILE__, line)(FALSE, "Got unexpected figure %u segment %u "
                            "{%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}\n",
                            i, j, segment->u.bezier.point1.x, segment->u.bezier.point1.y,
                            segment->u.bezier.point2.x, segment->u.bezier.point2.y,
                            segment->u.bezier.point3.x, segment->u.bezier.point3.y);
                    break;
            }
        }
    }
}

static void test_clip(BOOL d3d11)
{
    struct d2d1_test_context ctx;
    D2D1_MATRIX_3X2_F matrix;
    D2D1_SIZE_U pixel_size;
    ID2D1RenderTarget *rt;
    D2D1_POINT_2F point;
    D2D1_COLOR_F color;
    float dpi_x, dpi_y;
    D2D1_RECT_F rect;
    D2D1_SIZE_F size;
    HRESULT hr;
    BOOL match;
    static const D2D1_MATRIX_3X2_F identity =
    {{{
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    }}};

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_y);
    size = ID2D1RenderTarget_GetSize(rt);
    ok(size.width == 640.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 480.0f, "Got unexpected height %.8e.\n", size.height);
    pixel_size = ID2D1RenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == 640, "Got unexpected width %u.\n", pixel_size.width);
    ok(pixel_size.height == 480, "Got unexpected height %u.\n", pixel_size.height);

    ID2D1RenderTarget_GetTransform(rt, &matrix);
    ok(!memcmp(&matrix, &identity, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 1.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    ID2D1RenderTarget_SetDpi(rt, 48.0f, 192.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 48.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_y);
    size = ID2D1RenderTarget_GetSize(rt);
    ok(size.width == 1280.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 240.0f, "Got unexpected height %.8e.\n", size.height);
    pixel_size = ID2D1RenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == 640, "Got unexpected width %u.\n", pixel_size.width);
    ok(pixel_size.height == 480, "Got unexpected height %u.\n", pixel_size.height);

    /* The effective clip rect is the intersection of all currently pushed
     * clip rects. Clip rects are in DIPs. */
    set_rect(&rect, 0.0f, 0.0f, 1280.0f, 80.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    set_rect(&rect, 0.0f, 0.0f, 426.0f, 240.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);

    set_color(&color, 0.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetDpi(rt, 0.0f, 0.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 96.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 192.0f);
    ID2D1RenderTarget_SetDpi(rt, 0.0f, 96.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, -10.0f, 96.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, 96.0f, -10.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, 96.0f, 0.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, 96.0f, 96.0f);

    /* Transformations apply to clip rects, the effective clip rect is the
     * (axis-aligned) bounding box of the transformed clip rect. */
    set_point(&point, 320.0f, 240.0f);
    D2D1MakeRotateMatrix(30.0f, point, &matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 215.0f, 208.0f, 425.0f, 272.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 1.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    /* Transformations are applied when pushing the clip rect, transformations
     * set afterwards have no effect on the current clip rect. This includes
     * SetDpi(). */
    ID2D1RenderTarget_SetTransform(rt, &identity);
    set_rect(&rect, 427.0f, 320.0f, 640.0f, 480.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RenderTarget_SetDpi(rt, 48.0f, 192.0f);
    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "035a44d4198d6e422e9de6185b5b2c2bac5e33c9");
    ok(match, "Surface does not match.\n");

    /* Fractional clip rectangle coordinates, aliased mode. */
    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RenderTarget_SetDpi(rt, 96.0f, 96.0f);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    scale_matrix(&matrix, 2.0f, 2.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 0.0f, 0.5f, 200.0f, 100.5f);
    set_color(&color, 1.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 0.0f, 0.5f, 100.0f, 200.5f);
    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 0.5f, 250.0f, 100.5f, 300.0f);
    set_color(&color, 1.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    translate_matrix(&matrix, 0.1f, 0.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 110.0f, 250.25f, 150.0f, 300.25f);
    set_color(&color, 0.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    set_rect(&rect, 160.0f, 250.75f, 200.0f, 300.75f);
    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetDpi(rt, 48.0f, 192.0f);
    set_rect(&rect, 160.25f, 0.0f, 200.25f, 100.0f);
    set_color(&color, 1.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    set_rect(&rect, 160.75f, 100.0f, 200.75f, 120.0f);
    set_color(&color, 0.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "cb418ec4a7c8407b5e36db06fc6292a06bb8476c");
    ok(match, "Surface does not match.\n");

    release_test_context(&ctx);
}

static void test_state_block(BOOL d3d11)
{
    IDWriteRenderingParams *text_rendering_params1, *text_rendering_params2;
    D2D1_DRAWING_STATE_DESCRIPTION drawing_state;
    ID2D1DrawingStateBlock *state_block;
    IDWriteFactory *dwrite_factory;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory1;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    ULONG refcount;
    HRESULT hr;
    void *ptr;
    static const D2D1_MATRIX_3X2_F identity =
    {{{
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    }}};
    static const D2D1_MATRIX_3X2_F transform1 =
    {{{
        1.0f, 2.0f,
        3.0f, 4.0f,
        5.0f, 6.0f,
    }}};
    static const D2D1_MATRIX_3X2_F transform2 =
    {{{
        7.0f,  8.0f,
        9.0f,  10.0f,
        11.0f, 12.0f,
    }}};

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    factory = ctx.factory;
    factory1 = ctx.factory1;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown **)&dwrite_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDWriteFactory_CreateRenderingParams(dwrite_factory, &text_rendering_params1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDWriteFactory_Release(dwrite_factory);

    drawing_state.antialiasMode = ID2D1RenderTarget_GetAntialiasMode(rt);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    drawing_state.textAntialiasMode = ID2D1RenderTarget_GetTextAntialiasMode(rt);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ID2D1RenderTarget_GetTags(rt, &drawing_state.tag1, &drawing_state.tag2);
    ok(!drawing_state.tag1 && !drawing_state.tag2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ID2D1RenderTarget_GetTransform(rt, &drawing_state.transform);
    ok(!memcmp(&drawing_state.transform, &identity, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1RenderTarget_GetTextRenderingParams(rt, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);

    hr = ID2D1Factory_CreateDrawingStateBlock(factory, NULL, NULL, &state_block);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(!drawing_state.tag1 && !drawing_state.tag2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &identity, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);
    ID2D1DrawingStateBlock_Release(state_block);

    drawing_state.antialiasMode = D2D1_ANTIALIAS_MODE_ALIASED;
    drawing_state.textAntialiasMode = D2D1_TEXT_ANTIALIAS_MODE_ALIASED;
    drawing_state.tag1 = 0xdead;
    drawing_state.tag2 = 0xbeef;
    drawing_state.transform = transform1;
    hr = ID2D1Factory_CreateDrawingStateBlock(factory, &drawing_state, text_rendering_params1, &state_block);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_ALIASED,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(drawing_state.tag1 == 0xdead && drawing_state.tag2 == 0xbeef, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &transform1, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
            text_rendering_params2, text_rendering_params1);
    IDWriteRenderingParams_Release(text_rendering_params2);

    ID2D1RenderTarget_RestoreDrawingState(rt, state_block);

    drawing_state.antialiasMode = ID2D1RenderTarget_GetAntialiasMode(rt);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    drawing_state.textAntialiasMode = ID2D1RenderTarget_GetTextAntialiasMode(rt);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_ALIASED,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ID2D1RenderTarget_GetTags(rt, &drawing_state.tag1, &drawing_state.tag2);
    ok(drawing_state.tag1 == 0xdead && drawing_state.tag2 == 0xbeef, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ID2D1RenderTarget_GetTransform(rt, &drawing_state.transform);
    ok(!memcmp(&drawing_state.transform, &transform1, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1RenderTarget_GetTextRenderingParams(rt, &text_rendering_params2);
    ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
            text_rendering_params2, text_rendering_params1);
    IDWriteRenderingParams_Release(text_rendering_params2);

    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    ID2D1RenderTarget_SetTextAntialiasMode(rt, D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    ID2D1RenderTarget_SetTags(rt, 1, 2);
    ID2D1RenderTarget_SetTransform(rt, &transform2);
    ID2D1RenderTarget_SetTextRenderingParams(rt, NULL);

    drawing_state.antialiasMode = ID2D1RenderTarget_GetAntialiasMode(rt);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    drawing_state.textAntialiasMode = ID2D1RenderTarget_GetTextAntialiasMode(rt);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ID2D1RenderTarget_GetTags(rt, &drawing_state.tag1, &drawing_state.tag2);
    ok(drawing_state.tag1 == 1 && drawing_state.tag2 == 2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ID2D1RenderTarget_GetTransform(rt, &drawing_state.transform);
    ok(!memcmp(&drawing_state.transform, &transform2, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1RenderTarget_GetTextRenderingParams(rt, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);

    ID2D1RenderTarget_SaveDrawingState(rt, state_block);

    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(drawing_state.tag1 == 1 && drawing_state.tag2 == 2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &transform2, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);

    drawing_state.antialiasMode = D2D1_ANTIALIAS_MODE_ALIASED;
    drawing_state.textAntialiasMode = D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;
    drawing_state.tag1 = 3;
    drawing_state.tag2 = 4;
    drawing_state.transform = transform1;
    ID2D1DrawingStateBlock_SetDescription(state_block, &drawing_state);
    ID2D1DrawingStateBlock_SetTextRenderingParams(state_block, text_rendering_params1);

    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(drawing_state.tag1 == 3 && drawing_state.tag2 == 4, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &transform1, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
            text_rendering_params2, text_rendering_params1);
    IDWriteRenderingParams_Release(text_rendering_params2);

    if (factory1)
    {
        D2D1_DRAWING_STATE_DESCRIPTION1 drawing_state1;
        ID2D1DrawingStateBlock1 *state_block1;

        hr = ID2D1DrawingStateBlock_QueryInterface(state_block, &IID_ID2D1DrawingStateBlock1, (void **)&state_block1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
                "Got unexpected antialias mode %#x.\n", drawing_state1.antialiasMode);
        ok(drawing_state1.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE,
                "Got unexpected text antialias mode %#x.\n", drawing_state1.textAntialiasMode);
        ok(drawing_state1.tag1 == 3 && drawing_state1.tag2 == 4, "Got unexpected tags %s:%s.\n",
                wine_dbgstr_longlong(drawing_state1.tag1), wine_dbgstr_longlong(drawing_state1.tag2));
        ok(!memcmp(&drawing_state1.transform, &transform1, sizeof(drawing_state1.transform)),
                "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
                drawing_state1.transform._11, drawing_state1.transform._12, drawing_state1.transform._21,
                drawing_state1.transform._22, drawing_state1.transform._31, drawing_state1.transform._32);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_SOURCE_OVER,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_DIPS, "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);
        ID2D1DrawingStateBlock1_GetTextRenderingParams(state_block1, &text_rendering_params2);
        ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
                text_rendering_params2, text_rendering_params1);
        IDWriteRenderingParams_Release(text_rendering_params2);

        drawing_state1.primitiveBlend = D2D1_PRIMITIVE_BLEND_COPY;
        drawing_state1.unitMode = D2D1_UNIT_MODE_PIXELS;
        ID2D1DrawingStateBlock1_SetDescription(state_block1, &drawing_state1);
        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_COPY,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_PIXELS,
                "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);

        ID2D1DrawingStateBlock_SetDescription(state_block, &drawing_state);
        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_COPY,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_PIXELS,
                "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);

        ID2D1DrawingStateBlock1_Release(state_block1);

        hr = ID2D1Factory1_CreateDrawingStateBlock(factory1, NULL, NULL, &state_block1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                "Got unexpected antialias mode %#x.\n", drawing_state1.antialiasMode);
        ok(drawing_state1.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
                "Got unexpected text antialias mode %#x.\n", drawing_state1.textAntialiasMode);
        ok(drawing_state1.tag1 == 0 && drawing_state1.tag2 == 0, "Got unexpected tags %s:%s.\n",
                wine_dbgstr_longlong(drawing_state1.tag1), wine_dbgstr_longlong(drawing_state1.tag2));
        ok(!memcmp(&drawing_state1.transform, &identity, sizeof(drawing_state1.transform)),
                "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
                drawing_state1.transform._11, drawing_state1.transform._12, drawing_state1.transform._21,
                drawing_state1.transform._22, drawing_state1.transform._31, drawing_state1.transform._32);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_SOURCE_OVER,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_DIPS, "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);
        ID2D1DrawingStateBlock1_GetTextRenderingParams(state_block1, &text_rendering_params2);
        ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);
        ID2D1DrawingStateBlock1_Release(state_block1);
    }

    ID2D1DrawingStateBlock_Release(state_block);

    refcount = IDWriteRenderingParams_Release(text_rendering_params1);
    ok(!refcount, "Rendering params %lu references left.\n", refcount);

    /* State block object pointer is validated, but does not result in an error state. */
    ptr = (void *)0xdeadbeef;
    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_SaveDrawingState(rt, (ID2D1DrawingStateBlock *)&ptr);
    ID2D1RenderTarget_RestoreDrawingState(rt, (ID2D1DrawingStateBlock *)&ptr);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    release_test_context(&ctx);
}

static void test_color_brush(BOOL d3d11)
{
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    D2D1_BRUSH_PROPERTIES brush_desc;
    D2D1_COLOR_F color, tmp_color;
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1RenderTarget *rt;
    D2D1_RECT_F rect;
    float opacity;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    opacity = ID2D1SolidColorBrush_GetOpacity(brush);
    ok(opacity == 1.0f, "Got unexpected opacity %.8e.\n", opacity);
    set_matrix_identity(&matrix);
    ID2D1SolidColorBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    tmp_color = ID2D1SolidColorBrush_GetColor(brush);
    ok(!memcmp(&tmp_color, &color, sizeof(color)),
            "Got unexpected color {%.8e, %.8e, %.8e, %.8e}.\n",
            tmp_color.r, tmp_color.g, tmp_color.b, tmp_color.a);
    ID2D1SolidColorBrush_Release(brush);

    set_color(&color, 0.0f, 1.0f, 0.0f, 0.8f);
    brush_desc.opacity = 0.3f;
    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 2.0f, 2.0f);
    brush_desc.transform = matrix;
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, &brush_desc, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    opacity = ID2D1SolidColorBrush_GetOpacity(brush);
    ok(opacity == 0.3f, "Got unexpected opacity %.8e.\n", opacity);
    ID2D1SolidColorBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    tmp_color = ID2D1SolidColorBrush_GetColor(brush);
    ok(!memcmp(&tmp_color, &color, sizeof(color)),
            "Got unexpected color {%.8e, %.8e, %.8e, %.8e}.\n",
            tmp_color.r, tmp_color.g, tmp_color.b, tmp_color.a);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    ID2D1SolidColorBrush_SetOpacity(brush, 1.0f);
    set_rect(&rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_color(&color, 1.0f, 0.0f, 0.0f, 0.625f);
    ID2D1SolidColorBrush_SetColor(brush, &color);
    ID2D1SolidColorBrush_SetOpacity(brush, 0.75f);
    set_rect(&rect, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "6d1218fca5e21fb7e287b3a439d60dbc251f5ceb");
    ok(match, "Surface does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    release_test_context(&ctx);
}

static void test_bitmap_brush(BOOL d3d11)
{
    D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode;
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *rectangle_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1Bitmap *bitmap, *tmp_bitmap;
    D2D1_RECT_F src_rect, dst_rect;
    struct d2d1_test_context ctx;
    D2D1_EXTEND_MODE extend_mode;
    ID2D1BitmapBrush1 *brush1;
    ID2D1BitmapBrush *brush;
    D2D1_SIZE_F image_size;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F color;
    ID2D1Image *image;
    D2D1_SIZE_U size;
    unsigned int i;
    ULONG refcount;
    float opacity;
    HRESULT hr;
    BOOL match;

    static const struct
    {
        D2D1_EXTEND_MODE extend_mode_x;
        D2D1_EXTEND_MODE extend_mode_y;
        float translate_x;
        float translate_y;
        D2D1_RECT_F rect;
    }
    extend_mode_tests[] =
    {
        {D2D1_EXTEND_MODE_MIRROR, D2D1_EXTEND_MODE_MIRROR, -7.0f, 1.0f, {-4.0f,  0.0f, -8.0f,  4.0f}},
        {D2D1_EXTEND_MODE_WRAP,   D2D1_EXTEND_MODE_MIRROR, -3.0f, 1.0f, {-4.0f,  4.0f,  0.0f,  0.0f}},
        {D2D1_EXTEND_MODE_CLAMP,  D2D1_EXTEND_MODE_MIRROR,  1.0f, 1.0f, { 4.0f,  0.0f,  0.0f,  4.0f}},
        {D2D1_EXTEND_MODE_MIRROR, D2D1_EXTEND_MODE_WRAP,   -7.0f, 5.0f, {-8.0f,  8.0f, -4.0f,  4.0f}},
        {D2D1_EXTEND_MODE_WRAP,   D2D1_EXTEND_MODE_WRAP,   -3.0f, 5.0f, { 0.0f,  4.0f, -4.0f,  8.0f}},
        {D2D1_EXTEND_MODE_CLAMP,  D2D1_EXTEND_MODE_WRAP,    1.0f, 5.0f, { 0.0f,  8.0f,  4.0f,  4.0f}},
        {D2D1_EXTEND_MODE_MIRROR, D2D1_EXTEND_MODE_CLAMP,  -7.0f, 9.0f, {-4.0f,  8.0f, -8.0f, 12.0f}},
        {D2D1_EXTEND_MODE_WRAP,   D2D1_EXTEND_MODE_CLAMP,  -3.0f, 9.0f, {-4.0f, 12.0f,  0.0f,  8.0f}},
        {D2D1_EXTEND_MODE_CLAMP,  D2D1_EXTEND_MODE_CLAMP,   1.0f, 9.0f, { 4.0f,  8.0f,  0.0f, 12.0f}},
    };
    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0xffffff00, 0xff00ff00, 0xff00ffff,
        0xff0000ff, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    image_size = ID2D1Bitmap_GetSize(bitmap);

    hr = ID2D1Bitmap_QueryInterface(bitmap, &IID_ID2D1Image, (void **)&image);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Vista */, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ID2D1DeviceContext *context = ctx.context;
        D2D1_POINT_2F offset;
        D2D1_RECT_F src_rect;

        ID2D1RenderTarget_BeginDraw(rt);
        set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
        ID2D1RenderTarget_Clear(rt, &color);

        ID2D1RenderTarget_GetTransform(rt, &tmp_matrix);
        set_matrix_identity(&matrix);
        translate_matrix(&matrix, 20.0f, 12.0f);
        scale_matrix(&matrix, 2.0f, 6.0f);
        ID2D1RenderTarget_SetTransform(rt, &matrix);

        /* Crash on Windows 7+ */
        if (0)
        {
            ID2D1DeviceContext_DrawImage(context, NULL, NULL, NULL, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                    D2D1_COMPOSITE_MODE_SOURCE_OVER);
        }

        ID2D1DeviceContext_DrawImage(context, image, NULL, NULL, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        set_rect(&src_rect, 0.0f, 0.0f, image_size.width, image_size.height);

        ID2D1DeviceContext_DrawImage(context, image, NULL, &src_rect, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = -1;
        offset.y = -1;
        ID2D1DeviceContext_DrawImage(context, image, &offset, NULL, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 2;
        offset.y = image_size.height;
        ID2D1DeviceContext_DrawImage(context, image, &offset, NULL, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 3;
        set_rect(&src_rect, image_size.width / 2, image_size.height / 2, image_size.width, image_size.height);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 4;
        set_rect(&src_rect, 0.0f, 0.0f, image_size.width * 2, image_size.height * 2);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 5;
        set_rect(&src_rect, image_size.width, image_size.height, 0.0f, 0.0f);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        match = compare_surface(&ctx, "95675fbc4a16404c9568d41b14e8f6be64240998");
        ok(match, "Surface does not match.\n");

        ID2D1RenderTarget_BeginDraw(rt);

        offset.x = image_size.width * 6;
        set_rect(&src_rect, 1.0f, 0.0f, 1.0f, image_size.height);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        match = compare_surface(&ctx, "95675fbc4a16404c9568d41b14e8f6be64240998");
        ok(match, "Surface does not match.\n");

        ID2D1RenderTarget_SetTransform(rt, &tmp_matrix);
        ID2D1Image_Release(image);
    }

    /* Creating a brush with a NULL bitmap crashes on Vista, but works fine on
     * Windows 7+. */
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_GetBitmap(brush, &tmp_bitmap);
    ok(tmp_bitmap == bitmap, "Got unexpected bitmap %p, expected %p.\n", tmp_bitmap, bitmap);
    ID2D1Bitmap_Release(tmp_bitmap);
    opacity = ID2D1BitmapBrush_GetOpacity(brush);
    ok(opacity == 1.0f, "Got unexpected opacity %.8e.\n", opacity);
    set_matrix_identity(&matrix);
    ID2D1BitmapBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    extend_mode = ID2D1BitmapBrush_GetExtendModeX(brush);
    ok(extend_mode == D2D1_EXTEND_MODE_CLAMP, "Got unexpected extend mode %#x.\n", extend_mode);
    extend_mode = ID2D1BitmapBrush_GetExtendModeY(brush);
    ok(extend_mode == D2D1_EXTEND_MODE_CLAMP, "Got unexpected extend mode %#x.\n", extend_mode);
    interpolation_mode = ID2D1BitmapBrush_GetInterpolationMode(brush);
    ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            "Got unexpected interpolation mode %#x.\n", interpolation_mode);
    ID2D1BitmapBrush_Release(brush);

    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 120.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    ID2D1BitmapBrush_SetInterpolationMode(brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&dst_rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &dst_rect, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, -80.0f, -60.0f);
    scale_matrix(&matrix, 64.0f, 32.0f);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    ID2D1BitmapBrush_SetOpacity(brush, 0.75f);
    set_rect(&dst_rect, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &dst_rect, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 120.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, NULL, 0.25f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
    set_rect(&dst_rect, -4.0f, 12.0f, -8.0f, 8.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 0.75f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
    set_rect(&dst_rect, 0.0f, 8.0f, 4.0f, 12.0f);
    set_rect(&src_rect, 2.0f, 1.0f, 4.0f, 3.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);
    set_rect(&dst_rect, 4.0f, 12.0f, 12.0f, 20.0f);
    set_rect(&src_rect, 0.0f, 0.0f, image_size.width * 2, image_size.height * 2);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);
    set_rect(&dst_rect, 4.0f, 8.0f, 12.0f, 12.0f);
    set_rect(&src_rect, image_size.width / 2, image_size.height / 2, image_size.width, image_size.height);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);
    set_rect(&dst_rect, 0.0f, 4.0f, 4.0f, 8.0f);
    set_rect(&src_rect, image_size.width, 0.0f, 0.0f, image_size.height);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "f5d039c280fa33ba05496c9883192a34108efbbe");
    ok(match, "Surface does not match.\n");

    /* Invalid interpolation mode. */
    ID2D1RenderTarget_BeginDraw(rt);

    set_rect(&dst_rect, 4.0f, 8.0f, 8.0f, 12.0f);
    set_rect(&src_rect, 0.0f, 1.0f, image_size.width, 1.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);

    set_rect(&dst_rect, 1.0f, 8.0f, 4.0f, 12.0f);
    set_rect(&src_rect, 2.0f, 1.0f, 4.0f, 3.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR + 1, &src_rect);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "f5d039c280fa33ba05496c9883192a34108efbbe");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&src_rect, image_size.width, 0.0f, 0.0f, image_size.height);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, NULL, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "59043096393570ad800dbcbfdd644394b79493bd");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &color);

    ID2D1BitmapBrush_SetOpacity(brush, 1.0f);
    for (i = 0; i < ARRAY_SIZE(extend_mode_tests); ++i)
    {
        ID2D1BitmapBrush_SetExtendModeX(brush, extend_mode_tests[i].extend_mode_x);
        extend_mode = ID2D1BitmapBrush_GetExtendModeX(brush);
        ok(extend_mode == extend_mode_tests[i].extend_mode_x,
                "Test %u: Got unexpected extend mode %#x, expected %#x.\n",
                i, extend_mode, extend_mode_tests[i].extend_mode_x);
        ID2D1BitmapBrush_SetExtendModeY(brush, extend_mode_tests[i].extend_mode_y);
        extend_mode = ID2D1BitmapBrush_GetExtendModeY(brush);
        ok(extend_mode == extend_mode_tests[i].extend_mode_y,
                "Test %u: Got unexpected extend mode %#x, expected %#x.\n",
                i, extend_mode, extend_mode_tests[i].extend_mode_y);
        set_matrix_identity(&matrix);
        translate_matrix(&matrix, extend_mode_tests[i].translate_x, extend_mode_tests[i].translate_y);
        scale_matrix(&matrix, 0.5f, 0.5f);
        ID2D1BitmapBrush_SetTransform(brush, &matrix);
        ID2D1RenderTarget_FillRectangle(rt, &extend_mode_tests[i].rect, (ID2D1Brush *)brush);
    }

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "b4b775afecdae2d26642001f4faff73663bb8b31");
    ok(match, "Surface does not match.\n");

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.dpiX = 96.0f / 20.0f;
    bitmap_desc.dpiY = 96.0f / 60.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetBitmap(brush, bitmap);

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &color);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 120.0f);
    skew_matrix(&matrix, 0.125f, 2.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_matrix_identity(&matrix);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    set_rect(&dst_rect, 0.0f, 0.0f, 80.0f, 240.0f);
    ID2D1RenderTarget_FillRectangle(rt, &dst_rect, (ID2D1Brush *)brush);

    factory = ctx.factory;

    set_rect(&dst_rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &dst_rect, &rectangle_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 240.0f, 720.0f);
    scale_matrix(&matrix, 40.0f, 120.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)rectangle_geometry,
            &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_Release(rectangle_geometry);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 600.0f);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    ID2D1TransformedGeometry_Release(transformed_geometry);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "cf7b90ba7b139fdfbe9347e1907d635cfb4ed197");
    ok(match, "Surface does not match.\n");

    if (SUCCEEDED(ID2D1BitmapBrush_QueryInterface(brush, &IID_ID2D1BitmapBrush1, (void **)&brush1)))
    {
        D2D1_INTERPOLATION_MODE interpolation_mode1;

        interpolation_mode = ID2D1BitmapBrush1_GetInterpolationMode(brush1);
        ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode);

        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode1(brush1, D2D1_INTERPOLATION_MODE_CUBIC);
        interpolation_mode = ID2D1BitmapBrush1_GetInterpolationMode(brush1);
        ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode);

        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_CUBIC,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode1(brush1, 100);
        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_CUBIC,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode(brush1, 100);
        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_CUBIC,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode(brush1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        interpolation_mode = ID2D1BitmapBrush1_GetInterpolationMode(brush1);
        ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode);

        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_Release(brush1);
    }

    ID2D1BitmapBrush_Release(brush);
    refcount = ID2D1Bitmap_Release(bitmap);
    ok(!refcount, "Bitmap has %lu references left.\n", refcount);
    release_test_context(&ctx);
}

static void test_image_brush(BOOL d3d11)
{
    D2D1_IMAGE_BRUSH_PROPERTIES image_brush_desc;
    D2D1_INTERPOLATION_MODE interp_mode;
    ID2D1DeviceContext *device_context;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    D2D1_BRUSH_PROPERTIES brush_desc;
    ID2D1Image *image, *tmp_image;
    D2D1_RECT_F dst_rect;
    struct d2d1_test_context ctx;
    D2D1_EXTEND_MODE extend_mode;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1ImageBrush *brush;
    ID2D1Bitmap *bitmap;
    D2D1_COLOR_F color;
    D2D1_SIZE_U size;
    D2D1_RECT_F rect;
    ULONG refcount;
    float opacity;
    HRESULT hr;
    BOOL match;

    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0xffffff00, 0xff00ff00, 0xff00ffff,
        0xff0000ff, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const D2D1_MATRIX_3X2_F identity =
    {{{
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    }}};

    if (!init_test_context(&ctx, d3d11))
        return;

    device_context = ctx.context;

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(ctx.rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Bitmap_QueryInterface(bitmap, &IID_ID2D1Image, (void **)&image);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Only image brush description is required. */
    set_rect(&image_brush_desc.sourceRectangle, 1.0f, 2.0f, 3.0f, 4.0f);
    image_brush_desc.extendModeX = D2D1_EXTEND_MODE_WRAP;
    image_brush_desc.extendModeY = D2D1_EXTEND_MODE_MIRROR;
    image_brush_desc.interpolationMode = D2D1_INTERPOLATION_MODE_LINEAR;

    hr = ID2D1DeviceContext_CreateImageBrush(device_context, NULL, &image_brush_desc, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    tmp_image = (void *)0xdeadbeef;
    ID2D1ImageBrush_GetImage(brush, &tmp_image);
    ok(!tmp_image, "Unexpected image %p.\n", image);

    opacity = ID2D1ImageBrush_GetOpacity(brush);
    ok(opacity == 1.0f, "Unexpected opacity %.8e.\n", opacity);
    ID2D1ImageBrush_GetTransform(brush, &matrix);
    ok(!memcmp(&matrix, &identity, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32);

    ID2D1ImageBrush_GetSourceRectangle(brush, &rect);
    match = compare_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    extend_mode = ID2D1ImageBrush_GetExtendModeX(brush);
    ok(extend_mode == D2D1_EXTEND_MODE_WRAP, "Unexpected extend mode %u.\n", extend_mode);
    extend_mode = ID2D1ImageBrush_GetExtendModeY(brush);
    ok(extend_mode == D2D1_EXTEND_MODE_MIRROR, "Unexpected extend mode %u.\n", extend_mode);
    interp_mode = ID2D1ImageBrush_GetInterpolationMode(brush);
    ok(interp_mode == D2D1_INTERPOLATION_MODE_LINEAR, "Unexpected interpolation mode %u.\n", interp_mode);

    ID2D1ImageBrush_Release(brush);

    /* FillRectangle */
    set_rect(&image_brush_desc.sourceRectangle, 0.0f, 0.0f, 4.0f, 4.0f);
    hr = ID2D1DeviceContext_CreateImageBrush(device_context, image, &image_brush_desc, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1ImageBrush_SetInterpolationMode(brush, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

    ID2D1RenderTarget_BeginDraw(ctx.rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(ctx.rt, &color);

    set_rect(&dst_rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(ctx.rt, &dst_rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(ctx.rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "89917481db82e6d683a75f068d3984fe2703cce5");
    ok(match, "Surface does not match.\n");

    ID2D1ImageBrush_Release(brush);

    set_rect(&image_brush_desc.sourceRectangle, 0.0f, 0.0f, 1.0f, 1.0f);
    hr = ID2D1DeviceContext_CreateImageBrush(device_context, image, &image_brush_desc, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1ImageBrush_SetInterpolationMode(brush, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

    ID2D1RenderTarget_BeginDraw(ctx.rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(ctx.rt, &color);

    set_rect(&dst_rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(ctx.rt, &dst_rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(ctx.rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "23544adf9695a51428c194a1cffd531be3416e65");
    todo_wine
    ok(match, "Surface does not match.\n");

    ID2D1ImageBrush_Release(brush);

    /* Custom brush description and image pointer. */
    brush_desc.opacity = 2.0f;
    set_matrix_identity(&brush_desc.transform);
    scale_matrix(&brush_desc.transform, 2.0f, 3.0f);

    hr = ID2D1DeviceContext_CreateImageBrush(device_context, image, &image_brush_desc, &brush_desc, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1ImageBrush_GetImage(brush, &tmp_image);
    ok(tmp_image == image, "Got unexpected image %p, expected %p.\n", tmp_image, image);
    ID2D1Image_Release(tmp_image);

    opacity = ID2D1ImageBrush_GetOpacity(brush);
    ok(opacity == 2.0f, "Unexpected opacity %.8e.\n", opacity);
    ID2D1ImageBrush_GetTransform(brush, &matrix);
    ok(!memcmp(&matrix, &brush_desc.transform, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32);

    ID2D1ImageBrush_Release(brush);

    ID2D1Image_Release(image);
    refcount = ID2D1Bitmap_Release(bitmap);
    ok(!refcount, "Bitmap has %lu references left.\n", refcount);

    release_test_context(&ctx);
}

static void test_linear_brush(BOOL d3d11)
{
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES gradient_properties;
    ID2D1GradientStopCollection *gradient, *tmp_gradient;
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *rectangle_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    ID2D1LinearGradientBrush *brush;
    struct d2d1_test_context ctx;
    struct resource_readback rb;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F colour;
    D2D1_POINT_2F p;
    unsigned int i;
    ULONG refcount;
    D2D1_RECT_F r;
    float opacity;
    HRESULT hr;

    static const D2D1_GRADIENT_STOP stops[] =
    {
        {0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.5f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    static const struct
    {
        unsigned int x, y;
        DWORD colour;
    }
    test1[] =
    {
        {80,  80, 0xff857a00}, {240,  80, 0xff926d00}, {400,  80, 0xff9f6000}, {560,  80, 0xffac5300},
        {80, 240, 0xff00eb14}, {240, 240, 0xff00f807}, {400, 240, 0xff06f900}, {560, 240, 0xff13ec00},
        {80, 400, 0xff0053ac}, {240, 400, 0xff005fa0}, {400, 400, 0xff006c93}, {560, 400, 0xff007986},
    },
    test2[] =
    {
        { 40,  30, 0xff005ba4}, {120,  30, 0xffffffff}, { 40,  60, 0xffffffff}, { 80,  60, 0xff00b44b},
        {120,  60, 0xff006c93}, {200,  60, 0xffffffff}, { 40,  90, 0xffffffff}, {120,  90, 0xff0ef100},
        {160,  90, 0xff00c53a}, {200,  90, 0xffffffff}, { 80, 120, 0xffffffff}, {120, 120, 0xffaf5000},
        {160, 120, 0xff679800}, {200, 120, 0xff1fe000}, {240, 120, 0xffffffff}, {160, 150, 0xffffffff},
        {200, 150, 0xffc03e00}, {240, 150, 0xffffffff}, {280, 150, 0xffffffff}, {320, 150, 0xffffffff},
        {240, 180, 0xffffffff}, {280, 180, 0xffff4040}, {320, 180, 0xffff4040}, {380, 180, 0xffffffff},
        {200, 210, 0xffffffff}, {240, 210, 0xffa99640}, {280, 210, 0xffb28d40}, {320, 210, 0xffbb8440},
        {360, 210, 0xffc47b40}, {400, 210, 0xffffffff}, {200, 240, 0xffffffff}, {280, 240, 0xff41fe40},
        {320, 240, 0xff49f540}, {360, 240, 0xff52ec40}, {440, 240, 0xffffffff}, {240, 270, 0xffffffff},
        {280, 270, 0xff408eb0}, {320, 270, 0xff4097a7}, {360, 270, 0xff40a19e}, {440, 270, 0xffffffff},
        {280, 300, 0xffffffff}, {320, 300, 0xff4040ff}, {360, 300, 0xff4040ff}, {400, 300, 0xff406ad4},
        {440, 300, 0xff4061de}, {480, 300, 0xff4057e7}, {520, 300, 0xff404ef1}, {280, 330, 0xffffffff},
        {360, 330, 0xffffffff}, {400, 330, 0xff40c17e}, {440, 330, 0xff40b788}, {480, 330, 0xff40ae91},
        {520, 330, 0xff40a49b}, {400, 360, 0xff57e740}, {440, 360, 0xff4ef140}, {480, 360, 0xff44fb40},
        {520, 360, 0xff40fa45}, {400, 390, 0xffae9140}, {440, 390, 0xffa49b40}, {480, 390, 0xff9aa540},
        {520, 390, 0xff90ae40},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&gradient_properties.startPoint, 320.0f, 0.0f);
    set_point(&gradient_properties.endPoint, 0.0f, 960.0f);
    hr = ID2D1RenderTarget_CreateLinearGradientBrush(rt, &gradient_properties, NULL, gradient, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    opacity = ID2D1LinearGradientBrush_GetOpacity(brush);
    ok(opacity == 1.0f, "Got unexpected opacity %.8e.\n", opacity);
    set_matrix_identity(&matrix);
    ID2D1LinearGradientBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    p = ID2D1LinearGradientBrush_GetStartPoint(brush);
    ok(compare_point(&p, 320.0f, 0.0f, 0), "Got unexpected start point {%.8e, %.8e}.\n", p.x, p.y);
    p = ID2D1LinearGradientBrush_GetEndPoint(brush);
    ok(compare_point(&p, 0.0f, 960.0f, 0), "Got unexpected end point {%.8e, %.8e}.\n", p.x, p.y);
    ID2D1LinearGradientBrush_GetGradientStopCollection(brush, &tmp_gradient);
    ok(tmp_gradient == gradient, "Got unexpected gradient %p, expected %p.\n", tmp_gradient, gradient);
    ID2D1GradientStopCollection_Release(tmp_gradient);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&colour, 1.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &colour);

    set_rect(&r, 0.0f, 0.0f, 320.0f, 960.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test1); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test1[i].x, test1[i].y);
        ok(compare_colour(colour, test1[i].colour, 1),
                "Got unexpected colour 0x%08lx at position {%u, %u}.\n",
                colour, test1[i].x, test1[i].y);
    }
    release_resource_readback(&rb);

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &colour);

    set_matrix_identity(&matrix);
    skew_matrix(&matrix, 0.2146f, 1.575f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 0.0f, 240.0f);
    scale_matrix(&matrix, 0.25f, -0.25f);
    ID2D1LinearGradientBrush_SetTransform(brush, &matrix);

    set_rect(&r, 0.0f, 0.0f, 80.0f, 240.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 0.0f, -50.0f);
    scale_matrix(&matrix, 0.1f, 0.1f);
    rotate_matrix(&matrix, -M_PI / 3.0f);
    ID2D1LinearGradientBrush_SetTransform(brush, &matrix);

    ID2D1LinearGradientBrush_SetOpacity(brush, 0.75f);
    set_rect(&r, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    factory = ctx.factory;

    set_rect(&r, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &r, &rectangle_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 228.5f, 714.0f);
    scale_matrix(&matrix, 40.0f, 120.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)rectangle_geometry,
            &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_Release(rectangle_geometry);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1LinearGradientBrush_SetTransform(brush, &matrix);
    set_point(&p, 188.5f, 834.0f);
    ID2D1LinearGradientBrush_SetStartPoint(brush, p);
    set_point(&p, 268.5f, 594.0f);
    ID2D1LinearGradientBrush_SetEndPoint(brush, p);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    ID2D1TransformedGeometry_Release(transformed_geometry);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test2); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test2[i].x, test2[i].y);
        ok(compare_colour(colour, test2[i].colour, 1),
                "Got unexpected colour 0x%08lx at position {%u, %u}.\n",
                colour, test2[i].x, test2[i].y);
    }
    release_resource_readback(&rb);

    ID2D1LinearGradientBrush_Release(brush);
    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(!refcount, "Gradient has %lu references left.\n", refcount);
    release_test_context(&ctx);
}

static void test_radial_brush(BOOL d3d11)
{
    D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES gradient_properties;
    ID2D1GradientStopCollection *gradient, *tmp_gradient;
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *rectangle_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    ID2D1RadialGradientBrush *brush;
    struct d2d1_test_context ctx;
    struct resource_readback rb;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F colour;
    D2D1_POINT_2F p;
    unsigned int i;
    ULONG refcount;
    D2D1_RECT_F r;
    HRESULT hr;
    float f;

    static const D2D1_GRADIENT_STOP stops[] =
    {
        {0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.5f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    static const struct
    {
        unsigned int x, y;
        DWORD colour;
    }
    test1[] =
    {
        {80,  80, 0xff0000ff}, {240,  80, 0xff00a857}, {400,  80, 0xff00d728}, {560,  80, 0xff0000ff},
        {80, 240, 0xff006699}, {240, 240, 0xff29d600}, {400, 240, 0xff966900}, {560, 240, 0xff00a55a},
        {80, 400, 0xff0000ff}, {240, 400, 0xff006e91}, {400, 400, 0xff007d82}, {560, 400, 0xff0000ff},
    },
    test2[] =
    {
        { 40,  30, 0xff000df2}, {120,  30, 0xffffffff}, { 40,  60, 0xffffffff}, { 80,  60, 0xff00b04f},
        {120,  60, 0xff007689}, {200,  60, 0xffffffff}, { 40,  90, 0xffffffff}, {120,  90, 0xff47b800},
        {160,  90, 0xff00c13e}, {200,  90, 0xffffffff}, { 80, 120, 0xffffffff}, {120, 120, 0xff0000ff},
        {160, 120, 0xff6f9000}, {200, 120, 0xff00718e}, {240, 120, 0xffffffff}, {160, 150, 0xffffffff},
        {200, 150, 0xff00609f}, {240, 150, 0xffffffff}, {280, 150, 0xffffffff}, {320, 150, 0xffffffff},
        {240, 180, 0xffffffff}, {280, 180, 0xff4040ff}, {320, 180, 0xff40b788}, {380, 180, 0xffffffff},
        {200, 210, 0xffffffff}, {240, 210, 0xff4040ff}, {280, 210, 0xff4040ff}, {320, 210, 0xff76c940},
        {360, 210, 0xff40cc73}, {400, 210, 0xffffffff}, {200, 240, 0xffffffff}, {280, 240, 0xff4061de},
        {320, 240, 0xff9fa040}, {360, 240, 0xff404af5}, {440, 240, 0xffffffff}, {240, 270, 0xffffffff},
        {280, 270, 0xff40aa95}, {320, 270, 0xff4ef140}, {360, 270, 0xff4040ff}, {440, 270, 0xffffffff},
        {280, 300, 0xffffffff}, {320, 300, 0xff4093ac}, {360, 300, 0xff4040ff}, {400, 300, 0xff4040ff},
        {440, 300, 0xff404af5}, {480, 300, 0xff4045fa}, {520, 300, 0xff4040ff}, {280, 330, 0xffffffff},
        {360, 330, 0xffffffff}, {400, 330, 0xff4069d6}, {440, 330, 0xff40c579}, {480, 330, 0xff40e956},
        {520, 330, 0xff4072cd}, {400, 360, 0xff408ab4}, {440, 360, 0xff49f540}, {480, 360, 0xffb98640},
        {520, 360, 0xff40dc62}, {400, 390, 0xff405ee1}, {440, 390, 0xff40d56a}, {480, 390, 0xff62dd40},
        {520, 390, 0xff4059e6},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&gradient_properties.center, 160.0f, 480.0f);
    set_point(&gradient_properties.gradientOriginOffset, 40.0f, -120.0f);
    gradient_properties.radiusX = 160.0f;
    gradient_properties.radiusY = 480.0f;
    hr = ID2D1RenderTarget_CreateRadialGradientBrush(rt, &gradient_properties, NULL, gradient, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    f = ID2D1RadialGradientBrush_GetOpacity(brush);
    ok(f == 1.0f, "Got unexpected opacity %.8e.\n", f);
    set_matrix_identity(&matrix);
    ID2D1RadialGradientBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    p = ID2D1RadialGradientBrush_GetCenter(brush);
    ok(compare_point(&p, 160.0f, 480.0f, 0), "Got unexpected center {%.8e, %.8e}.\n", p.x, p.y);
    p = ID2D1RadialGradientBrush_GetGradientOriginOffset(brush);
    ok(compare_point(&p, 40.0f, -120.0f, 0), "Got unexpected origin offset {%.8e, %.8e}.\n", p.x, p.y);
    f = ID2D1RadialGradientBrush_GetRadiusX(brush);
    ok(compare_float(f, 160.0f, 0), "Got unexpected x-radius %.8e.\n", f);
    f = ID2D1RadialGradientBrush_GetRadiusY(brush);
    ok(compare_float(f, 480.0f, 0), "Got unexpected y-radius %.8e.\n", f);
    ID2D1RadialGradientBrush_GetGradientStopCollection(brush, &tmp_gradient);
    ok(tmp_gradient == gradient, "Got unexpected gradient %p, expected %p.\n", tmp_gradient, gradient);
    ID2D1GradientStopCollection_Release(tmp_gradient);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&colour, 1.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &colour);

    set_rect(&r, 0.0f, 0.0f, 320.0f, 960.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test1); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test1[i].x, test1[i].y);
        ok(compare_colour(colour, test1[i].colour, 1),
                "Got unexpected colour 0x%08lx at position {%u, %u}.\n",
                colour, test1[i].x, test1[i].y);
    }
    release_resource_readback(&rb);

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &colour);

    set_matrix_identity(&matrix);
    skew_matrix(&matrix, 0.2146f, 1.575f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 0.0f, 240.0f);
    scale_matrix(&matrix, 0.25f, -0.25f);
    ID2D1RadialGradientBrush_SetTransform(brush, &matrix);

    set_rect(&r, 0.0f, 0.0f, 80.0f, 240.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, -75.0f, -50.0f);
    scale_matrix(&matrix, 0.15f, 0.5f);
    rotate_matrix(&matrix, -M_PI / 3.0f);
    ID2D1RadialGradientBrush_SetTransform(brush, &matrix);

    ID2D1RadialGradientBrush_SetOpacity(brush, 0.75f);
    set_rect(&r, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    factory = ctx.factory;

    set_rect(&r, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &r, &rectangle_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 228.5f, 714.0f);
    scale_matrix(&matrix, 40.0f, 120.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)rectangle_geometry,
            &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_Release(rectangle_geometry);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RadialGradientBrush_SetTransform(brush, &matrix);
    set_point(&p, 228.5f, 714.0f);
    ID2D1RadialGradientBrush_SetCenter(brush, p);
    ID2D1RadialGradientBrush_SetRadiusX(brush, -40.0f);
    ID2D1RadialGradientBrush_SetRadiusY(brush, 120.0f);
    set_point(&p, 20.0f, 30.0f);
    ID2D1RadialGradientBrush_SetGradientOriginOffset(brush, p);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    ID2D1TransformedGeometry_Release(transformed_geometry);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test2); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test2[i].x, test2[i].y);
        ok(compare_colour(colour, test2[i].colour, 1),
                "Got unexpected colour 0x%08lx at position {%u, %u}.\n",
                colour, test2[i].x, test2[i].y);
    }
    release_resource_readback(&rb);

    ID2D1RadialGradientBrush_Release(brush);
    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(!refcount, "Gradient has %lu references left.\n", refcount);
    release_test_context(&ctx);
}

static void fill_geometry_sink(ID2D1GeometrySink *sink, unsigned int hollow_count)
{
    D2D1_FIGURE_BEGIN begin;
    unsigned int idx = 0;
    D2D1_POINT_2F point;

    set_point(&point, 15.0f,  20.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 55.0f,  20.0f);
    line_to(sink, 55.0f, 220.0f);
    line_to(sink, 25.0f, 220.0f);
    line_to(sink, 25.0f, 100.0f);
    line_to(sink, 75.0f, 100.0f);
    line_to(sink, 75.0f, 300.0f);
    line_to(sink,  5.0f, 300.0f);
    line_to(sink,  5.0f,  60.0f);
    line_to(sink, 45.0f,  60.0f);
    line_to(sink, 45.0f, 180.0f);
    line_to(sink, 35.0f, 180.0f);
    line_to(sink, 35.0f, 140.0f);
    line_to(sink, 65.0f, 140.0f);
    line_to(sink, 65.0f, 260.0f);
    line_to(sink, 15.0f, 260.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 155.0f, 300.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 155.0f, 160.0f);
    line_to(sink,  85.0f, 160.0f);
    line_to(sink,  85.0f, 300.0f);
    line_to(sink, 120.0f, 300.0f);
    line_to(sink, 120.0f,  20.0f);
    line_to(sink, 155.0f,  20.0f);
    line_to(sink, 155.0f, 160.0f);
    line_to(sink,  85.0f, 160.0f);
    line_to(sink,  85.0f,  20.0f);
    line_to(sink, 120.0f,  20.0f);
    line_to(sink, 120.0f, 300.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 165.0f,  20.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 165.0f, 300.0f);
    line_to(sink, 235.0f, 300.0f);
    line_to(sink, 235.0f,  20.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    set_point(&point, 225.0f,  60.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 225.0f, 260.0f);
    line_to(sink, 175.0f, 260.0f);
    line_to(sink, 175.0f,  60.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    set_point(&point, 215.0f, 220.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 185.0f, 220.0f);
    line_to(sink, 185.0f, 100.0f);
    line_to(sink, 215.0f, 100.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    set_point(&point, 195.0f, 180.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 205.0f, 180.0f);
    line_to(sink, 205.0f, 140.0f);
    line_to(sink, 195.0f, 140.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
}

static void fill_geometry_sink_bezier(ID2D1GeometrySink *sink, unsigned int hollow_count)
{
    D2D1_FIGURE_BEGIN begin;
    unsigned int idx = 0;
    D2D1_POINT_2F point;

    set_point(&point, 5.0f, 160.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 40.0f, 160.0f, 40.0f,  20.0f);
    quadratic_to(sink, 40.0f, 160.0f, 75.0f, 160.0f);
    quadratic_to(sink, 40.0f, 160.0f, 40.0f, 300.0f);
    quadratic_to(sink, 40.0f, 160.0f,  5.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 160.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 20.0f,  80.0f, 40.0f,  80.0f);
    quadratic_to(sink, 60.0f,  80.0f, 60.0f, 160.0f);
    quadratic_to(sink, 60.0f, 240.0f, 40.0f, 240.0f);
    quadratic_to(sink, 20.0f, 240.0f, 20.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 5.0f, 612.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 40.0f, 612.0f, 40.0f, 752.0f);
    quadratic_to(sink, 40.0f, 612.0f, 75.0f, 612.0f);
    quadratic_to(sink, 40.0f, 612.0f, 40.0f, 472.0f);
    quadratic_to(sink, 40.0f, 612.0f,  5.0f, 612.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 612.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 20.0f, 692.0f, 40.0f, 692.0f);
    quadratic_to(sink, 60.0f, 692.0f, 60.0f, 612.0f);
    quadratic_to(sink, 60.0f, 532.0f, 40.0f, 532.0f);
    quadratic_to(sink, 20.0f, 532.0f, 20.0f, 612.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
}

static void test_path_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    ID2D1GeometrySink *sink, *tmp_sink;
    struct geometry_sink simplify_sink;
    D2D1_POINT_2F point = {0.0f, 0.0f};
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    ID2D1Geometry *tmp_geometry;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    BOOL match, contains;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    UINT32 count;
    HRESULT hr;

    static const struct geometry_segment expected_segments[] =
    {
        /* Figure 0. */
        {SEGMENT_LINE,   {{{ 55.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{ 55.0f, 220.0f}}}},
        {SEGMENT_LINE,   {{{ 25.0f, 220.0f}}}},
        {SEGMENT_LINE,   {{{ 25.0f, 100.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 100.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f,  60.0f}}}},
        {SEGMENT_LINE,   {{{ 45.0f,  60.0f}}}},
        {SEGMENT_LINE,   {{{ 45.0f, 180.0f}}}},
        {SEGMENT_LINE,   {{{ 35.0f, 180.0f}}}},
        {SEGMENT_LINE,   {{{ 35.0f, 140.0f}}}},
        {SEGMENT_LINE,   {{{ 65.0f, 140.0f}}}},
        {SEGMENT_LINE,   {{{ 65.0f, 260.0f}}}},
        {SEGMENT_LINE,   {{{ 15.0f, 260.0f}}}},
        /* Figure 1. */
        {SEGMENT_LINE,   {{{155.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{120.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{120.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{155.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{155.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{120.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{120.0f, 300.0f}}}},
        /* Figure 2. */
        {SEGMENT_LINE,   {{{165.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{235.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{235.0f,  20.0f}}}},
        /* Figure 3. */
        {SEGMENT_LINE,   {{{225.0f, 260.0f}}}},
        {SEGMENT_LINE,   {{{175.0f, 260.0f}}}},
        {SEGMENT_LINE,   {{{175.0f,  60.0f}}}},
        /* Figure 4. */
        {SEGMENT_LINE,   {{{185.0f, 220.0f}}}},
        {SEGMENT_LINE,   {{{185.0f, 100.0f}}}},
        {SEGMENT_LINE,   {{{215.0f, 100.0f}}}},
        /* Figure 5. */
        {SEGMENT_LINE,   {{{205.0f, 180.0f}}}},
        {SEGMENT_LINE,   {{{205.0f, 140.0f}}}},
        {SEGMENT_LINE,   {{{195.0f, 140.0f}}}},
        /* Figure 6. */
        {SEGMENT_LINE,   {{{135.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{135.0f, 420.0f}}}},
        {SEGMENT_LINE,   {{{105.0f, 420.0f}}}},
        {SEGMENT_LINE,   {{{105.0f, 540.0f}}}},
        {SEGMENT_LINE,   {{{155.0f, 540.0f}}}},
        {SEGMENT_LINE,   {{{155.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 580.0f}}}},
        {SEGMENT_LINE,   {{{125.0f, 580.0f}}}},
        {SEGMENT_LINE,   {{{125.0f, 460.0f}}}},
        {SEGMENT_LINE,   {{{115.0f, 460.0f}}}},
        {SEGMENT_LINE,   {{{115.0f, 500.0f}}}},
        {SEGMENT_LINE,   {{{145.0f, 500.0f}}}},
        {SEGMENT_LINE,   {{{145.0f, 380.0f}}}},
        {SEGMENT_LINE,   {{{ 95.0f, 380.0f}}}},
        /* Figure 7. */
        {SEGMENT_LINE,   {{{235.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{235.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{235.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 340.0f}}}},
        /* Figure 8. */
        {SEGMENT_LINE,   {{{245.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{315.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{315.0f, 620.0f}}}},
        /* Figure 9. */
        {SEGMENT_LINE,   {{{305.0f, 380.0f}}}},
        {SEGMENT_LINE,   {{{255.0f, 380.0f}}}},
        {SEGMENT_LINE,   {{{255.0f, 580.0f}}}},
        /* Figure 10. */
        {SEGMENT_LINE,   {{{265.0f, 420.0f}}}},
        {SEGMENT_LINE,   {{{265.0f, 540.0f}}}},
        {SEGMENT_LINE,   {{{295.0f, 540.0f}}}},
        /* Figure 11. */
        {SEGMENT_LINE,   {{{285.0f, 460.0f}}}},
        {SEGMENT_LINE,   {{{285.0f, 500.0f}}}},
        {SEGMENT_LINE,   {{{275.0f, 500.0f}}}},
        /* Figure 12. */
        {SEGMENT_BEZIER, {{{2.83333340e+01f, 1.60000000e+02f},
                           {4.00000000e+01f, 1.13333336e+02f},
                           {4.00000000e+01f, 2.00000000e+01f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 1.13333336e+02f},
                           {5.16666641e+01f, 1.60000000e+02f},
                           {7.50000000e+01f, 1.60000000e+02f}}}},
        {SEGMENT_BEZIER, {{{5.16666641e+01f, 1.60000000e+02f},
                           {4.00000000e+01f, 2.06666656e+02f},
                           {4.00000000e+01f, 3.00000000e+02f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 2.06666656e+02f},
                           {2.83333340e+01f, 1.60000000e+02f},
                           {5.00000000e+00f, 1.60000000e+02f}}}},
        /* Figure 13. */
        {SEGMENT_BEZIER, {{{2.00000000e+01f, 1.06666664e+02f},
                           {2.66666660e+01f, 8.00000000e+01f},
                           {4.00000000e+01f, 8.00000000e+01f}}}},
        {SEGMENT_BEZIER, {{{5.33333321e+01f, 8.00000000e+01f},
                           {6.00000000e+01f, 1.06666664e+02f},
                           {6.00000000e+01f, 1.60000000e+02f}}}},
        {SEGMENT_BEZIER, {{{6.00000000e+01f, 2.13333328e+02f},
                           {5.33333321e+01f, 2.40000000e+02f},
                           {4.00000000e+01f, 2.40000000e+02f}}}},
        {SEGMENT_BEZIER, {{{2.66666660e+01f, 2.40000000e+02f},
                           {2.00000000e+01f, 2.13333328e+02f},
                           {2.00000000e+01f, 1.60000000e+02f}}}},
        /* Figure 14. */
        {SEGMENT_BEZIER, {{{2.83333340e+01f, 6.12000000e+02f},
                           {4.00000000e+01f, 6.58666687e+02f},
                           {4.00000000e+01f, 7.52000000e+02f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 6.58666687e+02f},
                           {5.16666641e+01f, 6.12000000e+02f},
                           {7.50000000e+01f, 6.12000000e+02f}}}},
        {SEGMENT_BEZIER, {{{5.16666641e+01f, 6.12000000e+02f},
                           {4.00000000e+01f, 5.65333313e+02f},
                           {4.00000000e+01f, 4.72000000e+02f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 5.65333313e+02f},
                           {2.83333340e+01f, 6.12000000e+02f},
                           {5.00000000e+00f, 6.12000000e+02f}}}},
        /* Figure 15. */
        {SEGMENT_BEZIER, {{{2.00000000e+01f, 6.65333313e+02f},
                           {2.66666660e+01f, 6.92000000e+02f},
                           {4.00000000e+01f, 6.92000000e+02f}}}},
        {SEGMENT_BEZIER, {{{5.33333321e+01f, 6.92000000e+02f},
                           {6.00000000e+01f, 6.65333313e+02f},
                           {6.00000000e+01f, 6.12000000e+02f}}}},
        {SEGMENT_BEZIER, {{{6.00000000e+01f, 5.58666687e+02f},
                           {5.33333321e+01f, 5.32000000e+02f},
                           {4.00000000e+01f, 5.32000000e+02f}}}},
        {SEGMENT_BEZIER, {{{2.66666660e+01f, 5.32000000e+02f},
                           {2.00000000e+01f, 5.58666687e+02f},
                           {2.00000000e+01f, 6.12000000e+02f}}}},
        /* Figure 16. */
        {SEGMENT_BEZIER, {{{1.91750427e+02f, 1.27275856e+02f},
                           {2.08249573e+02f, 1.27275856e+02f},
                           {2.24748734e+02f, 6.12792168e+01f}}}},
        {SEGMENT_BEZIER, {{{2.08249573e+02f, 1.27275856e+02f},
                           {2.08249573e+02f, 1.93272476e+02f},
                           {2.24748734e+02f, 2.59269104e+02f}}}},
        {SEGMENT_BEZIER, {{{2.08249573e+02f, 1.93272476e+02f},
                           {1.91750427e+02f, 1.93272476e+02f},
                           {1.75251266e+02f, 2.59269104e+02f}}}},
        {SEGMENT_BEZIER, {{{1.91750427e+02f, 1.93272476e+02f},
                           {1.91750427e+02f, 1.27275856e+02f},
                           {1.75251266e+02f, 6.12792168e+01f}}}},
        /* Figure 17. */
        {SEGMENT_BEZIER, {{{1.95285950e+02f, 6.59932632e+01f},
                           {2.04714050e+02f, 6.59932632e+01f},
                           {2.14142136e+02f, 1.03705627e+02f}}}},
        {SEGMENT_BEZIER, {{{2.23570221e+02f, 1.41417984e+02f},
                           {2.23570221e+02f, 1.79130356e+02f},
                           {2.14142136e+02f, 2.16842712e+02f}}}},
        {SEGMENT_BEZIER, {{{2.04714050e+02f, 2.54555069e+02f},
                           {1.95285950e+02f, 2.54555069e+02f},
                           {1.85857864e+02f, 2.16842712e+02f}}}},
        {SEGMENT_BEZIER, {{{1.76429779e+02f, 1.79130356e+02f},
                           {1.76429779e+02f, 1.41417984e+02f},
                           {1.85857864e+02f, 1.03705627e+02f}}}},
        /* Figure 18. */
        {SEGMENT_BEZIER, {{{1.11847351e+02f, 4.46888092e+02f},
                           {1.11847351e+02f, 5.12884705e+02f},
                           {9.53481979e+01f, 5.78881348e+02f}}}},
        {SEGMENT_BEZIER, {{{1.11847351e+02f, 5.12884705e+02f},
                           {1.28346512e+02f, 5.12884705e+02f},
                           {1.44845673e+02f, 5.78881348e+02f}}}},
        {SEGMENT_BEZIER, {{{1.28346512e+02f, 5.12884705e+02f},
                           {1.28346512e+02f, 4.46888092e+02f},
                           {1.44845673e+02f, 3.80891479e+02f}}}},
        {SEGMENT_BEZIER, {{{1.28346512e+02f, 4.46888092e+02f},
                           {1.11847351e+02f, 4.46888092e+02f},
                           {9.53481979e+01f, 3.80891479e+02f}}}},
        /* Figure 19. */
        {SEGMENT_BEZIER, {{{9.65267105e+01f, 4.61030243e+02f},
                           {9.65267105e+01f, 4.98742584e+02f},
                           {1.05954803e+02f, 5.36454956e+02f}}}},
        {SEGMENT_BEZIER, {{{1.15382889e+02f, 5.74167297e+02f},
                           {1.24810982e+02f, 5.74167297e+02f},
                           {1.34239075e+02f, 5.36454956e+02f}}}},
        {SEGMENT_BEZIER, {{{1.43667160e+02f, 4.98742584e+02f},
                           {1.43667160e+02f, 4.61030243e+02f},
                           {1.34239075e+02f, 4.23317871e+02f}}}},
        {SEGMENT_BEZIER, {{{1.24810982e+02f, 3.85605499e+02f},
                           {1.15382889e+02f, 3.85605499e+02f},
                           {1.05954803e+02f, 4.23317871e+02f}}}},
        /* Figure 20. */
        {SEGMENT_LINE,   {{{ 40.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 160.0f}}}},
        /* Figure 21. */
        {SEGMENT_LINE,   {{{ 40.0f,  80.0f}}}},
        {SEGMENT_LINE,   {{{ 60.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 240.0f}}}},
        {SEGMENT_LINE,   {{{ 20.0f, 160.0f}}}},
        /* Figure 22. */
        {SEGMENT_LINE,   {{{ 40.0f, 752.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 612.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 472.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 612.0f}}}},
        /* Figure 23. */
        {SEGMENT_LINE,   {{{ 40.0f, 692.0f}}}},
        {SEGMENT_LINE,   {{{ 60.0f, 612.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 532.0f}}}},
        {SEGMENT_LINE,   {{{ 20.0f, 612.0f}}}},
        /* Figure 24. */
        {SEGMENT_LINE,   {{{2.03125019e+01f, 1.51250000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 1.25000008e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 8.12500076e+01f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 2.00000000e+01f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 8.12500076e+01f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 1.25000008e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 1.51250000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{7.50000000e+01f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 1.68750000e+02f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 1.95000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 2.38750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 3.00000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 2.38750000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 1.95000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.03125019e+01f, 1.68750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.00000000e+00f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 25. */
        {SEGMENT_LINE,   {{{2.50000000e+01f, 1.00000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 8.00000000e+01f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 1.00000000e+02f}}}},
        {SEGMENT_LINE,   {{{6.00000000e+01f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 2.20000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 2.40000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.50000000e+01f, 2.20000000e+02f}}}},
        {SEGMENT_LINE,   {{{2.00000000e+01f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 26. */
        {SEGMENT_LINE,   {{{2.03125019e+01f, 6.20750000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 6.47000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 6.90750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 7.52000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 6.90750000e+02f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 6.47000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 6.20750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{7.50000000e+01f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 6.03250000e+02f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 5.77000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 5.33250000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 4.72000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 5.33250000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 5.77000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.03125019e+01f, 6.03250000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.00000000e+00f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 27. */
        {SEGMENT_LINE,   {{{2.50000000e+01f, 6.72000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 6.92000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 6.72000000e+02f}}}},
        {SEGMENT_LINE,   {{{6.00000000e+01f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 5.52000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 5.32000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.50000000e+01f, 5.52000000e+02f}}}},
        {SEGMENT_LINE,   {{{2.00000000e+01f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 28. */
        {SEGMENT_LINE,   {{{ 75.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 300.0f}}}},
        /* Figure 29. */
        {SEGMENT_LINE,   {{{ 40.0f, 100.0f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 7.66666666e+01f},
                           {3.33333333e+01f, 7.00000000e+01f},
                           {2.00000000e+01f, 8.00000000e+01f}}}},
        {SEGMENT_LINE,   {{{ 60.0f,  40.0f}}}},
        {SEGMENT_BEZIER, {{{8.33333333e+01f, 4.00000000e+01f},
                           {9.00000000e+01f, 3.33333333e+01f},
                           {8.00000000e+01f, 2.00000000e+01f}}}},
        {SEGMENT_LINE,   {{{140.0f, 160.0f}}}},
        {SEGMENT_BEZIER, {{{1.16666666e+02f, 1.60000000e+02f},
                           {1.10000000e+02f, 1.66666666e+02f},
                           {1.20000000e+02f, 1.80000000e+02f}}}},
        {SEGMENT_LINE,   {{{170.0f, 110.0f}}}},
        {SEGMENT_BEZIER, {{{1.70000000e+02f, 1.26666666e+02f},
                           {1.73333333e+02f, 1.30000000e+02f},
                           {1.80000000e+02f, 1.20000000e+02f}}}},
    };
    static const struct expected_geometry_figure expected_figures[] =
    {
        /* 0 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 15.0f,  20.0f}, 15, &expected_segments[0]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {155.0f, 300.0f}, 11, &expected_segments[15]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {165.0f,  20.0f},  3, &expected_segments[26]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {225.0f,  60.0f},  3, &expected_segments[29]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {215.0f, 220.0f},  3, &expected_segments[32]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {195.0f, 180.0f},  3, &expected_segments[35]},
        /* 6 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 95.0f, 620.0f}, 15, &expected_segments[38]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {235.0f, 340.0f}, 11, &expected_segments[53]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {245.0f, 620.0f},  3, &expected_segments[64]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {305.0f, 580.0f},  3, &expected_segments[67]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {295.0f, 420.0f},  3, &expected_segments[70]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {275.0f, 460.0f},  3, &expected_segments[73]},
        /* 12 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 160.0f},  4, &expected_segments[76]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 160.0f},  4, &expected_segments[80]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 612.0f},  4, &expected_segments[84]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 612.0f},  4, &expected_segments[88]},
        /* 16 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {1.75251266e+02f, 6.12792168e+01f}, 4, &expected_segments[92]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {1.85857864e+02f, 1.03705627e+02f}, 4, &expected_segments[96]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {9.53481979e+01f, 3.80891479e+02f}, 4, &expected_segments[100]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {1.05954803e+02f, 4.23317871e+02f}, 4, &expected_segments[104]},
        /* 20 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 160.0f},  4, &expected_segments[108]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 160.0f},  4, &expected_segments[112]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 612.0f},  4, &expected_segments[116]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 612.0f},  4, &expected_segments[120]},
        /* 24 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 160.0f}, 16, &expected_segments[124]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 160.0f},  8, &expected_segments[140]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 612.0f}, 16, &expected_segments[148]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 612.0f},  8, &expected_segments[164]},
        /* 28 */
        {D2D1_FIGURE_BEGIN_HOLLOW, D2D1_FIGURE_END_OPEN,   { 40.0f,  20.0f},  2, &expected_segments[172]},
        /* 29 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_OPEN,   {  20.0f,  80.0f},  2, &expected_segments[174]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_OPEN,   {  80.0f,  20.0f},  2, &expected_segments[176]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 120.0f, 180.0f},  2, &expected_segments[178]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 180.0f, 120.0f},  2, &expected_segments[180]},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    factory = ctx.factory;

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Close() when closed. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    ID2D1PathGeometry_Release(geometry);

    /* Open() when closed. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    ID2D1PathGeometry_Release(geometry);

    /* Open() when open. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &tmp_sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    ID2D1PathGeometry_Release(geometry);

    /* BeginFigure() without EndFigure(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    /* EndFigure() without BeginFigure(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    /* BeginFigure()/EndFigure() mismatch. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    ID2D1PathGeometry_Release(geometry);

    /* AddLine() outside BeginFigure()/EndFigure(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_AddLine(sink, point);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_AddLine(sink, point);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    /* Empty figure. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_point(&point, 123.0f, 456.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 1, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 1, "Got unexpected segment count %u.\n", count);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 123.0f, 456.0f, 123.0f, 456.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 326.0f, 868.0f, 326.0f, 868.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* Degenerate figure. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_point(&point, 123.0f, 456.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 123.0f, 456.0f, 123.0f, 456.0f);
    quadratic_to(sink, 123.0f, 456.0f, 123.0f, 456.0f);
    quadratic_to(sink, 123.0f, 456.0f, 123.0f, 456.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 1, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 4, "Got unexpected segment count %u.\n", count);
    ID2D1PathGeometry_Release(geometry);

    /* Close right after Open(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Not open yet. */
    set_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Open, not closed. */
    set_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 0, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 0, "Got unexpected segment count %u.\n", count);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rect.left > rect.right && rect.top > rect.bottom,
            "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n", rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 10.0f, 20.0f);
    scale_matrix(&matrix, 10.0f, 20.0f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rect.left > rect.right && rect.top > rect.bottom,
            "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n", rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* GetBounds() with bezier segments. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fill_geometry_sink_bezier(sink, 0);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 5.0f, 20.0f, 75.0f, 752.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 90.0f, 650.0f, 230.0f, 1016.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* GetBounds() with bezier segments and some hollow components. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fill_geometry_sink_bezier(sink, 2);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 5.0f, 472.0f, 75.0f, 752.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 90.0f, 876.0f, 230.0f, 1016.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* GetBounds() with bezier segments and all hollow components. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fill_geometry_sink_bezier(sink, 4);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, INFINITY, INFINITY, FLT_MAX, FLT_MAX, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, INFINITY, INFINITY, FLT_MAX, FLT_MAX, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    /* The fillmode that's used is the last one set before the sink is closed. */
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    fill_geometry_sink(sink, 0);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_ALTERNATE);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 6, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    /* Intersections don't create extra segments. */
    ok(count == 44, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    ID2D1GeometrySink_Release(sink);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[0], 0);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[0], 0);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 1.0f, -1.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[6], 0);
    geometry_sink_cleanup(&simplify_sink);

    ID2D1TransformedGeometry_GetSourceGeometry(transformed_geometry, &tmp_geometry);
    ok(tmp_geometry == (ID2D1Geometry *)geometry,
            "Got unexpected source geometry %p, expected %p.\n", tmp_geometry, geometry);
    ID2D1TransformedGeometry_GetTransform(transformed_geometry, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1Geometry_Simplify(tmp_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &tmp_matrix, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[6], 0);
    geometry_sink_cleanup(&simplify_sink);
    ID2D1Geometry_Release(tmp_geometry);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "3aace1b22aae111cb577614fed16e4eb1650dba5");
    ok(match, "Surface does not match.\n");

    /* Edge test. */
    set_point(&point, 94.0f, 620.0f);
    contains = TRUE;
    hr = ID2D1TransformedGeometry_FillContainsPoint(transformed_geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!contains, "Got unexpected contains %#x.\n", contains);

    set_point(&point, 95.0f, 620.0f);
    contains = FALSE;
    hr = ID2D1TransformedGeometry_FillContainsPoint(transformed_geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(contains == TRUE, "Got unexpected contains %#x.\n", contains);

    /* With transformation matrix. */
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, -10.0f, 0.0f);
    set_point(&point, 85.0f, 620.0f);
    contains = FALSE;
    hr = ID2D1TransformedGeometry_FillContainsPoint(transformed_geometry, point, &matrix, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(contains == TRUE, "Got unexpected contains %#x.\n", contains);

    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fill_geometry_sink(sink, 0);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 6, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 44, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 5.0f, 20.0f, 235.0f, 300.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 100.0f, 50.0f);
    scale_matrix(&matrix, 2.0f, 1.5f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, -3.17192993e+02f, 8.71231079e+01f, 4.04055908e+02f, 6.17453125e+02f, 1);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 6, &expected_figures[0], 0);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 320.0f, 320.0f);
    scale_matrix(&matrix, -1.0f, 1.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "bfb40a1f007694fa07dbd3b854f3f5d9c3e1d76b");
    ok(match, "Surface does not match.\n");
    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fill_geometry_sink_bezier(sink, 0);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 4, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 20, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[12], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 100.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[20], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 10.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[24], 1);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 400.0f, -33.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[16], 4);
    geometry_sink_cleanup(&simplify_sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_figure(&ctx, 0, 0, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEECASLAQgKCIEBDQoMew8KD3YQDBByEgwSbhMOEmwUDhRpFBAUZxUQFWUVEhVjFhIWYRYUFl8X"
            "FBddFxYWXRYYFlsXGBdaFhoWWRYcFlgVHhVXFSAVVhQiFFUUIxRVEyYTVBIoElQRKhFUECwQUxAu"
            "EFIOMg5SDTQNUgs4C1IJPAlRCEAIUAZEBlAESARQAU4BTgJQAkgGUAY/C1ALMhNQEyoTUBMyC1AL"
            "PwZQBkgCUAJOAU4BUARIBFAGRAZQCEAIUQk8CVILOAtSDTQNUg4yDlIQLhBTECwQVBEqEVQSKBJU"
            "EyYTVBQjFFYUIhRWFSAVVxUeFVgWHBZZFhoWWhcYF1sWGBZcFxYWXhcUF18WFBZhFhIWYxUSFWUV"
            "EBVnFBAUaRQOFGsTDhJvEgwSchAMEHYPCg96DQoMggEICgiLAQQIBJQBCJgBCJkBBpoBBpoBBpoB"
            "BpsBBJwBBJwBBJwBBJwBBJ0BAp4BAp4BAp4BAp4BAp4BAp4BAp4BAgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 0, 226, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEECASLAQgKCIEBDQoMew8KD3YQDBByEgwSbhMOEmwUDhRpFBAUZxUQFWUVEhVjFhIWYRYUFl8X"
            "FBddFxYWXRYYFlsXGBdaFhoWWRYcFlgVHhVXFSAVVhQiFFUUIxRVEyYTVBIoElQRKhFUECwQUxAu"
            "EFIOMg5SDTQNUgs4C1IJPAlRCEAIUAZEBlAESARQAU4BTgJQAkgGUAY/C1ALMhNQEyoTUBMyC1AL"
            "PwZQBkgCUAJOAU4BUARIBFAGRAZQCEAIUQk8CVILOAtSDTQNUg4yDlIQLhBTECwQVBEqEVQSKBJU"
            "EyYTVBQjFFYUIhRWFSAVVxUeFVgWHBZZFhoWWhcYF1sWGBZcFxYWXhcUF18WFBZhFhIWYxUSFWUV"
            "EBVnFBAUaRQOFGsTDhJvEgwSchAMEHYPCg96DQoMggEICgiLAQQIBJQBCJgBCJkBBpoBBpoBBpoB"
            "BpsBBJwBBJwBBJwBBJwBBJ0BAp4BAp4BAp4BAp4BAp4BAp4BAp4BAgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 0, 320, 160, 0xff652e89, 64,
            "gVQBwAIBWgHlAQFYAecBAVYB6QEBVAHrAQEjDCMB7AECHhQeAu0BAxoYGgPvAQMWHhYD8QEDFCAU"
            "A/MBBBAkEAT0AQUOJw0F9QEGCioKBvcBBggsCAb4AQgFLgUI+QEJATIBCfsBCAIwAgj8AQcFLAUH"
            "/QEFCCgIBf4BBAwiDAT/AQIQHBAClwISlwIBPgGAAgI8Av8BAzwD/QEEPAT7AQY6BvkBBzoH+AEI"
            "OAj3AQk4CfYBCTgK9AELNgvzAQw2DPIBDDYM8QEONA7wAQ40DvABDjQO7wEPNA/uAQ80D+4BEDIQ"
            "7QERMhHsAREyEewBETIR7AERMhHsAREyEewBETIR7AERMhHsAREyEewBETIR7AERMhHsAREyEewB"
            "ETIR7AERMhHsAREyEe0BEDIQ7gEQMw/uAQ80D+4BDzQP7wEONA7wAQ40DvEBDDYM8gEMNgzzAQs2"
            "C/QBCzcK9QEJOAn3AQg4CfcBBzoH+QEGOgb7AQU6BfwBBDwE/QEDPAP/AQE+AZkCDpkCAhIYEgKA"
            "AgMNIA0D/wEFCSYJBf4BBgYqBgf8AQgDLgMI+wFG+gEIAzADCPkBBwYuBgf3AQYKKgoG9gEFDCgM"
            "BfUBBBAlDwTzAQQSIhIE8QEDFh4WA/ABAhkaGQLvAQIcFhwC7QECIBAgAusBASgEKAHpAQFWAecB"
            "AVgB5QEBWgHAAgHhUgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 160, 0xff652e89, 64,
            "/VUB5QEBWAHnAQFWAekBAVQB6wECIQ8hAe0BAh0VHQLuAQIZGhkD7wEDFh4WA/EBBBMhEwPzAQQQ"
            "JQ8F9AEFDCgNBfUBBgoqCgb3AQcHLQcG+QEIBC8ECPkBPAEJ+wEIAy8CCP0BBgYrBQf9AQUJJgkF"
            "/wEDDSANBP8BAhEaEQKYAhAXAYACAT4BgAICPQL+AQM8BPwBBTsE+wEGOgb6AQc5B/gBCDgJ9gEJ"
            "OAn2AQo3CvQBCzcK8wEMNgzyAQ01DPIBDTUN8AEONA7wAQ40D+4BDzQP7gEQMw/uARAzEO0BEDIR"
            "7AERMhHsAREyEewBETIR7AERMhLrAREyEusBETIS6wERMhLrAREyEusBETIS6wERMhHsAREyEewB"
            "ETIR7QEQMhHtARAzEO0BEDMP7gEPNA/vAQ40D+8BDjQO8QENNQ3xAQ01DPMBCzYM8wELNwr1AQo3"
            "CvUBCTgJ9wEIOAn4AQc5B/kBBjoG+wEFOwT9AQM8BP4BAj0C/wEBPgGYAhAXAYACAhEaEQKAAgMN"
            "IA0E/gEFCSYJBf4BBgYrBQf8AQgDLwII+wE8AQn6AQgELwQI+AEHBy0HBvcBBgoqCgb2AQUNJw0F"
            "9AEEECQQBfIBBBMhEwPxAQMWHhYD8AECGRoZA+4BAh0VHQLsAQIhDiIB6wEBVAHpAQFWAecBAVgB"
            "wAIBwlYA");
    ok(match, "Figure does not match.\n");
    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fill_geometry_sink_bezier(sink, 0);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 4, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 20, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 4, &expected_figures[12], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 100.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 4, &expected_figures[20], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 10.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 4, &expected_figures[24], 1);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 127.0f, 80.0f);
    rotate_matrix(&matrix, M_PI / -4.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_figure(&ctx, 0, 0, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEQiwEagQEjeyh2LHIwbjNsNmk4ZzplPGM+YUBfQl1DXURbRlpGWUhYSFdKVkpVS1VMVExUTFRM"
            "U05STlJOUk5STlFQUFBQUFBQTlRIXD9mMnYqdjJmP1xIVE5QUFBQUFBQUU5STlJOUk5STlNMVExU"
            "TFRMVEtWSlZKV0hYSFlGWkZbRFxDXkJfQGE+YzxlOmc4aTZrM28wcix2KHojggEaiwEQlAEImAEI"
            "mQEGmgEGmgEGmgEGmwEEnAEEnAEEnAEEnAEEnQECngECngECngECngECngECngECngEC");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 0, 226, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEQiwEagQEjeyh2LHIwbjNsNmk4ZzplPGM+YUBfQl1DXURbRlpGWUhYSFdKVkpVS1VMVExUTFRM"
            "U05STlJOUk5STlFQUFBQUFBQTlRIXD9mMnYqdjJmP1xIVE5QUFBQUFBQUU5STlJOUk5STlNMVExU"
            "TFRMVEtWSlZKV0hYSFlGWkZbRFxDXkJfQGE+YzxlOmc4aTZrM28wcix2KHojggEaiwEQlAEImAEI"
            "mQEGmgEGmgEGmgEGmwEEnAEEnAEEnAEEnAEEnQECngECngECngECngECngECngECngEC");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 0, 320, 160, 0xff652e89, 64,
            "4VIBwAIBWgHlAQFYAecBAVYB6QEBVAHrAQIhDiIB7QECHRUdAu4BAhkaGQPvAQMWHhYD8QEEEyET"
            "A/MBBBAkEAT1AQUMKA0F9QEGCioKBvcBBwctBwb5AQgELwQI+QEJATIBCfsBRP0BQ/0BQv8BQf8B"
            "QIECP4ACQIACQf4BQ/wBRPsBRvoBR/gBSPcBSvYBS/QBTPMBTvIBTvIBT/ABUPABUe4BUu4BUu4B"
            "U+0BU+wBVOwBVOwBVOwBVOwBVesBVesBVesBVesBVOwBVOwBVOwBVO0BU+0BU+0BUu4BUu8BUe8B"
            "UPEBT/EBTvIBTvMBTPUBS/UBSvcBSfcBSPkBRvsBRP0BQ/4BQf8BQIECP4ACQIACQf4BQv4BQ/wB"
            "RPsBCQEyAQn6AQgELwQI+AEHBy0GB/cBBgoqCgb2AQUMKA0F9AEEECUPBPMBBBIiEwPxAQMWHhYD"
            "8AECGRoZA+4BAh0VHQLsAQIhDiIB6wEBVAHpAQFWAecBAVgB5QEBWgHAAgEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 160, 0xff652e89, 64,
            "gVQBXAHjAQFaAeUBAVgB5wEBVgHpAQEpAikB6wECIBAgAu0BAhwWHALvAQIZGhkC8AEDFh4WA/EB"
            "BBIiEgTzAQQPJRAE9QEFDCgMBfYBBgoqCgb3AQcGLgYH+QEIAzADCPoBRvsBRPwBRP0BQv8BQIAC"
            "QIECPoECQP8BQv0BRPwBRPsBRvkBSPgBSPcBSvUBTPQBTPMBTvIBTvEBUPABUO8BUu4BUu4BUu4B"
            "Uu0BVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVO0BUu4BUu4BUu8BUPAB"
            "UPABUPEBTvIBTvMBTPQBS/YBSvcBSPgBSPkBRvsBRP0BQv8BQIACQIECPoECQP8BQv4BQv0BRPwB"
            "RPsBCQEyAQn5AQgFLgUI+AEGCCwIBvcBBgoqCgb1AQUNJw4F9AEEECQQBPMBAxQgFAPxAQMWHhYD"
            "7wEDGhgaA+0BAh4UHgLsAQEjDCMB6wEBVAHpAQFWAecBAVgB5QEBWgGiVQAA");
    ok(match, "Figure does not match.\n");
    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 40.0f,   20.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 75.0f,  300.0f);
    line_to(sink,  5.0f,  300.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 40.0f,  290.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 55.0f,  160.0f);
    line_to(sink, 25.0f,  160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 2, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 6, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "a875e68e0cb9c055927b1b50b879f90b24e38470");
    ok(match, "Surface does not match.\n");
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_point(&point, 40.0f,   20.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    line_to(sink, 75.0f,  300.0f);
    line_to(sink,  5.0f,  300.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count == 2, "Got unexpected segment count %u.\n", count);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[28], 1);
    geometry_sink_cleanup(&simplify_sink);

    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 20.0f,   80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 40.0f, 100.0f);
    quadratic_to(sink, 40.0f, 65.0f, 20.0f, 80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 80.0f,   20.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 60.0f, 40.0f);
    quadratic_to(sink, 95.0f, 40.0f, 80.0f, 20.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f,  180.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 140.0f, 160.0f);
    quadratic_to(sink, 105.0f, 160.0f, 120.0f, 180.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f,  120.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 170.0f, 110.0f);
    quadratic_to(sink, 170.0f, 135.0f, 180.0f, 120.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 20.0f, 20.0f, 180.0f, 180.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[29], 1);
    geometry_sink_cleanup(&simplify_sink);

    ID2D1PathGeometry_Release(geometry);

    ID2D1SolidColorBrush_Release(brush);
    release_test_context(&ctx);
}

static void test_rectangle_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *geometry;
    struct d2d1_test_context ctx;
    struct geometry_sink sink;
    D2D1_MATRIX_3X2_F matrix;
    D2D1_RECT_F rect, rect2;
    D2D1_POINT_2F point;
    BOOL contains;
    HRESULT hr;
    BOOL match;

    static const struct geometry_segment expected_segments[] =
    {
        /* Figure 0. */
        {SEGMENT_LINE, {{{10.0f,  0.0f}}}},
        {SEGMENT_LINE, {{{10.0f, 20.0f}}}},
        {SEGMENT_LINE, {{{ 0.0f, 20.0f}}}},
        /* Figure 1. */
        {SEGMENT_LINE, {{{4.42705116e+01f, 1.82442951e+01f}}}},
        {SEGMENT_LINE, {{{7.95376282e+01f, 5.06049728e+01f}}}},
        {SEGMENT_LINE, {{{5.52671127e+01f, 6.23606796e+01f}}}},
        /* Figure 2. */
        {SEGMENT_LINE, {{{25.0f, 15.0f}}}},
        {SEGMENT_LINE, {{{25.0f, 55.0f}}}},
        {SEGMENT_LINE, {{{25.0f, 55.0f}}}},
        /* Figure 3. */
        {SEGMENT_LINE, {{{35.0f, 45.0f}}}},
        {SEGMENT_LINE, {{{35.0f, 45.0f}}}},
        {SEGMENT_LINE, {{{30.0f, 45.0f}}}},
        /* Figure 4. */
        {SEGMENT_LINE, {{{ 1.07179585e+01f, 2.23205078e+02f}}}},
        {SEGMENT_LINE, {{{-5.85640755e+01f, 2.73205078e+02f}}}},
        {SEGMENT_LINE, {{{-7.85640717e+01f, 2.29903809e+02f}}}},
        /* Figure 5. */
        {SEGMENT_LINE, {{{40.0f, 20.0f}}}},
        {SEGMENT_LINE, {{{40.0f, 40.0f}}}},
        {SEGMENT_LINE, {{{30.0f, 40.0f}}}},
        /* Figure 6. */
        {SEGMENT_LINE, {{{ 2.14359169e+01f, 0.0f}}}},
        {SEGMENT_LINE, {{{-1.17128151e+02f, 0.0f}}}},
        {SEGMENT_LINE, {{{-1.57128143e+02f, 0.0f}}}},
        /* Figure 7. */
        {SEGMENT_LINE, {{{0.0f, 1.11602539e+02f}}}},
        {SEGMENT_LINE, {{{0.0f, 1.36602539e+02f}}}},
        {SEGMENT_LINE, {{{0.0f, 1.14951904e+02f}}}},
    };
    static const struct expected_geometry_figure expected_figures[] =
    {
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 0.0f,  0.0f}, 3, &expected_segments[0]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {20.0f, 30.0f}, 3, &expected_segments[3]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {25.0f, 15.0f}, 3, &expected_segments[6]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {30.0f, 45.0f}, 3, &expected_segments[9]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {-9.28203964e+00f, 1.79903809e+02f},
                3, &expected_segments[12]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {30.0f, 20.0f}, 3, &expected_segments[15]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {-1.85640793e+01f, 0.0f}, 3, &expected_segments[18]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {0.0f, 8.99519043e+01f}, 3, &expected_segments[21]},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 50.0f, 0.0f, 40.0f, 100.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 50.0f, 0.0f, 40.0f, 100.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 0.0f, 100.0f, 40.0f, 50.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 0.0f, 100.0f, 40.0f, 50.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 50.0f, 100.0f, 40.0f, 50.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 50.0f, 100.0f, 40.0f, 50.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 0.0f, 0.0f, 10.0f, 20.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Edge. */
    contains = FALSE;
    set_point(&point, 0.0f, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!contains, "Got wrong hit test result %d.\n", contains);

    /* Within tolerance limit around corner. */
    contains = TRUE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!contains, "Got wrong hit test result %d.\n", contains);

    contains = FALSE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE + 0.01f, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!contains, "Got wrong hit test result %d.\n", contains);

    contains = TRUE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE - 0.01f, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!contains, "Got wrong hit test result %d.\n", contains);

    contains = TRUE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE, -D2D1_DEFAULT_FLATTENING_TOLERANCE);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!contains, "Got wrong hit test result %d.\n", contains);

    /* Inside. */
    contains = FALSE;
    set_point(&point, 5.0f, 5.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!contains, "Got wrong hit test result %d.\n", contains);

    /* Test GetBounds() and Simplify(). */
    hr = ID2D1RectangleGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 0.0f, 0.0f, 10.0f, 20.0f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[0], 0);
    geometry_sink_cleanup(&sink);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[0], 0);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 20.0f, 30.0f);
    scale_matrix(&matrix, 3.0f, 2.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1RectangleGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 2.00000000e+01f, 1.82442951e+01f, 7.95376282e+01f, 6.23606796e+01f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[1], 1);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 25.0f, 15.0f);
    scale_matrix(&matrix, 0.0f, 2.0f);
    hr = ID2D1RectangleGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 25.0f, 15.0f, 25.0f, 55.0f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[2], 0);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 30.0f, 45.0f);
    scale_matrix(&matrix, 0.5f, 0.0f);
    hr = ID2D1RectangleGeometry_GetBounds(geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 30.0f, 45.0f, 35.0f, 45.0f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[3], 0);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 4.0f, 5.0f);
    rotate_matrix(&matrix, M_PI / 3.0f);
    translate_matrix(&matrix, 30.0f, 20.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(ctx.factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1TransformedGeometry_GetBounds(transformed_geometry, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, -7.85640717e+01f, 1.79903809e+02f, 1.07179594e+01f, 2.73205078e+02f, 1);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[4], 1);
    geometry_sink_cleanup(&sink);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[4], 1);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / -3.0f);
    scale_matrix(&matrix, 0.25f, 0.2f);
    hr = ID2D1TransformedGeometry_GetBounds(transformed_geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 30.0f, 20.0f, 40.0f, 40.0f, 2);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[5], 4);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 2.0f, 0.0f);
    hr = ID2D1TransformedGeometry_GetBounds(transformed_geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, -1.57128143e+02f, 0.00000000e+00f, 2.14359188e+01f, 0.00000000e+00f, 1);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[6], 1);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.0f, 0.5f);
    hr = ID2D1TransformedGeometry_GetBounds(transformed_geometry, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 0.00000000e+00f, 8.99519043e+01f, 0.00000000e+00, 1.36602539e+02f, 1);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[7], 1);
    geometry_sink_cleanup(&sink);

    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1RectangleGeometry_Release(geometry);

    release_test_context(&ctx);
}

static void test_rounded_rectangle_geometry(BOOL d3d11)
{
    ID2D1RoundedRectangleGeometry *geometry;
    D2D1_ROUNDED_RECT rect, rect2;
    struct d2d1_test_context ctx;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    set_rounded_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(ctx.factory, &rect, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    /* X radius larger than half width. */
    set_rounded_rect(&rect, 0.0f, 0.0f, 50.0f, 40.0f, 30.0f, 5.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(ctx.factory, &rect, &geometry);
    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    /* Y radius larger than half height. */
    set_rounded_rect(&rect, 0.0f, 0.0f, 50.0f, 40.0f, 5.0f, 30.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(ctx.factory, &rect, &geometry);
    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    /* Both exceed rectangle size. */
    set_rounded_rect(&rect, 0.0f, 0.0f, 50.0f, 40.0f, 30.0f, 25.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(ctx.factory, &rect, &geometry);
    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    release_test_context(&ctx);
}

static void test_bitmap_formats(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    struct d2d1_test_context ctx;
    D2D1_SIZE_U size = {4, 4};
    ID2D1RenderTarget *rt;
    ID2D1Bitmap *bitmap;
    unsigned int i, j;
    HRESULT hr;

    static const struct
    {
        DXGI_FORMAT format;
        DWORD mask;
    }
    bitmap_formats[] =
    {
        {DXGI_FORMAT_R32G32B32A32_FLOAT,    0x8a},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,    0x8a},
        {DXGI_FORMAT_R16G16B16A16_UNORM,    0x8a},
        {DXGI_FORMAT_R8G8B8A8_TYPELESS,     0x00},
        {DXGI_FORMAT_R8G8B8A8_UNORM,        0x0a},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   0x8a},
        {DXGI_FORMAT_R8G8B8A8_UINT,         0x00},
        {DXGI_FORMAT_R8G8B8A8_SNORM,        0x00},
        {DXGI_FORMAT_R8G8B8A8_SINT,         0x00},
        {DXGI_FORMAT_A8_UNORM,              0x06},
        {DXGI_FORMAT_B8G8R8A8_UNORM,        0x0a},
        {DXGI_FORMAT_B8G8R8X8_UNORM,        0x88},
        {DXGI_FORMAT_B8G8R8A8_TYPELESS,     0x00},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,   0x8a},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    for (i = 0; i < ARRAY_SIZE(bitmap_formats); ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if ((bitmap_formats[i].mask & (0x80 | (1u << j))) == (0x80 | (1u << j)))
                continue;

            bitmap_desc.pixelFormat.format = bitmap_formats[i].format;
            bitmap_desc.pixelFormat.alphaMode = j;
            hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
            if (bitmap_formats[i].mask & (1u << j))
                ok(hr == S_OK, "Got unexpected hr %#lx, for format %#x/%#x.\n",
                        hr, bitmap_formats[i].format, j);
            else
                ok(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT, "Got unexpected hr %#lx, for format %#x/%#x.\n",
                        hr, bitmap_formats[i].format, j);
            if (SUCCEEDED(hr))
                ID2D1Bitmap_Release(bitmap);
        }
    }

    release_test_context(&ctx);
}

static void test_alpha_mode(BOOL d3d11)
{
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    D2D1_BITMAP_PROPERTIES1 bitmap_desc1;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1SolidColorBrush *color_brush;
    ID2D1BitmapBrush *bitmap_brush;
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    ID2D1Bitmap1 *bitmap1;
    ID2D1RenderTarget *rt;
    IDXGISurface *surface;
    ID2D1Bitmap *bitmap;
    D2D1_COLOR_F color;
    BOOL match, match2;
    D2D1_RECT_F rect;
    D2D1_SIZE_U size;
    ULONG refcount;
    HRESULT hr;

    static const DWORD bitmap_data[] =
    {
        0x7f7f0000, 0x7f7f7f00, 0x7f007f00, 0x7f007f7f,
        0x7f00007f, 0x7f7f007f, 0x7f000000, 0x7f404040,
        0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f, 0x7f000000,
        0x7f7f7f7f, 0x7f000000, 0x7f000000, 0x7f000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f / 40.0f;
    bitmap_desc.dpiY = 96.0f / 30.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &bitmap_brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(bitmap_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    ID2D1BitmapBrush_SetExtendModeX(bitmap_brush, D2D1_EXTEND_MODE_WRAP);
    ID2D1BitmapBrush_SetExtendModeY(bitmap_brush, D2D1_EXTEND_MODE_WRAP);

    set_color(&color, 0.0f, 1.0f, 0.0f, 0.75f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &color_brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "48c41aff3a130a17ee210866b2ab7d36763934d5");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 0.0f, 0.0f, 0.25f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "6487e683730fb5a77c1911388d00b04664c5c4e4");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.0f, 0.0f, 1.0f, 0.75f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "7a35ba09e43cbaf591388ff1ef8de56157630c98");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);

    set_rect(&rect,   0.0f,   0.0f, 160.0f, 120.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f,   0.0f, 320.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f,   0.0f, 480.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetBitmap(bitmap_brush, bitmap);

    set_rect(&rect,   0.0f, 120.0f, 160.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 1.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f, 120.0f, 320.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f, 120.0f, 480.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    set_rect(&rect,   0.0f, 240.0f, 160.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 160.0f, 240.0f, 320.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 320.0f, 240.0f, 480.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "14f8ac64b70966c7c3c6281c59aaecdb17c3b16a");
    ok(match, "Surface does not match.\n");

    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    rt_desc.dpiX = 0.0f;
    rt_desc.dpiY = 0.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    rt = create_render_target_desc(ctx.surface, &rt_desc, d3d11, D2D1_FACTORY_TYPE_SINGLE_THREADED);
    ok(!!rt, "Failed to create render target.\n");

    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetBitmap(bitmap_brush, bitmap);

    ID2D1BitmapBrush_Release(bitmap_brush);
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &bitmap_brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(bitmap_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    ID2D1BitmapBrush_SetExtendModeX(bitmap_brush, D2D1_EXTEND_MODE_WRAP);
    ID2D1BitmapBrush_SetExtendModeY(bitmap_brush, D2D1_EXTEND_MODE_WRAP);

    ID2D1SolidColorBrush_Release(color_brush);
    set_color(&color, 0.0f, 1.0f, 0.0f, 0.75f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &color_brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "b44510bf2d2e61a8d7c0ad862de49a471f1fd13f");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 0.0f, 0.0f, 0.25f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "2184f4a9198fc1de09ac85301b7a03eebadd9b81");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.0f, 0.0f, 1.0f, 0.75f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "6527ec83b4039c895b50f9b3e144fe0cf90d1889");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);

    set_rect(&rect,   0.0f,   0.0f, 160.0f, 120.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f,   0.0f, 320.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f,   0.0f, 480.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetBitmap(bitmap_brush, bitmap);

    set_rect(&rect,   0.0f, 120.0f, 160.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 1.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f, 120.0f, 320.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f, 120.0f, 480.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    set_rect(&rect,   0.0f, 240.0f, 160.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 160.0f, 240.0f, 320.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 320.0f, 240.0f, 480.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "465f5a3190d7bde408b3206b4be939fb22f8a3d6");
    ok(match, "Surface does not match.\n");

    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Bitmap has %lu references left.\n", refcount);
    ID2D1BitmapBrush_Release(bitmap_brush);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_size_u(&size, 4, 4);
    memset(&bitmap_desc1, 0, sizeof(bitmap_desc1));
    bitmap_desc1.dpiX = 96.0f;
    bitmap_desc1.dpiY = 96.0f;
    bitmap_desc1.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc1.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc1.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    hr = ID2D1DeviceContext_CreateBitmap(context, size, bitmap_data, 4 * sizeof(*bitmap_data),
            &bitmap_desc1, &bitmap1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DeviceContext_BeginDraw(context);
    ID2D1DeviceContext_SetTarget(context, (ID2D1Image *)bitmap1);
    set_rect(&rect, 0.0f, 2.0f, 1.0f, 2.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 1.0f);
    ID2D1DeviceContext_FillRectangle(context, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 1.0f, 2.0f, 3.0f, 3.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.75f);
    ID2D1DeviceContext_FillRectangle(context, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 3.0f, 2.0f, 3.0f, 3.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.25f);
    ID2D1DeviceContext_FillRectangle(context, &rect, (ID2D1Brush *)color_brush);
    hr = ID2D1DeviceContext_EndDraw(context, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    surface = ctx.surface;
    hr = ID2D1Bitmap1_GetSurface(bitmap1, &ctx.surface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "e7ee77e89745fa5d195fd78bd398738330cfcde8");
    match2 = compare_surface(&ctx, "4855c7c082c8ede364cf6e2dcde83f95b88aecbe");
    ok(match || broken(match2) /* Win7 TestBots */, "Surface does not match.\n");
    IDXGISurface_Release(ctx.surface);
    ctx.surface = surface;
    ID2D1DeviceContext_SetTarget(context, NULL);

    ID2D1Bitmap1_Release(bitmap1);
    ID2D1DeviceContext_Release(context);
    ID2D1SolidColorBrush_Release(color_brush);
    ID2D1RenderTarget_Release(rt);
    release_test_context(&ctx);
}

static void test_shared_bitmap(BOOL d3d11)
{
    IWICBitmap *wic_bitmap1, *wic_bitmap2;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1RenderTarget *rt1, *rt2, *rt3;
    IDXGISurface *surface2;
    ID2D1Factory *factory1, *factory2;
    IWICImagingFactory *wic_factory;
    ID2D1Bitmap *bitmap1, *bitmap2;
    DXGI_SURFACE_DESC surface_desc;
    D2D1_PIXEL_FORMAT pixel_format;
    struct d2d1_test_context ctx;
    IDXGISwapChain *swapchain2;
    D2D1_SIZE_U size = {4, 4};
    IDXGISurface1 *surface3;
    IDXGIDevice *device2;
    HWND window2;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    window2 = create_window();
    swapchain2 = create_swapchain(ctx.device, window2, TRUE);
    hr = IDXGISwapChain_GetBuffer(swapchain2, 0, &IID_IDXGISurface, (void **)&surface2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 640, 480,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 640, 480,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* DXGI surface render targets with the same device and factory. */
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, ctx.surface, &desc, &rt1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmap(rt1, size, NULL, 0, &bitmap_desc, &bitmap1);
    check_bitmap_surface(bitmap1, TRUE, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, surface2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_bitmap_surface(bitmap2, TRUE, 0);
    ID2D1Bitmap_Release(bitmap2);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IUnknown, bitmap1, NULL, &bitmap2);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render targets with the same device but different factories. */
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory2, surface2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_WRONG_FACTORY, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render targets with different devices but the same factory. */
    IDXGISurface_Release(surface2);
    IDXGISwapChain_Release(swapchain2);
    device2 = create_device(d3d11);
    ok(!!device2, "Failed to create device.\n");
    swapchain2 = create_swapchain(device2, window2, TRUE);
    hr = IDXGISwapChain_GetBuffer(swapchain2, 0, &IID_IDXGISurface, (void **)&surface2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, surface2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_UNSUPPORTED_OPERATION, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render targets with different devices and different factories. */
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory2, surface2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_WRONG_FACTORY, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render target and WIC bitmap render target, same factory. */
    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory1, wic_bitmap2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_UNSUPPORTED_OPERATION, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* WIC bitmap render targets on different D2D factories. */
    ID2D1Bitmap_Release(bitmap1);
    ID2D1RenderTarget_Release(rt1);
    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory1, wic_bitmap1, &desc, &rt1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmap(rt1, size, NULL, 0, &bitmap_desc, &bitmap1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt1, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rt3 == rt1, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    ID2D1GdiInteropRenderTarget_Release(interop);

    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory2, wic_bitmap2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_WRONG_FACTORY, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* WIC bitmap render targets on the same D2D factory. */
    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory1, wic_bitmap2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_bitmap_surface(bitmap2, FALSE, 0);
    ID2D1Bitmap_Release(bitmap2);
    ID2D1RenderTarget_Release(rt2);

    /* Shared DXGI surface. */
    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, surface2, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 0.0f;
    bitmap_desc.dpiY = 0.0f;

    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IDXGISurface, surface2, &bitmap_desc, &bitmap2);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* vista */, "Got unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        static const struct bitmap_format_test
        {
            D2D1_PIXEL_FORMAT original;
            D2D1_PIXEL_FORMAT result;
            HRESULT hr;
        }
        bitmap_format_tests[] =
        {
            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },

            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },

            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },
            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },
        };
        unsigned int i;

        size = ID2D1Bitmap_GetPixelSize(bitmap2);
        hr = IDXGISurface_GetDesc(surface2, &surface_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(size.width == surface_desc.Width && size.height == surface_desc.Height, "Got wrong bitmap size.\n");

        check_bitmap_surface(bitmap2, TRUE, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);

        ID2D1Bitmap_Release(bitmap2);

        /* IDXGISurface1 is supported too. */
        if (IDXGISurface_QueryInterface(surface2, &IID_IDXGISurface1, (void **)&surface3) == S_OK)
        {
            hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IDXGISurface1, surface3, &bitmap_desc, &bitmap2);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            ID2D1Bitmap_Release(bitmap2);
            IDXGISurface1_Release(surface3);
        }

        for (i = 0; i < ARRAY_SIZE(bitmap_format_tests); ++i)
        {
            bitmap_desc.pixelFormat = bitmap_format_tests[i].original;

            hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IDXGISurface, surface2, &bitmap_desc, &bitmap2);
            todo_wine_if(i == 2 || i == 3 || i == 5 || i == 6)
            ok(hr == bitmap_format_tests[i].hr, "%u: Got unexpected hr %#lx.\n", i, hr);

            if (SUCCEEDED(hr) && hr == bitmap_format_tests[i].hr)
            {
                pixel_format = ID2D1Bitmap_GetPixelFormat(bitmap2);
                ok(pixel_format.format == bitmap_format_tests[i].result.format, "%u: unexpected pixel format %#x.\n",
                        i, pixel_format.format);
                ok(pixel_format.alphaMode == bitmap_format_tests[i].result.alphaMode, "%u: unexpected alpha mode %d.\n",
                        i, pixel_format.alphaMode);
            }

            if (SUCCEEDED(hr))
                ID2D1Bitmap_Release(bitmap2);
        }
    }

    ID2D1Bitmap_Release(bitmap1);

    /* Create from another bitmap, with a different description. */
    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt2, size, NULL, 0, &bitmap_desc, &bitmap1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        static const struct bitmap_format_test
        {
            D2D1_PIXEL_FORMAT original;
            D2D1_PIXEL_FORMAT result;
            HRESULT hr;
        }
        bitmap_format_tests[] =
        {
            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },

            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },

            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },
            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },
        };
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(bitmap_format_tests); ++i)
        {
            bitmap_desc.pixelFormat = bitmap_format_tests[i].original;

            hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, &bitmap_desc, &bitmap2);
            ok(hr == bitmap_format_tests[i].hr, "%u: Got unexpected hr %#lx.\n", i, hr);

            if (SUCCEEDED(hr) && hr == bitmap_format_tests[i].hr)
            {
                pixel_format = ID2D1Bitmap_GetPixelFormat(bitmap2);
                ok(pixel_format.format == bitmap_format_tests[i].result.format, "%u: unexpected pixel format %#x.\n",
                        i, pixel_format.format);
                ok(pixel_format.alphaMode == bitmap_format_tests[i].result.alphaMode, "%u: unexpected alpha mode %d.\n",
                        i, pixel_format.alphaMode);
            }

            if (SUCCEEDED(hr))
                ID2D1Bitmap_Release(bitmap2);
        }
    }

    ID2D1RenderTarget_Release(rt2);

    ID2D1Bitmap_Release(bitmap1);
    ID2D1RenderTarget_Release(rt1);
    ID2D1Factory_Release(factory2);
    ID2D1Factory_Release(factory1);
    IWICBitmap_Release(wic_bitmap2);
    IWICBitmap_Release(wic_bitmap1);
    IDXGISurface_Release(surface2);
    IDXGISwapChain_Release(swapchain2);
    IDXGIDevice_Release(device2);
    release_test_context(&ctx);
    DestroyWindow(window2);
    CoUninitialize();
}

static void test_bitmap_updates(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1Bitmap *bitmap, *dst_bitmap;
    D2D1_RECT_U dst_rect, src_rect;
    struct d2d1_test_context ctx;
    D2D1_POINT_2U dst_point;
    ID2D1RenderTarget *rt;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    D2D1_SIZE_U size;
    HRESULT hr;
    BOOL match;

    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0xffffff00, 0xff00ff00, 0xff00ffff,
        0xff0000ff, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 320.0f, 240.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    ID2D1Bitmap_Release(bitmap);

    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, 0.0f, 240.0f, 320.0f, 480.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    set_rect_u(&dst_rect, 1, 1, 3, 3);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, bitmap_data, 4 * sizeof(*bitmap_data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_rect_u(&dst_rect, 0, 3, 3, 4);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, &bitmap_data[6], 4 * sizeof(*bitmap_data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_rect_u(&dst_rect, 0, 0, 4, 1);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, &bitmap_data[10], 4 * sizeof(*bitmap_data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_rect_u(&dst_rect, 0, 1, 1, 3);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, &bitmap_data[2], sizeof(*bitmap_data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_rect_u(&dst_rect, 4, 4, 3, 1);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, bitmap_data, sizeof(*bitmap_data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_rect(&rect, 320.0f, 240.0f, 640.0f, 480.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    hr = ID2D1Bitmap_CopyFromMemory(bitmap, NULL, bitmap_data, 4 * sizeof(*bitmap_data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_rect(&rect, 320.0f, 0.0f, 640.0f, 240.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_surface(&ctx, "cb8136c91fbbdc76bb83b8c09edc1907b0a5d0a6");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &dst_bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Bitmap_CopyFromBitmap(dst_bitmap, NULL, bitmap, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    dst_point.x = 1; dst_point.y = 1;
    hr = ID2D1Bitmap_CopyFromBitmap(dst_bitmap, &dst_point, bitmap, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    dst_point.x = 0; dst_point.y = 1;
    set_rect_u(&src_rect, 1, 1, 3, 3);
    hr = ID2D1Bitmap_CopyFromBitmap(dst_bitmap, &dst_point, bitmap, &src_rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    dst_point.x = 1; dst_point.y = 0;
    set_rect_u(&src_rect, 2, 2, 1, 1);
    hr = ID2D1Bitmap_CopyFromBitmap(dst_bitmap, &dst_point, bitmap, &src_rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_rect_u(&src_rect, 1, 1, 2, 2);
    hr = ID2D1Bitmap_CopyFromBitmap(dst_bitmap, NULL, bitmap, &src_rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, 320.0f, 0.0f, 640.0f, 240.0f);
    ID2D1RenderTarget_DrawBitmap(rt, dst_bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "a1758bd644d897f75d43769ba26456af70cd9056");
    ok(match, "Surface does not match.\n");
    ID2D1Bitmap_Release(dst_bitmap);

    ID2D1Bitmap_Release(bitmap);
    release_test_context(&ctx);
}

static void test_opacity_brush(BOOL d3d11)
{
    ID2D1BitmapBrush *bitmap_brush, *opacity_brush;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1RectangleGeometry *geometry;
    ID2D1SolidColorBrush *color_brush;
    struct d2d1_test_context ctx;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    ID2D1Bitmap *bitmap;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    D2D1_SIZE_U size;
    ULONG refcount;
    HRESULT hr;
    BOOL match;

    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0x40ffff00, 0x4000ff00, 0xff00ffff,
        0x7f0000ff, 0x00ff00ff, 0x00000000, 0x7f7f7f7f,
        0x7fffffff, 0x00ffffff, 0x00ffffff, 0x7f000000,
        0xffffffff, 0x40000000, 0x40000000, 0xff000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    factory = ctx.factory;

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_color(&color, 0.0f, 1.0f, 0.0f, 0.8f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &color_brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &opacity_brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(opacity_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    set_size_u(&size, 1, 1);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, &bitmap_data[2], sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &bitmap_brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(bitmap_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 120.0f, 120.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 120.0f, 120.0f, 200.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)opacity_brush);

    set_rect(&rect, 200.0f, 120.0f, 280.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 360.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 40.0f, 360.0f, 120.0f, 600.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)opacity_brush, (ID2D1Brush *)color_brush);
    ID2D1RectangleGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 120.0f, 360.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 120.0f, 360.0f, 200.0f, 600.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)color_brush, (ID2D1Brush *)opacity_brush);
    ID2D1RectangleGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 360.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 200.0f, 360.0f, 280.0f, 600.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)bitmap_brush, (ID2D1Brush *)opacity_brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == D2DERR_INCOMPATIBLE_BRUSH_TYPES, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "7141c6c7b3decb91196428efb1856bcbf9872935");
    ok(match, "Surface does not match.\n");
    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)bitmap_brush, (ID2D1Brush *)opacity_brush);
    ID2D1RectangleGeometry_Release(geometry);

    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.5f);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 600.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 40.0f, 600.0f, 120.0f, 840.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)opacity_brush, (ID2D1Brush *)color_brush);
    ID2D1RectangleGeometry_Release(geometry);

    ID2D1BitmapBrush_SetOpacity(opacity_brush, 0.8f);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 120.0f, 600.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 120.0f, 600.0f, 200.0f, 840.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)opacity_brush, (ID2D1Brush *)bitmap_brush);
    ID2D1RectangleGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 600.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 200.0f, 600.0f, 280.0f, 840.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)bitmap_brush, (ID2D1Brush *)opacity_brush);
    ID2D1RectangleGeometry_Release(geometry);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_surface(&ctx, "c3a5802d1750efa3e9122c1a92f6064df3872732");
    ok(match, "Surface does not match.\n");

    ID2D1BitmapBrush_Release(bitmap_brush);
    ID2D1BitmapBrush_Release(opacity_brush);
    ID2D1SolidColorBrush_Release(color_brush);
    release_test_context(&ctx);
}

static void test_create_target(BOOL d3d11)
{
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    HRESULT hr;
    static const struct
    {
        float dpi_x, dpi_y;
        float rt_dpi_x, rt_dpi_y;
        HRESULT hr;
    }
    create_dpi_tests[] =
    {
        {   0.0f,   0.0f, 96.0f, 96.0f, S_OK },
        { 192.0f,   0.0f, 96.0f, 96.0f, E_INVALIDARG },
        {   0.0f, 192.0f, 96.0f, 96.0f, E_INVALIDARG },
        { 192.0f, -10.0f, 96.0f, 96.0f, E_INVALIDARG },
        { -10.0f, 192.0f, 96.0f, 96.0f, E_INVALIDARG },
        {  48.0f,  96.0f, 48.0f, 96.0f, S_OK },
    };
    unsigned int i;

    if (!init_test_context(&ctx, d3d11))
        return;

    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        ID2D1GdiInteropRenderTarget *interop;
        D2D1_RENDER_TARGET_PROPERTIES desc;
        ID2D1RenderTarget *rt2;
        float dpi_x, dpi_y;
        IUnknown *unk;

        desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        desc.dpiX = create_dpi_tests[i].dpi_x;
        desc.dpiY = create_dpi_tests[i].dpi_y;
        desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
        desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

        hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(ctx.factory, ctx.surface, &desc, &rt);
        ok(hr == create_dpi_tests[i].hr, "Test %u: Got unexpected hr %#lx, expected %#lx.\n",
                i, hr, create_dpi_tests[i].hr);

        if (FAILED(hr))
            continue;

        hr = ID2D1RenderTarget_QueryInterface(rt, &IID_IUnknown, (void **)&unk);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(unk == (IUnknown *)rt, "Expected same interface pointer.\n");
        IUnknown_Release(unk);

        hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt2);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(rt2 == rt, "Unexpected render target\n");
        ID2D1RenderTarget_Release(rt2);
        ID2D1GdiInteropRenderTarget_Release(interop);

        ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
        ok(dpi_x == create_dpi_tests[i].rt_dpi_x, "Wrong dpi_x %.8e, expected %.8e, test %u\n",
            dpi_x, create_dpi_tests[i].rt_dpi_x, i);
        ok(dpi_y == create_dpi_tests[i].rt_dpi_y, "Wrong dpi_y %.8e, expected %.8e, test %u\n",
            dpi_y, create_dpi_tests[i].rt_dpi_y, i);

        ID2D1RenderTarget_Release(rt);
    }

    release_test_context(&ctx);
}

static void test_draw_text_layout(BOOL d3d11)
{
    static const struct
    {
        D2D1_TEXT_ANTIALIAS_MODE aa_mode;
        DWRITE_RENDERING_MODE rendering_mode;
        HRESULT hr;
    }
    antialias_mode_tests[] =
    {
        { D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, DWRITE_RENDERING_MODE_ALIASED },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_ALIASED },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_DEFAULT },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_OUTLINE },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_DEFAULT },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_OUTLINE },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_DEFAULT },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_OUTLINE, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_ALIASED, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_ALIASED, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC, E_INVALIDARG },
    };
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1RenderTarget *rt, *rt2;
    HRESULT hr;
    IDWriteFactory *dwrite_factory;
    IDWriteTextFormat *text_format;
    IDWriteTextLayout *text_layout;
    struct d2d1_test_context ctx;
    ID2D1Factory *factory2;
    D2D1_POINT_2F origin;
    DWRITE_TEXT_RANGE range;
    D2D1_COLOR_F color;
    ID2D1SolidColorBrush *brush, *brush2;
    ID2D1RectangleGeometry *geometry;
    D2D1_RECT_F rect;
    unsigned int i;

    if (!init_test_context(&ctx, d3d11))
        return;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ctx.factory != factory2, "got same factory\n");

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(ctx.factory, ctx.surface, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory2, ctx.surface, &desc, &rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown **)&dwrite_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextFormat(dwrite_factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"", &text_format);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(dwrite_factory, L"text", 4, text_format, 100.0f, 100.0f, &text_layout);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt2, &color, NULL, &brush2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* effect brush is created from different factory */
    range.startPosition = 0;
    range.length = 4;
    hr = IDWriteTextLayout_SetDrawingEffect(text_layout, (IUnknown*)brush2, range);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);

    origin.x = origin.y = 0.0f;
    ID2D1RenderTarget_DrawTextLayout(rt, origin, text_layout, (ID2D1Brush*)brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    todo_wine ok(hr == D2DERR_WRONG_FACTORY, "Got unexpected hr %#lx.\n", hr);

    /* Effect is d2d resource, but not a brush. */
    set_rect(&rect, 0.0f, 0.0f, 10.0f, 10.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 4;
    hr = IDWriteTextLayout_SetDrawingEffect(text_layout, (IUnknown*)geometry, range);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RectangleGeometry_Release(geometry);

    ID2D1RenderTarget_BeginDraw(rt);

    origin.x = origin.y = 0.0f;
    ID2D1RenderTarget_DrawTextLayout(rt, origin, text_layout, (ID2D1Brush*)brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(antialias_mode_tests); ++i)
    {
        IDWriteRenderingParams *rendering_params;

        ID2D1RenderTarget_SetTextAntialiasMode(rt, antialias_mode_tests[i].aa_mode);

        hr = IDWriteFactory_CreateCustomRenderingParams(dwrite_factory, 2.0f, 1.0f, 0.0f, DWRITE_PIXEL_GEOMETRY_FLAT,
                antialias_mode_tests[i].rendering_mode, &rendering_params);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        ID2D1RenderTarget_SetTextRenderingParams(rt, rendering_params);

        ID2D1RenderTarget_BeginDraw(rt);

        ID2D1RenderTarget_DrawTextLayout(rt, origin, text_layout, (ID2D1Brush *)brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

        hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
        ok(hr == antialias_mode_tests[i].hr, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        IDWriteRenderingParams_Release(rendering_params);
    }

    ID2D1SolidColorBrush_Release(brush2);
    ID2D1SolidColorBrush_Release(brush);
    IDWriteTextFormat_Release(text_format);
    IDWriteTextLayout_Release(text_layout);
    IDWriteFactory_Release(dwrite_factory);
    ID2D1RenderTarget_Release(rt);
    ID2D1RenderTarget_Release(rt2);

    ID2D1Factory_Release(factory2);
    release_test_context(&ctx);
}

static void create_target_dibsection(HDC hdc, UINT32 width, UINT32 height)
{
    char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bmi = (BITMAPINFO*)bmibuf;
    HBITMAP hbm;

    memset(bmi, 0, sizeof(bmibuf));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = -height;
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    ok(hbm != NULL, "Failed to create a dib section.\n");

    DeleteObject(SelectObject(hdc, hbm));
}

static void test_dc_target(BOOL d3d11)
{
    static const D2D1_PIXEL_FORMAT invalid_formats[] =
    {
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
    };
    D2D1_TEXT_ANTIALIAS_MODE text_aa_mode;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    D2D1_MATRIX_3X2_F matrix, matrix2;
    ID2D1DCRenderTarget *rt, *rt2;
    struct d2d1_test_context ctx;
    D2D1_ANTIALIAS_MODE aa_mode;
    ID2D1SolidColorBrush *brush;
    ID2D1RenderTarget *rt3;
    FLOAT dpi_x, dpi_y;
    D2D1_COLOR_F color;
    HENHMETAFILE hemf;
    D2D1_SIZE_U sizeu;
    D2D1_SIZE_F size;
    D2D1_TAG t1, t2;
    unsigned int i;
    HDC hdc, hdc2;
    HMETAFILE hmf;
    D2D_RECT_F r;
    COLORREF clr;
    HRESULT hr;
    RECT rect;
    HWND hwnd;

    if (!init_test_context(&ctx, d3d11))
        return;

    for (i = 0; i < ARRAY_SIZE(invalid_formats); ++i)
    {
        desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        desc.pixelFormat = invalid_formats[i];
        desc.dpiX = 96.0f;
        desc.dpiY = 96.0f;
        desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
        desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

        hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &rt);
        ok(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT, "Got unexpected hr %#lx.\n", hr);
    }

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 96.0f;
    desc.dpiY = 96.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DCRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rt3 == (ID2D1RenderTarget *)rt, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1DCRenderTarget, (void **)&rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rt2 == rt, "Unexpected render target\n");
    ID2D1DCRenderTarget_Release(rt2);
    ID2D1GdiInteropRenderTarget_Release(interop);

    size = ID2D1DCRenderTarget_GetSize(rt);
    ok(size.width == 0.0f, "got width %.08e.\n", size.width);
    ok(size.height == 0.0f, "got height %.08e.\n", size.height);

    sizeu = ID2D1DCRenderTarget_GetPixelSize(rt);
    ok(sizeu.width == 0, "got width %u.\n", sizeu.width);
    ok(sizeu.height == 0, "got height %u.\n", sizeu.height);

    /* object creation methods work without BindDC() */
    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1DCRenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1SolidColorBrush_Release(brush);

    ID2D1DCRenderTarget_BeginDraw(rt);
    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);

    ID2D1DCRenderTarget_Release(rt);

    /* BindDC() */
    hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    aa_mode = ID2D1DCRenderTarget_GetAntialiasMode(rt);
    ok(aa_mode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, "Got wrong default aa mode %d.\n", aa_mode);
    text_aa_mode = ID2D1DCRenderTarget_GetTextAntialiasMode(rt);
    ok(text_aa_mode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, "Got wrong default text aa mode %d.\n", text_aa_mode);

    ID2D1DCRenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f && dpi_y == 96.0f, "Got dpi_x %f, dpi_y %f.\n", dpi_x, dpi_y);

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "Failed to create an HDC.\n");

    create_target_dibsection(hdc, 16, 16);

    SetRect(&rect, 0, 0, 32, 32);
    hr = ID2D1DCRenderTarget_BindDC(rt, NULL, &rect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Target properties are retained during BindDC() */
    ID2D1DCRenderTarget_SetTags(rt, 1, 2);
    ID2D1DCRenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1DCRenderTarget_SetTextAntialiasMode(rt, D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 600.0f);
    ID2D1DCRenderTarget_SetTransform(rt, &matrix);

    hr = ID2D1DCRenderTarget_BindDC(rt, hdc, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DCRenderTarget_GetTags(rt, &t1, &t2);
    ok(t1 == 1 && t2 == 2, "Got wrong tags.\n");

    aa_mode = ID2D1DCRenderTarget_GetAntialiasMode(rt);
    ok(aa_mode == D2D1_ANTIALIAS_MODE_ALIASED, "Got wrong aa mode %d.\n", aa_mode);

    text_aa_mode = ID2D1DCRenderTarget_GetTextAntialiasMode(rt);
    ok(text_aa_mode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, "Got wrong text aa mode %d.\n", text_aa_mode);

    ID2D1DCRenderTarget_GetTransform(rt, &matrix2);
    ok(!memcmp(&matrix, &matrix2, sizeof(matrix)), "Got wrong target transform.\n");

    set_matrix_identity(&matrix);
    ID2D1DCRenderTarget_SetTransform(rt, &matrix);

    /* target size comes from specified dimensions, not from selected bitmap size */
    size = ID2D1DCRenderTarget_GetSize(rt);
    ok(size.width == 32.0f, "got width %.08e.\n", size.width);
    ok(size.height == 32.0f, "got height %.08e.\n", size.height);

    /* clear one HDC to red, switch to another one, partially fill it and test contents */
    ID2D1DCRenderTarget_BeginDraw(rt);

    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1DCRenderTarget_Clear(rt, &color);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    clr = GetPixel(hdc, 0, 0);
    ok(clr == RGB(255, 0, 0), "Got unexpected colour 0x%08lx.\n", clr);

    hdc2 = CreateCompatibleDC(NULL);
    ok(hdc2 != NULL, "Failed to create an HDC.\n");

    create_target_dibsection(hdc2, 16, 16);

    hr = ID2D1DCRenderTarget_BindDC(rt, hdc2, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    clr = GetPixel(hdc2, 0, 0);
    ok(clr == 0, "Got unexpected colour 0x%08lx.\n", clr);

    set_color(&color, 0.0f, 1.0f, 0.0f, 1.0f);
    hr = ID2D1DCRenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);

    r.left = r.top = 0.0f;
    r.bottom = 16.0f;
    r.right = 8.0f;
    ID2D1DCRenderTarget_FillRectangle(rt, &r, (ID2D1Brush*)brush);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1SolidColorBrush_Release(brush);

    clr = GetPixel(hdc2, 0, 0);
    ok(clr == RGB(0, 255, 0), "Got unexpected colour 0x%08lx.\n", clr);

    clr = GetPixel(hdc2, 10, 0);
    ok(clr == 0, "Got unexpected colour 0x%08lx.\n", clr);

    /* Invalid DC. */
    hr = ID2D1DCRenderTarget_BindDC(rt, (HDC)0xdeadbeef, &rect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);

    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1DCRenderTarget_Clear(rt, &color);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    clr = GetPixel(hdc2, 0, 0);
    ok(clr == RGB(255, 0, 0), "Got unexpected colour 0x%08lx.\n", clr);

    hr = ID2D1DCRenderTarget_BindDC(rt, NULL, &rect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1DCRenderTarget_Clear(rt, &color);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    clr = GetPixel(hdc2, 0, 0);
    ok(clr == RGB(0, 0, 255), "Got unexpected colour 0x%08lx.\n", clr);

    DeleteDC(hdc);
    DeleteDC(hdc2);

    /* Metafile context. */
    hdc = CreateMetaFileA(NULL);
    ok(!!hdc, "Failed to create a device context.\n");

    hr = ID2D1DCRenderTarget_BindDC(rt, hdc, &rect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hmf = CloseMetaFile(hdc);
    ok(!!hmf, "Failed to close a metafile, error %ld.\n", GetLastError());
    DeleteMetaFile(hmf);

    /* Enhanced metafile context. */
    hdc = CreateEnhMetaFileA(NULL, NULL, NULL, NULL);
    ok(!!hdc, "Failed to create a device context.\n");

    hr = ID2D1DCRenderTarget_BindDC(rt, hdc, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hemf = CloseEnhMetaFile(hdc);
    ok(!!hemf, "Failed to close a metafile, error %ld.\n", GetLastError());
    DeleteEnhMetaFile(hemf);

    /* Window context. */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 100, 100, 0, 0, 0, NULL);
    ok(!!hwnd, "Failed to create a test window.\n");

    hdc = GetDC(hwnd);
    ok(!!hdc, "Failed to get a context.\n");

    hr = ID2D1DCRenderTarget_BindDC(rt, hdc, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    ID2D1DCRenderTarget_Release(rt);
    release_test_context(&ctx);
}

static void test_dc_target_gdi_interop(BOOL d3d11)
{
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    struct d2d1_test_context ctx;
    ID2D1DCRenderTarget *rt;
    HDC hdc, target_hdc;
    HRESULT hr;
    RECT rect;

    if (!init_test_context(&ctx, d3d11))
        return;

    target_hdc = CreateCompatibleDC(NULL);
    ok(!!target_hdc, "Failed to create an HDC.\n");
    create_target_dibsection(target_hdc, 16, 16);

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 96.0f;
    desc.dpiY = 96.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Default usage. */
    hr = ID2D1DCRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);
    hdc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &hdc);
    todo_wine
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ok(!hdc, "Got unexpected DC %p.\n", hdc);
    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);

    /* Default usage with set target context. */

    SetRect(&rect, 0, 0, 16, 16);
    hr = ID2D1DCRenderTarget_BindDC(rt, target_hdc, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);
    hdc = NULL;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &hdc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!hdc, "Got unexpected DC %p.\n", hdc);
    hr = ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1GdiInteropRenderTarget_Release(interop);

    DeleteDC(target_hdc);

    ID2D1DCRenderTarget_Release(rt);
    release_test_context(&ctx);
}

static void test_hwnd_target(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1HwndRenderTarget *rt, *rt2;
    D2D1_PIXEL_FORMAT pixel_format;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt3;
    D2D1_SIZE_U size;
    unsigned int i;
    HRESULT hr;

    static const struct format_test
    {
        D2D1_PIXEL_FORMAT format;
        D2D1_PIXEL_FORMAT expected_format;
        BOOL expected_failure;
    }
    format_tests[] =
    {
        {{DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
        {{DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}},
        {{DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT}, TRUE},
        {{DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
        {{DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
        {{DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}},
        {{DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT}, TRUE},
        {{DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_rt_desc.hwnd = NULL;
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
    ok(hr != S_OK, "Got unexpected hr %#lx.\n", hr);

    hwnd_rt_desc.hwnd = (HWND)0xdeadbeef;
    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
    ok(hr != S_OK, "Got unexpected hr %#lx.\n", hr);

    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");
    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1HwndRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rt3 == (ID2D1RenderTarget *)rt, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1HwndRenderTarget, (void **)&rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rt2 == rt, "Unexpected render target\n");
    ID2D1HwndRenderTarget_Release(rt2);
    ID2D1GdiInteropRenderTarget_Release(interop);

    size.width = 128;
    size.height = 64;
    hr = ID2D1HwndRenderTarget_Resize(rt, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1HwndRenderTarget_Release(rt);

    /* Test render target format */
    for (i = 0; i < ARRAY_SIZE(format_tests); ++i)
    {
        winetest_push_context("test %d", i);

        desc.pixelFormat = format_tests[i].format;
        hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
        if (format_tests[i].expected_failure)
        {
            ok(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT, "Got unexpected hr %#lx.\n", hr);
            winetest_pop_context();
            continue;
        }
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        pixel_format = ID2D1HwndRenderTarget_GetPixelFormat(rt);
        ok(pixel_format.format == format_tests[i].expected_format.format,
                "Got unexpected format %#x.\n", pixel_format.format);
        ok(pixel_format.alphaMode == format_tests[i].expected_format.alphaMode,
                "Got unexpected alpha mode %d.\n", pixel_format.alphaMode);

        ID2D1HwndRenderTarget_Release(rt);
        winetest_pop_context();
    }

    DestroyWindow(hwnd_rt_desc.hwnd);
    release_test_context(&ctx);
}

static void test_hwnd_target_gdi_interop(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    struct d2d1_test_context ctx;
    ID2D1HwndRenderTarget *rt;
    HRESULT hr;
    HDC dc;

    if (!init_test_context(&ctx, d3d11))
        return;

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_rt_desc.hwnd = create_window();
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1HwndRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1HwndRenderTarget_BeginDraw(rt);
    dc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == D2DERR_TARGET_NOT_GDI_COMPATIBLE, "Got unexpected hr %#lx.\n", hr);
    ok(!dc, "Got unexpected DC %p.\n", dc);
    hr = ID2D1HwndRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GdiInteropRenderTarget_Release(interop);

    ID2D1HwndRenderTarget_Release(rt);

    /* GDI-compatible. */
    desc.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1HwndRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1HwndRenderTarget_BeginDraw(rt);
    dc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!dc, "Got unexpected DC %p.\n", dc);
    hr = ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1HwndRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GdiInteropRenderTarget_Release(interop);

    ID2D1HwndRenderTarget_Release(rt);

    DestroyWindow(hwnd_rt_desc.hwnd);
    release_test_context(&ctx);
}

static IDXGISurface *create_surface(IDXGIDevice *dxgi_device, unsigned int width,
        unsigned int height, unsigned int bind_flags, unsigned int misc_flags, DXGI_FORMAT format)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *texture;
    ID3D10Device *d3d_device;
    IDXGISurface *surface;
    HRESULT hr;

    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = format;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = bind_flags;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = misc_flags;

    hr = IDXGIDevice_QueryInterface(dxgi_device, &IID_ID3D10Device, (void **)&d3d_device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3D10Device_CreateTexture2D(d3d_device, &texture_desc, NULL, &texture);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_Release(d3d_device);
    ID3D10Texture2D_Release(texture);

    return surface;
}

static void test_dxgi_surface_target_gdi_interop(BOOL d3d11)
{
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1DeviceContext *device_context;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    IDXGISurface *surface;
    ID2D1Bitmap *bitmap;
    ULONG refcount;
    HRESULT hr;
    HDC dc;

    if (!init_test_context(&ctx, d3d11))
        return;

    surface = create_surface(ctx.device, 64, 64, D3D10_BIND_RENDER_TARGET,
            0, DXGI_FORMAT_B8G8R8A8_UNORM);
    ok(!!surface, "Failed to create a surface.\n");

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(ctx.factory, surface, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_BeginDraw(rt);
    dc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == D2DERR_TARGET_NOT_GDI_COMPATIBLE, "Got unexpected hr %#lx.\n", hr);
    ok(!dc, "Got unexpected DC %p.\n", dc);
    hr = ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GdiInteropRenderTarget_Release(interop);
    ID2D1RenderTarget_Release(rt);

    /* GDI-compatible, requested usage does not match the surface. */
    desc.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(ctx.factory, surface, &desc, &rt);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    refcount = IDXGISurface_Release(surface);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    /* GDI-compatible, surface is GDI-compatible. */
    surface = create_surface(ctx.device, 64, 64, D3D10_BIND_RENDER_TARGET,
            D3D10_RESOURCE_MISC_GDI_COMPATIBLE, DXGI_FORMAT_B8G8R8A8_UNORM);
    ok(!!surface, "Failed to create a surface.\n");

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(ctx.factory, surface, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context)))
    {
        ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
        check_bitmap_surface(bitmap, TRUE, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE);
        ID2D1Bitmap_Release(bitmap);
        ID2D1DeviceContext_Release(device_context);
    }

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_BeginDraw(rt);
    dc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!dc, "Got unexpected DC %p.\n", dc);
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    ok(!dc, "Got unexpected DC %p.\n", dc);
    hr = ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GdiInteropRenderTarget_Release(interop);
    ID2D1RenderTarget_Release(rt);

    /* GDI-compatible target wasn't requested, used surface is compatible. */
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(ctx.factory, surface, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_BeginDraw(rt);
    dc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == D2DERR_TARGET_NOT_GDI_COMPATIBLE, "Got unexpected hr %#lx.\n", hr);
    ok(!dc, "Got unexpected DC %p.\n", dc);
    hr = ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GdiInteropRenderTarget_Release(interop);
    ID2D1RenderTarget_Release(rt);

    refcount = IDXGISurface_Release(surface);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    release_test_context(&ctx);
}

#define test_compatible_target_size(r) test_compatible_target_size_(__LINE__, r)
static void test_compatible_target_size_(unsigned int line, ID2D1RenderTarget *rt)
{
    static const D2D1_SIZE_F size_1_0 = { 1.0f, 0.0f };
    static const D2D1_SIZE_F size_1_1 = { 1.0f, 1.0f };
    static const D2D1_SIZE_U px_size_1_1 = { 1, 1 };
    static const D2D1_SIZE_U zero_px_size;
    static const D2D1_SIZE_F zero_size;
    static const struct size_test
    {
        const D2D1_SIZE_U *pixel_size;
        const D2D1_SIZE_F *size;
    }
    size_tests[] =
    {
        { &zero_px_size, NULL },
        { &zero_px_size, &zero_size },
        { NULL, &zero_size },
        { NULL, &size_1_0 },
        { &px_size_1_1, &size_1_1 },
    };
    float dpi_x, dpi_y, rt_dpi_x, rt_dpi_y;
    D2D1_SIZE_U pixel_size, expected_size;
    ID2D1BitmapRenderTarget *bitmap_rt;
    ID2D1DeviceContext *context;
    unsigned int i;
    HRESULT hr;

    ID2D1RenderTarget_GetDpi(rt, &rt_dpi_x, &rt_dpi_y);

    for (i = 0; i < ARRAY_SIZE(size_tests); ++i)
    {
        hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, size_tests[i].size, size_tests[i].pixel_size,
                NULL, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &bitmap_rt);
        ok_(__FILE__, line)(SUCCEEDED(hr), "%u: Failed to create render target, hr %#lx.\n", i, hr);

        if (size_tests[i].pixel_size)
        {
            expected_size = *size_tests[i].pixel_size;
        }
        else if (size_tests[i].size)
        {
            expected_size.width = ceilf((size_tests[i].size->width * rt_dpi_x) / 96.0f);
            expected_size.height = ceilf((size_tests[i].size->height * rt_dpi_y) / 96.0f);
        }
        else
        {
            expected_size = ID2D1RenderTarget_GetPixelSize(rt);
        }

        pixel_size = ID2D1BitmapRenderTarget_GetPixelSize(bitmap_rt);
        ok_(__FILE__, line)(!memcmp(&pixel_size, &expected_size, sizeof(pixel_size)),
                "%u: unexpected target size %ux%u.\n", i, pixel_size.width, pixel_size.height);

        ID2D1BitmapRenderTarget_GetDpi(bitmap_rt, &dpi_x, &dpi_y);
        if (size_tests[i].pixel_size && size_tests[i].size && size_tests[i].size->width != 0.0f
                && size_tests[i].size->height != 0.0f)
        {
            ok_(__FILE__, line)(dpi_x == pixel_size.width * 96.0f / size_tests[i].size->width
                    && dpi_y == pixel_size.height * 96.0f / size_tests[i].size->height,
                    "%u: unexpected target dpi %.8ex%.8e.\n", i, dpi_x, dpi_y);
        }
        else
            ok_(__FILE__, line)(dpi_x == rt_dpi_x && dpi_y == rt_dpi_y,
                    "%u: unexpected target dpi %.8ex%.8e.\n", i, dpi_x, dpi_y);
        ID2D1BitmapRenderTarget_Release(bitmap_rt);
    }

    pixel_size.height = pixel_size.width = 0;
    hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, NULL, &pixel_size, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &bitmap_rt);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create render target, hr %#lx.\n", hr);

    if (SUCCEEDED(ID2D1BitmapRenderTarget_QueryInterface(bitmap_rt, &IID_ID2D1DeviceContext, (void **)&context)))
    {
        ID2D1Bitmap *bitmap;

        pixel_size = ID2D1DeviceContext_GetPixelSize(context);
        ok_(__FILE__, line)(!pixel_size.width && !pixel_size.height, "Unexpected target size %ux%u.\n",
                pixel_size.width, pixel_size.height);

        ID2D1DeviceContext_GetTarget(context, (ID2D1Image **)&bitmap);
        pixel_size = ID2D1Bitmap_GetPixelSize(bitmap);
        ok_(__FILE__, line)(!pixel_size.width && !pixel_size.height, "Unexpected target size %ux%u.\n",
                pixel_size.width, pixel_size.height);
        ID2D1Bitmap_Release(bitmap);

        ID2D1DeviceContext_Release(context);
    }

    ID2D1BitmapRenderTarget_Release(bitmap_rt);
}

static void test_bitmap_target(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_SIZE_U pixel_size, pixel_size2;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1BitmapRenderTarget *rt, *rt2;
    ID2D1HwndRenderTarget *hwnd_rt;
    ID2D1Bitmap *bitmap, *bitmap2;
    struct d2d1_test_context ctx;
    ID2D1DCRenderTarget *dc_rt;
    D2D1_SIZE_F size, size2;
    ID2D1RenderTarget *rt3;
    float dpi[2], dpi2[2];
    D2D1_COLOR_F color;
    ULONG refcount;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 96.0f;
    desc.dpiY = 192.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &hwnd_rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    test_compatible_target_size((ID2D1RenderTarget *)hwnd_rt);

    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, NULL, NULL, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1BitmapRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rt3 == (ID2D1RenderTarget *)rt, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1BitmapRenderTarget, (void **)&rt2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(rt2 == rt, "Unexpected render target\n");
    ID2D1BitmapRenderTarget_Release(rt2);
    ID2D1GdiInteropRenderTarget_Release(interop);

    /* See if parent target is referenced. */
    ID2D1HwndRenderTarget_AddRef(hwnd_rt);
    refcount = ID2D1HwndRenderTarget_Release(hwnd_rt);
    ok(refcount == 1, "Target should not have been referenced, got %lu.\n", refcount);

    /* Size was not specified, should match parent. */
    pixel_size = ID2D1HwndRenderTarget_GetPixelSize(hwnd_rt);
    pixel_size2 = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(!memcmp(&pixel_size, &pixel_size2, sizeof(pixel_size)), "Got target pixel size mismatch.\n");

    size = ID2D1HwndRenderTarget_GetSize(hwnd_rt);
    size2 = ID2D1BitmapRenderTarget_GetSize(rt);
    ok(!memcmp(&size, &size2, sizeof(size)), "Got target DIP size mismatch.\n");

    ID2D1HwndRenderTarget_GetDpi(hwnd_rt, dpi, dpi + 1);
    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);
    ok(!memcmp(dpi, dpi2, sizeof(dpi)), "Got dpi mismatch.\n");

    ID2D1BitmapRenderTarget_Release(rt);

    /* Pixel size specified. */
    set_size_u(&pixel_size, 32, 32);
    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, NULL, &pixel_size, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    pixel_size2 = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(!memcmp(&pixel_size, &pixel_size2, sizeof(pixel_size)), "Got target pixel size mismatch.\n");

    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);
    ok(!memcmp(dpi, dpi2, sizeof(dpi)), "Got dpi mismatch.\n");

    ID2D1BitmapRenderTarget_Release(rt);

    /* Both pixel size and DIP size are specified. */
    set_size_u(&pixel_size, 128, 128);
    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, &size, &pixel_size, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Doubled pixel size dimensions with the same DIP size give doubled dpi. */
    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);
    ok(dpi[0] == dpi2[0] / 2.0f && dpi[1] == dpi2[1] / 2.0f, "Got dpi mismatch.\n");

    ID2D1BitmapRenderTarget_Release(rt);

    /* DIP size is specified, fractional. */
    set_size_f(&size, 70.1f, 70.4f);
    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, &size, NULL, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);

    pixel_size = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == ceilf(size.width * dpi[0] / 96.0f)
            && pixel_size.height == ceilf(size.height * dpi[1] / 96.0f), "Wrong pixel size %ux%u\n",
            pixel_size.width, pixel_size.height);

    dpi[0] *= (pixel_size.width / size.width) * (96.0f / dpi[0]);
    dpi[1] *= (pixel_size.height / size.height) * (96.0f / dpi[1]);

    ok(compare_float(dpi[0], dpi2[0], 1) && compare_float(dpi[1], dpi2[1], 1), "Got dpi mismatch.\n");

    ID2D1HwndRenderTarget_Release(hwnd_rt);

    /* Check if GetBitmap() returns same instance. */
    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(bitmap == bitmap2, "Got different bitmap instances.\n");

    /* Draw something, see if bitmap instance is retained. */
    ID2D1BitmapRenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 1.0f, 0.0f, 1.0f);
    ID2D1BitmapRenderTarget_Clear(rt, &color);
    hr = ID2D1BitmapRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1Bitmap_Release(bitmap2);
    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(bitmap == bitmap2, "Got different bitmap instances.\n");

    ID2D1Bitmap_Release(bitmap);
    ID2D1Bitmap_Release(bitmap2);

    refcount = ID2D1BitmapRenderTarget_Release(rt);
    ok(!refcount, "Target should be released, got %lu.\n", refcount);

    DestroyWindow(hwnd_rt_desc.hwnd);

    /* Compatible target created from a DC target without associated HDC */
    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 192.0f;
    desc.dpiY = 96.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &dc_rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    test_compatible_target_size((ID2D1RenderTarget *)dc_rt);

    hr = ID2D1DCRenderTarget_CreateCompatibleRenderTarget(dc_rt, NULL, NULL, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    pixel_size = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == 0 && pixel_size.height == 0, "Got wrong size\n");

    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    pixel_size = ID2D1Bitmap_GetPixelSize(bitmap);
    ok(pixel_size.width == 0 && pixel_size.height == 0, "Got wrong size\n");
    ID2D1Bitmap_Release(bitmap);

    ID2D1BitmapRenderTarget_Release(rt);
    ID2D1DCRenderTarget_Release(dc_rt);

    release_test_context(&ctx);
}

static void test_desktop_dpi(BOOL d3d11)
{
    ID2D1Factory *factory;
    float dpi_x, dpi_y;
    HRESULT hr;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    dpi_x = dpi_y = 0.0f;
    ID2D1Factory_GetDesktopDpi(factory, &dpi_x, &dpi_y);
    ok(dpi_x > 0.0f && dpi_y > 0.0f, "Got wrong dpi %f x %f.\n", dpi_x, dpi_y);

    ID2D1Factory_Release(factory);
}

static void test_stroke_style(BOOL d3d11)
{
    static const struct
    {
        D2D1_DASH_STYLE dash_style;
        UINT32 dash_count;
        float dashes[6];
    }
    dash_style_tests[] =
    {
        {D2D1_DASH_STYLE_SOLID,        0},
        {D2D1_DASH_STYLE_DASH,         2, {2.0f, 2.0f}},
        {D2D1_DASH_STYLE_DOT,          2, {0.0f, 2.0f}},
        {D2D1_DASH_STYLE_DASH_DOT,     4, {2.0f, 2.0f, 0.0f, 2.0f}},
        {D2D1_DASH_STYLE_DASH_DOT_DOT, 6, {2.0f, 2.0f, 0.0f, 2.0f, 0.0f, 2.0f}},
    };
    D2D1_STROKE_STYLE_PROPERTIES desc;
    struct d2d1_test_context ctx;
    ID2D1StrokeStyle *style;
    UINT32 count;
    HRESULT hr;
    D2D1_CAP_STYLE cap_style;
    D2D1_LINE_JOIN line_join;
    float miter_limit, dash_offset;
    D2D1_DASH_STYLE dash_style;
    unsigned int i;
    float dashes[2];

    if (!init_test_context(&ctx, d3d11))
        return;

    desc.startCap = D2D1_CAP_STYLE_SQUARE;
    desc.endCap = D2D1_CAP_STYLE_ROUND;
    desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    desc.miterLimit = 1.5f;
    desc.dashStyle = D2D1_DASH_STYLE_DOT;
    desc.dashOffset = -1.0f;

    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, NULL, 0, &style);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    cap_style = ID2D1StrokeStyle_GetStartCap(style);
    ok(cap_style == D2D1_CAP_STYLE_SQUARE, "Unexpected cap style %d.\n", cap_style);
    cap_style = ID2D1StrokeStyle_GetEndCap(style);
    ok(cap_style == D2D1_CAP_STYLE_ROUND, "Unexpected cap style %d.\n", cap_style);
    cap_style = ID2D1StrokeStyle_GetDashCap(style);
    ok(cap_style == D2D1_CAP_STYLE_TRIANGLE, "Unexpected cap style %d.\n", cap_style);
    line_join = ID2D1StrokeStyle_GetLineJoin(style);
    ok(line_join == D2D1_LINE_JOIN_BEVEL, "Unexpected line joind %d.\n", line_join);
    miter_limit = ID2D1StrokeStyle_GetMiterLimit(style);
    ok(miter_limit == 1.5f, "Unexpected miter limit %f.\n", miter_limit);
    dash_style = ID2D1StrokeStyle_GetDashStyle(style);
    ok(dash_style == D2D1_DASH_STYLE_DOT, "Unexpected dash style %d.\n", dash_style);
    dash_offset = ID2D1StrokeStyle_GetDashOffset(style);
    ok(dash_offset == -1.0f, "Unexpected dash offset %f.\n", dash_offset);

    /* Custom dash pattern, no dashes data specified. */
    desc.startCap = D2D1_CAP_STYLE_SQUARE;
    desc.endCap = D2D1_CAP_STYLE_ROUND;
    desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    desc.miterLimit = 1.5f;
    desc.dashStyle = D2D1_DASH_STYLE_CUSTOM;
    desc.dashOffset = 0.0f;

    ID2D1StrokeStyle_Release(style);

    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, NULL, 0, &style);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, dashes, 0, &style);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, dashes, 1, &style);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1StrokeStyle_Release(style);

    /* Builtin style, dashes are specified. */
    desc.dashStyle = D2D1_DASH_STYLE_DOT;
    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, dashes, 1, &style);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Invalid style. */
    desc.dashStyle = 100;
    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, NULL, 0, &style);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Test returned dash pattern for builtin styles. */
    desc.startCap = D2D1_CAP_STYLE_SQUARE;
    desc.endCap = D2D1_CAP_STYLE_ROUND;
    desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    desc.miterLimit = 1.5f;
    desc.dashOffset = 0.0f;

    for (i = 0; i < ARRAY_SIZE(dash_style_tests); ++i)
    {
        float dashes[10];
        UINT dash_count;

        desc.dashStyle = dash_style_tests[i].dash_style;

        hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, NULL, 0, &style);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        dash_count = ID2D1StrokeStyle_GetDashesCount(style);
        ok(dash_count == dash_style_tests[i].dash_count, "%u: unexpected dash count %u, expected %u.\n",
                i, dash_count, dash_style_tests[i].dash_count);
        ok(dash_count < ARRAY_SIZE(dashes), "%u: unexpectedly large dash count %u.\n", i, dash_count);
        if (dash_count == dash_style_tests[i].dash_count)
        {
            unsigned int j;

            ID2D1StrokeStyle_GetDashes(style, dashes, dash_count);
            ok(!memcmp(dashes, dash_style_tests[i].dashes, sizeof(*dashes) * dash_count),
                    "%u: unexpected dash array.\n", i);

            /* Ask for more dashes than style actually has. */
            memset(dashes, 0xcc, sizeof(dashes));
            ID2D1StrokeStyle_GetDashes(style, dashes, ARRAY_SIZE(dashes));
            ok(!memcmp(dashes, dash_style_tests[i].dashes, sizeof(*dashes) * dash_count),
                    "%u: unexpected dash array.\n", i);

            for (j = dash_count; j < ARRAY_SIZE(dashes); ++j)
                ok(dashes[j] == 0.0f, "%u: unexpected dash value at %u.\n", i, j);
        }

        ID2D1StrokeStyle_Release(style);
    }

    /* NULL dashes array, non-zero length. */
    memset(&desc, 0, sizeof(desc));
    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &desc, NULL, 1, &style);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID2D1StrokeStyle_GetDashesCount(style);
    ok(count == 0, "Unexpected dashes count %u.\n", count);

    ID2D1StrokeStyle_Release(style);

    release_test_context(&ctx);
}

static void test_gradient(BOOL d3d11)
{
    ID2D1GradientStopCollection *gradient;
    D2D1_GRADIENT_STOP stops[3], stops2[3];
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    D2D1_COLOR_F color;
    unsigned int i;
    UINT32 count;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    stops2[0].position = 0.5f;
    set_color(&stops2[0].color, 1.0f, 1.0f, 0.0f, 1.0f);
    stops2[1] = stops2[0];
    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops2, 2, D2D1_GAMMA_2_2,
            D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID2D1GradientStopCollection_GetGradientStopCount(gradient);
    ok(count == 2, "Unexpected stop count %u.\n", count);

    /* Request more stops than collection has. */
    stops[0].position = 123.4f;
    set_color(&stops[0].color, 1.0f, 0.5f, 0.4f, 1.0f);
    color = stops[0].color;
    stops[2] = stops[1] = stops[0];
    ID2D1GradientStopCollection_GetGradientStops(gradient, stops, ARRAY_SIZE(stops));
    ok(!memcmp(stops, stops2, sizeof(*stops) * count), "Unexpected gradient stops array.\n");
    for (i = count; i < ARRAY_SIZE(stops); ++i)
    {
        ok(stops[i].position == 123.4f, "%u: unexpected stop position %f.\n", i, stops[i].position);
        ok(!memcmp(&stops[i].color, &color, sizeof(color)), "%u: unexpected stop color.\n", i);
    }

    ID2D1GradientStopCollection_Release(gradient);

    release_test_context(&ctx);
}

static void test_draw_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry[4];
    ID2D1RectangleGeometry *rect_geometry[2];
    D2D1_POINT_2F point = {0.0f, 0.0f};
    D2D1_ROUNDED_RECT rounded_rect;
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1GeometrySink *sink;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_POINT_2F p0, p1;
    D2D1_ELLIPSE ellipse;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    factory = ctx.factory;

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_point(&p0, 40.0f, 160.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p0, (ID2D1Brush *)brush, 10.0f, NULL);
    set_point(&p0, 100.0f, 160.0f);
    set_point(&p1, 140.0f, 160.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, (ID2D1Brush *)brush, 10.0f, NULL);
    set_point(&p0, 200.0f,  80.0f);
    set_point(&p1, 200.0f, 240.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, (ID2D1Brush *)brush, 10.0f, NULL);
    set_point(&p0, 260.0f, 240.0f);
    set_point(&p1, 300.0f,  80.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rect(&rect, 40.0f, 480.0f, 40.0f, 480.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rect(&rect, 100.0f, 480.0f, 140.0f, 480.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rect(&rect, 200.0f, 400.0f, 200.0f, 560.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rect(&rect, 260.0f, 560.0f, 300.0f, 400.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_ellipse(&ellipse,  40.0f, 800.0f,  0.0f,  0.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);
    set_ellipse(&ellipse, 120.0f, 800.0f, 20.0f,  0.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);
    set_ellipse(&ellipse, 200.0f, 800.0f,  0.0f, 80.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);
    set_ellipse(&ellipse, 280.0f, 800.0f, 20.0f, 80.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "yGBQUFBQUFBQUFDoYQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "xjIUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUxjIA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 2,
            "zjECnQETjAEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVjAETnQECzjEA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "5mAUjAEUjAEUjAEUjAEUhmIA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "vmBkPGQ8ZDxkPGTeYQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0,
            "5i4UjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUhjAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 10,
            "hDAYgwEieyh1LnAybBcIF2gWDhZkFhIWYRUWFV4VGhVbFRwVWRUeFVcVIBVVFCQUUxQmFFEUKBRP"
            "FSgVTRUqFUwULBRLFC4USRQwFEgUMBRHFDIURhQyFEUUNBREFDQUQxQ2FEIUNhRBFDgUQBQ4FEAU"
            "OBQ/FDoUPhQ6FD4UOhQ+FDoUPhQ6FD0UPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FD0UOhQ+FDoUPhQ6FD4UOhQ+FDoUPxQ4FEAUOBRAFDgUQRQ2FEIUNhRDFDQU"
            "RBQ0FEUUMhRGFDIURxQwFEgUMBRJFC4USxQsFEwVKhVNFSgVTxQoFFEUJhRTFCQUVRUgFVcVHhVZ"
            "FRwVWxUaFV4VFhVhFhIWZBYOFmgXCBdsMnAudSh7IoMBGIQw");
    todo_wine ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "3C4oaUZVUExYRlxCHCgcPxU4FT0UPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FD0VOBU/YEJcRlhMUFVG7S8A");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 8,
            "3C4obT5dSFRQTlRKGCgYRhYwFkMVNBVBFTYVPxU5FD4UOhQ9FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPRQ6FD4UOhQ/FTYVQRU0FUMWMBZGWEpVTVBTSltA8C8A");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "3C4oZU5NWERgP2I9HigePBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZD1iP2BEWE1O6S8A");
    todo_wine ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 16,
            "hDAYgwEieyh1LnAybBcIF2gWDhZkFhIWYRUWFV4WGRVbFRwVWRUeFVcVIBVVFSMUUxQmFFEVJxRP"
            "FSgVTRUqFUwULBRLFC4USRUvFEgUMBRHFDIURhQyFEUUNBREFDQUQxQ2FEIUNhRBFDgUQBQ4FEAU"
            "OBQ/FTkUPhQ6FD4UOhQ+FDoUPhQ6FD0UPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FD0UOhQ+FDoUPhQ6FD4UOhQ+FDoUPxQ4FEAUOBRAFDgUQRQ2FEIUNhRDFDQU"
            "RBQ0FEUUMhRGFDIURxQwFEgUMBRJFC4USxQsFEwVKhVNFSgVTxQoFFEUJhRTFCQUVRUgFVcVHhVZ"
            "FRwVWxUaFV4VFhVhFhIWZBYOFmgWChZsMnAudCp6IoMBGIQw");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 16,
            "3C4obzpjQF5EWkhXFSAVVRQkFFMUJhRRFCgUTxQqFE0VKhVMFCwUSxQuFEoULhVIFDAUSBQwFUYU"
            "MhRGFDIURRQ0FEQUNBRDFTQVQhQ2FEIUNhRCFDYUQRQ4FEAUOBRAFDgUQBQ4FD8UOhQ+FDoUPhQ6"
            "FD4UOhQ+FDoUPhQ6FD0VOxQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBU7FD0UOhQ+FDoUPhQ6FD4UOhQ+FDoUPhQ6FD8UOBRA"
            "FDgUQBQ4FEAUOBRBFDYUQhQ2FEIUNhRCFTQVQxQ0FEQUNBRFFDIURhQyFEYVMBVHFDAUSBUuFUkU"
            "LhRLFCwUTBUrFE0UKhRPFCgUURQmFFMUJBRVSldIWUZdQWI78i8A");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 8,
            "9i80ZERWUExYRV5AHCocPRY4FjwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FToVPRssG0BeRFpLUFVGYzT2LwAA");
    todo_wine ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 40.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 200.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 60.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 560.0f);
    line_to(sink, 120.0f, 400.0f);
    line_to(sink, 120.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 220.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 560.0f);
    line_to(sink, 280.0f, 400.0f);
    line_to(sink, 280.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 40.0f, 720.0f);
    line_to(sink, 60.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 880.0f);
    line_to(sink, 140.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 200.0f, 720.0f);
    line_to(sink, 220.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 880.0f);
    line_to(sink, 300.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_ALTERNATE);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, 5.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0, "q2MKlgEKq2MA");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "iGNQUFCIYwAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0,
            "qyIKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKQQpLCkEKSwqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQrLIwAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "4GLAAuBi");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0,
            "qyIKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKSwpBCksKQQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQrLIwAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0,
            "rycCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEKAgqKAQoCCokBCgQKiAEKBAqHAQoGCoYBCgYKhQEKCAqEAQoICoMBCgoKggEKCgqBAQoM"
            "CoABCgwKfwoOCn4KDgp9ChAKfAoQCnsKEgp6ChIKeQoUCngKFAp3ChYKdgoWCnUKGAp0ChgKcwoa"
            "CnIKGgpxChwKcAocCm8KHgpuCh4KbQogCmwKIAprCiIKagoiCmkKJApoCiQKZwomCmYKJgplCigK"
            "ZAooCmMKKgpiCioKYQosCmAKLApfCi4KXgouCl0KMApcCjAKWwoyCloKMgpZCjQKWAo0ClcKNgpW"
            "CjYKVQo4ClQKOApTCjoKUgo6ClEKPApQCjwKTwo+Ck4KPgpNCkAKTApACksKQgpKCkIKSQpECkgK"
            "RApHCkYKozIA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0,
            "ozIKRgpHCkQKSApECkkKQgpKCkIKSwpACkwKQApNCj4KTgo+Ck8KPApQCjwKUQo6ClIKOgpTCjgK"
            "VAo4ClUKNgpWCjYKVwo0ClgKNApZCjIKWgoyClsKMApcCjAKXQouCl4KLgpfCiwKYAosCmEKKgpi"
            "CioKYwooCmQKKAplCiYKZgomCmcKJApoCiQKaQoiCmoKIgprCiAKbAogCm0KHgpuCh4KbwocCnAK"
            "HApxChoKcgoaCnMKGAp0ChgKdQoWCnYKFgp3ChQKeAoUCnkKEgp6ChIKewoQCnwKEAp9Cg4KfgoO"
            "Cn8KDAqAAQoMCoEBCgoKggEKCgqDAQoICoQBCggKhQEKBgqGAQoGCocBCgQKiAEKBAqJAQoCCooB"
            "CgIKiwEUjAEUjQESjgESjwEQkAEQkQEOkgEOkwEMlAEMlQEKlgEKlwEImAEImQEGmgEGmwEEnAEE"
            "nQECngECrycA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "rycCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEKAgqKAQoCCokBCgQKiAEKBAqHAQoGCoYBCgYKhQEKCAqEAQoICoMBCgoKggEKCgqBAQoM"
            "CoABCgwKfwoOCn4KDgp9ChAKfAoQCnsKEgp6ChIKeQoUCngKFAp3ChYKdgoWCnUKGAp0ChgKcwoa"
            "CnIKGgpxChwKcAocCm8KHgpuCh4KbQogCmwKIAprCiIKagoiCmkKJApoCiQKZwomCmYKJgplCigK"
            "ZAooCmMKKgpiCioKYQosCmAKLApfCi4KXgouCl0KMApcCjAKWwoyCloKMgpZCjQKWAo0ClcKNgpW"
            "CjYKVQo4ClQKOApTCjoKUgo6ClEKPApQCjwKTwo+Ck4KPgpNCkAKTApACksKQgpKCkIKSQpECkgK"
            "RApHWkZagzEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "gzFaRlpHCkQKSApECkkKQgpKCkIKSwpACkwKQApNCj4KTgo+Ck8KPApQCjwKUQo6ClIKOgpTCjgK"
            "VAo4ClUKNgpWCjYKVwo0ClgKNApZCjIKWgoyClsKMApcCjAKXQouCl4KLgpfCiwKYAosCmEKKgpi"
            "CioKYwooCmQKKAplCiYKZgomCmcKJApoCiQKaQoiCmoKIgprCiAKbAogCm0KHgpuCh4KbwocCnAK"
            "HApxChoKcgoaCnMKGAp0ChgKdQoWCnYKFgp3ChQKeAoUCnkKEgp6ChIKewoQCnwKEAp9Cg4KfgoO"
            "Cn8KDAqAAQoMCoEBCgoKggEKCgqDAQoICoQBCggKhQEKBgqGAQoGCocBCgQKiAEKBAqJAQoCCooB"
            "CgIKiwEUjAEUjQESjgESjwEQkAEQkQEOkgEOkwEMlAEMlQEKlgEKlwEImAEImQEGmgEGmwEEnAEE"
            "nQECngECrycA");
    ok(match, "Figure does not match.\n");

    set_rect(&rect, 20.0f, 80.0f, 60.0f, 240.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)rect_geometry[1], &matrix, &transformed_geometry[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[0], &matrix, &transformed_geometry[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)rect_geometry[0], (ID2D1Brush *)brush, 10.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, 10.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, 5.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, 15.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);
    ID2D1RectangleGeometry_Release(rect_geometry[1]);
    ID2D1RectangleGeometry_Release(rect_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 32,
            "8XYGtQIOrAIXpAIfmwIokwIwigI4gwJA+gFJ8gFR6QEzAiXhATMKJdgBMxMl0AEzGyXHATMkJb8B"
            "MysmtgEzNCWvATM8JaYBM0UlngEzTSWVATNWJY0BM14lhAEzZyV8M28lczN4JWszgAElYjOIASZa"
            "M5ABJVgtmQElWCWhASVYJaEBJVgloQElWCWhASVYJaEBJVgloQElWCWhASVYJaEBJVglmQEtWCWQ"
            "ATNaJogBM2IlgAEzayV4M3MlbzN8JWczhAElXjONASVWM5UBJU0zngElRTOmASU8M68BJTQztgEm"
            "KzO/ASUkM8cBJRsz0AElEzPYASUKM+EBJQIz6QFR8gFJ+gFAgwI4igIwkwIomwIfpAIXrAIOtQIG"
            "8XYA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 32,
            "ujEBngECnQEDnQEEmwEFmgEHmQEHmAEIlwEKlgEKlQELlAENkwENkgEOkQEQjwERjwESjQETjAEU"
            "jAEKAQqKAQoCCokBCgMKiQEKBAqHAQoFCoYBCgYKhgEKBwqEAQoICoMBCgkKgwEKCgqBAQoLCoAB"
            "Cg0KfgsNCn4KDgp9ChAKewsQCnsKEQp6ChMKeAoUCngKFAp3ChYKdQoXCnUKGApzChkKcgoaCnIK"
            "GwpwChwKbwodCm4LHgptCh8KbAogCmsLIQpqCiIKaQokCmcKJQpnCiUKZgonCmQKKApkCigKYwoq"
            "CmEKKwphCisKYAotCl4KLgpdCy8KXAowClsKMQpaCzIKWQozClgKNApXCjYKVgo2ClUKNwpUCjkK"
            "Uwo5ClIKOwpQCjwKTws8Ck8KPgpNCj8KTAs/CkwKQQpKCkIKSQtCCkkKRApHCkUKRgpHCkUKRwpE"
            "CkgKQwpKCkIKSgpBCksKQApNCj4LTQo+Ck4KPQpQCjsLUAo7ClIKOQpTCjgLUwo4ClUKNgpWCjUK"
            "Vwo1ClgKMwpZCjQKWAo0ClkKMwpZCjQKWQozClkKMwpZCjQKWQozClkKMwpZCjQKWQozClkKNApY"
            "CjQKWQozClkKNApZCjMKWQozClkKNApZCjMKWQozClkKNApZCjMKWQo0ClgKNApZCjMKWQo0ClkK"
            "MwpZCjMKWQo0ClkKMwpZCjMKWQo0ClkKMwpZCjQKWAo0ClkKMwpYCjUKVwo1ClYKNgpVCjgKUws4"
            "ClMKOQpSCjsKUAs7ClAKPQpOCj4KTQs+Ck0KQApLCkEKSgpCCkoKQwpICkQKRwpFCkcKRgpFCkcK"
            "RApJCkILSQpCCkoKQQpMCj8LTAo/Ck0KPgpPCjwLTwo8ClAKOwpSCjkKUwo5ClQKNwpVCjYKVgo2"
            "ClcKNApYCjMKWQoyC1oKMQpbCjAKXAovC10KLgpeCi0KYAorCmEKKwphCioKYwooCmQKKApkCicK"
            "ZgolCmcKJQpnCiQKaQoiCmoKIQtrCiAKbAofCm0KHgtuCh0KbwocCnAKGwpyChoKcgoZCnMKGAp1"
            "ChcKdQoWCncKFAp4ChQKeAoTCnoKEQp7ChALewoQCn0KDgp+Cg0LfgoNCoABCgsKgQEKCgqDAQoJ"
            "CoMBCggKhAEKBwqGAQoGCoYBCgUKhwEKBAqJAQoDCokBCgIKigEKAQqMARSMARONARKPARGPARCR"
            "AQ6SAQ2TAQ2UAQuVAQqWAQqXAQiYAQeZAQeaAQWbAQSdAQOdAQKeAQG6MQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 64,
            "82ICvQIEugIHuAIJtgIKtAINsgIPsAIRrQITrAIVqQIYpwIZpgIbowIeoQIgnwIhnQIkmwImmAIp"
            "lgIVARSVAhUDFJICFQUVkAIVBxSPAhUJFIwCFQwUigIVDRWHAhYPFIYCFRIUhAIVFBSBAhUWFf8B"
            "FRgU/gEVGhT7ARUcFfkBFR4U9wEWIBT1ARUjFPMBFSQV8AEVJxTvARUpFOwBFisU6gEVLRXoARUv"
            "FOYBFjEU5AEVMxXiARU1FOABFTgU3gEVOhTbARY7FdkBFT4U2AEVQBTVARZCFNMBFUQV0QEVRhTP"
            "ARVJFM0BFUoVygEWTBTJARVPFMcBFVEUxAEVUxXCARVVFMEBFVcUvgEVWRW8ARVbFbkBFl0UuAEV"
            "YBS2ARVhFbMBFWQUsgEVZhSwARVoFK0BFWoVqwEVbBSpARZuFKcBFXAVpQEVchWiARV1FKEBFXcU"
            "nwEVeBWcARV7FJsBFX0UmAEWfxSWARWBARWUARWDARSSARWGARSQARWHARWOARWJARWLARWMARSK"
            "ARWOARSHARaPARWFARWSARSEARWUARSBARWXARR/FZgBFX0VmgEUexWdARR5FZ4BFXYWoAEVdBWj"
            "ARRzFaUBFHAVpwEVbhWpARRtFasBFGoVrgEUaBWvARVmFbEBFGcUsgEUZxSxARVmFbEBFWYUsgEU"
            "ZxSyARRnFLEBFWYVsQEUZxSyARRnFLIBFGcUsQEVZhWxARRnFLIBFGcUsQEVZhWxARVmFLIBFGcU"
            "sgEUZxSxARVmFbEBFGcUsgEUZxSyARRmFbEBFWYVsQEUZxSyARRnFLEBFWYVsQEUZxSyARRnFLIB"
            "FGcUsQEVZhWxARRnFLIBFGcUsgEUZhWxARVmFbEBFGcUsgEUZxSxARVmFa8BFWgUrgEVahSrARVt"
            "FKkBFW4VpwEVcBSlARVzFKMBFXQVoAEWdhWeARV5FJ0BFXsUmgEVfRWYARV/FJcBFYEBFJQBFYQB"
            "FJIBFYUBFY8BFocBFI4BFYoBFIwBFYsBFYkBFY4BFYcBFZABFIYBFZIBFIMBFZQBFYEBFZYBFH8W"
            "mAEUfRWbARR7FZwBFXgVnwEUdxWhARR1FaIBFXIVpQEVcBWnARRuFqkBFGwVqwEVahWtARRoFbAB"
            "FGYVsgEUZBWzARVhFbYBFGAVuAEUXRa5ARVbFbwBFVkVvgEUVxXBARRVFcIBFVMVxAEUURXHARRP"
            "FckBFEwWygEVShXNARRJFc8BFEYV0QEVRBXTARRCFtUBFEAV2AEUPhXZARU7FtsBFDoV3gEUOBXg"
            "ARQ1FeIBFTMV5AEUMRbmARQvFegBFS0V6gEUKxbsARQpFe8BFCcV8AEVJBXzARQjFfUBFCAW9wEU"
            "HhX5ARUcFfsBFBoV/gEUGBX/ARUWFYECFBQVhAIUEhWGAhQPFocCFQ0VigIUDBWMAhQJFY8CFAcV"
            "kAIVBRWSAhQDFZUCFAEVlgIpmAImmwIknQIhnwIgoQIeowIbpgIZpwIYqQIVrAITrQIRsAIPsgIN"
            "tAIKtgIJuAIHugIEvQIC82IA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 20.0f, 79.5f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 40.0f,  79.5f, 60.0f,  79.5f);
    quadratic_to(sink, 60.0f, 159.5f, 60.0f, 239.5f);
    quadratic_to(sink, 40.0f, 239.5f, 20.0f, 239.5f);
    quadratic_to(sink, 20.0f, 159.5f, 20.0f,  79.5f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 79.5f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 105.0f,  79.5f, 140.0f,  79.5f);
    quadratic_to(sink, 140.0f,  99.5f, 140.0f, 239.5f);
    quadratic_to(sink, 135.0f, 239.5f, 100.0f, 239.5f);
    quadratic_to(sink, 100.0f, 219.5f, 100.0f,  79.5f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 79.5f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 215.0f,  79.5f, 220.0f,  79.5f);
    quadratic_to(sink, 220.0f, 219.5f, 220.0f, 239.5f);
    quadratic_to(sink, 185.0f, 239.5f, 180.0f, 239.5f);
    quadratic_to(sink, 180.0f,  99.5f, 180.0f,  79.5f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 79.5f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 280.0f,  79.5f, 300.0f,  79.5f);
    quadratic_to(sink, 300.0f, 159.5f, 300.0f, 239.5f);
    quadratic_to(sink, 280.0f, 239.5f, 260.0f, 239.5f);
    quadratic_to(sink, 260.0f, 159.5f, 260.0f,  79.5f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 40.0f, 420.0f, 60.0f, 400.0f);
    quadratic_to(sink, 55.0f, 480.0f, 60.0f, 560.0f);
    quadratic_to(sink, 40.0f, 540.0f, 20.0f, 560.0f);
    quadratic_to(sink, 25.0f, 480.0f, 20.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 105.0f, 420.0f, 140.0f, 400.0f);
    quadratic_to(sink, 135.0f, 420.0f, 140.0f, 560.0f);
    quadratic_to(sink, 135.0f, 540.0f, 100.0f, 560.0f);
    quadratic_to(sink, 105.0f, 540.0f, 100.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 215.0f, 420.0f, 220.0f, 400.0f);
    quadratic_to(sink, 215.0f, 540.0f, 220.0f, 560.0f);
    quadratic_to(sink, 185.0f, 540.0f, 180.0f, 560.0f);
    quadratic_to(sink, 185.0f, 420.0f, 180.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 280.0f, 420.0f, 300.0f, 400.0f);
    quadratic_to(sink, 295.0f, 480.0f, 300.0f, 560.0f);
    quadratic_to(sink, 280.0f, 540.0f, 260.0f, 560.0f);
    quadratic_to(sink, 265.0f, 480.0f, 260.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 40.0f, 700.0f, 60.0f, 720.0f);
    quadratic_to(sink, 65.0f, 800.0f, 60.0f, 880.0f);
    quadratic_to(sink, 40.0f, 900.0f, 20.0f, 880.0f);
    quadratic_to(sink, 15.0f, 800.0f, 20.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 105.0f, 700.0f, 140.0f, 720.0f);
    quadratic_to(sink, 145.0f, 740.0f, 140.0f, 880.0f);
    quadratic_to(sink, 135.0f, 900.0f, 100.0f, 880.0f);
    quadratic_to(sink,  95.0f, 860.0f, 100.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 215.0f, 700.0f, 220.0f, 720.0f);
    quadratic_to(sink, 225.0f, 860.0f, 220.0f, 880.0f);
    quadratic_to(sink, 185.0f, 900.0f, 180.0f, 880.0f);
    quadratic_to(sink, 175.0f, 740.0f, 180.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 280.0f, 700.0f, 300.0f, 720.0f);
    quadratic_to(sink, 305.0f, 800.0f, 300.0f, 880.0f);
    quadratic_to(sink, 280.0f, 900.0f, 260.0f, 880.0f);
    quadratic_to(sink, 255.0f, 800.0f, 260.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, 10.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "yC5aRlpGWjxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 64,
            "3SoDYAM6B1gHOgtQCzoPSA87EkASPBc2FzwcLBw8IiAiPWI+Yj5iPhQBOAEUPhQKJgoUPxQ4FEAU"
            "OBRAFDgUQBQ4FEAUOBRBFDYUQhQ2FEIUNhRCFDYUQhQ2FEIUNhRDFDQURBQ0FEQUNBREFDQURBQ0"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURRQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIU"
            "RhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRG"
            "FDIURRQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEMUNhRCFDYUQhQ2FEIU"
            "NhRCFDYUQhQ2FEEUOBRAFDgUQBQ4FEAUOBRAFDgUPxQKJgoUPhQBOAEUPmI+Yj5iPSIgIjwcLBw8"
            "FzYXPBJAEjsPSA86C1ALOgdYBzoDYAPdKgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 1024,
            "uxUBnwECngEDnQEEnAEFmwEGmwEGmgEHmQEImAEJlwEKlgELlQEMlQEMlAENkwEOkgEPkQEQkAER"
            "VQQ2Ek0KOBJFEDkTPRY6FDUcOxUrJDwYHi09Yj5iP2BAQwkUQDgUFEAUOBRAFDcUQRQ3FEEUNxRC"
            "FDYUQhQ2FEIUNhRCFDUUQxQ1FEMUNRRDFDUUQxQ1FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQU"
            "NBREFDQURBQ0FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQ0"
            "FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQ0FEQUNBREFDQU"
            "RBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNRRDFDUUQxQ1FEMUNRRDFDUUQhQ2FEIUNhRC"
            "FDYUQhQ3FEEUNxRBFDcUQBQ4FEAUFDhAFAlDQGA/Yj5iPS0eGDwkKxU7HDUUOhY9EzkQRRI4Ck0S"
            "NgRVEZABEJEBD5IBDpMBDZQBDJUBDJUBC5YBCpcBCZgBCJkBB5oBBpsBBpsBBZwBBJ0BA54BAp8B"
            "AbsV");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 1024,
            "pBYBngECnQEDnAEEmwEFmgEGmQEGmQEHmAEIlwEJlgEKlQELlAEMkwEMkwENkgEOkQEPkAEQNgRV"
            "ETcKTRI4EEUSOhY9EzscNRQ8JCsVPS0eGD5iPmI/YEAUCUNAFBQ4QBQ4FEEUNxRBFDcUQRQ3FEEU"
            "NhRCFDYUQhQ2FEMUNRRDFDUUQxQ1FEMUNRRDFDUUQxQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0"
            "FEQUNBREFDQURBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxREFDQU"
            "RBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQUQxQ1FEMUNRRDFDUUQxQ1FEMUNRRDFDYUQhQ2FEIU"
            "NhRBFDcUQRQ3FEEUNxRBFDgUQDgUFEBDCRRAYD9iPmI+GB4tPRUrJDwUNRw7Ez0WOhJFEDgSTQo3"
            "EVUENhCQAQ+RAQ6SAQ2TAQyTAQyUAQuVAQqWAQmXAQiYAQeZAQaZAQaaAQWbAQScAQOdAQKeAQGk"
            "FgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 64,
            "wCsDmQEHlQELkQEPSwJAEkgLNhc8HCwcPCIgIj1iPmI+Yj4UATgBFD4UCiYKFD8UOBRAFDgUQBQ4"
            "FEAUOBRAFDgUQRQ2FEIUNhRCFDYUQhQ2FEIUNhRCFDYUQxQ0FEQUNBREFDQURBQ0FEQUNBREFDQU"
            "RBQ0FEQUNBREFDQURBQ0FEUUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRG"
            "FDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEUU"
            "NBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBRDFDYUQhQ2FEIUNhRCFDYUQhQ2"
            "FEIUNhRBFDgUQBQ4FEAUOBRAFDgUQBQ4FD8UCiYKFD4UATgBFD5iPmI+Yj0iICI8HCwcPBc2FzwS"
            "QBI7D0gPOgtQCzoHWAc6A2AD3SoA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 64,
            "3SkmcThiRFdOTVhEICAgPhwsHDwXNhc8FDwUOxQ+FDoUPhQ6FD4UOhQ+FDoUPhQ5FEAUOBRAFDgU"
            "QBQ4FEAUOBRAFDcUQhQ2FEIUNhRCFDYUQhQ2FEIUNhRCFDUURBQ0FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQzFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYU"
            "MhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQz"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNRRCFDYUQhQ2FEIUNhRCFDYU"
            "QhQ2FEIUNxRAFDgUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDoUPhQ6FD4UOhQ+FDsUPBQ8FzYXPBws"
            "HD4gICBEWE1OV0RiOHEm3SkA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 1024,
            "zykoczhkRVhQTlpEFx4tPRUrJDwUNRw7FDwVOxQ+FDoUPhQ5FEAUOBRAFDgUQBQ4FEAUOBRBFDcU"
            "QRQ3FEEUNhRCFDYUQhQ2FEIUNhRDFDUUQxQ1FEMUNRRDFDUUQxQ0FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUU"
            "MxRFFDMURBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURBQ0"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEMUNRRDFDUUQxQ1FEMUNRRDFDYU"
            "QhQ2FEIUNhRCFDYUQRQ3FEEUNxRBFDgUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDsVPBQ7HDUUPCQr"
            "FT0tHhdEWk5QWEVkOHMozykA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 1024,
            "6SkobThfRVNQSFpALR4XPSQrFTscNRQ7FTwUOhQ+FDoUPhQ5FEAUOBRAFDgUQBQ4FEAUNxRBFDcU"
            "QRQ3FEEUNxRCFDYUQhQ2FEIUNRRDFDUUQxQ1FEMUNRRDFDUUQxQ1FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQ0FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUU"
            "MxRFFDQURBQ0FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDQURBQ0"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ1FEMUNRRDFDUUQxQ1FEMUNRRDFDUU"
            "QhQ2FEIUNhRCFDcUQRQ3FEEUNxRBFDcUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDoUPBU7FDUcOxUr"
            "JD0XHi1AWkhQU0VfOG0o6SkA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 64,
            "3SkmcThiRFdOTVhGHiAgRhQsHDwXNhc8FDwUOxQ+FDoUPhQ6FD4UOhQ+FDoUPhQ5FEAUOBRAFDgU"
            "QBQ4FEAUOBRAFDcUQhQ2FEIUNhRCFDYUQhQ2FEIUNhRCFDUURBQ0FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQzFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYU"
            "MhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQz"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNRRCFDYUQhQ2FEIUNhRCFDYU"
            "QhQ2FEIUNxRAFDgUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDoUPhQ6FD4UOhQ+FDsUPBQ8FzYXPBws"
            "HD4gICBEWE1OV0RiOHEm3SkA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, -0.402914f, 0.915514f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, -0.310379f,  0.882571f, -0.116057f,  0.824000f);
    quadratic_to(sink,  0.008350f,  0.693614f, -0.052343f,  0.448886f);
    quadratic_to(sink, -0.154236f,  0.246072f, -0.279229f,  0.025343f);
    quadratic_to(sink, -0.370064f, -0.588586f, -0.383029f, -0.924114f);
    quadratic_to(sink, -0.295479f, -0.958764f, -0.017086f, -0.988400f);
    quadratic_to(sink,  0.208836f, -0.954157f,  0.272200f, -0.924114f);
    quadratic_to(sink,  0.295614f, -0.569071f,  0.230143f,  0.022886f);
    quadratic_to(sink,  0.101664f,  0.220643f,  0.012057f,  0.451571f);
    quadratic_to(sink, -0.028764f,  0.709014f,  0.104029f,  0.833943f);
    quadratic_to(sink,  0.319414f,  0.913057f,  0.403229f,  0.942628f);
    quadratic_to(sink,  0.317721f,  1.023450f, -0.017086f,  1.021771f);
    quadratic_to(sink, -0.310843f,  1.007472f, -0.402914f,  0.915514f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 160.0f);
    scale_matrix(&matrix, 20.0f, 80.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[2], &matrix, &transformed_geometry[3]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, 2.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, 10.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, 5.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[3], (ID2D1Brush *)brush, 15.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[3]);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 128,
            "yjIJkQEHBwaIAQUSBYMBBBYEggEEFgSCAQQWBIIBBBYEggEEFgSCAQQWBIIBBBYEggEEFgSCAQQW"
            "BIIBBBYEggEEFgSDAQQVBIMBBBUEgwEEFQSDAQQVBIMBBBUEgwEEFQSDAQQVBIMBBBUEgwEEFQSD"
            "AQQVBIQBBBQEhAEEFASEAQQTBIUBBBMEhQEEEwSFAQQTBIUBBBMEhQEEEwSGAQQSBIYBBBIEhgEE"
            "EgSGAQQSBIYBBBIEhgEEEgSGAQQRBIgBBBAEiAEEEASIAQQQBIkBBA4EigEEDgSLAQQMBIwBBAwE"
            "jQEECgSOAQQJBJABBAgEkAEFBgSSAQQGBJMBBAQElAEEBASVAQQDBJUBBAIElwEEAQSXAQiZAQeZ"
            "AQaaAQaaAQaaAQabAQWbAQWbAQWbAQWaAQeZAQeZAQeZAQiXAQQBBJYBBAMElQEEAwWRAQUGBY0B"
            "BQwFhwEFEgSCAQUXBYABBBoFfgUYBIIBBhEFiAEUpTEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 512,
            "yJIBArkCDa4CGKMCIZoCK5ECM4gCO4ECQ/gBS/EBUesBLAYl5QEsDiPeASwWIdkBLBwh0wEsISHO"
            "ASsgKMsBKR4vyAEnHDPIASUaNMsBIxg1mQEFMCIUN54BCygiDzijAREhIgY9qAEYGWGuAR4RXbMB"
            "JAhbuQGAAcABesYBc84Ba9YBTvQBP4MCOIoCNI4CM5ACMZICL5QCLZYCK5kCKJsCJ54CI6MCHq8C"
            "EraSAQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 512,
            "xWkCmwEFmAEJlQELlAENkgEOkQEPjwESjQETjAEVigELAQqJAQsCCogBCwQKhwEKBQqGAQoGCoYB"
            "CgcKhAEKCAqEAQoIC4IBCgoKggEKCgqBAQoMCoABCgwKfwoNCn8KDgp9Cg8KfQoPCnwKEQp7ChEK"
            "egoSCnoKEwp4ChQKeAoUCncLFQp2ChYKdgoWCnYKFwp2ChYKdgoWCncKFgp2ChYKdgoWCncKFQt2"
            "ChYKdwoVCncKFQp4ChUKdwoVCncKFQp4ChUKdwoVCngKFAp4ChUKeAoUCngKFAp4CxMKeQoUCngK"
            "FAp5ChMKeQoUCnkKEwp5ChMKegoSC3kKEwp6ChIKegoSCnoLEgp6ChIKegoSCnsKEQp7ChEKfAoQ"
            "CnwKEAp9Cg8KfQoPCn4KDgp+Cg4KfwoOCn4KDgp/Cg0KfwoNCoABCgwKgAEKDAqBAQoLCoEBCgsK"
            "gQELCgqCAQoKCoIBCwkKgwEKCQqDAQoJCoQBCggKhAEKCQqEAQsHCoUBCwYKhgELBQqHAQsECogB"
            "CwMKiQELAgqLAQoBCowBFI0BE44BE44BEo8BEZABEJEBD5IBDpMBDpMBDZMBDZQBDJQBDZQBDJQB"
            "DBUCfgwSBH4MEQV/DA4GgAEMDAiAAQ0KCYEBDAgLgQENBQ2BAQ0EDoIBDQEPgwEdgwEdgwEdgwEc"
            "hAEKAgQCCoUBCgYKhgEKBgqGAQoFC4YBCgUKhwEKBAqIAQoECogBCgMKiQEKAwqIAQoDCokBCgMK"
            "iQEKAgqJAQoCCooBCgIKiQEKAgqKAQoBCosBCgEKigEKAQqLARSMARSLARSMAROMARONARKOARGO"
            "ARGPARCQAQ6RAQ2YAQTEZAAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 1024,
            "ytABA7gCCbICD60CFKkCF6cCGqMCHqACIZ0CJJoCJpgCKZUCFgIUkgIWBBWPAhYHFI4CFQoUjAIV"
            "DBSKAhUNFYgCFQ8UhwIVERSFAhUTFIMCFRQVgQIUFxSAAhQZFP4BFBoV/AEUHBT7ARQeFPkBFB8V"
            "9wEUIRT2ARQjFPQBFSMV8gEVJRTxARUnFPABFCgV7gEUKhTtARQsFOwBFCwV7AEULBTsARUsFOwB"
            "FSsV7AEULBTtARQsFO0BFCsU7QEVKxTtARUqFe0BFSoU7gEUKxTuARQqFe4BFCoU7wEUKhTuARUp"
            "FO8BFSkU7wEVKBXvARUoFPABFCkU8AEUKBTxARQoFPEBFCcV8QEUJxTxARUnFPEBFSYU8gEVJhTy"
            "ARUlFfIBFSUU8wEUJRXzARQlFPQBFCUU9AEUJBT1ARQkFPUBFCMU9gEUIhT2ARUhFPcBFSAU+AEV"
            "HxT5ARUeFPoBFR4U+gEVHRT7ARUcFPwBFRsU/QEVGhT+ARUZFP8BFBkUgAIUGBSBAhQXFIICFBcU"
            "ggIUFhSDAhQVFIQCFBQUhQIUExSGAhQSFIcCFBIUhwIUERSIAhUPFIkCFg0UigIXCxSNAhYJFI8C"
            "FggUkAIXBRSSAhcDFJQCFwEUlgIrlwIpmgImnAIkngIjnwIhoQIfowIepAIcpgIbpgIaqAIZqAIZ"
            "qAIYKwP7ARgnBf0BGCMI/QEZHgz+ARgbD/8BGBcSgAIYEhaAAhoNGIICGggcgwIaBB+DAjyEAjyF"
            "AjqGAjmIAjiIAiECFIkCFAIIBBSKAhQNFIsCFAwUjAIUCxSNAhQKFI4CFAkUjwIUBxWQAhQGFZEC"
            "FAUVkQIUBRWRAhQFFZECFQMVkwIUAxWTAhQDFZMCFAIVlAIVARWVAiqVAimWAimWAiiYAiaZAiaZ"
            "AiWaAiScAiKdAiGeAh+hAhyjAhmuAg3GxgEA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 20.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 20.0f, 160.0f,  60.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 100.0f, 160.0f, 140.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, 10.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 32,
            "3iUCngEEnAEGmgEImAEKlgEMlAEOkgEQkAESjgEUjAEWigEYiAEahgEchAEeggEggQEhfyN9JXsn"
            "eih4KnYUAhZ1FAMWcxQFFnIUBhZwFQcWbxQJFm4UChZsFQsWaxUMFmoVDRZpFQ4WaBUPFmcVEBZm"
            "FREWZhURFmUVEhZkFhIWZBYSFmQXERZkFxEWZBgQFmQaDhZlGwwWZh0JFmchBBZpOWw2bzN0Lnsn"
            "2mEA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 32,
            "njIUjAEUjAEUjAEUjAEUjAEUjQEUjAEUjAEUjAEUjQEUjAEUjAEUjQEUjAEUjQEUjAEVjAEUjQEU"
            "jAEVjAEVjAEVjAEVjAEVjAEVjAEVjQEVjAEVjAEWjAEWjAEXiwEXiwEYigEaiQEbiAEdhgEhgwEz"
            "ci53KX4ihwEZ6GEA");
    ok(match, "Figure does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    release_test_context(&ctx);
}

static void test_fill_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry[4];
    ID2D1RectangleGeometry *rect_geometry[2];
    D2D1_POINT_2F point = {0.0f, 0.0f};
    D2D1_ROUNDED_RECT rounded_rect;
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1GeometrySink *sink;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_ELLIPSE ellipse;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    factory = ctx.factory;

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&rect, 40.0f, 480.0f, 40.0f, 480.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);
    set_rect(&rect, 100.0f, 480.0f, 140.0f, 480.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);
    set_rect(&rect, 200.0f, 400.0f, 200.0f, 560.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);
    set_rect(&rect, 260.0f, 560.0f, 300.0f, 400.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);

    set_ellipse(&ellipse,  40.0f, 800.0f,  0.0f,  0.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);
    set_ellipse(&ellipse, 120.0f, 800.0f, 20.0f,  0.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);
    set_ellipse(&ellipse, 200.0f, 800.0f,  0.0f, 80.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);
    set_ellipse(&ellipse, 280.0f, 800.0f, 20.0f, 80.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 8,
            "yjIMjwEWhwEcggEgfiR6KHYscy5xMG40azZpOGc6ZTxjPmI+YUBfQl1EXERbRlpGWUhYSFdKVkpV"
            "TFRMVExTTlJOUk5STlJOUVBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUU5STlJOUk5STlNMVExUTFVK"
            "VkpXSFhIWUZaRltEXERdQl9AYT5iPmM8ZTpnOGk2azRuMHEucyx2KHokfiCCARyHARaPAQzKMgAA");
     ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "szI6YURZSlROUVBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUU5USllEYTqzMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 2,
            "tjI0aDxhQlxGWEpVTFNOUk5RUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFFOUk5TTFVKWEZcQmA+ZzS2MgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "sDJAWkxSUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFJMWkCwMgAA");
    ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 10,
            "yjIMjwEWhwEcggEgfiR6KHYscy5xMG40azZpOGc6ZTxjPmI+YUBfQl1EXERbRlpGWUhYSFdKVkpV"
            "TFRMVExTTlJOUk5STlJOUVBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUU5STlJOUk5STlNMVExUTFVK"
            "VkpXSFhIWUZaRltEXERdQl9AYT5iPmM8ZTpnOGk2azRuMHEucyx2KHokfiCCARyHARaPAQzKMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 10,
            "uTIucDJsNmk4ZzplPGM+YUBgQF9CXkJdRFxEW0ZaRllIWEhXSlZKVkpWSlVMVExUTFRMU05STlJO"
            "Uk5STlJOUk9QUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFFPUU5STlJOUk5STlJOU0xU"
            "TFRMVExVSlZKVkpWSldIWEhZRlpGW0RcRF1CXkJfQGBAYT5jPGU6ZzhpNmwycC65MgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 10,
            "vzIiczhhRldMUlBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUkxXRmA6cSS+MgAA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 40.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 200.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 60.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 560.0f);
    line_to(sink, 120.0f, 400.0f);
    line_to(sink, 120.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 220.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 560.0f);
    line_to(sink, 280.0f, 400.0f);
    line_to(sink, 280.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 40.0f, 720.0f);
    line_to(sink, 60.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 880.0f);
    line_to(sink, 140.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 200.0f, 720.0f);
    line_to(sink, 220.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 880.0f);
    line_to(sink, 300.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_ALTERNATE);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0,
            "7zMCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEWigEWiQEYiAEYhwEahgEahQEchAEcgwEeggEegQEggAEgfyJ+In0kfCR7JnomeSh4KHcq"
            "dip1LHQscy5yLnEwcDBvMm4ybTRsNGs2ajZpOGg4ZzpmOmU8ZDxjPmI+YUBgQF9CXkJdRFxEW0Za"
            "RllIWEhXSlZKVUxUTFNOUk5RUKgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0,
            "qDJQUU5STlNMVExVSlZKV0hYSFlGWkZbRFxEXUJeQl9AYEBhPmI+YzxkPGU6ZjpnOGg4aTZqNms0"
            "bDRtMm4ybzBwMHEuci5zLHQsdSp2KncoeCh5JnomeyR8JH0ifiJ/IIABIIEBHoIBHoMBHIQBHIUB"
            "GoYBGocBGIgBGIkBFooBFosBFIwBFI0BEo4BEo8BEJABEJEBDpIBDpMBDJQBDJUBCpYBCpcBCJgB"
            "CJkBBpoBBpsBBJwBBJ0BAp4BAu8z");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "7zMCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEWigEWiQEYiAEYhwEahgEahQEchAEcgwEeggEegQEggAEgfyJ+In0kfCR7JnomeSh4KHcq"
            "dip1LHQscy5yLnEwcDBvMm4ybTRsNGs2ajZpOGg4ZzpmOmU8ZDxjPmI+YUBgQF9CXkJdRFxEW0Za"
            "RllIWEhXSlZKVUxUTFNOUk5RUKgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "qDJQUU5STlNMVExVSlZKV0hYSFlGWkZbRFxEXUJeQl9AYEBhPmI+YzxkPGU6ZjpnOGg4aTZqNms0"
            "bDRtMm4ybzBwMHEuci5zLHQsdSp2KncoeCh5JnomeyR8JH0ifiJ/IIABIIEBHoIBHoMBHIQBHIUB"
            "GoYBGocBGIgBGIkBFooBFosBFIwBFI0BEo4BEo8BEJABEJEBDpIBDpMBDJQBDJUBCpYBCpcBCJgB"
            "CJkBBpoBBpsBBJwBBJ0BAp4BAu8z");
    ok(match, "Figure does not match.\n");

    set_rect(&rect, 20.0f, 80.0f, 60.0f, 240.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)rect_geometry[1], &matrix, &transformed_geometry[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[0], &matrix, &transformed_geometry[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)rect_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);
    ID2D1RectangleGeometry_Release(rect_geometry[1]);
    ID2D1RectangleGeometry_Release(rect_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 32,
            "sIMBA7cCDK8CFKYCHZ4CJJYCLY4CNYUCPv0BRvQBT+wBV+MBYNsBaNIBccoBecEBgQG6AYkBsQGS"
            "AakBmgGgAaMBmAGrAY8BtAGHAbwBfsUBfcYBfcYBfcUBfsUBfcYBfcYBfcYBfcYBfcUBfr0BhgG0"
            "AY8BrAGXAaMBoAGbAagBkgGwAYsBuAGCAcEBeskBcdIBadoBYOMBWOsBT/QBR/wBPoUCNowCLpUC"
            "Jp0CHaYCFa4CDLcCBK+DAQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 32,
            "+D0BngEDnQEDnAEEmwEGmgEGmQEHmAEJlwEJlgELlAEMkwENkwEOkQEPkAEQkAERjgESjQETjQEU"
            "iwEVigEXiQEXiAEYhwEahgEahQEbhAEdggEeggEegQEgfyF/In0jfCR8JXomeSd5KHcpdip2K3Qs"
            "cy5xL3EvcDFuMm4ybTRrNWs1ajdoOGg5ZjplO2U8Yz1iPmFAYEBfQV5DXUNcRVpGWkZZSFdJV0lW"
            "S1RMVExTTlFPUFFPUU5STVRMVEtVSldJV0hYR1pGWkVcQ11CXkJfQGA/YT9iPWM+Yj5jPWM+Yz1j"
            "PWM+Yz1jPmI+Yz1jPmI+Yz1jPmM9Yz1jPmM9Yz5iPmM9Yz5iPmM9Yz5jPWM9Yz5jPWM+Yj5jPWM+"
            "Yj5jPWI/YT9gQF9CXkJdRFtFW0VaR1hIV0lXSlVLVExUTVJOUVBQUE9RTlNNU0xUS1ZKVklXSFlG"
            "WkZaRVxDXUNeQV9AYEBhPmI9Yz1kO2U6ZjpnOGg3ajVrNWs0bTJuMm4xcC9xL3Eucyx0LHUqdil3"
            "KXgneSZ6JXwkfCN9In8hfyCBAR6CAR6CAR2EARuFARuFARqHARiIAReJAReKARWLARSNARONARKO"
            "ARGQARCQAQ+RAQ6TAQ2TAQyUAQuWAQqWAQmYAQeZAQaaAQabAQScAQOdAQOeAQH4PQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 32,
            "sXkBvgIDvAIEugIHuAIJtgILswINsgIPrwISrQITrAIVqQIYpwIapQIbowIeoQIgngIjnAIkmwIm"
            "mAIplgIqlQIskgIvkAIxjQIzjAI1igI3hwI5hgI7hAI9gQJA/wFB/QFE+wFG+QFI9gFK9QFM8wFO"
            "8AFQ7wFS7AFV6gFX6AFY5gFb5AFd4gFf3wFh3gFj2wFm2QFn2AFp1QFs0wFu0QFvzwFyzQF0ygF3"
            "yAF4xwF6xAF9wgF+wAGBAb4BgwG8AYUBuQGHAbgBiQG2AYsBswGOAbEBjwGvAZIBrQGUAasBlQGp"
            "AZgBpwGaAaUBnAGiAZ4BoQGgAZ4BowGcAaUBmgGmAZgBqQGWAasBlAGsAZIBrwGQAbEBjQG0AYsB"
            "tQGKAbcBhwG6AYUBvAGDAb0BgQHAAX/CAXzEAXvGAXvGAXvGAXvFAXvGAXvGAXvGAXvFAXvGAXvG"
            "AXvFAXvGAXvGAXvGAXvFAXvGAXvGAXvFAXzFAXvGAXvGAXvFAXvGAXvGAXvGAXvFAXvGAXvGAXvF"
            "AXzFAXvGAXvGAXvFAXvGAXvGAXvGAXvEAXzCAX/AAYEBvgGCAbwBhQG6AYcBtwGKAbUBiwG0AY0B"
            "sQGQAa8BkgGtAZMBqwGWAakBmAGmAZoBpQGcAaMBngGgAaEBngGiAZ0BpAGaAacBmAGpAZUBqwGU"
            "Aa0BkgGvAY8BsQGOAbMBjAG1AYkBuAGHAbkBhQG8AYMBvgGBAcABfsIBfcQBe8YBeMgBd8oBdM0B"
            "cs8BcNABbtMBbNUBatcBZ9kBZtsBY94BYd8BYOEBXeQBW+YBWOgBV+oBVewBUu8BUPABT/IBTPUB"
            "SvYBSPkBRvsBRP0BQf8BQIECPoMCO4YCOYcCN4oCNYwCM40CMZACL5ICLZQCKpYCKZgCJpsCJJ0C"
            "Ip4CIKECHqMCHKQCGqcCGKkCFawCE60CEq8CD7ICDbMCDLUCCbgCB7oCBLwCA74CAbF5");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 20.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 40.0f,  80.0f, 60.0f,  80.0f);
    quadratic_to(sink, 60.0f, 160.0f, 60.0f, 240.0f);
    quadratic_to(sink, 40.0f, 240.0f, 20.0f, 240.0f);
    quadratic_to(sink, 20.0f, 160.0f, 20.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 105.0f,  80.0f, 140.0f,  80.0f);
    quadratic_to(sink, 140.0f, 100.0f, 140.0f, 240.0f);
    quadratic_to(sink, 135.0f, 240.0f, 100.0f, 240.0f);
    quadratic_to(sink, 100.0f, 220.0f, 100.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 215.0f,  80.0f, 220.0f,  80.0f);
    quadratic_to(sink, 220.0f, 220.0f, 220.0f, 240.0f);
    quadratic_to(sink, 185.0f, 240.0f, 180.0f, 240.0f);
    quadratic_to(sink, 180.0f, 100.0f, 180.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 280.0f,  80.0f, 300.0f,  80.0f);
    quadratic_to(sink, 300.0f, 160.0f, 300.0f, 240.0f);
    quadratic_to(sink, 280.0f, 240.0f, 260.0f, 240.0f);
    quadratic_to(sink, 260.0f, 160.0f, 260.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 40.0f, 420.0f, 60.0f, 400.0f);
    quadratic_to(sink, 55.0f, 480.0f, 60.0f, 560.0f);
    quadratic_to(sink, 40.0f, 540.0f, 20.0f, 560.0f);
    quadratic_to(sink, 25.0f, 480.0f, 20.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 105.0f, 420.0f, 140.0f, 400.0f);
    quadratic_to(sink, 135.0f, 420.0f, 140.0f, 560.0f);
    quadratic_to(sink, 135.0f, 540.0f, 100.0f, 560.0f);
    quadratic_to(sink, 105.0f, 540.0f, 100.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 215.0f, 420.0f, 220.0f, 400.0f);
    quadratic_to(sink, 215.0f, 540.0f, 220.0f, 560.0f);
    quadratic_to(sink, 185.0f, 540.0f, 180.0f, 560.0f);
    quadratic_to(sink, 185.0f, 420.0f, 180.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 280.0f, 420.0f, 300.0f, 400.0f);
    quadratic_to(sink, 295.0f, 480.0f, 300.0f, 560.0f);
    quadratic_to(sink, 280.0f, 540.0f, 260.0f, 560.0f);
    quadratic_to(sink, 265.0f, 480.0f, 260.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 40.0f, 700.0f, 60.0f, 720.0f);
    quadratic_to(sink, 65.0f, 800.0f, 60.0f, 880.0f);
    quadratic_to(sink, 40.0f, 900.0f, 20.0f, 880.0f);
    quadratic_to(sink, 15.0f, 800.0f, 20.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 105.0f, 700.0f, 140.0f, 720.0f);
    quadratic_to(sink, 145.0f, 740.0f, 140.0f, 880.0f);
    quadratic_to(sink, 135.0f, 900.0f, 100.0f, 880.0f);
    quadratic_to(sink,  95.0f, 860.0f, 100.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 215.0f, 700.0f, 220.0f, 720.0f);
    quadratic_to(sink, 225.0f, 860.0f, 220.0f, 880.0f);
    quadratic_to(sink, 185.0f, 900.0f, 180.0f, 880.0f);
    quadratic_to(sink, 175.0f, 740.0f, 180.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 280.0f, 700.0f, 300.0f, 720.0f);
    quadratic_to(sink, 305.0f, 800.0f, 300.0f, 880.0f);
    quadratic_to(sink, 280.0f, 900.0f, 260.0f, 880.0f);
    quadratic_to(sink, 255.0f, 800.0f, 260.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 16,
            "qDICTAJQB0IHUQs4C1IRLBFSGxgbUk5STlNMVExUTFRMVExVSlZKVkpWSlZKVkpXSFhIWEhYSFhI"
            "WEhYSFhIWEhYSFlGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZa"
            "RllIWEhYSFhIWEhYSFhIWEhYSFhIV0pWSlZKVkpWSlZKVUxUTFRMVExUTFNOUk5SGxgbUhEsEVIL"
            "OAtRB0IHUAJMAqgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 16,
            "qDIBSwRQAkMKUQQ5EVIIKxtTDRkmVExUTFRMVEtVS1VLVkpWSlZKVklXSVdJV0lXSVhIWEhYSFhI"
            "WEhYSFhIWEhYSFhIWUdZR1lHWUdZR1lHWUdZR1lHWUdZSFhIWUdZR1lHWUdZR1lHWUdZR1lHWUdZ"
            "SFhIWEhYSFhIWEhYSFhIWEhYSFhJV0lXSVdJV0lWSlZKVkpWS1VLVUtUTFRMVExUJhkNUxsrCFIR"
            "OQRRCkMCUARLAagy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 16,
            "qDIESwFRCkMCUhE5BFIbKwhTJhkNVExUTFRMVUtVS1VLVUpWSlZKV0lXSVdJV0lXSVdIWEhYSFhI"
            "WEhYSFhIWEhYSFhIWEdZR1lHWUdZR1lHWUdZR1lHWUdYSFhIWEdZR1lHWUdZR1lHWUdZR1lHWUdY"
            "SFhIWEhYSFhIWEhYSFhIWEhYSFdJV0lXSVdJV0lXSlZKVkpVS1VLVUtVTFRMVExUDRkmUwgrG1IE"
            "ORFSAkMKUQFLBKgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 16,
            "qDICTAJQB0IHUQs4C1IRLBFSGxgbUk5STlNMVExUTFRMVExVSlZKVkpWSlZKVkpXSFhIWEhYSFhI"
            "WEhYSFhIWEhYSFlGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZa"
            "RllIWEhYSFhIWEhYSFhIWEhYSFhIV0pWSlZKVkpWSlZKVUxUTFRMVExUTFNOUk5SGxgbUhEsEVIL"
            "OAtRB0IHUAJMAqgy");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 16,
            "pCwYfixuOGNCWUxSUFBQT1JOUk5STlJOUk1UTFRMVExUTFRLVkpWSlZKVkpWSlZJWEhYSFhIWEhY"
            "SFhIWEhYSFhIWEdaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpG"
            "WkdYSFhIWEhYSFhIWEhYSFhIWEhYSVZKVkpWSlZKVkpWS1RMVExUTFRMVE1STlJOUk5STlJPUFBQ"
            "UkxZQmM4bix+GKQs");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 16,
            "liwZgQErcTllQ1xLVFBQUU9STlJNVExUTFRMVExVS1VLVUpWSlZKVkpXSVdJV0lXSVdIWEhYSFhI"
            "WEhYSFhIWEhYSFhIWEdZR1lHWUdZR1lHWUdZR1lHWUdZR1hIWEdZR1lHWUdZR1lHWUdZR1lHWUdZ"
            "R1hIWEhYSFhIWEhYSFhIWEhYSFhIV0lXSVdJV0lXSlZKVkpWSlVLVUtVTFRMVExUTFRNUk5ST1FQ"
            "UFRLXENlOXErgQEZliwA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 16,
            "sSwZeytrOV9DVktRUE9RTlJOUk1UTFRMVExUS1VLVUtVS1ZKVkpWSVdJV0lXSVdJV0lYSFhIWEhY"
            "SFhIWEhYSFhIWEhYSFlHWUdZR1lHWUdZR1lHWUdZR1lIWEhYSFlHWUdZR1lHWUdZR1lHWUdZR1lI"
            "WEhYSFhIWEhYSFhIWEhYSFhIWElXSVdJV0lXSVdJVkpWSlZLVUtVS1VLVExUTFRMVE1STlJOUU9Q"
            "UUtWQ185ayt7GbEs");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 16,
            "pCwYfixuOGNCWUxSUFBQT1JOUk5STlJOUk1UTFRMVExUTFRLVkpWSlZKVkpWSlZJWEhYSFhIWEhY"
            "SFhIWEhYSFhIWEdaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpG"
            "WkdYSFhIWEhYSFhIWEhYSFhIWEhYSVZKVkpWSlZKVkpWS1RMVExUTFRMVE1STlJOUk5STlJPUFBQ"
            "UkxZQmM4bix+GKQs");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, -0.402914f, 0.915514f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, -0.310379f,  0.882571f, -0.116057f,  0.824000f);
    quadratic_to(sink,  0.008350f,  0.693614f, -0.052343f,  0.448886f);
    quadratic_to(sink, -0.154236f,  0.246072f, -0.279229f,  0.025343f);
    quadratic_to(sink, -0.370064f, -0.588586f, -0.383029f, -0.924114f);
    quadratic_to(sink, -0.295479f, -0.958764f, -0.017086f, -0.988400f);
    quadratic_to(sink,  0.208836f, -0.954157f,  0.272200f, -0.924114f);
    quadratic_to(sink,  0.295614f, -0.569071f,  0.230143f,  0.022886f);
    quadratic_to(sink,  0.101664f,  0.220643f,  0.012057f,  0.451571f);
    quadratic_to(sink, -0.028764f,  0.709014f,  0.104029f,  0.833943f);
    quadratic_to(sink,  0.319414f,  0.913057f,  0.403229f,  0.942628f);
    quadratic_to(sink,  0.317721f,  1.023450f, -0.017086f,  1.021771f);
    quadratic_to(sink, -0.310843f,  1.007472f, -0.402914f,  0.915514f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 160.0f);
    scale_matrix(&matrix, 20.0f, 80.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[2], &matrix, &transformed_geometry[3]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[3], (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[3]);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 32,
            "6DMNjgEWiAEahgEahgEahgEahgEahgEahgEahgEahgEahgEahgEahwEZhwEZhwEZhwEZhwEZhwEZ"
            "hwEZhwEZhwEZiAEYiAEYiAEYiAEYiAEXiQEXiQEXiQEXigEWigEWigEWigEWigEWigEWigEWiwEU"
            "jAEUjAEUjAEUjQESjgESjwEQkAEQkQEOkgENlAEMlQEKlgEKlwEImAEImQEHmQEGmwEFmwEEnQED"
            "nQECngECngECnwEBnwEBnwEBnwEBnwEBnwECnQEDnQEDnQEEmwEFmgEHmQEHlwELkQERjAEXhgEd"
            "hAEfgwEchgEXjwEMqTEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 32,
            "h58BBrYCDq0CF6QCIJwCKJMCMIwCNoUCPf8BQ/kBSPQBTu4BTe8BTPEBSfUBRvgBQf0BPYECOYUC"
            "NIoCMI4CK+UBAS0W/AEHIwiPAgsaBZcCEAwIngIepAIaqAIWrAITsAIRsgIPtQIMtwILugIHwAIB"
            "ypwB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 32,
            "wW4DnAEEmwEFmgEHmAEIlwEKlQELlAEMkwEOkQEPkAEQkAERjgESjgETjAEUjAEUiwEWigEWiQEX"
            "iQEYhwEZhwEZhgEbhQEbhAEchAEdggEeggEeggEfgAEggAEggAEhgAEggAEggQEggAEggAEggQEg"
            "gAEggQEfgQEfggEfgQEfgQEfggEfgQEfggEeggEfggEeggEegwEdgwEeggEegwEdgwEegwEdgwEd"
            "hAEchAEdhAEchAEchAEdhAEchAEchQEbhQEbhgEahgEahwEZhwEZiAEYiAEYiQEYiAEYiQEXiQEX"
            "igEWigEWiwEViwEViwEVjAEUjAEUjQETjQETjgESjgETjgESjwERkAEQkQEPkwENlAEMlQELlgEK"
            "lwEKlwEJmAEImQEHmgEGmwEFnAEEnQEEnQEDnQEDngECngEDngECngECnwECngECnwECngECngED"
            "ngECEgGLAQMQAosBAw4EjAEDCwaMAQQJBo0BBQYIjQEHAgqNARKOARKPARCQARCQARCQAQ+RAQ6S"
            "AQ6SAQ2TAQ2SAQ2TAQ2TAQyTAQyUAQyUAQuUAQuVAQuUAQuVAQqWAQmWAQqWAQmXAQiXAQiYAQeY"
            "AQeZAQWbAQSDZwAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 32,
            "g90BBLkCCLYCC7ICDrACEa0CFKoCF6cCGqQCHKMCHqECIJ8CIpwCJJsCJpkCKJcCKZYCK5QCLZIC"
            "L5ACMI8CMo0CNIsCNYoCN4gCOYcCOYYCO4QCPYICPoECQIACQYACQIECQIACQIECQIECQIECP4IC"
            "P4ICP4ECP4ICP4ICPoMCPoMCPoMCPYQCPYMCPYQCPYQCPYQCPIUCPIUCPIUCO4YCO4YCOoYCO4YC"
            "OocCOocCOocCOYgCOYgCOIkCOIkCN4oCNosCNYwCNI0CM44CMo4CM44CMo8CMZACMJECL5ICLpMC"
            "LZQCLJUCK5YCK5YCKpcCKZgCKJkCJ5oCJpsCJpsCJZwCJJ4CIqACIKICH6MCHaUCG6cCGakCF6wC"
            "Fa0CE68CEbECD7MCDrQCDLYCCrgCCbkCB7sCBrsCBbwCBbwCBL0CBL0CBL0CBL0CA70CBL0CBL0C"
            "BLwCBSUBlgIFIQSXAgYbCJcCBxcKmQIIEQ6ZAgoMEJoCDQUTnAIknAIjnQIingIhnwIgoAIfoQIe"
            "ogIdowIcpAIbpQIapQIZpgIZpgIZpwIYpwIXqAIXqAIXqQIVqgIVqgIUqwITrQISrQIRrgIQsAIO"
            "sQIMswILtQIIhs4B");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, -0.402914f, 0.915514f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, -0.310379f,  0.882571f, -0.116057f,  0.824000f);
    quadratic_to(sink,  0.008350f,  0.693614f, -0.052343f,  0.448886f);
    quadratic_to(sink, -0.154236f,  0.246072f, -0.279229f,  0.025343f);
    quadratic_to(sink, -0.370064f, -0.588586f, -0.383029f, -0.924114f);
    quadratic_to(sink, -0.295479f, -0.958764f, -0.017086f, -0.988400f);
    quadratic_to(sink,  0.208836f, -0.954157f,  0.272200f, -0.924114f);
    quadratic_to(sink,  0.295614f, -0.569071f,  0.230143f,  0.022886f);
    quadratic_to(sink,  0.101664f,  0.220643f,  0.012057f,  0.451571f);
    quadratic_to(sink, -0.028764f,  0.709014f,  0.104029f,  0.833943f);
    quadratic_to(sink,  0.319414f,  0.913057f,  0.403229f,  0.942628f);
    quadratic_to(sink,  0.317721f,  1.023450f, -0.017086f,  1.021771f);
    quadratic_to(sink, -0.310843f,  1.007472f, -0.402914f,  0.915514f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 160.0f);
    scale_matrix(&matrix, 20.0f, 80.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[2], &matrix, &transformed_geometry[3]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[3], (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[3]);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 0, "gJAD");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 0, "gJAD");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 0, "gKAG");
    ok(match, "Figure does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    release_test_context(&ctx);
}

static void test_wic_gdi_interop(BOOL d3d11)
{
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    IWICImagingFactory *wic_factory;
    struct d2d1_test_context ctx;
    IWICBitmapLock *wic_lock;
    IWICBitmap *wic_bitmap;
    ID2D1RenderTarget *rt;
    D2D1_COLOR_F color;
    HRESULT hr;
    BOOL match;
    RECT rect;
    HDC dc;

    if (!init_test_context(&ctx, d3d11))
        return;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    /* WIC target, default usage */
    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory_CreateWicBitmapRenderTarget(ctx.factory, wic_bitmap, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    dc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == D2DERR_TARGET_NOT_GDI_COMPATIBLE, "Got unexpected hr %#lx.\n", hr);
    ok(!dc, "Got unexpected DC %p.\n", dc);
    ID2D1GdiInteropRenderTarget_Release(interop);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_Release(rt);

    /* WIC target, gdi compatible */
    desc.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

    hr = ID2D1Factory_CreateWicBitmapRenderTarget(ctx.factory, wic_bitmap, &desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    dc = NULL;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(dc != NULL, "Expected NULL dc, got %p.\n", dc);
    hr = ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_wic_bitmap(wic_bitmap, "54034063dbc1c1bb61cb60ec57e4498678dc2b13");
    ok(match, "Bitmap does not match.\n");

    /* Do solid fill using GDI */
    ID2D1RenderTarget_BeginDraw(rt);

    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    SetRect(&rect, 0, 0, 16, 16);
    FillRect(dc, &rect, GetStockObject(BLACK_BRUSH));
    hr = ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    match = compare_wic_bitmap(wic_bitmap, "60cacbf3d72e1e7834203da608037b1bf83b40e8");
    ok(match, "Bitmap does not match.\n");

    /* Bitmap is locked at BeginDraw(). */
    hr = IWICBitmap_Lock(wic_bitmap, NULL, WICBitmapLockRead, &wic_lock);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IWICBitmapLock_Release(wic_lock);

    ID2D1RenderTarget_BeginDraw(rt);
    hr = IWICBitmap_Lock(wic_bitmap, NULL, WICBitmapLockRead, &wic_lock);
    todo_wine ok(hr == WINCODEC_ERR_ALREADYLOCKED, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IWICBitmapLock_Release(wic_lock);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Lock before BeginDraw(). */
    hr = IWICBitmap_Lock(wic_bitmap, NULL, WICBitmapLockRead, &wic_lock);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1RenderTarget_BeginDraw(rt);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    todo_wine ok(hr == WINCODEC_ERR_ALREADYLOCKED, "Got unexpected hr %#lx.\n", hr);
    IWICBitmapLock_Release(wic_lock);

    ID2D1GdiInteropRenderTarget_Release(interop);
    ID2D1RenderTarget_Release(rt);

    IWICBitmap_Release(wic_bitmap);
    release_test_context(&ctx);

    CoUninitialize();
}

static void test_layer(BOOL d3d11)
{
    ID2D1Factory *factory, *layer_factory;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    ID2D1Layer *layer;
    D2D1_SIZE_F size;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    factory = ctx.factory;

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    hr = ID2D1RenderTarget_CreateLayer(rt, NULL, &layer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1Layer_GetFactory(layer, &layer_factory);
    ok(layer_factory == factory, "Got unexpected layer factory %p, expected %p.\n", layer_factory, factory);
    ID2D1Factory_Release(layer_factory);
    size = ID2D1Layer_GetSize(layer);
    ok(size.width == 0.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 0.0f, "Got unexpected height %.8e.\n", size.height);
    ID2D1Layer_Release(layer);

    set_size_f(&size, 800.0f, 600.0f);
    hr = ID2D1RenderTarget_CreateLayer(rt, &size, &layer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    size = ID2D1Layer_GetSize(layer);
    ok(size.width == 800.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 600.0f, "Got unexpected height %.8e.\n", size.height);
    ID2D1Layer_Release(layer);

    release_test_context(&ctx);
}

static void test_bezier_intersect(BOOL d3d11)
{
    D2D1_POINT_2F point = {0.0f, 0.0f};
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    ID2D1GeometrySink *sink;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F color;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    factory = ctx.factory;

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 160.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 119.0f, 720.0f,  83.0f, 600.0f,  80.0f, 474.0f);
    cubic_to(sink,  78.0f, 349.0f, 108.0f, 245.0f, 135.0f, 240.0f);
    cubic_to(sink, 163.0f, 235.0f, 180.0f, 318.0f, 176.0f, 370.0f);
    cubic_to(sink, 171.0f, 422.0f, 149.0f, 422.0f, 144.0f, 370.0f);
    cubic_to(sink, 140.0f, 318.0f, 157.0f, 235.0f, 185.0f, 240.0f);
    cubic_to(sink, 212.0f, 245.0f, 242.0f, 349.0f, 240.0f, 474.0f);
    cubic_to(sink, 238.0f, 600.0f, 201.0f, 720.0f, 160.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 160.0f, 240.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 240.0f, 240.0f);
    line_to(sink, 240.0f, 720.0f);
    line_to(sink, 160.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx, 160, 120, 320, 240, 0xff652e89, 2048,
            "aRQjIxRpYiIcHCJiXSwXFyxdWTQTEzRZVTsQEDtVUkIMDEJST0cKCkdPTUsICEtNSlEFBVFKSFUD"
            "A1VIRlkBAVlGRFsBAVtEQlwCAlxCQFwEBFxAPl0FBV0+PF0HB108Ol4ICF46OV0KCl05N14LC143"
            "Nl4MDF42NF8NDV80M14PD14zMV8QEF8xMF8REV8wL18SEl8vLWATE2AtLGAUFGAsK2EUFGErKWIV"
            "FWIpKGIWFmIoJ2IXF2InJmIYGGImJWMYGGMlJGMZGWMkI2MaGmMjImQaGmQiIWQbG2QhIGQcHGQg"
            "H2UcHGUfHmUdHWUeHWYdHWYdHGcdHWccG2ceHmcbGmgeHmgaGWgfH2gZGWgfH2gZGGkfH2kYF2kg"
            "IGkXFmogIGoWFmogIGoWFWsgIGsVFGshIWsUE2whIWwTE2whIWwTEm0hIW0SEW4hIW4REW4hIW4R"
            "EG8hIW8QD3AhIXAPD3AhIXAPDnEhIXEODnEhIXEODXIhIXINDHQgIHQMDHQgIHQMC3UgIHULC3Yf"
            "H3YLCncfH3cKCngeHngKCXkeHnkJCXodHXoJCXscHHsJCHwcHHwICH0bG30IB38aGn8HB4ABGRmA"
            "AQcHgQEYGIEBBwaEARYWhAEGBoUBFRWFAQYFiAETE4gBBQWKARERigEFBYwBDw+MAQUEkAEMDJAB"
            "BASTAQkJkwEEBJwBnAEEA50BnQEDA50BnQEDA50BnQEDA50BnQEDAp4BngECAp4BngECAp4BngEC"
            "Ap4BngECAp4BngECAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwGhAaAB"
            "oAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGg"
            "AaABoAGgAaABoAGgAaABoAGhAZ8BoQGfAZ8BAQGfAZ8BAQGfAZ8BAQGfAZ8BAQGfAZ8BAQKeAZ8B"
            "AQKeAZ4BAgKeAZ4BAgKeAZ4BAgOdAZ4BAgOdAZ4BAgOdAZ0BAwScAZ0BAwScAZ0BAwScAZ0BAwSc"
            "AZwBBAWbAZwBBAWbAZwBBAWbAZsBBQaaAZsBBQaaAZoBBgeZAZoBBgeZAZoBBgeZAZkBBwiYAZkB"
            "BwiYAZgBCAmXAZgBCAmXAZgBCAmXAZcBCQqWAZcBCQqWAZYBCguVAZYBCguVAZUBCwyUAZUBCw2T"
            "AZQBDA2TAZQBDA6SAZMBDQ6SAZMBDQ+RAZIBDg+RAZIBDhCQAZEBDxCQAZABEBGPAZABEBKOAY8B"
            "ERONAY4BEhONAY4BEhSMAY0BExWLAYwBFBWLAYwBFBaKAYsBFReJAYoBFheJAYoBFhiIAYkBFxmH"
            "AYgBGBqGAYcBGRuFAYYBGhuFAYUBGxyEAYUBGx2DAYQBHB6CAYMBHR+BAYIBHiCAAYEBHyF/gAEg"
            "In5/ISJ+fiIjfX0jJHx8JCV7eyUmenomJ3l5Jyh4eCgpd3cpK3V2Kix0dSstc3QsLnJzLS9xci4w"
            "cHAwMm5vMTNtbjI0bG0zNWtrNTdpajY4aGk3OmZnOTtlZjo8ZGQ8PmJjPT9hYj5BX2BAQl5eQkRc"
            "XUNGWltFR1lZR0lXWEhLVVZKTVNUTE9RUk5RT1BQUk5OUlRMTFRWSkpWWUdIWFtFRVteQkNdYEBA"
            "YGI+PmJlOztlaDg4aGs1NWtuMjJuci4vcXUrK3V6JiZ6fiIifoMBHR2DAYsBFRWLAZUBCwuVAQAA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&point, 240.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 152.0f, 720.0f,  80.0f, 613.0f,  80.0f, 480.0f);
    cubic_to(sink,  80.0f, 347.0f, 152.0f, 240.0f, 240.0f, 240.0f);
    cubic_to(sink, 152.0f, 339.0f, 134.0f, 528.0f, 200.0f, 660.0f);
    cubic_to(sink, 212.0f, 683.0f, 225.0f, 703.0f, 240.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx, 160, 120, 320, 240, 0xff652e89, 2048,
            "pQIZkgIrhAI5/QE/9gFH7wFO6wFS5wFW4gFb3gFf2wFi2AFl1gFn1AFp0gFszwFuzQFxywFyyQF1"
            "xwF2xgF4xAF5xAF6wgF8wAF+vwF+vwF/vQGBAbwBggG7AYMBugGEAbkBhQG4AYYBtwGHAbcBiAG1"
            "AYkBtAGKAbQBigGzAYsBswGMAbEBjQGxAY0BsQGOAa8BjwGvAZABrgGQAa4BkQGtAZEBrQGSAawB"
            "kgGsAZMBqwGTAasBlAGrAZQBqgGVAakBlQGqAZUBqQGWAagBlwGoAZYBqAGXAagBlwGnAZgBpwGY"
            "AaYBmQGmAZkBpgGZAaUBmgGlAZoBpQGaAaUBmgGkAZsBpAGbAaQBmwGkAZsBpAGcAaMBnAGjAZwB"
            "owGcAaMBnAGjAZ0BogGdAaIBnQGiAZ4BoQGeAaEBngGiAZ4BoQGeAaEBnwGgAZ8BoQGeAaEBnwGh"
            "AZ4BoQGfAaABnwGhAZ8BoAGgAaABnwGgAaABoAGfAaABoAGgAaABnwGgAaABoAGgAaABnwGgAaAB"
            "oAGgAaABnwGhAZ8BoQGfAaABoAGgAaABoAGfAaEBnwGhAZ8BoQGfAaEBnwGhAZ8BoQGfAaABoAGg"
            "AaABoAGgAaEBnwGhAZ8BoQGfAaEBnwGhAaABoAGgAaABoAGgAaABoAGgAaEBoAGgAaABoAGgAaAB"
            "oQGgAaABoAGgAaABoQGfAaEBoAGhAZ8BoQGfAaIBnwGhAZ8BogGfAaEBnwGiAZ8BogGeAaIBnwGi"
            "AZ4BogGfAaIBngGjAZ4BowGdAaMBngGjAZ4BowGdAaQBnQGkAZ0BpAGcAaUBnAGlAZwBpQGcAaUB"
            "mwGmAZsBpgGbAaYBmwGmAZsBpgGbAacBmgGnAZkBqAGZAagBmQGpAZgBqQGZAagBmQGpAZgBqQGY"
            "AaoBlwGqAZcBqwGWAasBlgGsAZUBrQGVAawBlQGtAZQBrgGUAa0BlAGuAZMBrwGTAa8BkgGwAZEB"
            "sQGRAbEBkAGyAZABsgGPAbMBjwG0AY4BtAGNAbUBjQG2AYwBtgGLAbgBigG4AYoBuQGJAboBhwG7"
            "AYcBvAGGAb0BhQG+AYQBvwGDAcABggHBAYIBwgGAAcMBf8QBfsYBfMgBe8gBesoBeMwBd80BddAB"
            "c9EBcdQBb9YBbNkBatsBaN0BZeEBYuQBX+gBW+0BVvEBUvUBTvwBR4QCQIoCOZgCK6oCGQIA");
    ok(match, "Figure does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    release_test_context(&ctx);
}

static void test_create_device(BOOL d3d11)
{
    D2D1_CREATION_PROPERTIES properties = {0};
    struct d2d1_test_context ctx;
    ID2D1Factory *factory2;
    ID2D1Device *device;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (!ctx.factory1)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1Factory1_CreateDevice(ctx.factory1, ctx.device, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1Device_GetFactory(device, &factory2);
    ok(factory2 == (ID2D1Factory *)ctx.factory1, "Got unexpected factory.\n");
    ID2D1Factory_Release(factory2);
    ID2D1Device_Release(device);

    if (pD2D1CreateDevice)
    {
        hr = pD2D1CreateDevice(ctx.device, NULL, &device);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID2D1Device_Release(device);

        hr = pD2D1CreateDevice(ctx.device, &properties, &device);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID2D1Device_Release(device);
    }
    else
        win_skip("D2D1CreateDevice() is unavailable.\n");

    release_test_context(&ctx);
}

#define check_rt_bitmap_surface(r, s, o) check_rt_bitmap_surface_(__LINE__, r, s, o)
static void check_rt_bitmap_surface_(unsigned int line, ID2D1RenderTarget *rt, BOOL has_surface, DWORD options)
{
    ID2D1BitmapRenderTarget *compatible_rt;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    IWICImagingFactory *wic_factory;
    ID2D1Bitmap *bitmap, *bitmap2;
    ID2D1DeviceContext *context;
    ID2D1DCRenderTarget *dc_rt;
    IWICBitmap *wic_bitmap;
    ID2D1Image *target;
    D2D1_SIZE_U size;
    HRESULT hr;

    static const DWORD bitmap_data[] =
    {
        0x7f7f0000,
    };

    /* Raw data bitmap. */
    set_size_u(&size, 1, 1);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create bitmap, hr %#lx.\n", hr);

    check_bitmap_surface_(line, bitmap, has_surface, options);

    ID2D1Bitmap_Release(bitmap);

    /* Zero sized bitmaps. */
    set_size_u(&size, 0, 0);
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create bitmap, hr %#lx.\n", hr);
    check_bitmap_surface_(line, bitmap, has_surface, options);
    ID2D1Bitmap_Release(bitmap);

    set_size_u(&size, 2, 0);
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create bitmap, hr %#lx.\n", hr);
    check_bitmap_surface_(line, bitmap, has_surface, options);
    ID2D1Bitmap_Release(bitmap);

    set_size_u(&size, 0, 2);
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create bitmap, hr %#lx.\n", hr);
    check_bitmap_surface_(line, bitmap, has_surface, options);
    ID2D1Bitmap_Release(bitmap);

    /* WIC bitmap. */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create WIC imaging factory, hr %#lx.\n", hr);

    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create WIC bitmap, hr %#lx.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    hr = ID2D1RenderTarget_CreateBitmapFromWicBitmap(rt, (IWICBitmapSource *)wic_bitmap, NULL, &bitmap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create bitmap from WIC source, hr %#lx.\n", hr);

    check_bitmap_surface_(line, bitmap, has_surface, options);

    ID2D1Bitmap_Release(bitmap);

    CoUninitialize();

    /* Compatible target follows its parent. */
    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get device context, hr %#lx.\n", hr);

    dc_rt = NULL;
    ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DCRenderTarget, (void **)&dc_rt);

    bitmap = NULL;
    target = NULL;
    ID2D1DeviceContext_GetTarget(context, &target);
    if (target && FAILED(ID2D1Image_QueryInterface(target, &IID_ID2D1Bitmap, (void **)&bitmap)))
    {
        ID2D1Image_Release(target);
        target = NULL;
    }
    if (bitmap)
    {
        D2D1_PIXEL_FORMAT rt_format, bitmap_format;

        rt_format = ID2D1RenderTarget_GetPixelFormat(rt);
        bitmap_format = ID2D1Bitmap_GetPixelFormat(bitmap);
        ok_(__FILE__, line)(!memcmp(&rt_format, &bitmap_format, sizeof(rt_format)), "Unexpected bitmap format.\n");

        ID2D1Bitmap_Release(bitmap);
    }

    /* Pixel format is not defined until target is set, for DC target it's specified on creation. */
    if (target || dc_rt)
    {
        ID2D1Device *device, *device2;
        ID2D1DeviceContext *context2;

        ID2D1DeviceContext_GetDevice(context, &device);

        hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, NULL, NULL, NULL,
                D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &compatible_rt);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create compatible render target, hr %#lx.\n", hr);

        hr = ID2D1BitmapRenderTarget_QueryInterface(compatible_rt, &IID_ID2D1DeviceContext, (void **)&context2);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get device context, hr %#lx.\n", hr);

        ID2D1DeviceContext_GetDevice(context2, &device2);
        ok_(__FILE__, line)(device == device2, "Unexpected device.\n");

        ID2D1Device_Release(device);
        ID2D1Device_Release(device2);

        hr = ID2D1BitmapRenderTarget_CreateBitmap(compatible_rt, size,
                bitmap_data, sizeof(*bitmap_data), &bitmap_desc, &bitmap);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create bitmap, hr %#lx.\n", hr);
        check_bitmap_surface_(line, bitmap, has_surface, options);
        ID2D1Bitmap_Release(bitmap);

        hr = ID2D1BitmapRenderTarget_GetBitmap(compatible_rt, &bitmap);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get compatible target bitmap, hr %#lx.\n", hr);

        bitmap2 = NULL;
        ID2D1DeviceContext_GetTarget(context2, (ID2D1Image **)&bitmap2);
        ok_(__FILE__, line)(bitmap2 == bitmap, "Unexpected bitmap.\n");

        check_bitmap_surface_(line, bitmap, has_surface, D2D1_BITMAP_OPTIONS_TARGET);
        ID2D1Bitmap_Release(bitmap2);
        ID2D1Bitmap_Release(bitmap);

        ID2D1BitmapRenderTarget_Release(compatible_rt);
        ID2D1DeviceContext_Release(context2);
    }
    else
    {
        hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, NULL, NULL, NULL,
                 D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &compatible_rt);
        ok_(__FILE__, line)(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT, "Unexpected hr %#lx.\n", hr);
    }

    ID2D1DeviceContext_Release(context);
    if (target)
        ID2D1Image_Release(target);
    if (dc_rt)
        ID2D1DCRenderTarget_Release(dc_rt);
}

static void test_bitmap_surface(BOOL d3d11)
{
    static const struct bitmap_format_test
    {
        D2D1_PIXEL_FORMAT original;
        D2D1_PIXEL_FORMAT result;
        HRESULT hr;
    }
    bitmap_format_tests[] =
    {
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
          { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },

        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
          { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
          { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, D2DERR_UNSUPPORTED_PIXEL_FORMAT },
    };
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    ID2D1DeviceContext *device_context;
    IDXGISurface *surface2;
    D2D1_PIXEL_FORMAT pixel_format;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    ID2D1Bitmap1 *bitmap;
    ID2D1Device *device;
    ID2D1Image *target;
    D2D1_SIZE_U size;
    D2D1_TAG t1, t2;
    unsigned int i;
    HRESULT hr;

    IWICBitmap *wic_bitmap;
    IWICImagingFactory *wic_factory;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (!ctx.factory1)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    /* DXGI target */
    hr = ID2D1RenderTarget_QueryInterface(ctx.rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    bitmap = NULL;
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(!!bitmap, "Unexpected target.\n");
    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);
    ID2D1Bitmap1_Release(bitmap);

    check_rt_bitmap_surface(ctx.rt, TRUE, D2D1_BITMAP_OPTIONS_NONE);

    ID2D1DeviceContext_Release(device_context);

    /* Bitmap created from DXGI surface. */
    hr = ID2D1Factory1_CreateDevice(ctx.factory1, ctx.device, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(bitmap_format_tests); ++i)
    {
        memset(&bitmap_desc, 0, sizeof(bitmap_desc));
        bitmap_desc.pixelFormat = bitmap_format_tests[i].original;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, ctx.surface, &bitmap_desc, &bitmap);
        todo_wine_if(bitmap_format_tests[i].hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT)
        ok(hr == bitmap_format_tests[i].hr, "%u: Got unexpected hr %#lx.\n", i, hr);

        if (SUCCEEDED(hr) && hr == bitmap_format_tests[i].hr)
        {
            pixel_format = ID2D1Bitmap1_GetPixelFormat(bitmap);

            ok(pixel_format.format == bitmap_format_tests[i].result.format, "%u: unexpected pixel format %#x.\n",
                    i, pixel_format.format);
            ok(pixel_format.alphaMode == bitmap_format_tests[i].result.alphaMode, "%u: unexpected alpha mode %d.\n",
                    i, pixel_format.alphaMode);
        }

        if (SUCCEEDED(hr))
            ID2D1Bitmap1_Release(bitmap);
    }

    /* A8 surface */
    surface2 = create_surface(ctx.device, 1, 1, D3D10_BIND_SHADER_RESOURCE, 0, DXGI_FORMAT_A8_UNORM);

    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, surface2, NULL, &bitmap);
    ok(hr == S_OK || broken(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT) /* Win7 */,
            "Got unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        pixel_format = ID2D1Bitmap1_GetPixelFormat(bitmap);
        ok(pixel_format.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED,
                "Unexpected alpha mode %#x.\n", pixel_format.alphaMode);

        ID2D1Bitmap1_Release(bitmap);
    }

    IDXGISurface_Release(surface2);

    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, ctx.surface, NULL, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    pixel_format = ID2D1Bitmap1_GetPixelFormat(bitmap);
    ok(pixel_format.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED,
            "Unexpected alpha mode %#x.\n", pixel_format.alphaMode);

    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);
    check_rt_bitmap_surface((ID2D1RenderTarget *)device_context, TRUE, D2D1_BITMAP_OPTIONS_NONE);

    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    ID2D1DeviceContext_GetTarget(device_context, &target);
    ok(target == (ID2D1Image *)bitmap, "Unexpected target.\n");
    ID2D1Image_Release(target);

    check_rt_bitmap_surface((ID2D1RenderTarget *)device_context, TRUE, D2D1_BITMAP_OPTIONS_NONE);

    ID2D1Bitmap1_Release(bitmap);

    /* Without D2D1_BITMAP_OPTIONS_TARGET. */
    memset(&bitmap_desc, 0, sizeof(bitmap_desc));
    bitmap_desc.pixelFormat = ID2D1DeviceContext_GetPixelFormat(device_context);
    size.width = size.height = 4;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_NONE);
    ID2D1DeviceContext_SetTags(device_context, 1, 2);

    ID2D1DeviceContext_BeginDraw(device_context);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    hr = ID2D1DeviceContext_EndDraw(device_context, &t1, &t2);
    ok(hr == D2DERR_INVALID_TARGET, "Got unexpected hr %#lx.\n", hr);
    ok(t1 == 1 && t2 == 2, "Unexpected tags %s:%s.\n", wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));

    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(!!bitmap, "Expected target bitmap.\n");
    ID2D1Bitmap1_Release(bitmap);

    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_TARGET);
    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_SetTags(device_context, 3, 4);

    ID2D1DeviceContext_BeginDraw(device_context);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    hr = ID2D1DeviceContext_EndDraw(device_context, &t1, &t2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!t1 && !t2, "Unexpected tags %s:%s.\n", wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));

    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_Release(device_context);

    ID2D1Device_Release(device);

    /* DC target */
    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rt_desc.dpiX = 96.0f;
    rt_desc.dpiY = 96.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory1_CreateDCRenderTarget(ctx.factory1, &rt_desc, (ID2D1DCRenderTarget **)&rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(!bitmap, "Unexpected target.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1RenderTarget_Release(rt);

    /* HWND target */
    hwnd_rt_desc.hwnd = NULL;
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");

    hr = ID2D1Factory1_CreateHwndRenderTarget(ctx.factory1, &rt_desc, &hwnd_rt_desc, (ID2D1HwndRenderTarget **)&rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_rt_bitmap_surface(rt, FALSE, D2D1_BITMAP_OPTIONS_NONE);
    ID2D1RenderTarget_Release(rt);
    DestroyWindow(hwnd_rt_desc.hwnd);

    /* WIC target */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    hr = ID2D1Factory1_CreateWicBitmapRenderTarget(ctx.factory1, wic_bitmap, &rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IWICBitmap_Release(wic_bitmap);

    check_rt_bitmap_surface(rt, FALSE, D2D1_BITMAP_OPTIONS_NONE);
    ID2D1RenderTarget_Release(rt);

    CoUninitialize();

    release_test_context(&ctx);
}

static void test_device_context(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    ID2D1DeviceContext *device_context;
    IDXGISurface *surface, *surface2;
    ID2D1Device *device, *device2;
    struct d2d1_test_context ctx;
    D2D1_BITMAP_OPTIONS options;
    ID2D1DCRenderTarget *dc_rt;
    D2D1_UNIT_MODE unit_mode;
    ID2D1RenderTarget *rt;
    ID2D1Bitmap1 *bitmap;
    ID2D1Image *target;
    HRESULT hr;
    RECT rect;
    HDC hdc;

    IWICBitmap *wic_bitmap;
    IWICImagingFactory *wic_factory;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (!ctx.factory1)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1Factory1_CreateDevice(ctx.factory1, ctx.device, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DeviceContext_GetDevice(device_context, &device2);
    ok(device2 == device, "Unexpected device instance.\n");
    ID2D1Device_Release(device2);

    target = (void *)0xdeadbeef;
    ID2D1DeviceContext_GetTarget(device_context, &target);
    ok(target == NULL, "Unexpected target instance %p.\n", target);

    unit_mode = ID2D1DeviceContext_GetUnitMode(device_context);
    ok(unit_mode == D2D1_UNIT_MODE_DIPS, "Unexpected unit mode %d.\n", unit_mode);

    ID2D1DeviceContext_Release(device_context);

    if (ctx.factory2)
    {
        ID2D1Device1 *device1;
        ID2D1DeviceContext1 *device_context1;

        hr = ID2D1Factory2_CreateDevice(ctx.factory2, ctx.device, &device1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Device1_CreateDeviceContext(device1, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID2D1DeviceContext1_Release(device_context1);
        ID2D1Device1_Release(device1);
    }

    /* DXGI target */
    rt = ctx.rt;
    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(surface2 == ctx.surface, "Unexpected surface instance.\n");
    IDXGISurface_Release(surface2);

    ID2D1DeviceContext_BeginDraw(device_context);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(surface2 == ctx.surface, "Unexpected surface instance.\n");
    IDXGISurface_Release(surface2);
    ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);

    /* WIC target */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rt_desc.dpiX = 96.0f;
    rt_desc.dpiY = 96.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory1_CreateWicBitmapRenderTarget(ctx.factory1, wic_bitmap, &rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1RenderTarget_Release(rt);

    CoUninitialize();

    /* HWND target */
    hwnd_rt_desc.hwnd = NULL;
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");

    hr = ID2D1Factory1_CreateHwndRenderTarget(ctx.factory1, &rt_desc, &hwnd_rt_desc, (ID2D1HwndRenderTarget **)&rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
    ok(hr == D2DERR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1RenderTarget_Release(rt);
    DestroyWindow(hwnd_rt_desc.hwnd);

    /* DC target */
    hr = ID2D1Factory1_CreateDCRenderTarget(ctx.factory1, &rt_desc, &dc_rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DCRenderTarget_QueryInterface(dc_rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected bitmap instance.\n");

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "Failed to create an HDC.\n");

    create_target_dibsection(hdc, 16, 16);

    SetRect(&rect, 0, 0, 16, 16);
    hr = ID2D1DCRenderTarget_BindDC(dc_rt, hdc, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
    ok(hr == D2DERR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1DCRenderTarget_Release(dc_rt);
    DeleteDC(hdc);

    ID2D1Device_Release(device);
    release_test_context(&ctx);
}

static void test_invert_matrix(BOOL d3d11)
{
    static const struct
    {
        D2D1_MATRIX_3X2_F matrix;
        D2D1_MATRIX_3X2_F inverse;
        BOOL invertible;
    }
    invert_tests[] =
    {
        { {{{ 0 }}}, {{{ 0 }}}, FALSE },
        {
            {{{
               1.0f, 2.0f,
               1.0f, 2.0f,
               4.0f, 8.0f
            }}},
            {{{
               1.0f, 2.0f,
               1.0f, 2.0f,
               4.0f, 8.0f
            }}},
            FALSE
        },
        {
            {{{
               2.0f, 0.0f,
               0.0f, 2.0f,
               4.0f, 8.0f
            }}},
            {{{
               0.5f, -0.0f,
              -0.0f,  0.5f,
              -2.0f, -4.0f
            }}},
            TRUE
        },
        {
            {{{
               2.0f, 1.0f,
               2.0f, 2.0f,
               4.0f, 8.0f
            }}},
            {{{
               1.0f, -0.5f,
              -1.0f,  1.0f,
               4.0f, -6.0f
            }}},
            TRUE
        },
        {
            {{{
               2.0f, 1.0f,
               3.0f, 1.0f,
               4.0f, 8.0f
            }}},
            {{{
              -1.0f,  1.0f,
               3.0f, -2.0f,
             -20.0f, 12.0f
            }}},
            TRUE
        },
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(invert_tests); ++i)
    {
        D2D1_MATRIX_3X2_F m;
        BOOL ret;

        m = invert_tests[i].matrix;
        ret = D2D1InvertMatrix(&m);
        ok(ret == invert_tests[i].invertible, "%u: unexpected return value %d.\n", i, ret);
        ok(!memcmp(&m, &invert_tests[i].inverse, sizeof(m)),
                "%u: unexpected matrix value {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n", i,
                m._11, m._12, m._21, m._22, m._31, m._32);

        ret = D2D1IsMatrixInvertible(&invert_tests[i].matrix);
        ok(ret == invert_tests[i].invertible, "%u: unexpected return value %d.\n", i, ret);
    }
}

static void test_skew_matrix(BOOL d3d11)
{
    static const struct
    {
        float angle_x;
        float angle_y;
        D2D1_POINT_2F center;
        D2D1_MATRIX_3X2_F matrix;
    }
    skew_tests[] =
    {
        { 0.0f, 0.0f, { 0.0f, 0.0f }, {{{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }}} },
        { 45.0f, 0.0f, { 0.0f, 0.0f }, {{{ 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f }}} },
        { 0.0f, 0.0f, { 10.0f, -3.0f }, {{{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }}} },
        { -45.0f, 45.0f, { 0.1f, 0.5f }, {{{ 1.0f, 1.0f, -1.0f, 1.0f, 0.5f, -0.1f }}} },
        { -45.0f, 45.0f, { 1.0f, 2.0f }, {{{ 1.0f, 1.0f, -1.0f, 1.0f, 2.0f, -1.0f }}} },
        { 45.0f, -45.0f, { 1.0f, 2.0f }, {{{ 1.0f, -1.0f, 1.0f, 1.0f, -2.0f, 1.0f }}} },
        { 30.0f, -60.0f, { 12.0f, -5.0f }, {{{ 1.0f, -1.7320509f, 0.577350259f, 1.0f, 2.88675117f, 20.7846107f }}} },
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(skew_tests); ++i)
    {
        const D2D1_MATRIX_3X2_F *expected = &skew_tests[i].matrix;
        D2D1_MATRIX_3X2_F m;
        BOOL ret;

        D2D1MakeSkewMatrix(skew_tests[i].angle_x, skew_tests[i].angle_y, skew_tests[i].center, &m);
        ret = compare_float(m._11, expected->_11, 3) && compare_float(m._12, expected->_12, 3)
                && compare_float(m._21, expected->_21, 3) && compare_float(m._22, expected->_22, 3)
                && compare_float(m._31, expected->_31, 3) && compare_float(m._32, expected->_32, 3);

        ok(ret, "%u: unexpected matrix value {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}, expected "
                "{%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n", i, m._11, m._12, m._21, m._22, m._31, m._32,
                expected->_11, expected->_12, expected->_21, expected->_22, expected->_31, expected->_32);
    }
}

static ID2D1DeviceContext *create_device_context(ID2D1Factory1 *factory, IDXGIDevice *dxgi_device, BOOL d3d11)
{
    ID2D1DeviceContext *device_context;
    ID2D1Device *device;
    HRESULT hr;

    hr = ID2D1Factory1_CreateDevice(factory, dxgi_device, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1Device_Release(device);

    return device_context;
}

static void test_command_list(BOOL d3d11)
{
    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0xffffff00, 0xff00ff00, 0xff00ffff,
    };
    static const D2D1_GRADIENT_STOP stops[] =
    {
        {0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.5f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radial_gradient_properties;
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES linear_gradient_properties;
    ID2D1DeviceContext *device_context, *device_context2;
    D2D1_STROKE_STYLE_PROPERTIES stroke_desc;
    ID2D1GradientStopCollection *gradient;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1StrokeStyle *stroke_style;
    ID2D1CommandList *command_list;
    struct d2d1_test_context ctx;
    D2D1_PIXEL_FORMAT format;
    ID2D1Geometry *geometry;
    ID2D1RenderTarget *rt;
    D2D1_POINT_2F p0, p1;
    ID2D1Bitmap *bitmap;
    ID2D1Image *target;
    D2D1_COLOR_F color;
    ID2D1Brush *brush;
    D2D1_RECT_F rect;
    D2D_SIZE_U size;
    ULONG refcount;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (!ctx.factory1)
    {
        win_skip("Command lists are not supported.\n");
        release_test_context(&ctx);
        return;
    }

    device_context = ctx.context;

    hr = ID2D1DeviceContext_CreateCommandList(device_context, &command_list);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)command_list);

    size = ID2D1DeviceContext_GetPixelSize(device_context);
    ok(!size.width, "Got unexpected width %u.\n", size.width);
    ok(!size.height, "Got unexpected height %u.\n", size.height);

    format = ID2D1DeviceContext_GetPixelFormat(device_context);
    ok(format.format == DXGI_FORMAT_UNKNOWN && format.alphaMode == D2D1_ALPHA_MODE_UNKNOWN,
            "Unexpected format %u, alpha mode %u.\n", format.format, format.alphaMode);

    ID2D1DeviceContext_BeginDraw(device_context);

    hr = ID2D1DeviceContext_QueryInterface(device_context, &IID_ID2D1RenderTarget, (void **)&rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test how resources are referenced by the list. */

    /* Bitmap. */
    set_size_u(&size, 4, 1);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_DrawBitmap(rt, bitmap, NULL, 0.25f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    /* Solid color brush. */
    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, (ID2D1SolidColorBrush **)&brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 16.0f, 16.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, brush);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %lu.\n", refcount);

    /* Bitmap brush. */
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, (ID2D1BitmapBrush **)&brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 16.0f, 16.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, brush);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %lu.\n", refcount);

    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    /* Linear gradient brush. */
    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&linear_gradient_properties.startPoint, 320.0f, 0.0f);
    set_point(&linear_gradient_properties.endPoint, 0.0f, 960.0f);
    hr = ID2D1RenderTarget_CreateLinearGradientBrush(rt, &linear_gradient_properties, NULL, gradient,
            (ID2D1LinearGradientBrush **)&brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory1_CreateRectangleGeometry(ctx.factory1, &rect, (ID2D1RectangleGeometry **)&geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_FillGeometry(rt, geometry, brush, NULL);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %lu.\n", refcount);

    refcount = ID2D1Geometry_Release(geometry);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    /* Radial gradient brush. */
    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&radial_gradient_properties.center, 160.0f, 480.0f);
    set_point(&radial_gradient_properties.gradientOriginOffset, 40.0f, -120.0f);
    radial_gradient_properties.radiusX = 160.0f;
    radial_gradient_properties.radiusY = 480.0f;
    hr = ID2D1RenderTarget_CreateRadialGradientBrush(rt, &radial_gradient_properties, NULL, gradient,
            (ID2D1RadialGradientBrush **)&brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory1_CreateRectangleGeometry(ctx.factory1, &rect, (ID2D1RectangleGeometry **)&geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_FillGeometry(rt, geometry, brush, NULL);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %lu.\n", refcount);

    refcount = ID2D1Geometry_Release(geometry);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    /* Geometry. */
    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory1_CreateRectangleGeometry(ctx.factory1, &rect, (ID2D1RectangleGeometry **)&geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, (ID2D1SolidColorBrush **)&brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1RenderTarget_FillGeometry(rt, geometry, brush, NULL);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %lu.\n", refcount);

    refcount = ID2D1Geometry_Release(geometry);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    /* Stroke style. */
    stroke_desc.startCap = D2D1_CAP_STYLE_SQUARE;
    stroke_desc.endCap = D2D1_CAP_STYLE_ROUND;
    stroke_desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    stroke_desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    stroke_desc.miterLimit = 1.5f;
    stroke_desc.dashStyle = D2D1_DASH_STYLE_DOT;
    stroke_desc.dashOffset = -1.0f;

    hr = ID2D1Factory_CreateStrokeStyle(ctx.factory, &stroke_desc, NULL, 0, &stroke_style);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, (ID2D1SolidColorBrush **)&brush);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_point(&p0, 100.0f, 160.0f);
    set_point(&p1, 140.0f, 160.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, brush, 1.0f, stroke_style);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %lu.\n", refcount);

    refcount = ID2D1StrokeStyle_Release(stroke_style);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    /* Close on attached list. */
    ID2D1DeviceContext_GetTarget(device_context, &target);
    ok(target == (ID2D1Image *)command_list, "Unexpected context target.\n");
    ID2D1Image_Release(target);

    hr = ID2D1CommandList_Close(command_list);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DeviceContext_GetTarget(device_context, &target);
    todo_wine
    ok(target == NULL, "Unexpected context target.\n");
    if (target) ID2D1Image_Release(target);

    hr = ID2D1CommandList_Close(command_list);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);

    ID2D1CommandList_Release(command_list);

    /* Close empty list. */
    hr = ID2D1DeviceContext_CreateCommandList(device_context, &command_list);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1CommandList_Close(command_list);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1CommandList_Release(command_list);

    /* List created with different context. */
    device_context2 = create_device_context(ctx.factory1, ctx.device, d3d11);
    ok(device_context2 != NULL, "Failed to create device context.\n");

    hr = ID2D1DeviceContext_CreateCommandList(device_context, &command_list);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1DeviceContext_SetTarget(device_context2, (ID2D1Image *)command_list);
    ID2D1DeviceContext_GetTarget(device_context2, &target);
    todo_wine
    ok(target == NULL, "Unexpected target.\n");
    if (target) ID2D1Image_Release(target);

    ID2D1CommandList_Release(command_list);
    ID2D1DeviceContext_Release(device_context2);

    ID2D1RenderTarget_Release(rt);
    release_test_context(&ctx);
}

static void test_max_bitmap_size(BOOL d3d11)
{
    D2D1_RENDER_TARGET_PROPERTIES desc;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    struct d2d1_test_context ctx;
    IDXGISwapChain *swapchain;
    IDXGISurface *surface;
    ID2D1RenderTarget *rt;
    ID3D10Device1 *device;
    ID2D1Bitmap *bitmap;
    UINT32 bitmap_size;
    unsigned int i, j;
    HWND window;
    HRESULT hr;

    static const struct
    {
        const char *name;
        DWORD type;
    }
    device_types[] =
    {
        { "HW",   D3D10_DRIVER_TYPE_HARDWARE },
        { "WARP", D3D10_DRIVER_TYPE_WARP },
        { "REF",  D3D10_DRIVER_TYPE_REFERENCE },
    };
    static const struct
    {
        const char *name;
        DWORD type;
    }
    target_types[] =
    {
        { "DEFAULT", D2D1_RENDER_TARGET_TYPE_DEFAULT },
        { "HW",      D2D1_RENDER_TARGET_TYPE_HARDWARE },
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    for (i = 0; i < ARRAY_SIZE(device_types); ++i)
    {
        if (FAILED(hr = D3D10CreateDevice1(NULL, device_types[i].type, NULL, D3D10_CREATE_DEVICE_BGRA_SUPPORT,
                D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        {
            skip("Failed to create %s d3d device, hr %#lx.\n", device_types[i].name, hr);
            continue;
        }

        window = create_window();
        swapchain = create_d3d10_swapchain(device, window, TRUE);
        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        for (j = 0; j < ARRAY_SIZE(target_types); ++j)
        {
            D3D10_TEXTURE2D_DESC texture_desc;
            ID3D10Texture2D *texture;
            D2D1_SIZE_U size;

            desc.type = target_types[j].type;
            desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
            desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
            desc.dpiX = 0.0f;
            desc.dpiY = 0.0f;
            desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
            desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

            hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(ctx.factory, surface, &desc, &rt);
            ok(hr == S_OK, "%s/%s: Got unexpected hr %#lx.\n", device_types[i].name, target_types[j].name, hr);

            bitmap_size = ID2D1RenderTarget_GetMaximumBitmapSize(rt);
            ok(bitmap_size >= D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION, "%s/%s: unexpected bitmap size %u.\n",
                    device_types[i].name, target_types[j].name, bitmap_size);

            bitmap_desc.dpiX = 96.0f;
            bitmap_desc.dpiY = 96.0f;
            bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
            bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

            size.width = bitmap_size;
            size.height = 1;
            hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ID2D1Bitmap_Release(bitmap);

            ID2D1RenderTarget_Release(rt);

            texture_desc.Width = bitmap_size;
            texture_desc.Height = 1;
            texture_desc.MipLevels = 1;
            texture_desc.ArraySize = 1;
            texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            texture_desc.SampleDesc.Count = 1;
            texture_desc.SampleDesc.Quality = 0;
            texture_desc.Usage = D3D10_USAGE_DEFAULT;
            texture_desc.BindFlags = 0;
            texture_desc.CPUAccessFlags = 0;
            texture_desc.MiscFlags = 0;

            hr = ID3D10Device1_CreateTexture2D(device, &texture_desc, NULL, &texture);
            ok(hr == S_OK || broken(hr == E_INVALIDARG && device_types[i].type == D3D10_DRIVER_TYPE_WARP) /* Vista */,
                    "%s/%s: Got unexpected hr %#lx.\n", device_types[i].name, target_types[j].name, hr);
            if (SUCCEEDED(hr))
                ID3D10Texture2D_Release(texture);
        }

        IDXGISurface_Release(surface);
        IDXGISwapChain_Release(swapchain);
        DestroyWindow(window);

        ID3D10Device1_Release(device);
    }

    release_test_context(&ctx);
}

static void test_dpi(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    ID2D1DeviceContext *device_context;
    IWICImagingFactory *wic_factory;
    struct d2d1_test_context ctx;
    ID2D1Bitmap1 *bitmap;
    float dpi_x, dpi_y;
    HRESULT hr;

    static const struct
    {
        float dpi_x, dpi_y;
        HRESULT hr;
    }
    create_dpi_tests[] =
    {
        {  0.0f,   0.0f, S_OK},
        {192.0f,   0.0f, E_INVALIDARG},
        {  0.0f, 192.0f, E_INVALIDARG},
        {192.0f, -10.0f, E_INVALIDARG},
        {-10.0f, 192.0f, E_INVALIDARG},
        {-10.0f, -10.0f, E_INVALIDARG},
        { 48.0f,  96.0f, S_OK},
        { 96.0f,  48.0f, S_OK},
    };
    static const float init_dpi_x = 60.0f, init_dpi_y = 288.0f;
    static const float dc_dpi_x = 120.0f, dc_dpi_y = 144.0f;
    unsigned int i;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (!ctx.factory1)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    device_context = create_device_context(ctx.factory1, ctx.device, d3d11);
    ok(!!device_context, "Failed to create device context.\n");

    ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 96.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    /* DXGI surface */
    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        ID2D1DeviceContext_SetDpi(device_context, init_dpi_x, init_dpi_y);

        bitmap_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_desc.dpiX = create_dpi_tests[i].dpi_x;
        bitmap_desc.dpiY = create_dpi_tests[i].dpi_y;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, ctx.surface, &bitmap_desc, &bitmap);
        /* Native accepts negative DPI values for DXGI surface bitmap. */
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        ID2D1DeviceContext_SetDpi(device_context, dc_dpi_x, dc_dpi_y);

        ID2D1Bitmap1_GetDpi(bitmap, &dpi_x, &dpi_y);
        todo_wine_if(bitmap_desc.dpiX == 0.0f)
            ok(dpi_x == bitmap_desc.dpiX, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n",
                    i, dpi_x, bitmap_desc.dpiX);
        todo_wine_if(bitmap_desc.dpiY == 0.0f)
            ok(dpi_y == bitmap_desc.dpiY, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n",
                    i, dpi_y, bitmap_desc.dpiY);

        ID2D1DeviceContext_BeginDraw(device_context);
        ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
        hr = ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        /* Device context DPI values aren't updated by SetTarget. */
        ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
        ok(dpi_x == dc_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, dc_dpi_x);
        ok(dpi_y == dc_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, dc_dpi_y);

        ID2D1Bitmap1_Release(bitmap);
    }

    /* WIC bitmap */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        IWICBitmapSource *wic_bitmap_src;
        IWICBitmap *wic_bitmap;

        ID2D1DeviceContext_SetDpi(device_context, init_dpi_x, init_dpi_y);

        hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
                &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IWICBitmap_QueryInterface(wic_bitmap, &IID_IWICBitmapSource, (void **)&wic_bitmap_src);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        IWICBitmap_Release(wic_bitmap);

        bitmap_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_desc.dpiX = create_dpi_tests[i].dpi_x;
        bitmap_desc.dpiY = create_dpi_tests[i].dpi_y;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmapFromWicBitmap(device_context, wic_bitmap_src, &bitmap_desc, &bitmap);
        ok(hr == create_dpi_tests[i].hr, "Test %u: Got unexpected hr %#lx, expected %#lx.\n",
                i, hr, create_dpi_tests[i].hr);
        IWICBitmapSource_Release(wic_bitmap_src);

        if (FAILED(hr))
            continue;

        ID2D1DeviceContext_SetDpi(device_context, dc_dpi_x, dc_dpi_y);

        ID2D1Bitmap1_GetDpi(bitmap, &dpi_x, &dpi_y);
        if (bitmap_desc.dpiX == 0.0f && bitmap_desc.dpiY == 0.0f)
        {
            /* Bitmap DPI values are inherited at creation time. */
            ok(dpi_x == init_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, init_dpi_x);
            ok(dpi_y == init_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, init_dpi_y);
        }
        else
        {
            ok(dpi_x == bitmap_desc.dpiX, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n",
                    i, dpi_x, bitmap_desc.dpiX);
            ok(dpi_y == bitmap_desc.dpiY, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n",
                    i, dpi_y, bitmap_desc.dpiY);
        }

        ID2D1DeviceContext_BeginDraw(device_context);
        ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
        hr = ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        /* Device context DPI values aren't updated by SetTarget. */
        ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
        ok(dpi_x == dc_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, dc_dpi_x);
        ok(dpi_y == dc_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, dc_dpi_y);

        ID2D1Bitmap1_Release(bitmap);
    }
    IWICImagingFactory_Release(wic_factory);
    CoUninitialize();

    /* D2D bitmap */
    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        const D2D1_SIZE_U size = {16, 16};

        ID2D1DeviceContext_SetDpi(device_context, init_dpi_x, init_dpi_y);

        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_desc.dpiX = create_dpi_tests[i].dpi_x;
        bitmap_desc.dpiY = create_dpi_tests[i].dpi_y;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
        ok(hr == create_dpi_tests[i].hr, "Test %u: Got unexpected hr %#lx, expected %#lx.\n",
                i, hr, create_dpi_tests[i].hr);

        if (FAILED(hr))
            continue;

        ID2D1DeviceContext_SetDpi(device_context, dc_dpi_x, dc_dpi_y);

        ID2D1Bitmap1_GetDpi(bitmap, &dpi_x, &dpi_y);
        if (bitmap_desc.dpiX == 0.0f && bitmap_desc.dpiY == 0.0f)
        {
            /* Bitmap DPI values are inherited at creation time. */
            ok(dpi_x == init_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, init_dpi_x);
            ok(dpi_y == init_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, init_dpi_y);
        }
        else
        {
            ok(dpi_x == bitmap_desc.dpiX, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n",
                    i, dpi_x, bitmap_desc.dpiX);
            ok(dpi_y == bitmap_desc.dpiY, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n",
                    i, dpi_y, bitmap_desc.dpiY);
        }

        ID2D1DeviceContext_BeginDraw(device_context);
        ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
        hr = ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        /* Device context DPI values aren't updated by SetTarget. */
        ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
        ok(dpi_x == dc_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, dc_dpi_x);
        ok(dpi_y == dc_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, dc_dpi_y);

        ID2D1Bitmap1_Release(bitmap);
    }

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
    ok(dpi_x == dc_dpi_x, "Got unexpected dpi_x %.8e, expected %.8e.\n", dpi_x, dc_dpi_x);
    ok(dpi_y == dc_dpi_y, "Got unexpected dpi_y %.8e, expected %.8e.\n", dpi_y, dc_dpi_y);

    ID2D1DeviceContext_Release(device_context);
    release_test_context(&ctx);
}

static void test_unit_mode(BOOL d3d11)
{
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    D2D1_UNIT_MODE unit_mode;

    if (!init_test_context(&ctx, d3d11))
        return;

    context = ctx.context;

    unit_mode = ID2D1DeviceContext_GetUnitMode(context);
    ok(unit_mode == D2D1_UNIT_MODE_DIPS, "Got unexpected unit mode %#x.\n", unit_mode);

    ID2D1DeviceContext_SetUnitMode(context, 0xdeadbeef);
    unit_mode = ID2D1DeviceContext_GetUnitMode(context);
    ok(unit_mode == D2D1_UNIT_MODE_DIPS, "Got unexpected unit mode %#x.\n", unit_mode);

    ID2D1DeviceContext_SetUnitMode(context, D2D1_UNIT_MODE_PIXELS);
    unit_mode = ID2D1DeviceContext_GetUnitMode(context);
    ok(unit_mode == D2D1_UNIT_MODE_PIXELS, "Got unexpected unit mode %#x.\n", unit_mode);

    ID2D1DeviceContext_SetUnitMode(context, 0xdeadbeef);
    unit_mode = ID2D1DeviceContext_GetUnitMode(context);
    ok(unit_mode == D2D1_UNIT_MODE_PIXELS, "Got unexpected unit mode %#x.\n", unit_mode);

    release_test_context(&ctx);
}

static void test_wic_bitmap_format(BOOL d3d11)
{
    IWICImagingFactory *wic_factory;
    struct d2d1_test_context ctx;
    D2D1_PIXEL_FORMAT format;
    IWICBitmap *wic_bitmap;
    ID2D1RenderTarget *rt;
    ID2D1Bitmap *bitmap;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        const WICPixelFormatGUID *wic;
        D2D1_PIXEL_FORMAT d2d;
    }
    tests[] =
    {
        {&GUID_WICPixelFormat32bppPBGRA, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}},
        {&GUID_WICPixelFormat32bppPRGBA, {DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}},
        {&GUID_WICPixelFormat32bppBGR,   {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
        {&GUID_WICPixelFormat32bppRGB,   {DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
                tests[i].wic, WICBitmapCacheOnDemand, &wic_bitmap);
        ok(hr == S_OK, "%s: Got unexpected hr %#lx.\n", debugstr_guid(tests[i].wic), hr);

        hr = ID2D1RenderTarget_CreateBitmapFromWicBitmap(rt, (IWICBitmapSource *)wic_bitmap, NULL, &bitmap);
        ok(hr == S_OK, "%s: Got unexpected hr %#lx.\n", debugstr_guid(tests[i].wic), hr);

        format = ID2D1Bitmap_GetPixelFormat(bitmap);
        ok(format.format == tests[i].d2d.format, "%s: Got unexpected DXGI format %#x.\n",
                debugstr_guid(tests[i].wic), format.format);
        ok(format.alphaMode == tests[i].d2d.alphaMode, "%s: Got unexpected alpha mode %#x.\n",
                debugstr_guid(tests[i].wic), format.alphaMode);

        ID2D1Bitmap_Release(bitmap);
        IWICBitmap_Release(wic_bitmap);
    }

    IWICImagingFactory_Release(wic_factory);
    CoUninitialize();
    release_test_context(&ctx);
}

static void test_math(BOOL d3d11)
{
    float s, c, t, l;
    unsigned int i;

    static const struct
    {
        float x;
        float s;
        float c;
    }
    sc_data[] =
    {
        {0.0f,         0.0f,              1.0f},
        {1.0f,         8.41470957e-001f,  5.40302277e-001f},
        {2.0f,         9.09297407e-001f, -4.16146845e-001f},
        {M_PI / 2.0f,  1.0f,             -4.37113883e-008f},
        {M_PI,        -8.74227766e-008f, -1.0f},
    };

    static const struct
    {
        float x;
        float t;
    }
    t_data[] =
    {
        {0.0f,         0.0f},
        {1.0f,         1.55740774f},
        {2.0f,        -2.18503976f},
        {M_PI / 2.0f, -2.28773320e+007f},
        {M_PI,         8.74227766e-008f},
    };

    static const struct
    {
        float x;
        float y;
        float z;
        float l;
    }
    l_data[] =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.73205078f},
        {1.0f, 2.0f, 2.0f, 3.0f},
        {1.0f, 2.0f, 3.0f, 3.74165750f},
    };

    if (!pD2D1SinCos || !pD2D1Tan || !pD2D1Vec3Length)
    {
        win_skip("D2D1SinCos/D2D1Tan/D2D1Vec3Length not available, skipping test.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(sc_data); ++i)
    {
        pD2D1SinCos(sc_data[i].x, &s, &c);
        ok(compare_float(s, sc_data[i].s, 0),
                "Test %u: Got unexpected sin %.8e, expected %.8e.\n", i, s, sc_data[i].s);
        ok(compare_float(c, sc_data[i].c, 0),
                "Test %u: Got unexpected cos %.8e, expected %.8e.\n", i, c, sc_data[i].c);
    }

    for (i = 0; i < ARRAY_SIZE(t_data); ++i)
    {
        t = pD2D1Tan(t_data[i].x);
        ok(compare_float(t, t_data[i].t, 1),
                "Test %u: Got unexpected tan %.8e, expected %.8e.\n", i, t, t_data[i].t);
    }

    for (i = 0; i < ARRAY_SIZE(l_data); ++i)
    {
        l = pD2D1Vec3Length(l_data[i].x, l_data[i].y, l_data[i].z);
        ok(compare_float(l, l_data[i].l, 0),
                "Test %u: Got unexpected length %.8e, expected %.8e.\n", i, l, l_data[i].l);
    }
}

static void test_colour_space(BOOL d3d11)
{
    D2D1_COLOR_F src_colour, dst_colour, expected;
    D2D1_COLOR_SPACE src_space, dst_space;
    unsigned i, j, k;

    static const D2D1_COLOR_SPACE colour_spaces[] =
    {
        D2D1_COLOR_SPACE_CUSTOM,
        D2D1_COLOR_SPACE_SRGB,
        D2D1_COLOR_SPACE_SCRGB,
    };
    static struct
    {
        D2D1_COLOR_F srgb;
        D2D1_COLOR_F scrgb;
    }
    const test_data[] =
    {
        {{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        /* Samples in the non-linear region. */
        {{0.2f, 0.4f, 0.6f, 0.8f}, {0.0331047624f, 0.132868335f, 0.318546832f, 0.8f}},
        {{0.3f, 0.5f, 0.7f, 0.9f}, {0.0732389688f, 0.214041144f, 0.447988421f, 0.9f}},
        /* Samples in the linear region. */
        {{0.0002f, 0.0004f, 0.0006f, 0.0008f}, {1.54798763e-005f, 3.09597526e-005f, 4.64396289e-005f, 0.0008f}},
        {{0.0003f, 0.0005f, 0.0007f, 0.0009f}, {2.32198145e-005f, 3.86996908e-005f, 5.41795634e-005f, 0.0009f}},
        /* Out of range samples */
        {{-0.3f,  1.5f, -0.7f,  2.0f}, { 0.0f,  1.0f,  0.0f,  2.0f}},
        {{ 1.5f, -0.3f,  2.0f, -0.7f}, { 1.0f,  0.0f,  1.0f, -0.7f}},
        {{ 0.0f,  1.0f,  0.0f,  1.5f}, {-0.7f,  2.0f, -0.3f,  1.5f}},
        {{ 1.0f,  0.0f,  1.0f, -0.3f}, { 2.0f, -0.7f,  1.5f, -0.3f}},
    };

    if (!pD2D1ConvertColorSpace)
    {
        win_skip("D2D1ConvertColorSpace() not available, skipping test.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(colour_spaces); ++i)
    {
        src_space = colour_spaces[i];
        for (j = 0; j < ARRAY_SIZE(colour_spaces); ++j)
        {
            dst_space = colour_spaces[j];
            for (k = 0; k < ARRAY_SIZE(test_data); ++k)
            {
                if (src_space == D2D1_COLOR_SPACE_SCRGB)
                    src_colour = test_data[k].scrgb;
                else
                    src_colour = test_data[k].srgb;

                if (dst_space == D2D1_COLOR_SPACE_SCRGB)
                    expected = test_data[k].scrgb;
                else
                    expected = test_data[k].srgb;

                if (src_space == D2D1_COLOR_SPACE_CUSTOM || dst_space == D2D1_COLOR_SPACE_CUSTOM)
                {
                    set_color(&expected, 0.0f, 0.0f, 0.0f, 0.0f);
                }
                else if (src_space != dst_space)
                {
                    expected.r = clamp_float(expected.r, 0.0f, 1.0f);
                    expected.g = clamp_float(expected.g, 0.0f, 1.0f);
                    expected.b = clamp_float(expected.b, 0.0f, 1.0f);
                }

                dst_colour = pD2D1ConvertColorSpace(src_space, dst_space, &src_colour);
                ok(compare_colour_f(&dst_colour, expected.r, expected.g, expected.b, expected.a, 1),
                        "Got unexpected destination colour {%.8e, %.8e, %.8e, %.8e}, "
                        "expected destination colour {%.8e, %.8e, %.8e, %.8e} for "
                        "source colour {%.8e, %.8e, %.8e, %.8e}, "
                        "source colour space %#x, destination colour space %#x.\n",
                        dst_colour.r, dst_colour.g, dst_colour.b, dst_colour.a,
                        expected.r, expected.g, expected.b, expected.a,
                        src_colour.r, src_colour.g, src_colour.b, src_colour.a,
                        src_space, dst_space);
            }
        }
    }
}

static void test_geometry_group(BOOL d3d11)
{
    struct d2d1_test_context ctx;
    ID2D1Geometry *geometries[2];
    ID2D1GeometryGroup *group;
    D2D1_MATRIX_3X2_F matrix;
    D2D1_RECT_F rect;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, (ID2D1RectangleGeometry **)&geometries[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, -2.0f, -2.0f, 0.0f, 2.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, (ID2D1RectangleGeometry **)&geometries[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_CreateGeometryGroup(ctx.factory, D2D1_FILL_MODE_ALTERNATE, geometries, 2, &group);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1GeometryGroup_GetBounds(group, NULL, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, -2.0f, -2.0f, 1.0f, 2.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1GeometryGroup_GetBounds(group, &matrix, &rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    match = compare_rect(&rect, 76.0f, 639.0f, 82.0f, 641.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1GeometryGroup_Release(group);

    ID2D1Geometry_Release(geometries[0]);
    ID2D1Geometry_Release(geometries[1]);

    release_test_context(&ctx);
}

static DWORD WINAPI mt_factory_test_thread_func(void *param)
{
    ID2D1Multithread *multithread = param;

    ID2D1Multithread_Enter(multithread);

    return 0;
}

static DWORD WINAPI mt_factory_test_thread_draw_func(void *param)
{
    ID2D1RenderTarget *rt = param;
    D2D1_COLOR_F color;
    HRESULT hr;

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    return 0;
}

static void test_mt_factory(BOOL d3d11)
{
    struct d2d1_test_context ctx;
    ID2D1Multithread *multithread;
    ID2D1Factory *factory;
    HANDLE thread;
    HRESULT hr;
    DWORD ret;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED + 1, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_QueryInterface(factory, &IID_ID2D1Multithread, (void **)&multithread);
    if (hr == E_NOINTERFACE)
    {
        win_skip("ID2D1Multithread is not supported.\n");
        ID2D1Factory_Release(factory);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = ID2D1Multithread_GetMultithreadProtected(multithread);
    ok(!ret, "Unexpected return value.\n");

    ID2D1Multithread_Enter(multithread);
    thread = CreateThread(NULL, 0, mt_factory_test_thread_func, multithread, 0, NULL);
    ok(!!thread, "Failed to create a thread.\n");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ID2D1Multithread_Release(multithread);
    ID2D1Factory_Release(factory);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory_QueryInterface(factory, &IID_ID2D1Multithread, (void **)&multithread);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = ID2D1Multithread_GetMultithreadProtected(multithread);
    ok(!!ret, "Unexpected return value.\n");

    ID2D1Multithread_Enter(multithread);
    thread = CreateThread(NULL, 0, mt_factory_test_thread_func, multithread, 0, NULL);
    ok(!!thread, "Failed to create a thread.\n");
    ret = WaitForSingleObject(thread, 10);
    ok(ret == WAIT_TIMEOUT, "Expected timeout.\n");
    ID2D1Multithread_Leave(multithread);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ID2D1Multithread_Release(multithread);

    ID2D1Factory_Release(factory);

    if (!init_test_multithreaded_context(&ctx, d3d11))
        return;

    hr = ID2D1Factory_QueryInterface(ctx.factory, &IID_ID2D1Multithread, (void **)&multithread);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1Multithread_Enter(multithread);
    thread = CreateThread(NULL, 0, mt_factory_test_thread_draw_func, ctx.rt, 0, NULL);
    ok(!!thread, "Failed to create a thread.\n");
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_TIMEOUT, "Expected timeout.\n");
    ID2D1Multithread_Leave(multithread);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ID2D1Multithread_Release(multithread);
    release_test_context(&ctx);

    if (!init_test_context(&ctx, d3d11))
        return;

    hr = ID2D1Factory_QueryInterface(ctx.factory, &IID_ID2D1Multithread, (void **)&multithread);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1Multithread_Enter(multithread);
    thread = CreateThread(NULL, 0, mt_factory_test_thread_draw_func, ctx.rt, 0, NULL);
    ok(!!thread, "Failed to create a thread.\n");
    ret = WaitForSingleObject(thread, 5000);
    ok(ret == WAIT_OBJECT_0, "Didn't expect timeout.\n");
    ID2D1Multithread_Leave(multithread);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ID2D1Multithread_Release(multithread);
    release_test_context(&ctx);
}

#define check_system_properties(effect) check_system_properties_(__LINE__, effect)
static void check_system_properties_(unsigned int line, ID2D1Effect *effect)
{
    UINT i, value_size, str_size;
    WCHAR name[32], buffer[256];
    D2D1_PROPERTY_TYPE type;
    UINT32 count;
    HRESULT hr;

    static const struct system_property_test
    {
        UINT32 index;
        const WCHAR *name;
        D2D1_PROPERTY_TYPE type;
        UINT32 value_size;
    }
    system_property_tests[] =
    {
        {D2D1_PROPERTY_CLSID,       L"CLSID",       D2D1_PROPERTY_TYPE_CLSID,  sizeof(CLSID)},
        {D2D1_PROPERTY_DISPLAYNAME, L"DisplayName", D2D1_PROPERTY_TYPE_STRING },
        {D2D1_PROPERTY_AUTHOR,      L"Author",      D2D1_PROPERTY_TYPE_STRING },
        {D2D1_PROPERTY_CATEGORY,    L"Category",    D2D1_PROPERTY_TYPE_STRING },
        {D2D1_PROPERTY_DESCRIPTION, L"Description", D2D1_PROPERTY_TYPE_STRING },
        {D2D1_PROPERTY_INPUTS,      L"Inputs",      D2D1_PROPERTY_TYPE_ARRAY,  sizeof(UINT32)},
        {D2D1_PROPERTY_CACHED,      L"Cached",      D2D1_PROPERTY_TYPE_BOOL,   sizeof(BOOL)},
        {D2D1_PROPERTY_PRECISION,   L"Precision",   D2D1_PROPERTY_TYPE_ENUM,   sizeof(UINT32)},
        {D2D1_PROPERTY_MIN_INPUTS,  L"MinInputs",   D2D1_PROPERTY_TYPE_UINT32, sizeof(UINT32)},
        {D2D1_PROPERTY_MAX_INPUTS,  L"MaxInputs",   D2D1_PROPERTY_TYPE_UINT32, sizeof(UINT32)},
    };

    hr = ID2D1Effect_GetPropertyName(effect, 0xdeadbeef, name, sizeof(name));
    ok_(__FILE__, line)(hr == D2DERR_INVALID_PROPERTY, "GetPropertyName() got unexpected hr %#lx for 0xdeadbeef.\n", hr);
    type = ID2D1Effect_GetType(effect, 0xdeadbeef);
    ok_(__FILE__, line)(type == D2D1_PROPERTY_TYPE_UNKNOWN, "Got unexpected property type %#x for 0xdeadbeef.\n", type);
    value_size = ID2D1Effect_GetValueSize(effect, 0xdeadbeef);
    ok_(__FILE__, line)(value_size == 0, "Got unexpected value size %u for 0xdeadbeef.\n", value_size);

    for (i = 0; i < ARRAY_SIZE(system_property_tests); ++i)
    {
        const struct system_property_test *test = &system_property_tests[i];
        winetest_push_context("Property %u", i);

        name[0] = 0;
        hr = ID2D1Effect_GetPropertyName(effect, test->index, name, ARRAY_SIZE(name));
        ok_(__FILE__, line)(hr == S_OK, "Failed to get property name, hr %#lx\n", hr);
        if (hr == D2DERR_INVALID_PROPERTY)
        {
            winetest_pop_context();
            continue;
        }
        ok_(__FILE__, line)(!wcscmp(name, test->name), "Got unexpected property name %s, expected %s.\n",
                debugstr_w(name), debugstr_w(test->name));
        count = ID2D1Effect_GetPropertyNameLength(effect, test->index);
        ok_(__FILE__, line)(wcslen(name) == count, "Got unexpected property name length %u.\n", count);

        name[0] = 0;
        hr = ID2D1Effect_GetPropertyName(effect, test->index, name, count);
        ok_(__FILE__, line)(hr == D2DERR_INSUFFICIENT_BUFFER, "Failed to get property name, hr %#lx\n", hr);

        name[0] = 0;
        hr = ID2D1Effect_GetPropertyName(effect, test->index, name, count + 1);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get property name, hr %#lx\n", hr);

        type = ID2D1Effect_GetType(effect, test->index);
        ok_(__FILE__, line)(type == test->type, "Got unexpected property type %#x, expected %#x.\n",
                type, test->type);

        value_size = 0;
        value_size = ID2D1Effect_GetValueSize(effect, test->index);
        if (test->value_size != 0)
        {
            ok_(__FILE__, line)(value_size == test->value_size, "Got unexpected value size %u, expected %u.\n",
                    value_size, test->value_size);
        }
        else if (test->type == D2D1_PROPERTY_TYPE_STRING)
        {
            hr = ID2D1Effect_GetValue(effect, test->index, D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
            ok_(__FILE__, line)(hr == S_OK, "Failed to get value, hr %#lx.\n", hr);
            str_size = (wcslen(buffer) + 1) * sizeof(WCHAR);
            ok_(__FILE__, line)(value_size == str_size, "Got unexpected value size %u, expected %u.\n",
                    value_size, str_size);
        }
        winetest_pop_context();
    }
}

static void test_builtin_effect(BOOL d3d11)
{
    unsigned int i, j, min_inputs, max_inputs, str_size, input_count;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    D2D1_BUFFER_PRECISION precision;
    ID2D1Effect *effect, *effect2;
    ID2D1Image *image_a, *image_b;
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    ID2D1Factory1 *factory;
    ID2D1Bitmap *bitmap;
    D2D1_SIZE_U size;
    BYTE buffer[256];
    IUnknown *unk;
    BOOL cached;
    CLSID clsid;
    HRESULT hr;

    const struct effect_test
    {
        const CLSID *clsid;
        UINT32 factory_version;
        UINT32 default_input_count;
        UINT32 min_inputs;
        UINT32 max_inputs;
    }
    effect_tests[] =
    {
        {&CLSID_D2D12DAffineTransform,       1, 1, 1, 1},
        {&CLSID_D2D13DPerspectiveTransform,  1, 1, 1, 1},
        {&CLSID_D2D1Composite,               1, 2, 1, 0xffffffff},
        {&CLSID_D2D1Crop,                    1, 1, 1, 1},
        {&CLSID_D2D1Shadow,                  1, 1, 1, 1},
        {&CLSID_D2D1Grayscale,               3, 1, 1, 1},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }
    if (!ctx.factory3)
        win_skip("ID2D1Factory3 is not supported.\n");

    context = ctx.context;

    for (i = 0; i < ARRAY_SIZE(effect_tests); ++i)
    {
        const struct effect_test *test = effect_tests + i;

        if (test->factory_version == 3 && !ctx.factory3)
            continue;

        winetest_push_context("Test %u", i);

        hr = ID2D1DeviceContext_CreateEffect(context, test->clsid, &effect);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        /* Test output image pointer */
        hr = ID2D1Effect_QueryInterface(effect, &IID_ID2D1Image, (void **)&image_a);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID2D1Effect_GetOutput(effect, &image_b);
        ok(image_b == image_a, "Got unexpected image_b %p, expected %p.\n", image_b, image_a);

        hr = ID2D1Image_QueryInterface(image_a, &IID_ID2D1Effect, (void **)&effect2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(effect2 == effect, "Unexpected pointer.\n");
        ID2D1Effect_Release(effect2);

        hr = ID2D1Image_QueryInterface(image_a, &IID_IUnknown, (void **)&unk);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(unk == (IUnknown *)effect, "Unexpected pointer.\n");
        IUnknown_Release(unk);

        ID2D1Image_Release(image_b);
        ID2D1Image_Release(image_a);

        /* Test GetValue() */
        hr = ID2D1Effect_GetValue(effect, 0xdeadbeef, D2D1_PROPERTY_TYPE_CLSID, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == D2DERR_INVALID_PROPERTY, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID, D2D1_PROPERTY_TYPE_CLSID, buffer, sizeof(clsid) + 1);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID, D2D1_PROPERTY_TYPE_CLSID, buffer, sizeof(clsid) - 1);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID, D2D1_PROPERTY_TYPE_CLSID, buffer, sizeof(clsid));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_DISPLAYNAME, D2D1_PROPERTY_TYPE_STRING, buffer, sizeof(buffer));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        str_size = (wcslen((WCHAR *)buffer) + 1) * sizeof(WCHAR);
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_DISPLAYNAME, D2D1_PROPERTY_TYPE_STRING, buffer, str_size);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_DISPLAYNAME, D2D1_PROPERTY_TYPE_STRING, buffer, str_size - 1);
        ok(hr == D2DERR_INSUFFICIENT_BUFFER, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID, 0xdeadbeef, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID, D2D1_PROPERTY_TYPE_UNKNOWN, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID, D2D1_PROPERTY_TYPE_VECTOR4, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID, D2D1_PROPERTY_TYPE_VECTOR4, buffer, sizeof(buffer));
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID,
                D2D1_PROPERTY_TYPE_CLSID, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&clsid, test->clsid), "Got unexpected clsid %s, expected %s.\n",
                debugstr_guid(&clsid), debugstr_guid(test->clsid));

        cached = TRUE;
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CACHED, D2D1_PROPERTY_TYPE_BOOL,
                (BYTE *)&cached, sizeof(cached));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(cached == FALSE, "Got unexpected cached %d.\n", cached);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_PRECISION,
                D2D1_PROPERTY_TYPE_ENUM, (BYTE *)&precision, sizeof(precision));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(precision == D2D1_BUFFER_PRECISION_UNKNOWN, "Got unexpected precision %u.\n", precision);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MIN_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&min_inputs, sizeof(min_inputs));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(min_inputs == test->min_inputs, "Got unexpected min inputs %u, expected %u.\n",
                min_inputs, test->min_inputs);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MAX_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&max_inputs, sizeof(max_inputs));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(max_inputs == test->max_inputs, "Got unexpected max inputs %u, expected %u.\n",
                max_inputs, test->max_inputs);

        /* Test default input count */
        input_count = ID2D1Effect_GetInputCount(effect);
        ok(input_count == test->default_input_count, "Got unexpected input count %u, expected %u.\n",
                input_count, test->default_input_count);

        /* Test SetInputCount() */
        input_count = (test->max_inputs < 16 ? test->max_inputs : 16);
        for (j = 0; j < input_count + 4; ++j)
        {
            winetest_push_context("Input %u", j);
            hr = ID2D1Effect_SetInputCount(effect, j);
            if (j < test->min_inputs || j > test->max_inputs)
                ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
            else
                ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            winetest_pop_context();
        }

        /* Test GetInput() before any input is set */
        input_count = ID2D1Effect_GetInputCount(effect);
        for (j = 0; j < input_count + 4; ++j)
        {
            winetest_push_context("Input %u", j);
            ID2D1Effect_GetInput(effect, j, &image_a);
            ok(image_a == NULL, "Got unexpected image_a %p.\n", image_a);
            winetest_pop_context();
        }

        /* Test GetInput() after an input is set */
        set_size_u(&size, 1, 1);
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        hr = ID2D1RenderTarget_CreateBitmap(ctx.rt, size, NULL, 4, &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID2D1Effect_SetInput(effect, 0, (ID2D1Image *)bitmap, FALSE);
        for (j = 0; j < input_count + 4; ++j)
        {
            winetest_push_context("Input %u", j);
            image_a = (ID2D1Image *)0xdeadbeef;
            ID2D1Effect_GetInput(effect, j, &image_a);
            if (j == 0)
            {
                ok(image_a == (ID2D1Image *)bitmap, "Got unexpected image_a %p.\n", image_a);
                ID2D1Image_Release(image_a);
            }
            else
            {
                ok(image_a == NULL, "Got unexpected image_a %p.\n", image_a);
            }
            winetest_pop_context();
        }

        /* Test setting inputs with out-of-bounds index */
        for (j = input_count; j < input_count + 4; ++j)
        {
            winetest_push_context("Input %u", j);
            image_a = (ID2D1Image *)0xdeadbeef;
            ID2D1Effect_SetInput(effect, j, (ID2D1Image *)bitmap, FALSE);
            ID2D1Effect_GetInput(effect, j, &image_a);
            ok(image_a == NULL, "Got unexpected image_a %p.\n", image_a);
            winetest_pop_context();
        }
        ID2D1Bitmap_Release(bitmap);

        ID2D1Effect_Release(effect);
        winetest_pop_context();
    }

    release_test_context(&ctx);
}

static inline struct effect_impl *impl_from_ID2D1EffectImpl(ID2D1EffectImpl *iface)
{
    return CONTAINING_RECORD(iface, struct effect_impl, ID2D1EffectImpl_iface);
}

static inline struct effect_impl *impl_from_ID2D1DrawTransform(ID2D1DrawTransform *iface)
{
    return CONTAINING_RECORD(iface, struct effect_impl, ID2D1DrawTransform_iface);
}

static HRESULT STDMETHODCALLTYPE effect_impl_QueryInterface(ID2D1EffectImpl *iface, REFIID iid, void **out)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl(iface);

    if (IsEqualGUID(iid, &IID_ID2D1EffectImpl)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1EffectImpl_AddRef(iface);
        *out = iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_ID2D1DrawTransform)
            || IsEqualGUID(iid, &IID_ID2D1Transform)
            || IsEqualGUID(iid, &IID_ID2D1TransformNode))
    {
        ID2D1EffectImpl_AddRef(iface);
        *out = &effect_impl->ID2D1DrawTransform_iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE effect_impl_AddRef(ID2D1EffectImpl *iface)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl(iface);
    ULONG refcount = InterlockedIncrement(&effect_impl->refcount);
    return refcount;
}

static ULONG STDMETHODCALLTYPE effect_impl_Release(ID2D1EffectImpl *iface)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl(iface);
    ULONG refcount = InterlockedDecrement(&effect_impl->refcount);

    if (!refcount)
    {
        if (effect_impl->effect_context)
            ID2D1EffectContext_Release(effect_impl->effect_context);
        if (effect_impl->draw_info)
            ID2D1DrawInfo_Release(effect_impl->draw_info);
        free(effect_impl->blob.data);
        free(effect_impl);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE effect_impl_Initialize(ID2D1EffectImpl *iface,
        ID2D1EffectContext *context,ID2D1TransformGraph *graph)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl(iface);
    ID2D1EffectContext_AddRef(effect_impl->effect_context = context);
    ID2D1TransformGraph_AddRef(effect_impl->transform_graph = graph);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_PrepareForRender(ID2D1EffectImpl *iface, D2D1_CHANGE_TYPE type)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_SetGraph(ID2D1EffectImpl *iface, ID2D1TransformGraph *graph)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl(iface);

    ID2D1TransformGraph_Release(effect_impl->transform_graph);
    ID2D1TransformGraph_AddRef(effect_impl->transform_graph = graph);

    return S_OK;
}

static const ID2D1EffectImplVtbl effect_impl_vtbl =
{
    effect_impl_QueryInterface,
    effect_impl_AddRef,
    effect_impl_Release,
    effect_impl_Initialize,
    effect_impl_PrepareForRender,
    effect_impl_SetGraph,
};

static HRESULT STDMETHODCALLTYPE effect_impl_create(IUnknown **effect_impl)
{
    struct effect_impl *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1EffectImpl_iface.lpVtbl = &effect_impl_vtbl;
    object->refcount = 1;
    object->integer = 10;
    object->effect_context = NULL;
    object->transform_graph = NULL;

    *effect_impl = (IUnknown *)&object->ID2D1EffectImpl_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_set_integer(IUnknown *iface, const BYTE *data, UINT32 data_size)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl((ID2D1EffectImpl *)iface);

    if (!data)
        return E_INVALIDARG;

    effect_impl->integer = *((UINT *)data);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_get_integer(const IUnknown *iface,
        BYTE *data, UINT32 data_size, UINT32 *actual_size)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl((ID2D1EffectImpl *)iface);

    if (!data)
        return E_INVALIDARG;

    *((UINT *)data) = effect_impl->integer;
    if (actual_size)
        *actual_size = sizeof(effect_impl->integer);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_set_blob(IUnknown *iface, const BYTE *data,
        UINT32 data_size)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl((ID2D1EffectImpl *)iface);

    free(effect_impl->blob.data);
    effect_impl->blob.data = NULL;
    effect_impl->blob.size = 0;

    if (!(effect_impl->blob.data = malloc(data_size)))
        return E_OUTOFMEMORY;
    memcpy(effect_impl->blob.data, data, data_size);
    effect_impl->blob.size = data_size;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_get_blob(const IUnknown *iface,
        BYTE *data, UINT32 data_size, UINT32 *actual_size)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl((ID2D1EffectImpl *)iface);

    if (actual_size)
        *actual_size = effect_impl->blob.size;

    if (!data)
        return S_OK;

    if (data_size != effect_impl->blob.size)
        return E_INVALIDARG;

    memcpy(data, effect_impl->blob.data, data_size);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_get_context(const IUnknown *iface,
        BYTE *data, UINT32 data_size, UINT32 *actual_size)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl((ID2D1EffectImpl *)iface);

    if (!data)
        return E_INVALIDARG;

    *((ID2D1EffectContext **)data) = effect_impl->effect_context;
    if (actual_size)
        *actual_size = sizeof(effect_impl->effect_context);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_get_graph(const IUnknown *iface,
        BYTE *data, UINT32 data_size, UINT32 *actual_size)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl((ID2D1EffectImpl *)iface);

    if (!data)
        return E_INVALIDARG;

    *((ID2D1TransformGraph **)data) = effect_impl->transform_graph;
    if (actual_size)
        *actual_size = sizeof(effect_impl->transform_graph);

    return S_OK;
}

static void test_effect_register(BOOL d3d11)
{
    ID2D1DeviceContext *device_context;
    ID2D1EffectContext *effect_context;
    struct d2d1_test_context ctx;
    ID2D1TransformGraph *graph;
    unsigned int i, integer;
    ID2D1Factory1 *factory;
    WCHAR display_name[64];
    ID2D1Effect *effect;
    UINT32 count;
    HRESULT hr;

    const struct xml_test
    {
        const WCHAR *xml;
        HRESULT hr;
    }
    xml_tests[] =
    {
        {effect_xml_a,       S_OK},
        {effect_xml_b,       S_OK},
        {effect_xml_c,       S_OK},
        {effect_xml_minimum, S_OK},
        {effect_xml_without_version,     HRESULT_FROM_WIN32(ERROR_NOT_FOUND)},
        {effect_xml_without_inputs,      E_INVALIDARG},
        {effect_xml_without_name,        E_INVALIDARG},
        {effect_xml_without_author,      E_INVALIDARG},
        {effect_xml_without_category,    E_INVALIDARG},
        {effect_xml_without_description, E_INVALIDARG},
        {effect_xml_without_type,        E_INVALIDARG},
    };

    const D2D1_PROPERTY_BINDING binding[] =
    {
        {L"Integer",  effect_impl_set_integer, effect_impl_get_integer},
        {L"Context",  NULL,                    effect_impl_get_context},
        {L"Integer",  NULL,                    effect_impl_get_integer},
        {L"Integer",  effect_impl_set_integer, NULL},
        {L"Integer",  NULL,                    NULL},
        {L"DeadBeef", effect_impl_set_integer, effect_impl_get_integer},
    };

    static const D2D1_PROPERTY_BINDING bindings2[] =
    {
        {L"Graph", NULL, effect_impl_get_graph},
    };

    const struct binding_test
    {
        const D2D1_PROPERTY_BINDING *binding;
        UINT32 binding_count;
        const WCHAR *effect_xml;
        HRESULT hr;
    }
    binding_tests[] =
    {
        {NULL,        0, effect_xml_a, S_OK},
        {NULL,        0, effect_xml_b, S_OK},
        {binding,     1, effect_xml_a, S_OK},
        {binding,     1, effect_xml_b, D2DERR_INVALID_PROPERTY},
        {binding + 1, 1, effect_xml_b, S_OK},
        {binding + 2, 1, effect_xml_a, S_OK},
        {binding + 3, 1, effect_xml_a, S_OK},
        {binding + 4, 1, effect_xml_a, S_OK},
        {binding + 5, 1, effect_xml_a, D2DERR_INVALID_PROPERTY},
        {binding,     2, effect_xml_a, D2DERR_INVALID_PROPERTY},
        {binding,     2, effect_xml_b, D2DERR_INVALID_PROPERTY},
        {binding,     2, effect_xml_c, S_OK},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    device_context = ctx.context;
    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    /* Using builtin effect CLSID. */
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_D2D1Crop, effect_xml_a, NULL,
            0, effect_impl_create);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Register effect once */
    for (i = 0; i < ARRAY_SIZE(xml_tests); ++i)
    {
        const struct xml_test *test = &xml_tests[i];
        winetest_push_context("Test %u", i);

        hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, test->xml, NULL, 0, effect_impl_create);
        ok(hr == test->hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, test->hr);
        if (hr == S_OK)
        {
            hr = ID2D1DeviceContext_CreateEffect(device_context, &CLSID_TestEffect, &effect);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
            ok(hr == D2DERR_EFFECT_IS_NOT_REGISTERED, "Got unexpected hr %#lx.\n", hr);
            ID2D1Effect_Release(effect);
        }

        winetest_pop_context();
    }

    /* Register effect multiple times */
    for (i = 0; i < ARRAY_SIZE(xml_tests); ++i)
    {
        const struct xml_test *test = &xml_tests[i];

        if (test->hr != S_OK)
            continue;

        winetest_push_context("Test %u", i);

        hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, test->xml, NULL, 0, effect_impl_create);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, test->xml, NULL, 0, effect_impl_create);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
        ok(hr == D2DERR_EFFECT_IS_NOT_REGISTERED, "Got unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    /* Register effect multiple times with different xml */
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_a, NULL, 0, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DeviceContext_CreateEffect(device_context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_DISPLAYNAME,
            D2D1_PROPERTY_TYPE_STRING, (BYTE *)display_name, sizeof(display_name));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(display_name, L"TestEffectA"), "Got unexpected display name %s.\n", debugstr_w(display_name));
    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_b, NULL, 0, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DeviceContext_CreateEffect(device_context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_DISPLAYNAME,
            D2D1_PROPERTY_TYPE_STRING, (BYTE *)display_name, sizeof(display_name));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(display_name, L"TestEffectA"), "Got unexpected display name %s.\n", debugstr_w(display_name));
    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Register effect with property binding */
    for (i = 0; i < ARRAY_SIZE(binding_tests); ++i)
    {
        const struct binding_test *test = &binding_tests[i];
        winetest_push_context("Test %u", i);

        hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
                test->effect_xml, test->binding, test->binding_count, effect_impl_create);
        ok(hr == test->hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, test->hr);
        if (SUCCEEDED(hr))
        {
            hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }

        winetest_pop_context();
    }

    /* Register effect multiple times with different binding */
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_c, binding, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DeviceContext_CreateEffect(device_context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    integer = 0xdeadbeef;
    effect_context = (ID2D1EffectContext *)0xdeadbeef;
    hr = ID2D1Effect_GetValueByName(effect, L"Integer",
            D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 10, "Got unexpected integer %u.\n", integer);
    ok(effect_context == NULL, "Got unexpected effect context %p.\n", effect_context);
    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_c, binding + 1, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DeviceContext_CreateEffect(device_context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    integer = 0xdeadbeef;
    effect_context = (ID2D1EffectContext *)0xdeadbeef;
    hr = ID2D1Effect_GetValueByName(effect, L"Integer",
            D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 10, "Got unexpected integer %u.\n", integer);
    ok(effect_context == NULL, "Got unexpected effect context %p.\n", effect_context);
    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Unregister builtin effect */
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_D2D1Composite);
    ok(hr == D2DERR_EFFECT_IS_NOT_REGISTERED, "Got unexpected hr %#lx.\n", hr);

    /* Variable input count. */
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_variable_input_count, bindings2, ARRAY_SIZE(bindings2), effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DeviceContext_CreateEffect(device_context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    integer = ID2D1Effect_GetInputCount(effect);
    ok(integer == 3, "Unexpected input count %u.\n", integer);

    hr = ID2D1Effect_GetValueByName(effect, L"Graph", D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&graph, sizeof(graph));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID2D1TransformGraph_GetInputCount(graph);
    ok(count == 3, "Unexpected input count %u.\n", count);

    integer = 0;
    hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MIN_INPUTS, D2D1_PROPERTY_TYPE_UINT32,
            (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 2, "Unexpected value %u.\n", integer);
    hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MAX_INPUTS, D2D1_PROPERTY_TYPE_UINT32,
            (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 5, "Unexpected value %u.\n", integer);
    hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_INPUTS, D2D1_PROPERTY_TYPE_ARRAY,
            (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 3, "Unexpected data %u.\n", integer);
    hr = ID2D1Effect_SetInputCount(effect, 4);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_INPUTS, D2D1_PROPERTY_TYPE_ARRAY,
            (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 3, "Unexpected data %u.\n", integer);

    hr = ID2D1Effect_GetValueByName(effect, L"Graph", D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&graph, sizeof(graph));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    count = ID2D1TransformGraph_GetInputCount(graph);
    ok(count == 4, "Unexpected input count %u.\n", count);

    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    release_test_context(&ctx);
}

static void test_effect_context(BOOL d3d11)
{
    ID2D1EffectContext *effect_context, *effect_context2;
    ID2D1Effect *effect = NULL, *effect2 = NULL;
    ID2D1DeviceContext *device_context;
    D2D1_PROPERTY_BINDING binding;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    ID2D1Device *device;
    ID3D10Blob *vs, *ps;
    ULONG refcount;
    BOOL loaded;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = D3DCompile(test_vs_code, sizeof(test_vs_code) - 1, "test_vs", NULL, NULL,
            "main", "vs_4_0", 0, 0, &vs, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = D3DCompile(test_ps_code, sizeof(test_ps_code) - 1, "test_ps", NULL, NULL,
            "main", "ps_4_0", 0, 0, &ps, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    binding.propertyName = L"Context";
    binding.setFunction = NULL;
    binding.getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_b, &binding, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    effect_context = NULL;
    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test shader loading */
    loaded = ID2D1EffectContext_IsShaderLoaded(effect_context, &GUID_TestVertexShader);
    ok(!loaded, "Unexpected shader loaded state.\n");
    loaded = ID2D1EffectContext_IsShaderLoaded(effect_context, &GUID_TestPixelShader);
    ok(!loaded, "Unexpected shader loaded state.\n");

    hr = ID2D1EffectContext_LoadVertexShader(effect_context,
            &GUID_TestVertexShader, ID3D10Blob_GetBufferPointer(ps), ID3D10Blob_GetBufferSize(ps));
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1EffectContext_LoadVertexShader(effect_context,
            &GUID_TestVertexShader, ID3D10Blob_GetBufferPointer(vs), ID3D10Blob_GetBufferSize(vs));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    loaded = ID2D1EffectContext_IsShaderLoaded(effect_context, &GUID_TestVertexShader);
    ok(loaded, "Unexpected shader loaded state.\n");

    hr = ID2D1EffectContext_LoadVertexShader(effect_context,
            &GUID_TestVertexShader, ID3D10Blob_GetBufferPointer(ps), ID3D10Blob_GetBufferSize(ps));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1EffectContext_LoadVertexShader(effect_context,
            &GUID_TestVertexShader, ID3D10Blob_GetBufferPointer(vs), ID3D10Blob_GetBufferSize(vs));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1EffectContext_LoadPixelShader(effect_context,
            &GUID_TestPixelShader, ID3D10Blob_GetBufferPointer(vs), ID3D10Blob_GetBufferSize(vs));
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1EffectContext_LoadPixelShader(effect_context,
            &GUID_TestPixelShader, ID3D10Blob_GetBufferPointer(ps), ID3D10Blob_GetBufferSize(ps));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    loaded = ID2D1EffectContext_IsShaderLoaded(effect_context, &GUID_TestPixelShader);
    ok(loaded, "Unexpected shader loaded state.\n");

    /* Same shader id, using different context. */
    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    effect_context2 = NULL;
    hr = ID2D1Effect_GetValueByName(effect2, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context2, sizeof(effect_context2));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    loaded = ID2D1EffectContext_IsShaderLoaded(effect_context2, &GUID_TestPixelShader);
    ok(loaded, "Unexpected shader loaded state.\n");

    ID2D1Effect_Release(effect2);

    /* Same shader id, using different device. */
    hr = ID2D1Factory1_CreateDevice(factory, ctx.device, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(device_context, &CLSID_TestEffect, &effect2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    effect_context = NULL;
    hr = ID2D1Effect_GetValueByName(effect2, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    loaded = ID2D1EffectContext_IsShaderLoaded(effect_context, &GUID_TestPixelShader);
    ok(!loaded, "Unexpected shader loaded state.\n");

    ID2D1Effect_Release(effect2);

    ID2D1DeviceContext_Release(device_context);
    ID2D1Device_Release(device);

    /* Release all created effects, check shader map. */
    refcount = ID2D1Effect_Release(effect);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    effect_context = NULL;
    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    loaded = ID2D1EffectContext_IsShaderLoaded(effect_context, &GUID_TestPixelShader);
    ok(loaded, "Unexpected shader loaded state.\n");

    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Blob_Release(vs);
    ID3D10Blob_Release(ps);

    release_test_context(&ctx);
}

#define check_zero_buffer(a, b) check_zero_buffer_(a, b, __LINE__)
static void check_zero_buffer_(const void *buffer, size_t size, unsigned int line)
{
    const uint32_t *ptr = buffer;
    const uint8_t *ptr2 = buffer;
    size_t i;

    for (i = 0; i < size / sizeof(*ptr); ++i)
        ok_(__FILE__, line)(!ptr[i], "Expected zero value %#x.\n", ptr[i]);
    for (i = 0; i < size % sizeof(*ptr); ++i)
        ok_(__FILE__, line)(!ptr2[size - i - 1], "Expected zero value %#x.\n", ptr2[i]);
}

static void test_effect_properties(BOOL d3d11)
{
    UINT32 i, min_inputs, max_inputs, integer, index, size;
    ID2D1EffectContext *effect_context;
    D2D1_BUFFER_PRECISION precision;
    ID2D1Properties *subproperties;
    D2D1_PROPERTY_TYPE prop_type;
    struct d2d1_test_context ctx;
    D2D_MATRIX_3X2_F mat3x2;
    D2D_MATRIX_4X3_F mat4x3;
    D2D_MATRIX_4X4_F mat4x4;
    D2D_MATRIX_5X4_F mat5x4;
    ID2D1Factory1 *factory;
    ID2D1Effect *effect;
    UINT32 count, data;
    D2D_VECTOR_2F vec2;
    D2D_VECTOR_3F vec3;
    D2D_VECTOR_4F vec4;
    WCHAR buffer[128];
    BYTE value[128];
    CLSID clsid;
    BOOL cached;
    HRESULT hr;
    float *ptr;
    INT32 val;

    static const WCHAR *effect_author = L"The Wine Project";
    static const WCHAR *effect_category = L"Test";
    static const WCHAR *effect_description = L"Test effect.";

    const D2D1_PROPERTY_BINDING binding[] =
    {
        {L"Integer",  effect_impl_set_integer, effect_impl_get_integer},
        {L"Context",  NULL,                    effect_impl_get_context},
    };

    const struct system_property_test
    {
        const WCHAR *xml;
        const WCHAR *display_name;
        const WCHAR *author;
        const WCHAR *category;
        const WCHAR *description;
        UINT32 min_inputs;
        UINT32 max_inputs;
    }
    system_property_tests[] =
    {
        {effect_xml_a,       L"TestEffectA", effect_author, effect_category, effect_description, 1, 1},
        {effect_xml_b,       L"TestEffectB", effect_author, effect_category, effect_description, 1, 1},
        {effect_xml_c,       L"TestEffectC", effect_author, effect_category, effect_description, 1, 1},
        {effect_xml_minimum, L"", L"" ,L"", L"", 0, 0},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    /* Inputs array */
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, effect_xml_a,
            NULL, 0, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID2D1Effect_GetValueSize(effect, D2D1_PROPERTY_INPUTS);
    ok(size == 4, "Unexpected size %u.\n", size);
    prop_type = ID2D1Effect_GetType(effect, D2D1_PROPERTY_INPUTS);
    ok(prop_type == D2D1_PROPERTY_TYPE_ARRAY, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetPropertyName(effect, D2D1_PROPERTY_INPUTS, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Inputs"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));

    /* Value is the number of elements. */
    data = 123;
    hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_INPUTS, D2D1_PROPERTY_TYPE_ARRAY,
            (BYTE *)&data, sizeof(data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(data == 1, "Unexpected data %u.\n", data);

    hr = ID2D1Effect_GetSubProperties(effect, D2D1_PROPERTY_INPUTS, &subproperties);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID2D1Properties_GetPropertyCount(subproperties);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = ID2D1Properties_GetPropertyName(subproperties, 0, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"0"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    hr = ID2D1Properties_GetValue(subproperties, 0, D2D1_PROPERTY_TYPE_STRING,
            (BYTE *)buffer, sizeof(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Source"), "Unexpected value %s.\n", wine_dbgstr_w(buffer));

    hr = ID2D1Properties_GetValue(subproperties, D2D1_SUBPROPERTY_ISREADONLY, D2D1_PROPERTY_TYPE_BOOL,
            (BYTE *)&data, sizeof(data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(data == TRUE, "Unexpected value %u.\n", data);
    hr = ID2D1Properties_GetPropertyName(subproperties, D2D1_SUBPROPERTY_ISREADONLY,
            buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"IsReadOnly"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));

    ID2D1Properties_Release(subproperties);

    /* Int32 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Int32Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Int32Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_INT32, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_INT32, (BYTE *)&val, sizeof(val));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(val == -2, "Unexpected value %d.\n", val);

    /* Int32 property, hex literal. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Int32PropHex");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Int32PropHex"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_INT32, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_INT32, (BYTE *)&val, sizeof(val));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(val == -65535, "Unexpected value %d.\n", val);

    /* Int32 property, octal literal. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Int32PropOct");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Int32PropOct"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_INT32, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_INT32, (BYTE *)&val, sizeof(val));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(val == 10, "Unexpected value %d.\n", val);

    /* UInt32 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"UInt32Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"UInt32Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_UINT32, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == -3, "Unexpected value %u.\n", integer);

    /* UInt32 property, hex literal. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"UInt32PropHex");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"UInt32PropHex"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_UINT32, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 0xfff, "Unexpected value %x.\n", integer);

    /* Vector2 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Vec2Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Vec2Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_VECTOR2, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_VECTOR2, (BYTE *)&vec2, sizeof(vec2));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(vec2.x == 3.0f, "Unexpected value %.8e.\n", vec2.x);
    ok(vec2.y == 4.0f, "Unexpected value %.8e.\n", vec2.y);

    /* Vector3 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Vec3Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Vec3Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_VECTOR3, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_VECTOR3, (BYTE *)&vec3, sizeof(vec3));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(vec3.x == 5.0f, "Unexpected value %.8e.\n", vec3.x);
    ok(vec3.y == 6.0f, "Unexpected value %.8e.\n", vec3.y);
    ok(vec3.z == 7.0f, "Unexpected value %.8e.\n", vec3.z);

    /* Vector4 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Vec4Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Vec4Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_VECTOR4, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_VECTOR4, (BYTE *)&vec4, sizeof(vec4));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(vec4.x == 8.0f, "Unexpected value %.8e.\n", vec4.x);
    ok(vec4.y == 9.0f, "Unexpected value %.8e.\n", vec4.y);
    ok(vec4.z == 10.0f, "Unexpected value %.8e.\n", vec4.z);
    ok(vec4.w == 11.0f, "Unexpected value %.8e.\n", vec4.w);

    /* Matrix3x2 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Mat3x2Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Mat3x2Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_MATRIX_3X2, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_MATRIX_3X2, (BYTE *)&mat3x2, sizeof(mat3x2));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (i = 0, ptr = (float *)&mat3x2; i < sizeof(mat3x2) / sizeof(*ptr); ++i, ++ptr)
        ok(*ptr == 1.0f + i, "Unexpected value %.8e.\n", *ptr);

    /* Matrix4x3 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Mat4x3Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Mat4x3Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_MATRIX_4X3, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_MATRIX_4X3, (BYTE *)&mat4x3, sizeof(mat4x3));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (i = 0, ptr = (float *)&mat4x3; i < sizeof(mat4x3) / sizeof(*ptr); ++i, ++ptr)
        ok(*ptr == 1.0f + i, "Unexpected value %.8e.\n", *ptr);

    /* Matrix4x4 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Mat4x4Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Mat4x4Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_MATRIX_4X4, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_MATRIX_4X4, (BYTE *)&mat4x4, sizeof(mat4x4));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (i = 0, ptr = (float *)&mat4x4; i < sizeof(mat4x4) / sizeof(*ptr); ++i, ++ptr)
        ok(*ptr == 1.0f + i, "Unexpected value %.8e.\n", *ptr);

    /* Matrix5x4 property. */
    index = ID2D1Effect_GetPropertyIndex(effect, L"Mat5x4Prop");
    hr = ID2D1Effect_GetPropertyName(effect, index, buffer, ARRAY_SIZE(buffer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffer, L"Mat5x4Prop"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
    prop_type = ID2D1Effect_GetType(effect, index);
    ok(prop_type == D2D1_PROPERTY_TYPE_MATRIX_5X4, "Unexpected type %u.\n", prop_type);
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_MATRIX_5X4, (BYTE *)&mat5x4, sizeof(mat5x4));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (i = 0, ptr = (float *)&mat5x4; i < sizeof(mat5x4) / sizeof(*ptr); ++i, ++ptr)
        ok(*ptr == 1.0f + i, "Unexpected value %.8e.\n", *ptr);

    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test system properties */

    for (i = 0; i < ARRAY_SIZE(system_property_tests); ++i)
    {
        const struct system_property_test *test = &system_property_tests[i];
        winetest_push_context("Test %u", i);

        hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, test->xml, NULL, 0, effect_impl_create);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        effect = NULL;
        hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        check_system_properties(effect);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID,
                D2D1_PROPERTY_TYPE_CLSID, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&clsid, &CLSID_TestEffect), "Got unexpected clsid %s, expected %s.\n",
                debugstr_guid(&clsid), debugstr_guid(&CLSID_TestEffect));

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_DISPLAYNAME,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!wcscmp(buffer, test->display_name), "Got unexpected display name %s, expected %s.\n",
                debugstr_w(buffer), debugstr_w(test->display_name));

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_AUTHOR,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!wcscmp(buffer, test->author), "Got unexpected author %s, expected %s.\n",
                debugstr_w(buffer), debugstr_w(test->author));

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CATEGORY,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!wcscmp(buffer, test->category), "Got unexpected category %s, expected %s.\n",
                debugstr_w(buffer), debugstr_w(test->category));

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_DESCRIPTION,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!wcscmp(buffer, test->description), "Got unexpected description %s, expected %s.\n",
                debugstr_w(buffer), debugstr_w(test->description));

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CACHED,
                D2D1_PROPERTY_TYPE_BOOL, (BYTE *)&cached, sizeof(cached));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(cached == FALSE, "Got unexpected cached %d.\n", cached);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_PRECISION,
                D2D1_PROPERTY_TYPE_ENUM, (BYTE *)&precision, sizeof(precision));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(precision == D2D1_BUFFER_PRECISION_UNKNOWN, "Got unexpected precision %#x.\n", precision);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MIN_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&min_inputs, sizeof(min_inputs));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(min_inputs == test->min_inputs, "Got unexpected min inputs %u, expected %u.\n",
                min_inputs, test->min_inputs);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MAX_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&max_inputs, sizeof(max_inputs));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(max_inputs == test->max_inputs, "Got unexpected max inputs %u, expected %u.\n",
                max_inputs, test->max_inputs);

        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_CLSID,
                D2D1_PROPERTY_TYPE_CLSID, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_DISPLAYNAME,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_AUTHOR,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_CATEGORY,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_DISPLAYNAME,
                D2D1_PROPERTY_TYPE_STRING, (BYTE *)buffer, sizeof(buffer));
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_CACHED,
                D2D1_PROPERTY_TYPE_BOOL, (BYTE *)&cached, sizeof(cached));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_PRECISION,
                D2D1_PROPERTY_TYPE_ENUM, (BYTE *)&precision, sizeof(precision));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_MIN_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&min_inputs, sizeof(min_inputs));
        ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Effect_SetValue(effect, D2D1_PROPERTY_MAX_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&max_inputs, sizeof(max_inputs));
        ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "Got unexpected hr %#lx.\n", hr);

        ID2D1Effect_Release(effect);
        hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        winetest_pop_context();
    }

    /* Test custom properties */

    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_c, binding, 2, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    effect = NULL;
    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID2D1Effect_GetPropertyCount(effect);
    ok(count == 3, "Got unexpected property count %u.\n", count);

    index = ID2D1Effect_GetPropertyIndex(effect, L"Context");
    ok(index == 0, "Got unexpected index %u.\n", index);
    index = ID2D1Effect_GetPropertyIndex(effect, L"Integer");
    ok(index == 1, "Got unexpected index %u.\n", index);

    /* On type mismatch output buffer is zeroed. */
    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValueByName(effect, L"Context", D2D1_PROPERTY_TYPE_UINT32, value, sizeof(void *));
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, sizeof(void *));

    /* On size mismatch output buffer is zeroed. */
    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValueByName(effect, L"Context", D2D1_PROPERTY_TYPE_IUNKNOWN, value, sizeof(void *) - 1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, sizeof(void *) - 1);

    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValueByName(effect, L"Context", D2D1_PROPERTY_TYPE_IUNKNOWN, value, sizeof(void *) + 1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, sizeof(void *) + 1);

    effect_context = NULL;
    hr = ID2D1Effect_GetValueByName(effect,
            L"Context", D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!effect_context, "Got unexpected effect context %p.\n", effect_context);

    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValue(effect, 0, D2D1_PROPERTY_TYPE_IUNKNOWN, value, sizeof(void *) - 1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, sizeof(void *) - 1);

    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValue(effect, 0, D2D1_PROPERTY_TYPE_IUNKNOWN, value, sizeof(void *) + 1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, sizeof(void *) + 1);

    effect_context = NULL;
    hr = ID2D1Effect_GetValue(effect, 0, D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!effect_context, "Got unexpected effect context %p.\n", effect_context);

    hr = ID2D1Effect_SetValue(effect, 0, D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValueByName(effect, L"Integer", D2D1_PROPERTY_TYPE_UINT32, value, 3);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, 3);

    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValueByName(effect, L"Integer", D2D1_PROPERTY_TYPE_UINT32, value, 5);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, 5);

    integer = 0;
    hr = ID2D1Effect_GetValueByName(effect, L"Integer", D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 10, "Got unexpected integer %u.", integer);

    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, value, 3);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, 3);

    memset(value, 0xcc, sizeof(value));
    hr = ID2D1Effect_GetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, value, 5);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    check_zero_buffer(value, 5);

    integer = 0;
    hr = ID2D1Effect_GetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 10, "Got unexpected integer %u.", integer);

    memset(value, 0, sizeof(value));
    hr = ID2D1Effect_SetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, value, 3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Effect_SetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, value, 5);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    integer = 20;
    hr = ID2D1Effect_SetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    integer = 0xdeadbeef;
    hr = ID2D1Effect_GetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 20, "Got unexpected integer %u\n.", integer);

    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test custom properties without binding */

    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, effect_xml_c, NULL, 0, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    index = ID2D1Effect_GetPropertyIndex(effect, L"Context");
    ok(index == 0, "Got unexpected index %u.\n", index);
    index = ID2D1Effect_GetPropertyIndex(effect, L"Integer");
    ok(index == 1, "Got unexpected index %u.\n", index);

    hr = ID2D1Effect_GetValueByName(effect, L"DeadBeef", D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == D2DERR_INVALID_PROPERTY, "Got unexpected hr %#lx.\n", hr);

    effect_context = (ID2D1EffectContext *)0xdeadbeef;
    hr = ID2D1Effect_GetValue(effect, 0, D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(effect_context == NULL, "Got unexpected effect context %p.\n", effect_context);

    integer = 0xdeadbeef;
    hr = ID2D1Effect_GetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(integer == 0, "Got unexpected integer %u.", integer);

    hr = ID2D1Effect_SetValue(effect, 0, D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Effect_SetValue(effect, 1, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&integer, sizeof(integer));
    ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "Got unexpected hr %#lx.\n", hr);

    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    release_test_context(&ctx);
}

static void check_enum_property(ID2D1Effect *effect, UINT32 property,
        const UINT32 *values, UINT32 count)
{
    ID2D1Properties *subproperties, *fields_subproperties, *field_subproperties;
    D2D1_PROPERTY_TYPE prop_type;
    UINT32 prop_count, value;
    WCHAR buffer[64];
    unsigned int i;
    HRESULT hr;

    prop_type = ID2D1Effect_GetType(effect, property);
    ok(prop_type == D2D1_PROPERTY_TYPE_ENUM, "Unexpected type %d.\n", prop_type);

    hr = ID2D1Effect_GetSubProperties(effect, property, &subproperties);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    prop_type = ID2D1Properties_GetType(subproperties, D2D1_SUBPROPERTY_FIELDS);
    ok(prop_type == D2D1_PROPERTY_TYPE_ARRAY, "Unexpected type %d.\n", prop_type);

    /* Number of enum members. */
    hr = ID2D1Properties_GetValue(subproperties, D2D1_SUBPROPERTY_FIELDS, D2D1_PROPERTY_TYPE_ARRAY,
            (BYTE *)&value, sizeof(value));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(value == count, "Unexpected value %u.\n", value);

    prop_count = ID2D1Properties_GetPropertyCount(subproperties);
    ok(!prop_count, "Got unexpected property count %u.\n", count);

    /* Fields is an array of strings providing display names for members. */
    hr = ID2D1Properties_GetSubProperties(subproperties, D2D1_SUBPROPERTY_FIELDS,
            &fields_subproperties);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    prop_count = ID2D1Properties_GetPropertyCount(fields_subproperties);
    ok(prop_count == count, "Got unexpected property count %u.\n", count);

    for (i = 0; i < prop_count; ++i)
    {
        winetest_push_context("Test %u", i);

        prop_type = ID2D1Properties_GetType(fields_subproperties, i);
        ok(prop_type == D2D1_PROPERTY_TYPE_STRING, "Unexpected type %d.\n", prop_type);

        /* Field names are localized. */
        hr = ID2D1Properties_GetSubProperties(fields_subproperties, i, &field_subproperties);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        prop_count = ID2D1Properties_GetPropertyCount(field_subproperties);
        ok(prop_count == 1, "Got unexpected property count %u.\n", count);

        prop_type = ID2D1Properties_GetType(field_subproperties, 0);
        ok(prop_type == D2D1_PROPERTY_TYPE_UINT32, "Unexpected type %d.\n", prop_type);
        hr = ID2D1Properties_GetPropertyName(field_subproperties, 0, buffer, ARRAY_SIZE(buffer));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!wcscmp(buffer, L"Index"), "Unexpected name %s.\n", wine_dbgstr_w(buffer));
        hr = ID2D1Properties_GetValue(field_subproperties, 0, D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&value, sizeof(value));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(value == values[i], "Unexpected value %u, expected %u.\n", value, values[i]);

        ID2D1Properties_Release(field_subproperties);

        winetest_pop_context();
    }

    ID2D1Properties_Release(fields_subproperties);
    ID2D1Properties_Release(subproperties);
}

static void test_effect_2d_affine(BOOL d3d11)
{
    D2D1_MATRIX_3X2_F rotate, scale, skew;
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    unsigned int i, x, y, w, h, count;
    D2D_RECT_F output_bounds = {0};
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    D2D1_SIZE_U input_size;
    ID2D1Factory1 *factory;
    D2D1_POINT_2F offset;
    ID2D1Bitmap1 *bitmap;
    ID2D1Effect *effect;
    ID2D1Image *output;
    UINT32 value;
    BOOL match;
    HRESULT hr;

    static const DWORD image_4x4[] =
    {
        0xfdcba987, 0xffff0000, 0x98765432, 0xffffffff,
        0x4b4b4b4b, 0x89abcdef, 0xdeadbeef, 0xabcdef01,
        0x7f000011, 0x40ffffff, 0x12345678, 0xaabbccdd,
        0x44444444, 0xff7f7f7f, 0x1221abba, 0x00000000
    };
    DWORD image_16x16[16 * 16];

    const struct effect_2d_affine_test
    {
        const DWORD *img_data;
        unsigned int img_width;
        unsigned int img_height;

        D2D1_MATRIX_3X2_F *matrix;

        D2D_RECT_F bounds;
        const char *figure;
    }
    effect_2d_affine_tests[] =
    {
        {image_4x4,   4,  4,  &rotate, {-6.0f,  -3.0f, 2.0f,  4.0f},
         "ASgBAQkBAQEBBwEBAQEBAQYBAQEBAQEHAQEBASgA"},
        {image_4x4,   4,  4,  &scale,  {-2.0f,  -3.0f, 1.0f,  9.0f},
         "AQ8BAQEEAQEBBAEBAQQBAQEEAQEBBAEBAQQBAQEEAQEBBAEBAQQBAQEEAQEBBAEBARAA/"},
        {image_4x4,   4,  4,  &skew,   {-7.0f,  -3.0f, 3.0f,  7.0f},
         "AS8CCwEBAQEJAQEBAQEBBwEBAQEBAQEBBgEBAQEBAQEBBwEBAQEBAQkCAj0A"},
        {image_16x16, 16, 16, &rotate, {-14.0f, -3.0f, 10.0f, 21.0f},
         "AWACGQECARcBBAEVAQYBEwEIAREBCgEPAQwBDQEOAQsBEAEJARIBBwEUAQUBARQBAQUBARIBAQcB"
         "ARABAQkBAQ4BAQsBAQwBAQ0BAQoBAQ8BAQgBAREBAQYBARMBAQQBARUBAQIBARcBAgEZAkUA"},
        {image_16x16, 16, 16, &scale,  {-2.0f,  -3.0f, 10.0f, 39.0f},
         "ASEMBAwEDAQMBAwEDAQMBAwEDAQMBAwEDAQMBAwEDAQMBAwEDAQMBAwEDAQMBAwEDAQMBAwEDAQM"
         "BAwEDAQMBAwEDAQMBAwEDAQMBAwEDAQMBAwEDCIA"},
        {image_16x16, 16, 16, &skew,   {-19.0f, -3.0f, 15.0f, 31.0f},
         "AYMBAiMBAgEhAQQBHwEGAR0BCAEbAQoBGQEMARcBDgEVARABEwESAREBFAEPARYBDQEYAQsBGgEJ"
         "ARwBBwEeAQYBHgEHARwBCQEaAQsBGAENARYBDwEUAREBEgETARABFQEOARcBDAEZAQoBGwEIAR0B"
         "BgEfAQQBIQECASMChAEA"},
    };

    static const unsigned int border_modes[] =
    {
        D2D1_BORDER_MODE_SOFT,
        D2D1_BORDER_MODE_HARD,
    };

    static const unsigned int interp_modes[] =
    {
        D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
        D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_LINEAR,
        D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_CUBIC,
        D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR,
        D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_ANISOTROPIC,
        D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC,
    };

    memset(image_16x16, 0xff, sizeof(image_16x16));
    set_matrix_identity(&rotate);
    set_matrix_identity(&scale);
    set_matrix_identity(&skew);
    translate_matrix(&rotate, -2.0f, -2.0f);
    translate_matrix(&scale, -2.0f, -2.0f);
    translate_matrix(&skew, -2.0f, -2.0f);
    rotate_matrix(&rotate, M_PI_4);
    scale_matrix(&scale, 0.75f, 2.5f);
    skew_matrix(&skew, -1.0f, 1.0f);

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    context = ctx.context;

    hr = ID2D1DeviceContext_CreateEffect(context, &CLSID_D2D12DAffineTransform, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_system_properties(effect);

    count = ID2D1Effect_GetPropertyCount(effect);
    todo_wine ok(count == 4, "Got unexpected property count %u.\n", count);

    hr = ID2D1Effect_GetValue(effect, D2D1_2DAFFINETRANSFORM_PROP_INTERPOLATION_MODE,
            D2D1_PROPERTY_TYPE_ENUM, (BYTE *)&value, sizeof(value));
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(value == D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_LINEAR, "Unexpected value %u.\n", value);
        check_enum_property(effect, D2D1_2DAFFINETRANSFORM_PROP_INTERPOLATION_MODE, interp_modes,
                ARRAY_SIZE(interp_modes));
    }

    hr = ID2D1Effect_GetValue(effect, D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE, D2D1_PROPERTY_TYPE_ENUM,
            (BYTE *)&value, sizeof(value));
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(value == D2D1_BORDER_MODE_SOFT, "Unexpected value %u.\n", value);
        check_enum_property(effect, D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE, border_modes,
                ARRAY_SIZE(border_modes));
    }

    for (i = 0; i < ARRAY_SIZE(effect_2d_affine_tests); ++i)
    {
        const struct effect_2d_affine_test *test = &effect_2d_affine_tests[i];
        winetest_push_context("Test %u", i);

        set_size_u(&input_size, test->img_width, test->img_height);
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(context, input_size, test->img_data,
                sizeof(*test->img_data) * test->img_width, &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID2D1Effect_SetInput(effect, 0, (ID2D1Image *)bitmap, FALSE);
        hr = ID2D1Effect_SetValue(effect, D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX,
                D2D1_PROPERTY_TYPE_MATRIX_3X2, (const BYTE *)test->matrix, sizeof(*test->matrix));
        todo_wine
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        if (hr == D2DERR_INVALID_PROPERTY)
        {
            ID2D1Bitmap1_Release(bitmap);
            winetest_pop_context();
            continue;
        }
        ID2D1Effect_GetOutput(effect, &output);

        hr = ID2D1DeviceContext_GetImageLocalBounds(context, output, &output_bounds);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        todo_wine
        ok(compare_rect(&output_bounds, test->bounds.left, test->bounds.top, test->bounds.right, test->bounds.bottom, 1),
                "Got unexpected output bounds {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                output_bounds.left, output_bounds.top, output_bounds.right, output_bounds.bottom,
                test->bounds.left, test->bounds.top, test->bounds.right, test->bounds.bottom);
        if (output_bounds.left == output_bounds.right || output_bounds.top == output_bounds.bottom)
        {
            ID2D1Image_Release(output);
            ID2D1Bitmap1_Release(bitmap);
            winetest_pop_context();
            continue;
        }

        ID2D1DeviceContext_BeginDraw(context);
        ID2D1DeviceContext_Clear(context, 0);
        offset.x = 320.0f;
        offset.y = 240.0f;
        ID2D1DeviceContext_DrawImage(context, output, &offset, NULL, 0, 0);
        hr = ID2D1DeviceContext_EndDraw(context, NULL, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        x = offset.x + output_bounds.left - 2.0f;
        y = offset.y + output_bounds.top - 2.0f;
        w = (output_bounds.right - output_bounds.left) + 4.0f;
        h = (output_bounds.bottom - output_bounds.top) + 4.0f;
        match = compare_figure(&ctx, x, y, w, h, 0xff652e89, 0, test->figure);
        todo_wine
        ok(match, "Figure does not match.\n");

        ID2D1Image_Release(output);
        ID2D1Bitmap1_Release(bitmap);
        winetest_pop_context();
    }

    ID2D1Effect_Release(effect);
    release_test_context(&ctx);
}

static void test_effect_crop(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    D2D_RECT_F output_bounds;
    D2D1_SIZE_U input_size;
    ID2D1Factory1 *factory;
    unsigned int count, i;
    ID2D1Bitmap1 *bitmap;
    DWORD image[16 * 16];
    ID2D1Effect *effect;
    ID2D1Image *output;
    HRESULT hr;

    const struct crop_effect_test
    {
        D2D_VECTOR_4F crop_rect; /* {x: left, y: top, z: right, w: bottom} */
        D2D_RECT_F    bounds;
    }
    crop_effect_tests[] =
    {
        {{0.0f,   0.0f,  8.0f,  8.0f},  {0.0f,  0.0f,  8.0f,  8.0f}},
        {{4.0f,   4.0f,  8.0f,  8.0f},  {4.0f,  4.0f,  8.0f,  8.0f}},
        {{10.0f,  10.0f, 20.0f, 20.0f}, {10.0f, 10.0f, 16.0f, 16.0f}},
        {{-10.0f, 10.0f, 20.0f, 12.0f}, {0.0f,  10.0f, 16.0f, 12.0f}},
        {{3.0f,   -2.0f, 5.0f,  1.0f},  {3.0f,  0.0f,  5.0f,  1.0f}},
        {{-1.0f,  -1.0f, 20.0f, 20.0f}, {0.0f,  0.0f,  16.0f, 16.0f}},
        {{-5.0f,  -5.0f, -1.0f, -1.0f}, {0.0f,  0.0f,  0.0f,  0.0f}},
    };
    memset(image, 0xff, sizeof(image));

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    context = ctx.context;

    hr = ID2D1DeviceContext_CreateEffect(context, &CLSID_D2D1Crop, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_system_properties(effect);

    count = ID2D1Effect_GetPropertyCount(effect);
    todo_wine ok(count == 2, "Got unexpected property count %u.\n", count);

    for (i = 0; i < ARRAY_SIZE(crop_effect_tests); ++i)
    {
        const struct crop_effect_test *test = &crop_effect_tests[i];
        winetest_push_context("Test %u", i);

        set_size_u(&input_size, 16, 16);
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(context, input_size, image,
                sizeof(*image) * 16, &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID2D1Effect_SetInput(effect, 0, (ID2D1Image *)bitmap, FALSE);
        hr = ID2D1Effect_SetValue(effect, D2D1_CROP_PROP_RECT, D2D1_PROPERTY_TYPE_VECTOR4,
                (const BYTE *)&test->crop_rect, sizeof(test->crop_rect));
        todo_wine
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        if (hr == D2DERR_INVALID_PROPERTY)
        {
            ID2D1Bitmap1_Release(bitmap);
            winetest_pop_context();
            continue;
        }
        ID2D1Effect_GetOutput(effect, &output);

        set_rect(&output_bounds, -1.0f, -1.0f, -1.0f, -1.0f);
        hr = ID2D1DeviceContext_GetImageLocalBounds(context, output, &output_bounds);
        todo_wine
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        todo_wine
        ok(compare_rect(&output_bounds, test->bounds.left, test->bounds.top, test->bounds.right, test->bounds.bottom, 0),
                "Got unexpected output bounds {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                output_bounds.left, output_bounds.top, output_bounds.right, output_bounds.bottom,
                test->bounds.left, test->bounds.top, test->bounds.right, test->bounds.bottom);

        ID2D1Image_Release(output);
        ID2D1Bitmap1_Release(bitmap);
        winetest_pop_context();
    }

    ID2D1Effect_Release(effect);
    release_test_context(&ctx);
}

static void test_effect_grayscale(BOOL d3d11)
{
    DWORD colour, expected_colour, luminance;
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    struct d2d1_test_context ctx;
    struct resource_readback rb;
    ID2D1DeviceContext *context;
    D2D1_SIZE_U input_size;
    ID2D1Factory3 *factory;
    unsigned int count, i;
    ID2D1Bitmap1 *bitmap;
    ID2D1Effect *effect;
    ID2D1Image *output;
    HRESULT hr;

    const DWORD test_pixels[] = {0xffffffff, 0x12345678, 0x89abcdef, 0x77777777, 0xdeadbeef};

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory3;
    if (!factory)
    {
        win_skip("ID2D1Factory3 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    context = ctx.context;

    hr = ID2D1DeviceContext_CreateEffect(context, &CLSID_D2D1Grayscale, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_system_properties(effect);

    count = ID2D1Effect_GetPropertyCount(effect);
    ok(!count, "Got unexpected property count %u.\n", count);

    for (i = 0; i < ARRAY_SIZE(test_pixels); ++i)
    {
        DWORD pixel = test_pixels[i];
        winetest_push_context("Test %u", i);

        set_size_u(&input_size, 1, 1);
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(context, input_size, &pixel, sizeof(pixel), &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID2D1Effect_SetInput(effect, 0, (ID2D1Image *)bitmap, FALSE);
        ID2D1Effect_GetOutput(effect, &output);

        ID2D1DeviceContext_BeginDraw(context);
        ID2D1DeviceContext_Clear(context, 0);
        ID2D1DeviceContext_DrawImage(context, output, NULL, NULL, 0, 0);
        hr = ID2D1DeviceContext_EndDraw(context, NULL, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        get_surface_readback(&ctx, &rb);
        colour = get_readback_colour(&rb, 0, 0);
        luminance = (DWORD)(0.299f * ((pixel >> 16) & 0xff)
                + 0.587f * ((pixel >> 8) & 0xff)
                + 0.114f * ((pixel >> 0) & 0xff) + 0.5f);
        expected_colour = (pixel & 0xff000000) | (luminance << 16) | (luminance << 8) | luminance;
        todo_wine ok(compare_colour(colour, expected_colour, 1),
                "Got unexpected colour %#lx, expected %#lx.\n", colour, expected_colour);
        release_resource_readback(&rb);

        ID2D1Image_Release(output);
        ID2D1Bitmap1_Release(bitmap);
        winetest_pop_context();
    }

    ID2D1Effect_Release(effect);
    release_test_context(&ctx);
}

static void test_registered_effects(BOOL d3d11)
{
    UINT32 ret, count, count2, count3;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    CLSID *effects;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;

    count = 0;
    hr = ID2D1Factory1_GetRegisteredEffects(factory, NULL, 0, NULL, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count > 0, "Unexpected effect count %u.\n", count);

    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, effect_xml_a,
            NULL, 0, effect_impl_create);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count2 = 0;
    hr = ID2D1Factory1_GetRegisteredEffects(factory, NULL, 0, NULL, &count2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count2 == count + 1, "Unexpected effect count %u.\n", count2);

    effects = calloc(count2, sizeof(*effects));

    count3 = 0;
    hr = ID2D1Factory1_GetRegisteredEffects(factory, effects, 0, NULL, &count3);
    ok(hr == D2DERR_INSUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count2 == count3, "Unexpected effect count %u.\n", count3);

    ret = 999;
    hr = ID2D1Factory1_GetRegisteredEffects(factory, effects, 0, &ret, NULL);
    ok(hr == D2DERR_INSUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(!ret, "Unexpected count %u.\n", ret);

    ret = 0;
    hr = ID2D1Factory1_GetRegisteredEffects(factory, effects, 1, &ret, NULL);
    ok(hr == D2DERR_INSUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(ret == 1, "Unexpected count %u.\n", ret);
    ok(!IsEqualGUID(effects, &CLSID_TestEffect), "Unexpected clsid.\n");

    ret = 0;
    hr = ID2D1Factory1_GetRegisteredEffects(factory, effects, count2, &ret, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret == count2, "Unexpected count %u.\n", ret);
    ok(IsEqualGUID(&effects[ret - 1], &CLSID_TestEffect), "Unexpected clsid.\n");

    free(effects);

    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count2 = 0;
    hr = ID2D1Factory1_GetRegisteredEffects(factory, NULL, 0, NULL, &count2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count2 == count, "Unexpected effect count %u.\n", count2);

    release_test_context(&ctx);
}

static void test_transform_graph(BOOL d3d11)
{
    ID2D1OffsetTransform *offset_transform = NULL;
    ID2D1BlendTransform *blend_transform = NULL;
    D2D1_BLEND_DESCRIPTION blend_desc = {0};
    ID2D1EffectContext *effect_context;
    struct d2d1_test_context ctx;
    ID2D1TransformGraph *graph;
    ID2D1Factory1 *factory;
    POINT point = {0 ,0};
    ID2D1Effect *effect;
    UINT i, count;
    HRESULT hr;

    const D2D1_PROPERTY_BINDING binding[] =
    {
        {L"Context", NULL, effect_impl_get_context},
        {L"Graph",   NULL, effect_impl_get_graph},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_c, binding, ARRAY_SIZE(binding), effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValueByName(effect, L"Graph", D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&graph, sizeof(graph));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID2D1TransformGraph_GetInputCount(graph);
    ok(count == 1, "Unexpected input count %u.\n", count);

    /* Create transforms */
    hr = ID2D1EffectContext_CreateOffsetTransform(effect_context, point, &offset_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1EffectContext_CreateBlendTransform(effect_context, 2, &blend_desc, &blend_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Add nodes */
    hr = ID2D1TransformGraph_ConnectToEffectInput(graph, 1, (ID2D1TransformNode *)offset_transform, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_SetOutputNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    /* Single input effect, single input node. */
    hr = ID2D1TransformGraph_SetSingleTransformNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Single input effect, two-input node. */
    hr = ID2D1TransformGraph_SetSingleTransformNode(graph, (ID2D1TransformNode *)blend_transform);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)blend_transform);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ID2D1TransformGraph_Clear(graph);

    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)blend_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Invalid effect input index. */
    hr = ID2D1TransformGraph_ConnectToEffectInput(graph, 1, (ID2D1TransformNode *)offset_transform, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    /* Invalid object input index. */
    hr = ID2D1TransformGraph_ConnectToEffectInput(graph, 0, (ID2D1TransformNode *)offset_transform, 1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_ConnectToEffectInput(graph, 0, (ID2D1TransformNode *)offset_transform, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_SetOutputNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Remove nodes */
    hr = ID2D1TransformGraph_RemoveNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_RemoveNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_RemoveNode(graph, (ID2D1TransformNode *)blend_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Connect nodes which are both un-added */
    ID2D1TransformGraph_Clear(graph);
    hr = ID2D1TransformGraph_ConnectNode(graph,
            (ID2D1TransformNode *)offset_transform, (ID2D1TransformNode *)blend_transform, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    /* Connect added node to un-added node */
    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_ConnectNode(graph,
            (ID2D1TransformNode *)offset_transform, (ID2D1TransformNode *)blend_transform, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    /* Connect un-added node to added node */
    ID2D1TransformGraph_Clear(graph);
    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)blend_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_ConnectNode(graph,
            (ID2D1TransformNode *)offset_transform, (ID2D1TransformNode *)blend_transform, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    /* Connect nodes */
    ID2D1TransformGraph_Clear(graph);
    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)offset_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1TransformGraph_AddNode(graph, (ID2D1TransformNode *)blend_transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    count = ID2D1BlendTransform_GetInputCount(blend_transform);
    for (i = 0; i < count; ++i)
    {
        hr = ID2D1TransformGraph_ConnectNode(graph,
                (ID2D1TransformNode *)offset_transform, (ID2D1TransformNode *)blend_transform, i);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }

    /* Connect node to out-of-bounds index */
    hr = ID2D1TransformGraph_ConnectNode(graph,
            (ID2D1TransformNode *)offset_transform, (ID2D1TransformNode *)blend_transform, count);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Passthrough graph. */
    hr = ID2D1TransformGraph_SetPassthroughGraph(graph, 100);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1TransformGraph_SetPassthroughGraph(graph, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1BlendTransform_Release(blend_transform);
    ID2D1OffsetTransform_Release(offset_transform);
    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    release_test_context(&ctx);
}

static void test_offset_transform(BOOL d3d11)
{
    ID2D1OffsetTransform *transform = NULL;
    ID2D1EffectContext *effect_context;
    D2D1_PROPERTY_BINDING binding;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    D2D1_POINT_2L offset;
    ID2D1Effect *effect;
    UINT input_count;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    binding.propertyName = L"Context";
    binding.setFunction = NULL;
    binding.getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_b, &binding, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Create transform */
    offset.x = 1;
    offset.y = 2;
    hr = ID2D1EffectContext_CreateOffsetTransform(effect_context, offset, &transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    offset = ID2D1OffsetTransform_GetOffset(transform);
    ok(offset.x == 1 && offset.y == 2, "Got unexpected offset {%ld, %ld}.\n", offset.x, offset.y);

    check_interface(transform, &IID_ID2D1OffsetTransform, TRUE);
    check_interface(transform, &IID_ID2D1TransformNode, TRUE);

    /* Input count */
    input_count = ID2D1OffsetTransform_GetInputCount(transform);
    ok(input_count == 1, "Got unexpected input count %u.\n", input_count);

    /* Set offset */
    offset.x = -10;
    offset.y = 20;
    ID2D1OffsetTransform_SetOffset(transform, offset);
    offset = ID2D1OffsetTransform_GetOffset(transform);
    ok(offset.x == -10 && offset.y == 20, "Got unexpected offset {%ld, %ld}.\n", offset.x, offset.y);

    ID2D1OffsetTransform_Release(transform);
    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    release_test_context(&ctx);
}

#define check_blend_desc(blend_desc, expected) check_blend_desc_(__LINE__, blend_desc, expected)
static void check_blend_desc_(unsigned int line,
        const D2D1_BLEND_DESCRIPTION *blend_desc, const D2D1_BLEND_DESCRIPTION *expected)
{
    ok_(__FILE__, line)(blend_desc->sourceBlend == expected->sourceBlend,
            "Got unexpected source blend %u, expected %u.\n",
            blend_desc->sourceBlend, expected->sourceBlend);
    ok_(__FILE__, line)(blend_desc->destinationBlend == expected->destinationBlend,
            "Got unexpected destination blend %u, expected %u.\n",
            blend_desc->destinationBlend, expected->destinationBlend);
    ok_(__FILE__, line)(blend_desc->blendOperation == expected->blendOperation,
            "Got unexpected blend operation %u, expected %u.\n",
            blend_desc->blendOperation, expected->blendOperation);
    ok_(__FILE__, line)(blend_desc->sourceBlendAlpha == expected->sourceBlendAlpha,
            "Got unexpected source blend alpha %u, expected %u.\n",
            blend_desc->sourceBlendAlpha, expected->sourceBlendAlpha);
    ok_(__FILE__, line)(blend_desc->destinationBlendAlpha == expected->destinationBlendAlpha,
            "Got unexpected destination blend alpha %u, expected %u.\n",
            blend_desc->destinationBlendAlpha, expected->destinationBlendAlpha);
    ok_(__FILE__, line)(blend_desc->blendOperationAlpha == expected->blendOperationAlpha,
            "Got unexpected blend operation alpha %u, expected %u.\n",
            blend_desc->blendOperationAlpha, expected->blendOperationAlpha);
    ok_(__FILE__, line)(blend_desc->blendFactor[0] == expected->blendFactor[0],
            "Got unexpected blendFactor[0] %.8e, expected %.8e.\n",
            blend_desc->blendFactor[0], expected->blendFactor[0]);
    ok_(__FILE__, line)(blend_desc->blendFactor[1] == expected->blendFactor[1],
            "Got unexpected blendFactor[1] %.8e, expected %.8e.\n",
            blend_desc->blendFactor[1], expected->blendFactor[1]);
    ok_(__FILE__, line)(blend_desc->blendFactor[2] == expected->blendFactor[2],
            "Got unexpected blendFactor[2] %.8e, expected %.8e.\n",
            blend_desc->blendFactor[2], expected->blendFactor[2]);
    ok_(__FILE__, line)(blend_desc->blendFactor[3] == expected->blendFactor[3],
            "Got unexpected blendFactor[3] %.8e, expected %.8e.\n",
            blend_desc->blendFactor[3], expected->blendFactor[3]);
}

static void test_blend_transform(BOOL d3d11)
{
    D2D1_BLEND_DESCRIPTION blend_desc, expected= {0};
    ID2D1BlendTransform *transform = NULL;
    ID2D1EffectContext *effect_context;
    D2D1_PROPERTY_BINDING binding;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    ID2D1Effect *effect;
    UINT input_count;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    binding.propertyName = L"Context";
    binding.setFunction = NULL;
    binding.getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_b, &binding, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Create transform */
    transform = (void *)0xdeadbeef;
    hr = ID2D1EffectContext_CreateBlendTransform(effect_context, 0, &expected, &transform);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!transform, "Unexpected pointer %p.\n", transform);
    hr = ID2D1EffectContext_CreateBlendTransform(effect_context, 1, &expected, &transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1BlendTransform_Release(transform);

    hr = ID2D1EffectContext_CreateBlendTransform(effect_context, 4, &expected, &transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_interface(transform, &IID_ID2D1BlendTransform, TRUE);
    check_interface(transform, &IID_ID2D1ConcreteTransform, TRUE);
    check_interface(transform, &IID_ID2D1TransformNode, TRUE);

    /* Get description */
    ID2D1BlendTransform_GetDescription(transform, &blend_desc);
    check_blend_desc(&blend_desc, &expected);

    /* Input count */
    input_count = ID2D1BlendTransform_GetInputCount(transform);
    ok(input_count == 4, "Got unexpected input count %u.\n", input_count);

    /* Set output buffer */
    hr = ID2D1BlendTransform_SetOutputBuffer(transform, 0xdeadbeef, D2D1_CHANNEL_DEPTH_DEFAULT);
    todo_wine
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1BlendTransform_SetOutputBuffer(transform, D2D1_BUFFER_PRECISION_UNKNOWN, 0xdeadbeef);
    todo_wine
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1BlendTransform_SetOutputBuffer(transform, D2D1_BUFFER_PRECISION_UNKNOWN, D2D1_CHANNEL_DEPTH_DEFAULT);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Set description */
    expected.sourceBlend = D2D1_BLEND_ZERO;
    expected.destinationBlend = D2D1_BLEND_ZERO;
    expected.blendOperation = D2D1_BLEND_OPERATION_ADD;
    expected.sourceBlendAlpha = 0xdeadbeef;
    expected.destinationBlendAlpha = 0;
    expected.blendOperationAlpha = 12345678;
    expected.blendFactor[0] = 0.0f;
    expected.blendFactor[1] = 0.0f;
    expected.blendFactor[2] = 0.0f;
    expected.blendFactor[3] = 0.0f;
    ID2D1BlendTransform_SetDescription(transform, &expected);
    ID2D1BlendTransform_GetDescription(transform, &blend_desc);
    check_blend_desc(&blend_desc, &expected);

    ID2D1BlendTransform_Release(transform);
    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    release_test_context(&ctx);
}

static void test_border_transform(BOOL d3d11)
{
    ID2D1BorderTransform *transform = NULL;
    ID2D1EffectContext *effect_context;
    D2D1_PROPERTY_BINDING binding;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    D2D1_EXTEND_MODE mode;
    ID2D1Effect *effect;
    UINT input_count;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    binding.propertyName = L"Context";
    binding.setFunction = NULL;
    binding.getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_b, &binding, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Create transform with invalid extend mode */
    hr = ID2D1EffectContext_CreateBorderTransform(effect_context, 0xdeadbeef, 0xdeadbeef, &transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_interface(transform, &IID_ID2D1BorderTransform, TRUE);
    check_interface(transform, &IID_ID2D1ConcreteTransform, TRUE);
    check_interface(transform, &IID_ID2D1TransformNode, TRUE);

    mode = ID2D1BorderTransform_GetExtendModeX(transform);
    ok(mode == 0xdeadbeef, "Got unexpected extend mode %u.\n", mode);
    mode = ID2D1BorderTransform_GetExtendModeY(transform);
    ok(mode == 0xdeadbeef, "Got unexpected extend mode %u.\n", mode);
    ID2D1BorderTransform_Release(transform);

    /* Create transform */
    hr = ID2D1EffectContext_CreateBorderTransform(effect_context, D2D1_EXTEND_MODE_CLAMP, D2D1_EXTEND_MODE_WRAP, &transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Get extend mode */
    mode = ID2D1BorderTransform_GetExtendModeX(transform);
    ok(mode == D2D1_EXTEND_MODE_CLAMP, "Got unexpected extend mode %u.\n", mode);
    mode = ID2D1BorderTransform_GetExtendModeY(transform);
    ok(mode == D2D1_EXTEND_MODE_WRAP, "Got unexpected extend mode %u.\n", mode);

    /* Input count */
    input_count = ID2D1BorderTransform_GetInputCount(transform);
    ok(input_count == 1, "Got unexpected input count %u.\n", input_count);

    /* Set output buffer */
    hr = ID2D1BorderTransform_SetOutputBuffer(transform, 0xdeadbeef, D2D1_CHANNEL_DEPTH_DEFAULT);
    todo_wine
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1BorderTransform_SetOutputBuffer(transform, D2D1_BUFFER_PRECISION_UNKNOWN, 0xdeadbeef);
    todo_wine
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1BorderTransform_SetOutputBuffer(transform, D2D1_BUFFER_PRECISION_UNKNOWN, D2D1_CHANNEL_DEPTH_DEFAULT);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Set extend mode */
    ID2D1BorderTransform_SetExtendModeX(transform, D2D1_EXTEND_MODE_MIRROR);
    mode = ID2D1BorderTransform_GetExtendModeX(transform);
    ok(mode == D2D1_EXTEND_MODE_MIRROR, "Got unexpected extend mode %u.\n", mode);
    ID2D1BorderTransform_SetExtendModeY(transform, D2D1_EXTEND_MODE_CLAMP);
    mode = ID2D1BorderTransform_GetExtendModeY(transform);
    ok(mode == D2D1_EXTEND_MODE_CLAMP, "Got unexpected extend mode %u.\n", mode);

    /* Set extend mode with invalid value */
    ID2D1BorderTransform_SetExtendModeX(transform, 0xdeadbeef);
    mode = ID2D1BorderTransform_GetExtendModeX(transform);
    ok(mode == D2D1_EXTEND_MODE_MIRROR, "Got unexpected extend mode %u.\n", mode);
    ID2D1BorderTransform_SetExtendModeY(transform, 0xdeadbeef);
    mode = ID2D1BorderTransform_GetExtendModeY(transform);
    ok(mode == D2D1_EXTEND_MODE_CLAMP, "Got unexpected extend mode %u.\n", mode);

    ID2D1BorderTransform_Release(transform);
    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    release_test_context(&ctx);
}

static void test_bounds_adjustment_transform(BOOL d3d11)
{
    ID2D1BoundsAdjustmentTransform *transform = NULL;
    ID2D1EffectContext *effect_context;
    D2D1_PROPERTY_BINDING binding;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    ID2D1Effect *effect;
    UINT input_count;
    D2D1_RECT_L rect;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    binding.propertyName = L"Context";
    binding.setFunction = NULL;
    binding.getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_b, &binding, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_rect_l(&rect, -1, 0, 25, -50);
    hr = ID2D1EffectContext_CreateBoundsAdjustmentTransform(effect_context, &rect, &transform);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_interface(transform, &IID_ID2D1BoundsAdjustmentTransform, TRUE);
    check_interface(transform, &IID_ID2D1TransformNode, TRUE);

    memset(&rect, 0, sizeof(rect));
    ID2D1BoundsAdjustmentTransform_GetOutputBounds(transform, &rect);
    ok(rect.left == -1 && rect.top == 0 && rect.right == 25 && rect.bottom == -50,
            "Unexpected rectangle.\n");

    set_rect_l(&rect, -50, 25, 0, -1);
    ID2D1BoundsAdjustmentTransform_SetOutputBounds(transform, &rect);
    memset(&rect, 0, sizeof(rect));
    ID2D1BoundsAdjustmentTransform_GetOutputBounds(transform, &rect);
    ok(rect.left == -50 && rect.top == 25 && rect.right == 0 && rect.bottom == -1,
            "Unexpected rectangle.\n");

    /* Input count */
    input_count = ID2D1BoundsAdjustmentTransform_GetInputCount(transform);
    ok(input_count == 1, "Got unexpected input count %u.\n", input_count);

    ID2D1BoundsAdjustmentTransform_Release(transform);
    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    release_test_context(&ctx);
}

static void test_stroke_contains_point(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *rectangle;
    struct d2d1_test_context ctx;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1GeometrySink *sink;
    ID2D1PathGeometry *path;
    D2D1_POINT_2F point;
    D2D1_RECT_F rect;
    unsigned int i;
    BOOL contains;
    HRESULT hr;

    static const struct contains_point_test
    {
        D2D1_MATRIX_3X2_F transform;
        D2D1_POINT_2F point;
        float tolerance;
        float stroke_width;
        BOOL matrix;
        BOOL contains;
    }
    rectangle_tests[] =
    {
        /* 0. Stroked area hittesting. Edge. */
        {{{{0.0f}}}, { 0.4f, 10.0f},  0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, { 0.5f, 10.0f},  0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, { 0.6f, 10.0f},  0.0f, 1.0f, FALSE, FALSE},

        /* 3. Negative tolerance. */
        {{{{0.0f}}}, {-0.6f, 10.0f}, -1.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, { 0.6f, 10.0f}, -1.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, { 1.4f, 10.0f}, -1.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, { 1.5f, 10.0f}, -1.0f, 1.0f, FALSE, FALSE},

        /* 7. Within tolerance limit around corner. */
        {{{{0.0f}}}, {-D2D1_DEFAULT_FLATTENING_TOLERANCE - 1.00f, 0.0f}, 0.0f, 2.0f, FALSE, FALSE},
        {{{{0.0f}}}, {-D2D1_DEFAULT_FLATTENING_TOLERANCE - 1.01f, 0.0f}, 0.0f, 2.0f, FALSE, FALSE},
        {{{{0.0f}}}, {-D2D1_DEFAULT_FLATTENING_TOLERANCE, -D2D1_DEFAULT_FLATTENING_TOLERANCE},
                0.0f, 2.0f, FALSE, TRUE},
        {{{{0.0f}}}, {-D2D1_DEFAULT_FLATTENING_TOLERANCE, -D2D1_DEFAULT_FLATTENING_TOLERANCE - 1.01f},
                0.0f, 2.0f, FALSE, FALSE},

        /* 11. Center point. */
        {{{{0.0f}}}, { 5.0f, 10.0f},  0.0f, 1.0f, FALSE, FALSE},

        /* 12. Center point, large stroke width. */
        {{{{0.0f}}}, { 5.0f, 10.0f},  0.0f, 100.0f, FALSE, TRUE},

        /* 13. Center point, large tolerance. */
        {{{{0.0f}}}, { 5.0f, 10.0f}, 50.0f, 10.0f, FALSE, TRUE},

        /* With transformation matrix. */

        /* 14. */
        {{{{0.0f, 0.0f, 0.0f, 1.0f}}}, {0.1f, 10.0f},  0.0f, 1.0f, TRUE, FALSE},
        {{{{0.0f, 0.0f, 0.0f, 1.0f}}}, {5.0f, 10.0f},  5.0f, 1.0f, TRUE, FALSE},
        {{{{0.0f, 0.0f, 0.0f, 1.0f}}}, {4.9f, 10.0f},  5.0f, 1.0f, TRUE, TRUE},
        {{{{0.0f, 0.0f, 0.0f, 1.0f}}}, {5.0f, 10.0f}, -5.0f, 1.0f, TRUE, FALSE},
        {{{{0.0f, 0.0f, 0.0f, 1.0f}}}, {4.9f, 10.0f}, -5.0f, 1.0f, TRUE, TRUE},

        /* 19. */
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {0.0f, 10.0f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {0.1f, 10.0f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {0.5f, 10.0f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {0.0f, 10.0f}, 1.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {0.59f, 10.0f}, 1.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {-0.59f, 10.0f}, 1.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {0.59f, 10.0f}, -1.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f}}}, {-0.59f, 10.0f}, -1.0f, 1.0f, TRUE, TRUE},

        /* 27. */
        {{{{1.0f, 1.0f, 0.0f, 1.0f}}}, {5.0f, 4.4f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 1.0f, 0.0f, 1.0f}}}, {5.0f, 4.6f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 1.0f, 0.0f, 1.0f}}}, {5.0f, 5.4f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 1.0f, 0.0f, 1.0f}}}, {5.0f, 5.6f}, 0.0f, 1.0f, TRUE, FALSE},

        /* 31. */
        {{{{1.0f, 1.0f, 0.0f, 1.0f}}}, {5.0f, 6.9f}, 1.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 1.0f, 0.0f, 1.0f}}}, {5.0f, 7.0f}, 1.0f, 1.0f, TRUE, FALSE},

        /* 33. Offset matrix */
        {{{{1.0f, 0.0f, 0.0f, 1.0f, -11.0f,   0.0f}}}, { 5.0f,  20.0f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f,   0.0f,  21.0f}}}, {10.0f,  10.0f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f,  11.0f,   0.0f}}}, { 5.0f,   0.0f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f,   0.0f, -21.0f}}}, { 0.0f,  10.0f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, -11.0f,   0.0f}}}, {-6.0f,  20.0f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f,   0.0f,  21.0f}}}, {10.0f,  31.0f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f,  11.0f,   0.0f}}}, {16.0f,   0.0f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f,   0.0f, -21.0f}}}, { 0.0f, -11.0f}, 0.0f, 1.0f, TRUE, TRUE},
    },
    path_tests[] =
    {
        /* 0. Stroked area hittesting. Edge. */
        {{{{0.0f}}}, {160.0f,  600.0f},  0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {239.24f, 600.0f},  0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {239.26f, 600.0f},  0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {240.74f, 600.0f},  0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {240.76f, 600.0f},  0.0f, 1.0f, FALSE, FALSE},

        /* 5. Negative tolerance. */
        {{{{0.0f}}}, {239.24f, 600.0f}, -1.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {239.26f, 600.0f}, -1.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {240.74f, 600.0f}, -1.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {240.76f, 600.0f}, -1.0f, 1.0f, FALSE, FALSE},

        /* 9. Less than default tolerance. */
        {{{{0.0f}}}, {239.39f, 600.0f},  0.1f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {239.41f, 600.0f},  0.1f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {240.59f, 600.0f},  0.1f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {240.61f, 600.0f},  0.1f, 1.0f, FALSE, FALSE},

        /* 13. Curves. */
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {170.0f, 418.6f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {170.0f, 420.1f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {170.0f, 417.7f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, { 89.5f, 485.3f}, 0.1f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, { 90.5f, 485.3f}, 0.1f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, { 91.5f, 485.3f}, 0.1f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, { 89.0f, 485.3f}, 0.1f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {167.0f, 729.8f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {167.0f, 728.6f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {167.0f, 731.0f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {173.0f, 729.8f}, 0.0f, 1.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {173.0f, 728.6f}, 0.0f, 1.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 0.0f, 1.0f, 10.0f, 10.0f}}}, {173.0f, 731.0f}, 0.0f, 1.0f, TRUE, FALSE},

        /* 26. A curve where the control points project beyond the endpoints
         *     onto the line through the endpoints. */
        {{{{0.0f}}}, {306.97f, 791.67f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {307.27f, 791.67f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {308.47f, 791.67f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {308.77f, 791.67f}, 0.0f, 1.0f, FALSE, FALSE},

        {{{{0.0f}}}, {350.00f, 824.10f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {350.00f, 824.40f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {350.00f, 825.60f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {350.00f, 825.90f}, 0.0f, 1.0f, FALSE, FALSE},

        {{{{0.0f}}}, {391.23f, 791.67f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {391.53f, 791.67f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {392.73f, 791.67f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {393.03f, 791.67f}, 0.0f, 1.0f, FALSE, FALSE},

        /* 38. A curve where the endpoints coincide. */
        {{{{0.0f}}}, {570.23f, 799.77f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {570.53f, 799.77f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {571.73f, 799.77f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {572.03f, 799.77f}, 0.0f, 1.0f, FALSE, FALSE},

        {{{{0.0f}}}, {600.00f, 824.10f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {600.00f, 824.40f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {600.00f, 825.60f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {600.00f, 825.90f}, 0.0f, 1.0f, FALSE, FALSE},

        {{{{0.0f}}}, {627.97f, 799.77f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {628.27f, 799.77f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {629.47f, 799.77f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {629.77f, 799.77f}, 0.0f, 1.0f, FALSE, FALSE},

        /* 50. A curve with coinciding endpoints, as well as coinciding
         *     control points. */
        {{{{0.0f}}}, {825.00f, 800.00f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {861.00f, 824.00f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {861.00f, 826.00f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {862.50f, 825.00f}, 0.0f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {864.00f, 824.00f}, 0.0f, 1.0f, FALSE, FALSE},
        {{{{0.0f}}}, {864.00f, 826.00f}, 0.0f, 1.0f, FALSE, FALSE},

        /* 56. Shear transforms. */
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, { 837.2f, 600.0f}, 0.1f, 5.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, { 837.5f, 600.0f}, 0.1f, 5.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, {1186.3f, 791.7f}, 0.1f, 5.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, {1186.6f, 791.7f}, 0.1f, 5.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, {1425.0f, 827.3f}, 0.1f, 5.0f, TRUE, TRUE},
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, {1425.0f, 827.6f}, 0.1f, 5.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, {1620.1f, 800.0f}, 0.1f, 5.0f, TRUE, FALSE},
        {{{{1.0f, 0.0f, 1.0f, 1.0f}}}, {1620.4f, 800.0f}, 0.1f, 5.0f, TRUE, TRUE},
    },
    transformed_tests[] =
    {
        /* 0. Stroked area hittesting. Edge. Tolerance is 0.0f */
        {{{{0.0f}}}, {0.74f,  2.5f},  0.0f,   1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {0.75f,  2.5f},  0.0f,   1.0f, FALSE, FALSE},

        /* 2. Stroked area hittesting. Edge. Tolerance is negative */
        {{{{0.0f}}}, {0.74f,  2.5f}, -1.0f,   1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {0.75f,  2.5f}, -1.0f,   1.0f, FALSE, FALSE},

        /* 4. Stroked area hittesting. Edge. Tolerance is close to zero */
        {{{{0.0f}}}, {0.501f, 2.5f},  0.001f, 1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {0.502f, 2.5f},  0.001f, 1.0f, FALSE, FALSE},

        /* 6. Stroked area hittesting. Edge. Tolerance is D2D1_DEFAULT_FLATTENING_TOLERANCE */
        {{{{0.0f}}}, {0.74f,  2.5f},  0.25f,  1.0f, FALSE, TRUE},
        {{{{0.0f}}}, {0.75f,  2.5f},  0.25f,  1.0f, FALSE, FALSE},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    set_rect(&rect, 0.0f, 0.0f, 10.0f, 20.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &rectangle);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (i = 0; i < ARRAY_SIZE(rectangle_tests); ++i)
    {
        const struct contains_point_test *test = &rectangle_tests[i];

        winetest_push_context("Test %u", i);

        contains = !test->contains;
        hr = ID2D1RectangleGeometry_StrokeContainsPoint(rectangle, test->point, test->stroke_width,
                NULL, test->matrix ? &test->transform : NULL, test->tolerance, &contains);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(contains == test->contains, "Got unexpected result %#x.\n", contains);

        winetest_pop_context();
    }
    ID2D1RectangleGeometry_Release(rectangle);

    hr = ID2D1Factory_CreatePathGeometry(ctx.factory, &path);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1PathGeometry_Open(path, &sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* A limaçon. */
    set_point(&point, 160.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 119.0f, 720.0f,  83.0f, 600.0f,  80.0f, 474.0f);
    cubic_to(sink,  78.0f, 349.0f, 108.0f, 245.0f, 135.0f, 240.0f);
    cubic_to(sink, 163.0f, 235.0f, 180.0f, 318.0f, 176.0f, 370.0f);
    cubic_to(sink, 171.0f, 422.0f, 149.0f, 422.0f, 144.0f, 370.0f);
    cubic_to(sink, 140.0f, 318.0f, 157.0f, 235.0f, 185.0f, 240.0f);
    cubic_to(sink, 212.0f, 245.0f, 242.0f, 349.0f, 240.0f, 474.0f);
    cubic_to(sink, 238.0f, 600.0f, 201.0f, 720.0f, 160.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    /* Some straight lines. */
    set_point(&point, 160.0f, 240.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 240.0f, 240.0f);
    line_to(sink, 240.0f, 720.0f);
    line_to(sink, 160.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    /* Projected control points extending beyond the line segment through the
     * endpoints. */
    set_point(&point, 325.0f, 750.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 250.0f, 850.0f, 450.0f, 850.0f, 375.0f, 750.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    /* Coinciding endpoints. */
    set_point(&point, 600.0f, 750.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 500.0f, 850.0f, 700.0f, 850.0f, 600.0f, 750.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    /* Coinciding endpoints, as well as coinciding control points. */
    set_point(&point, 750.0f, 750.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 900.0f, 850.0f, 900.0f, 850.0f, 750.0f, 750.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    for (i = 0; i < ARRAY_SIZE(path_tests); ++i)
    {
        const struct contains_point_test *test = &path_tests[i];

        winetest_push_context("Test %u", i);

        contains = !test->contains;
        hr = ID2D1PathGeometry_StrokeContainsPoint(path, test->point, test->stroke_width,
                NULL, test->matrix ? &test->transform : NULL, test->tolerance, &contains);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(contains == test->contains, "Got unexpected result %#x.\n", contains);

        winetest_pop_context();
    }

    ID2D1PathGeometry_Release(path);

    set_rect(&rect, 0.0f, 0.0f, 5.0f, 5.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &rectangle);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 2.0f, 4.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(ctx.factory, (ID2D1Geometry *)rectangle, &matrix,
                &transformed_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(transformed_tests); ++i)
    {
        const struct contains_point_test *test = &transformed_tests[i];

        winetest_push_context("Test %u", i);

        contains = !test->contains;
        hr = ID2D1TransformedGeometry_StrokeContainsPoint(transformed_geometry, test->point,
                test->stroke_width, NULL, test->matrix ? &test->transform : NULL, test->tolerance,
                &contains);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(contains == test->contains, "Got unexpected result %#x.\n", contains);

        winetest_pop_context();
    }
    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1RectangleGeometry_Release(rectangle);

    release_test_context(&ctx);
}

static void test_image_bounds(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    D2D1_UNIT_MODE unit_mode;
    ID2D1Factory1 *factory;
    ID2D1Bitmap *bitmap;
    float dpi_x, dpi_y;
    D2D_RECT_F bounds;
    D2D1_SIZE_F size;
    unsigned int i;
    HRESULT hr;

    const struct bitmap_bounds_test
    {
        float dpi_x;
        float dpi_y;
        D2D_SIZE_U pixel_size;
    }
    bitmap_bounds_tests[] =
    {
        {96.0f,  96.0f,  {100, 100}},
        {48.0f,  48.0f,  {100, 100}},
        {192.0f, 192.0f, {100, 100}},
        {96.0f,  10.0f,  {100, 100}},
        {50.0f,  100.0f, {100, 100}},
        {150.0f, 150.0f, {100, 100}},
        {48.0f,  48.0f,  {1, 1}},
        {192.0f, 192.0f, {1, 1}},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    context = ctx.context;

    for (i = 0; i < ARRAY_SIZE(bitmap_bounds_tests); ++i)
    {
        const struct bitmap_bounds_test *test = &bitmap_bounds_tests[i];
        winetest_push_context("Test %u", i);

        bitmap_desc.dpiX = test->dpi_x;
        bitmap_desc.dpiY = test->dpi_y;
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        hr = ID2D1RenderTarget_CreateBitmap(ctx.rt, test->pixel_size, NULL, 0, &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        set_rect(&bounds, 0.0f, 0.0f, 0.0f, 0.0f);
        size = ID2D1Bitmap_GetSize(bitmap);
        hr = ID2D1DeviceContext_GetImageLocalBounds(context, (ID2D1Image *)bitmap, &bounds);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(compare_rect(&bounds, 0.0f, 0.0f, size.width, size.height, 0),
                "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                bounds.left, bounds.top, bounds.right, bounds.bottom, 0.0f, 0.0f, size.width, size.height);

        /* Test bitmap local bounds after changing context dpi */
        ID2D1DeviceContext_GetDpi(context, &dpi_x, &dpi_y);
        ID2D1DeviceContext_SetDpi(context, dpi_x * 2.0f, dpi_y * 2.0f);
        hr = ID2D1DeviceContext_GetImageLocalBounds(context, (ID2D1Image *)bitmap, &bounds);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(compare_rect(&bounds, 0.0f, 0.0f, size.width, size.height, 0),
                "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                bounds.left, bounds.top, bounds.right, bounds.bottom, 0.0f, 0.0f, size.width, size.height);
        ID2D1DeviceContext_SetDpi(context, dpi_x, dpi_y);

        /* Test bitmap local bounds after changing context unit mode */
        unit_mode = ID2D1DeviceContext_GetUnitMode(context);
        ok(unit_mode == D2D1_UNIT_MODE_DIPS, "Got unexpected unit mode %#x.\n", unit_mode);
        ID2D1DeviceContext_SetUnitMode(context, D2D1_UNIT_MODE_PIXELS);
        hr = ID2D1DeviceContext_GetImageLocalBounds(context, (ID2D1Image *)bitmap, &bounds);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(compare_rect(&bounds, 0.0f, 0.0f, test->pixel_size.width, test->pixel_size.height, 0),
                "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                bounds.left, bounds.top, bounds.right, bounds.bottom, 0.0f, 0.0f,
                (float)test->pixel_size.width, (float)test->pixel_size.height);
        ID2D1DeviceContext_SetUnitMode(context, unit_mode);

        ID2D1Bitmap_Release(bitmap);
        winetest_pop_context();
    }

    release_test_context(&ctx);
}

static void test_bitmap_map(BOOL d3d11)
{
    static const struct
    {
        unsigned int options;
    } invalid_map_options[] =
    {
        { D2D1_BITMAP_OPTIONS_NONE },
        { D2D1_BITMAP_OPTIONS_TARGET },
        { D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_TARGET },
        { D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE },
    };
    static const struct
    {
        unsigned int options;
    } valid_map_options[] =
    {
        { D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ },
    };
    static const struct
    {
        unsigned int bind_flags;
        unsigned int options;
        HRESULT hr;
    } options_tests[] =
    {
        { D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW },
        { D3D11_BIND_RENDER_TARGET, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW },
        { D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D2D1_BITMAP_OPTIONS_NONE },
        { 0, D2D1_BITMAP_OPTIONS_CANNOT_DRAW },

        { 0, D2D1_BITMAP_OPTIONS_TARGET, E_INVALIDARG },
        { 0, D2D1_BITMAP_OPTIONS_NONE, E_INVALIDARG },
        { D3D11_BIND_RENDER_TARGET, D2D1_BITMAP_OPTIONS_TARGET, E_INVALIDARG },
        { D3D11_BIND_RENDER_TARGET, D2D1_BITMAP_OPTIONS_NONE, E_INVALIDARG },
    };
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID2D1Bitmap *bitmap2, *bitmap3;
    struct d2d1_test_context ctx;
    D2D1_MAPPED_RECT mapped_rect;
    ID3D11Device *d3d_device;
    ID3D11Texture2D *texture;
    unsigned int i, options;
    IDXGISurface *surface;
    ID2D1Bitmap1 *bitmap;
    D2D1_SIZE_U size;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    for (i = 0; i < ARRAY_SIZE(invalid_map_options); ++i)
    {
        winetest_push_context("Test %u", i);

        set_size_u(&size, 4, 4);
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.bitmapOptions = invalid_map_options[i].options;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(ctx.context, size, NULL, 0, &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_NONE, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGISurface_QueryInterface(surface, &IID_ID3D11Texture2D, (void **)&texture);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID3D11Texture2D_GetDesc(texture, &texture_desc);
        ok(!texture_desc.CPUAccessFlags, "Unexpected CPU access flags %#x.\n", texture_desc.CPUAccessFlags);
        ID3D11Texture2D_Release(texture);
        IDXGISurface_Release(surface);

        ID2D1Bitmap1_Release(bitmap);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(valid_map_options); ++i)
    {
        winetest_push_context("Test %u", i);

        set_size_u(&size, 4, 4);
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.bitmapOptions = valid_map_options[i].options;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(ctx.context, size, NULL, 0, &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_NONE, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ, &mapped_rect);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ, &mapped_rect);
        ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE, &mapped_rect);
        todo_wine
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Bitmap1_Unmap(bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Unmap(bitmap);
        ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ | D2D1_MAP_OPTIONS_DISCARD, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE | D2D1_MAP_OPTIONS_DISCARD, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ | D2D1_MAP_OPTIONS_WRITE, &mapped_rect);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

        hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGISurface_QueryInterface(surface, &IID_ID3D11Texture2D, (void **)&texture);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID3D11Texture2D_GetDesc(texture, &texture_desc);
        ok(texture_desc.Usage == D3D11_USAGE_STAGING, "Unexpected usage %u.\n", texture_desc.Usage);
        ok(!texture_desc.BindFlags, "Unexpected bind flags %#x.\n", texture_desc.BindFlags);
        ok(texture_desc.CPUAccessFlags == D3D11_CPU_ACCESS_READ, "Unexpected CPU access flags %#x.\n",
                texture_desc.CPUAccessFlags);
        ok(!texture_desc.MiscFlags, "Unexpected misc flags %#x.\n", texture_desc.MiscFlags);
        ID3D11Texture2D_Release(texture);
        IDXGISurface_Release(surface);

        ID2D1Bitmap1_Release(bitmap);

        winetest_pop_context();
    }

    hr = IDXGIDevice_QueryInterface(ctx.device, &IID_ID3D11Device, (void **)&d3d_device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    texture_desc.Width = 4;
    texture_desc.Height = 4;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(d3d_device, &texture_desc, NULL, &texture);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ;
    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(ctx.context, surface, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ | D2D1_MAP_OPTIONS_DISCARD, &mapped_rect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ, &mapped_rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Bitmap1_Unmap(bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE, &mapped_rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Bitmap1_Unmap(bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE | D2D1_MAP_OPTIONS_READ, &mapped_rect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Bitmap1_Unmap(bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE | D2D1_MAP_OPTIONS_DISCARD, &mapped_rect);
    todo_wine
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) ID2D1Bitmap1_Unmap(bitmap);
    hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_WRITE | D2D1_MAP_OPTIONS_READ | D2D1_MAP_OPTIONS_DISCARD, &mapped_rect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    ID2D1Bitmap1_Release(bitmap);

    /* Options are derived from the surface properties. */
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = bitmap_desc.dpiY = 0.0f;
    hr = ID2D1DeviceContext_CreateSharedBitmap(ctx.context, &IID_IDXGISurface, surface,
            (const D2D1_BITMAP_PROPERTIES *)&bitmap_desc, &bitmap2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_bitmap_options(bitmap2, D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ);

    hr = ID2D1DeviceContext_CreateSharedBitmap(ctx.context, &IID_ID2D1Bitmap, bitmap2, NULL, &bitmap3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_bitmap_options(bitmap3, D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ);
    ID2D1Bitmap_Release(bitmap3);

    ID2D1Bitmap_Release(bitmap2);

    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(ctx.context, surface, NULL, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ),
            "Unexpected options %#x.\n", options);
    ID2D1Bitmap1_Release(bitmap);

    /* Options incompatible with the surface. */
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(ctx.context, surface, &bitmap_desc, &bitmap);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(ctx.context, surface, &bitmap_desc, &bitmap);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(ctx.context, surface, &bitmap_desc, &bitmap);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Create without D2D1_BITMAP_OPTIONS_CPU_READ, surface supports CPU reads. */
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(ctx.context, surface, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Bitmap1_Map(bitmap, D2D1_MAP_OPTIONS_READ, &mapped_rect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ID2D1Bitmap1_Release(bitmap);

    ID3D11Texture2D_Release(texture);
    IDXGISurface_Release(surface);

    for (i = 0; i < ARRAY_SIZE(options_tests); ++i)
    {
        winetest_push_context("Test %u", i);

        texture_desc.Width = 4;
        texture_desc.Height = 4;
        texture_desc.MipLevels = 1;
        texture_desc.ArraySize = 1;
        texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texture_desc.SampleDesc.Count = 1;
        texture_desc.SampleDesc.Quality = 0;
        texture_desc.Usage = D3D11_USAGE_DEFAULT;
        texture_desc.BindFlags = options_tests[i].bind_flags;
        texture_desc.CPUAccessFlags = 0;
        texture_desc.MiscFlags = 0;

        hr = ID3D11Device_CreateTexture2D(d3d_device, &texture_desc, NULL, &texture);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        bitmap = NULL;
        bitmap_desc.bitmapOptions = options_tests[i].options;
        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(ctx.context, surface, &bitmap_desc, &bitmap);
        ok(hr == options_tests[i].hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_bitmap_options((ID2D1Bitmap *)bitmap, options_tests[i].options);
            ID2D1Bitmap1_Release(bitmap);
        }

        ID3D11Texture2D_Release(texture);
        IDXGISurface_Release(surface);

        winetest_pop_context();
    }

    ID3D11Device_Release(d3d_device);

    release_test_context(&ctx);
}

static void test_bitmap_create(BOOL d3d11)
{
    static const struct
    {
        unsigned int options;
    } invalid_options[] =
    {
        { D2D1_BITMAP_OPTIONS_CANNOT_DRAW },
        { D2D1_BITMAP_OPTIONS_CPU_READ },
        { D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE },
    };
    static const struct
    {
        unsigned int options;
    } valid_options[] =
    {
        { D2D1_BITMAP_OPTIONS_NONE },
        { D2D1_BITMAP_OPTIONS_TARGET },
        { D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW },
        { D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE },
        { D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE },
        { D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ },
    };
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    struct d2d1_test_context ctx;
    ID2D1Bitmap1 *bitmap;
    D2D1_SIZE_U size;
    unsigned int i;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    for (i = 0; i < ARRAY_SIZE(invalid_options); ++i)
    {
        winetest_push_context("Test %u", i);

        set_size_u(&size, 4, 4);
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.bitmapOptions = invalid_options[i].options;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(ctx.context, size, NULL, 0, &bitmap_desc, &bitmap);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(valid_options); ++i)
    {
        winetest_push_context("Test %u", i);

        set_size_u(&size, 4, 4);
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bitmap_desc.bitmapOptions = valid_options[i].options;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(ctx.context, size, NULL, 0, &bitmap_desc, &bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID2D1Bitmap1_Release(bitmap);

        winetest_pop_context();
    }

    release_test_context(&ctx);
}

static void test_hwnd_target_is_supported(BOOL d3d11)
{
    static const unsigned int target_types[] =
    {
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1_RENDER_TARGET_TYPE_SOFTWARE,
        D2D1_RENDER_TARGET_TYPE_HARDWARE,
    };
    static const unsigned int usages[] =
    {
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING,
        D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
        D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING | D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
    };
    static const D2D1_PIXEL_FORMAT pixel_formats[] =
    {
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN },
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },

        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },

        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
    };
    static const D2D1_PIXEL_FORMAT unsupported_pixel_formats[] =
    {
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT },
        { DXGI_FORMAT_R16G16B16A16_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
    };
    D2D1_RENDER_TARGET_PROPERTIES desc, template_desc, test_desc;
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    ID2D1DeviceContext *device_context;
    D2D1_PIXEL_FORMAT pixel_format;
    struct d2d1_test_context ctx;
    ID2D1HwndRenderTarget *rt;
    BOOL expected, supported;
    ID2D1Bitmap1 *bitmap;
    unsigned int i, j;
    D2D1_SIZE_U size;
    HWND window;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    window = create_window();
    ok(!!window, "Failed to create test window.\n");

    template_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    template_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    template_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    template_desc.dpiX = 96.0f;
    template_desc.dpiY = 96.0f;
    template_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    template_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_rt_desc.hwnd = window;
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    /* Make sure the template render target description is valid */
    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &template_desc, &hwnd_rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    supported = ID2D1HwndRenderTarget_IsSupported(rt, &template_desc);
    ok(supported, "Expected supported.\n");
    ID2D1HwndRenderTarget_Release(rt);

    /* Test that SetTarget() with a bitmap of a different usage doesn't change the render target usage. */
    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &template_desc, &hwnd_rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1HwndRenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    test_desc = template_desc;
    test_desc.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
    supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);
    ok(!supported, "Expected unsupported.\n");

    memset(&bitmap_desc, 0, sizeof(bitmap_desc));
    bitmap_desc.pixelFormat = ID2D1DeviceContext_GetPixelFormat(device_context);
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE;
    size.width = 4;
    size.height = 4;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);
    ok(!supported, "Expected unsupported.\n");
    ID2D1Bitmap1_Release(bitmap);

    /* Test that SetTarget() with a bitmap of a different format changes the render target format */
    test_desc = template_desc;
    test_desc.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    test_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);
    ok(!supported, "Expected unsupported.\n");

    memset(&bitmap_desc, 0, sizeof(bitmap_desc));
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    size.width = 4;
    size.height = 4;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    pixel_format = ID2D1DeviceContext_GetPixelFormat(device_context);
    ok(pixel_format.format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected format %#x.\n", pixel_format.format);
    ok(pixel_format.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED, "Got unexpected alpha %u.\n", pixel_format.alphaMode);
    supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);
    ok(supported, "Expected supported.\n");
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_Release(device_context);
    ID2D1HwndRenderTarget_Release(rt);

    /* Target type. */
    for (i = 0; i < ARRAY_SIZE(target_types); ++i)
    {
        /* Default target type resolves to either HW or SW, there is no way to tell. */
        desc = template_desc;
        desc.type = target_types[i];
        hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
        if (desc.type == D2D1_RENDER_TARGET_TYPE_DEFAULT)
        {
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            test_desc = template_desc;
            test_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
            supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);
            ok(supported, "Unexpected return value %d.\n", supported);
        }
        else
        {
            if (FAILED(hr))
                continue;

            for (j = 0; j < ARRAY_SIZE(target_types); ++j)
            {
                winetest_push_context("type test %u/%u", i, j);

                test_desc = template_desc;
                test_desc.type = target_types[j];
                supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);
                expected = target_types[j] == D2D1_RENDER_TARGET_TYPE_DEFAULT
                        || target_types[i] == target_types[j];
                ok(supported == expected, "Unexpected return value %d.\n", supported);

                winetest_pop_context();
            }
        }

        ID2D1HwndRenderTarget_Release(rt);
    }

    /* Pixel formats. */
    for (i = 0; i < ARRAY_SIZE(pixel_formats); ++i)
    {
        desc = template_desc;
        desc.pixelFormat = pixel_formats[i];
        hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
        ok(hr == S_OK, "%u: unexpected hr %#lx.\n", i, hr);

        /* Resolve to default format. */
        if (desc.pixelFormat.format == DXGI_FORMAT_UNKNOWN)
            desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        if (desc.pixelFormat.alphaMode == D2D1_ALPHA_MODE_UNKNOWN)
            desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;

        for (j = 0; j < ARRAY_SIZE(pixel_formats); ++j)
        {
            BOOL format_supported, alpha_mode_supported;

            winetest_push_context("format test %u/%u.", i, j);

            test_desc = template_desc;
            test_desc.pixelFormat = pixel_formats[j];
            supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);

            format_supported = pixel_formats[j].format == DXGI_FORMAT_UNKNOWN
                    || pixel_formats[j].format == desc.pixelFormat.format;
            alpha_mode_supported = pixel_formats[j].alphaMode == D2D1_ALPHA_MODE_UNKNOWN
                    || pixel_formats[j].alphaMode == desc.pixelFormat.alphaMode;
            expected = format_supported && alpha_mode_supported;
            ok(supported == expected, "Unexpected return value.\n");

            winetest_pop_context();
        }

        for (j = 0; j < ARRAY_SIZE(unsupported_pixel_formats); ++j)
        {
            test_desc = template_desc;
            test_desc.pixelFormat = unsupported_pixel_formats[j];
            supported = ID2D1HwndRenderTarget_IsSupported(rt, &test_desc);
            ok(!supported, "Unexpected return value.\n");
        }

        ID2D1HwndRenderTarget_Release(rt);
    }

    /* Target usage. */
    for (i = 0; i < ARRAY_SIZE(usages); ++i)
    {
        desc = template_desc;
        desc.usage = usages[i];
        hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &rt);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        for (j = 0; j < ARRAY_SIZE(usages); ++j)
        {
            winetest_push_context("usage %#x testing usage %#x", usages[i], usages[j]);

            desc = template_desc;
            desc.usage = usages[j];
            supported = ID2D1HwndRenderTarget_IsSupported(rt, &desc);
            expected = (usages[i] & usages[j]) == usages[j];
            ok(supported == expected, "Unexpected result %d.\n", supported);

            winetest_pop_context();
        }

        ID2D1HwndRenderTarget_Release(rt);
    }

    DestroyWindow(window);

    release_test_context(&ctx);
}

static void test_dc_target_is_supported(BOOL d3d11)
{
    static const unsigned int target_types[] =
    {
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1_RENDER_TARGET_TYPE_SOFTWARE,
        D2D1_RENDER_TARGET_TYPE_HARDWARE,
    };
    static const unsigned int usages[] =
    {
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING,
        D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
        D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING | D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
    };
    static const D2D1_PIXEL_FORMAT pixel_formats[] =
    {
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN },
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT },

        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },

        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
    };
    static const D2D1_PIXEL_FORMAT unsupported_pixel_formats[] =
    {
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT },

        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT },

        { DXGI_FORMAT_R16G16B16A16_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
    };
    D2D1_RENDER_TARGET_PROPERTIES desc, template_desc, test_desc;
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    ID2D1DeviceContext *device_context;
    D2D1_PIXEL_FORMAT pixel_format;
    struct d2d1_test_context ctx;
    BOOL expected, supported;
    ID2D1DCRenderTarget *rt;
    ID2D1Bitmap1 *bitmap;
    unsigned int i, j;
    D2D1_SIZE_U size;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    template_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    template_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    template_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    template_desc.dpiX = 96.0f;
    template_desc.dpiY = 96.0f;
    template_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    template_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    /* Make sure the render target description template is valid. */
    hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &template_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    supported = ID2D1DCRenderTarget_IsSupported(rt, &template_desc);
    ok(supported, "Expected supported.\n");
    ID2D1DCRenderTarget_Release(rt);

    /* Test that SetTarget() with a bitmap of a different usage doesn't change the render target usage. */
    hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &template_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1DCRenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    test_desc = template_desc;
    test_desc.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
    supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);
    ok(supported, "Expected unsupported.\n");

    memset(&bitmap_desc, 0, sizeof(bitmap_desc));
    bitmap_desc.pixelFormat = ID2D1DeviceContext_GetPixelFormat(device_context);
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    size.width = 4;
    size.height = 4;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);
    ok(supported, "Expected unsupported.\n");
    ID2D1Bitmap1_Release(bitmap);

    /* Test that SetTarget() with a bitmap of a different format changes the render target format. */
    test_desc = template_desc;
    test_desc.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    test_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);
    ok(!supported, "Expected unsupported.\n");

    memset(&bitmap_desc, 0, sizeof(bitmap_desc));
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    size.width = 4;
    size.height = 4;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    pixel_format = ID2D1DeviceContext_GetPixelFormat(device_context);
    ok(pixel_format.format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected format %#x.\n", pixel_format.format);
    ok(pixel_format.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED, "Got unexpected alpha %u.\n", pixel_format.alphaMode);
    supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);
    ok(supported, "Expected supported.\n");
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_Release(device_context);
    ID2D1DCRenderTarget_Release(rt);

    /* Target type. */
    for (i = 0; i < ARRAY_SIZE(target_types); ++i)
    {
        /* Default target type resolves to either HW or SW, there is no way to tell. */
        desc = template_desc;
        desc.type = target_types[i];
        hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &rt);
        if (desc.type == D2D1_RENDER_TARGET_TYPE_DEFAULT)
        {
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            test_desc = template_desc;
            test_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
            supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);
            ok(supported, "Unexpected return value %d.\n", supported);
        }
        else
        {
            if (FAILED(hr))
                continue;

            for (j = 0; j < ARRAY_SIZE(target_types); ++j)
            {
                winetest_push_context("type test %u/%u", i, j);

                test_desc = template_desc;
                test_desc.type = target_types[j];
                supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);
                expected = target_types[j] == D2D1_RENDER_TARGET_TYPE_DEFAULT
                        || target_types[i] == target_types[j];
                ok(supported == expected, "Unexpected return value %d.\n", supported);

                winetest_pop_context();
            }
        }

        ID2D1DCRenderTarget_Release(rt);
    }

    /* Pixel formats. */
    for (i = 0; i < ARRAY_SIZE(pixel_formats); ++i)
    {
        desc = template_desc;
        desc.pixelFormat = pixel_formats[i];
        hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &rt);
        if (pixel_formats[i].format == DXGI_FORMAT_UNKNOWN
                || pixel_formats[i].alphaMode == D2D1_ALPHA_MODE_UNKNOWN)
        {
            ok(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT, "%u: unexpected hr %#lx.\n", i, hr);
            continue;
        }
        ok(hr == S_OK, "%u: unexpected hr %#lx.\n", i, hr);

        for (j = 0; j < ARRAY_SIZE(pixel_formats); ++j)
        {
            BOOL format_supported, alpha_mode_supported;

            winetest_push_context("format test %u/%u.", i, j);

            test_desc = template_desc;
            test_desc.pixelFormat = pixel_formats[j];
            supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);

            format_supported = pixel_formats[j].format == DXGI_FORMAT_UNKNOWN
                    || pixel_formats[j].format == desc.pixelFormat.format;
            alpha_mode_supported = pixel_formats[j].alphaMode == D2D1_ALPHA_MODE_UNKNOWN
                    || pixel_formats[j].alphaMode == desc.pixelFormat.alphaMode;
            expected = format_supported && alpha_mode_supported;
            ok(supported == expected, "Unexpected return value.\n");

            winetest_pop_context();
        }

        for (j = 0; j < ARRAY_SIZE(unsupported_pixel_formats); ++j)
        {
            winetest_push_context("unsupported format test %u/%u.", i, j);

            test_desc = template_desc;
            test_desc.pixelFormat = unsupported_pixel_formats[j];
            supported = ID2D1DCRenderTarget_IsSupported(rt, &test_desc);
            ok(!supported, "Unexpected return value.\n");

            winetest_pop_context();
        }

        ID2D1DCRenderTarget_Release(rt);
    }

    /* Target usage. */
    for (i = 0; i < ARRAY_SIZE(usages); ++i)
    {
        desc = template_desc;
        desc.usage = usages[i];
        hr = ID2D1Factory_CreateDCRenderTarget(ctx.factory, &desc, &rt);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        for (j = 0; j < ARRAY_SIZE(usages); ++j)
        {
            winetest_push_context("usage %#x testing usage %#x", usages[i], usages[j]);

            desc = template_desc;
            desc.usage = usages[j];
            supported = ID2D1DCRenderTarget_IsSupported(rt, &desc);
            expected = ((usages[i] | D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE) & usages[j]) == usages[j];
            ok(supported == expected, "Unexpected result %d.\n", supported);

            winetest_pop_context();
        }

        ID2D1DCRenderTarget_Release(rt);
    }

    release_test_context(&ctx);
}

static HRESULT STDMETHODCALLTYPE ps_effect_impl_Initialize(ID2D1EffectImpl *iface,
        ID2D1EffectContext *context, ID2D1TransformGraph *graph)
{
    struct effect_impl *effect_impl = impl_from_ID2D1EffectImpl(iface);

    effect_impl->effect_context = context;
    ID2D1EffectContext_AddRef(effect_impl->effect_context);

    return ID2D1TransformGraph_SetSingleTransformNode(graph,
            (ID2D1TransformNode *)&effect_impl->ID2D1DrawTransform_iface);
}

static const ID2D1EffectImplVtbl custom_effect_impl_vtbl =
{
    effect_impl_QueryInterface,
    effect_impl_AddRef,
    effect_impl_Release,
    ps_effect_impl_Initialize,
    effect_impl_PrepareForRender,
    effect_impl_SetGraph,
};

static HRESULT STDMETHODCALLTYPE effect_impl_draw_transform_QueryInterface(
        ID2D1DrawTransform *iface, REFIID iid, void **out)
{
    struct effect_impl *effect_impl = impl_from_ID2D1DrawTransform(iface);
    return ID2D1EffectImpl_QueryInterface(&effect_impl->ID2D1EffectImpl_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE effect_impl_draw_transform_AddRef(ID2D1DrawTransform *iface)
{
    struct effect_impl *effect_impl = impl_from_ID2D1DrawTransform(iface);
    return ID2D1EffectImpl_AddRef(&effect_impl->ID2D1EffectImpl_iface);
}

static ULONG STDMETHODCALLTYPE effect_impl_draw_transform_Release(ID2D1DrawTransform *iface)
{
    struct effect_impl *effect_impl = impl_from_ID2D1DrawTransform(iface);
    return ID2D1EffectImpl_Release(&effect_impl->ID2D1EffectImpl_iface);
}

static UINT32 STDMETHODCALLTYPE effect_impl_draw_transform_GetInputCount(ID2D1DrawTransform *iface)
{
    return 1;
}

static HRESULT STDMETHODCALLTYPE effect_impl_draw_transform_MapOutputRectToInputRects(
        ID2D1DrawTransform *iface, const D2D1_RECT_L *output_rect, D2D1_RECT_L *input_rects,
        UINT32 input_rect_count)
{
    if (input_rect_count != 1)
        return E_INVALIDARG;

    input_rects[0] = *output_rect;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_draw_transform_MapInputRectsToOutputRect(
        ID2D1DrawTransform *iface, const D2D1_RECT_L *input_rects, const D2D1_RECT_L *input_opaque_rects,
        UINT32 input_rect_count, D2D1_RECT_L *output_rect, D2D1_RECT_L *output_opaque_rect)
{
    if (input_rect_count != 1)
        return E_INVALIDARG;

    *output_rect = input_rects[0];
    memset(output_opaque_rect, 0, sizeof(*output_opaque_rect));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_impl_draw_transform_MapInvalidRect(
        ID2D1DrawTransform *iface, UINT32 index, D2D1_RECT_L input_rect, D2D1_RECT_L *output_rect)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE effect_impl_draw_transform_SetDrawInfo(ID2D1DrawTransform *iface,
        ID2D1DrawInfo *info)
{
    struct effect_impl *effect_impl = impl_from_ID2D1DrawTransform(iface);
    ID2D1VertexBuffer *buffer = NULL;
    HRESULT hr = S_OK;
    ULONG refcount;

    check_interface(info, &IID_IUnknown, TRUE);
    check_interface(info, &IID_ID2D1RenderInfo, TRUE);
    check_interface(info, &IID_ID2D1EffectContext, FALSE);

    refcount = get_refcount(info);
    ok(refcount == 1, "Unexpected refcount %lu.\n", refcount);

    if (effect_impl->pixel_shader)
        hr = ID2D1DrawInfo_SetPixelShader(info, effect_impl->pixel_shader, 0);

    if (SUCCEEDED(hr) && (effect_impl->vertex_buffer || effect_impl->vertex_shader))
    {
        if (effect_impl->vertex_buffer)
        {
            hr = ID2D1EffectContext_FindVertexBuffer(effect_impl->effect_context,
                    effect_impl->vertex_buffer, &buffer);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        }

        hr = ID2D1DrawInfo_SetVertexProcessing(info, buffer, 0, NULL, NULL, effect_impl->vertex_shader);
    }

    if (buffer)
        ID2D1VertexBuffer_Release(buffer);

    return hr;
}

static const ID2D1DrawTransformVtbl custom_shader_effect_draw_transform_vtbl =
{
    effect_impl_draw_transform_QueryInterface,
    effect_impl_draw_transform_AddRef,
    effect_impl_draw_transform_Release,
    effect_impl_draw_transform_GetInputCount,
    effect_impl_draw_transform_MapOutputRectToInputRects,
    effect_impl_draw_transform_MapInputRectsToOutputRect,
    effect_impl_draw_transform_MapInvalidRect,
    effect_impl_draw_transform_SetDrawInfo,
};

static HRESULT create_custom_shader_effect(IUnknown **effect_impl, const GUID *pixel_shader,
        const GUID *vertex_shader, const GUID *vertex_buffer)
{
    struct effect_impl *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1EffectImpl_iface.lpVtbl = &custom_effect_impl_vtbl;
    object->ID2D1DrawTransform_iface.lpVtbl = &custom_shader_effect_draw_transform_vtbl;
    object->refcount = 1;
    object->pixel_shader = pixel_shader;
    object->vertex_shader = vertex_shader;
    object->vertex_buffer = vertex_buffer;

    *effect_impl = (IUnknown *)&object->ID2D1EffectImpl_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE custom_effect_impl_create(IUnknown **effect_impl)
{
    return create_custom_shader_effect(effect_impl, &GUID_TestPixelShader, NULL, NULL);
}

static void preload_shader(const char *target, const char *code, const GUID *id,
        struct d2d1_test_context *ctx)
{
    D2D1_PROPERTY_BINDING bindings[1] = { 0 };
    ID2D1EffectContext *effect_context;
    ID2D1DeviceContext *context;
    ID2D1Factory1 *factory;
    ID2D1Effect *effect;
    ID3D10Blob *blob;
    HRESULT hr;

    context = ctx->context;
    factory = ctx->factory1;

    bindings[0].propertyName = L"Context";
    bindings[0].getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_AuxEffect, effect_xml_b,
            bindings, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(context, &CLSID_AuxEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_GetValueByName(effect, L"Context",
            D2D1_PROPERTY_TYPE_IUNKNOWN, (BYTE *)&effect_context, sizeof(effect_context));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DCompile(code, strlen(code), NULL, NULL, NULL, "main",
            target, 0, 0, &blob, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (!strcmp(target, "ps_4_0"))
    {
        hr = ID2D1EffectContext_LoadPixelShader(effect_context, id,
                ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    else
        ok(0, "Unexpected target %s.\n", target);

    ID3D10Blob_Release(blob);

    ID2D1Effect_Release(effect);

    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_AuxEffect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_effect_custom_pixel_shader(BOOL d3d11)
{
    static const WCHAR *description =
        L"<?xml version='1.0'?>                                                       \
            <Effect>                                                                  \
                <Property name='DisplayName' type='string' value='PSEffect'/>         \
                <Property name='Author'      type='string' value='The Wine Project'/> \
                <Property name='Category'    type='string' value='Test'/>             \
                <Property name='Description' type='string' value='Test effect.'/>     \
                <Inputs>                                                              \
                    <Input name='Source'/>                                            \
                </Inputs>                                                             \
                <Property name='Context' type='iunknown'>                             \
                    <Property name='DisplayName' type='string' value='Context'/>      \
                </Property>                                                           \
            </Effect>                                                                 \
        ";

    static const char ps_code[] =
        "float4 main() : sv_target\n"
        "{\n"
        "    return float4(0.1, 0.2, 0.3, 0.4);\n"
        "}";

    D2D1_PROPERTY_BINDING bindings[1] = { 0 };
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    DWORD colour, expected_colour;
    struct d2d1_test_context ctx;
    struct resource_readback rb;
    ID2D1DeviceContext *context;
    D2D1_SIZE_U input_size;
    ID2D1Factory1 *factory;
    ID2D1Bitmap1 *bitmap;
    ID2D1Effect *effect;
    ID2D1Image *output;
    DWORD pixel;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    context = ctx.context;
    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    preload_shader("ps_4_0", ps_code, &GUID_TestPixelShader, &ctx);

    bindings[0].propertyName = L"Context";
    bindings[0].getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect, description, bindings,
            ARRAY_SIZE(bindings), custom_effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_size_u(&input_size, 1, 1);
    pixel = 0xabcd00ff;
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
    bitmap_desc.colorContext = NULL;
    hr = ID2D1DeviceContext_CreateBitmap(context, input_size, &pixel, sizeof(pixel),
            &bitmap_desc, &bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1Effect_SetInput(effect, 0, (ID2D1Image *)bitmap, FALSE);
    ID2D1Effect_GetOutput(effect, &output);

    ID2D1DeviceContext_BeginDraw(context);
    ID2D1DeviceContext_Clear(context, 0);
    ID2D1DeviceContext_DrawImage(context, output, NULL, NULL, 0, 0);
    hr = ID2D1DeviceContext_EndDraw(context, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    get_surface_readback(&ctx, &rb);
    colour = get_readback_colour(&rb, 0, 0);
    expected_colour = 0x661a334c;
    todo_wine ok(compare_colour(colour, expected_colour, 1),
            "Got unexpected colour %#lx, expected %#lx.\n", colour, expected_colour);
    release_resource_readback(&rb);

    ID2D1Image_Release(output);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1Effect_Release(effect);

    release_test_context(&ctx);
}

static void test_get_effect_properties(BOOL d3d11)
{
    ID2D1Properties *properties, *properties2;
    struct d2d1_test_context ctx;
    D2D_VECTOR_4F rect, rect2;
    D2D1_PROPERTY_TYPE type;
    ID2D1Factory1 *factory;
    UINT32 count, index;
    WCHAR buffW[64];
    ULONG refcount;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1Factory1_GetEffectProperties(factory, &GUID_NULL, &properties);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Unexpected hr %#lx.\n", hr);

    hr = ID2D1Factory1_GetEffectProperties(factory, &CLSID_D2D1Crop, &properties);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    refcount = get_refcount(properties);
    ok(refcount == 2, "Unexpected refcount %lu.\n", refcount);

    hr = ID2D1Factory1_GetEffectProperties(factory, &CLSID_D2D1Crop, &properties2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(properties == properties2, "Unexpected instance.\n");
    refcount = get_refcount(properties);
    ok(refcount == 3, "Unexpected refcount %lu.\n", refcount);
    ID2D1Properties_Release(properties2);

    count = ID2D1Properties_GetPropertyCount(properties);
    todo_wine
    ok(count == 2, "Unexpected property count %u.\n", count);

    hr = ID2D1Properties_GetPropertyName(properties, 0, buffW, ARRAY_SIZE(buffW));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buffW, L"Rect"), "Unexpected name %s.\n", debugstr_w(buffW));

    count = ID2D1Properties_GetPropertyNameLength(properties, 0);
    ok(count == 4, "Unexpected name length %u.\n", count);

    type = ID2D1Properties_GetType(properties, 0);
    ok(type == D2D1_PROPERTY_TYPE_VECTOR4, "Unexpected property type %u.\n", type);

    index = ID2D1Properties_GetPropertyIndex(properties, L"prop");
    ok(index == ~0u, "Unexpected index %u.\n", index);

    set_vec4(&rect, 0.0f, 2.0f, 10.0f, 20.0f);
    hr = ID2D1Properties_SetValue(properties, D2D1_CROP_PROP_RECT, D2D1_PROPERTY_TYPE_VECTOR4,
            (const BYTE *)&rect, sizeof(rect));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ID2D1Properties_SetValue(properties, 1000, D2D1_PROPERTY_TYPE_VECTOR4,
            (const BYTE *)&rect, sizeof(rect));
    ok(hr == D2DERR_INVALID_PROPERTY, "Unexpected hr %#lx.\n", hr);

    set_vec4(&rect2, 1.0f, 2.0f, 3.0f, 4.0f);
    set_vec4(&rect, 0.0f, .0f, 0.0f, 0.0f);
    hr = ID2D1Properties_GetValue(properties, D2D1_CROP_PROP_RECT, D2D1_PROPERTY_TYPE_VECTOR4,
            (BYTE *)&rect2, sizeof(rect2));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Unexpected value.\n");

    ID2D1Properties_Release(properties);

    release_test_context(&ctx);
}

static void create_effect(ID2D1DeviceContext *device_context, const GUID *id,
        ID2D1EffectContext **effect_context, ID2D1Effect **effect)
{
    HRESULT hr;

    hr = ID2D1DeviceContext_CreateEffect(device_context, id, effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (effect_context)
    {
        *effect_context = NULL;
        hr = ID2D1Effect_GetValueByName(*effect, L"Context", D2D1_PROPERTY_TYPE_IUNKNOWN,
                (BYTE *)effect_context, sizeof*(effect_context));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }
}

static void test_effect_vertex_buffer(BOOL d3d11)
{
    static const char custom_vs_code[] =
        "struct vs_out"
        "{"
        "    float4 clipSpaceOutput  : SV_POSITION;"
        "    float4 sceneSpaceOutput : SCENE_POSITION;"
        "    float4 texelSpaceInput0 : TEXCOORD0;"
        "};"
        "vs_out main(float2 position : CUSTOM_POSITION)"
        "{"
        "    vs_out output = (vs_out)0;"
        "    return output;"
        "}";

    static const D2D1_INPUT_ELEMENT_DESC custom_layout[] =
    {
        {"CUSTOM_POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0},
    };

    D2D1_CUSTOM_VERTEX_BUFFER_PROPERTIES custom_buffer_desc;
    D2D1_VERTEX_BUFFER_PROPERTIES buffer_desc;
    ID2D1VertexBuffer *buffer, *buffer2;
    ID2D1EffectContext *effect_context;
    ID2D1DeviceContext *device_context;
    ID2D1Effect *effect, *effect2;
    D2D1_PROPERTY_BINDING binding;
    struct d2d1_test_context ctx;
    D2D_VECTOR_4F data[3 * 6];
    ID2D1Factory1 *factory;
    ID2D1Device *device;
    BYTE *ptr, *ptr2;
    ID3D10Blob *vs;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    binding.propertyName = L"Context";
    binding.setFunction = NULL;
    binding.getFunction = effect_impl_get_context;
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_b, &binding, 1, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    create_effect(ctx.context, &CLSID_TestEffect, &effect_context, &effect);

    buffer_desc.inputCount = 1;
    buffer_desc.usage = D2D1_VERTEX_USAGE_STATIC;
    buffer_desc.data = (const BYTE *)data;
    buffer_desc.byteWidth = sizeof(data);

    hr = ID2D1EffectContext_CreateVertexBuffer(effect_context, &buffer_desc, NULL, NULL, &buffer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1EffectContext_CreateVertexBuffer(effect_context, &buffer_desc, NULL, NULL, &buffer2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(buffer != buffer2, "Unexpected buffer instance.\n");
    ID2D1VertexBuffer_Release(buffer2);

    /* Mapping static buffer. */
    ptr = NULL;
    hr = ID2D1VertexBuffer_Map(buffer, &ptr, buffer_desc.byteWidth);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!!ptr, "Unexpected pointer.\n");
    hr = ID2D1VertexBuffer_Unmap(buffer);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1VertexBuffer_Map(buffer, &ptr, buffer_desc.byteWidth + 1);
    todo_wine
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1VertexBuffer_Map(buffer, &ptr, buffer_desc.byteWidth - 1);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!!ptr, "Unexpected pointer.\n");
    hr = ID2D1VertexBuffer_Unmap(buffer);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Map already mapped. */
    hr = ID2D1VertexBuffer_Map(buffer, &ptr, buffer_desc.byteWidth);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!!ptr, "Unexpected pointer.\n");
    ptr2 = NULL;
    hr = ID2D1VertexBuffer_Map(buffer, &ptr2, buffer_desc.byteWidth);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == ptr2, "Unexpected pointer.\n");
    hr = ID2D1VertexBuffer_Unmap(buffer);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1VertexBuffer_Release(buffer);

    /* With an id. */
    buffer = (void *)0xdeadbeef;
    hr = ID2D1EffectContext_FindVertexBuffer(effect_context, &GUID_NULL, &buffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);
    ok(!buffer, "Unexpected pointer %p.\n", buffer);

    hr = ID2D1EffectContext_CreateVertexBuffer(effect_context, &buffer_desc, &GUID_NULL, NULL, &buffer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1EffectContext_FindVertexBuffer(effect_context, &GUID_NULL, &buffer2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(buffer == buffer2, "Unexpected buffer instance.\n");
    ID2D1VertexBuffer_Release(buffer2);

    /* Try to create with the same id.*/
    hr = ID2D1EffectContext_CreateVertexBuffer(effect_context, &buffer_desc, &GUID_NULL, NULL, &buffer2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(buffer == buffer2, "Unexpected buffer instance.\n");
    ID2D1VertexBuffer_Release(buffer2);

    buffer_desc.usage = D2D1_VERTEX_USAGE_DYNAMIC;
    hr = ID2D1EffectContext_CreateVertexBuffer(effect_context, &buffer_desc, &GUID_NULL, NULL, &buffer2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(buffer == buffer2, "Unexpected buffer instance.\n");
    ID2D1VertexBuffer_Release(buffer2);

    ID2D1VertexBuffer_Release(buffer);

    /* Custom input layout. */
    hr = D3DCompile(custom_vs_code, sizeof(custom_vs_code) - 1, "test_vs", NULL, NULL,
            "main", "vs_4_0", 0, 0, &vs, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    custom_buffer_desc.elementCount = ARRAYSIZE(custom_layout);
    custom_buffer_desc.inputElements = custom_layout;
    custom_buffer_desc.stride = sizeof(D2D_VECTOR_2F);
    custom_buffer_desc.shaderBufferWithInputSignature = ID3D10Blob_GetBufferPointer(vs);
    custom_buffer_desc.shaderBufferSize = ID3D10Blob_GetBufferSize(vs);

    hr = ID2D1EffectContext_CreateVertexBuffer(effect_context, &buffer_desc,
            &GUID_CustomVertexBuffer, &custom_buffer_desc, &buffer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1EffectContext_FindVertexBuffer(effect_context, &GUID_CustomVertexBuffer, &buffer2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1VertexBuffer_Release(buffer2);

    ID2D1VertexBuffer_Release(buffer);

    ID3D10Blob_Release(vs);

    /* Buffer is not accessible using different device. */
    hr = ID2D1Factory1_CreateDevice(factory, ctx.device, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    create_effect(device_context, &CLSID_TestEffect, &effect_context, &effect2);

    hr = ID2D1EffectContext_FindVertexBuffer(effect_context, &GUID_CustomVertexBuffer, &buffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    ID2D1Effect_Release(effect2);

    ID2D1DeviceContext_Release(device_context);
    ID2D1Device_Release(device);

    /* Using same device and different device context. */
    ID2D1DeviceContext_GetDevice(ctx.context, &device);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    create_effect(device_context, &CLSID_TestEffect, &effect_context, &effect2);

    hr = ID2D1EffectContext_FindVertexBuffer(effect_context, &GUID_CustomVertexBuffer, &buffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    ID2D1Effect_Release(effect2);

    ID2D1DeviceContext_Release(device_context);

    ID2D1Device_Release(device);

    ID2D1Effect_Release(effect);
    hr = ID2D1Factory1_UnregisterEffect(factory, &CLSID_TestEffect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    release_test_context(&ctx);
}

static void test_compute_geometry_area(BOOL d3d11)
{
    ID2D1RectangleGeometry *rectangle_geometry;
    ID2D1EllipseGeometry *ellipse_geometry;
    struct d2d1_test_context ctx;
    D2D1_MATRIX_3X2_F matrix;
    D2D1_ELLIPSE ellipse;
    D2D1_RECT_F rect;
    HRESULT hr;
    float area;

    if (!init_test_context(&ctx, d3d11))
        return;

    /* Ellipse */
    set_ellipse(&ellipse, 0.0f, 0.0f, 10.0f, 5.0f);
    hr = ID2D1Factory_CreateEllipseGeometry(ctx.factory, &ellipse, &ellipse_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1EllipseGeometry_ComputeArea(ellipse_geometry, NULL, 0.01f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 156.9767f, 0), "Unexpected value %.8e.\n", area);

    hr = ID2D1EllipseGeometry_ComputeArea(ellipse_geometry, NULL, 200.0f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 100.0f, 0), "Unexpected value %.8e.\n", area);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 1.0f, 2.0f);
    hr = ID2D1EllipseGeometry_ComputeArea(ellipse_geometry, &matrix, 0.01f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 314.12088f, 0), "Unexpected value %.8e.\n", area);

    hr = ID2D1EllipseGeometry_ComputeArea(ellipse_geometry, &matrix, 200.0f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 200.0f, 0), "Unexpected value %.8e.\n", area);

    ID2D1EllipseGeometry_Release(ellipse_geometry);

    /* Rectangle */
    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(ctx.factory, &rect, &rectangle_geometry);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1RectangleGeometry_ComputeArea(rectangle_geometry, NULL, 0.01f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 4.0f, 0), "Unexpected value %.8e.\n", area);

    hr = ID2D1RectangleGeometry_ComputeArea(rectangle_geometry, NULL, 200.0f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 4.0f, 0), "Unexpected value %.8e.\n", area);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 1.0f, 2.0f);
    hr = ID2D1RectangleGeometry_ComputeArea(rectangle_geometry, &matrix, 0.01f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 8.0f, 0), "Unexpected value %.8e.\n", area);

    rotate_matrix(&matrix, 0.5f);
    hr = ID2D1RectangleGeometry_ComputeArea(rectangle_geometry, &matrix, 200.0f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 8.0f, 0), "Unexpected value %.8e.\n", area);

    skew_matrix(&matrix, 0.1f, 1.5f);
    hr = ID2D1RectangleGeometry_ComputeArea(rectangle_geometry, &matrix, 200.0f, &area);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_float(area, 6.8f, 0), "Unexpected value %.8e.\n", area);

    ID2D1RectangleGeometry_Release(rectangle_geometry);

    release_test_context(&ctx);
}

static void test_wic_target_format(BOOL d3d11)
{
    static const struct
    {
        D2D1_PIXEL_FORMAT pixel_format;
        const GUID *wic_format;
        HRESULT hr;
    }
    wic_target_formats[] =
    {
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
                &GUID_WICPixelFormat32bppPBGRA },
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
                &GUID_WICPixelFormat32bppBGR },
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppPBGRA },
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppBGR },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                &GUID_WICPixelFormat32bppPBGRA },
        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
                &GUID_WICPixelFormat32bppBGR },
        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppPBGRA },
        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppBGR },

        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
                &GUID_WICPixelFormat32bppPRGBA },
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
                &GUID_WICPixelFormat32bppRGB },
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppPRGBA },
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppRGB },

        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                &GUID_WICPixelFormat32bppPRGBA },
        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
                &GUID_WICPixelFormat32bppRGB },
        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppPRGBA },
        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppRGB },

        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                &GUID_WICPixelFormat32bppPBGRA, E_INVALIDARG },
        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
                &GUID_WICPixelFormat32bppBGR, E_INVALIDARG },
        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppPBGRA, E_INVALIDARG },
        { { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
                &GUID_WICPixelFormat32bppBGR, E_INVALIDARG },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                &GUID_WICPixelFormat32bppBGR, E_INVALIDARG },
        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
                &GUID_WICPixelFormat32bppPBGRA, E_INVALIDARG },
    };
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    IWICImagingFactory *wic_factory;
    struct d2d1_test_context ctx;
    IWICBitmap *wic_bitmap;
    ID2D1RenderTarget *rt;
    unsigned int i;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(wic_target_formats); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
                wic_target_formats[i].wic_format, WICBitmapCacheOnDemand, &wic_bitmap);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        rt_desc.pixelFormat = wic_target_formats[i].pixel_format;
        rt_desc.dpiX = 96.0f;
        rt_desc.dpiY = 96.0f;
        rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
        rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
        hr = ID2D1Factory_CreateWicBitmapRenderTarget(ctx.factory, wic_bitmap, &rt_desc, &rt);
        todo_wine_if(FAILED(wic_target_formats[i].hr))
        ok(hr == wic_target_formats[i].hr, "Got unexpected hr %#lx.\n", hr);

        IWICBitmap_Release(wic_bitmap);

        if (SUCCEEDED(hr))
             ID2D1RenderTarget_Release(rt);

        winetest_pop_context();
    }
    IWICImagingFactory_Release(wic_factory);

    CoUninitialize();
    release_test_context(&ctx);
}

static void test_effect_blob_property(BOOL d3d11)
{
    static const D2D1_PROPERTY_BINDING bindings[] =
    {
        { L"BlobProp", effect_impl_set_blob, effect_impl_get_blob },
    };
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    ID2D1Effect *effect;
    UINT32 index, size;
    BYTE buffer[64];
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    factory = ctx.factory1;
    if (!factory)
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect,
            effect_xml_a, bindings, ARRAY_SIZE(bindings), effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    index = ID2D1Effect_GetPropertyIndex(effect, L"BlobProp");
    ok(index != D2D1_INVALID_PROPERTY_INDEX, "Invalid property index.\n");

    size = ID2D1Effect_GetValueSize(effect, index);
    ok(!size, "Unexpected property size %u.\n", size);

    hr = ID2D1Effect_SetValue(effect, index, D2D1_PROPERTY_TYPE_BLOB,
            (const BYTE *)"blob-value", 11);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID2D1Effect_GetValueSize(effect, index);
    ok(size == 11, "Unexpected property size %u.\n", size);

    ID2D1Effect_Release(effect);

    /* Without property binding. */
    hr = ID2D1Factory1_RegisterEffectFromString(factory, &CLSID_TestEffect2,
            effect_xml_a, NULL, 0, effect_impl_create);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1DeviceContext_CreateEffect(ctx.context, &CLSID_TestEffect2, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    index = ID2D1Effect_GetPropertyIndex(effect, L"BlobProp");
    ok(index != D2D1_INVALID_PROPERTY_INDEX, "Invalid property index.\n");

    size = ID2D1Effect_GetValueSize(effect, index);
    ok(!size, "Unexpected property size %u.\n", size);

    hr = ID2D1Effect_SetValue(effect, index, D2D1_PROPERTY_TYPE_BLOB,
            (const BYTE *)"blob-value", 11);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    size = ID2D1Effect_GetValueSize(effect, index);
    ok(!size, "Unexpected property size %u.\n", size);

    memset(buffer, 0xa, sizeof(buffer));
    hr = ID2D1Effect_GetValue(effect, index, D2D1_PROPERTY_TYPE_BLOB, buffer, 4);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!*buffer, "Unexpected buffer contents.\n");
    ok(!*(buffer+1), "Unexpected buffer contents.\n");
    ok(!*(buffer+2), "Unexpected buffer contents.\n");
    ok(!*(buffer+3), "Unexpected buffer contents.\n");
    ok(*(buffer+4) == 0xa, "Unexpected buffer contents.\n");

    hr = ID2D1Effect_SetValue(effect, index, D2D1_PROPERTY_TYPE_BLOB,
            (const BYTE *)"new-value", 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Effect_SetValue(effect, index, D2D1_PROPERTY_TYPE_BLOB,
            (const BYTE *)"new-value", 1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    size = ID2D1Effect_GetValueSize(effect, index);
    ok(!size, "Unexpected property size %u.\n", size);

    ID2D1Effect_Release(effect);

    release_test_context(&ctx);
}

static void test_get_dxgi_device(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1BitmapRenderTarget *bitmap_rt;
    IWICImagingFactory *wic_factory;
    ID2D1HwndRenderTarget *hwnd_rt;
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    IDXGIDevice *dxgi_device;
    D2D1_SIZE_U pixel_size;
    IWICBitmap *wic_bitmap;
    ID2D1Device2 *device2;
    ID2D1RenderTarget *rt;
    ID2D1Device *device;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (!ctx.factory3)
    {
        win_skip("ID2D1Factory3 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    /* Test user-provided DXGI device */
    hr = ID2D1Factory3_CreateDevice(ctx.factory3, ctx.device, &device2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Device2_GetDxgiDevice(device2, &dxgi_device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(dxgi_device == ctx.device, "Got unexpected IDXGIDevice.\n");

    IDXGIDevice_Release(dxgi_device);
    ID2D1Device2_Release(device2);

    /* WIC target */
    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rt_desc.dpiX = 96.0f;
    rt_desc.dpiY = 96.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16, &GUID_WICPixelFormat32bppPBGRA,
            WICBitmapCacheOnDemand, &wic_bitmap);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1Factory1_CreateWicBitmapRenderTarget(ctx.factory1, wic_bitmap, &rt_desc, &rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetDevice(context, &device);
    hr = ID2D1Device_QueryInterface(device, &IID_ID2D1Device2, (void **)&device2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    dxgi_device = (IDXGIDevice *)0xdeadbeef;
    hr = ID2D1Device2_GetDxgiDevice(device2, &dxgi_device);
    ok(hr == D2DERR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ok(!dxgi_device, "Expected NULL DXGI device.\n");

    ID2D1Device2_Release(device2);
    ID2D1Device_Release(device);
    ID2D1DeviceContext_Release(context);
    ID2D1RenderTarget_Release(rt);
    IWICBitmap_Release(wic_bitmap);
    IWICImagingFactory_Release(wic_factory);
    CoUninitialize();

    /* HWND target */
    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_rt_desc.hwnd = create_window();
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    hr = ID2D1Factory_CreateHwndRenderTarget(ctx.factory, &desc, &hwnd_rt_desc, &hwnd_rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1HwndRenderTarget_QueryInterface(hwnd_rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetDevice(context, &device);
    hr = ID2D1Device_QueryInterface(device, &IID_ID2D1Device2, (void **)&device2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    dxgi_device = (IDXGIDevice *)0xdeadbeef;
    hr = ID2D1Device2_GetDxgiDevice(device2, &dxgi_device);
    ok(hr == D2DERR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ok(!dxgi_device, "Expected NULL DXGI device.\n");

    ID2D1Device2_Release(device2);
    ID2D1Device_Release(device);
    ID2D1DeviceContext_Release(context);
    ID2D1HwndRenderTarget_Release(hwnd_rt);
    DestroyWindow(hwnd_rt_desc.hwnd);

    /* DXGI surface target */
    ID2D1DeviceContext_GetDevice(ctx.context, &device);
    hr = ID2D1Device_QueryInterface(device, &IID_ID2D1Device2, (void **)&device2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID2D1Device2_GetDxgiDevice(device2, &dxgi_device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(dxgi_device == ctx.device, "Got unexpected IDXGIDevice.\n");

    IDXGIDevice_Release(dxgi_device);
    ID2D1Device2_Release(device2);
    ID2D1Device_Release(device);

    /* DXGI compatible bitmap target */
    set_size_u(&pixel_size, 32, 32);
    hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(ctx.rt, NULL, &pixel_size, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &bitmap_rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID2D1BitmapRenderTarget_QueryInterface(bitmap_rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetDevice(context, &device);
    hr = ID2D1Device_QueryInterface(device, &IID_ID2D1Device2, (void **)&device2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    dxgi_device = (IDXGIDevice *)0xdeadbeef;
    hr = ID2D1Device2_GetDxgiDevice(device2, &dxgi_device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(dxgi_device == ctx.device, "Got unexpected IDXGIDevice.\n");

    IDXGIDevice_Release(dxgi_device);
    ID2D1Device2_Release(device2);
    ID2D1Device_Release(device);
    ID2D1DeviceContext_Release(context);
    ID2D1BitmapRenderTarget_Release(bitmap_rt);

    /* DC target */
    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rt_desc.dpiX = 96.0f;
    rt_desc.dpiY = 96.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory1_CreateDCRenderTarget(ctx.factory1, &rt_desc, (ID2D1DCRenderTarget **)&rt);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID2D1DeviceContext_GetDevice(context, &device);
    hr = ID2D1Device_QueryInterface(device, &IID_ID2D1Device2, (void **)&device2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    dxgi_device = (IDXGIDevice *)0xdeadbeef;
    hr = ID2D1Device2_GetDxgiDevice(device2, &dxgi_device);
    ok(hr == D2DERR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ok(!dxgi_device, "Expected NULL DXGI device.\n");

    ID2D1Device2_Release(device2);
    ID2D1Device_Release(device);
    ID2D1DeviceContext_Release(context);
    ID2D1RenderTarget_Release(rt);

    release_test_context(&ctx);
}

START_TEST(d2d1)
{
    HMODULE d2d1_dll = GetModuleHandleA("d2d1.dll");
    unsigned int argc, i;
    char **argv;

    pD2D1CreateDevice = (void *)GetProcAddress(d2d1_dll, "D2D1CreateDevice");
    pD2D1SinCos = (void *)GetProcAddress(d2d1_dll, "D2D1SinCos");
    pD2D1Tan = (void *)GetProcAddress(d2d1_dll, "D2D1Tan");
    pD2D1Vec3Length = (void *)GetProcAddress(d2d1_dll, "D2D1Vec3Length");
    pD2D1ConvertColorSpace = (void *)GetProcAddress(d2d1_dll, "D2D1ConvertColorSpace");

    use_mt = !getenv("WINETEST_NO_MT_D3D");
    /* Some host drivers (MacOS, Mesa radeonsi) never unmap memory even when
     * requested. When using the chunk allocator, running the tests with more
     * than one thread can exceed the 32-bit virtual address space. */
    if (sizeof(void *) == 4 && !strcmp(winetest_platform, "wine"))
        use_mt = FALSE;

    argc = winetest_get_mainargs(&argv);
    for (i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--single"))
            use_mt = FALSE;
    }

    queue_test(test_clip);
    queue_test(test_state_block);
    queue_test(test_color_brush);
    queue_test(test_bitmap_brush);
    queue_test(test_image_brush);
    queue_test(test_linear_brush);
    queue_test(test_radial_brush);
    queue_test(test_path_geometry);
    queue_d3d10_test(test_rectangle_geometry);
    queue_d3d10_test(test_rounded_rectangle_geometry);
    queue_test(test_bitmap_formats);
    queue_test(test_alpha_mode);
    queue_test(test_shared_bitmap);
    queue_test(test_bitmap_updates);
    queue_test(test_opacity_brush);
    queue_test(test_create_target);
    queue_test(test_dxgi_surface_target_gdi_interop);
    queue_test(test_draw_text_layout);
    queue_test(test_dc_target);
    queue_test(test_dc_target_gdi_interop);
    queue_test(test_dc_target_is_supported);
    queue_test(test_hwnd_target);
    queue_test(test_hwnd_target_gdi_interop);
    queue_test(test_hwnd_target_is_supported);
    queue_test(test_bitmap_target);
    queue_d3d10_test(test_desktop_dpi);
    queue_d3d10_test(test_stroke_style);
    queue_test(test_gradient);
    queue_test(test_draw_geometry);
    queue_test(test_fill_geometry);
    queue_test(test_wic_gdi_interop);
    queue_test(test_layer);
    queue_test(test_bezier_intersect);
    queue_test(test_create_device);
    queue_test(test_bitmap_surface);
    queue_test(test_device_context);
    queue_d3d10_test(test_invert_matrix);
    queue_d3d10_test(test_skew_matrix);
    queue_test(test_command_list);
    queue_d3d10_test(test_max_bitmap_size);
    queue_test(test_dpi);
    queue_test(test_unit_mode);
    queue_test(test_wic_bitmap_format);
    queue_d3d10_test(test_math);
    queue_d3d10_test(test_colour_space);
    queue_test(test_geometry_group);
    queue_test(test_mt_factory);
    queue_test(test_effect_register);
    queue_test(test_effect_context);
    queue_test(test_effect_properties);
    queue_test(test_builtin_effect);
    queue_test(test_effect_2d_affine);
    queue_test(test_effect_crop);
    queue_test(test_effect_grayscale);
    queue_test(test_registered_effects);
    queue_test(test_transform_graph);
    queue_test(test_offset_transform);
    queue_test(test_blend_transform);
    queue_test(test_border_transform);
    queue_test(test_bounds_adjustment_transform);
    queue_d3d10_test(test_stroke_contains_point);
    queue_test(test_image_bounds);
    queue_test(test_bitmap_map);
    queue_test(test_bitmap_create);
    queue_test(test_effect_custom_pixel_shader);
    queue_test(test_get_effect_properties);
    queue_test(test_effect_vertex_buffer);
    queue_test(test_compute_geometry_area);
    queue_test(test_wic_target_format);
    queue_test(test_effect_blob_property);
    queue_test(test_get_dxgi_device);

    run_queued_tests();
}
