/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
 * Copyright 2009 Rico Schüller
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
#include "d3d10.h"
#include "d3dcompiler.h"
#include "wine/test.h"

#include <float.h>
#include <math.h>

#define D3DERR_INVALIDCALL 0x8876086c

static void set_vec4(float *v, float x, float y, float z, float w)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}

static void set_int4(int *v, int x, int y, int z, int w)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static inline HRESULT create_effect(DWORD *data, UINT flags, ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3D10Effect **effect)
{
    /*
     * Don't use sizeof(data), use data[6] as size,
     * because the DWORD data[] has only complete DWORDs and
     * so it could happen that there are padded bytes at the end.
     *
     * The fx size (data[6]) could be up to 3 BYTEs smaller
     * than the sizeof(data).
     */
    return D3D10CreateEffectFromMemory(data, data[6], flags, device, effect_pool, effect);
}

static HRESULT create_effect_pool(DWORD *data, ID3D10Device *device, ID3D10EffectPool **pool)
{
    return D3D10CreateEffectPoolFromMemory(data, data[6], 0, device, pool);
}

/*
 * test_effect_constant_buffer_type
 */
#if 0
cbuffer cb : register(b1)
{
    float   f1 : SV_POSITION;
    float   f2 : COLOR0;
};

cbuffer cb2 : register(b0)
{
    float   f3 : packoffset(c2);
};
#endif
static DWORD fx_test_ecbt[] =
{
    0x43425844, 0x7cfb8cde, 0x31ec2d95, 0x38500042, 0xa9330c67, 0x00000001, 0x00000145, 0x00000001,
    0x00000024, 0x30315846, 0x00000119, 0xfeff1001, 0x00000002, 0x00000003, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000049, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x66006263,
    0x74616f6c, 0x00000700, 0x00000100, 0x00000000, 0x00000400, 0x00001000, 0x00000400, 0x00090900,
    0x00316600, 0x505f5653, 0x5449534f, 0x004e4f49, 0x43003266, 0x524f4c4f, 0x62630030, 0x33660032,
    0x00000400, 0x00001000, 0x00000000, 0x00000200, 0x00000100, 0x00000000, 0x00002900, 0x00000d00,
    0x00002c00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00003800, 0x00000d00, 0x00003b00,
    0x00000400, 0x00000000, 0x00000000, 0x00000000, 0x00004200, 0x00003000, 0x00000000, 0x00000100,
    0x00000000, 0x00000000, 0x00004600, 0x00000d00, 0x00000000, 0x00002000, 0x00000000, 0x00000400,
    0x00000000, 0x00000000,
};

static void test_effect_constant_buffer_type(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectType *type, *type2, *null_type;
    D3D10_EFFECT_VARIABLE_DESC var_desc;
    D3D10_EFFECT_TYPE_DESC type_desc;
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10ShaderResourceView *srv;
    ID3D10EffectVariable *v;
    D3D10_EFFECT_DESC desc;
    ID3D10Device *device;
    ID3D10Buffer *buffer;
    ULONG refcount;
    HRESULT hr;
    LPCSTR string;
    unsigned int i;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_ecbt, 0, NULL, NULL, &effect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = create_effect(fx_test_ecbt, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(desc.ConstantBuffers == 2, "Unexpected constant buffers count %u.\n", desc.ConstantBuffers);
    ok(desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            desc.SharedConstantBuffers);
    ok(desc.GlobalVariables == 3, "Unexpected global variables count %u.\n", desc.GlobalVariables);
    ok(desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            desc.SharedGlobalVariables);
    ok(desc.Techniques == 0, "Unexpected techniques count %u.\n", desc.Techniques);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);

    hr = constantbuffer->lpVtbl->GetDesc(constantbuffer, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_EXPLICIT_BIND_POINT, "Unexpected variable flags %#x.\n", var_desc.Flags);
    ok(var_desc.ExplicitBindPoint == 1, "Unexpected bind point %#x.\n", var_desc.ExplicitBindPoint);

    type = constantbuffer->lpVtbl->GetType(constantbuffer);

    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_OBJECT);
    ok(type_desc.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_CBUFFER);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 2, "Members is %u, expected 2\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x8, "PackedSize is %#x, expected 0x8\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x10, "UnpackedSize is %#x, expected 0x10\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    hr = constantbuffer->lpVtbl->GetConstantBuffer(constantbuffer, &buffer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Buffer_GetDesc(buffer, &buffer_desc);
    ok(buffer_desc.ByteWidth == type_desc.UnpackedSize, "Unexpected buffer size %u.\n", buffer_desc.ByteWidth);
    ok(!buffer_desc.Usage, "Unexpected buffer usage %u.\n", buffer_desc.Usage);
    ok(buffer_desc.BindFlags == D3D10_BIND_CONSTANT_BUFFER, "Unexpected bind flags %#x.\n",
            buffer_desc.BindFlags);
    ok(!buffer_desc.CPUAccessFlags, "Unexpected CPU access flags %#x.\n", buffer_desc.CPUAccessFlags);
    ok(!buffer_desc.MiscFlags, "Unexpected misc flags %#x.\n", buffer_desc.MiscFlags);
    ID3D10Buffer_Release(buffer);

    hr = constantbuffer->lpVtbl->GetTextureBuffer(constantbuffer, &srv);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    string = type->lpVtbl->GetMemberName(type, 0);
    ok(strcmp(string, "f1") == 0, "GetMemberName is \"%s\", expected \"f1\"\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 0);
    ok(strcmp(string, "SV_POSITION") == 0, "GetMemberSemantic is \"%s\", expected \"SV_POSITION\"\n", string);

    string = type->lpVtbl->GetMemberName(type, 1);
    ok(strcmp(string, "f2") == 0, "GetMemberName is \"%s\", expected \"f2\"\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 1);
    ok(strcmp(string, "COLOR0") == 0, "GetMemberSemantic is \"%s\", expected \"COLOR0\"\n", string);

    for (i = 0; i < 3; ++i)
    {
        if (i == 0) type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
        else if (i == 1) type2 = type->lpVtbl->GetMemberTypeByName(type, "f1");
        else type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "SV_POSITION");

        hr = type2->lpVtbl->GetDesc(type2, &type_desc);
        ok(hr == S_OK, "%u: Got unexpected hr %#lx.\n", i, hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

        if (i == 0) type2 = type->lpVtbl->GetMemberTypeByIndex(type, 1);
        else if (i == 1) type2 = type->lpVtbl->GetMemberTypeByName(type, "f2");
        else type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "COLOR0");

        hr = type2->lpVtbl->GetDesc(type2, &type_desc);
        ok(hr == S_OK, "%u: Got unexpected hr %#lx.\n", i, hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);
    }

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
    hr = type2->lpVtbl->GetDesc(type2, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    null_type = type->lpVtbl->GetMemberTypeByIndex(type, 3);
    hr = null_type->lpVtbl->GetDesc(null_type, &type_desc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = null_type->lpVtbl->GetDesc(null_type, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    type2 = type->lpVtbl->GetMemberTypeByName(type, "invalid");
    ok(type2 == null_type, "GetMemberTypeByName got wrong type %p, expected %p\n", type2, null_type);

    type2 = type->lpVtbl->GetMemberTypeByName(type, NULL);
    ok(type2 == null_type, "GetMemberTypeByName got wrong type %p, expected %p\n", type2, null_type);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "invalid");
    ok(type2 == null_type, "GetMemberTypeBySemantic got wrong type %p, expected %p\n", type2, null_type);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, NULL);
    ok(type2 == null_type, "GetMemberTypeBySemantic got wrong type %p, expected %p\n", type2, null_type);

    string = type->lpVtbl->GetMemberName(type, 3);
    ok(string == NULL, "GetMemberName is \"%s\", expected \"NULL\"\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 3);
    ok(string == NULL, "GetMemberSemantic is \"%s\", expected \"NULL\"\n", string);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 1);
    v = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 0);
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "f3"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_EXPLICIT_BIND_POINT, "Unexpected variable flags %#x.\n", var_desc.Flags);
    ok(var_desc.BufferOffset == 0x20, "Unexpected buffer offset %#x.\n", var_desc.BufferOffset);
    ok(var_desc.ExplicitBindPoint == 0x20, "Unexpected bind point %#x.\n", var_desc.ExplicitBindPoint);

    /* Invalid buffer variable */
    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 100);
    hr = constantbuffer->lpVtbl->GetConstantBuffer(constantbuffer, &buffer);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = constantbuffer->lpVtbl->GetTextureBuffer(constantbuffer, &srv);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_variable_type
 */
#if 0
struct test
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR0;
};
struct test1
{
    float   f1;
    float   f2;
    test    t;
};
cbuffer cb
{
    test1 t1;
};
#endif
static DWORD fx_test_evt[] = {
0x43425844, 0xe079efed, 0x90bda0f2, 0xa6e2d0b4,
0xd2d6c200, 0x00000001, 0x0000018c, 0x00000001,
0x00000024, 0x30315846, 0x00000160, 0xfeff1001,
0x00000001, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x000000e0,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x74006263,
0x31747365, 0x00316600, 0x616f6c66, 0x00100074,
0x00010000, 0x00000000, 0x00040000, 0x00100000,
0x00040000, 0x09090000, 0x32660000, 0x74007400,
0x00747365, 0x53003366, 0x4f505f56, 0x49544953,
0x66004e4f, 0x4f430034, 0x30524f4c, 0x00003700,
0x00000300, 0x00000000, 0x00000800, 0x00001000,
0x00000800, 0x00000200, 0x00003c00, 0x00003f00,
0x00000000, 0x00001600, 0x00004b00, 0x00004e00,
0x00000400, 0x00001600, 0x00000700, 0x00000300,
0x00000000, 0x00001800, 0x00002000, 0x00001000,
0x00000300, 0x00000d00, 0x00000000, 0x00000000,
0x00001600, 0x00003200, 0x00000000, 0x00000400,
0x00001600, 0x00003500, 0x00000000, 0x00001000,
0x00005500, 0x00317400, 0x00000004, 0x00000020,
0x00000000, 0x00000001, 0xffffffff, 0x00000000,
0x000000dd, 0x00000091, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000,
};

static void test_effect_variable_type(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectVariable *variable;
    ID3D10EffectType *type, *type2, *type3;
    D3D10_EFFECT_TYPE_DESC type_desc;
    D3D10_EFFECT_DESC desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    LPCSTR string;
    unsigned int i;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_evt, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(desc.ConstantBuffers == 1, "Unexpected constant buffers count %u.\n", desc.ConstantBuffers);
    ok(desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            desc.SharedConstantBuffers);
    ok(desc.GlobalVariables == 1, "Unexpected global variables count %u.\n", desc.GlobalVariables);
    ok(desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            desc.SharedGlobalVariables);
    ok(desc.Techniques == 0, "Unexpected techniques count %u.\n", desc.Techniques);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    type = constantbuffer->lpVtbl->GetType(constantbuffer);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_OBJECT);
    ok(type_desc.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_CBUFFER);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 1, "Members is %u, expected 1\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x10, "PackedSize is %#x, expected 0x10\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x20, "UnpackedSize is %#x, expected 0x20\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x20, "Stride is %#x, expected 0x20\n", type_desc.Stride);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 0);
    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "test1") == 0, "TypeName is \"%s\", expected \"test1\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x10, "PackedSize is %#x, expected 0x10\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x18, "UnpackedSize is %#x, expected 0x18\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x20, "Stride is %#x, expected 0x20\n", type_desc.Stride);

    string = type->lpVtbl->GetMemberName(type, 0);
    ok(strcmp(string, "f1") == 0, "GetMemberName is \"%s\", expected \"f1\"\n", string);

    string = type->lpVtbl->GetMemberName(type, 1);
    ok(strcmp(string, "f2") == 0, "GetMemberName is \"%s\", expected \"f2\"\n", string);

    string = type->lpVtbl->GetMemberName(type, 2);
    ok(strcmp(string, "t") == 0, "GetMemberName is \"%s\", expected \"t\"\n", string);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 1);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 2);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 2, "Members is %u, expected 2\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x8, "PackedSize is %#x, expected 0x8\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x8, "UnpackedSize is %#x, expected 0x8\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %x, expected 0x10\n", type_desc.Stride);

    for (i = 0; i < 4; ++i)
    {
        if (i == 0) type3 = type2->lpVtbl->GetMemberTypeByIndex(type2, 0);
        else if (i == 1) type3 = type2->lpVtbl->GetMemberTypeByName(type2, "f3");
        else if (i == 2) type3 = type2->lpVtbl->GetMemberTypeBySemantic(type2, "SV_POSITION");
        else type3 = type2->lpVtbl->GetMemberTypeBySemantic(type2, "sv_POSITION");

        hr = type3->lpVtbl->GetDesc(type3, &type_desc);
        ok(hr == S_OK, "%u: Got unexpected hr %#lx.\n", i, hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n",
            type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

        if (i == 0) type3 = type2->lpVtbl->GetMemberTypeByIndex(type2, 1);
        else if (i == 1) type3 = type2->lpVtbl->GetMemberTypeByName(type2, "f4");
        else if (i == 2) type3 = type2->lpVtbl->GetMemberTypeBySemantic(type2, "COLOR0");
        else type3 = type2->lpVtbl->GetMemberTypeBySemantic(type2, "color0");

        hr = type3->lpVtbl->GetDesc(type3, &type_desc);
        ok(hr == S_OK, "%u: Got unexpected hr %#lx.\n", i, hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n",
            type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);
    }

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
    hr = type2->lpVtbl->GetDesc(type2, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 4);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    type2 = type->lpVtbl->GetMemberTypeByName(type, "invalid");
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    type2 = type->lpVtbl->GetMemberTypeByName(type, NULL);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "invalid");
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, NULL);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    string = type->lpVtbl->GetMemberName(type, 4);
    ok(string == NULL, "GetMemberName is \"%s\", expected NULL\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 4);
    ok(string == NULL, "GetMemberSemantic is \"%s\", expected NULL\n", string);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_variable_member
 */
#if 0
struct test
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR0;
};
struct test1
{
    float   f1;
    float   f2;
    test    t;
};
cbuffer cb
{
    test1 t1;
};
#endif
static DWORD fx_test_evm[] = {
0x43425844, 0xe079efed, 0x90bda0f2, 0xa6e2d0b4,
0xd2d6c200, 0x00000001, 0x0000018c, 0x00000001,
0x00000024, 0x30315846, 0x00000160, 0xfeff1001,
0x00000001, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x000000e0,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x74006263,
0x31747365, 0x00316600, 0x616f6c66, 0x00100074,
0x00010000, 0x00000000, 0x00040000, 0x00100000,
0x00040000, 0x09090000, 0x32660000, 0x74007400,
0x00747365, 0x53003366, 0x4f505f56, 0x49544953,
0x66004e4f, 0x4f430034, 0x30524f4c, 0x00003700,
0x00000300, 0x00000000, 0x00000800, 0x00001000,
0x00000800, 0x00000200, 0x00003c00, 0x00003f00,
0x00000000, 0x00001600, 0x00004b00, 0x00004e00,
0x00000400, 0x00001600, 0x00000700, 0x00000300,
0x00000000, 0x00001800, 0x00002000, 0x00001000,
0x00000300, 0x00000d00, 0x00000000, 0x00000000,
0x00001600, 0x00003200, 0x00000000, 0x00000400,
0x00001600, 0x00003500, 0x00000000, 0x00001000,
0x00005500, 0x00317400, 0x00000004, 0x00000020,
0x00000000, 0x00000001, 0xffffffff, 0x00000000,
0x000000dd, 0x00000091, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000,
};

static void test_effect_variable_member(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectVariable *variable, *variable2, *variable3, *null_variable;
    D3D10_EFFECT_VARIABLE_DESC desc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_evm, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 1, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 1, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = constantbuffer->lpVtbl->GetDesc(constantbuffer, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "cb") == 0, "Name is \"%s\", expected \"cb\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Type is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Elements is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "Members is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    null_variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 1);
    hr = null_variable->lpVtbl->GetDesc(null_variable, &desc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 0);
    hr = variable->lpVtbl->GetDesc(variable, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, "t1");
    ok(variable == variable2, "GetMemberByName got %p, expected %p\n", variable2, variable);
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "t1") == 0, "Name is \"%s\", expected \"t1\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, "invalid");
    ok(null_variable == variable2, "GetMemberByName got %p, expected %p\n", variable2, null_variable);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, NULL);
    ok(null_variable == variable2, "GetMemberByName got %p, expected %p\n", variable2, null_variable);

    variable2 = constantbuffer->lpVtbl->GetMemberBySemantic(constantbuffer, "invalid");
    ok(null_variable == variable2, "GetMemberBySemantic got %p, expected %p\n", variable2, null_variable);

    variable2 = constantbuffer->lpVtbl->GetMemberBySemantic(constantbuffer, NULL);
    ok(null_variable == variable2, "GetMemberBySemantic got %p, expected %p\n", variable2, null_variable);

    /* check members of "t1" */
    variable2 = variable->lpVtbl->GetMemberByName(variable, "f1");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f1") == 0, "Name is \"%s\", expected \"f1\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable3 = variable->lpVtbl->GetMemberByIndex(variable, 0);
    ok(variable2 == variable3, "GetMemberByIndex got %p, expected %p\n", variable3, variable2);

    variable2 = variable->lpVtbl->GetMemberByName(variable, "f2");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 4, "BufferOffset is %u, expected 4\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable3 = variable->lpVtbl->GetMemberByIndex(variable, 1);
    ok(variable2 == variable3, "GetMemberByIndex got %p, expected %p\n", variable3, variable2);

    variable2 = variable->lpVtbl->GetMemberByName(variable, "t");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable3 = variable->lpVtbl->GetMemberByIndex(variable, 2);
    ok(variable2 == variable3, "GetMemberByIndex got %p, expected %p\n", variable3, variable2);

    /* check members of "t" */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f3");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f3") == 0, "Name is \"%s\", expected \"f3\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "SV_POSITION") == 0, "Semantic is \"%s\", expected \"SV_POSITION\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable = variable2->lpVtbl->GetMemberBySemantic(variable2, "SV_POSITION");
    ok(variable == variable3, "GetMemberBySemantic got %p, expected %p\n", variable, variable3);

    variable = variable2->lpVtbl->GetMemberBySemantic(variable2, "sv_POSITION");
    ok(variable == variable3, "GetMemberBySemantic got %p, expected %p\n", variable, variable3);

    variable = variable2->lpVtbl->GetMemberByIndex(variable2, 0);
    ok(variable == variable3, "GetMemberByIndex got %p, expected %p\n", variable, variable3);

    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f4");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f4") == 0, "Name is \"%s\", expected \"f4\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR0") == 0, "Semantic is \"%s\", expected \"COLOR0\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 20, "BufferOffset is %u, expected 20\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable = variable2->lpVtbl->GetMemberBySemantic(variable2, "COLOR0");
    ok(variable == variable3, "GetMemberBySemantic got %p, expected %p\n", variable, variable3);

    variable = variable2->lpVtbl->GetMemberBySemantic(variable2, "color0");
    ok(variable == variable3, "GetMemberBySemantic got %p, expected %p\n", variable, variable3);

    variable = variable2->lpVtbl->GetMemberByIndex(variable2, 1);
    ok(variable == variable3, "GetMemberByIndex got %p, expected %p\n", variable, variable3);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_variable_element
 */
#if 0
struct test
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR0;
    float   f5 : COLOR1;
};
struct test1
{
    float   f1;
    float   f2[3];
    test    t[2];
};
cbuffer cb
{
    test1 t1;
};
#endif
static DWORD fx_test_eve[] = {
0x43425844, 0x6ea69fd9, 0x9b4e6390, 0x006f9f71,
0x57ad58f4, 0x00000001, 0x000001c2, 0x00000001,
0x00000024, 0x30315846, 0x00000196, 0xfeff1001,
0x00000001, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000116,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x74006263,
0x31747365, 0x00316600, 0x616f6c66, 0x00100074,
0x00010000, 0x00000000, 0x00040000, 0x00100000,
0x00040000, 0x09090000, 0x32660000, 0x00001000,
0x00000100, 0x00000300, 0x00002400, 0x00001000,
0x00000c00, 0x00090900, 0x74007400, 0x00747365,
0x53003366, 0x4f505f56, 0x49544953, 0x66004e4f,
0x4f430034, 0x30524f4c, 0x00356600, 0x4f4c4f43,
0x53003152, 0x03000000, 0x02000000, 0x1c000000,
0x10000000, 0x18000000, 0x03000000, 0x58000000,
0x5b000000, 0x00000000, 0x16000000, 0x67000000,
0x6a000000, 0x04000000, 0x16000000, 0x71000000,
0x74000000, 0x08000000, 0x16000000, 0x07000000,
0x03000000, 0x00000000, 0x5c000000, 0x60000000,
0x28000000, 0x03000000, 0x0d000000, 0x00000000,
0x00000000, 0x16000000, 0x32000000, 0x00000000,
0x10000000, 0x35000000, 0x51000000, 0x00000000,
0x40000000, 0x7b000000, 0x74000000, 0x00040031,
0x00600000, 0x00000000, 0x00010000, 0xffff0000,
0x0000ffff, 0x01130000, 0x00c70000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000,
};

static void test_effect_variable_element(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer, *parent;
    ID3D10EffectVariable *variable, *variable2, *variable3, *variable4, *variable5;
    ID3D10EffectType *type, *type2;
    D3D10_EFFECT_VARIABLE_DESC desc;
    D3D10_EFFECT_TYPE_DESC type_desc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_eve, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 1, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 1, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = constantbuffer->lpVtbl->GetDesc(constantbuffer, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "cb") == 0, "Name is \"%s\", expected \"cb\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Type is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Elements is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "Members is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 0);
    hr = variable->lpVtbl->GetDesc(variable, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, "t1");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    parent = variable2->lpVtbl->GetParentConstantBuffer(variable2);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    /* check variable f1 */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f1");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f1") == 0, "Name is \"%s\", expected \"f1\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable3->lpVtbl->GetParentConstantBuffer(variable3);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable3->lpVtbl->GetType(variable3);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    /* check variable f2 */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f2");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable3->lpVtbl->GetParentConstantBuffer(variable3);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable3->lpVtbl->GetType(variable3);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 3, "Elements is %u, expected 3\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0xc, "PackedSize is %#x, expected 0xc\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x24, "UnpackedSize is %#x, expected 0x24\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 0);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable4->lpVtbl->GetType(variable4);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 1);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 32, "BufferOffset is %u, expected 32\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type2 = variable4->lpVtbl->GetType(variable4);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(type == type2, "type(%p) != type2(%p)\n", type, type2);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 2);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 48, "BufferOffset is %u, expected 48\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type2 = variable4->lpVtbl->GetType(variable4);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(type == type2, "type(%p) != type2(%p)\n", type, type2);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    /* check variable t */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "t");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 64, "BufferOffset is %u, expected 64\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable3->lpVtbl->GetParentConstantBuffer(variable3);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable3->lpVtbl->GetType(variable3);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 2, "Elements is %u, expected 2\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x18, "PackedSize is %#x, expected 0x18\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x1c, "UnpackedSize is %#x, expected 0x1c\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 0);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 64, "BufferOffset is %u, expected 64\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable4->lpVtbl->GetType(variable4);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0xc, "PackedSize is %#x, expected 0xc\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0xc, "UnpackedSize is %#x, expected 0xc\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 0);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f3") == 0, "Name is \"%s\", expected \"f3\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "SV_POSITION") == 0, "Semantic is \"%s\", expected \"SV_POSITION\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 64, "BufferOffset is %u, expected 64\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 1);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f4") == 0, "Name is \"%s\", expected \"f4\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR0") == 0, "Semantic is \"%s\", expected \"COLOR0\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 68, "BufferOffset is %u, expected 68\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 2);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f5") == 0, "Name is \"%s\", expected \"f5\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR1") == 0, "Semantic is \"%s\", expected \"COLOR1\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 72, "BufferOffset is %u, expected 72\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 1);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 80, "BufferOffset is %u, expected 80\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable4->lpVtbl->GetType(variable4);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0xc, "PackedSize is %#x, expected 0xc\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0xc, "UnpackedSize is %#x, expected 0xc\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 0);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f3") == 0, "Name is \"%s\", expected \"f3\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "SV_POSITION") == 0, "Semantic is \"%s\", expected \"SV_POSITION\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 80, "BufferOffset is %u, expected 80\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 1);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f4") == 0, "Name is \"%s\", expected \"f4\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR0") == 0, "Semantic is \"%s\", expected \"COLOR0\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 84, "BufferOffset is %u, expected 84\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 2);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(desc.Name, "f5") == 0, "Name is \"%s\", expected \"f5\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR1") == 0, "Semantic is \"%s\", expected \"COLOR1\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 88, "BufferOffset is %u, expected 88\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_variable_type_class
 */
#if 0
cbuffer cb <String s = "STRING"; String s2 = "STRING"; >
{
    float f;
    vector <int, 2> i;
    matrix <uint, 2, 3> u;
    row_major matrix <bool, 2, 3> b;
};
BlendState blend;
DepthStencilState depthstencil;
RasterizerState rast;
SamplerState sam;
RenderTargetView rtv;
DepthStencilView dsv;
Texture t0;
Texture1D t1;
Texture1DArray t1a;
Texture2D t2;
Texture2DMS <float4, 4> t2dms;
Texture2DArray t2a;
Texture2DMSArray <float4, 4> t2dmsa;
Texture3D t3;
TextureCube tq;
GeometryShader gs[2];
PixelShader ps;
VertexShader vs[1];
BlendState blend2[2];
DepthStencilState depthstencil2[2];
RasterizerState rast2[2];
SamplerState sam2[2];
#endif
static DWORD fx_test_evtc[] =
{
    0x43425844, 0xbcafa6f1, 0x8baf9e54, 0x8e1fd2ef, 0x115ba370, 0x00000001, 0x000007a3, 0x00000001,
    0x00000024, 0x30315846, 0x00000777, 0xfeff1001, 0x00000001, 0x00000004, 0x00000016, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000493, 0x00000000, 0x00000009, 0x00000003, 0x00000003,
    0x00000003, 0x00000003, 0x00000001, 0x00000001, 0x00000004, 0x00000000, 0x00000000, 0x53006263,
    0x6e697274, 0x00070067, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00010000,
    0x00730000, 0x49525453, 0x7300474e, 0x6c660032, 0x0074616f, 0x00000036, 0x00000001, 0x00000000,
    0x00000004, 0x00000010, 0x00000004, 0x00000909, 0x6e690066, 0x5a003274, 0x01000000, 0x00000000,
    0x08000000, 0x10000000, 0x08000000, 0x12000000, 0x69000011, 0x6e697500, 0x33783274, 0x00007d00,
    0x00000100, 0x00000000, 0x00002800, 0x00003000, 0x00001800, 0x005a1b00, 0x62007500, 0x326c6f6f,
    0xa3003378, 0x01000000, 0x00000000, 0x1c000000, 0x20000000, 0x18000000, 0x23000000, 0x6200001a,
    0x656c4200, 0x7453646e, 0x00657461, 0x000000c9, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000002, 0x6e656c62, 0x65440064, 0x53687470, 0x636e6574, 0x74536c69, 0x00657461,
    0x000000f6, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x74706564,
    0x65747368, 0x6c69636e, 0x73615200, 0x69726574, 0x5372657a, 0x65746174, 0x00013100, 0x00000200,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000400, 0x73617200, 0x61530074, 0x656c706d,
    0x61745372, 0x62006574, 0x02000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x15000000,
    0x73000000, 0x52006d61, 0x65646e65, 0x72615472, 0x56746567, 0x00776569, 0x0000018f, 0x00000002,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000013, 0x00767472, 0x74706544, 0x65745368,
    0x6c69636e, 0x77656956, 0x0001c000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00001400, 0x76736400, 0x78657400, 0x65727574, 0x0001f100, 0x00000200, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000900, 0x00307400, 0x74786554, 0x31657275, 0x02180044, 0x00020000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000a0000, 0x31740000, 0x78655400, 0x65727574,
    0x72414431, 0x00796172, 0x00000241, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0000000b, 0x00613174, 0x74786554, 0x32657275, 0x02700044, 0x00020000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x000c0000, 0x32740000, 0x78655400, 0x65727574, 0x534d4432, 0x00029900,
    0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000e00, 0x64327400, 0x5400736d,
    0x75747865, 0x44326572, 0x61727241, 0x02c70079, 0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x000d0000, 0x32740000, 0x65540061, 0x72757478, 0x4d443265, 0x72724153, 0xf6007961,
    0x02000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0f000000, 0x74000000, 0x736d6432,
    0x65540061, 0x72757478, 0x00443365, 0x0000032a, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000010, 0x54003374, 0x75747865, 0x75436572, 0x53006562, 0x02000003, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x11000000, 0x74000000, 0x65470071, 0x74656d6f, 0x68537972,
    0x72656461, 0x00037e00, 0x00000200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000700,
    0x00736700, 0x65786950, 0x6168536c, 0x00726564, 0x000003ac, 0x00000002, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000005, 0x56007370, 0x65747265, 0x61685378, 0x00726564, 0x000003d7,
    0x00000002, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0xc9007376, 0x02000000,
    0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x02000000, 0x62000000, 0x646e656c, 0x00f60032,
    0x00020000, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00030000, 0x65640000, 0x73687470,
    0x636e6574, 0x00326c69, 0x00000131, 0x00000002, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x00000004, 0x74736172, 0x01620032, 0x00020000, 0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x00150000, 0x61730000, 0x0400326d, 0x60000000, 0x00000000, 0x04000000, 0xff000000, 0x02ffffff,
    0x2a000000, 0x0e000000, 0x2c000000, 0x33000000, 0x0e000000, 0x2c000000, 0x58000000, 0x3c000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7b000000, 0x5f000000, 0x00000000,
    0x04000000, 0x00000000, 0x00000000, 0x00000000, 0xa1000000, 0x85000000, 0x00000000, 0x10000000,
    0x00000000, 0x00000000, 0x00000000, 0xc7000000, 0xab000000, 0x00000000, 0x40000000, 0x00000000,
    0x00000000, 0x00000000, 0xf0000000, 0xd4000000, 0x00000000, 0xff000000, 0x00ffffff, 0x00000000,
    0x24000000, 0x08000001, 0x00000001, 0xff000000, 0x00ffffff, 0x00000000, 0x5d000000, 0x41000001,
    0x00000001, 0xff000000, 0x00ffffff, 0x00000000, 0x8b000000, 0x6f000001, 0x00000001, 0xff000000,
    0x00ffffff, 0x00000000, 0xbc000000, 0xa0000001, 0x00000001, 0xff000000, 0x00ffffff, 0xed000000,
    0xd1000001, 0x00000001, 0xff000000, 0x00ffffff, 0x15000000, 0xf9000002, 0x00000001, 0xff000000,
    0x00ffffff, 0x3e000000, 0x22000002, 0x00000002, 0xff000000, 0x00ffffff, 0x6c000000, 0x50000002,
    0x00000002, 0xff000000, 0x00ffffff, 0x96000000, 0x7a000002, 0x00000002, 0xff000000, 0x00ffffff,
    0xc1000000, 0xa5000002, 0x00000002, 0xff000000, 0x00ffffff, 0xf2000000, 0xd6000002, 0x00000002,
    0xff000000, 0x00ffffff, 0x23000000, 0x07000003, 0x00000003, 0xff000000, 0x00ffffff, 0x50000000,
    0x34000003, 0x00000003, 0xff000000, 0x00ffffff, 0x7b000000, 0x5f000003, 0x00000003, 0xff000000,
    0x00ffffff, 0xa9000000, 0x8d000003, 0x00000003, 0xff000000, 0x00ffffff, 0x00000000, 0x00000000,
    0xd4000000, 0xb8000003, 0x00000003, 0xff000000, 0x00ffffff, 0x00000000, 0x00000000, 0xe4000004,
    0x00000003, 0xff000000, 0x00ffffff, 0x00000000, 0x1f000000, 0x03000004, 0x00000004, 0xff000000,
    0x00ffffff, 0x00000000, 0x00000000, 0x42000000, 0x26000004, 0x00000004, 0xff000000, 0x00ffffff,
    0x00000000, 0x00000000, 0x6c000000, 0x50000004, 0x00000004, 0xff000000, 0x00ffffff, 0x00000000,
    0x00000000, 0x8e000000, 0x72000004, 0x00000004, 0xff000000, 0x00ffffff, 0x00000000, 0x00000000,
    0x00000000,
};

static BOOL is_valid_check(BOOL a, BOOL b)
{
    return (a && b) || (!a && !b);
}

static void check_as(ID3D10EffectVariable *variable)
{
    ID3D10EffectVariable *variable2;
    ID3D10EffectType *type;
    D3D10_EFFECT_TYPE_DESC td;
    BOOL ret, is_valid;
    HRESULT hr;

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsConstantBuffer(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_CBUFFER);
    ok(ret, "AsConstantBuffer valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsString(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_STRING);
    ok(ret, "AsString valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsScalar(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Class == D3D10_SVC_SCALAR);
    ok(ret, "AsScalar valid check failed (Class is %x)\n", td.Class);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsVector(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Class == D3D10_SVC_VECTOR);
    ok(ret, "AsVector valid check failed (Class is %x)\n", td.Class);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsMatrix(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Class == D3D10_SVC_MATRIX_ROWS
        || td.Class == D3D10_SVC_MATRIX_COLUMNS);
    ok(ret, "AsMatrix valid check failed (Class is %x)\n", td.Class);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsBlend(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_BLEND);
    ok(ret, "AsBlend valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsDepthStencil(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_DEPTHSTENCIL);
    ok(ret, "AsDepthStencil valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsRasterizer(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_RASTERIZER);
    ok(ret, "AsRasterizer valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsSampler(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_SAMPLER);
    ok(ret, "AsSampler valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsDepthStencilView(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_DEPTHSTENCILVIEW);
    ok(ret, "AsDepthStencilView valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsRenderTargetView(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_RENDERTARGETVIEW);
    ok(ret, "AsRenderTargetView valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsShaderResource(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_TEXTURE || td.Type == D3D10_SVT_TEXTURE1D
        || td.Type == D3D10_SVT_TEXTURE1DARRAY || td.Type == D3D10_SVT_TEXTURE2D
        || td.Type == D3D10_SVT_TEXTURE2DMS || td.Type == D3D10_SVT_TEXTURE2DARRAY
        || td.Type == D3D10_SVT_TEXTURE2DMSARRAY || td.Type == D3D10_SVT_TEXTURE3D
        || td.Type == D3D10_SVT_TEXTURECUBE);
    ok(ret, "AsShaderResource valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsShader(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_GEOMETRYSHADER
        || td.Type == D3D10_SVT_PIXELSHADER || td.Type == D3D10_SVT_VERTEXSHADER);
    ok(ret, "AsShader valid check failed (Type is %x)\n", td.Type);
}

static void test_effect_variable_type_class(void)
{
    ID3D10EffectStringVariable *string_var;
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer, *null_buffer, *parent;
    ID3D10EffectVariable *variable;
    ID3D10EffectType *type;
    D3D10_EFFECT_VARIABLE_DESC vd;
    D3D10_EFFECT_TYPE_DESC td;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    unsigned int variable_nr = 0;
    const char *str1, *str2;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_evtc, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 1, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 26, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    /* get the null_constantbuffer, so that we can compare it to variables->GetParentConstantBuffer */
    null_buffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 1);

    /* check constantbuffer cb */
    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = constantbuffer->lpVtbl->GetDesc(constantbuffer, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "cb") == 0, "Name is \"%s\", expected \"cb\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Type is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 2, "Unexpected Annotations %u.\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "Members is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as((ID3D10EffectVariable *)constantbuffer);

    parent = constantbuffer->lpVtbl->GetParentConstantBuffer(constantbuffer);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = constantbuffer->lpVtbl->GetType(constantbuffer);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_CBUFFER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 4, "Members is %u, expected 4\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x3c, "PackedSize is %#x, expected 0x3c\n", td.PackedSize);
    ok(td.UnpackedSize == 0x60, "UnpackedSize is %#x, expected 0x60\n", td.UnpackedSize);
    ok(td.Stride == 0x60, "Stride is %#x, expected 0x60\n", td.Stride);

    variable = constantbuffer->lpVtbl->GetAnnotationByIndex(constantbuffer, 0);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "s") == 0, "Name is \"%s\", expected \"s\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == D3D10_EFFECT_VARIABLE_ANNOTATION, "Unexpected flags %#x.\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "String") == 0, "TypeName is \"%s\", expected \"String\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_STRING, "Type is %x, expected %x\n", td.Type, D3D10_SVT_STRING);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    string_var = variable->lpVtbl->AsString(variable);
    hr = string_var->lpVtbl->GetString(string_var, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = string_var->lpVtbl->GetString(string_var, &str1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(str1, "STRING"), "Unexpected value %s.\n", str1);

    variable = constantbuffer->lpVtbl->GetAnnotationByIndex(constantbuffer, 1);
    string_var = variable->lpVtbl->AsString(variable);
    hr = string_var->lpVtbl->GetString(string_var, &str2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(str2 != str1, "Unexpected string pointer.\n");
    ok(!strcmp(str2, "STRING"), "Unexpected value %s.\n", str1);

    /* Only two annotations */
    variable = constantbuffer->lpVtbl->GetAnnotationByIndex(constantbuffer, 2);
    string_var = variable->lpVtbl->AsString(variable);
    hr = string_var->lpVtbl->GetString(string_var, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    str1 = (void *)0xdeadbeef;
    hr = string_var->lpVtbl->GetString(string_var, &str1);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(str1 == (void *)0xdeadbeef, "Unexpected pointer.\n");

    /* check float f */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "f") == 0, "Name is \"%s\", expected \"f\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", td.Class, D3D10_SVC_SCALAR);
    ok(td.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", td.Type, D3D10_SVT_FLOAT);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 1, "Rows is %u, expected 1\n", td.Rows);
    ok(td.Columns == 1, "Columns is %u, expected 1\n", td.Columns);
    ok(td.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", td.PackedSize);
    ok(td.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", td.UnpackedSize);
    ok(td.Stride == 0x10, "Stride is %#x, expected 0x10\n", td.Stride);

    /* check int2 i */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "i") == 0, "Name is \"%s\", expected \"i\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 4, "BufferOffset is %u, expected 4\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "int2") == 0, "TypeName is \"%s\", expected \"int2\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_VECTOR, "Class is %x, expected %x\n", td.Class, D3D10_SVC_VECTOR);
    ok(td.Type == D3D10_SVT_INT, "Type is %x, expected %x\n", td.Type, D3D10_SVT_INT);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 1, "Rows is %u, expected 1\n", td.Rows);
    ok(td.Columns == 2, "Columns is %u, expected 2\n", td.Columns);
    ok(td.PackedSize == 0x8, "PackedSize is %#x, expected 0x8\n", td.PackedSize);
    ok(td.UnpackedSize == 0x8, "UnpackedSize is %#x, expected 0x8\n", td.UnpackedSize);
    ok(td.Stride == 0x10, "Stride is %#x, expected 0x10\n", td.Stride);

    /* check uint2x3 u */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "u") == 0, "Name is \"%s\", expected \"u\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 16, "BufferOffset is %u, expected 16\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "uint2x3") == 0, "TypeName is \"%s\", expected \"uint2x3\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_MATRIX_COLUMNS, "Class is %x, expected %x\n", td.Class, D3D10_SVC_MATRIX_COLUMNS);
    ok(td.Type == D3D10_SVT_UINT, "Type is %x, expected %x\n", td.Type, D3D10_SVT_UINT);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 2, "Rows is %u, expected 2\n", td.Rows);
    ok(td.Columns == 3, "Columns is %u, expected 3\n", td.Columns);
    ok(td.PackedSize == 0x18, "PackedSize is %#x, expected 0x18\n", td.PackedSize);
    ok(td.UnpackedSize == 0x28, "UnpackedSize is %#x, expected 0x28\n", td.UnpackedSize);
    ok(td.Stride == 0x30, "Stride is %#x, expected 0x30\n", td.Stride);

    /* check bool2x3 b */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "b") == 0, "Name is \"%s\", expected \"b\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 64, "BufferOffset is %u, expected 64\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "bool2x3") == 0, "TypeName is \"%s\", expected \"bool2x3\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_MATRIX_ROWS, "Class is %x, expected %x\n", td.Class, D3D10_SVC_MATRIX_ROWS);
    ok(td.Type == D3D10_SVT_BOOL, "Type is %x, expected %x\n", td.Type, D3D10_SVT_BOOL);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 2, "Rows is %u, expected 2\n", td.Rows);
    ok(td.Columns == 3, "Columns is %u, expected 3\n", td.Columns);
    ok(td.PackedSize == 0x18, "PackedSize is %#x, expected 0x18\n", td.PackedSize);
    ok(td.UnpackedSize == 0x1c, "UnpackedSize is %#x, expected 0x1c\n", td.UnpackedSize);
    ok(td.Stride == 0x20, "Stride is %#x, expected 0x20\n", td.Stride);

    /* check BlendState blend */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "blend") == 0, "Name is \"%s\", expected \"blend\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "BlendState") == 0, "TypeName is \"%s\", expected \"BlendState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_BLEND, "Type is %x, expected %x\n", td.Type, D3D10_SVT_BLEND);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check DepthStencilState depthstencil */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "depthstencil") == 0, "Name is \"%s\", expected \"depthstencil\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "DepthStencilState") == 0, "TypeName is \"%s\", expected \"DepthStencilState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_DEPTHSTENCIL, "Type is %x, expected %x\n", td.Type, D3D10_SVT_DEPTHSTENCIL);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check RasterizerState rast */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "rast") == 0, "Name is \"%s\", expected \"rast\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "RasterizerState") == 0, "TypeName is \"%s\", expected \"RasterizerState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_RASTERIZER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_RASTERIZER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check SamplerState sam */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "sam") == 0, "Name is \"%s\", expected \"sam\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "SamplerState") == 0, "TypeName is \"%s\", expected \"SamplerState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_SAMPLER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_SAMPLER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check RenderTargetView rtv */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "rtv") == 0, "Name is \"%s\", expected \"rtv\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "RenderTargetView") == 0, "TypeName is \"%s\", expected \"RenderTargetView\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_RENDERTARGETVIEW, "Type is %x, expected %x\n", td.Type, D3D10_SVT_RENDERTARGETVIEW);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check DepthStencilView dsv */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "dsv") == 0, "Name is \"%s\", expected \"dsv\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "DepthStencilView") == 0, "TypeName is \"%s\", expected \"DepthStencilView\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_DEPTHSTENCILVIEW, "Type is %x, expected %x\n", td.Type, D3D10_SVT_DEPTHSTENCILVIEW);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture t0 */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(!strcmp(vd.Name, "t0"), "Unexpected Name \"%s\".\n", vd.Name);
    ok(!vd.Semantic, "Unexpected Semantic \"%s\".\n", vd.Semantic);
    ok(!vd.Flags, "Unexpected Flags %u.\n", vd.Flags);
    ok(!vd.Annotations, "Unexpected Annotations %u.\n", vd.Annotations);
    ok(!vd.BufferOffset, "Unexpected BufferOffset %u.\n", vd.BufferOffset);
    ok(!vd.ExplicitBindPoint, "Unexpected ExplicitBindPoint %u.\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "Unexpected parent %p.\n", parent);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(!strcmp(td.TypeName, "texture"), "Unexpected TypeName \"%s\".\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Unexpected Class %x.\n", td.Class);
    ok(td.Type == D3D10_SVT_TEXTURE, "Unexpected Type %x.\n", td.Type);
    ok(!td.Elements, "Unexpected Elements %u.\n", td.Elements);
    ok(!td.Members, "Unexpected Members %u.\n", td.Members);
    ok(!td.Rows, "Unexpected Rows %u.\n", td.Rows);
    ok(!td.Columns, "Unexpected Columns %u.\n", td.Columns);
    ok(!td.PackedSize, "Unexpected PackedSize %#x.\n", td.PackedSize);
    ok(!td.UnpackedSize, "Unexpected UnpackedSize %#x.\n", td.UnpackedSize);
    ok(!td.Stride, "Unexpected Stride %#x.\n", td.Stride);

    /* check Texture1D t1 */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "t1") == 0, "Name is \"%s\", expected \"t1\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "Texture1D") == 0, "TypeName is \"%s\", expected \"Texture1D\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE1D, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE1D);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture1DArray t1a */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "t1a") == 0, "Name is \"%s\", expected \"t1a\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "Texture1DArray") == 0, "TypeName is \"%s\", expected \"Texture1DArray\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE1DARRAY, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE1DARRAY);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2D t2 */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "t2") == 0, "Name is \"%s\", expected \"t2\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "Texture2D") == 0, "TypeName is \"%s\", expected \"Texture2D\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2D, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2D);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2DMS t2dms */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "t2dms") == 0, "Name is \"%s\", expected \"t2dms\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "Texture2DMS") == 0, "TypeName is \"%s\", expected \"Texture2DMS\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2DMS, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2DMS);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2DArray t2a */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "t2a") == 0, "Name is \"%s\", expected \"t2a\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "Texture2DArray") == 0, "TypeName is \"%s\", expected \"Texture2DArray\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2DARRAY, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2DARRAY);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2DMSArray t2dmsa */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "t2dmsa") == 0, "Name is \"%s\", expected \"t2dmsa\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "Texture2DMSArray") == 0, "TypeName is \"%s\", expected \"TTexture2DMSArray\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2DMSARRAY, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2DMSARRAY);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture3D t3 */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "t3") == 0, "Name is \"%s\", expected \"t3\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "Texture3D") == 0, "TypeName is \"%s\", expected \"Texture3D\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE3D, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE3D);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check TextureCube tq */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "tq") == 0, "Name is \"%s\", expected \"tq\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "TextureCube") == 0, "TypeName is \"%s\", expected \"TextureCube\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURECUBE, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURECUBE);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check GeometryShader gs[2] */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "gs") == 0, "Name is \"%s\", expected \"gs\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(td.Elements == 2, "Elements is %u, expected 2\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check PixelShader ps */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "ps") == 0, "Name is \"%s\", expected \"ps\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_PIXELSHADER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check VertexShader vs[1] */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vd.Name, "vs") == 0, "Name is \"%s\", expected \"vs\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(td.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_VERTEXSHADER);
    ok(td.Elements == 1, "Elements is %u, expected 1\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_constant_buffer_stride
 */
#if 0
cbuffer cb1
{
    float   a1;
    float   b1;
    float   c1;
    float   d1;
};
cbuffer cb2
{
    float   a2;
    float2  b2;
};
cbuffer cb3
{
    float2  a3;
    float3  b3;
};
cbuffer cb4
{
    float2  a4;
    float3  b4;
    float2  c4;
};
cbuffer cb5
{
    float2  a5;
    float2  b5;
    float3  c5;
};
cbuffer cb6
{
    float2  a6 : packoffset(c0);
    float3  b6 : packoffset(c1);
    float2  c6 : packoffset(c0.z);
};
cbuffer cb7
{
    float2  a7 : packoffset(c0);
    float3  b7 : packoffset(c1);
    float2  c7 : packoffset(c2);
};
cbuffer cb8
{
    float2  a8 : packoffset(c0);
    float3  b8 : packoffset(c0.y);
    float4  c8 : packoffset(c2);
};
cbuffer cb9
{
    float2  a9 : packoffset(c0);
    float2  b9 : packoffset(c0.y);
    float2  c9 : packoffset(c0.z);
};
cbuffer cb10
{
    float4  a10 : packoffset(c2);
};
cbuffer cb11
{
    struct {
        float4 a11;
        float  b11;
    } s11;
    float  c11;
};
cbuffer cb12
{
    float  c12;
    struct {
        float  b12;
        float4 a12;
    } s12;
};
cbuffer cb13
{
    float  a13;
    struct {
        float  b13;
    } s13;
};
cbuffer cb14
{
    struct {
        float  a14;
    } s14;
    struct {
        float  b14;
    } t14;
};
cbuffer cb15
{
    float2  a15[2] : packoffset(c0);
};
#endif
static DWORD fx_test_ecbs[] = {
0x43425844, 0x615d7d77, 0x21289d92, 0xe9e8d98e,
0xcae7b74e, 0x00000001, 0x00000855, 0x00000001,
0x00000024, 0x30315846, 0x00000829, 0xfeff1001,
0x0000000f, 0x00000024, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000285,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00316263,
0x616f6c66, 0x00080074, 0x00010000, 0x00000000,
0x00040000, 0x00100000, 0x00040000, 0x09090000,
0x31610000, 0x00316200, 0x64003163, 0x62630031,
0x32610032, 0x6f6c6600, 0x00327461, 0x0000003d,
0x00000001, 0x00000000, 0x00000008, 0x00000010,
0x00000008, 0x0000110a, 0x63003262, 0x61003362,
0x6c660033, 0x3374616f, 0x00006a00, 0x00000100,
0x00000000, 0x00000c00, 0x00001000, 0x00000c00,
0x00190a00, 0x00336200, 0x00346263, 0x62003461,
0x34630034, 0x35626300, 0x00356100, 0x63003562,
0x62630035, 0x36610036, 0x00366200, 0x63003663,
0x61003762, 0x37620037, 0x00376300, 0x00386263,
0x62003861, 0x6c660038, 0x3474616f, 0x0000ce00,
0x00000100, 0x00000000, 0x00001000, 0x00001000,
0x00001000, 0x00210a00, 0x00386300, 0x00396263,
0x62003961, 0x39630039, 0x31626300, 0x31610030,
0x62630030, 0x3c003131, 0x616e6e75, 0x3e64656d,
0x31316100, 0x31316200, 0x00010f00, 0x00000300,
0x00000000, 0x00001400, 0x00002000, 0x00001400,
0x00000200, 0x00011900, 0x00000000, 0x00000000,
0x0000d500, 0x00011d00, 0x00000000, 0x00001000,
0x00000e00, 0x31317300, 0x31316300, 0x31626300,
0x31630032, 0x31620032, 0x31610032, 0x010f0032,
0x00030000, 0x00000000, 0x00200000, 0x00200000,
0x00140000, 0x00020000, 0x016e0000, 0x00000000,
0x00000000, 0x000e0000, 0x01720000, 0x00000000,
0x00100000, 0x00d50000, 0x31730000, 0x62630032,
0x61003331, 0x62003331, 0x0f003331, 0x03000001,
0x00000000, 0x04000000, 0x10000000, 0x04000000,
0x01000000, 0xbf000000, 0x00000001, 0x00000000,
0x0e000000, 0x73000000, 0x63003331, 0x00343162,
0x00343161, 0x0000010f, 0x00000003, 0x00000000,
0x00000004, 0x00000010, 0x00000004, 0x00000001,
0x000001f8, 0x00000000, 0x00000000, 0x0000000e,
0x00343173, 0x00343162, 0x0000010f, 0x00000003,
0x00000000, 0x00000004, 0x00000010, 0x00000004,
0x00000001, 0x0000022c, 0x00000000, 0x00000000,
0x0000000e, 0x00343174, 0x35316263, 0x00003d00,
0x00000100, 0x00000200, 0x00001800, 0x00001000,
0x00001000, 0x00110a00, 0x35316100, 0x00000400,
0x00001000, 0x00000000, 0x00000400, 0xffffff00,
0x000000ff, 0x00002a00, 0x00000e00, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00002d00, 0x00000e00, 0x00000000, 0x00000400,
0x00000000, 0x00000000, 0x00000000, 0x00003000,
0x00000e00, 0x00000000, 0x00000800, 0x00000000,
0x00000000, 0x00000000, 0x00003300, 0x00000e00,
0x00000000, 0x00000c00, 0x00000000, 0x00000000,
0x00000000, 0x00003600, 0x00001000, 0x00000000,
0x00000200, 0xffffff00, 0x000000ff, 0x00003a00,
0x00000e00, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00006000, 0x00004400,
0x00000000, 0x00000400, 0x00000000, 0x00000000,
0x00000000, 0x00006300, 0x00002000, 0x00000000,
0x00000200, 0xffffff00, 0x000000ff, 0x00006700,
0x00004400, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00008d00, 0x00007100,
0x00000000, 0x00001000, 0x00000000, 0x00000000,
0x00000000, 0x00009000, 0x00003000, 0x00000000,
0x00000300, 0xffffff00, 0x000000ff, 0x00009400,
0x00004400, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00009700, 0x00007100,
0x00000000, 0x00001000, 0x00000000, 0x00000000,
0x00000000, 0x00009a00, 0x00004400, 0x00000000,
0x00002000, 0x00000000, 0x00000000, 0x00000000,
0x00009d00, 0x00002000, 0x00000000, 0x00000300,
0xffffff00, 0x000000ff, 0x0000a100, 0x00004400,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0000a400, 0x00004400, 0x00000000,
0x00000800, 0x00000000, 0x00000000, 0x00000000,
0x0000a700, 0x00007100, 0x00000000, 0x00001000,
0x00000000, 0x00000000, 0x00000000, 0x0000aa00,
0x00002000, 0x00000000, 0x00000300, 0xffffff00,
0x000000ff, 0x0000ae00, 0x00004400, 0x00000000,
0x00000000, 0x00000000, 0x00000400, 0x00000000,
0x0000b100, 0x00007100, 0x00000000, 0x00001000,
0x00000000, 0x00000400, 0x00000000, 0x0000b400,
0x00004400, 0x00000000, 0x00000800, 0x00000000,
0x00000400, 0x00000000, 0x0000b700, 0x00003000,
0x00000000, 0x00000300, 0xffffff00, 0x000000ff,
0x0000bb00, 0x00004400, 0x00000000, 0x00000000,
0x00000000, 0x00000400, 0x00000000, 0x0000be00,
0x00007100, 0x00000000, 0x00001000, 0x00000000,
0x00000400, 0x00000000, 0x0000c100, 0x00004400,
0x00000000, 0x00002000, 0x00000000, 0x00000400,
0x00000000, 0x0000c400, 0x00003000, 0x00000000,
0x00000300, 0xffffff00, 0x000000ff, 0x0000c800,
0x00004400, 0x00000000, 0x00000000, 0x00000000,
0x00000400, 0x00000000, 0x0000cb00, 0x00007100,
0x00000000, 0x00000400, 0x00000000, 0x00000400,
0x00000000, 0x0000f100, 0x0000d500, 0x00000000,
0x00002000, 0x00000000, 0x00000400, 0x00000000,
0x0000f400, 0x00001000, 0x00000000, 0x00000300,
0xffffff00, 0x000000ff, 0x0000f800, 0x00004400,
0x00000000, 0x00000000, 0x00000000, 0x00000400,
0x00000000, 0x0000fb00, 0x00004400, 0x00000000,
0x00000400, 0x00000000, 0x00000400, 0x00000000,
0x0000fe00, 0x00004400, 0x00000000, 0x00000800,
0x00000000, 0x00000400, 0x00000000, 0x00010100,
0x00003000, 0x00000000, 0x00000100, 0xffffff00,
0x000000ff, 0x00010600, 0x0000d500, 0x00000000,
0x00002000, 0x00000000, 0x00000400, 0x00000000,
0x00010a00, 0x00002000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x00015d00, 0x00012100,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00016100, 0x00000e00, 0x00000000,
0x00001400, 0x00000000, 0x00000000, 0x00000000,
0x00016500, 0x00003000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x00016a00, 0x00000e00,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0001b200, 0x00017600, 0x00000000,
0x00001000, 0x00000000, 0x00000000, 0x00000000,
0x0001b600, 0x00002000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x0001bb00, 0x00000e00,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0001ef00, 0x0001c300, 0x00000000,
0x00001000, 0x00000000, 0x00000000, 0x00000000,
0x0001f300, 0x00002000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x00022800, 0x0001fc00,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00025c00, 0x00023000, 0x00000000,
0x00001000, 0x00000000, 0x00000000, 0x00000000,
0x00026000, 0x00002000, 0x00000000, 0x00000100,
0xffffff00, 0x000000ff, 0x00028100, 0x00026500,
0x00000000, 0x00000000, 0x00000000, 0x00000400,
0x00000000, 0x00000000,
};

static void test_effect_constant_buffer_stride(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectType *type;
    D3D10_EFFECT_TYPE_DESC tdesc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    unsigned int i;

    static const struct {
        unsigned int m; /* members */
        unsigned int p; /* packed size */
        unsigned int u; /* unpacked size */
        unsigned int s; /* stride */
    } tv_ecbs[] = {
        {4, 0x10,  0x10,  0x10},
        {2,  0xc,  0x10,  0x10},
        {2, 0x14,  0x20,  0x20},
        {3, 0x1c,  0x30,  0x30},
        {3, 0x1c,  0x20,  0x20},
        {3, 0x1c,  0x20,  0x20},
        {3, 0x1c,  0x30,  0x30},
        {3, 0x24,  0x30,  0x30},
        {3, 0x18,  0x10,  0x10},
        {1, 0x10,  0x30,  0x30},
        {2, 0x18,  0x20,  0x20},
        {2, 0x18,  0x30,  0x30},
        {2,  0x8,  0x20,  0x20},
        {2,  0x8,  0x20,  0x20},
        {1, 0x10,  0x20,  0x20},
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_ecbs, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 15, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 36, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    for (i=0; i<ARRAY_SIZE(tv_ecbs); i++)
    {
        constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, i);
        type = constantbuffer->lpVtbl->GetType(constantbuffer);

        hr = type->lpVtbl->GetDesc(type, &tdesc);
        ok(hr == S_OK, "%u: Got unexpected hr %#lx.\n", i, hr);

        ok(strcmp(tdesc.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", tdesc.TypeName);
        ok(tdesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", tdesc.Class, D3D10_SVC_OBJECT);
        ok(tdesc.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", tdesc.Type, D3D10_SVT_CBUFFER);
        ok(tdesc.Elements == 0, "Elements is %u, expected 0\n", tdesc.Elements);
        ok(tdesc.Members == tv_ecbs[i].m, "Members is %u, expected %u\n", tdesc.Members, tv_ecbs[i].m);
        ok(tdesc.Rows == 0, "Rows is %u, expected 0\n", tdesc.Rows);
        ok(tdesc.Columns == 0, "Columns is %u, expected 0\n", tdesc.Columns);
        ok(tdesc.PackedSize == tv_ecbs[i].p, "PackedSize is %#x, expected %#x\n", tdesc.PackedSize, tv_ecbs[i].p);
        ok(tdesc.UnpackedSize == tv_ecbs[i].u, "UnpackedSize is %#x, expected %#x\n", tdesc.UnpackedSize, tv_ecbs[i].u);
        ok(tdesc.Stride == tv_ecbs[i].s, "Stride is %#x, expected %#x\n", tdesc.Stride, tv_ecbs[i].s);
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
float4 VS( float4 Pos : POSITION ) : SV_POSITION { return Pos; }
float4 VS2( int4 Pos : POSITION ) : SV_POSITION { return Pos; }
float4 PS( float4 Pos : SV_POSITION ) : SV_Target { return float4( 1.0f, 1.0f, 0.0f, 1.0f ); }
struct GS_OUT { float4 Pos : SV_POSITION; };
[maxvertexcount(3)]
void GS( triangle float4 Pos[3] : SV_POSITION, inout TriangleStream<GS_OUT> TriStream )
{
     GS_OUT out1;
     out1.Pos = Pos[0];
     TriStream.Append( out1 );
     out1.Pos = Pos[1];
     TriStream.Append( out1 );
     out1.Pos = Pos[2];
     TriStream.Append( out1 );
     TriStream.RestartStrip();
}
VertexShader v0 = NULL;
PixelShader p0 = NULL;
GeometryShader g0 = NULL;
VertexShader v[2] = { CompileShader( vs_4_0, VS() ), CompileShader( vs_4_0, VS2() ) };
PixelShader p = CompileShader( ps_4_0, PS() );
GeometryShader g = CompileShader( gs_4_0, GS() );
GeometryShader g_so = ConstructGSWithSO
(
    CompileShader( gs_4_0, GS()), "SV_POSITION.x; $Skip.x; SV_POSITION.gb"
);
technique10 Render
{
    pass P0 {}
    pass P1
    {
        SetPixelShader( NULL );
        SetVertexShader( NULL );
        SetGeometryShader( NULL );
    }
    pass P2
    {
        SetPixelShader( NULL );
        SetVertexShader( NULL );
        SetGeometryShader( NULL );
    }
    pass P3
    {
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, GS() ) );
    }
    pass P4
    {
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
        SetVertexShader( CompileShader( vs_4_0, VS2() ) );
        SetGeometryShader( CompileShader( gs_4_0, GS() ) );
    }
    pass P5
    {
        SetPixelShader( p0 );
        SetVertexShader( v0 );
        SetGeometryShader( g0 );
    }
    pass P6
    {
        SetPixelShader( p );
        SetVertexShader( v[0] );
        SetGeometryShader( g );
    }
    pass P7
    {
        SetPixelShader( p );
        SetVertexShader( v[1] );
        SetGeometryShader( g );
    }
    pass P8
    {
        SetPixelShader( p );
        SetVertexShader( v[1] );
        SetGeometryShader( ConstructGSWithSO(CompileShader( gs_4_0, GS()), "SV_POSITION.y") );
    }
}
#endif
static DWORD fx_local_shader[] =
{
    0x43425844, 0x37f7879b, 0xb6640b9a, 0x373a9d5a, 0x84b37ed2, 0x00000001, 0x00001aab, 0x00000001,
    0x00000024, 0x30315846, 0x00001a7f, 0xfeff1001, 0x00000000, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x0000178b, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000000f, 0x00000007, 0x00000000, 0x74726556,
    0x68537865, 0x72656461, 0x00000400, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000600, 0x00307600, 0x65786950, 0x6168536c, 0x00726564, 0x00000030, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x47003070, 0x656d6f65, 0x53797274, 0x65646168,
    0x005b0072, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00070000, 0x30670000,
    0x00000400, 0x00000200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000600, 0xa8007600,
    0x44000001, 0x92434258, 0xa75ea3fc, 0x13affa34, 0x46be1a21, 0x0104d04a, 0xa8000000, 0x05000001,
    0x34000000, 0x80000000, 0xb4000000, 0xe8000000, 0x2c000000, 0x52000001, 0x44464544, 0x00000000,
    0x00000000, 0x00000000, 0x1c000000, 0x00000000, 0x00fffe04, 0x1c000001, 0x4d000000, 0x6f726369,
    0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072,
    0x4900312e, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000, 0x00000000, 0x00000000, 0x03000000,
    0x00000000, 0x0f000000, 0x5000000f, 0x5449534f, 0x004e4f49, 0x4fababab, 0x2c4e4753, 0x01000000,
    0x08000000, 0x20000000, 0x00000000, 0x01000000, 0x03000000, 0x00000000, 0x0f000000, 0x53000000,
    0x4f505f56, 0x49544953, 0x53004e4f, 0x3c524448, 0x40000000, 0x0f000100, 0x5f000000, 0xf2030000,
    0x00001010, 0x67000000, 0xf2040000, 0x00001020, 0x01000000, 0x36000000, 0xf2050000, 0x00001020,
    0x46000000, 0x0000101e, 0x3e000000, 0x53010000, 0x74544154, 0x02000000, 0x00000000, 0x00000000,
    0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xa8000000, 0x44000001, 0x84434258, 0xa05ac55b, 0x929a6a46, 0x4f0df40a,
    0x01a2fea3, 0xa8000000, 0x05000001, 0x34000000, 0x80000000, 0xb4000000, 0xe8000000, 0x2c000000,
    0x52000001, 0x44464544, 0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x00000000, 0x00fffe04,
    0x1c000001, 0x4d000000, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
    0x6d6f4320, 0x656c6970, 0x30312072, 0x4900312e, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000,
    0x00000000, 0x00000000, 0x02000000, 0x00000000, 0x0f000000, 0x5000000f, 0x5449534f, 0x004e4f49,
    0x4fababab, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000, 0x00000000, 0x01000000, 0x03000000,
    0x00000000, 0x0f000000, 0x53000000, 0x4f505f56, 0x49544953, 0x53004e4f, 0x3c524448, 0x40000000,
    0x0f000100, 0x5f000000, 0xf2030000, 0x00001010, 0x67000000, 0xf2040000, 0x00001020, 0x01000000,
    0x2b000000, 0xf2050000, 0x00001020, 0x46000000, 0x0000101e, 0x3e000000, 0x53010000, 0x74544154,
    0x02000000, 0x00000000, 0x00000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x70000000, 0x0001a400, 0x42584400,
    0xf4813443, 0xb6605b8e, 0xc69999b8, 0xcb75f1b5, 0x000001eb, 0x0001a400, 0x00000500, 0x00003400,
    0x00008000, 0x0000b400, 0x0000e800, 0x00012800, 0x45445200, 0x00004446, 0x00000000, 0x00000000,
    0x00000000, 0x00001c00, 0xff040000, 0x000100ff, 0x00001c00, 0x63694d00, 0x6f736f72, 0x28207466,
    0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x312e3031, 0x47534900,
    0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000,
    0x00000f00, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x47534f00, 0x00002c4e, 0x00000100, 0x00000800,
    0x00002000, 0x00000000, 0x00000000, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x67726154,
    0xab007465, 0x444853ab, 0x00003852, 0x00004000, 0x00000e00, 0x00006500, 0x1020f203, 0x00000000,
    0x00003600, 0x1020f208, 0x00000000, 0x00400200, 0x80000000, 0x8000003f, 0x0000003f, 0x80000000,
    0x00003e3f, 0x41545301, 0x00007454, 0x00000200, 0x00000000, 0x00000000, 0x00000100, 0x00000000,
    0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x04006700, 0x44000002, 0x23434258, 0x6e8e325e, 0x5f0774e8, 0xbfe636e1, 0x01d3f67a, 0x04000000,
    0x05000002, 0x34000000, 0x80000000, 0xb4000000, 0xe8000000, 0x88000000, 0x52000001, 0x44464544,
    0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x00000000, 0x00475304, 0x1c000001, 0x4d000000,
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
    0x30312072, 0x4900312e, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000, 0x00000000, 0x01000000,
    0x03000000, 0x00000000, 0x0f000000, 0x5300000f, 0x4f505f56, 0x49544953, 0x4f004e4f, 0x2c4e4753,
    0x01000000, 0x08000000, 0x20000000, 0x00000000, 0x01000000, 0x03000000, 0x00000000, 0x0f000000,
    0x53000000, 0x4f505f56, 0x49544953, 0x53004e4f, 0x98524448, 0x40000000, 0x26000200, 0x61000000,
    0xf2050000, 0x03002010, 0x00000000, 0x01000000, 0x5d000000, 0x5c010018, 0x67010028, 0xf2040000,
    0x00001020, 0x01000000, 0x5e000000, 0x03020000, 0x36000000, 0xf2060000, 0x00001020, 0x46000000,
    0x0000201e, 0x00000000, 0x13000000, 0x36010000, 0xf2060000, 0x00001020, 0x46000000, 0x0100201e,
    0x00000000, 0x13000000, 0x36010000, 0xf2060000, 0x00001020, 0x46000000, 0x0200201e, 0x00000000,
    0x13000000, 0x09010000, 0x3e010000, 0x53010000, 0x74544154, 0x08000000, 0x00000000, 0x00000000,
    0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x01000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x05000000, 0x03000000, 0x00000000,
    0x00000000, 0x00000000, 0x5b000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x08000000, 0x67000000, 0x006f735f, 0x00000204, 0x43425844, 0x8e325e23, 0x0774e86e, 0xe636e15f,
    0xd3f67abf, 0x00000001, 0x00000204, 0x00000005, 0x00000034, 0x00000080, 0x000000b4, 0x000000e8,
    0x00000188, 0x46454452, 0x00000044, 0x00000000, 0x00000000, 0x00000000, 0x0000001c, 0x47530400,
    0x00000100, 0x0000001c, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168,
    0x6f432072, 0x6c69706d, 0x31207265, 0x00312e30, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008,
    0x00000020, 0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x00000f0f, 0x505f5653, 0x5449534f,
    0x004e4f49, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001,
    0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x00000098,
    0x00020040, 0x00000026, 0x05000061, 0x002010f2, 0x00000003, 0x00000000, 0x00000001, 0x0100185d,
    0x0100285c, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x0200005e, 0x00000003, 0x06000036,
    0x001020f2, 0x00000000, 0x00201e46, 0x00000000, 0x00000000, 0x01000013, 0x06000036, 0x001020f2,
    0x00000000, 0x00201e46, 0x00000001, 0x00000000, 0x01000013, 0x06000036, 0x001020f2, 0x00000000,
    0x00201e46, 0x00000002, 0x00000000, 0x01000013, 0x01000009, 0x0100003e, 0x54415453, 0x00000074,
    0x00000008, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003,
    0x00000005, 0x00000003, 0x00000000, 0x00000000, 0x00000000, 0x505f5653, 0x5449534f, 0x2e4e4f49,
    0x24203b78, 0x70696b53, 0x203b782e, 0x505f5653, 0x5449534f, 0x2e4e4f49, 0x52006267, 0x65646e65,
    0x30500072, 0x00315000, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000,
    0x00000001, 0x00000002, 0x00000000, 0x01003250, 0x02000000, 0x00000000, 0x01000000, 0x02000000,
    0x00000000, 0x01000000, 0x02000000, 0x00000000, 0x50000000, 0x01a40033, 0x58440000, 0x81344342,
    0x605b8ef4, 0x9999b8b6, 0x75f1b5c6, 0x0001ebcb, 0x01a40000, 0x00050000, 0x00340000, 0x00800000,
    0x00b40000, 0x00e80000, 0x01280000, 0x44520000, 0x00444645, 0x00000000, 0x00000000, 0x00000000,
    0x001c0000, 0x04000000, 0x0100ffff, 0x001c0000, 0x694d0000, 0x736f7263, 0x2074666f, 0x20295228,
    0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x2e303120, 0x53490031, 0x002c4e47,
    0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x000f0000,
    0x56530000, 0x534f505f, 0x4f495449, 0x534f004e, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000,
    0x00000000, 0x00000000, 0x00030000, 0x00000000, 0x000f0000, 0x56530000, 0x7261545f, 0x00746567,
    0x4853abab, 0x00385244, 0x00400000, 0x000e0000, 0x00650000, 0x20f20300, 0x00000010, 0x00360000,
    0x20f20800, 0x00000010, 0x40020000, 0x00000000, 0x00003f80, 0x00003f80, 0x00000000, 0x003e3f80,
    0x54530100, 0x00745441, 0x00020000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000,
    0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0a5e0000,
    0x00000000, 0x01a80000, 0x58440000, 0xfc924342, 0x34a75ea3, 0x2113affa, 0x4a46be1a, 0x000104d0,
    0x01a80000, 0x00050000, 0x00340000, 0x00800000, 0x00b40000, 0x00e80000, 0x012c0000, 0x44520000,
    0x00444645, 0x00000000, 0x00000000, 0x00000000, 0x001c0000, 0x04000000, 0x0100fffe, 0x001c0000,
    0x694d0000, 0x736f7263, 0x2074666f, 0x20295228, 0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43,
    0x72656c69, 0x2e303120, 0x53490031, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000,
    0x00000000, 0x00030000, 0x00000000, 0x0f0f0000, 0x4f500000, 0x49544953, 0xab004e4f, 0x534fabab,
    0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000,
    0x000f0000, 0x56530000, 0x534f505f, 0x4f495449, 0x4853004e, 0x003c5244, 0x00400000, 0x000f0001,
    0x005f0000, 0x10f20300, 0x00000010, 0x00670000, 0x20f20400, 0x00000010, 0x00010000, 0x00360000,
    0x20f20500, 0x00000010, 0x1e460000, 0x00000010, 0x003e0000, 0x54530100, 0x00745441, 0x00020000,
    0x00000000, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0c0e0000, 0x00000000, 0x02040000, 0x58440000,
    0x5e234342, 0xe86e8e32, 0xe15f0774, 0x7abfe636, 0x0001d3f6, 0x02040000, 0x00050000, 0x00340000,
    0x00800000, 0x00b40000, 0x00e80000, 0x01880000, 0x44520000, 0x00444645, 0x00000000, 0x00000000,
    0x00000000, 0x001c0000, 0x04000000, 0x01004753, 0x001c0000, 0x694d0000, 0x736f7263, 0x2074666f,
    0x20295228, 0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x2e303120, 0x53490031,
    0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000,
    0x0f0f0000, 0x56530000, 0x534f505f, 0x4f495449, 0x534f004e, 0x002c4e47, 0x00010000, 0x00080000,
    0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x000f0000, 0x56530000, 0x534f505f,
    0x4f495449, 0x4853004e, 0x00985244, 0x00400000, 0x00260002, 0x00610000, 0x10f20500, 0x00030020,
    0x00000000, 0x00010000, 0x185d0000, 0x285c0100, 0x00670100, 0x20f20400, 0x00000010, 0x00010000,
    0x005e0000, 0x00030200, 0x00360000, 0x20f20600, 0x00000010, 0x1e460000, 0x00000020, 0x00000000,
    0x00130000, 0x00360100, 0x20f20600, 0x00000010, 0x1e460000, 0x00010020, 0x00000000, 0x00130000,
    0x00360100, 0x20f20600, 0x00000010, 0x1e460000, 0x00020020, 0x00000000, 0x00130000, 0x00090100,
    0x003e0100, 0x54530100, 0x00745441, 0x00080000, 0x00000000, 0x00000000, 0x00020000, 0x00000000,
    0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00010000,
    0x00030000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00030000, 0x00050000, 0x00030000, 0x00000000, 0x00000000, 0x00000000,
    0x0dc20000, 0x00000000, 0x34500000, 0x0001a400, 0x42584400, 0xf4813443, 0xb6605b8e, 0xc69999b8,
    0xcb75f1b5, 0x000001eb, 0x0001a400, 0x00000500, 0x00003400, 0x00008000, 0x0000b400, 0x0000e800,
    0x00012800, 0x45445200, 0x00004446, 0x00000000, 0x00000000, 0x00000000, 0x00001c00, 0xff040000,
    0x000100ff, 0x00001c00, 0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c, 0x64616853,
    0x43207265, 0x69706d6f, 0x2072656c, 0x312e3031, 0x47534900, 0x00002c4e, 0x00000100, 0x00000800,
    0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x49534f50,
    0x4e4f4954, 0x47534f00, 0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000000,
    0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x67726154, 0xab007465, 0x444853ab, 0x00003852,
    0x00004000, 0x00000e00, 0x00006500, 0x1020f203, 0x00000000, 0x00003600, 0x1020f208, 0x00000000,
    0x00400200, 0x80000000, 0x8000003f, 0x0000003f, 0x80000000, 0x00003e3f, 0x41545301, 0x00007454,
    0x00000200, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000100,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000fd500, 0x00000000, 0x0001a800,
    0x42584400, 0xc55b8443, 0x6a46a05a, 0xf40a929a, 0xfea34f0d, 0x000001a2, 0x0001a800, 0x00000500,
    0x00003400, 0x00008000, 0x0000b400, 0x0000e800, 0x00012c00, 0x45445200, 0x00004446, 0x00000000,
    0x00000000, 0x00000000, 0x00001c00, 0xfe040000, 0x000100ff, 0x00001c00, 0x63694d00, 0x6f736f72,
    0x28207466, 0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x312e3031,
    0x47534900, 0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000000, 0x00000200,
    0x00000000, 0x000f0f00, 0x534f5000, 0x4f495449, 0xabab004e, 0x47534fab, 0x00002c4e, 0x00000100,
    0x00000800, 0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300,
    0x49534f50, 0x4e4f4954, 0x44485300, 0x00003c52, 0x01004000, 0x00000f00, 0x00005f00, 0x1010f203,
    0x00000000, 0x00006700, 0x1020f204, 0x00000000, 0x00000100, 0x00002b00, 0x1020f205, 0x00000000,
    0x101e4600, 0x00000000, 0x00003e00, 0x41545301, 0x00007454, 0x00000200, 0x00000000, 0x00000000,
    0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00118500, 0x00000000, 0x00020400, 0x42584400, 0x325e2343, 0x74e86e8e,
    0x36e15f07, 0xf67abfe6, 0x000001d3, 0x00020400, 0x00000500, 0x00003400, 0x00008000, 0x0000b400,
    0x0000e800, 0x00018800, 0x45445200, 0x00004446, 0x00000000, 0x00000000, 0x00000000, 0x00001c00,
    0x53040000, 0x00010047, 0x00001c00, 0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c,
    0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x312e3031, 0x47534900, 0x00002c4e, 0x00000100,
    0x00000800, 0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000, 0x000f0f00, 0x5f565300,
    0x49534f50, 0x4e4f4954, 0x47534f00, 0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000,
    0x00000100, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x44485300,
    0x00009852, 0x02004000, 0x00002600, 0x00006100, 0x2010f205, 0x00000300, 0x00000000, 0x00000100,
    0x00185d00, 0x00285c01, 0x00006701, 0x1020f204, 0x00000000, 0x00000100, 0x00005e00, 0x00000302,
    0x00003600, 0x1020f206, 0x00000000, 0x201e4600, 0x00000000, 0x00000000, 0x00001300, 0x00003601,
    0x1020f206, 0x00000000, 0x201e4600, 0x00000100, 0x00000000, 0x00001300, 0x00003601, 0x1020f206,
    0x00000000, 0x201e4600, 0x00000200, 0x00000000, 0x00001300, 0x00000901, 0x00003e01, 0x41545301,
    0x00007454, 0x00000800, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000,
    0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000300, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000300, 0x00000500, 0x00000300, 0x00000000, 0x00000000, 0x00000000, 0x00133900, 0x00000000,
    0x00355000, 0xa5003650, 0x00000000, 0x50000000, 0x00a50037, 0x00010000, 0x38500000, 0x0000a500,
    0x00000100, 0x00020400, 0x42584400, 0x325e2343, 0x74e86e8e, 0x36e15f07, 0xf67abfe6, 0x000001d3,
    0x00020400, 0x00000500, 0x00003400, 0x00008000, 0x0000b400, 0x0000e800, 0x00018800, 0x45445200,
    0x00004446, 0x00000000, 0x00000000, 0x00000000, 0x00001c00, 0x53040000, 0x00010047, 0x00001c00,
    0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f,
    0x2072656c, 0x312e3031, 0x47534900, 0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000,
    0x00000100, 0x00000300, 0x00000000, 0x000f0f00, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x47534f00,
    0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000,
    0x00000f00, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x44485300, 0x00009852, 0x02004000, 0x00002600,
    0x00006100, 0x2010f205, 0x00000300, 0x00000000, 0x00000100, 0x00185d00, 0x00285c01, 0x00006701,
    0x1020f204, 0x00000000, 0x00000100, 0x00005e00, 0x00000302, 0x00003600, 0x1020f206, 0x00000000,
    0x201e4600, 0x00000000, 0x00000000, 0x00001300, 0x00003601, 0x1020f206, 0x00000000, 0x201e4600,
    0x00000100, 0x00000000, 0x00001300, 0x00003601, 0x1020f206, 0x00000000, 0x201e4600, 0x00000200,
    0x00000000, 0x00001300, 0x00000901, 0x00003e01, 0x41545301, 0x00007454, 0x00000800, 0x00000000,
    0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000100, 0x00000300, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000300, 0x00000500, 0x00000300,
    0x00000000, 0x00000000, 0x00000000, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x6d00792e, 0x75000015,
    0x2d000017, 0x11000000, 0x00000000, 0xff000000, 0x00ffffff, 0x00000000, 0x58000000, 0x3c000000,
    0x00000000, 0xff000000, 0x00ffffff, 0x00000000, 0x86000000, 0x6a000000, 0x00000000, 0xff000000,
    0x00ffffff, 0x00000000, 0xa5000000, 0x89000000, 0x00000000, 0xff000000, 0xa7ffffff, 0x53000000,
    0x00000002, 0xff000000, 0x3c000003, 0x00000000, 0xff000000, 0x01ffffff, 0x00000004, 0xa9000000,
    0x6a000005, 0x00000000, 0xff000000, 0xabffffff, 0x00000005, 0xcf000000, 0xb3000007, 0x00000007,
    0xff000000, 0xd4ffffff, 0xdc000007, 0x00000009, 0x03000000, 0x0900000a, 0x00000000, 0x0a000000,
    0x0000000a, 0x00000000, 0x0d000000, 0x0300000a, 0x00000000, 0x07000000, 0x00000000, 0x01000000,
    0x10000000, 0x0600000a, 0x00000000, 0x01000000, 0x1c000000, 0x0800000a, 0x00000000, 0x01000000,
    0x28000000, 0x3400000a, 0x0300000a, 0x00000000, 0x07000000, 0x00000000, 0x01000000, 0x37000000,
    0x0600000a, 0x00000000, 0x01000000, 0x43000000, 0x0800000a, 0x00000000, 0x01000000, 0x4f000000,
    0x5b00000a, 0x0300000a, 0x00000000, 0x07000000, 0x00000000, 0x07000000, 0x06000000, 0x0600000c,
    0x00000000, 0x07000000, 0xba000000, 0x0800000d, 0x00000000, 0x07000000, 0xca000000, 0xd200000f,
    0x0300000f, 0x00000000, 0x07000000, 0x00000000, 0x07000000, 0x7d000000, 0x06000011, 0x00000000,
    0x07000000, 0x31000000, 0x08000013, 0x00000000, 0x07000000, 0x41000000, 0x49000015, 0x03000015,
    0x00000000, 0x07000000, 0x00000000, 0x02000000, 0x58000000, 0x06000000, 0x00000000, 0x02000000,
    0x2d000000, 0x08000000, 0x00000000, 0x02000000, 0x86000000, 0x4c000000, 0x03000015, 0x00000000,
    0x07000000, 0x00000000, 0x02000000, 0xff000000, 0x06000003, 0x00000000, 0x03000000, 0x4f000000,
    0x08000015, 0x00000000, 0x02000000, 0xa9000000, 0x57000005, 0x03000015, 0x00000000, 0x07000000,
    0x00000000, 0x02000000, 0xff000000, 0x06000003, 0x00000000, 0x03000000, 0x5a000000, 0x08000015,
    0x00000000, 0x02000000, 0xa9000000, 0x62000005, 0x03000015, 0x00000000, 0x07000000, 0x00000000,
    0x02000000, 0xff000000, 0x06000003, 0x00000000, 0x03000000, 0x65000000, 0x08000015, 0x00000000,
    0x07000000, 0x83000000, 0x00000017,
};

static void test_effect_local_shader(void)
{
    HRESULT hr;
    BOOL ret;
    ID3D10Effect* effect;
    ID3D10EffectVariable* v;
    ID3D10EffectPass *p, *p2, *null_pass;
    ID3D10EffectTechnique *t, *t2, *null_technique;
    D3D10_PASS_SHADER_DESC pdesc = {0};
    D3D10_EFFECT_VARIABLE_DESC vdesc = {0};
    ID3D10EffectType *type;
    D3D10_EFFECT_TYPE_DESC typedesc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10EffectShaderVariable *null_shader, *null_anon_vs, *null_anon_ps, *null_anon_gs,
        *p3_anon_vs, *p3_anon_ps, *p3_anon_gs, *p6_vs, *p6_ps, *p6_gs, *gs, *ps, *vs;
    ID3D10PixelShader *ps_d3d, *ps_d3d_2;
    D3D10_EFFECT_SHADER_DESC shaderdesc;
    D3D10_SIGNATURE_PARAMETER_DESC sign;
    D3D10_STATE_BLOCK_MASK mask;
    D3D10_PASS_DESC pass_desc;
    ID3D10Device *device;
    ULONG refcount;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_local_shader, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 0, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 7, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 1, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    null_technique = effect->lpVtbl->GetTechniqueByIndex(effect, 0xffffffff);
    null_pass = null_technique->lpVtbl->GetPassByIndex(null_technique, 0xffffffff);

    /* check technique */
    t = effect->lpVtbl->GetTechniqueByName(effect, NULL);
    ok(null_technique == t, "GetTechniqueByName got %p, expected %p\n", t, null_technique);

    t = effect->lpVtbl->GetTechniqueByName(effect, "invalid");
    ok(null_technique == t, "GetTechniqueByName got %p, expected %p\n", t, null_technique);

    t = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    ok(null_technique != t, "GetTechniqueByIndex failed %p\n", t);

    t2 = effect->lpVtbl->GetTechniqueByName(effect, "Render");
    ok(t2 == t, "GetTechniqueByName got %p, expected %p\n", t2, t);

    /* check invalid pass arguments */
    p = null_technique->lpVtbl->GetPassByName(null_technique, NULL);
    ok(null_pass == p, "GetPassByName got %p, expected %p\n", p, null_pass);

    p = null_technique->lpVtbl->GetPassByName(null_technique, "invalid");
    ok(null_pass == p, "GetPassByName got %p, expected %p\n", p, null_pass);

if (0)
{
    /* This crashes on W7/DX10, if t is a valid technique and name=NULL. */
    p = t->lpVtbl->GetPassByName(t, NULL);
    ok(null_pass == p, "GetPassByName got %p, expected %p\n", p, null_pass);
}

    p = t->lpVtbl->GetPassByIndex(t, 0xffffffff);
    ok(null_pass == p, "GetPassByIndex got %p, expected %p\n", p, null_pass);

    /* check for invalid arguments */
    hr = null_pass->lpVtbl->GetVertexShaderDesc(null_pass, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = null_pass->lpVtbl->GetVertexShaderDesc(null_pass, &pdesc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = null_pass->lpVtbl->GetPixelShaderDesc(null_pass, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = null_pass->lpVtbl->GetPixelShaderDesc(null_pass, &pdesc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = null_pass->lpVtbl->GetGeometryShaderDesc(null_pass, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = null_pass->lpVtbl->GetGeometryShaderDesc(null_pass, &pdesc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* check valid pass arguments */
    t = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    p = t->lpVtbl->GetPassByIndex(t, 0);

    p2 = t->lpVtbl->GetPassByName(t, "P0");
    ok(p2 == p, "GetPassByName got %p, expected %p\n", p2, p);

    hr = p->lpVtbl->GetVertexShaderDesc(p, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = p->lpVtbl->GetPixelShaderDesc(p, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = p->lpVtbl->GetGeometryShaderDesc(p, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    v = effect->lpVtbl->GetVariableByName(effect, "p");
    ps = v->lpVtbl->AsShader(v);

    hr = ps->lpVtbl->GetPixelShader(ps, 0, &ps_d3d);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* get the null_shader_variable */
    v = effect->lpVtbl->GetVariableByIndex(effect, 10000);
    null_shader = v->lpVtbl->AsShader(v);

    /* pass 0 */
    p = t->lpVtbl->GetPassByIndex(t, 0);

    /* Pass without Set*Shader() instructions */
    hr = D3D10StateBlockMaskDisableAll(&mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = p->lpVtbl->ComputeStateBlockMask(p, &mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_VS, 0);
    ok(!ret, "Unexpected mask.\n");
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_PS, 0);
    ok(!ret, "Unexpected mask.\n");
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_GS, 0);
    ok(!ret, "Unexpected mask.\n");

    ID3D10Device_PSSetShader(device, ps_d3d);
    hr = p->lpVtbl->Apply(p, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_PSGetShader(device, &ps_d3d_2);
    ok(ps_d3d_2 == ps_d3d, "Unexpected shader object.\n");
    ID3D10PixelShader_Release(ps_d3d_2);

    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == null_shader, "Got %p, expected %p\n", pdesc.pShaderVariable, null_shader);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == null_shader, "Got %p, expected %p\n", pdesc.pShaderVariable, null_shader);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == null_shader, "Got %p, expected %p\n", pdesc.pShaderVariable, null_shader);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* pass 1 */
    p = t->lpVtbl->GetPassByIndex(t, 1);

    ID3D10Device_PSSetShader(device, ps_d3d);
    hr = p->lpVtbl->Apply(p, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_PSGetShader(device, &ps_d3d_2);
    ok(!ps_d3d_2, "Unexpected shader object.\n");

    /* pass 1 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    null_anon_vs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "vertexshader") == 0, "TypeName is \"%s\", expected \"vertexhader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 1 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    null_anon_ps = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "pixelshader") == 0, "TypeName is \"%s\", expected \"pixelshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 1 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    null_anon_gs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "geometryshader") == 0, "TypeName is \"%s\", expected \"geometryshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* Pass is using Set*Shader(NULL) */
    hr = D3D10StateBlockMaskDisableAll(&mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = p->lpVtbl->ComputeStateBlockMask(p, &mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_VS, 0);
    ok(ret, "Unexpected mask.\n");
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_PS, 0);
    ok(ret, "Unexpected mask.\n");
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_GS, 0);
    ok(ret, "Unexpected mask.\n");

    /* pass 2 */
    p = t->lpVtbl->GetPassByIndex(t, 2);

    /* pass 2 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == null_anon_vs, "Got %p, expected %p\n", pdesc.pShaderVariable, null_anon_vs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    /* pass 2 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == null_anon_ps, "Got %p, expected %p\n", pdesc.pShaderVariable, null_anon_ps);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    /* pass 2 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == null_anon_gs, "Got %p, expected %p\n", pdesc.pShaderVariable, null_anon_gs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    /* pass 3 */
    p = t->lpVtbl->GetPassByIndex(t, 3);

    /* pass 3 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    p3_anon_vs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "vertexshader") == 0, "TypeName is \"%s\", expected \"vertexshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 3 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    p3_anon_ps = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "pixelshader") == 0, "TypeName is \"%s\", expected \"pixelshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 3 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    p3_anon_gs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "geometryshader") == 0, "TypeName is \"%s\", expected \"geometryshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 4 */
    p = t->lpVtbl->GetPassByIndex(t, 4);

    /* pass 4 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable != p3_anon_vs, "Got %p, expected %p\n", pdesc.pShaderVariable, p3_anon_vs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "vertexshader") == 0, "TypeName is \"%s\", expected \"vertexshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 4 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable != p3_anon_ps, "Got %p, expected %p\n", pdesc.pShaderVariable, p3_anon_ps);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "pixelshader") == 0, "TypeName is \"%s\", expected \"pixelshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 4 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable != p3_anon_gs, "Got %p, expected %p\n", pdesc.pShaderVariable, p3_anon_gs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %x\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "geometryshader") == 0, "TypeName is \"%s\", expected \"geometryshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 5 */
    p = t->lpVtbl->GetPassByIndex(t, 5);

    /* pass 5 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "v0") == 0, "Name is \"%s\", expected \"v0\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 5 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "p0") == 0, "Name is \"%s\", expected \"p0\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 5 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "g0") == 0, "Name is \"%s\", expected \"g0\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 6 */
    p = t->lpVtbl->GetPassByIndex(t, 6);

    /* pass 6 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    p6_vs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "v") == 0, "Name is \"%s\", expected \"v\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 2, "Elements is %u, expected 2\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* Get input signature from vertex shader set from array element. */
    hr = p->lpVtbl->GetDesc(p, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!pass_desc.pIAInputSignature, "Expected input signature.\n");
    ok(pass_desc.IAInputSignatureSize == 88, "Unexpected input signature size.\n");

    /* pass 6 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    p6_ps = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "p") == 0, "Name is \"%s\", expected \"p\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 6 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    p6_gs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "g") == 0, "Name is \"%s\", expected \"g\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 7 */
    p = t->lpVtbl->GetPassByIndex(t, 7);

    /* pass 7 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == p6_vs, "Got %p, expected %p\n", pdesc.pShaderVariable, p6_vs);
    ok(pdesc.ShaderIndex == 1, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 1);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "v") == 0, "Name is \"%s\", expected \"v\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 2, "Elements is %u, expected 2\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 7 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == p6_ps, "Got %p, expected %p\n", pdesc.pShaderVariable, p6_ps);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "p") == 0, "Name is \"%s\", expected \"p\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 7 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pdesc.pShaderVariable == p6_gs, "Got %p, expected %p\n", pdesc.pShaderVariable, p6_gs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(strcmp(vdesc.Name, "g") == 0, "Name is \"%s\", expected \"g\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(strcmp(typedesc.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* Geometry shader with stream output */

    /* Inline variant */
    p = t->lpVtbl->GetPassByName(t, "P8");
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = pdesc.pShaderVariable->lpVtbl->GetShaderDesc(pdesc.pShaderVariable, 0, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shaderdesc.IsInline, "Unexpected inline flag.\n");
    ok(!strcmp(shaderdesc.SODecl, "SV_POSITION.y"), "Unexpected stream output declaration %s.\n",
                shaderdesc.SODecl);

    v = effect->lpVtbl->GetVariableByName(effect, "g_so");
    gs = v->lpVtbl->AsShader(v);
    hr = gs->lpVtbl->GetShaderDesc(gs, 0, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shaderdesc.IsInline, "Unexpected inline flag.\n");
    ok(!strcmp(shaderdesc.SODecl, "SV_POSITION.x; $Skip.x; SV_POSITION.gb"),
            "Unexpected stream output declaration %s.\n", shaderdesc.SODecl);

    /* Signature description */
    v = effect->lpVtbl->GetVariableByName(effect, "p");
    ps = v->lpVtbl->AsShader(v);

    hr = ps->lpVtbl->GetOutputSignatureElementDesc(ps, 0, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "SV_Target"), "Unexpected semantic %s.\n", sign.SemanticName);
    todo_wine
    ok(!sign.SystemValueType, "Unexpected system value type %u.\n", sign.SystemValueType);

    hr = ps->lpVtbl->GetOutputSignatureElementDesc(ps, 4, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "SV_POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(sign.SystemValueType == D3D10_NAME_POSITION, "Unexpected system value type %u.\n",
            sign.SystemValueType);

    hr = ps->lpVtbl->GetInputSignatureElementDesc(ps, 0, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "SV_POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(sign.SystemValueType == D3D10_NAME_POSITION, "Unexpected system value type %u.\n",
            sign.SystemValueType);

    hr = ps->lpVtbl->GetInputSignatureElementDesc(ps, 4, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(!sign.SystemValueType, "Unexpected system value type %u.\n", sign.SystemValueType);

    v = effect->lpVtbl->GetVariableByName(effect, "v");
    vs = v->lpVtbl->AsShader(v);

    hr = vs->lpVtbl->GetOutputSignatureElementDesc(vs, 0, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "SV_POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(sign.SystemValueType == D3D10_NAME_POSITION, "Unexpected system value type %u.\n",
            sign.SystemValueType);

    hr = vs->lpVtbl->GetOutputSignatureElementDesc(vs, 1, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "SV_POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(sign.SystemValueType == D3D10_NAME_POSITION, "Unexpected system value type %u.\n",
            sign.SystemValueType);

    hr = vs->lpVtbl->GetInputSignatureElementDesc(vs, 0, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(!sign.SystemValueType, "Unexpected system value type %u.\n", sign.SystemValueType);

    hr = vs->lpVtbl->GetInputSignatureElementDesc(vs, 1, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(!sign.SystemValueType, "Unexpected system value type %u.\n", sign.SystemValueType);

    /* NULL shader variable */
    v = effect->lpVtbl->GetVariableByName(effect, "v0");
    vs = v->lpVtbl->AsShader(v);

    hr = vs->lpVtbl->GetOutputSignatureElementDesc(vs, 0, 0, &sign);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = vs->lpVtbl->GetOutputSignatureElementDesc(vs, 1, 0, &sign);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = vs->lpVtbl->GetOutputSignatureElementDesc(vs, 2, 0, &sign);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = vs->lpVtbl->GetOutputSignatureElementDesc(vs, 3, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "SV_POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(sign.SystemValueType == D3D10_NAME_POSITION, "Unexpected system value type %u.\n",
            sign.SystemValueType);

    hr = vs->lpVtbl->GetInputSignatureElementDesc(vs, 0, 0, &sign);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = vs->lpVtbl->GetInputSignatureElementDesc(vs, 1, 0, &sign);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = vs->lpVtbl->GetInputSignatureElementDesc(vs, 2, 0, &sign);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = vs->lpVtbl->GetInputSignatureElementDesc(vs, 3, 0, &sign);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(sign.SemanticName, "POSITION"), "Unexpected semantic %s.\n", sign.SemanticName);
    ok(!sign.SystemValueType, "Unexpected system value type %u.\n", sign.SystemValueType);

    effect->lpVtbl->Release(effect);

    ID3D10PixelShader_Release(ps_d3d);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_get_variable_by
 */
#if 0
cbuffer cb
{
    float   f1 : SV_POSITION;
    float   f2 : COLOR0;
};
cbuffer cb2
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR1;
};
Texture1D tex1 : COLOR2;
Texture1D tex2 : COLOR1;
#endif
static DWORD fx_test_egvb[] = {
0x43425844, 0x63d60ede, 0xf75a09d1, 0x47da5604, 0x7ef6e331, 0x00000001, 0x000001ca, 0x00000001,
0x00000024, 0x30315846, 0x0000019e, 0xfeff1001, 0x00000002, 0x00000004, 0x00000002, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x0000008a, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x66006263,
0x74616f6c, 0x00000700, 0x00000100, 0x00000000, 0x00000400, 0x00001000, 0x00000400, 0x00090900,
0x00316600, 0x505f5653, 0x5449534f, 0x004e4f49, 0x43003266, 0x524f4c4f, 0x62630030, 0x33660032,
0x00346600, 0x4f4c4f43, 0x54003152, 0x75747865, 0x44316572, 0x00005300, 0x00000200, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000a00, 0x78657400, 0x4f430031, 0x32524f4c, 0x78657400,
0x00040032, 0x00100000, 0x00000000, 0x00020000, 0xffff0000, 0x0000ffff, 0x00290000, 0x000d0000,
0x002c0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00380000, 0x000d0000, 0x003b0000,
0x00040000, 0x00000000, 0x00000000, 0x00000000, 0x00420000, 0x00100000, 0x00000000, 0x00020000,
0xffff0000, 0x0000ffff, 0x00460000, 0x000d0000, 0x002c0000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00490000, 0x000d0000, 0x004c0000, 0x00040000, 0x00000000, 0x00000000, 0x00000000,
0x00790000, 0x005d0000, 0x007e0000, 0xffff0000, 0x0000ffff, 0x00850000, 0x005d0000, 0x004c0000,
0xffff0000, 0x0000ffff, 0x00000000,
};

static void test_effect_get_variable_by(void)
{
    ID3D10Effect *effect;
    ID3D10EffectVariable *variable_by_index, *variable, *null_variable;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_egvb, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 2, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 6, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    /* get the null variable */
    null_variable = effect->lpVtbl->GetVariableByIndex(effect, 0xffffffff);

    /* check for invalid arguments */
    variable = effect->lpVtbl->GetVariableByName(effect, NULL);
    ok(null_variable == variable, "GetVariableByName got %p, expected %p\n", variable, null_variable);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, NULL);
    ok(null_variable == variable, "GetVariableBySemantic got %p, expected %p\n", variable, null_variable);

    variable = effect->lpVtbl->GetVariableByName(effect, "invalid");
    ok(null_variable == variable, "GetVariableByName got %p, expected %p\n", variable, null_variable);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "invalid");
    ok(null_variable == variable, "GetVariableBySemantic got %p, expected %p\n", variable, null_variable);

    /* variable f1 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 0);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f1");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "SV_POSITION");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "sv_POSITION");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable f2 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 1);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f2");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR0");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "color0");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable f3 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 2);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f3");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "SV_POSITION");
    ok(variable != null_variable, "GetVariableBySemantic failed %p\n", variable);
    ok(variable != variable_by_index, "GetVariableBySemantic failed %p\n", variable);

    /* variable f4 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 3);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f4");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR1");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable tex1 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 4);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "tex1");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR2");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable tex2 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 5);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "tex2");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR1");
    ok(variable != null_variable, "GetVariableBySemantic failed %p\n", variable);
    ok(variable != variable_by_index, "GetVariableBySemantic failed %p\n", variable);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
RasterizerState rast_state
{
    FillMode = wireframe;                       /* 0x0c */
    CullMode = front;                           /* 0x0d */
    FrontCounterClockwise = true;               /* 0x0e */
    DepthBias = -4;                             /* 0x0f */
    DepthBiasClamp = 0.5f;                      /* 0x10 */
    SlopeScaledDepthBias = 0.25f;               /* 0x11 */
    DepthClipEnable = false;                    /* 0x12 */
    ScissorEnable = true;                       /* 0x13 */
    MultisampleEnable = true;                   /* 0x14 */
    AntialiasedLineEnable = true;               /* 0x15 */
};

RasterizerState rast_state2
{
    CullMode = back;
};

DepthStencilState ds_state
{
    DepthEnable = true;                         /* 0x16 */
    DepthWriteMask = zero;                      /* 0x17 */
    DepthFunc = equal;                          /* 0x18 */
    StencilEnable = true;                       /* 0x19 */
    StencilReadMask = 0x4;                      /* 0x1a */
    StencilWriteMask = 0x5;                     /* 0x1b */
    FrontFaceStencilFail = invert;              /* 0x1c */
    FrontFaceStencilDepthFail = incr;           /* 0x1d */
    FrontFaceStencilPass = decr;                /* 0x1e */
    FrontFaceStencilFunc = less_equal;          /* 0x1f */
    BackFaceStencilFail = replace;              /* 0x20 */
    BackFaceStencilDepthFail = incr_sat;        /* 0x21 */
    BackFaceStencilPass = decr_sat;             /* 0x22 */
    BackFaceStencilFunc = greater_equal;        /* 0x23 */
};

DepthStencilState ds_state2
{
    DepthEnable = false;
};

BlendState blend_state
{
    AlphaToCoverageEnable = false;              /* 0x24 */
    BlendEnable[0] = true;                      /* 0x25[0] */
    BlendEnable[7] = false;                     /* 0x25[7] */
    SrcBlend = one;                             /* 0x26 */
    DestBlend = src_color;                      /* 0x27 */
    BlendOp = min;                              /* 0x28 */
    SrcBlendAlpha = src_alpha;                  /* 0x29 */
    DestBlendAlpha = inv_src_alpha;             /* 0x2a */
    BlendOpAlpha = max;                         /* 0x2b */
    RenderTargetWriteMask[0] = 0x8;             /* 0x2c[0] */
    RenderTargetWriteMask[7] = 0x7;             /* 0x2c[7] */
};

BlendState blend_state2
{
    SrcBlend = src_color;
};

SamplerState sampler0
{
    Filter = min_mag_mip_linear;                /* 0x2d */
    AddressU = wrap;                            /* 0x2e */
    AddressV = mirror;                          /* 0x2f */
    AddressW = clamp;                           /* 0x30 */
    MipLODBias = -1;                            /* 0x31 */
    MaxAnisotropy = 4u;                         /* 0x32 */
    ComparisonFunc = always;                    /* 0x33 */
    BorderColor = float4(1.0, 2.0, 3.0, 4.0);   /* 0x34 */
    MinLOD = 6u;                                /* 0x35 */
    MaxLOD = 5u;                                /* 0x36 */
    Texture = NULL;                             /* 0x37 */
};

SamplerState sample0_1
{
    AddressU = mirror;
};

Texture t0;
Texture1D t1;
Texture1DArray t1a;
Texture2D t2;
Texture2DArray t2a;
Texture2DMS<float4, 4> t2dms;
Texture2DMSArray <float4, 4> t2dmsa;
Texture3D t3;
TextureCube tq;

SamplerState sampler1 { Texture = t0; };
SamplerState sampler2 { Texture = t1; };
SamplerState sampler2a { Texture = t1a; };
SamplerState sampler3 { Texture = t2; };
SamplerState sampler3a { Texture = t2a; };
SamplerState sampler4 { Texture = t2dms; };
SamplerState sampler4a { Texture = t2dmsa; };
SamplerState sampler5 { Texture = t3; };
SamplerState sampler6 { Texture = tq; };

Texture t0_a[3];
Texture1D t1_a[3];
Texture1DArray t1a_a[3];
Texture2D t2_a[3];
Texture2DArray t2a_a[3];
Texture2DMS<float4, 4> t2dms_a[3];
Texture2DMSArray <float4, 4> t2dmsa_a[3];
Texture3D t3_a[3];
TextureCube tq_a[3];

SamplerState sampler7 { Texture = t0_a[0]; };
SamplerState sampler8 { Texture = t1_a[1]; };
SamplerState sampler9 { Texture = t1a_a[2]; };
SamplerState sampler10 { Texture = t2_a[0]; };
SamplerState sampler11 { Texture = t2a_a[1]; };
SamplerState sampler12 { Texture = t2dms_a[2]; };
SamplerState sampler13 { Texture = t2dmsa_a[0]; };
SamplerState sampler14 { Texture = t3_a[1]; };
SamplerState sampler15 { Texture = tq_a[2]; };

SamplerState ss { Texture = NULL; };

float4 PS() : COLOR
{
    return tex2D( ss, float2(0.0f, 0.0f) );
}

technique10 tech0
{
    pass pass0
    {
        SetBlendState(blend_state, float4(0.5f, 0.6f, 0.7f, 0.8f), 0xffff);
        SetDepthStencilState(ds_state, 1.0f);
        SetRasterizerState(rast_state);
    }
    pass pass1
    {
        SetPixelShader( CompileShader(ps_4_0, PS()) );
    }
    pass pass2
    {
        SetDepthStencilState(NULL, 0);
        SetBlendState(NULL, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffff);
    }
};
#endif
static DWORD fx_test_state_groups[] =
{
    0x43425844, 0x0851fd73, 0x383acb2d, 0x862fcb27, 0x66b5dc75, 0x00000001, 0x00001381, 0x00000001,
    0x00000024, 0x30315846, 0x00001355, 0xfeff1001, 0x00000000, 0x00000000, 0x0000002d, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x000009d9, 0x00000000, 0x00000024, 0x00000002, 0x00000002,
    0x00000002, 0x00000015, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000000, 0x74736152,
    0x7a697265, 0x74537265, 0x00657461, 0x00000004, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000004, 0x74736172, 0x6174735f, 0x01006574, 0x02000000, 0x02000000, 0x01000000,
    0x02000000, 0x02000000, 0x01000000, 0x04000000, 0x01000000, 0x01000000, 0x02000000, 0xfc000000,
    0x01ffffff, 0x01000000, 0x00000000, 0x013f0000, 0x01000000, 0x00000000, 0x013e8000, 0x04000000,
    0x00000000, 0x01000000, 0x04000000, 0x01000000, 0x01000000, 0x04000000, 0x01000000, 0x01000000,
    0x04000000, 0x01000000, 0x72000000, 0x5f747361, 0x74617473, 0x01003265, 0x02000000, 0x03000000,
    0x44000000, 0x68747065, 0x6e657453, 0x536c6963, 0x65746174, 0x0000cb00, 0x00000200, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000300, 0x5f736400, 0x74617473, 0x00010065, 0x00040000,
    0x00010000, 0x00010000, 0x00020000, 0x00000000, 0x00010000, 0x00020000, 0x00030000, 0x00010000,
    0x00040000, 0x00010000, 0x00010000, 0x00030000, 0x00040000, 0x00010000, 0x00030000, 0x00050000,
    0x00010000, 0x00020000, 0x00060000, 0x00010000, 0x00020000, 0x00070000, 0x00010000, 0x00020000,
    0x00080000, 0x00010000, 0x00020000, 0x00040000, 0x00010000, 0x00020000, 0x00030000, 0x00010000,
    0x00020000, 0x00040000, 0x00010000, 0x00020000, 0x00050000, 0x00010000, 0x00020000, 0x00070000,
    0x73640000, 0x6174735f, 0x00326574, 0x00000001, 0x00000004, 0x00000000, 0x6e656c42, 0x61745364,
    0xc0006574, 0x02000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02000000, 0x62000000,
    0x646e656c, 0x6174735f, 0x01006574, 0x04000000, 0x00000000, 0x01000000, 0x04000000, 0x01000000,
    0x01000000, 0x04000000, 0x00000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000, 0x02000000,
    0x03000000, 0x01000000, 0x02000000, 0x04000000, 0x01000000, 0x02000000, 0x05000000, 0x01000000,
    0x02000000, 0x06000000, 0x01000000, 0x02000000, 0x05000000, 0x01000000, 0x03000000, 0x08000000,
    0x01000000, 0x03000000, 0x07000000, 0x62000000, 0x646e656c, 0x6174735f, 0x00326574, 0x00000001,
    0x00000002, 0x00000003, 0x706d6153, 0x5372656c, 0x65746174, 0x00029000, 0x00000200, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00001500, 0x6d617300, 0x72656c70, 0x00010030, 0x00020000,
    0x00150000, 0x00010000, 0x00020000, 0x00010000, 0x00010000, 0x00020000, 0x00020000, 0x00010000,
    0x00020000, 0x00030000, 0x00010000, 0x00020000, 0xffff0000, 0x0001ffff, 0x00030000, 0x00040000,
    0x00010000, 0x00020000, 0x00080000, 0x00040000, 0x00010000, 0x00000000, 0x00013f80, 0x00000000,
    0x00014000, 0x00000000, 0x00014040, 0x00000000, 0x00014080, 0x00030000, 0x00060000, 0x00010000,
    0x00030000, 0x00050000, 0x00010000, 0x00020000, 0x00000000, 0x61730000, 0x656c706d, 0x00315f30,
    0x00000001, 0x00000002, 0x00000002, 0x74786574, 0x00657275, 0x00000374, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000009, 0x54003074, 0x75747865, 0x44316572, 0x00039b00,
    0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000a00, 0x00317400, 0x74786554,
    0x31657275, 0x72724144, 0xc4007961, 0x02000003, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0b000000, 0x74000000, 0x54006131, 0x75747865, 0x44326572, 0x0003f300, 0x00000200, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000c00, 0x00327400, 0x74786554, 0x32657275, 0x72724144,
    0x1c007961, 0x02000004, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0d000000, 0x74000000,
    0x54006132, 0x75747865, 0x44326572, 0x4b00534d, 0x02000004, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0e000000, 0x74000000, 0x736d6432, 0x78655400, 0x65727574, 0x534d4432, 0x61727241,
    0x04790079, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000f0000, 0x32740000,
    0x61736d64, 0x78655400, 0x65727574, 0xad004433, 0x02000004, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x10000000, 0x74000000, 0x65540033, 0x72757478, 0x62754365, 0x04d60065, 0x00020000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00110000, 0x71740000, 0x6d617300, 0x72656c70,
    0x61730031, 0x656c706d, 0x73003272, 0x6c706d61, 0x61327265, 0x6d617300, 0x72656c70, 0x61730033,
    0x656c706d, 0x00613372, 0x706d6173, 0x3472656c, 0x6d617300, 0x72656c70, 0x73006134, 0x6c706d61,
    0x00357265, 0x706d6173, 0x3672656c, 0x00037400, 0x00000200, 0x00000300, 0x00000000, 0x00000000,
    0x00000000, 0x00000900, 0x5f307400, 0x039b0061, 0x00020000, 0x00030000, 0x00000000, 0x00000000,
    0x00000000, 0x000a0000, 0x31740000, 0xc400615f, 0x02000003, 0x03000000, 0x00000000, 0x00000000,
    0x00000000, 0x0b000000, 0x74000000, 0x615f6131, 0x0003f300, 0x00000200, 0x00000300, 0x00000000,
    0x00000000, 0x00000000, 0x00000c00, 0x5f327400, 0x041c0061, 0x00020000, 0x00030000, 0x00000000,
    0x00000000, 0x00000000, 0x000d0000, 0x32740000, 0x00615f61, 0x0000044b, 0x00000002, 0x00000003,
    0x00000000, 0x00000000, 0x00000000, 0x0000000e, 0x6d643274, 0x00615f73, 0x00000479, 0x00000002,
    0x00000003, 0x00000000, 0x00000000, 0x00000000, 0x0000000f, 0x6d643274, 0x615f6173, 0x0004ad00,
    0x00000200, 0x00000300, 0x00000000, 0x00000000, 0x00000000, 0x00001000, 0x5f337400, 0x04d60061,
    0x00020000, 0x00030000, 0x00000000, 0x00000000, 0x00000000, 0x00110000, 0x71740000, 0x7300615f,
    0x6c706d61, 0x00377265, 0x00000571, 0x00000000, 0x706d6173, 0x3872656c, 0x00059200, 0x00000100,
    0x6d617300, 0x72656c70, 0x05b30039, 0x00020000, 0x61730000, 0x656c706d, 0x00303172, 0x000005d5,
    0x00000000, 0x706d6173, 0x3172656c, 0x05f60031, 0x00010000, 0x61730000, 0x656c706d, 0x00323172,
    0x00000618, 0x00000002, 0x706d6173, 0x3172656c, 0x063c0033, 0x00000000, 0x61730000, 0x656c706d,
    0x00343172, 0x00000661, 0x00000001, 0x706d6173, 0x3172656c, 0x06820035, 0x00020000, 0x73730000,
    0x00000100, 0x00000200, 0x00000000, 0x63657400, 0x70003068, 0x30737361, 0x00000400, 0x00000100,
    0x00000000, 0x0000013f, 0x19999a00, 0x0000013f, 0x33333300, 0x0000013f, 0x4ccccd00, 0x0000013f,
    0x00000300, 0x00ffff00, 0x00000100, 0x00000100, 0x80000000, 0x7361703f, 0xf0003173, 0x44000001,
    0x03434258, 0x91426d35, 0x99a1723e, 0x0d495627, 0x016c6001, 0xf0000000, 0x05000001, 0x34000000,
    0xc4000000, 0xd4000000, 0x08000000, 0x74000001, 0x52000001, 0x88464544, 0x00000000, 0x00000000,
    0x02000000, 0x1c000000, 0x00000000, 0x00ffff04, 0x5f000011, 0x5c000000, 0x03000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x5c000000, 0x02000000, 0x05000000,
    0x04000000, 0xff000000, 0x00ffffff, 0x01000000, 0x0c000000, 0x73000000, 0x694d0073, 0x736f7263,
    0x2074666f, 0x20295228, 0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x2e303120,
    0x49ab0031, 0x084e4753, 0x00000000, 0x08000000, 0x4f000000, 0x2c4e4753, 0x01000000, 0x08000000,
    0x20000000, 0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x0f000000, 0x53000000, 0x61545f56,
    0x74656772, 0x53abab00, 0x64524448, 0x40000000, 0x19000000, 0x5a000000, 0x00030000, 0x00001060,
    0x58000000, 0x00040018, 0x00001070, 0x55000000, 0x65000055, 0xf2030000, 0x00001020, 0x45000000,
    0xf20c0000, 0x00001020, 0x02000000, 0x00000040, 0x00000000, 0x00000000, 0x00000000, 0x46000000,
    0x0000107e, 0x00000000, 0x00001060, 0x3e000000, 0x53010000, 0x74544154, 0x02000000, 0x00000000,
    0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x83000000, 0x00000007, 0x70000000, 0x32737361, 0x00000100,
    0x00000200, 0x00000000, 0x00000100, 0x00000200, 0x00000000, 0x00000400, 0x00000100, 0x00000000,
    0x00000100, 0x00000000, 0x00000100, 0x00000000, 0x00000100, 0x00000000, 0x00000100, 0x00000300,
    0x00ffff00, 0x00000100, 0x00000200, 0x00000000, 0x00003000, 0x00001400, 0x00000000, 0xffffff00,
    0x00000aff, 0x00000c00, 0x00000000, 0x00000100, 0x00003b00, 0x00000d00, 0x00000000, 0x00000100,
    0x00004700, 0x00000e00, 0x00000000, 0x00000100, 0x00005300, 0x00000f00, 0x00000000, 0x00000100,
    0x00005f00, 0x00001000, 0x00000000, 0x00000100, 0x00006b00, 0x00001100, 0x00000000, 0x00000100,
    0x00007700, 0x00001200, 0x00000000, 0x00000100, 0x00008300, 0x00001300, 0x00000000, 0x00000100,
    0x00008f00, 0x00001400, 0x00000000, 0x00000100, 0x00009b00, 0x00001500, 0x00000000, 0x00000100,
    0x0000a700, 0x00000000, 0x0000b300, 0x00001400, 0x00000000, 0xffffff00, 0x000001ff, 0x00000d00,
    0x00000000, 0x00000100, 0x0000bf00, 0x00000000, 0x0000f900, 0x0000dd00, 0x00000000, 0xffffff00,
    0x00000eff, 0x00001600, 0x00000000, 0x00000100, 0x00010200, 0x00001700, 0x00000000, 0x00000100,
    0x00010e00, 0x00001800, 0x00000000, 0x00000100, 0x00011a00, 0x00001900, 0x00000000, 0x00000100,
    0x00012600, 0x00001a00, 0x00000000, 0x00000100, 0x00013200, 0x00001b00, 0x00000000, 0x00000100,
    0x00013e00, 0x00001c00, 0x00000000, 0x00000100, 0x00014a00, 0x00001d00, 0x00000000, 0x00000100,
    0x00015600, 0x00001e00, 0x00000000, 0x00000100, 0x00016200, 0x00001f00, 0x00000000, 0x00000100,
    0x00016e00, 0x00002000, 0x00000000, 0x00000100, 0x00017a00, 0x00002100, 0x00000000, 0x00000100,
    0x00018600, 0x00002200, 0x00000000, 0x00000100, 0x00019200, 0x00002300, 0x00000000, 0x00000100,
    0x00019e00, 0x00000000, 0x0001aa00, 0x0000dd00, 0x00000000, 0xffffff00, 0x000001ff, 0x00001600,
    0x00000000, 0x00000100, 0x0001b400, 0x00000000, 0x0001e700, 0x0001cb00, 0x00000000, 0xffffff00,
    0x00000bff, 0x00002400, 0x00000000, 0x00000100, 0x0001f300, 0x00002500, 0x00000000, 0x00000100,
    0x0001ff00, 0x00002500, 0x00000700, 0x00000100, 0x00020b00, 0x00002600, 0x00000000, 0x00000100,
    0x00021700, 0x00002700, 0x00000000, 0x00000100, 0x00022300, 0x00002800, 0x00000000, 0x00000100,
    0x00022f00, 0x00002900, 0x00000000, 0x00000100, 0x00023b00, 0x00002a00, 0x00000000, 0x00000100,
    0x00024700, 0x00002b00, 0x00000000, 0x00000100, 0x00025300, 0x00002c00, 0x00000000, 0x00000100,
    0x00025f00, 0x00002c00, 0x00000700, 0x00000100, 0x00026b00, 0x00000000, 0x00027700, 0x0001cb00,
    0x00000000, 0xffffff00, 0x000001ff, 0x00002600, 0x00000000, 0x00000100, 0x00028400, 0x00000000,
    0x0002b900, 0x00029d00, 0x00000000, 0xffffff00, 0x00000bff, 0x00002d00, 0x00000000, 0x00000100,
    0x0002c200, 0x00002e00, 0x00000000, 0x00000100, 0x0002ce00, 0x00002f00, 0x00000000, 0x00000100,
    0x0002da00, 0x00003000, 0x00000000, 0x00000100, 0x0002e600, 0x00003100, 0x00000000, 0x00000100,
    0x0002f200, 0x00003200, 0x00000000, 0x00000100, 0x0002fe00, 0x00003300, 0x00000000, 0x00000100,
    0x00030a00, 0x00003400, 0x00000000, 0x00000100, 0x00031600, 0x00003500, 0x00000000, 0x00000100,
    0x00033a00, 0x00003600, 0x00000000, 0x00000100, 0x00034600, 0x00003700, 0x00000000, 0x00000100,
    0x00035200, 0x00000000, 0x00035e00, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00002e00,
    0x00000000, 0x00000100, 0x00036800, 0x00000000, 0x00039800, 0x00037c00, 0x00000000, 0xffffff00,
    0x000000ff, 0x0003c100, 0x0003a500, 0x00000000, 0xffffff00, 0x000000ff, 0x0003ef00, 0x0003d300,
    0x00000000, 0xffffff00, 0x000000ff, 0x00041900, 0x0003fd00, 0x00000000, 0xffffff00, 0x000000ff,
    0x00044700, 0x00042b00, 0x00000000, 0xffffff00, 0x000000ff, 0x00047300, 0x00045700, 0x00000000,
    0xffffff00, 0x000000ff, 0x0004a600, 0x00048a00, 0x00000000, 0xffffff00, 0x000000ff, 0x0004d300,
    0x0004b700, 0x00000000, 0xffffff00, 0x000000ff, 0x0004fe00, 0x0004e200, 0x00000000, 0xffffff00,
    0x000000ff, 0x00050100, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000,
    0x00000200, 0x00039800, 0x00000000, 0x00050a00, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff,
    0x00003700, 0x00000000, 0x00000200, 0x0003c100, 0x00000000, 0x00051300, 0x00029d00, 0x00000000,
    0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000200, 0x0003ef00, 0x00000000, 0x00051d00,
    0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000200, 0x00041900,
    0x00000000, 0x00052600, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000,
    0x00000200, 0x00044700, 0x00000000, 0x00053000, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff,
    0x00003700, 0x00000000, 0x00000200, 0x00047300, 0x00000000, 0x00053900, 0x00029d00, 0x00000000,
    0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000200, 0x0004a600, 0x00000000, 0x00054300,
    0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000200, 0x0004d300,
    0x00000000, 0x00054c00, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000,
    0x00000200, 0x0004fe00, 0x00000000, 0x00057100, 0x00055500, 0x00000000, 0xffffff00, 0x000000ff,
    0x00059200, 0x00057600, 0x00000000, 0xffffff00, 0x000000ff, 0x0005b300, 0x00059700, 0x00000000,
    0xffffff00, 0x000000ff, 0x0005d500, 0x0005b900, 0x00000000, 0xffffff00, 0x000000ff, 0x0005f600,
    0x0005da00, 0x00000000, 0xffffff00, 0x000000ff, 0x00061800, 0x0005fc00, 0x00000000, 0xffffff00,
    0x000000ff, 0x00063c00, 0x00062000, 0x00000000, 0xffffff00, 0x000000ff, 0x00066100, 0x00064500,
    0x00000000, 0xffffff00, 0x000000ff, 0x00068200, 0x00066600, 0x00000000, 0xffffff00, 0x000000ff,
    0x00068700, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000300,
    0x00069000, 0x00000000, 0x00069800, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700,
    0x00000000, 0x00000300, 0x0006a100, 0x00000000, 0x0006a900, 0x00029d00, 0x00000000, 0xffffff00,
    0x000001ff, 0x00003700, 0x00000000, 0x00000300, 0x0006b200, 0x00000000, 0x0006ba00, 0x00029d00,
    0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000300, 0x0006c400, 0x00000000,
    0x0006cc00, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000300,
    0x0006d600, 0x00000000, 0x0006de00, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700,
    0x00000000, 0x00000300, 0x0006e800, 0x00000000, 0x0006f000, 0x00029d00, 0x00000000, 0xffffff00,
    0x000001ff, 0x00003700, 0x00000000, 0x00000300, 0x0006fa00, 0x00000000, 0x00070200, 0x00029d00,
    0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000300, 0x00070c00, 0x00000000,
    0x00071400, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700, 0x00000000, 0x00000300,
    0x00071e00, 0x00000000, 0x00072600, 0x00029d00, 0x00000000, 0xffffff00, 0x000001ff, 0x00003700,
    0x00000000, 0x00000100, 0x00072900, 0x00000000, 0x00073500, 0x00000300, 0x00000000, 0x00073b00,
    0x00000600, 0x00000000, 0x00000a00, 0x00000000, 0x00000100, 0x00074100, 0x00000b00, 0x00000000,
    0x00000100, 0x00076500, 0x00000200, 0x00000000, 0x00000200, 0x0001e700, 0x00000900, 0x00000000,
    0x00000100, 0x00077100, 0x00000100, 0x00000000, 0x00000200, 0x0000f900, 0x00000000, 0x00000000,
    0x00000200, 0x00003000, 0x00077d00, 0x00000100, 0x00000000, 0x00000700, 0x00000000, 0x00000700,
    0x00097700, 0x00097f00, 0x00000500, 0x00000000, 0x00000900, 0x00000000, 0x00000100, 0x00098500,
    0x00000100, 0x00000000, 0x00000100, 0x00099100, 0x00000a00, 0x00000000, 0x00000100, 0x00099d00,
    0x00000b00, 0x00000000, 0x00000100, 0x0009c100, 0x00000200, 0x00000000, 0x00000100, 0x0009cd00,
    0x00000000,
};

static void create_effect_texture_resource(ID3D10Device *device, ID3D10ShaderResourceView **srv,
        ID3D10Texture2D **tex)
{
    D3D10_TEXTURE2D_DESC tex_desc;
    HRESULT hr;

    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.Width  = 8;
    tex_desc.Height = 8;
    tex_desc.ArraySize = 1;
    tex_desc.MipLevels = 0;
    tex_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    tex_desc.Usage = D3D10_USAGE_DEFAULT;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &tex_desc, NULL, tex);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)*tex, NULL, srv);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
}

static void test_effect_state_groups(void)
{
    ID3D10DepthStencilState *ds_state, *ds_state2;
    ID3D10BlendState *blend_state, *blend_state2;
    ID3D10ShaderResourceView *srv0, *srv1;
    ID3D10EffectDepthStencilVariable *d;
    ID3D10EffectRasterizerVariable *r;
    ID3D10RasterizerState *rast_state;
    ID3D10EffectTechnique *technique;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    D3D10_RASTERIZER_DESC rast_desc;
    D3D10_SAMPLER_DESC sampler_desc;
    ID3D10EffectSamplerVariable *s;
    D3D10_EFFECT_DESC effect_desc;
    UINT sample_mask, stencil_ref;
    ID3D10EffectBlendVariable *b;
    D3D10_BLEND_DESC blend_desc;
    D3D10_STATE_BLOCK_MASK mask;
    ID3D10SamplerState *sampler;
    D3D10_PASS_DESC pass_desc;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    float blend_factor[4];
    ID3D10Texture2D *tex0;
    ID3D10Effect *effect;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_state_groups, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(!effect_desc.ConstantBuffers, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 45, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 1, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    v = effect->lpVtbl->GetVariableByName(effect, "sampler0");
    s = v->lpVtbl->AsSampler(v);
    s->lpVtbl->GetBackingStore(s, 0, &sampler_desc);
    ok(sampler_desc.Filter == D3D10_FILTER_MIN_MAG_MIP_LINEAR, "Got unexpected Filter %#x.\n", sampler_desc.Filter);
    ok(sampler_desc.AddressU == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressU %#x.\n", sampler_desc.AddressU);
    ok(sampler_desc.AddressV == D3D10_TEXTURE_ADDRESS_MIRROR, "Got unexpected AddressV %#x.\n", sampler_desc.AddressV);
    ok(sampler_desc.AddressW == D3D10_TEXTURE_ADDRESS_CLAMP, "Got unexpected AddressW %#x.\n", sampler_desc.AddressW);
    ok(sampler_desc.MipLODBias == -1.0f, "Got unexpected MipLODBias %.8e.\n", sampler_desc.MipLODBias);
    ok(sampler_desc.MaxAnisotropy == 4, "Got unexpected MaxAnisotropy %#x.\n", sampler_desc.MaxAnisotropy);
    ok(sampler_desc.ComparisonFunc == D3D10_COMPARISON_ALWAYS, "Got unexpected ComparisonFunc %#x.\n",
            sampler_desc.ComparisonFunc);
    ok(sampler_desc.BorderColor[0] == 1.0f, "Got unexpected BorderColor[0] %.8e.\n", sampler_desc.BorderColor[0]);
    ok(sampler_desc.BorderColor[1] == 2.0f, "Got unexpected BorderColor[1] %.8e.\n", sampler_desc.BorderColor[1]);
    ok(sampler_desc.BorderColor[2] == 3.0f, "Got unexpected BorderColor[2] %.8e.\n", sampler_desc.BorderColor[2]);
    ok(sampler_desc.BorderColor[3] == 4.0f, "Got unexpected BorderColor[3] %.8e.\n", sampler_desc.BorderColor[3]);
    ok(sampler_desc.MinLOD == 6.0f, "Got unexpected MinLOD %.8e.\n", sampler_desc.MinLOD);
    ok(sampler_desc.MaxLOD == 5.0f, "Got unexpected MaxLOD %.8e.\n", sampler_desc.MaxLOD);
    hr = s->lpVtbl->GetSampler(s, 0, &sampler);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10SamplerState_GetDesc(sampler, &sampler_desc);
    ok(sampler_desc.Filter == D3D10_FILTER_MIN_MAG_MIP_LINEAR, "Got unexpected Filter %#x.\n", sampler_desc.Filter);
    ID3D10SamplerState_Release(sampler);

    s->lpVtbl->GetBackingStore(s, 1, &sampler_desc);
    ok(sampler_desc.AddressU == D3D10_TEXTURE_ADDRESS_MIRROR, "Got unexpected AddressU %#x.\n", sampler_desc.AddressU);
    hr = s->lpVtbl->GetSampler(s, 1, &sampler);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10SamplerState_GetDesc(sampler, &sampler_desc);
    ok(sampler_desc.AddressU == D3D10_TEXTURE_ADDRESS_MIRROR, "Got unexpected AddressU %#x.\n", sampler_desc.AddressU);
    ID3D10SamplerState_Release(sampler);

    v = effect->lpVtbl->GetVariableByName(effect, "blend_state");
    b = v->lpVtbl->AsBlend(v);
    b->lpVtbl->GetBackingStore(b, 0, &blend_desc);
    ok(!blend_desc.AlphaToCoverageEnable, "Got unexpected AlphaToCoverageEnable %#x.\n",
            blend_desc.AlphaToCoverageEnable);
    ok(blend_desc.BlendEnable[0], "Got unexpected BlendEnable[0] %#x.\n", blend_desc.BlendEnable[0]);
    ok(!blend_desc.BlendEnable[7], "Got unexpected BlendEnable[7] %#x.\n", blend_desc.BlendEnable[7]);
    ok(blend_desc.SrcBlend == D3D10_BLEND_ONE, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    ok(blend_desc.DestBlend == D3D10_BLEND_SRC_COLOR, "Got unexpected DestBlend %#x.\n", blend_desc.DestBlend);
    ok(blend_desc.BlendOp == D3D10_BLEND_OP_MIN, "Got unexpected BlendOp %#x.\n", blend_desc.BlendOp);
    ok(blend_desc.SrcBlendAlpha == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlendAlpha %#x.\n",
            blend_desc.SrcBlendAlpha);
    ok(blend_desc.DestBlendAlpha == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlendAlpha %#x.\n",
            blend_desc.DestBlendAlpha);
    ok(blend_desc.BlendOpAlpha == D3D10_BLEND_OP_MAX, "Got unexpected BlendOpAlpha %#x.\n", blend_desc.BlendOpAlpha);
    ok(blend_desc.RenderTargetWriteMask[0] == 0x8, "Got unexpected RenderTargetWriteMask[0] %#x.\n",
            blend_desc.RenderTargetWriteMask[0]);
    ok(blend_desc.RenderTargetWriteMask[7] == 0x7, "Got unexpected RenderTargetWriteMask[7] %#x.\n",
            blend_desc.RenderTargetWriteMask[7]);
    hr = b->lpVtbl->GetBlendState(b, 0, &blend_state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10BlendState_GetDesc(blend_state, &blend_desc);
    ok(blend_desc.SrcBlend == D3D10_BLEND_ONE, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    ID3D10BlendState_Release(blend_state);
    b->lpVtbl->GetBackingStore(b, 1, &blend_desc);
    ok(blend_desc.SrcBlend == D3D10_BLEND_SRC_COLOR, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    hr = b->lpVtbl->GetBlendState(b, 1, &blend_state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10BlendState_GetDesc(blend_state, &blend_desc);
    /* We can't check the SrcBlend value from the ID3D10BlendState object
     * descriptor because BlendEnable[0] is effectively false, which forces
     * normalization of all the other descriptor values. We can at least
     * confirm that we got blend_state2 by checking BlendEnable[0] itself. */
    ok(!blend_desc.BlendEnable[0], "Got unexpected BlendEnable[0] %#x.\n", blend_desc.BlendEnable[0]);
    ID3D10BlendState_Release(blend_state);

    v = effect->lpVtbl->GetVariableByName(effect, "ds_state");
    d = v->lpVtbl->AsDepthStencil(v);
    d->lpVtbl->GetBackingStore(d, 0, &ds_desc);
    ok(ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ZERO, "Got unexpected DepthWriteMask %#x.\n",
            ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D10_COMPARISON_EQUAL, "Got unexpected DepthFunc %#x.\n", ds_desc.DepthFunc);
    ok(ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == 0x4, "Got unexpected StencilReadMask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == 0x5, "Got unexpected StencilWriteMask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilFailOp == D3D10_STENCIL_OP_INVERT, "Got unexpected FrontFaceStencilFail %#x.\n",
            ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR,
            "Got unexpected FrontFaceStencilDepthFail %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D10_STENCIL_OP_DECR, "Got unexpected FrontFaceStencilPass %#x.\n",
            ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D10_COMPARISON_LESS_EQUAL, "Got unexpected FrontFaceStencilFunc %#x.\n",
            ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilFailOp == D3D10_STENCIL_OP_REPLACE, "Got unexpected BackFaceStencilFail %#x.\n",
            ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR_SAT,
            "Got unexpected BackFaceStencilDepthFail %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D10_STENCIL_OP_DECR_SAT, "Got unexpected BackFaceStencilPass %#x.\n",
            ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFunc == D3D10_COMPARISON_GREATER_EQUAL, "Got unexpected BackFaceStencilFunc %#x.\n",
            ds_desc.BackFace.StencilFunc);
    hr = d->lpVtbl->GetDepthStencilState(d, 0, &ds_state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10DepthStencilState_GetDesc(ds_state, &ds_desc);
    ok(ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ID3D10DepthStencilState_Release(ds_state);
    d->lpVtbl->GetBackingStore(d, 1, &ds_desc);
    ok(!ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    hr = d->lpVtbl->GetDepthStencilState(d, 1, &ds_state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10DepthStencilState_GetDesc(ds_state, &ds_desc);
    ok(!ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ID3D10DepthStencilState_Release(ds_state);

    v = effect->lpVtbl->GetVariableByName(effect, "rast_state");
    r = v->lpVtbl->AsRasterizer(v);
    r->lpVtbl->GetBackingStore(r, 0, &rast_desc);
    ok(rast_desc.FillMode == D3D10_FILL_WIREFRAME, "Got unexpected FillMode %#x.\n", rast_desc.FillMode);
    ok(rast_desc.CullMode == D3D10_CULL_FRONT, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ok(rast_desc.FrontCounterClockwise, "Got unexpected FrontCounterClockwise %#x.\n",
            rast_desc.FrontCounterClockwise);
    ok(rast_desc.DepthBias == -4, "Got unexpected DepthBias %#x.\n", rast_desc.DepthBias);
    ok(rast_desc.DepthBiasClamp == 0.5f, "Got unexpected DepthBiasClamp %.8e.\n", rast_desc.DepthBiasClamp);
    ok(rast_desc.SlopeScaledDepthBias == 0.25f, "Got unexpected SlopeScaledDepthBias %.8e.\n",
            rast_desc.SlopeScaledDepthBias);
    ok(!rast_desc.DepthClipEnable, "Got unexpected DepthClipEnable %#x.\n", rast_desc.DepthClipEnable);
    ok(rast_desc.ScissorEnable, "Got unexpected ScissorEnable %#x.\n", rast_desc.ScissorEnable);
    ok(rast_desc.MultisampleEnable, "Got unexpected MultisampleEnable %#x.\n", rast_desc.MultisampleEnable);
    ok(rast_desc.AntialiasedLineEnable, "Got unexpected AntialiasedLineEnable %#x.\n",
            rast_desc.AntialiasedLineEnable);
    hr = r->lpVtbl->GetRasterizerState(r, 0, &rast_state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10RasterizerState_GetDesc(rast_state, &rast_desc);
    ok(rast_desc.CullMode == D3D10_CULL_FRONT, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ID3D10RasterizerState_Release(rast_state);

    r->lpVtbl->GetBackingStore(r, 1, &rast_desc);
    ok(rast_desc.CullMode == D3D10_CULL_BACK, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    hr = r->lpVtbl->GetRasterizerState(r, 1, &rast_state);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10RasterizerState_GetDesc(rast_state, &rast_desc);
    ok(rast_desc.CullMode == D3D10_CULL_BACK, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ID3D10RasterizerState_Release(rast_state);

    technique = effect->lpVtbl->GetTechniqueByName(effect, "tech0");
    ok(technique->lpVtbl->IsValid(technique), "Expected valid technique.\n");
    pass = technique->lpVtbl->GetPassByName(technique, "pass0");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(pass_desc.Name, "pass0"), "Got unexpected Name \"%s\".\n", pass_desc.Name);
    ok(!pass_desc.Annotations, "Got unexpected Annotations %#x.\n", pass_desc.Annotations);
    ok(!pass_desc.pIAInputSignature, "Got unexpected pIAInputSignature %p.\n", pass_desc.pIAInputSignature);
    ok(pass_desc.StencilRef == 1, "Got unexpected StencilRef %#x.\n", pass_desc.StencilRef);
    ok(pass_desc.SampleMask == 0xffff, "Got unexpected SampleMask %#x.\n", pass_desc.SampleMask);
    ok(pass_desc.BlendFactor[0] == 0.5f, "Got unexpected BlendFactor[0] %.8e.\n", pass_desc.BlendFactor[0]);
    ok(pass_desc.BlendFactor[1] == 0.6f, "Got unexpected BlendFactor[1] %.8e.\n", pass_desc.BlendFactor[1]);
    ok(pass_desc.BlendFactor[2] == 0.7f, "Got unexpected BlendFactor[2] %.8e.\n", pass_desc.BlendFactor[2]);
    ok(pass_desc.BlendFactor[3] == 0.8f, "Got unexpected BlendFactor[3] %.8e.\n", pass_desc.BlendFactor[3]);

    hr = D3D10StateBlockMaskDisableAll(&mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = pass->lpVtbl->ComputeStateBlockMask(pass, &mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_RS_RASTERIZER_STATE, 0);
    ok(ret, "Unexpected mask.\n");
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_OM_DEPTH_STENCIL_STATE, 0);
    ok(ret, "Unexpected mask.\n");
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_OM_BLEND_STATE, 0);
    ok(ret, "Unexpected mask.\n");

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ID3D10BlendState_GetDesc(blend_state, &blend_desc);
    ok(!blend_desc.AlphaToCoverageEnable, "Got unexpected AlphaToCoverageEnable %#x.\n",
            blend_desc.AlphaToCoverageEnable);
    ok(blend_desc.BlendEnable[0], "Got unexpected BlendEnable[0] %#x.\n", blend_desc.BlendEnable[0]);
    ok(!blend_desc.BlendEnable[7], "Got unexpected BlendEnable[7] %#x.\n", blend_desc.BlendEnable[7]);
    ok(blend_desc.SrcBlend == D3D10_BLEND_ONE, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    ok(blend_desc.DestBlend == D3D10_BLEND_SRC_COLOR, "Got unexpected DestBlend %#x.\n", blend_desc.DestBlend);
    ok(blend_desc.BlendOp == D3D10_BLEND_OP_MIN, "Got unexpected BlendOp %#x.\n", blend_desc.BlendOp);
    ok(blend_desc.SrcBlendAlpha == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlendAlpha %#x.\n",
            blend_desc.SrcBlendAlpha);
    ok(blend_desc.DestBlendAlpha == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlendAlpha %#x.\n",
            blend_desc.DestBlendAlpha);
    ok(blend_desc.BlendOpAlpha == D3D10_BLEND_OP_MAX, "Got unexpected BlendOpAlpha %#x.\n", blend_desc.BlendOpAlpha);
    ok(blend_desc.RenderTargetWriteMask[0] == 0x8, "Got unexpected RenderTargetWriteMask[0] %#x.\n",
            blend_desc.RenderTargetWriteMask[0]);
    ok(blend_desc.RenderTargetWriteMask[7] == 0x7, "Got unexpected RenderTargetWriteMask[7] %#x.\n",
            blend_desc.RenderTargetWriteMask[7]);
    ok(blend_factor[0] == 0.5f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 0.6f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 0.7f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.8f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(sample_mask == 0xffff, "Got unexpected sample_mask %#x.\n", sample_mask);

    ID3D10Device_OMGetDepthStencilState(device, &ds_state, &stencil_ref);
    ID3D10DepthStencilState_GetDesc(ds_state, &ds_desc);
    ok(ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ZERO, "Got unexpected DepthWriteMask %#x.\n",
            ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D10_COMPARISON_EQUAL, "Got unexpected DepthFunc %#x.\n", ds_desc.DepthFunc);
    ok(ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == 0x4, "Got unexpected StencilReadMask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == 0x5, "Got unexpected StencilWriteMask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilFailOp == D3D10_STENCIL_OP_INVERT, "Got unexpected FrontFaceStencilFail %#x.\n",
            ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR,
            "Got unexpected FrontFaceStencilDepthFail %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D10_STENCIL_OP_DECR, "Got unexpected FrontFaceStencilPass %#x.\n",
            ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D10_COMPARISON_LESS_EQUAL, "Got unexpected FrontFaceStencilFunc %#x.\n",
            ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilFailOp == D3D10_STENCIL_OP_REPLACE, "Got unexpected BackFaceStencilFail %#x.\n",
            ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR_SAT,
            "Got unexpected BackFaceStencilDepthFail %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D10_STENCIL_OP_DECR_SAT, "Got unexpected BackFaceStencilPass %#x.\n",
            ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFunc == D3D10_COMPARISON_GREATER_EQUAL, "Got unexpected BackFaceStencilFunc %#x.\n",
            ds_desc.BackFace.StencilFunc);
    ok(stencil_ref == 1, "Got unexpected stencil_ref %#x.\n", stencil_ref);

    ID3D10Device_RSGetState(device, &rast_state);
    ID3D10RasterizerState_GetDesc(rast_state, &rast_desc);
    ok(rast_desc.FillMode == D3D10_FILL_WIREFRAME, "Got unexpected FillMode %#x.\n", rast_desc.FillMode);
    ok(rast_desc.CullMode == D3D10_CULL_FRONT, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ok(rast_desc.FrontCounterClockwise, "Got unexpected FrontCounterClockwise %#x.\n",
            rast_desc.FrontCounterClockwise);
    ok(rast_desc.DepthBias == -4, "Got unexpected DepthBias %#x.\n", rast_desc.DepthBias);
    ok(rast_desc.DepthBiasClamp == 0.5f, "Got unexpected DepthBiasClamp %.8e.\n", rast_desc.DepthBiasClamp);
    ok(rast_desc.SlopeScaledDepthBias == 0.25f, "Got unexpected SlopeScaledDepthBias %.8e.\n",
            rast_desc.SlopeScaledDepthBias);
    ok(!rast_desc.DepthClipEnable, "Got unexpected DepthClipEnable %#x.\n", rast_desc.DepthClipEnable);
    ok(rast_desc.ScissorEnable, "Got unexpected ScissorEnable %#x.\n", rast_desc.ScissorEnable);
    ok(rast_desc.MultisampleEnable, "Got unexpected MultisampleEnable %#x.\n", rast_desc.MultisampleEnable);
    ok(rast_desc.AntialiasedLineEnable, "Got unexpected AntialiasedLineEnable %#x.\n",
            rast_desc.AntialiasedLineEnable);

    ID3D10RasterizerState_Release(rast_state);

    /* pass 1 - uses SamplerState.Texture = NULL, resource slot is reset. */
    pass = technique->lpVtbl->GetPassByName(technique, "pass1");
    ok(pass->lpVtbl->IsValid(pass), "Failed to get pass.\n");

    create_effect_texture_resource(device, &srv0, &tex0);

    hr = D3D10StateBlockMaskDisableAll(&mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = pass->lpVtbl->ComputeStateBlockMask(pass, &mask);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_VS_SHADER_RESOURCES, 0);
    ok(!ret, "Unexpected mask.\n");

    ID3D10Device_PSSetShaderResources(device, 0, 1, &srv0);
    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_PSGetShaderResources(device, 0, 1, &srv1);
    ok(!srv1, "Unexpected resource pointer.\n");

    ID3D10ShaderResourceView_Release(srv0);
    ID3D10Texture2D_Release(tex0);

    /* pass 2 - NULL depth stencil state */
    pass = technique->lpVtbl->GetPassByName(technique, "pass2");
    ok(pass->lpVtbl->IsValid(pass), "Failed to get pass.\n");

    ID3D10Device_OMSetDepthStencilState(device, ds_state, 0);
    ID3D10Device_OMSetBlendState(device, blend_state, NULL, 0);
    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetDepthStencilState(device, &ds_state2, &stencil_ref);
    ok(!ds_state2, "Unexpected depth stencil state.\n");
    ID3D10Device_OMGetBlendState(device, &blend_state2, blend_factor, &sample_mask);
    ok(!blend_state2, "Unexpected blend state.\n");

    ID3D10DepthStencilState_Release(ds_state);
    ID3D10BlendState_Release(blend_state);
    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
RasterizerState rast_state {};
DepthStencilState ds_state {};
BlendState blend_state {};
SamplerState sampler0 {};

technique10 tech0
{
    pass pass0
        <string a="text";>
    {
    }
};
#endif
static DWORD fx_test_state_group_defaults[] =
{
    0x43425844, 0xef2bd174, 0x123794f9, 0xbea23fa0, 0x953a31dd, 0x00000001, 0x00000209, 0x00000001,
    0x00000024, 0x30315846, 0x000001dd, 0xfeff1001, 0x00000000, 0x00000000, 0x00000004, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x0000010d, 0x00000000, 0x00000000, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x74736152,
    0x7a697265, 0x74537265, 0x00657461, 0x00000004, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000004, 0x74736172, 0x6174735f, 0x44006574, 0x68747065, 0x6e657453, 0x536c6963,
    0x65746174, 0x00003b00, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000300,
    0x5f736400, 0x74617473, 0x6c420065, 0x53646e65, 0x65746174, 0x00007200, 0x00000200, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000200, 0x656c6200, 0x735f646e, 0x65746174, 0x6d615300,
    0x72656c70, 0x74617453, 0x00a50065, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00150000, 0x61730000, 0x656c706d, 0x74003072, 0x30686365, 0x73617000, 0x53003073, 0x6e697274,
    0x00e30067, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00610000,
    0x74786574, 0x00003000, 0x00001400, 0x00000000, 0xffffff00, 0x000000ff, 0x00000000, 0x00006900,
    0x00004d00, 0x00000000, 0xffffff00, 0x000000ff, 0x00000000, 0x00009900, 0x00007d00, 0x00000000,
    0xffffff00, 0x000000ff, 0x00000000, 0x0000ce00, 0x0000b200, 0x00000000, 0xffffff00, 0x000000ff,
    0x00000000, 0x0000d700, 0x00000100, 0x00000000, 0x0000dd00, 0x00000000, 0x00000100, 0x00010600,
    0x0000ea00, 0x00010800, 0x00000000,
};

static void test_effect_state_group_defaults(void)
{
    ID3D10EffectDepthStencilVariable *d;
    ID3D10EffectRasterizerVariable *r;
    ID3D10EffectTechnique *technique;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    D3D10_RASTERIZER_DESC rast_desc;
    D3D10_SAMPLER_DESC sampler_desc;
    ID3D10EffectSamplerVariable *s;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10EffectBlendVariable *b;
    D3D10_BLEND_DESC blend_desc;
    D3D10_PASS_DESC pass_desc;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    ID3D10Effect *effect;
    ID3D10Device *device;
    unsigned int idx;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_state_group_defaults, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 0, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 4, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 1, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    v = effect->lpVtbl->GetVariableByName(effect, "sampler0");
    s = v->lpVtbl->AsSampler(v);
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    s->lpVtbl->GetBackingStore(s, 0, &sampler_desc);
    ok(sampler_desc.Filter == D3D10_FILTER_MIN_MAG_MIP_POINT, "Got unexpected Filter %#x.\n", sampler_desc.Filter);
    ok(sampler_desc.AddressU == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressU %#x.\n", sampler_desc.AddressU);
    ok(sampler_desc.AddressV == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressV %#x.\n", sampler_desc.AddressV);
    ok(sampler_desc.AddressW == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressW %#x.\n", sampler_desc.AddressW);
    ok(sampler_desc.MipLODBias == 0.0f, "Got unexpected MipLODBias %.8e.\n", sampler_desc.MipLODBias);
    ok(sampler_desc.MaxAnisotropy == 16, "Got unexpected MaxAnisotropy %#x.\n", sampler_desc.MaxAnisotropy);
    ok(sampler_desc.ComparisonFunc == D3D10_COMPARISON_NEVER, "Got unexpected ComparisonFunc %#x.\n",
            sampler_desc.ComparisonFunc);
    for (idx = 0; idx < ARRAY_SIZE(sampler_desc.BorderColor); ++idx)
        ok(sampler_desc.BorderColor[idx] == 0.0f, "Got unexpected BorderColor[%u] %.8e.\n", idx, sampler_desc.BorderColor[idx]);
    ok(sampler_desc.MinLOD == 0.0f, "Got unexpected MinLOD %.8e.\n", sampler_desc.MinLOD);
    ok(sampler_desc.MaxLOD == FLT_MAX, "Got unexpected MaxLOD %.8e.\n", sampler_desc.MaxLOD);

    v = effect->lpVtbl->GetVariableByName(effect, "blend_state");
    b = v->lpVtbl->AsBlend(v);
    memset(&blend_desc, 0, sizeof(blend_desc));
    b->lpVtbl->GetBackingStore(b, 0, &blend_desc);
    ok(!blend_desc.AlphaToCoverageEnable, "Got unexpected AlphaToCoverageEnable %#x.\n",
            blend_desc.AlphaToCoverageEnable);
    ok(!blend_desc.BlendEnable[0], "Got unexpected BlendEnable[0] %#x.\n", blend_desc.BlendEnable[0]);
    ok(!blend_desc.BlendEnable[7], "Got unexpected BlendEnable[7] %#x.\n", blend_desc.BlendEnable[7]);
    ok(blend_desc.SrcBlend == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    ok(blend_desc.DestBlend == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlend %#x.\n", blend_desc.DestBlend);
    ok(blend_desc.BlendOp == D3D10_BLEND_OP_ADD, "Got unexpected BlendOp %#x.\n", blend_desc.BlendOp);
    ok(blend_desc.SrcBlendAlpha == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlendAlpha %#x.\n",
            blend_desc.SrcBlendAlpha);
    ok(blend_desc.DestBlendAlpha == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlendAlpha %#x.\n",
            blend_desc.DestBlendAlpha);
    ok(blend_desc.BlendOpAlpha == D3D10_BLEND_OP_ADD, "Got unexpected BlendOpAlpha %#x.\n", blend_desc.BlendOpAlpha);
    ok(blend_desc.RenderTargetWriteMask[0] == 0xf, "Got unexpected RenderTargetWriteMask[0] %#x.\n",
            blend_desc.RenderTargetWriteMask[0]);
    ok(blend_desc.RenderTargetWriteMask[7] == 0xf, "Got unexpected RenderTargetWriteMask[7] %#x.\n",
            blend_desc.RenderTargetWriteMask[7]);

    v = effect->lpVtbl->GetVariableByName(effect, "ds_state");
    d = v->lpVtbl->AsDepthStencil(v);
    d->lpVtbl->GetBackingStore(d, 0, &ds_desc);
    ok(ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ALL, "Got unexpected DepthWriteMask %#x.\n",
            ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D10_COMPARISON_LESS, "Got unexpected DepthFunc %#x.\n", ds_desc.DepthFunc);
    ok(!ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == 0xff, "Got unexpected StencilReadMask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == 0xff, "Got unexpected StencilWriteMask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilFailOp == D3D10_STENCIL_OP_KEEP, "Got unexpected FrontFaceStencilFail %#x.\n",
            ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected FrontFaceStencilDepthFail %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D10_STENCIL_OP_KEEP, "Got unexpected FrontFaceStencilPass %#x.\n",
            ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D10_COMPARISON_ALWAYS, "Got unexpected FrontFaceStencilFunc %#x.\n",
            ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilFailOp == D3D10_STENCIL_OP_KEEP, "Got unexpected BackFaceStencilFail %#x.\n",
            ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected BackFaceStencilDepthFail %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D10_STENCIL_OP_KEEP, "Got unexpected BackFaceStencilPass %#x.\n",
            ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFunc == D3D10_COMPARISON_ALWAYS, "Got unexpected BackFaceStencilFunc %#x.\n",
            ds_desc.BackFace.StencilFunc);

    v = effect->lpVtbl->GetVariableByName(effect, "rast_state");
    r = v->lpVtbl->AsRasterizer(v);
    r->lpVtbl->GetBackingStore(r, 0, &rast_desc);
    ok(rast_desc.FillMode == D3D10_FILL_SOLID, "Got unexpected FillMode %#x.\n", rast_desc.FillMode);
    ok(rast_desc.CullMode == D3D10_CULL_BACK, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ok(!rast_desc.FrontCounterClockwise, "Got unexpected FrontCounterClockwise %#x.\n",
            rast_desc.FrontCounterClockwise);
    ok(rast_desc.DepthBias == 0, "Got unexpected DepthBias %#x.\n", rast_desc.DepthBias);
    ok(rast_desc.DepthBiasClamp == 0.0f, "Got unexpected DepthBiasClamp %.8e.\n", rast_desc.DepthBiasClamp);
    ok(rast_desc.SlopeScaledDepthBias == 0.0f, "Got unexpected SlopeScaledDepthBias %.8e.\n",
            rast_desc.SlopeScaledDepthBias);
    ok(rast_desc.DepthClipEnable, "Got unexpected DepthClipEnable %#x.\n", rast_desc.DepthClipEnable);
    ok(!rast_desc.ScissorEnable, "Got unexpected ScissorEnable %#x.\n", rast_desc.ScissorEnable);
    ok(!rast_desc.MultisampleEnable, "Got unexpected MultisampleEnable %#x.\n", rast_desc.MultisampleEnable);
    ok(!rast_desc.AntialiasedLineEnable, "Got unexpected AntialiasedLineEnable %#x.\n",
            rast_desc.AntialiasedLineEnable);

    technique = effect->lpVtbl->GetTechniqueByName(effect, "tech0");
    ok(technique->lpVtbl->IsValid(technique), "Expected valid technique.\n");
    pass = technique->lpVtbl->GetPassByName(technique, "pass0");
    ok(pass->lpVtbl->IsValid(pass), "Failed to get pass.\n");
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(pass_desc.Name, "pass0"), "Got unexpected Name \"%s\".\n", pass_desc.Name);
    ok(pass_desc.Annotations == 1, "Got unexpected Annotations %u.\n", pass_desc.Annotations);
    ok(!pass_desc.pIAInputSignature, "Got unexpected pIAInputSignature %p.\n", pass_desc.pIAInputSignature);
    ok(pass_desc.StencilRef == 0, "Got unexpected StencilRef %#x.\n", pass_desc.StencilRef);
    ok(pass_desc.SampleMask == 0, "Got unexpected SampleMask %#x.\n", pass_desc.SampleMask);
    for (idx = 0; idx < ARRAY_SIZE(pass_desc.BlendFactor); ++idx)
        ok(pass_desc.BlendFactor[idx] == 0.0f, "Got unexpected BlendFactor[%u] %.8e.\n", idx, pass_desc.BlendFactor[idx]);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_scalar_variable
 */
#if 0
cbuffer cb
{
    float f0, f_a[2];
    int i0, i_a[2];
    bool b0, b_a[2];
    uint i1, i1_a[2];
};
#endif
static DWORD fx_test_scalar_variable[] =
{
    0x43425844, 0x7d97f44c, 0x1da4b110, 0xb710407e, 0x26750c1c, 0x00000001, 0x00000288, 0x00000001,
    0x00000024, 0x30315846, 0x0000025c, 0xfeff1001, 0x00000001, 0x00000008, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000118, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x66006263,
    0x74616f6c, 0x00000700, 0x00000100, 0x00000000, 0x00000400, 0x00001000, 0x00000400, 0x00090900,
    0x00306600, 0x00000007, 0x00000001, 0x00000002, 0x00000014, 0x00000010, 0x00000008, 0x00000909,
    0x00615f66, 0x00746e69, 0x0000004c, 0x00000001, 0x00000000, 0x00000004, 0x00000010, 0x00000004,
    0x00000911, 0x4c003069, 0x01000000, 0x02000000, 0x14000000, 0x10000000, 0x08000000, 0x11000000,
    0x69000009, 0x6200615f, 0x006c6f6f, 0x0000008f, 0x00000001, 0x00000000, 0x00000004, 0x00000010,
    0x00000004, 0x00000921, 0x8f003062, 0x01000000, 0x02000000, 0x14000000, 0x10000000, 0x08000000,
    0x21000000, 0x62000009, 0x7500615f, 0x00746e69, 0x000000d3, 0x00000001, 0x00000000, 0x00000004,
    0x00000010, 0x00000004, 0x00000919, 0xd3003169, 0x01000000, 0x02000000, 0x14000000, 0x10000000,
    0x08000000, 0x19000000, 0x69000009, 0x00615f31, 0x00000004, 0x00000090, 0x00000000, 0x00000008,
    0xffffffff, 0x00000000, 0x00000029, 0x0000000d, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000048, 0x0000002c, 0x00000000, 0x00000010, 0x00000000, 0x00000000, 0x00000000,
    0x0000006c, 0x00000050, 0x00000000, 0x00000024, 0x00000000, 0x00000000, 0x00000000, 0x0000008b,
    0x0000006f, 0x00000000, 0x00000030, 0x00000000, 0x00000000, 0x00000000, 0x000000b0, 0x00000094,
    0x00000000, 0x00000044, 0x00000000, 0x00000000, 0x00000000, 0x000000cf, 0x000000b3, 0x00000000,
    0x00000050, 0x00000000, 0x00000000, 0x00000000, 0x000000f4, 0x000000d8, 0x00000000, 0x00000064,
    0x00000000, 0x00000000, 0x00000000, 0x00000113, 0x000000f7, 0x00000000, 0x00000070, 0x00000000,
    0x00000000, 0x00000000,
};

static void test_scalar_methods(ID3D10EffectScalarVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name)
{
    float ret_f, expected_f;
    int ret_i, expected_i;
    BOOL ret_b, expected_b;
    HRESULT hr;

    hr = var->lpVtbl->SetFloat(var, 5.0f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_f = type == D3D10_SVT_BOOL ? -1.0f : 5.0f;
    ok(ret_f == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 5;
    ok(ret_i == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    ok(ret_b == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b);

    hr = var->lpVtbl->SetInt(var, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_f = type == D3D10_SVT_BOOL ? -1.0f : 2.0f;
    ok(ret_f == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 2;
    ok(ret_i == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    ok(ret_b == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b);

    hr = var->lpVtbl->SetBool(var, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    ok(ret_f == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    ok(ret_i == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
    ok(ret_b == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b);

    hr = var->lpVtbl->SetBool(var, 32);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    ok(ret_f == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    ok(ret_i == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_b = type == D3D10_SVT_BOOL ? 32 : -1;
    ok(ret_b == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b);
}

static void test_scalar_array_methods(ID3D10EffectScalarVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name)
{
    float set_f[2], ret_f[2], expected_f;
    int set_i[6], ret_i[6], expected_i, expected_i_a[6];
    BOOL set_b[2], ret_b[2], expected_b, expected_b_a[6];
    unsigned int i;
    HRESULT hr;

    set_f[0] = 10.0f; set_f[1] = 20.0f;
    hr = var->lpVtbl->SetFloatArray(var, set_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : set_f[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : (int)set_f[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_i[0] = 5; set_i[1] = 6;
    hr = var->lpVtbl->SetIntArray(var, set_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : (float)set_i[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : set_i[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_b[0] = 1; set_b[1] = 1;
    hr = var->lpVtbl->SetBoolArray(var, set_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    set_b[0] = 10; set_b[1] = 20;
    hr = var->lpVtbl->SetBoolArray(var, set_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    /* Array offset tests. Offset argument goes unused for scalar arrays. */
    set_i[0] = 0; set_i[1] = 0;
    hr = var->lpVtbl->SetIntArray(var, set_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    /* After this, if offset is in use, return should be { 0, 5 }. */
    set_i[0] = 5;
    hr = var->lpVtbl->SetIntArray(var, set_i, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_b_a[0] = -1; expected_b_a[1] = 0;
    expected_i_a[0] =  5; expected_i_a[1] = 0;
    for (i = 0; i < 2; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? expected_b_a[i] : expected_i_a[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    /* Test the offset on GetArray methods. If offset was in use, we'd get
     * back 5 given that the variable was previously set to { 0, 5 }. */
    hr = var->lpVtbl->GetIntArray(var, ret_i, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 5;
    ok(ret_i[0] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[0]);

    /* Try setting offset larger than number of elements. */
    set_i[0] = 0; set_i[1] = 0;
    hr = var->lpVtbl->SetIntArray(var, set_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    set_i[0] = 1;
    hr = var->lpVtbl->SetIntArray(var, set_i, 6, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    /* Since offset goes unused, a larger offset than the number of elements
     * in the array should have no effect. */
    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 1;
    ok(ret_i[0] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[0]);

    memset(ret_i, 0, sizeof(ret_i));
    hr = var->lpVtbl->GetIntArray(var, ret_i, 6, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 1;
    ok(ret_i[0] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[0]);

    if (0)
    {
        /* Windows array setting function has no bounds checking, so this test
         * ends up writing over into the adjacent variables in the local buffer. */
        set_i[0] = 1; set_i[1] = 2; set_i[2] = 3; set_i[3] = 4; set_i[4] = 5; set_i[5] = 6;
        hr = var->lpVtbl->SetIntArray(var, set_i, 0, 6);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

        memset(ret_i, 0, sizeof(ret_i));
        hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 6);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

        expected_i_a[0] = 1; expected_i_a[1] = 2; expected_i_a[2] = 0; expected_i_a[3] = 0;
        expected_i_a[4] = 0; expected_i_a[5] = 0;
        expected_b_a[0] = -1; expected_b_a[1] = -1; expected_b_a[2] = 0; expected_b_a[3] = 0;
        expected_b_a[4] = 0; expected_b_a[5] = 0;
        for (i = 0; i < 6; ++i)
        {
            expected_i = type == D3D10_SVT_BOOL ? expected_b_a[i] : expected_i_a[i];
            ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
        }
    }
}

static void test_effect_scalar_variable(void)
{
    static const struct
    {
        const char *name;
        D3D10_EFFECT_TYPE_DESC type;
    }
    tests[] =
    {
        { "f0",   { "float", D3D10_SVC_SCALAR, D3D10_SVT_FLOAT, 0, 0, 1, 1, 4,  4, 16 } },
        { "f_a",  { "float", D3D10_SVC_SCALAR, D3D10_SVT_FLOAT, 2, 0, 1, 1, 8, 20, 16 } },
        { "i0",   { "int",   D3D10_SVC_SCALAR, D3D10_SVT_INT,   0, 0, 1, 1, 4,  4, 16 } },
        { "i_a",  { "int",   D3D10_SVC_SCALAR, D3D10_SVT_INT,   2, 0, 1, 1, 8, 20, 16 } },
        { "b0",   { "bool",  D3D10_SVC_SCALAR, D3D10_SVT_BOOL,  0, 0, 1, 1, 4,  4, 16 } },
        { "b_a",  { "bool",  D3D10_SVC_SCALAR, D3D10_SVT_BOOL,  2, 0, 1, 1, 8, 20, 16 } },
        { "i1",   { "uint",  D3D10_SVC_SCALAR, D3D10_SVT_UINT,  0, 0, 1, 1, 4,  4, 16 } },
        { "i1_a", { "uint",  D3D10_SVC_SCALAR, D3D10_SVT_UINT,  2, 0, 1, 1, 8, 20, 16 } },
    };
    ID3D10EffectScalarVariable *s_v, *s_v2;
    ID3D10EffectVariable *var, *var2;
    D3D10_EFFECT_TYPE_DESC type_desc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10EffectType *type;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;
    float f;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = create_effect(fx_test_scalar_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 1, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 8, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    /* Check each different scalar type, make sure the variable returned is
     * valid, set it to a value, and make sure what we get back is the same
     * as what we set it to. */
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const D3D10_EFFECT_TYPE_DESC *t = &tests[i].type;

        winetest_push_context("Variable %s", tests[i].name);

        var = effect->lpVtbl->GetVariableByName(effect, tests[i].name);
        type = var->lpVtbl->GetType(var);
        hr = type->lpVtbl->GetDesc(type, &type_desc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(!strcmp(type_desc.TypeName, t->TypeName), "Unexpected type name %s.\n", type_desc.TypeName);
        ok(type_desc.Class == t->Class, "Unexpected type class %u.\n", type_desc.Class);
        ok(type_desc.Type == t->Type, "Unexpected type %u.\n", type_desc.Type);
        ok(type_desc.Elements == t->Elements, "Unexpected elements count %u.\n", type_desc.Elements);
        ok(type_desc.Members == t->Members, "Unexpected members count %u.\n", type_desc.Members);
        ok(type_desc.Rows == t->Rows, "Unexpected rows count %u.\n", type_desc.Rows);
        ok(type_desc.Columns == t->Columns, "Unexpected columns count %u.\n", type_desc.Columns);
        ok(type_desc.PackedSize == t->PackedSize, "Unexpected packed size %u.\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == t->UnpackedSize, "Unexpected unpacked size %u.\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == t->Stride, "Unexpected stride %u.\n", type_desc.Stride);

        s_v = var->lpVtbl->AsScalar(var);
        test_scalar_methods(s_v, t->Type, tests[i].name);
        if (t->Elements)
            test_scalar_array_methods(s_v, t->Type, tests[i].name);

        winetest_pop_context();
    }

    /* Verify that offsets are working correctly between array elements and adjacent data. */
    var = effect->lpVtbl->GetVariableByName(effect, "f0");
    s_v = var->lpVtbl->AsScalar(var);
    hr = s_v->lpVtbl->SetFloat(s_v, 1.0f);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    var2 = effect->lpVtbl->GetVariableByName(effect, "f_a");
    var2 = var2->lpVtbl->GetElement(var2, 0);
    s_v2 = var->lpVtbl->AsScalar(var2);
    hr = s_v2->lpVtbl->SetFloat(s_v2, 2.0f);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = s_v->lpVtbl->GetFloat(s_v, &f);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(f == 1.0f, "Unexpected value %f.\n", f);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_vector_variable
 */
#if 0
cbuffer cb
{
    float4 v_f0, v_f_a[2];
    int3 v_i0, v_i_a[3];
    bool2 v_b0, v_b_a[4];
};
#endif
static DWORD fx_test_vector_variable[] =
{
    0x43425844, 0x581ae0ae, 0xa906b020, 0x26bba03e,
    0x5d7dfba2, 0x00000001, 0x0000021a, 0x00000001,
    0x00000024, 0x30315846, 0x000001ee, 0xfeff1001,
    0x00000001, 0x00000006, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x000000e2,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x66006263,
    0x74616f6c, 0x00070034, 0x00010000, 0x00000000,
    0x00100000, 0x00100000, 0x00100000, 0x210a0000,
    0x5f760000, 0x07003066, 0x01000000, 0x02000000,
    0x20000000, 0x10000000, 0x20000000, 0x0a000000,
    0x76000021, 0x615f665f, 0x746e6900, 0x00510033,
    0x00010000, 0x00000000, 0x000c0000, 0x00100000,
    0x000c0000, 0x19120000, 0x5f760000, 0x51003069,
    0x01000000, 0x03000000, 0x2c000000, 0x10000000,
    0x24000000, 0x12000000, 0x76000019, 0x615f695f,
    0x6f6f6200, 0x9900326c, 0x01000000, 0x00000000,
    0x08000000, 0x10000000, 0x08000000, 0x22000000,
    0x76000011, 0x0030625f, 0x00000099, 0x00000001,
    0x00000004, 0x00000038, 0x00000010, 0x00000020,
    0x00001122, 0x5f625f76, 0x00040061, 0x00c00000,
    0x00000000, 0x00060000, 0xffff0000, 0x0000ffff,
    0x002a0000, 0x000e0000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x004b0000,
    0x002f0000, 0x00000000, 0x00100000, 0x00000000,
    0x00000000, 0x00000000, 0x00720000, 0x00560000,
    0x00000000, 0x00300000, 0x00000000, 0x00000000,
    0x00000000, 0x00930000, 0x00770000, 0x00000000,
    0x00400000, 0x00000000, 0x00000000, 0x00000000,
    0x00bb0000, 0x009f0000, 0x00000000, 0x00700000,
    0x00000000, 0x00000000, 0x00000000, 0x00dc0000,
    0x00c00000, 0x00000000, 0x00800000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};

static void test_vector_methods(ID3D10EffectVectorVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int components)
{
    float set_f[4], ret_f[4], expected_f, expected_f_v[4];
    int set_i[4], ret_i[4], expected_i, expected_i_v[4];
    BOOL set_b[4], ret_b[4], expected_b;
    unsigned int i;
    HRESULT hr;

    set_vec4(set_f, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = var->lpVtbl->SetFloatVector(var, set_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : set_f[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : (int)set_f[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_int4(set_i, 5, 6, 7, 8);
    hr = var->lpVtbl->SetIntVector(var, set_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : (float)set_i[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : set_i[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_b[0] = 1; set_b[1] = 0; set_b[2] = 1; set_b[3] = 0;
    hr = var->lpVtbl->SetBoolVector(var, set_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_f_v[0] = -1.0f; expected_f_v[1] = 0.0f; expected_f_v[2] = -1.0f; expected_f_v[3] = 0.0f;
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == expected_f_v[i], "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_i_v[0] = -1; expected_i_v[1] = 0; expected_i_v[2] = -1; expected_i_v[3] = 0;
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == expected_i_v[i], "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : expected_i_v[i];
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    set_b[0] = 5; set_b[1] = 10; set_b[2] = 15; set_b[3] = 20;
    hr = var->lpVtbl->SetBoolVector(var, set_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }
}

static void test_vector_array_methods(ID3D10EffectVectorVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int components, unsigned int elements)
{
    float set_f[9], ret_f[9], expected_f, expected_f_a[9];
    int set_i[9], ret_i[9], expected_i, expected_i_a[9];
    BOOL set_b[9], ret_b[9], expected_b;
    unsigned int i;
    HRESULT hr;

    set_f[0] = 1.0f; set_f[1] = 2.0f; set_f[2] = 3.0f; set_f[3] = 4.0f;
    set_f[4] = 5.0f; set_f[5] = 6.0f; set_f[6] = 7.0f; set_f[7] = 8.0f;
    set_f[8] = 9.0f;
    hr = var->lpVtbl->SetFloatVectorArray(var, set_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : set_f[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : (int)set_f[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_i[0] = 10; set_i[1] = 11; set_i[2] = 12; set_i[3] = 13;
    set_i[4] = 14; set_i[5] = 15; set_i[6] = 16; set_i[7] = 17;
    set_i[8] = 18;
    hr = var->lpVtbl->SetIntVectorArray(var, set_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : (float)set_i[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : set_i[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_b[0] = 1; set_b[1] = 0; set_b[2] = 1; set_b[3] = 1;
    set_b[4] = 1; set_b[5] = 0; set_b[6] = 0; set_b[7] = 1;
    set_b[8] = 1;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_f_a[0] = -1.0f; expected_f_a[1] = 0.0f; expected_f_a[2] = -1.0f; expected_f_a[3] = -1.0f;
    expected_f_a[4] = -1.0f; expected_f_a[5] = 0.0f; expected_f_a[6] =  0.0f; expected_f_a[7] = -1.0f;
    expected_f_a[8] = -1.0f;
    for (i = 0; i < components * elements; ++i)
        ok(ret_f[i] == expected_f_a[i], "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    expected_i_a[0] = -1; expected_i_a[1] = 0; expected_i_a[2] = -1; expected_i_a[3] = -1;
    expected_i_a[4] = -1; expected_i_a[5] = 0; expected_i_a[6] =  0; expected_i_a[7] = -1;
    expected_i_a[8] = -1;
    for (i = 0; i < components * elements; ++i)
        ok(ret_i[i] == expected_i_a[i], "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : expected_i_a[i];
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    set_b[0] = 5;  set_b[1] = 10; set_b[2] = 15; set_b[3] = 20;
    set_b[4] = 25; set_b[5] = 30; set_b[6] = 35; set_b[7] = 40;
    set_b[8] = 45;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    /* According to MSDN, the offset argument goes unused for VectorArray
     * methods, same as the ScalarArray methods. This test shows that's not
     * the case. */
    set_b[0] = 0; set_b[1] = 0; set_b[2] = 0; set_b[3] = 0;
    set_b[4] = 0; set_b[5] = 0; set_b[6] = 0; set_b[7] = 0;
    set_b[8] = 0;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    set_b[0] = 1; set_b[1] = 1; set_b[2] = 1; set_b[3] = 1;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    /* If the previous offset of 1 worked, then the first vector value of the
     * array should still be false. */
    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == 0, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == 0, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(!ret_b[i], "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    /* Test the GetFloatVectorArray offset argument. If it works, we should
     * get a vector with all values set to true. */
    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    if (0)
    {
        /* Windows array setting function has no bounds checking on offset values
         * either, so this ends up writing into adjacent variables. */
        hr = var->lpVtbl->SetBoolVectorArray(var, set_b, elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

        hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
        for (i = 0; i < components; ++i)
        {
            expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
            ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
        }
    }
}

static void test_effect_vector_variable(void)
{
    static const struct
    {
        const char *name;
        D3D10_EFFECT_TYPE_DESC type;
    }
    tests[] =
    {
        { "v_f0",  { "float4", D3D10_SVC_VECTOR, D3D10_SVT_FLOAT, 0, 0, 1, 4, 16, 16, 16 } },
        { "v_f_a", { "float4", D3D10_SVC_VECTOR, D3D10_SVT_FLOAT, 2, 0, 1, 4, 32, 32, 16 } },
        { "v_i0",  { "int3",   D3D10_SVC_VECTOR, D3D10_SVT_INT,   0, 0, 1, 3, 12, 12, 16 } },
        { "v_i_a", { "int3",   D3D10_SVC_VECTOR, D3D10_SVT_INT,   3, 0, 1, 3, 36, 44, 16 } },
        { "v_b0",  { "bool2",  D3D10_SVC_VECTOR, D3D10_SVT_BOOL,  0, 0, 1, 2,  8,  8, 16 } },
        { "v_b_a", { "bool2",  D3D10_SVC_VECTOR, D3D10_SVT_BOOL,  4, 0, 1, 2, 32, 56, 16 } },
    };
    ID3D10EffectVectorVariable *v_var;
    D3D10_EFFECT_TYPE_DESC type_desc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10EffectVariable *var;
    ID3D10EffectType *type;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = create_effect(fx_test_vector_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 1, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 6, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const D3D10_EFFECT_TYPE_DESC *t = &tests[i].type;

        winetest_push_context("Variable %s", tests[i].name);

        var = effect->lpVtbl->GetVariableByName(effect, tests[i].name);
        type = var->lpVtbl->GetType(var);
        hr = type->lpVtbl->GetDesc(type, &type_desc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(!strcmp(type_desc.TypeName, t->TypeName), "Unexpected type name %s.\n", type_desc.TypeName);
        ok(type_desc.Class == t->Class, "Unexpected type class %u.\n", type_desc.Class);
        ok(type_desc.Type == t->Type, "Unexpected type %u.\n", type_desc.Type);
        ok(type_desc.Elements == t->Elements, "Unexpected elements count %u.\n", type_desc.Elements);
        ok(type_desc.Members == t->Members, "Unexpected members count %u.\n", type_desc.Members);
        ok(type_desc.Rows == t->Rows, "Unexpected rows count %u.\n", type_desc.Rows);
        ok(type_desc.Columns == t->Columns, "Unexpected columns count %u.\n", type_desc.Columns);
        ok(type_desc.PackedSize == t->PackedSize, "Unexpected packed size %u.\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == t->UnpackedSize, "Unexpected unpacked size %u.\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == t->Stride, "Unexpected stride %u.\n", type_desc.Stride);

        v_var = var->lpVtbl->AsVector(var);
        test_vector_methods(v_var, t->Type, tests[i].name, t->Rows);
        if (t->Elements)
            test_vector_array_methods(v_var, t->Type, tests[i].name, t->Rows, t->Elements);

        winetest_pop_context();
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_matrix_variable
 */
#if 0
cbuffer cb
{
    float4x4 m_f0;
    float4x4 m_f_a[2];

    row_major int2x3 m_i0;

    bool3x2 m_b0;
    bool3x2 m_b_a[2];
};
#endif

static DWORD fx_test_matrix_variable[] =
{
    0x43425844, 0xc95a5c42, 0xa138d3cb, 0x8a4ef493,
    0x3515b7ee, 0x00000001, 0x000001e2, 0x00000001,
    0x00000024, 0x30315846, 0x000001b6, 0xfeff1001,
    0x00000001, 0x00000005, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x000000c6,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x66006263,
    0x74616f6c, 0x00347834, 0x00000007, 0x00000001,
    0x00000000, 0x00000040, 0x00000040, 0x00000040,
    0x0000640b, 0x30665f6d, 0x00000700, 0x00000100,
    0x00000200, 0x00008000, 0x00004000, 0x00008000,
    0x00640b00, 0x665f6d00, 0x6900615f, 0x7832746e,
    0x00530033, 0x00010000, 0x00000000, 0x001c0000,
    0x00200000, 0x00180000, 0x1a130000, 0x5f6d0000,
    0x62003069, 0x336c6f6f, 0x7b003278, 0x01000000,
    0x00000000, 0x1c000000, 0x20000000, 0x18000000,
    0x23000000, 0x6d000053, 0x0030625f, 0x0000007b,
    0x00000001, 0x00000002, 0x0000003c, 0x00000020,
    0x00000030, 0x00005323, 0x5f625f6d, 0x00040061,
    0x01400000, 0x00000000, 0x00050000, 0xffff0000,
    0x0000ffff, 0x002c0000, 0x00100000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x004d0000, 0x00310000, 0x00000000, 0x00400000,
    0x00000000, 0x00000000, 0x00000000, 0x00760000,
    0x005a0000, 0x00000000, 0x00c00000, 0x00000000,
    0x00000000, 0x00000000, 0x009f0000, 0x00830000,
    0x00000000, 0x00e00000, 0x00000000, 0x00000000,
    0x00000000, 0x00c00000, 0x00a40000, 0x00000000,
    0x01000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};

struct d3d10_matrix
{
    float m[4][4];
};

static void set_test_matrix(struct d3d10_matrix *m, D3D10_SHADER_VARIABLE_TYPE type,
        unsigned int row_count, unsigned int col_count, unsigned int elements)
{
    unsigned int row, col, elem;
    float tmp_f;
    int tmp_i;
    BOOL tmp_b;

    memset(m, 0, elements * sizeof(*m));
    switch (type)
    {
        case D3D10_SVT_FLOAT:
            tmp_f = 1.0f;
            for (elem = 0; elem < elements; ++elem)
            {
                for (row = 0; row < row_count; ++row)
                {
                    for (col = 0; col < col_count; ++col)
                    {
                        m[elem].m[row][col] = tmp_f;
                        ++tmp_f;
                    }
                }
            }
            break;

        case D3D10_SVT_INT:
            tmp_i = 1;
            for (elem = 0; elem < elements; ++elem)
            {
                for (row = 0; row < row_count; ++row)
                {
                    for (col = 0; col < col_count; ++col)
                    {
                        m[elem].m[row][col] = *(float *)&tmp_i;
                        ++tmp_i;
                    }
                }
            }
            break;

        case D3D10_SVT_BOOL:
            tmp_b = FALSE;
            for (elem = 0; elem < elements; ++elem)
            {
                tmp_b = !tmp_b;
                for (row = 0; row < row_count; ++row)
                {
                    for (col = 0; col < col_count; ++col)
                    {
                        m[elem].m[row][col] = *(float *)&tmp_b;
                        tmp_b = !tmp_b;
                    }
                }
            }
            break;

        default:
            break;
    }
}

static void transpose_matrix(struct d3d10_matrix *src, struct d3d10_matrix *dst,
        unsigned int row_count, unsigned int col_count)
{
    unsigned int row, col;

    for (row = 0; row < col_count; ++row)
    {
        for (col = 0; col < row_count; ++col)
            dst->m[row][col] = src->m[col][row];
    }
}

static void compare_matrix(const char *name, unsigned int line, struct d3d10_matrix *a,
        struct d3d10_matrix *b, unsigned int row_count, unsigned int col_count, BOOL transpose)
{
    unsigned int row, col;
    float tmp;

    for (row = 0; row < row_count; ++row)
    {
        for (col = 0; col < col_count; ++col)
        {
            tmp = !transpose ? b->m[row][col] : b->m[col][row];
            ok_(__FILE__, line)(a->m[row][col] == tmp,
                    "Variable %s (%u, %u), got unexpected value 0x%08x.\n", name, row, col,
                    *(unsigned int *)&tmp);
        }
    }
}

static void test_matrix_methods(ID3D10EffectMatrixVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int row_count, unsigned int col_count)
{
    struct d3d10_matrix m_set, m_ret, m_expected;
    HRESULT hr;

    set_test_matrix(&m_set, type, row_count, col_count, 1);

    hr = var->lpVtbl->SetMatrix(var, &m_set.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrix(var, &m_ret.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    compare_matrix(name, __LINE__, &m_set, &m_ret, row_count, col_count, FALSE);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixTranspose(var, &m_ret.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    compare_matrix(name, __LINE__, &m_set, &m_ret, row_count, col_count, TRUE);

    hr = var->lpVtbl->SetMatrixTranspose(var, &m_set.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrix(var, &m_ret.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    compare_matrix(name, __LINE__, &m_ret, &m_set, row_count, col_count, TRUE);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    memset(&m_expected.m[0][0], 0, sizeof(m_expected));
    hr = var->lpVtbl->GetMatrixTranspose(var, &m_ret.m[0][0]);
    transpose_matrix(&m_set, &m_expected, row_count, col_count);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    compare_matrix(name, __LINE__, &m_expected, &m_ret, row_count, col_count, TRUE);
}

static void test_matrix_array_methods(ID3D10EffectMatrixVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int row_count, unsigned int col_count, unsigned int elements)
{
    struct d3d10_matrix m_set[2], m_ret[2], m_expected;
    unsigned int i;
    HRESULT hr;

    set_test_matrix(&m_set[0], type, row_count, col_count, elements);

    hr = var->lpVtbl->SetMatrixArray(var, &m_set[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < elements; ++i)
        compare_matrix(name, __LINE__, &m_set[i], &m_ret[i], row_count, col_count, FALSE);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < elements; ++i)
        compare_matrix(name, __LINE__, &m_set[i], &m_ret[i], row_count, col_count, TRUE);

    hr = var->lpVtbl->SetMatrixTransposeArray(var, &m_set[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < elements; ++i)
        compare_matrix(name, __LINE__, &m_ret[i], &m_set[i], row_count, col_count, TRUE);

    memset(m_ret, 0, sizeof(m_ret));
    memset(&m_expected, 0, sizeof(m_expected));
    hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    for (i = 0; i < elements; ++i)
    {
        memset(&m_expected, 0, sizeof(m_expected));
        transpose_matrix(&m_set[i], &m_expected, row_count, col_count);
        compare_matrix(name, __LINE__, &m_expected, &m_ret[i], row_count, col_count, TRUE);
    }

    /* Offset tests. */
    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->SetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);

    hr = var->lpVtbl->SetMatrixArray(var, &m_set[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    compare_matrix(name, __LINE__, &m_ret[0], &m_set[0], row_count, col_count, FALSE);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->SetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);

    hr = var->lpVtbl->SetMatrixTransposeArray(var, &m_set[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

    memset(&m_expected, 0, sizeof(m_expected));
    hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
    transpose_matrix(&m_set[0], &m_expected, row_count, col_count);
    compare_matrix(name, __LINE__, &m_expected, &m_ret[0], row_count, col_count, TRUE);

    if (0)
    {
        /* Like vector array functions, matrix array functions will allow for
         * writing out of bounds into adjacent memory. */
        hr = var->lpVtbl->SetMatrixArray(var, &m_set[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

        memset(m_ret, 0, sizeof(m_ret));
        hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
        compare_matrix(name, __LINE__, &m_expected, &m_ret[0], row_count, col_count, TRUE);

        hr = var->lpVtbl->SetMatrixTransposeArray(var, &m_set[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);

        memset(&m_expected, 0, sizeof(m_expected));
        hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#lx.\n", name, hr);
        transpose_matrix(&m_set[0], &m_expected, row_count, col_count);
        compare_matrix(name, __LINE__, &m_expected, &m_ret[0], row_count, col_count, TRUE);
    }
}

static void test_effect_matrix_variable(void)
{
    static const struct
    {
        const char *name;
        D3D10_EFFECT_TYPE_DESC type;
    }
    tests[] =
    {
        { "m_f0",  { "float4x4", D3D10_SVC_MATRIX_COLUMNS, D3D10_SVT_FLOAT, 0, 0, 4, 4,  64,  64, 64 } },
        { "m_i0",  { "int2x3",   D3D10_SVC_MATRIX_ROWS,    D3D10_SVT_INT,   0, 0, 2, 3,  24,  28, 32 } },
        { "m_b0",  { "bool3x2",  D3D10_SVC_MATRIX_COLUMNS, D3D10_SVT_BOOL,  0, 0, 3, 2,  24,  28, 32 } },
        { "m_f_a", { "float4x4", D3D10_SVC_MATRIX_COLUMNS, D3D10_SVT_FLOAT, 2, 0, 4, 4, 128, 128, 64 } },
        { "m_b_a", { "bool3x2",  D3D10_SVC_MATRIX_COLUMNS, D3D10_SVT_BOOL,  2, 0, 3, 2,  48,  60, 32 } },
    };
    ID3D10EffectMatrixVariable *m_var;
    D3D10_EFFECT_TYPE_DESC type_desc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10EffectVariable *var;
    ID3D10EffectType *type;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_matrix_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 1, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 5, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 0, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const D3D10_EFFECT_TYPE_DESC *t = &tests[i].type;

        winetest_push_context("Variable %s", tests[i].name);

        var = effect->lpVtbl->GetVariableByName(effect, tests[i].name);
        type = var->lpVtbl->GetType(var);
        hr = type->lpVtbl->GetDesc(type, &type_desc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(!strcmp(type_desc.TypeName, t->TypeName), "Unexpected type name %s.\n", type_desc.TypeName);
        ok(type_desc.Class == t->Class, "Unexpected type class %u.\n", type_desc.Class);
        ok(type_desc.Type == t->Type, "Unexpected type %u.\n", type_desc.Type);
        ok(type_desc.Elements == t->Elements, "Unexpected elements count %u.\n", type_desc.Elements);
        ok(type_desc.Members == t->Members, "Unexpected members count %u.\n", type_desc.Members);
        ok(type_desc.Rows == t->Rows, "Unexpected rows count %u.\n", type_desc.Rows);
        ok(type_desc.Columns == t->Columns, "Unexpected columns count %u.\n", type_desc.Columns);
        ok(type_desc.PackedSize == t->PackedSize, "Unexpected packed size %u.\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == t->UnpackedSize, "Unexpected unpacked size %u.\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == t->Stride, "Unexpected stride %u.\n", type_desc.Stride);

        m_var = var->lpVtbl->AsMatrix(var);
        test_matrix_methods(m_var, t->Type, tests[i].name, t->Rows, t->Columns);
        if (t->Elements)
            test_matrix_array_methods(m_var, t->Type, tests[i].name, t->Rows, t->Columns, t->Elements);

        winetest_pop_context();
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

/*
 * test_effect_resource_variable
 */
#if 0
Texture2D t0;
Texture2D t_a[2];

float4 VS( float4 Pos : POSITION ) : SV_POSITION { return float4(1.0f, 1.0f, 1.0f, 1.0f); }

float4 PS( float4 Pos : SV_POSITION ) : SV_Target
{
    uint4 tmp;

    tmp = t0.Load(int3(0, 0, 0));
    tmp = t_a[0].Load(int4(0, 0, 0, 0));
    tmp = t_a[1].Load(int4(0, 0, 0, 0));
    return float4(1.0f, 1.0f, 0.0f, 1.0f) + tmp;
}

technique10 rsrc_test
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}
#endif
static DWORD fx_test_resource_variable[] =
{
    0x43425844, 0x767a8421, 0xdcbfffe6, 0x83df123d, 0x189ce72a, 0x00000001, 0x0000065a, 0x00000001,
    0x00000024, 0x30315846, 0x0000062e, 0xfeff1001, 0x00000000, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000582, 0x00000000, 0x00000003, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x74786554,
    0x32657275, 0x00040044, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000c0000,
    0x30740000, 0x00000400, 0x00000200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000c00,
    0x615f7400, 0x72737200, 0x65745f63, 0x70007473, 0x01b40030, 0x58440000, 0x338d4342, 0xc5a69f46,
    0x56883ae5, 0xa98fccc2, 0x00018ead, 0x01b40000, 0x00050000, 0x00340000, 0x008c0000, 0x00c00000,
    0x00f40000, 0x01380000, 0x44520000, 0x00504645, 0x00000000, 0x00000000, 0x00000000, 0x001c0000,
    0x04000000, 0x0100fffe, 0x001c0000, 0x694d0000, 0x736f7263, 0x2074666f, 0x20295228, 0x4c534c48,
    0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x322e3920, 0x35392e39, 0x31332e32, 0xab003131,
    0x5349abab, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00000000, 0x00030000,
    0x00000000, 0x000f0000, 0x4f500000, 0x49544953, 0xab004e4f, 0x534fabab, 0x002c4e47, 0x00010000,
    0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x000f0000, 0x56530000,
    0x534f505f, 0x4f495449, 0x4853004e, 0x003c5244, 0x00400000, 0x000f0001, 0x00670000, 0x20f20400,
    0x00000010, 0x00010000, 0x00360000, 0x20f20800, 0x00000010, 0x40020000, 0x00000000, 0x00003f80,
    0x00003f80, 0x00003f80, 0x003e3f80, 0x54530100, 0x00745441, 0x00020000, 0x00000000, 0x00000000,
    0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x005a0000, 0x00000000, 0x035c0000, 0x58440000, 0xe3754342, 0x3e477f40,
    0xed6f143f, 0xf16d26ce, 0x00010c3a, 0x035c0000, 0x00050000, 0x00340000, 0x00d00000, 0x01040000,
    0x01380000, 0x02e00000, 0x44520000, 0x00944645, 0x00000000, 0x00000000, 0x00020000, 0x001c0000,
    0x04000000, 0x0100ffff, 0x00630000, 0x005c0000, 0x00020000, 0x00050000, 0x00040000, 0xffff0000,
    0x0000ffff, 0x00010000, 0x000c0000, 0x005f0000, 0x00020000, 0x00050000, 0x00040000, 0xffff0000,
    0x0001ffff, 0x00020000, 0x000c0000, 0x30740000, 0x615f7400, 0x63694d00, 0x6f736f72, 0x28207466,
    0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x39322e39, 0x3235392e,
    0x3131332e, 0x53490031, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00010000,
    0x00030000, 0x00000000, 0x000f0000, 0x56530000, 0x534f505f, 0x4f495449, 0x534f004e, 0x002c4e47,
    0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00000000, 0x00030000, 0x00000000, 0x000f0000,
    0x56530000, 0x7261545f, 0x00746567, 0x4853abab, 0x01a05244, 0x00400000, 0x00680000, 0x18580000,
    0x70000400, 0x00000010, 0x55550000, 0x18580000, 0x70000400, 0x00010010, 0x55550000, 0x18580000,
    0x70000400, 0x00020010, 0x55550000, 0x00650000, 0x20f20300, 0x00000010, 0x00680000, 0x00020200,
    0x002d0000, 0x00f20a00, 0x00000010, 0x40020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x7e460000, 0x00000010, 0x001c0000, 0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x00560000,
    0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x002d0000, 0x00f20a00, 0x00010010, 0x40020000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7e460000, 0x00010010, 0x00000000, 0x00f20700,
    0x00000010, 0x0e460000, 0x00000010, 0x0e460000, 0x00010010, 0x001c0000, 0x00f20500, 0x00000010,
    0x0e460000, 0x00000010, 0x00560000, 0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x002d0000,
    0x00f20a00, 0x00010010, 0x40020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7e460000,
    0x00020010, 0x00000000, 0x00f20700, 0x00000010, 0x0e460000, 0x00000010, 0x0e460000, 0x00010010,
    0x001c0000, 0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x00560000, 0x00f20500, 0x00000010,
    0x0e460000, 0x00000010, 0x00000000, 0x20f20a00, 0x00000010, 0x0e460000, 0x00000010, 0x40020000,
    0x00000000, 0x00003f80, 0x00003f80, 0x00000000, 0x003e3f80, 0x54530100, 0x00745441, 0x000d0000,
    0x00020000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x00000000, 0x00010000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00060000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x021a0000, 0x00000000, 0x002a0000, 0x000e0000,
    0x00000000, 0xffff0000, 0x0000ffff, 0x00490000, 0x002d0000, 0x00000000, 0xffff0000, 0x0000ffff,
    0x004d0000, 0x00010000, 0x00000000, 0x00570000, 0x00020000, 0x00000000, 0x00060000, 0x00000000,
    0x00070000, 0x02120000, 0x00070000, 0x00000000, 0x00070000, 0x057a0000, 0x00000000,
};

#if 0
Texture2D t_a[2];

SamplerState s[2] : register(s1);

float4 PS( float4 pos : SV_POSITION ) : SV_Target
{
    return t_a[1].Sample(s[1], float2(0, 0));
}

PixelShader ps[1] = { CompileShader(ps_4_0, PS()) };

technique10 rsrc_test
{
    pass p0
    {
        SetPixelShader(ps[0]);
    }
}
#endif
static DWORD fx_test_resource_variable2[] =
{
    0x43425844, 0xecd43fcd, 0x0654927c, 0x71931f03, 0xf166cc09, 0x00000001, 0x0000039d, 0x00000001,
    0x00000024, 0x30315846, 0x00000371, 0xfeff1001, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x000002b5, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x74786554,
    0x32657275, 0x00040044, 0x00020000, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x000c0000,
    0x5f740000, 0x61530061, 0x656c706d, 0x61745372, 0x2e006574, 0x02000000, 0x02000000, 0x00000000,
    0x00000000, 0x00000000, 0x15000000, 0x73000000, 0x78695000, 0x68536c65, 0x72656461, 0x00005900,
    0x00000200, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000500, 0x00737000, 0x00000218,
    0x43425844, 0x9e83b242, 0xfc183c88, 0x2920b8b8, 0x877a749f, 0x00000001, 0x00000218, 0x00000005,
    0x00000034, 0x000000c8, 0x000000fc, 0x00000130, 0x0000019c, 0x46454452, 0x0000008c, 0x00000000,
    0x00000000, 0x00000002, 0x0000001c, 0xffff0400, 0x00000100, 0x00000062, 0x0000005c, 0x00000003,
    0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000001, 0x0000005e, 0x00000002,
    0x00000005, 0x00000004, 0xffffffff, 0x00000000, 0x00000002, 0x0000000c, 0x5f740073, 0x694d0061,
    0x736f7263, 0x2074666f, 0x20295228, 0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69,
    0x2e303120, 0xabab0031, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000,
    0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4e47534f,
    0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
    0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000064, 0x00000040, 0x00000019,
    0x0300005a, 0x00106000, 0x00000002, 0x04001858, 0x00107000, 0x00000001, 0x00005555, 0x03000065,
    0x001020f2, 0x00000000, 0x0c000045, 0x001020f2, 0x00000000, 0x00004002, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00107e46, 0x00000001, 0x00106000, 0x00000002, 0x0100003e, 0x54415453,
    0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x63727372, 0x7365745f,
    0x30700074, 0x00008100, 0x00000000, 0x00002a00, 0x00000e00, 0x00000000, 0xffffff00, 0x000000ff,
    0x00005700, 0x00003b00, 0x00000000, 0xffffff00, 0x000000ff, 0x00000000, 0x00000000, 0x00008100,
    0x00006500, 0x00000000, 0xffffff00, 0x000084ff, 0x00000000, 0x0002a000, 0x00000100, 0x00000000,
    0x0002aa00, 0x00000100, 0x00000000, 0x00000700, 0x00000000, 0x00000300, 0x0002ad00, 0x00000000,
};

#define get_effect_shader_resource_variable(a) get_effect_shader_resource_variable_(__LINE__, a)
static ID3D10EffectShaderResourceVariable *get_effect_shader_resource_variable_(unsigned int line,
        ID3D10EffectVariable *var)
{
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10EffectType *type;
    HRESULT hr;

    type = var->lpVtbl->GetType(var);
    ok_(__FILE__, line)(!!type, "Got unexpected type %p.\n", type);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(type_desc.Type == D3D10_SVT_TEXTURE2D, "Type is %x, expected %x.\n",
            type_desc.Type, D3D10_SVT_TEXTURE2D);
    return var->lpVtbl->AsShaderResource(var);
}

static void test_effect_resource_variable(void)
{
    ID3D10SamplerState *samplers[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D10ShaderResourceView *srv0, *srv_a[2], *srv_tmp[2];
    ID3D10EffectShaderResourceVariable *t0, *t_a, *t_a_0;
    ID3D10EffectTechnique *technique;
    ID3D10Texture2D *tex0, *tex_a[2];
    ID3D10EffectSamplerVariable *s;
    ID3D10SamplerState *sampler[2];
    D3D10_EFFECT_DESC effect_desc;
    D3D10_PASS_DESC pass_desc;
    ID3D10EffectVariable *var;
    ID3D10EffectPass *pass;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = create_effect(fx_test_resource_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->GetDesc(effect, &effect_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!effect_desc.IsChildEffect, "Unexpected IsChildEffect.\n");
    ok(effect_desc.ConstantBuffers == 0, "Unexpected constant buffers count %u.\n",
            effect_desc.ConstantBuffers);
    ok(effect_desc.SharedConstantBuffers == 0, "Unexpected shared constant buffers count %u.\n",
            effect_desc.SharedConstantBuffers);
    ok(effect_desc.GlobalVariables == 2, "Unexpected global variables count %u.\n",
            effect_desc.GlobalVariables);
    ok(effect_desc.SharedGlobalVariables == 0, "Unexpected shared global variables count %u.\n",
            effect_desc.SharedGlobalVariables);
    ok(effect_desc.Techniques == 1, "Unexpected techniques count %u.\n", effect_desc.Techniques);

    create_effect_texture_resource(device, &srv0, &tex0);

    /* SetResource() on null resource variable. */
    var = effect->lpVtbl->GetVariableByName(effect, "dummy name");
    t0 = var->lpVtbl->AsShaderResource(var);
    hr = t0->lpVtbl->SetResource(t0, srv0);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    var = effect->lpVtbl->GetVariableByName(effect, "t0");
    t0 = get_effect_shader_resource_variable(var);
    hr = t0->lpVtbl->SetResource(t0, srv0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    var = effect->lpVtbl->GetVariableByName(effect, "t_a");
    t_a = get_effect_shader_resource_variable(var);
    for (i = 0; i < 2; ++i)
        create_effect_texture_resource(device, &srv_a[i], &tex_a[i]);
    hr = t_a->lpVtbl->SetResourceArray(t_a, srv_a, 0, 2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Apply the pass to bind the resource to the shader. */
    technique = effect->lpVtbl->GetTechniqueByName(effect, "rsrc_test");
    ok(technique->lpVtbl->IsValid(technique), "Expected valid technique.\n");
    pass = technique->lpVtbl->GetPassByName(technique, "p0");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!pass_desc.pIAInputSignature, "Unexpected input signature.\n");
    ok(pass_desc.IAInputSignatureSize == 88, "Got unexpected input signature size %Iu.\n",
            pass_desc.IAInputSignatureSize);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_PSGetShaderResources(device, 0, 1, &srv_tmp[0]);
    ok(srv_tmp[0] == srv0, "Got unexpected shader resource view %p.\n", srv_tmp[0]);
    ID3D10ShaderResourceView_Release(srv_tmp[0]);

    ID3D10Device_PSGetShaderResources(device, 1, 2, srv_tmp);
    for (i = 0; i < 2; ++i)
    {
        ok(srv_tmp[i] == srv_a[i], "Got unexpected shader resource view %p.\n", srv_tmp[i]);
        ID3D10ShaderResourceView_Release(srv_tmp[i]);
    }

    /* Test individual array element variable SetResource. */
    var = t_a->lpVtbl->GetElement(t_a, 1);
    t_a_0 = get_effect_shader_resource_variable(var);
    hr = t_a_0->lpVtbl->SetResource(t_a_0, srv0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_PSGetShaderResources(device, 1, 2, srv_tmp);
    ok(srv_tmp[0] == srv_a[0], "Got unexpected shader resource view %p.\n", srv_tmp[0]);
    ok(srv_tmp[1] == srv0, "Got unexpected shader resource view %p.\n", srv_tmp[1]);
    for (i = 0; i < 2; ++i)
        ID3D10ShaderResourceView_Release(srv_tmp[i]);

    /* Test offset. */
    hr = t_a->lpVtbl->SetResourceArray(t_a, srv_a, 1, 1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_PSGetShaderResources(device, 1, 2, srv_tmp);
    ok(srv_tmp[0] == srv_a[0], "Got unexpected shader resource view %p.\n", srv_tmp[0]);
    ok(srv_tmp[1] == srv_a[0], "Got unexpected shader resource view %p.\n", srv_tmp[1]);
    for (i = 0; i < 2; ++i)
        ID3D10ShaderResourceView_Release(srv_tmp[i]);

    if (0)
    {
        /* This crashes on Windows. */
        hr = t_a->lpVtbl->SetResourceArray(t_a, srv_a, 2, 2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }

    ID3D10ShaderResourceView_Release(srv0);
    ID3D10Texture2D_Release(tex0);
    for (i = 0; i < 2; ++i)
    {
        ID3D10ShaderResourceView_Release(srv_a[i]);
        ID3D10Texture2D_Release(tex_a[i]);
    }

    effect->lpVtbl->Release(effect);

    hr = create_effect(fx_test_resource_variable2, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    technique = effect->lpVtbl->GetTechniqueByName(effect, "rsrc_test");
    ok(technique->lpVtbl->IsValid(technique), "Expected valid technique.\n");
    pass = technique->lpVtbl->GetPassByName(technique, "p0");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    var = effect->lpVtbl->GetVariableByName(effect, "s");
    ok(var->lpVtbl->IsValid(var), "Expected valid variable.\n");

    s = var->lpVtbl->AsSampler(var);
    ok(s->lpVtbl->IsValid(s), "Expected valid sample variable.\n");

    hr = s->lpVtbl->GetSampler(s, 0, &sampler[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = s->lpVtbl->GetSampler(s, 1, &sampler[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_PSGetSamplers(device, 0, ARRAY_SIZE(samplers), samplers);
    for (i = 0; i < ARRAY_SIZE(samplers); ++i)
    {
        if (i == 1 || i == 2)
        {
            ok(samplers[i] == sampler[i - 1], "Unexpected sampler at %u.\n", i);
        }
        else
            ok(!samplers[i], "Unexpected sampler at %u.\n", i);
        if (samplers[i])
            ID3D10SamplerState_Release(samplers[i]);
    }
    for (i = 0; i < ARRAY_SIZE(sampler); ++i)
        ID3D10SamplerState_Release(sampler[i]);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
cbuffer cb <string s = "cb_a"; >
{
    float f1 : COLOR0 <string s = "f1_a"; >;
};

BlendState blendstate <string s = "bs_a"; >;
Texture2D tex <string s = "tex_a"; >;
PixelShader ps <string s = "ps_a"; >;
VertexShader vs <string s = "vs_a"; >;
GeometryShader gs <string s = "gs_a"; >;
DepthStencilState ds <string s = "ds_a"; >;
RasterizerState rs <string s = "rs_a"; >;
SamplerState s <string s = "s_a"; >;
RenderTargetView rtv <string s = "rtv_a"; >;
DepthStencilView dsv <string s = "dsv_a"; >;

technique10 tech <string s = "tech_a"; >
{
    pass P0 <string s = "P0_a"; >
    {
    }
};
#endif
static DWORD fx_test_annotations[] =
{
    0x43425844, 0x2b3e08d4, 0xe3eda7bb, 0x36982de8, 0x0dcab1aa, 0x00000001, 0x000004c6, 0x00000001,
    0x00000024, 0x30315846, 0x0000049a, 0xfeff1001, 0x00000001, 0x00000001, 0x0000000a, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000276, 0x00000000, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000003, 0x00000000, 0x00000000, 0x53006263,
    0x6e697274, 0x00070067, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00010000,
    0x00730000, 0x615f6263, 0x6f6c6600, 0x31007461, 0x01000000, 0x00000000, 0x04000000, 0x10000000,
    0x04000000, 0x09000000, 0x66000009, 0x4f430031, 0x30524f4c, 0x5f316600, 0x6c420061, 0x53646e65,
    0x65746174, 0x00006200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000200,
    0x656c6200, 0x7473646e, 0x00657461, 0x615f7362, 0x78655400, 0x65727574, 0x99004432, 0x02000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0c000000, 0x74000000, 0x74007865, 0x615f7865,
    0x78695000, 0x68536c65, 0x72656461, 0x0000c900, 0x00000200, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000500, 0x00737000, 0x615f7370, 0x72655600, 0x53786574, 0x65646168, 0x00f90072,
    0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00060000, 0x73760000, 0x5f737600,
    0x65470061, 0x74656d6f, 0x68537972, 0x72656461, 0x00012a00, 0x00000200, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000700, 0x00736700, 0x615f7367, 0x70654400, 0x74536874, 0x69636e65,
    0x6174536c, 0x5d006574, 0x02000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000,
    0x64000000, 0x73640073, 0x5200615f, 0x65747361, 0x657a6972, 0x61745372, 0x93006574, 0x02000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x04000000, 0x72000000, 0x73720073, 0x5300615f,
    0x6c706d61, 0x74537265, 0x00657461, 0x000001c7, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000015, 0x00615f73, 0x646e6552, 0x61547265, 0x74656772, 0x77656956, 0x0001f400,
    0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00001300, 0x76747200, 0x76747200,
    0x4400615f, 0x68747065, 0x6e657453, 0x566c6963, 0x00776569, 0x0000022b, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000014, 0x00767364, 0x5f767364, 0x65740061, 0x74006863,
    0x5f686365, 0x30500061, 0x5f305000, 0x00040061, 0x00100000, 0x00000000, 0x00010000, 0xffff0000,
    0x0001ffff, 0x002a0000, 0x000e0000, 0x002c0000, 0x00530000, 0x00370000, 0x00560000, 0x00000000,
    0x00000000, 0x00000000, 0x00010000, 0x002a0000, 0x000e0000, 0x005d0000, 0x00890000, 0x006d0000,
    0x00000000, 0xffff0000, 0x0000ffff, 0x00010000, 0x002a0000, 0x000e0000, 0x00940000, 0x00bf0000,
    0x00a30000, 0x00000000, 0xffff0000, 0x0001ffff, 0x002a0000, 0x000e0000, 0x00c30000, 0x00f10000,
    0x00d50000, 0x00000000, 0xffff0000, 0x0000ffff, 0x00010000, 0x002a0000, 0x000e0000, 0x00f40000,
    0x01220000, 0x01060000, 0x00000000, 0xffff0000, 0x0000ffff, 0x00010000, 0x002a0000, 0x000e0000,
    0x01250000, 0x01550000, 0x01390000, 0x00000000, 0xffff0000, 0x0000ffff, 0x00010000, 0x002a0000,
    0x000e0000, 0x01580000, 0x018b0000, 0x016f0000, 0x00000000, 0xffff0000, 0x0000ffff, 0x00010000,
    0x002a0000, 0x000e0000, 0x018e0000, 0x01bf0000, 0x01a30000, 0x00000000, 0xffff0000, 0x0000ffff,
    0x00010000, 0x002a0000, 0x000e0000, 0x01c20000, 0x002a0000, 0x01d40000, 0x00000000, 0xffff0000,
    0x0000ffff, 0x00010000, 0x002a0000, 0x000e0000, 0x01f00000, 0x02210000, 0x02050000, 0x00000000,
    0xffff0000, 0x0001ffff, 0x002a0000, 0x000e0000, 0x02250000, 0x02580000, 0x023c0000, 0x00000000,
    0xffff0000, 0x0001ffff, 0x002a0000, 0x000e0000, 0x025c0000, 0x02620000, 0x00010000, 0x00010000,
    0x002a0000, 0x000e0000, 0x02670000, 0x026e0000, 0x00000000, 0x00010000, 0x002a0000, 0x000e0000,
    0x02710000, 0x00000000,
};

static void test_effect_annotations(void)
{
    D3D10_EFFECT_VARIABLE_DESC var_desc;
    ID3D10EffectConstantBuffer *cb;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10EffectTechnique *tech;
    ID3D10EffectVariable *v, *a;
    ID3D10EffectPass *pass;
    ID3D10Effect *effect;
    ID3D10Device *device;
    ULONG i, refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_annotations, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    cb = effect->lpVtbl->GetConstantBufferByName(effect, "cb");
    a = cb->lpVtbl->GetAnnotationByName(cb, "s");
    ok(a->lpVtbl->IsValid(a), "Expected valid variable.\n");
    a = cb->lpVtbl->GetAnnotationByName(cb, "S");
    ok(!a->lpVtbl->IsValid(a), "Unexpected valid variable.\n");

    effect->lpVtbl->GetDesc(effect, &effect_desc);
    for (i = 0; i < effect_desc.GlobalVariables; ++i)
    {
        v = effect->lpVtbl->GetVariableByIndex(effect, i);
        hr = v->lpVtbl->GetDesc(v, &var_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(var_desc.Annotations == 1, "Unexpected annotations count %u.\n", var_desc.Annotations);
        v = v->lpVtbl->GetAnnotationByName(v, "s");
        ok(v->lpVtbl->IsValid(v), "Expected valid variable.\n");
        a = cb->lpVtbl->GetAnnotationByName(cb, "S");
        ok(!a->lpVtbl->IsValid(a), "Unexpected valid variable.\n");
    }

    tech = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    ok(tech->lpVtbl->IsValid(tech), "Expected valid technique.\n");
    a = tech->lpVtbl->GetAnnotationByName(tech, "s");
    ok(a->lpVtbl->IsValid(a), "Expected valid variable.\n");
    a = tech->lpVtbl->GetAnnotationByName(tech, "S");
    ok(!a->lpVtbl->IsValid(a), "Unexpected valid variable.\n");

    pass = tech->lpVtbl->GetPassByIndex(tech, 0);
    ok(pass->lpVtbl->IsValid(pass), "Expected valid technique.\n");
    a = pass->lpVtbl->GetAnnotationByName(pass, "s");
    ok(a->lpVtbl->IsValid(a), "Expected valid variable.\n");
    a = pass->lpVtbl->GetAnnotationByName(pass, "S");
    ok(!a->lpVtbl->IsValid(a), "Unexpected valid variable.\n");

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_effect_optimize(void)
{
    D3D10_EFFECT_SHADER_DESC shaderdesc;
    D3D10_EFFECT_VARIABLE_DESC var_desc;
    ID3D10EffectConstantBuffer *cb;
    ID3D10EffectShaderVariable *gs;
    D3D10_TECHNIQUE_DESC tech_desc;
    D3D10_EFFECT_DESC effect_desc;
    ID3D10EffectTechnique *tech;
    D3D10_PASS_DESC pass_desc;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    ID3D10Effect *effect;
    ID3D10Device *device;
    ULONG i, refcount;
    HRESULT hr;
    BOOL ret;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_local_shader, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    tech = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    hr = tech->lpVtbl->GetDesc(tech, &tech_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(tech_desc.Name, "Render"), "Unexpected technique name %s.\n", tech_desc.Name);

    pass = tech->lpVtbl->GetPassByIndex(tech, 0);
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(pass_desc.Name, "P0"), "Unexpected pass name %s.\n", pass_desc.Name);

    v = effect->lpVtbl->GetVariableByName(effect, "g_so");

    gs = v->lpVtbl->AsShader(v);
    hr = gs->lpVtbl->GetShaderDesc(gs, 0, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!shaderdesc.pInputSignature, "Expected input signature.\n");
    ok(!shaderdesc.IsInline, "Unexpected inline flag.\n");
    ok(!!shaderdesc.pBytecode, "Expected bytecode.\n");
    ok(!!shaderdesc.BytecodeLength, "Unexpected bytecode length.\n");
    ok(!strcmp(shaderdesc.SODecl, "SV_POSITION.x; $Skip.x; SV_POSITION.gb"),
            "Unexpected stream output declaration %s.\n", shaderdesc.SODecl);
    ok(!!shaderdesc.NumInputSignatureEntries, "Unexpected input signature count.\n");
    ok(!!shaderdesc.NumOutputSignatureEntries, "Unexpected output signature count.\n");

    ret = effect->lpVtbl->IsOptimized(effect);
    ok(!ret, "Unexpected return value.\n");

    hr = effect->lpVtbl->Optimize(effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = effect->lpVtbl->IsOptimized(effect);
    ok(ret, "Unexpected return value.\n");

    hr = gs->lpVtbl->GetShaderDesc(gs, 0, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!shaderdesc.pInputSignature, "Expected input signature.\n");
    ok(!shaderdesc.IsInline, "Unexpected inline flag.\n");
    ok(!shaderdesc.pBytecode, "Unexpected bytecode.\n");
    ok(!shaderdesc.BytecodeLength, "Unexpected bytecode length.\n");
    ok(!shaderdesc.SODecl, "Unexpected stream output declaration %p.\n", shaderdesc.SODecl);
    ok(!shaderdesc.NumInputSignatureEntries, "Unexpected input signature count.\n");
    ok(!shaderdesc.NumOutputSignatureEntries, "Unexpected output signature count.\n");

    hr = tech->lpVtbl->GetDesc(tech, &tech_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!tech_desc.Name, "Unexpected technique name %p.\n", tech_desc.Name);

    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!pass_desc.Name, "Unexpected pass name %p.\n", pass_desc.Name);

    pass = tech->lpVtbl->GetPassByName(tech, "P0");
    ok(!pass->lpVtbl->IsValid(pass), "Unexpected valid pass.\n");

    tech = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    ok(tech->lpVtbl->IsValid(tech), "Unexpected valid technique.\n");

    tech = effect->lpVtbl->GetTechniqueByName(effect, "Render");
    ok(!tech->lpVtbl->IsValid(tech), "Unexpected valid technique.\n");

    /* Already optimized */
    hr = effect->lpVtbl->Optimize(effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    effect->lpVtbl->Release(effect);

    /* Annotations are stripped. */
    hr = create_effect(fx_test_annotations, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    cb = effect->lpVtbl->GetConstantBufferByName(effect, "cb");
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(var_desc.Annotations == 1, "Unexpected annotations count %u.\n", var_desc.Annotations);
    v = cb->lpVtbl->GetAnnotationByName(cb, "s");
    ok(v->lpVtbl->IsValid(v), "Expected valid variable.\n");

    effect->lpVtbl->GetDesc(effect, &effect_desc);

    for (i = 0; i < effect_desc.GlobalVariables; ++i)
    {
        v = effect->lpVtbl->GetVariableByIndex(effect, i);
        hr = v->lpVtbl->GetDesc(v, &var_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(var_desc.Annotations == 1, "Unexpected annotations count %u.\n", var_desc.Annotations);
        v = v->lpVtbl->GetAnnotationByName(v, "s");
        ok(v->lpVtbl->IsValid(v), "Expected valid variable.\n");
    }

    tech = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    hr = tech->lpVtbl->GetDesc(tech, &tech_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(tech_desc.Annotations == 1, "Unexpected annotations count %u.\n", tech_desc.Annotations);
    v = tech->lpVtbl->GetAnnotationByName(tech, "s");
    ok(v->lpVtbl->IsValid(v), "Expected valid variable.\n");

    pass = tech->lpVtbl->GetPassByIndex(tech, 0);
    ok(pass->lpVtbl->IsValid(pass), "Expected valid technique.\n");
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pass_desc.Annotations == 1, "Unexpected annotations count %u.\n", pass_desc.Annotations);
    v = pass->lpVtbl->GetAnnotationByName(pass, "s");
    ok(v->lpVtbl->IsValid(v), "Expected valid variable.\n");

    hr = effect->lpVtbl->Optimize(effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    cb = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
todo_wine {
    ok(!var_desc.Annotations, "Unexpected annotations count %u.\n", var_desc.Annotations);
    v = cb->lpVtbl->GetAnnotationByName(cb, "s");
    ok(!v->lpVtbl->IsValid(v), "Expected valid variable.\n");
}

    effect->lpVtbl->GetDesc(effect, &effect_desc);

    for (i = 0; i < effect_desc.GlobalVariables; ++i)
    {
        v = effect->lpVtbl->GetVariableByIndex(effect, i);
        hr = v->lpVtbl->GetDesc(v, &var_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine {
        ok(!var_desc.Annotations, "Unexpected annotations count %u.\n", var_desc.Annotations);
        v = v->lpVtbl->GetAnnotationByName(v, "s");
        ok(!v->lpVtbl->IsValid(v), "Expected valid variable.\n");
    }
    }

    tech = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    hr = tech->lpVtbl->GetDesc(tech, &tech_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
todo_wine {
    ok(!tech_desc.Annotations, "Unexpected annotations count %u.\n", tech_desc.Annotations);
    v = tech->lpVtbl->GetAnnotationByName(tech, "s");
    ok(!v->lpVtbl->IsValid(v), "Expected valid variable.\n");
}

    pass = tech->lpVtbl->GetPassByIndex(tech, 0);
    ok(pass->lpVtbl->IsValid(pass), "Expected valid technique.\n");
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
todo_wine {
    ok(!pass_desc.Annotations, "Unexpected annotations count %u.\n", pass_desc.Annotations);
    v = pass->lpVtbl->GetAnnotationByName(pass, "s");
    ok(!v->lpVtbl->IsValid(v), "Expected valid variable.\n");
}

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_effect_shader_description(void)
{
    D3D10_EFFECT_SHADER_DESC shaderdesc;
    ID3D10EffectShaderVariable *s;
    ID3D10EffectVariable *v;
    ID3D10Effect *effect;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_local_shader, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    v = effect->lpVtbl->GetVariableByName(effect, "v0");

    /* GetShaderDesc() is indexing through all shaders in the effect.*/
    s = v->lpVtbl->AsShader(v);
    hr = s->lpVtbl->GetShaderDesc(s, 0, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shaderdesc.BytecodeLength, "Unexpected bytecode length %u.\n", shaderdesc.BytecodeLength);
    hr = s->lpVtbl->GetShaderDesc(s, 1, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shaderdesc.BytecodeLength, "Unexpected bytecode length %u.\n", shaderdesc.BytecodeLength);
    hr = s->lpVtbl->GetShaderDesc(s, 2, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shaderdesc.BytecodeLength, "Unexpected bytecode length %u.\n", shaderdesc.BytecodeLength);
    hr = s->lpVtbl->GetShaderDesc(s, 3, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shaderdesc.BytecodeLength == 424, "Unexpected bytecode length %u.\n",
            shaderdesc.BytecodeLength);
    hr = s->lpVtbl->GetShaderDesc(s, 4, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shaderdesc.BytecodeLength == 424, "Unexpected bytecode length %u.\n",
            shaderdesc.BytecodeLength);
    hr = s->lpVtbl->GetShaderDesc(s, 5, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shaderdesc.BytecodeLength == 420, "Unexpected bytecode length %u.\n",
            shaderdesc.BytecodeLength);
    hr = s->lpVtbl->GetShaderDesc(s, 6, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shaderdesc.BytecodeLength == 516, "Unexpected bytecode length %u.\n",
            shaderdesc.BytecodeLength);
    ok(!shaderdesc.SODecl, "Unexpected SO declaration %p.\n", shaderdesc.SODecl);
    hr = s->lpVtbl->GetShaderDesc(s, 7, &shaderdesc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shaderdesc.BytecodeLength == 516, "Unexpected bytecode length %u.\n",
            shaderdesc.BytecodeLength);
    ok(!strcmp(shaderdesc.SODecl, "SV_POSITION.x; $Skip.x; SV_POSITION.gb"),
            "Unexpected SO declaration %s.\n", wine_dbgstr_a(shaderdesc.SODecl));

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_effect_shader_object(void)
{
    ID3D10EffectShaderVariable *s;
    ID3D10GeometryShader *gs;
    ID3D10EffectVariable *v;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Effect* effect;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_local_shader, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    v = effect->lpVtbl->GetVariableByName(effect, "v0");

    s = v->lpVtbl->AsShader(v);

    vs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetVertexShader(s, 0, &vs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!vs, "Unexpected shader object.\n");

    vs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetVertexShader(s, 1, &vs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!vs, "Unexpected shader object.\n");

    vs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetVertexShader(s, 2, &vs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!vs, "Unexpected shader object.\n");

    vs = NULL;
    hr = s->lpVtbl->GetVertexShader(s, 3, &vs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!vs, "Unexpected shader object.\n");
    ID3D10VertexShader_Release(vs);

    vs = NULL;
    hr = s->lpVtbl->GetVertexShader(s, 4, &vs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!vs, "Unexpected shader object.\n");
    ID3D10VertexShader_Release(vs);

    ps = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetPixelShader(s, 0, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!ps, "Unexpected shader object.\n");

    ps = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetPixelShader(s, 1, &ps);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!ps, "Unexpected shader object.\n");

    ps = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetPixelShader(s, 2, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!ps, "Unexpected shader object.\n");

    ps = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetPixelShader(s, 3, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!ps, "Unexpected shader object.\n");

    ps = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetPixelShader(s, 4, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!ps, "Unexpected shader object.\n");

    ps = NULL;
    hr = s->lpVtbl->GetPixelShader(s, 5, &ps);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!ps, "Unexpected shader object.\n");
    ID3D10PixelShader_Release(ps);

    gs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetGeometryShader(s, 0, &gs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!gs, "Unexpected shader object.\n");

    gs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetGeometryShader(s, 1, &gs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!gs, "Unexpected shader object.\n");

    gs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetGeometryShader(s, 2, &gs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!gs, "Unexpected shader object.\n");

    gs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetGeometryShader(s, 3, &gs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!gs, "Unexpected shader object.\n");

    gs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetGeometryShader(s, 4, &gs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!gs, "Unexpected shader object.\n");

    gs = (void *)0xdeadbeef;
    hr = s->lpVtbl->GetGeometryShader(s, 5, &gs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!gs, "Unexpected shader object.\n");

    gs = NULL;
    hr = s->lpVtbl->GetGeometryShader(s, 6, &gs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!gs, "Unexpected shader object.\n");
    ID3D10GeometryShader_Release(gs);

    gs = NULL;
    hr = s->lpVtbl->GetGeometryShader(s, 7, &gs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!gs, "Unexpected shader object.\n");
    ID3D10GeometryShader_Release(gs);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
cbuffer s_cb <String s = "value"; >
{
    float f1 : COLOR0;
    float f2 : COLOR1;
};

BlendState s_blendstate;
Texture2D s_texture;
PixelShader ps;

float4 VS( float4 pos : POSITION ) : SV_POSITION
{
    return f1.xxxx;
}

technique10 tech
{
    pass P0
    {
        SetPixelShader(ps);
        SetVertexShader(NULL);
        SetGeometryShader(NULL);
    }

    pass P1
    {
        SetVertexShader(CompileShader(vs_4_0, VS()));
    }
};
#endif
static DWORD fx_test_pool[] =
{
    0x43425844, 0x5a29c5ce, 0xa6970df1, 0x3b2ae8f2, 0x7b225509, 0x00000001, 0x000004dc, 0x00000001,
    0x00000024, 0x30315846, 0x000004b0, 0xfeff1001, 0x00000001, 0x00000002, 0x00000003, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000360, 0x00000000, 0x00000001, 0x00000000, 0x00000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x62635f73,
    0x72745300, 0x00676e69, 0x00000009, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000001, 0x61760073, 0x0065756c, 0x616f6c66, 0x00340074, 0x00010000, 0x00000000, 0x00040000,
    0x00100000, 0x00040000, 0x09090000, 0x31660000, 0x4c4f4300, 0x0030524f, 0x43003266, 0x524f4c4f,
    0x6c420031, 0x53646e65, 0x65746174, 0x00006a00, 0x00000200, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000200, 0x625f7300, 0x646e656c, 0x74617473, 0x65540065, 0x72757478, 0x00443265,
    0x0000009e, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000000c, 0x65745f73,
    0x72757478, 0x69500065, 0x536c6578, 0x65646168, 0x00ce0072, 0x00020000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00050000, 0x73700000, 0x63657400, 0x30500068, 0x00000100, 0x00000200,
    0x00000000, 0x00000100, 0x00000200, 0x00000000, 0x00315000, 0x00000238, 0x43425844, 0x37b3f12d,
    0x2579b942, 0x27ed5925, 0x4a80132c, 0x00000001, 0x00000238, 0x00000005, 0x00000034, 0x00000108,
    0x0000013c, 0x00000170, 0x000001bc, 0x46454452, 0x000000cc, 0x00000001, 0x00000044, 0x00000001,
    0x0000001c, 0xfffe0400, 0x00000100, 0x000000a3, 0x0000003c, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x62635f73, 0xababab00, 0x0000003c, 0x00000002,
    0x0000005c, 0x00000010, 0x00000000, 0x00000000, 0x0000008c, 0x00000000, 0x00000004, 0x00000002,
    0x00000090, 0x00000000, 0x000000a0, 0x00000004, 0x00000004, 0x00000000, 0x00000090, 0x00000000,
    0xab003166, 0x00030000, 0x00010001, 0x00000000, 0x00000000, 0x4d003266, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
    0x00000000, 0x0000000f, 0x49534f50, 0x4e4f4954, 0xababab00, 0x4e47534f, 0x0000002c, 0x00000001,
    0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653,
    0x5449534f, 0x004e4f49, 0x52444853, 0x00000044, 0x00010040, 0x00000011, 0x04000059, 0x00208e46,
    0x00000000, 0x00000001, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x06000036, 0x001020f2,
    0x00000000, 0x00208006, 0x00000000, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002,
    0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000011c, 0x00000000, 0x00000004, 0x00000010,
    0x00000000, 0x00000002, 0xffffffff, 0x00000001, 0x0000002c, 0x00000010, 0x0000002e, 0x00000056,
    0x0000003a, 0x00000059, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000060, 0x0000003a,
    0x00000063, 0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000091, 0x00000075, 0x00000000,
    0xffffffff, 0x00000000, 0x00000000, 0x000000c4, 0x000000a8, 0x00000000, 0xffffffff, 0x00000000,
    0x000000f6, 0x000000da, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x000000f9, 0x00000002,
    0x00000000, 0x000000fe, 0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x00000002, 0x000000f6,
    0x00000006, 0x00000000, 0x00000001, 0x00000101, 0x00000008, 0x00000000, 0x00000001, 0x0000010d,
    0x00000119, 0x00000001, 0x00000000, 0x00000006, 0x00000000, 0x00000007, 0x00000358,
};

/* Compiled as a child with /Gch (D3DCOMPILE_EFFECT_CHILD_EFFECT) */
#if 0
cbuffer l_cb
{
    float f0 : COLOR0;
};

shared cbuffer s_cb
{
    float f1 : COLOR0;
    float f2 : COLOR1;
};

cbuffer l_cb2
{
    float f3;
};

shared BlendState s_blendstate;
shared Texture2D s_texture;
shared PixelShader ps;

float4 VS( float4 pos : POSITION ) : SV_POSITION
{
    return f1.xxxx;
}

float4 PS( float4 pos : SV_POSITION ) : SV_Target
{
    return s_texture.Load(int3(0,0,0));
}

PixelShader vs = CompileShader(ps_4_0, PS());

technique10 tech_child
{
    pass P0
    {
        SetPixelShader(ps);
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
    }
};
#endif
static DWORD fx_test_pool_child[] =
{
    0x43425844, 0xfbf00688, 0x8af86d85, 0x759a5832, 0x320fdd75, 0x00000001, 0x000006e0, 0x00000001,
    0x00000024, 0x30315846, 0x000006b4, 0xfeff1001, 0x00000002, 0x00000002, 0x00000001, 0x00000001,
    0x00000002, 0x00000003, 0x00000001, 0x0000052c, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x62635f6c,
    0x6f6c6600, 0x09007461, 0x01000000, 0x00000000, 0x04000000, 0x10000000, 0x04000000, 0x09000000,
    0x66000009, 0x4f430030, 0x30524f4c, 0x635f6c00, 0x66003262, 0x69500033, 0x536c6578, 0x65646168,
    0x003e0072, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00050000, 0x73760000,
    0x0001e800, 0x42584400, 0x8d083943, 0x1caa8131, 0x88ca8b86, 0xbdb25507, 0x000001d8, 0x0001e800,
    0x00000500, 0x00003400, 0x0000ac00, 0x0000e000, 0x00011400, 0x00016c00, 0x45445200, 0x00007046,
    0x00000000, 0x00000000, 0x00000100, 0x00001c00, 0xff040000, 0x000100ff, 0x00004600, 0x00003c00,
    0x00000200, 0x00000500, 0x00000400, 0xffffff00, 0x000000ff, 0x00000100, 0x00000c00, 0x745f7300,
    0x75747865, 0x4d006572, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
    0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x475349ab, 0x00002c4e, 0x00000100, 0x00000800,
    0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x49534f50,
    0x4e4f4954, 0x47534f00, 0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000000,
    0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x67726154, 0xab007465, 0x444853ab, 0x00005052,
    0x00004000, 0x00001400, 0x00185800, 0x10700004, 0x00000000, 0x00555500, 0x00006500, 0x1020f203,
    0x00000000, 0x00002d00, 0x1020f20a, 0x00000000, 0x00400200, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x107e4600, 0x00000000, 0x00003e00, 0x41545301, 0x00007454, 0x00000200, 0x00000000,
    0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x635f7300, 0x31660062, 0x00326600, 0x4f4c4f43, 0x42003152,
    0x646e656c, 0x74617453, 0x02670065, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00020000, 0x5f730000, 0x6e656c62, 0x61747364, 0x54006574, 0x75747865, 0x44326572, 0x00029b00,
    0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000c00, 0x745f7300, 0x75747865,
    0x70006572, 0x65740073, 0x635f6863, 0x646c6968, 0x00305000, 0x00000238, 0x43425844, 0x37b3f12d,
    0x2579b942, 0x27ed5925, 0x4a80132c, 0x00000001, 0x00000238, 0x00000005, 0x00000034, 0x00000108,
    0x0000013c, 0x00000170, 0x000001bc, 0x46454452, 0x000000cc, 0x00000001, 0x00000044, 0x00000001,
    0x0000001c, 0xfffe0400, 0x00000100, 0x000000a3, 0x0000003c, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x62635f73, 0xababab00, 0x0000003c, 0x00000002,
    0x0000005c, 0x00000010, 0x00000000, 0x00000000, 0x0000008c, 0x00000000, 0x00000004, 0x00000002,
    0x00000090, 0x00000000, 0x000000a0, 0x00000004, 0x00000004, 0x00000000, 0x00000090, 0x00000000,
    0xab003166, 0x00030000, 0x00010001, 0x00000000, 0x00000000, 0x4d003266, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
    0x00000000, 0x0000000f, 0x49534f50, 0x4e4f4954, 0xababab00, 0x4e47534f, 0x0000002c, 0x00000001,
    0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653,
    0x5449534f, 0x004e4f49, 0x52444853, 0x00000044, 0x00010040, 0x00000011, 0x04000059, 0x00208e46,
    0x00000000, 0x00000001, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x06000036, 0x001020f2,
    0x00000000, 0x00208006, 0x00000000, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002,
    0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000002dc, 0x00000000, 0x00000001, 0x00000002,
    0x00000000, 0x00000004, 0x00000010, 0x00000000, 0x00000001, 0xffffffff, 0x00000000, 0x0000002b,
    0x0000000f, 0x0000002e, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000035, 0x00000010,
    0x00000000, 0x00000001, 0xffffffff, 0x00000000, 0x0000003b, 0x0000000f, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000066, 0x0000004a, 0x00000000, 0xffffffff, 0x00000069,
    0x00000000, 0x00000255, 0x00000010, 0x00000000, 0x00000002, 0xffffffff, 0x0000025a, 0x0000000f,
    0x0000002e, 0x00000000, 0x00000000, 0x00000000, 0x0000025d, 0x0000000f, 0x00000260, 0x00000004,
    0x00000000, 0x00000000, 0x0000028e, 0x00000272, 0x00000000, 0xffffffff, 0x000002c1, 0x000002a5,
    0x00000000, 0xffffffff, 0x000002cb, 0x0000004a, 0x00000000, 0xffffffff, 0x000002ce, 0x00000001,
    0x00000000, 0x000002d9, 0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x00000002, 0x000002cb,
    0x00000006, 0x00000000, 0x00000007, 0x00000518, 0x00000008, 0x00000000, 0x00000001, 0x00000520,
};

static void test_effect_pool(void)
{
    ID3D10Effect *effect, *effect2, *child_effect;
    ID3D10Device *device, *device2, *device3;
    D3D10_EFFECT_VARIABLE_DESC var_desc;
    D3D10_PASS_SHADER_DESC shader_desc;
    ID3D10EffectPool *pool, *pool2;
    ID3D10EffectConstantBuffer *cb;
    ID3D10EffectShaderVariable *sv;
    ID3D10EffectVariable *v, *a;
    ID3D10EffectTechnique *t;
    ID3D10EffectPass *pass;
    D3D10_EFFECT_DESC desc;
    ID3D10Buffer *buffer;
    ULONG refcount;
    IUnknown *unk;
    HRESULT hr;
    BOOL ret;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    device2 = create_device();
    ok(!!device2, "Failed to create d3d device.\n");

    hr = D3D10CreateEffectPoolFromMemory(NULL, 0, 0, device, &pool);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = create_effect_pool(fx_test_pool, NULL, &pool);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = create_effect_pool(fx_test_pool, device, &pool);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = get_refcount(pool);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);
    effect = pool->lpVtbl->AsEffect(pool);
    ok(!!effect, "Expected effect pointer.\n");

    effect->lpVtbl->AddRef(effect);
    refcount = get_refcount(pool);
    ok(refcount == 2, "Got unexpected refcount %lu.\n", refcount);
    effect->lpVtbl->Release(effect);

    hr = pool->lpVtbl->QueryInterface(pool, &IID_IUnknown, (void **)&unk);
    todo_wine
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) IUnknown_Release(unk);

    hr = pool->lpVtbl->QueryInterface(pool, &IID_ID3D10Effect, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

    hr = pool->lpVtbl->QueryInterface(pool, &IID_ID3D10EffectPool, (void **)&unk);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown *)pool, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    hr = effect->lpVtbl->QueryInterface(effect, &IID_IUnknown, (void **)&unk);
    todo_wine
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) IUnknown_Release(unk);

    hr = effect->lpVtbl->QueryInterface(effect, &IID_ID3D10Effect, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

    hr = effect->lpVtbl->QueryInterface(effect, &IID_ID3D10EffectPool, (void **)&pool2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pool2 == pool, "Unexpected pool pointer.\n");
    pool2->lpVtbl->Release(pool2);

    ret = effect->lpVtbl->IsPool(effect);
    ok(ret, "Expected pool.\n");

    hr = effect->lpVtbl->GetDesc(effect, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.IsChildEffect, "Unexpected child flag.\n");
    ok(desc.ConstantBuffers == 1, "Unexpected buffer count %u.\n", desc.ConstantBuffers);
    ok(!desc.SharedConstantBuffers, "Unexpected shared buffer count %u.\n",
            desc.SharedConstantBuffers);
    ok(desc.GlobalVariables == 5, "Unexpected variables count %u.\n", desc.GlobalVariables);
    ok(!desc.SharedGlobalVariables, "Unexpected shared variables count %u.\n",
            desc.SharedGlobalVariables);

    cb = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "s_cb"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);
    a = cb->lpVtbl->GetAnnotationByIndex(cb, 0);
    ok(a->lpVtbl->IsValid(a), "Expected valid annotation.\n");
    hr = a->lpVtbl->GetDesc(a, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "s"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == (D3D10_EFFECT_VARIABLE_POOLED | D3D10_EFFECT_VARIABLE_ANNOTATION),
            "Unexpected flags %#x.\n", var_desc.Flags);

    t = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    pass = t->lpVtbl->GetPassByName(t, "P1");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");
    hr = pass->lpVtbl->GetVertexShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = shader_desc.pShaderVariable->lpVtbl->GetDesc(shader_desc.pShaderVariable, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "$Anonymous"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    v = effect->lpVtbl->GetVariableByName(effect, "s_blendstate");
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "s_blendstate"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    v = effect->lpVtbl->GetVariableByName(effect, "s_texture");
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "s_texture"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    v = effect->lpVtbl->GetVariableByName(effect, "ps");
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "ps"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    t = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    pass = t->lpVtbl->GetPassByIndex(t, 0);
    hr = pass->lpVtbl->GetVertexShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    sv = shader_desc.pShaderVariable;
    hr = sv->lpVtbl->GetDesc(sv, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "$Anonymous"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    /* Create standalone effect from the same blob used for pool,  */
    hr = create_effect(fx_test_pool, D3D10_EFFECT_COMPILE_CHILD_EFFECT, device, NULL, &child_effect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = create_effect(fx_test_pool, 0, device, NULL, &effect2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = effect2->lpVtbl->QueryInterface(effect2, &IID_IUnknown, (void **)&unk);
    todo_wine
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) IUnknown_Release(unk);

    hr = effect2->lpVtbl->QueryInterface(effect2, &IID_ID3D10Effect, (void **)&unk);
    todo_wine
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) IUnknown_Release(unk);

    /* For regular effects querying for ID3D10EffectPool is broken */
    hr = effect2->lpVtbl->QueryInterface(effect2, &IID_ID3D10EffectPool, (void **)&unk);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(unk == (IUnknown *)effect2, "Got unexpected pointer %p.\n", unk);
    if (SUCCEEDED(hr)) IUnknown_Release(unk);

    ret = effect2->lpVtbl->IsPool(effect2);
    ok(!ret, "Unexpected pool.\n");

    hr = effect2->lpVtbl->GetDesc(effect2, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.IsChildEffect, "Unexpected child flag.\n");
    ok(desc.ConstantBuffers == 1, "Unexpected buffer count %u.\n", desc.ConstantBuffers);
    ok(!desc.SharedConstantBuffers, "Unexpected shared buffer count %u.\n",
            desc.SharedConstantBuffers);
    ok(desc.GlobalVariables == 5, "Unexpected variables count %u.\n", desc.GlobalVariables);
    ok(!desc.SharedGlobalVariables, "Unexpected shared variables count %u.\n",
            desc.SharedGlobalVariables);

    cb = effect2->lpVtbl->GetConstantBufferByIndex(effect2, 0);
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "s_cb"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    effect2->lpVtbl->Release(effect2);

    /* Pool and child that use different devices. */
    hr = create_effect_pool(fx_test_pool, device2, &pool2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = create_effect(fx_test_pool_child, D3D10_EFFECT_COMPILE_CHILD_EFFECT, device, pool2, &child_effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = child_effect->lpVtbl->GetDevice(child_effect, &device3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(device3 == device, "Unexpected device.\n");
    ID3D10Device_Release(device3);

    cb = child_effect->lpVtbl->GetConstantBufferByName(child_effect, "s_cb");
    ok(cb->lpVtbl->IsValid(cb), "Expected valid constant buffer.\n");

    hr = cb->lpVtbl->GetConstantBuffer(cb, &buffer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Buffer_GetDevice(buffer, &device3);
    ok(device3 == device2, "Unexpected device.\n");
    ID3D10Device_Release(device3);
    ID3D10Buffer_Release(buffer);

    child_effect->lpVtbl->Release(child_effect);
    pool2->lpVtbl->Release(pool2);

    /* When pool is specified, corresponding flag has to be set. */
    hr = create_effect(fx_test_pool_child, 0, device, pool, &child_effect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = create_effect(fx_test_pool_child, D3D10_EFFECT_COMPILE_CHILD_EFFECT, device, NULL, &child_effect);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    refcount = get_refcount(pool);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);

    hr = create_effect(fx_test_pool_child, D3D10_EFFECT_COMPILE_CHILD_EFFECT, device, pool, &child_effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = get_refcount(pool);
    ok(refcount == 2, "Got unexpected refcount %lu.\n", refcount);

    hr = child_effect->lpVtbl->GetDesc(child_effect, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(desc.IsChildEffect, "Unexpected child flag.\n");
    ok(desc.ConstantBuffers == 2, "Unexpected buffer count %u.\n", desc.ConstantBuffers);
    ok(desc.SharedConstantBuffers == 1, "Unexpected shared buffer count %u.\n",
            desc.SharedConstantBuffers);
    ok(desc.GlobalVariables == 3, "Unexpected variables count %u.\n", desc.GlobalVariables);
    ok(desc.SharedGlobalVariables == 5, "Unexpected shared variables count %u.\n",
            desc.SharedGlobalVariables);

    /* Local buffers are enumerated first, then shared ones. */
    cb = child_effect->lpVtbl->GetConstantBufferByIndex(child_effect, 0);
    ret = cb->lpVtbl->IsValid(cb);
    ok(ret, "Unexpected invalid variable.\n");

    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "l_cb"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    cb = child_effect->lpVtbl->GetConstantBufferByIndex(child_effect, 1);
    ret = cb->lpVtbl->IsValid(cb);
    ok(ret, "Unexpected invalid variable.\n");

    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "l_cb2"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    cb = child_effect->lpVtbl->GetConstantBufferByIndex(child_effect, 2);
    ret = cb->lpVtbl->IsValid(cb);
    ok(ret, "Unexpected invalid variable.\n");

    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "s_cb"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    /* Pool techniques are not accessible */
    t = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    ok(t->lpVtbl->IsValid(t), "Unexpected invalid technique.\n");

    t = effect->lpVtbl->GetTechniqueByIndex(effect, 1);
    ok(!t->lpVtbl->IsValid(t), "Unexpected valid technique.\n");

    /* Local variables first */
    v = child_effect->lpVtbl->GetVariableByIndex(child_effect, 0);
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "f0"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    v = child_effect->lpVtbl->GetVariableByIndex(child_effect, 1);
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "f3"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    v = child_effect->lpVtbl->GetVariableByIndex(child_effect, 2);
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "vs"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    v = child_effect->lpVtbl->GetVariableByIndex(child_effect, 3);
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "f1"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    v = child_effect->lpVtbl->GetVariableByIndex(child_effect, 4);
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "f2"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    v = child_effect->lpVtbl->GetVariableByName(child_effect, "s_texture");
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "s_texture"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    v = child_effect->lpVtbl->GetVariableBySemantic(child_effect, "COLOR0");
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "f0"), "Unexpected name %s.\n", var_desc.Name);
    ok(!var_desc.Flags, "Unexpected flags %#x.\n", var_desc.Flags);

    v = child_effect->lpVtbl->GetVariableBySemantic(child_effect, "COLOR1");
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "f2"), "Unexpected name %s.\n", var_desc.Name);
    ok(var_desc.Flags == D3D10_EFFECT_VARIABLE_POOLED, "Unexpected flags %#x.\n", var_desc.Flags);

    child_effect->lpVtbl->Release(child_effect);

    pool->lpVtbl->Release(pool);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    refcount = ID3D10Device_Release(device2);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
float4 f4 = {1,2,3,4};
float2 f2 = {1,2};
float1 f1 = {1};
int i = 10;
int i2[2] = {9,12};
float f = 0.2f;

struct test
{
    bool b1;
    float f2;
    bool b2;
};
test s1 = { true, -0.2f, false };

matrix <uint, 2, 3> m1 = { 1, 2, 3, 4, 5, 6 };
row_major matrix <uint, 2, 3> m2 = { 1, 2, 3, 4, 5, 6 };
row_major matrix <bool, 2, 3> m3 = { true, false, true, false, true, false };
float2x2 m4 = { 1.0f, 2.0f, 3.0f, 4.0f };
#endif
static DWORD fx_test_default_variable_value[] =
{
    0x43425844, 0x3673286d, 0xd7bb3432, 0xb571508a, 0x5d70cc01, 0x00000001, 0x00000448, 0x00000001,
    0x00000024, 0x30315846, 0x0000041c, 0xfeff1001, 0x00000001, 0x0000000b, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000284, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6f6c4724,
    0x736c6162, 0x6f6c6600, 0x00347461, 0x0000000d, 0x00000001, 0x00000000, 0x00000010, 0x00000010,
    0x00000010, 0x0000210a, 0x00003466, 0x003f8000, 0x00400000, 0x00404000, 0x66408000, 0x74616f6c,
    0x00430032, 0x00010000, 0x00000000, 0x00080000, 0x00100000, 0x00080000, 0x110a0000, 0x32660000,
    0x80000000, 0x0000003f, 0x6f6c6640, 0x00317461, 0x00000071, 0x00000001, 0x00000000, 0x00000004,
    0x00000010, 0x00000004, 0x0000090a, 0x00003166, 0x693f8000, 0x9b00746e, 0x01000000, 0x00000000,
    0x04000000, 0x10000000, 0x04000000, 0x11000000, 0x69000009, 0x00000a00, 0x00009b00, 0x00000100,
    0x00000200, 0x00001400, 0x00001000, 0x00000800, 0x00091100, 0x00326900, 0x00000009, 0x0000000c,
    0x616f6c66, 0x00e80074, 0x00010000, 0x00000000, 0x00040000, 0x00100000, 0x00040000, 0x09090000,
    0x00660000, 0x3e4ccccd, 0x74736574, 0x00316200, 0x6c6f6f62, 0x00011800, 0x00000100, 0x00000000,
    0x00000400, 0x00001000, 0x00000400, 0x00092100, 0x00326200, 0x00000110, 0x00000003, 0x00000000,
    0x0000000c, 0x00000010, 0x0000000c, 0x00000003, 0x00000115, 0x00000000, 0x00000000, 0x0000011d,
    0x00000066, 0x00000000, 0x00000004, 0x000000ee, 0x00000139, 0x00000000, 0x00000008, 0x0000011d,
    0x01003173, 0xcd000000, 0x00be4ccc, 0x75000000, 0x32746e69, 0x97003378, 0x01000001, 0x00000000,
    0x28000000, 0x30000000, 0x18000000, 0x1b000000, 0x6d00005a, 0x00010031, 0x00020000, 0x00030000,
    0x00040000, 0x00050000, 0x00060000, 0x01970000, 0x00010000, 0x00000000, 0x001c0000, 0x00200000,
    0x00180000, 0x1a1b0000, 0x326d0000, 0x00000100, 0x00000200, 0x00000300, 0x00000400, 0x00000500,
    0x00000600, 0x6f6f6200, 0x3378326c, 0x00020d00, 0x00000100, 0x00000000, 0x00001c00, 0x00002000,
    0x00001800, 0x001a2300, 0x00336d00, 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001,
    0x00000000, 0x616f6c66, 0x32783274, 0x00024c00, 0x00000100, 0x00000000, 0x00001800, 0x00002000,
    0x00001000, 0x00520b00, 0x00346d00, 0x3f800000, 0x40000000, 0x40400000, 0x40800000, 0x00000004,
    0x000000e0, 0x00000000, 0x0000000b, 0xffffffff, 0x00000000, 0x00000030, 0x00000014, 0x00000000,
    0x00000000, 0x00000033, 0x00000000, 0x00000000, 0x00000066, 0x0000004a, 0x00000000, 0x00000010,
    0x00000069, 0x00000000, 0x00000000, 0x00000094, 0x00000078, 0x00000000, 0x00000018, 0x00000097,
    0x00000000, 0x00000000, 0x000000bb, 0x0000009f, 0x00000000, 0x0000001c, 0x000000bd, 0x00000000,
    0x00000000, 0x000000dd, 0x000000c1, 0x00000000, 0x00000020, 0x000000e0, 0x00000000, 0x00000000,
    0x0000010a, 0x000000ee, 0x00000000, 0x00000034, 0x0000010c, 0x00000000, 0x00000000, 0x00000188,
    0x0000013c, 0x00000000, 0x00000040, 0x0000018b, 0x00000000, 0x00000000, 0x000001bb, 0x0000019f,
    0x00000000, 0x00000050, 0x000001be, 0x00000000, 0x00000000, 0x000001f2, 0x000001d6, 0x00000000,
    0x00000080, 0x000001f5, 0x00000000, 0x00000000, 0x00000231, 0x00000215, 0x00000000, 0x000000a0,
    0x00000234, 0x00000000, 0x00000000, 0x00000271, 0x00000255, 0x00000000, 0x000000c0, 0x00000274,
    0x00000000, 0x00000000,
};

static void test_effect_default_variable_value(void)
{
    D3D10_EFFECT_VARIABLE_DESC var_desc;
    ID3D10EffectVectorVariable *vector;
    ID3D10EffectScalarVariable *scalar;
    ID3D10EffectMatrixVariable *matrix;
    struct d3d10_matrix m_set, m_ret;
    ID3D10EffectVariable *v, *v2, *m;
    float float_v[4], float_s;
    ID3D10Effect *effect;
    ID3D10Device *device;
    int int_v[2], int_s;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_default_variable_value, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    memset(float_v, 0, sizeof(float_v));
    v = effect->lpVtbl->GetVariableByName(effect, "f4");
    vector = v->lpVtbl->AsVector(v);
    hr = vector->lpVtbl->GetFloatVector(vector, float_v);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(float_v[0] == 1.0f && float_v[1] == 2.0f && float_v[2] == 3.0f && float_v[3] == 4.0f,
            "Unexpected vector {%.8e,%.8e,%.8e,%.8e}\n", float_v[0], float_v[1], float_v[2], float_v[3]);

    memset(float_v, 0, sizeof(float_v));
    float_v[2] = float_v[3] = 5.0f;
    v = effect->lpVtbl->GetVariableByName(effect, "f2");
    vector = v->lpVtbl->AsVector(v);
    hr = vector->lpVtbl->GetFloatVector(vector, float_v);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(float_v[0] == 1.0f && float_v[1] == 2.0f && float_v[2] == 5.0f && float_v[3] == 5.0f,
            "Unexpected vector {%.8e,%.8e,%.8e,%.8e}\n", float_v[0], float_v[1], float_v[2], float_v[3]);

    memset(float_v, 0, sizeof(float_v));
    float_v[1] = float_v[2] = float_v[3] = 5.0f;
    v = effect->lpVtbl->GetVariableByName(effect, "f1");
    vector = v->lpVtbl->AsVector(v);
    hr = vector->lpVtbl->GetFloatVector(vector, float_v);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(float_v[0] == 1.0f && float_v[1] == 5.0f && float_v[2] == 5.0f && float_v[3] == 5.0f,
            "Unexpected vector {%.8e,%.8e,%.8e,%.8e}\n", float_v[0], float_v[1], float_v[2], float_v[3]);

    int_s = 0;
    v = effect->lpVtbl->GetVariableByName(effect, "i");
    scalar = v->lpVtbl->AsScalar(v);
    hr = scalar->lpVtbl->GetInt(scalar, &int_s);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(int_s == 10, "Unexpected value %d.\n", int_s);

    memset(int_v, 0, sizeof(int_v));
    v = effect->lpVtbl->GetVariableByName(effect, "i2");
    scalar = v->lpVtbl->AsScalar(v);
    hr = scalar->lpVtbl->GetIntArray(scalar, int_v, 0, 2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(int_v[0] == 9 && int_v[1] == 12, "Unexpected vector {%d,%d}\n", int_v[0], int_v[1]);
    hr = v->lpVtbl->GetDesc(v, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(var_desc.BufferOffset == 32, "Unexpected offset %u.\n", var_desc.BufferOffset);
    v2 = v->lpVtbl->GetElement(v, 0);
    hr = v2->lpVtbl->GetDesc(v2, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(var_desc.BufferOffset == 32, "Unexpected offset %u.\n", var_desc.BufferOffset);
    v2 = v->lpVtbl->GetElement(v, 1);
    hr = v2->lpVtbl->GetDesc(v2, &var_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(var_desc.BufferOffset == 48, "Unexpected offset %u.\n", var_desc.BufferOffset);

    float_s = 0.0f;
    v = effect->lpVtbl->GetVariableByName(effect, "f");
    scalar = v->lpVtbl->AsScalar(v);
    hr = scalar->lpVtbl->GetFloat(scalar, &float_s);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(float_s == 0.2f, "Unexpected value %.8e.\n", float_s);

    /* Matrix */
    v = effect->lpVtbl->GetVariableByName(effect, "m1");
    matrix = v->lpVtbl->AsMatrix(v);
    ok(matrix->lpVtbl->IsValid(matrix), "Expected valid matrix.\n");
    memset(&m_ret, 0, sizeof(m_ret));
    hr = matrix->lpVtbl->GetMatrix(matrix, &m_ret.m[0][0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_test_matrix(&m_set, D3D10_SVT_INT, 2, 3, 1);
    compare_matrix("m1", __LINE__, &m_set, &m_ret, 2, 3, FALSE);

    v = effect->lpVtbl->GetVariableByName(effect, "m2");
    matrix = v->lpVtbl->AsMatrix(v);
    ok(matrix->lpVtbl->IsValid(matrix), "Expected valid matrix.\n");
    memset(&m_ret, 0, sizeof(m_ret));
    hr = matrix->lpVtbl->GetMatrix(matrix, &m_ret.m[0][0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_test_matrix(&m_set, D3D10_SVT_INT, 2, 3, 1);
    compare_matrix("m2", __LINE__, &m_set, &m_ret, 2, 3, FALSE);

    v = effect->lpVtbl->GetVariableByName(effect, "m3");
    matrix = v->lpVtbl->AsMatrix(v);
    ok(matrix->lpVtbl->IsValid(matrix), "Expected valid matrix.\n");
    memset(&m_ret, 0, sizeof(m_ret));
    hr = matrix->lpVtbl->GetMatrix(matrix, &m_ret.m[0][0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    memset(&m_set, 0, sizeof(m_set));
    *(unsigned int *)&m_set.m[0][0] = 1;
    *(unsigned int *)&m_set.m[0][1] = 0;
    *(unsigned int *)&m_set.m[0][2] = 1;
    *(unsigned int *)&m_set.m[1][0] = 0;
    *(unsigned int *)&m_set.m[1][1] = 1;
    *(unsigned int *)&m_set.m[1][2] = 0;
    compare_matrix("m3", __LINE__, &m_set, &m_ret, 2, 3, FALSE);

    v = effect->lpVtbl->GetVariableByName(effect, "m4");
    matrix = v->lpVtbl->AsMatrix(v);
    ok(matrix->lpVtbl->IsValid(matrix), "Expected valid matrix.\n");
    memset(&m_ret, 0, sizeof(m_ret));
    hr = matrix->lpVtbl->GetMatrix(matrix, &m_ret.m[0][0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    set_test_matrix(&m_set, D3D10_SVT_FLOAT, 2, 2, 1);
    compare_matrix("m4", __LINE__, &m_set, &m_ret, 2, 2, FALSE);

    /* Struct */
    v = effect->lpVtbl->GetVariableByName(effect, "s1");
    ok(v->lpVtbl->IsValid(v), "Expected valid variable.\n");

    m = v->lpVtbl->GetMemberByName(v, "b1");
    ok(m->lpVtbl->IsValid(m), "Expected valid variable.\n");
    scalar = m->lpVtbl->AsScalar(m);
    hr = scalar->lpVtbl->GetBool(scalar, &ret);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ret == 1, "Unexpected value.\n");

    m = v->lpVtbl->GetMemberByName(v, "f2");
    ok(m->lpVtbl->IsValid(m), "Expected valid variable.\n");
    scalar = m->lpVtbl->AsScalar(m);
    hr = scalar->lpVtbl->GetFloat(scalar, &float_s);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(float_s == -0.2f, "Unexpected value %f.\n", float_s);

    m = v->lpVtbl->GetMemberByName(v, "b2");
    ok(m->lpVtbl->IsValid(m), "Expected valid variable.\n");
    scalar = m->lpVtbl->AsScalar(m);
    hr = scalar->lpVtbl->GetBool(scalar, &ret);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!ret, "Unexpected value.\n");

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_effect_raw_value(void)
{
    ID3D10EffectConstantBuffer *cb;
    ID3D10EffectVariable *v;
    ID3D10Effect *effect;
    ID3D10Device *device;
    unsigned int i;
    ULONG refcount;
    int i_v[10];
    HRESULT hr;
    float f;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_default_variable_value, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Read 1 float at a time, from float4 vector. */
    v = effect->lpVtbl->GetVariableByName(effect, "f4");
    for (i = 0; i < 4; ++i)
    {
        hr = v->lpVtbl->GetRawValue(v, &f, sizeof(f) * i, sizeof(f));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(f == i + 1.0f, "Unexpected value %f.\n", f);
    }
    /* Offset outside of variable storage, returns adjacent memory contents. */
    hr = v->lpVtbl->GetRawValue(v, &f, 16, sizeof(f));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(f == 1.0f, "Unexpected value %f.\n", f);
    hr = v->lpVtbl->GetRawValue(v, &f, 20, sizeof(f));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(f == 2.0f, "Unexpected value %f.\n", f);

    /* Array */
    v = effect->lpVtbl->GetVariableByName(effect, "i2");
    ok(v->lpVtbl->IsValid(v), "Expected valid variable.\n");
    hr = v->lpVtbl->GetRawValue(v, i_v, 0, 8 * sizeof(float));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(i_v[0] == 9,  "Unexpected value %d.\n", i_v[0]);
    ok(i_v[1] == 0,  "Unexpected value %d.\n", i_v[1]);
    ok(i_v[2] == 0,  "Unexpected value %d.\n", i_v[2]);
    ok(i_v[3] == 0,  "Unexpected value %d.\n", i_v[3]);
    ok(i_v[4] == 12, "Unexpected value %d.\n", i_v[4]);

    hr = v->lpVtbl->GetRawValue(v, &f, 20, sizeof(f));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(f == 0.2f, "Unexpected value %f.\n", f);

    /* Matrix */
    v = effect->lpVtbl->GetVariableByName(effect, "m1");
    hr = v->lpVtbl->GetRawValue(v, i_v, 0, sizeof(i_v));
    ok(i_v[0] == 1, "Unexpected value %d.\n", i_v[0]);
    ok(i_v[1] == 4, "Unexpected value %d.\n", i_v[1]);
    ok(i_v[2] == 0, "Unexpected value %d.\n", i_v[2]);
    ok(i_v[3] == 0, "Unexpected value %d.\n", i_v[3]);
    ok(i_v[4] == 2, "Unexpected value %d.\n", i_v[4]);
    ok(i_v[5] == 5, "Unexpected value %d.\n", i_v[5]);
    ok(i_v[6] == 0, "Unexpected value %d.\n", i_v[6]);
    ok(i_v[7] == 0, "Unexpected value %d.\n", i_v[7]);
    ok(i_v[8] == 3, "Unexpected value %d.\n", i_v[8]);
    ok(i_v[9] == 6, "Unexpected value %d.\n", i_v[9]);

    v = effect->lpVtbl->GetVariableByName(effect, "m2");
    hr = v->lpVtbl->GetRawValue(v, i_v, 0, 7 * sizeof(i_v[0]));
    ok(i_v[0] == 1, "Unexpected value %d.\n", i_v[0]);
    ok(i_v[1] == 2, "Unexpected value %d.\n", i_v[1]);
    ok(i_v[2] == 3, "Unexpected value %d.\n", i_v[2]);
    ok(i_v[3] == 0, "Unexpected value %d.\n", i_v[3]);
    ok(i_v[4] == 4, "Unexpected value %d.\n", i_v[4]);
    ok(i_v[5] == 5, "Unexpected value %d.\n", i_v[5]);
    ok(i_v[6] == 6, "Unexpected value %d.\n", i_v[6]);

    /* Read from constant buffer directly. */
    cb = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);

    for (i = 0; i < 4; ++i)
    {
        hr = cb->lpVtbl->GetRawValue(cb, &f, sizeof(f) * i, sizeof(f));
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(f == i + 1.0f, "Unexpected value %f.\n", f);
    }
    hr = cb->lpVtbl->GetRawValue(cb, &f, 16, sizeof(f));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(f == 1.0f, "Unexpected value %f.\n", f);
    hr = cb->lpVtbl->GetRawValue(cb, &f, 20, sizeof(f));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(f == 2.0f, "Unexpected value %f.\n", f);

    /* Invalid variable */
    v = effect->lpVtbl->GetVariableByName(effect, "invalid");
    hr = v->lpVtbl->GetRawValue(v, &f, 0, sizeof(f));
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
uint i1;
uint i1_a[2];
float4 fv1 = {0.5f, 0.6f, 0.7f, 0.8f};
float4 fv1_a[2] = { { 1.0f, 1.1f, 1.2f, 1.3f }, {0.1f, 0.2f, 0.3f, 0.4f} };
int i2 = 0x123;
int i2_a[2] = { 0x1, 0x2 };
float f1 = 0.3f;

technique10 tech
{
    pass P0
    {
        SetBlendState(NULL, fv1, i2);
        SetDepthStencilState(NULL, i1);
    }
    pass P1
    {
        SetBlendState(NULL, fv1_a[1], i2_a[1]);
        SetDepthStencilState(NULL, i1_a[1]);
    }
    pass P2
    {
        SetBlendState(NULL, f1, 0);
    }
}
#endif
static DWORD fx_test_effect_dynamic_numeric_field[] =
{
    0x43425844, 0xec585518, 0x7c866383, 0xf6e767fd, 0x019e1f41, 0x00000001, 0x00000562, 0x00000001,
    0x00000024, 0x30315846, 0x00000536, 0xfeff1001, 0x00000001, 0x00000007, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x0000030e, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6f6c4724,
    0x736c6162, 0x6e697500, 0x000d0074, 0x00010000, 0x00000000, 0x00040000, 0x00100000, 0x00040000,
    0x09190000, 0x31690000, 0x00000d00, 0x00000100, 0x00000200, 0x00001400, 0x00001000, 0x00000800,
    0x00091900, 0x5f316900, 0x6c660061, 0x3474616f, 0x00005200, 0x00000100, 0x00000000, 0x00001000,
    0x00001000, 0x00001000, 0x00210a00, 0x31766600, 0x00000000, 0x19999a3f, 0x3333333f, 0x4ccccd3f,
    0x0000523f, 0x00000100, 0x00000200, 0x00002000, 0x00001000, 0x00002000, 0x00210a00, 0x31766600,
    0x0000615f, 0xcd3f8000, 0x9a3f8ccc, 0x663f9999, 0xcd3fa666, 0xcd3dcccc, 0x9a3e4ccc, 0xcd3e9999,
    0x693ecccc, 0xcb00746e, 0x01000000, 0x00000000, 0x04000000, 0x10000000, 0x04000000, 0x11000000,
    0x69000009, 0x01230032, 0x00cb0000, 0x00010000, 0x00020000, 0x00140000, 0x00100000, 0x00080000,
    0x09110000, 0x32690000, 0x0100615f, 0x02000000, 0x66000000, 0x74616f6c, 0x00011b00, 0x00000100,
    0x00000000, 0x00000400, 0x00001000, 0x00000400, 0x00090900, 0x00316600, 0x3e99999a, 0x68636574,
    0x00305000, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0xa5003150,
    0x04000000, 0x0e000000, 0x04000001, 0x01000000, 0x02000000, 0x00000000, 0x4d000000, 0x04000000,
    0x01000000, 0x02000000, 0x00000000, 0x50000000, 0x01580032, 0x58440000, 0x4df04342, 0xad7a0608,
    0xb4963c34, 0xe74c5ab9, 0x000151a3, 0x01580000, 0x00030000, 0x002c0000, 0x00b40000, 0x00c00000,
    0x54430000, 0x00804241, 0x001c0000, 0x00570000, 0x04000000, 0x00014658, 0x001c0000, 0x01000000,
    0x00540000, 0x00300000, 0x00020000, 0x00010000, 0x00340000, 0x00440000, 0x31660000, 0x0000ab00,
    0x00010003, 0x00010001, 0x00000000, 0x999a0000, 0x00003e99, 0x00000000, 0x00000000, 0x78740000,
    0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f,
    0x2072656c, 0x312e3031, 0x4c43ab00, 0x00043449, 0x00000000, 0x58460000, 0x0090434c, 0x00040000,
    0x00010000, 0x00011000, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00040000, 0x00000000,
    0x00010000, 0x00011000, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00040000, 0x00010000,
    0x00010000, 0x00011000, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00040000, 0x00020000,
    0x00010000, 0x00011000, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00040000, 0x00030000,
    0xf0f00000, 0x0f0ff0f0, 0xffff0f0f, 0x00010000, 0x00020000, 0x00000000, 0x00010000, 0x00020000,
    0x00000000, 0x00040000, 0x00900000, 0x00000000, 0x00070000, 0xffff0000, 0x0000ffff, 0x002e0000,
    0x00120000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x004d0000, 0x00310000,
    0x00000000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00750000, 0x00590000, 0x00000000,
    0x00300000, 0x00790000, 0x00000000, 0x00000000, 0x00a50000, 0x00890000, 0x00000000, 0x00400000,
    0x00ab0000, 0x00000000, 0x00000000, 0x00eb0000, 0x00cf0000, 0x00000000, 0x00600000, 0x00ee0000,
    0x00000000, 0x00000000, 0x010e0000, 0x00f20000, 0x00000000, 0x00700000, 0x01130000, 0x00000000,
    0x00000000, 0x013d0000, 0x01210000, 0x00000000, 0x00840000, 0x01400000, 0x00000000, 0x00000000,
    0x01440000, 0x00030000, 0x00000000, 0x01490000, 0x00050000, 0x00000000, 0x000a0000, 0x00000000,
    0x00020000, 0x00750000, 0x000b0000, 0x00000000, 0x00020000, 0x00eb0000, 0x00020000, 0x00000000,
    0x00010000, 0x014c0000, 0x00090000, 0x00000000, 0x00020000, 0x002e0000, 0x00010000, 0x00000000,
    0x00010000, 0x01580000, 0x01640000, 0x00050000, 0x00000000, 0x000a0000, 0x00000000, 0x00030000,
    0x01670000, 0x000b0000, 0x00000000, 0x00030000, 0x016f0000, 0x00020000, 0x00000000, 0x00010000,
    0x01770000, 0x00090000, 0x00000000, 0x00030000, 0x01830000, 0x00010000, 0x00000000, 0x00010000,
    0x018b0000, 0x01970000, 0x00030000, 0x00000000, 0x000a0000, 0x00000000, 0x00060000, 0x019a0000,
    0x000b0000, 0x00000000, 0x00010000, 0x02f60000, 0x00020000, 0x00000000, 0x00010000, 0x03020000,
    0x00000000,
};

static void test_effect_dynamic_numeric_field(void)
{
    ID3D10EffectScalarVariable *scalar;
    ID3D10DepthStencilState *ds_state;
    ID3D10BlendState *blend_state;
    UINT stencil_ref, sample_mask;
    ID3D10EffectTechnique *tech;
    D3D10_PASS_DESC pass_desc;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    float blend_factor[4];
    ID3D10Effect *effect;
    ID3D10Device *device;
    unsigned int idx;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_effect_dynamic_numeric_field, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    tech = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    ok(tech->lpVtbl->IsValid(tech), "Expected valid technique.\n");

    /* Pass fields */
    pass = tech->lpVtbl->GetPassByName(tech, "P0");

    ID3D10Device_OMSetDepthStencilState(device, NULL, 0x1);
    memset(blend_factor, 0, sizeof(blend_factor));
    ID3D10Device_OMSetBlendState(device, NULL, blend_factor, 0);
    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetDepthStencilState(device, &ds_state, &stencil_ref);
    ok(!stencil_ref, "Unexpected stencil ref value %#x.\n", stencil_ref);
    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(blend_factor[0] == 0.5f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 0.6f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 0.7f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.8f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(sample_mask == 0x123, "Unexpected sample mask %#x.\n", sample_mask);

    v = effect->lpVtbl->GetVariableByName(effect, "i1");
    scalar = v->lpVtbl->AsScalar(v);
    hr = scalar->lpVtbl->SetInt(scalar, 2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetDepthStencilState(device, &ds_state, &stencil_ref);
    ok(stencil_ref == 0x2, "Unexpected stencil ref value %#x.\n", stencil_ref);

    hr = scalar->lpVtbl->SetInt(scalar, 3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(pass_desc.StencilRef == 0x3, "Unexpected stencil ref value %#x.\n", stencil_ref);
    ID3D10Device_OMGetDepthStencilState(device, &ds_state, &stencil_ref);
    ok(stencil_ref == 0x2, "Unexpected stencil ref value %#x.\n", stencil_ref);

    pass = tech->lpVtbl->GetPassByName(tech, "P1");

    v = effect->lpVtbl->GetVariableByName(effect, "i1_a");
    v = v->lpVtbl->GetElement(v, 1);
    scalar = v->lpVtbl->AsScalar(v);
    hr = scalar->lpVtbl->SetInt(scalar, 4);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMSetDepthStencilState(device, NULL, 0x1);
    memset(blend_factor, 0, sizeof(blend_factor));
    ID3D10Device_OMSetBlendState(device, NULL, blend_factor, 0);
    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetDepthStencilState(device, &ds_state, &stencil_ref);
    ok(stencil_ref == 0x4, "Unexpected stencil ref value %#x.\n", stencil_ref);
    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(blend_factor[0] == 0.1f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 0.2f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 0.3f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.4f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(sample_mask == 0x2, "Unexpected sample mask %#x.\n", sample_mask);

    pass = tech->lpVtbl->GetPassByName(tech, "P2");

    v = effect->lpVtbl->GetVariableByName(effect, "f1");
    scalar = v->lpVtbl->AsScalar(v);
    hr = scalar->lpVtbl->SetFloat(scalar, 0.123f);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    memset(blend_factor, 0, sizeof(blend_factor));
    ID3D10Device_OMSetBlendState(device, NULL, blend_factor, 0);
    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetDepthStencilState(device, &ds_state, &stencil_ref);
    ok(stencil_ref == 0x4, "Unexpected stencil ref value %#x.\n", stencil_ref);
    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 0.123f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Unexpected sample mask %#x.\n", sample_mask);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
float4 g_var;
float4 g_var2;
PixelShader ps_array[2];
VertexShader vs_array[2];
GeometryShader gs_array[2];
BlendState bs_array[2]
{
    { BlendEnable[0] = true; SrcBlend = inv_src_color; },
    { BlendEnable[0] = true; SrcBlend = src_color; }
};
technique10 tech
{
    pass p0
    {
        SetBlendState( bs_array[g_var.w], float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffff );
        SetPixelShader( ps_array[g_var.z] );
        SetVertexShader( vs_array[g_var.x + 0.1f] );
        SetGeometryShader( gs_array[g_var2.y] );
    }
}
#endif
static DWORD fx_test_index_expression[] =
{
    0x43425844, 0x85cafff9, 0xf7f791b5, 0xfc973931, 0x1de9cde1, 0x00000001, 0x00000778, 0x00000001,
    0x00000024, 0x30315846, 0x0000074c, 0xfeff1001, 0x00000001, 0x00000002, 0x00000004, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000588, 0x00000000, 0x00000000, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000000, 0x6f6c4724,
    0x736c6162, 0x6f6c6600, 0x00347461, 0x0000000d, 0x00000001, 0x00000000, 0x00000010, 0x00000010,
    0x00000010, 0x0000210a, 0x61765f67, 0x5f670072, 0x32726176, 0x78695000, 0x68536c65, 0x72656461,
    0x00003d00, 0x00000200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000500, 0x5f737000,
    0x61727261, 0x65560079, 0x78657472, 0x64616853, 0x6e007265, 0x02000000, 0x02000000, 0x00000000,
    0x00000000, 0x00000000, 0x06000000, 0x76000000, 0x72615f73, 0x00796172, 0x6d6f6547, 0x79727465,
    0x64616853, 0xa0007265, 0x02000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x07000000,
    0x67000000, 0x72615f73, 0x00796172, 0x6e656c42, 0x61745364, 0xd4006574, 0x02000000, 0x02000000,
    0x00000000, 0x00000000, 0x00000000, 0x02000000, 0x62000000, 0x72615f73, 0x00796172, 0x00000001,
    0x00000004, 0x00000001, 0x00000001, 0x00000002, 0x00000004, 0x00000001, 0x00000004, 0x00000001,
    0x00000001, 0x00000002, 0x00000003, 0x68636574, 0x00307000, 0x00000004, 0x00000001, 0x00000000,
    0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000003,
    0x0000ffff, 0x000000ec, 0x43425844, 0xc647666b, 0x92d327e5, 0x82d21ef6, 0xc31dc0cf, 0x00000001,
    0x000000ec, 0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c,
    0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002,
    0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001,
    0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
    0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846,
    0x00000030, 0x00000001, 0x13300001, 0x00000001, 0x00000000, 0x00000002, 0x00000003, 0x00000000,
    0x00000004, 0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x000000fb, 0x0000016c, 0x000000ec,
    0x43425844, 0xe6990a97, 0x7ed17834, 0x9f40de47, 0x01f476a7, 0x00000001, 0x000000ec, 0x00000003,
    0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400,
    0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038,
    0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874,
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
    0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000030, 0x00000001,
    0x13300001, 0x00000001, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000004, 0x00000000,
    0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000065, 0x00000264, 0x00000128, 0x43425844, 0x7a78de2e,
    0xcd31414e, 0xb6f69158, 0x92416c97, 0x00000001, 0x00000128, 0x00000003, 0x0000002c, 0x000000a8,
    0x000000c4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c,
    0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67,
    0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x34494c43, 0x00000014, 0x00000004, 0x3dcccccd, 0x00000000, 0x00000000, 0x00000000, 0x434c5846,
    0x0000005c, 0x00000002, 0x20400001, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x13300001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff,
    0x00000097, 0x0000035c, 0x000000ec, 0x43425844, 0x90e93368, 0x6c8d5166, 0x276e99ab, 0x25ccef44,
    0x00000001, 0x000000ec, 0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074,
    0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030,
    0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xab003272, 0x00030001, 0x00040001,
    0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c,
    0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000,
    0x434c5846, 0x00000030, 0x00000001, 0x13300001, 0x00000001, 0x00000000, 0x00000002, 0x00000001,
    0x00000000, 0x00000004, 0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x000000cb, 0x00000490,
    0x00000004, 0x00000020, 0x00000000, 0x00000002, 0xffffffff, 0x00000000, 0x00000030, 0x00000014,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000036, 0x00000014, 0x00000000,
    0x00000010, 0x00000000, 0x00000000, 0x00000000, 0x00000065, 0x00000049, 0x00000000, 0xffffffff,
    0x00000000, 0x00000000, 0x00000000, 0x00000097, 0x0000007b, 0x00000000, 0xffffffff, 0x00000000,
    0x00000000, 0x00000000, 0x000000cb, 0x000000af, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
    0x00000000, 0x000000fb, 0x000000df, 0x00000000, 0xffffffff, 0x00000002, 0x00000025, 0x00000000,
    0x00000001, 0x00000104, 0x00000026, 0x00000000, 0x00000001, 0x00000110, 0x00000002, 0x00000025,
    0x00000000, 0x00000001, 0x0000011c, 0x00000026, 0x00000000, 0x00000001, 0x00000128, 0x00000000,
    0x00000134, 0x00000001, 0x00000000, 0x00000139, 0x00000006, 0x00000000, 0x0000000a, 0x00000000,
    0x00000001, 0x0000013c, 0x0000000b, 0x00000000, 0x00000001, 0x00000160, 0x00000002, 0x00000000,
    0x00000005, 0x0000025c, 0x00000007, 0x00000000, 0x00000005, 0x00000354, 0x00000006, 0x00000000,
    0x00000005, 0x00000488, 0x00000008, 0x00000000, 0x00000005, 0x00000580,
};

static void test_effect_index_expression(void)
{
    D3D10_PASS_SHADER_DESC shader_desc;
    ID3D10EffectVectorVariable *vector;
    ID3D10EffectTechnique *tech;
    D3D10_BLEND_DESC bs_desc;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    ID3D10BlendState *bs;
    ID3D10Effect *effect;
    ID3D10Device *device;
    ULONG refcount;
    float val[4];
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_index_expression, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Initial index */
    tech = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    pass = tech->lpVtbl->GetPassByIndex(tech, 0);
    hr = pass->lpVtbl->GetPixelShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ShaderIndex, "Unexpected shader index.\n");

    v = effect->lpVtbl->GetVariableByName(effect, "g_var");
    vector = v->lpVtbl->AsVector(v);

    set_vec4(val, 0.0f, 0.0f, 1.0f, 0.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->GetPixelShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shader_desc.ShaderIndex == 1, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetBlendState(device, &bs, NULL, NULL);
    ID3D10BlendState_GetDesc(bs, &bs_desc);
    ok(bs_desc.SrcBlend == D3D10_BLEND_INV_SRC_COLOR, "Unexpected blend state parameter %u.\n", bs_desc.SrcBlend);
    ID3D10BlendState_Release(bs);

    set_vec4(val, 0.0f, 0.0f, 1.0f, 1.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetBlendState(device, &bs, NULL, NULL);
    ID3D10BlendState_GetDesc(bs, &bs_desc);
    ok(bs_desc.SrcBlend == D3D10_BLEND_SRC_COLOR, "Unexpected blend state parameter %u.\n", bs_desc.SrcBlend);
    ID3D10BlendState_Release(bs);

    /* Out of bounds index */
    set_vec4(val, 0.0f, 0.0f, 2.0f, 2.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->GetPixelShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ShaderIndex, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetBlendState(device, &bs, NULL, NULL);
    ID3D10BlendState_GetDesc(bs, &bs_desc);
    ok(bs_desc.SrcBlend == D3D10_BLEND_INV_SRC_COLOR, "Unexpected blend state parameter %u.\n", bs_desc.SrcBlend);
    ID3D10BlendState_Release(bs);

    set_vec4(val, 0.0f, 0.0f, 3.0f, 3.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->GetPixelShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ShaderIndex, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetBlendState(device, &bs, NULL, NULL);
    ID3D10BlendState_GetDesc(bs, &bs_desc);
    ok(bs_desc.SrcBlend == D3D10_BLEND_INV_SRC_COLOR, "Unexpected blend state parameter %u.\n", bs_desc.SrcBlend);
    ID3D10BlendState_Release(bs);

    set_vec4(val, 0.0f, 0.0f, -1.0f, -1.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->GetPixelShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ShaderIndex, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetBlendState(device, &bs, NULL, NULL);
    ID3D10BlendState_GetDesc(bs, &bs_desc);
    ok(bs_desc.SrcBlend == D3D10_BLEND_INV_SRC_COLOR, "Unexpected blend state parameter %u.\n", bs_desc.SrcBlend);
    ID3D10BlendState_Release(bs);

    set_vec4(val, 0.0f, 0.0f, -11.0f, 0.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->GetPixelShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ShaderIndex, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_OMGetBlendState(device, &bs, NULL, NULL);
    ID3D10BlendState_GetDesc(bs, &bs_desc);
    ok(bs_desc.SrcBlend == D3D10_BLEND_INV_SRC_COLOR, "Unexpected blend state parameter %u.\n", bs_desc.SrcBlend);
    ID3D10BlendState_Release(bs);

    hr = pass->lpVtbl->GetVertexShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ShaderIndex, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    set_vec4(val, 0.9f, 0.0f, 1.0f, 0.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->GetVertexShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shader_desc.ShaderIndex == 1, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    v = effect->lpVtbl->GetVariableByName(effect, "g_var2");
    vector = v->lpVtbl->AsVector(v);

    hr = pass->lpVtbl->GetGeometryShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ShaderIndex, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    set_vec4(val, 0.0f, 1.0f, 0.0f, 0.0f);
    hr = vector->lpVtbl->SetFloatVector(vector, val);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->GetGeometryShaderDesc(pass, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shader_desc.ShaderIndex == 1, "Unexpected shader index %#x.\n", shader_desc.ShaderIndex);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
float4 g_var;
float4 g_var2;
int4 g_var3;
uint4 g_var4;

DepthStencilState ds_state
{
    StencilEnable = g_var.x;
};

technique10 tech
{
    pass p0
    {
        SetBlendState( NULL, -g_var + sin(g_var.x) + cos(g_var.y) + 0.1f + g_var2 * 2.1f, 0 );
    }
    pass p1
    {
        SetBlendState( NULL, g_var.x + g_var2 + g_var.y, 0 );
    }
    pass p2
    {
        SetBlendState( NULL, min(g_var.x, g_var2.x) + max(g_var.y, g_var2.y) + min(g_var, g_var2) + max(g_var, g_var2), 0 );
    }
    pass p3
    {
        SetBlendState( NULL, min(g_var.x, g_var2), 0 );
    }
    pass p4
    {
        SetBlendState( NULL, max(g_var.x, g_var2), 0 );
    }
    pass p5
    {
        SetBlendState( NULL, g_var.x / g_var, 0 );
    }
    pass p6
    {
        SetBlendState( NULL, 1.0f / g_var, 0 );
    }
    pass p7
    {
        SetBlendState( NULL, frac(g_var), 0 );
    }
    pass p8
    {
        SetBlendState( NULL, 0.1 * -max(g_var3.x, g_var3.y), 0 );
    }
    pass p9
    {
        SetBlendState( NULL, g_var4.x / g_var4.y, 0 );
    }
    pass p10
    {
        SetBlendState( NULL, g_var.x ? g_var.y : g_var.z, 0 );
    }
    pass p11
    {
        SetBlendState( NULL, min(g_var4.x, g_var4.y), 0 );
    }
    pass p12
    {
        SetBlendState( NULL, max(g_var4.x, g_var4.y), 0 );
    }
    pass p13
    {
        SetBlendState( NULL, min(g_var3.x, g_var3.y), 0 );
    }
    pass p14
    {
        SetBlendState( NULL, g_var4.x + g_var4.y, 0 );
    }
    pass p15
    {
        SetBlendState( NULL, asin(g_var.x), 0 );
    }
    pass p16
    {
        SetBlendState( NULL, acos(g_var.x), 0 );
    }
    pass p17
    {
        SetBlendState( NULL, atan(g_var.x), 0 );
    }
    pass p18
    {
        SetBlendState( NULL, atan2(g_var.x, g_var.y), 0 );
    }
    pass p19
    {
        SetBlendState( NULL, g_var3.x / g_var3.y, 0 );
    }
    pass p20
    {
        SetBlendState( NULL, g_var3.x < g_var3.y ? (g_var3.x >= g_var3.z ? 0 : 1 ) :
                (g_var3.x != g_var3.z ? g_var3.z == g_var3.w : 2), 0 );
    }
    pass p21
    {
        SetBlendState( NULL, g_var4.x >= g_var4.y ? 0 : (g_var4.y < g_var4.z ? 1 : 2), 0 );
    }
    pass p22
    {
        SetBlendState( NULL, ceil(g_var), 0 );
    }
    pass p23
    {
        SetBlendState( NULL, floor(g_var), 0 );
    }
    pass p24
    {
        SetBlendState( NULL, dot(g_var, g_var2), 0 );
    }
    pass p25
    {
        SetBlendState( NULL, dot(g_var.xyz, g_var2.xyz), 0 );
    }
    pass p26
    {
        SetBlendState( NULL, dot(g_var.zyxw, g_var2.yzwx), 0 );
    }
    pass p27
    {
        SetBlendState( NULL, dot(g_var.zyx, g_var2.yzw), 0 );
    }
    pass p28
    {
        SetBlendState( NULL, g_var3.x * g_var3.y, 0 );
    }
    pass p29
    {
        SetBlendState( NULL, sqrt(g_var), 0 );
    }
    pass p30
    {
        SetBlendState( NULL, rsqrt(g_var), 0 );
    }
    pass p31
    {
        SetBlendState( NULL, log(g_var), 0 );
    }
    pass p32
    {
        SetBlendState( NULL, exp(g_var), 0 );
    }
    pass p33
    {
        SetBlendState( NULL, ~g_var3, 0 );
    }
    pass p34
    {
        SetBlendState( NULL, g_var4 | g_var3, 0 );
    }
    pass p35
    {
        SetBlendState( NULL, float4(g_var3.x >> g_var3.y, g_var3.z << g_var3.w, g_var4.x >> g_var4.y, 0), 0 );
    }
    pass p36
    {
        SetBlendState( NULL, float4(log2(g_var.x), log10(g_var.y), exp2(g_var.z), 0.0), 0 );
    }
}
#endif
static DWORD fx_test_value_expression[] =
{
    0x43425844, 0xf55b96f2, 0x76722813, 0x530fae3e, 0x4cd9c847, 0x00000001, 0x00004964, 0x00000001,
    0x00000024, 0x30315846, 0x00004938, 0xfeff1001, 0x00000001, 0x00000004, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00003f84, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6f6c4724,
    0x736c6162, 0x6f6c6600, 0x00347461, 0x0000000d, 0x00000001, 0x00000000, 0x00000010, 0x00000010,
    0x00000010, 0x0000210a, 0x61765f67, 0x5f670072, 0x32726176, 0x746e6900, 0x003d0034, 0x00010000,
    0x00000000, 0x00100000, 0x00100000, 0x00100000, 0x21120000, 0x5f670000, 0x33726176, 0x6e697500,
    0x65003474, 0x01000000, 0x00000000, 0x10000000, 0x10000000, 0x10000000, 0x1a000000, 0x67000021,
    0x7261765f, 0x65440034, 0x53687470, 0x636e6574, 0x74536c69, 0x00657461, 0x0000008e, 0x00000002,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x735f7364, 0x65746174, 0x0000ec00,
    0x42584400, 0x12544043, 0xf05fca79, 0x55a99c90, 0xe16165fe, 0x00000145, 0x0000ec00, 0x00000300,
    0x00002c00, 0x0000a800, 0x0000b400, 0x41544300, 0x00007442, 0x00001c00, 0x00004b00, 0x58040000,
    0x00000146, 0x00001c00, 0x00010000, 0x00004800, 0x00003000, 0x00000200, 0x00000100, 0x00003800,
    0x00000000, 0x765f6700, 0xab007261, 0x030001ab, 0x04000100, 0x00000100, 0x00000000, 0x00787400,
    0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d,
    0x31207265, 0x00312e30, 0x494c43ab, 0x00000434, 0x00000000, 0x4c584600, 0x00003043, 0x00000100,
    0x70000100, 0x00000113, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000400, 0x00000000,
    0xf0f0f000, 0x0f0f0ff0, 0x00ffff0f, 0x63657400, 0x30700068, 0x00024400, 0x42584400, 0xff104243,
    0xa078c06f, 0xb9d7d003, 0x421eb8c7, 0x0000014c, 0x00024400, 0x00000300, 0x00002c00, 0x0000d400,
    0x0000f000, 0x41544300, 0x0000a042, 0x00001c00, 0x00007700, 0x58040000, 0x00000246, 0x00001c00,
    0x00010000, 0x00007400, 0x00004400, 0x00000200, 0x00000100, 0x00004c00, 0x00000000, 0x00005c00,
    0x01000200, 0x00000100, 0x00006400, 0x00000000, 0x765f6700, 0xab007261, 0x030001ab, 0x04000100,
    0x00000100, 0x00000000, 0x765f6700, 0x00327261, 0x030001ab, 0x04000100, 0x00000100, 0x00000000,
    0x00787400, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072,
    0x6c69706d, 0x31207265, 0x00312e30, 0x494c43ab, 0x00001434, 0x00000400, 0xcccccd00, 0x0666663d,
    0x00000040, 0x00000000, 0x4c584600, 0x00014c43, 0x00000800, 0x10000400, 0x00000110, 0x00000000,
    0x00000200, 0x00000000, 0x00000000, 0x00000700, 0x00000000, 0x80000100, 0x00000110, 0x00000000,
    0x00000200, 0x00000000, 0x00000000, 0x00000700, 0x00000400, 0x40000400, 0x000002a0, 0x00000000,
    0x00000700, 0x00000400, 0x00000000, 0x00000700, 0x00000000, 0x00000000, 0x00000700, 0x00000800,
    0x90000100, 0x00000110, 0x00000000, 0x00000200, 0x00000100, 0x00000000, 0x00000700, 0x00000000,
    0x40000400, 0x000002a0, 0x00000000, 0x00000700, 0x00000000, 0x00000000, 0x00000700, 0x00000800,
    0x00000000, 0x00000700, 0x00000400, 0x40000400, 0x000002a0, 0x00000000, 0x00000100, 0x00000000,
    0x00000000, 0x00000700, 0x00000400, 0x00000000, 0x00000700, 0x00000000, 0x50000400, 0x000002a0,
    0x00000000, 0x00000100, 0x00000100, 0x00000000, 0x00000200, 0x00000400, 0x00000000, 0x00000700,
    0x00000400, 0x40000400, 0x00000220, 0x00000000, 0x00000700, 0x00000000, 0x00000000, 0x00000700,
    0x00000400, 0x00000000, 0x00000400, 0x00000000, 0xf0f0f000, 0x0f0f0ff0, 0x00ffff0f, 0x00000100,
    0x00000200, 0x00000000, 0x00000100, 0x00000200, 0x00000000, 0x00317000, 0x00000150, 0x43425844,
    0xc6a29e4c, 0x6292ed35, 0xd90bb8cb, 0x50dcd25f, 0x00000001, 0x00000150, 0x00000003, 0x0000002c,
    0x000000d4, 0x000000e0, 0x42415443, 0x000000a0, 0x0000001c, 0x00000077, 0x46580400, 0x00000002,
    0x0000001c, 0x00000100, 0x00000074, 0x00000044, 0x00000002, 0x00000001, 0x0000004c, 0x00000000,
    0x0000005c, 0x00010002, 0x00000001, 0x00000064, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001,
    0x00040001, 0x00000001, 0x00000000, 0x61765f67, 0xab003272, 0x00030001, 0x00040001, 0x00000001,
    0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
    0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846,
    0x00000068, 0x00000002, 0xa0400004, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000002, 0x00000004, 0x00000000, 0x00000007, 0x00000000, 0xa0400004, 0x00000002, 0x00000000,
    0x00000002, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000,
    0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002,
    0x00000000, 0x2c003270, 0x44000002, 0x9e434258, 0xfae994e0, 0xb641b826, 0x0bac8f3a, 0x019d4eb7,
    0x2c000000, 0x03000002, 0x2c000000, 0xd4000000, 0xe0000000, 0x43000000, 0xa0424154, 0x1c000000,
    0x77000000, 0x00000000, 0x02465804, 0x1c000000, 0x00000000, 0x74000001, 0x44000000, 0x02000000,
    0x01000000, 0x4c000000, 0x00000000, 0x5c000000, 0x02000000, 0x01000100, 0x64000000, 0x00000000,
    0x67000000, 0x7261765f, 0x01abab00, 0x01000300, 0x01000400, 0x00000000, 0x67000000, 0x7261765f,
    0x01ab0032, 0x01000300, 0x01000400, 0x00000000, 0x74000000, 0x694d0078, 0x736f7263, 0x2074666f,
    0x20295228, 0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x2e303120, 0x43ab0031,
    0x0434494c, 0x00000000, 0x46000000, 0x44434c58, 0x07000001, 0x01000000, 0x02200000, 0x00000000,
    0x02000000, 0x00000000, 0x00000000, 0x02000000, 0x04000000, 0x00000000, 0x07000000, 0x00000000,
    0x01000000, 0x02201000, 0x00000000, 0x02000000, 0x01000000, 0x00000000, 0x02000000, 0x05000000,
    0x00000000, 0x07000000, 0x01000000, 0x01000000, 0x02204000, 0x00000000, 0x07000000, 0x01000000,
    0x00000000, 0x07000000, 0x00000000, 0x00000000, 0x07000000, 0x04000000, 0x04000000, 0x02200000,
    0x00000000, 0x02000000, 0x00000000, 0x00000000, 0x02000000, 0x04000000, 0x00000000, 0x07000000,
    0x00000000, 0x04000000, 0x02a04000, 0x00000000, 0x07000000, 0x04000000, 0x00000000, 0x07000000,
    0x00000000, 0x00000000, 0x07000000, 0x08000000, 0x04000000, 0x02201000, 0x00000000, 0x02000000,
    0x00000000, 0x00000000, 0x02000000, 0x04000000, 0x00000000, 0x07000000, 0x00000000, 0x04000000,
    0x02204000, 0x00000000, 0x07000000, 0x00000000, 0x00000000, 0x07000000, 0x08000000, 0x00000000,
    0x04000000, 0x00000000, 0xf0000000, 0x0ff0f0f0, 0xff0f0f0f, 0x010000ff, 0x02000000, 0x00000000,
    0x01000000, 0x02000000, 0x00000000, 0x70000000, 0x01240033, 0x58440000, 0xf4c54342, 0x070658ce,
    0xcabd2023, 0x24d22fca, 0x00010f0c, 0x01240000, 0x00030000, 0x002c0000, 0x00d40000, 0x00e00000,
    0x54430000, 0x00a04241, 0x001c0000, 0x00770000, 0x04000000, 0x00024658, 0x001c0000, 0x01000000,
    0x00740000, 0x00440000, 0x00020000, 0x00010000, 0x004c0000, 0x00000000, 0x005c0000, 0x00020000,
    0x00010001, 0x00640000, 0x00000000, 0x5f670000, 0x00726176, 0x0001abab, 0x00010003, 0x00010004,
    0x00000000, 0x5f670000, 0x32726176, 0x0001ab00, 0x00010003, 0x00010004, 0x00000000, 0x78740000,
    0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f,
    0x2072656c, 0x312e3031, 0x4c43ab00, 0x00043449, 0x00000000, 0x58460000, 0x003c434c, 0x00010000,
    0x00040000, 0x0002a000, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00020000, 0x00040000,
    0x00000000, 0x00040000, 0x00000000, 0xf0f00000, 0x0f0ff0f0, 0xffff0f0f, 0x00010000, 0x00020000,
    0x00000000, 0x00010000, 0x00020000, 0x00000000, 0x34700000, 0x00012400, 0x42584400, 0xc902bb43,
    0xafcba635, 0xa383ff89, 0xed53a9f6, 0x000001f8, 0x00012400, 0x00000300, 0x00002c00, 0x0000d400,
    0x0000e000, 0x41544300, 0x0000a042, 0x00001c00, 0x00007700, 0x58040000, 0x00000246, 0x00001c00,
    0x00010000, 0x00007400, 0x00004400, 0x00000200, 0x00000100, 0x00004c00, 0x00000000, 0x00005c00,
    0x01000200, 0x00000100, 0x00006400, 0x00000000, 0x765f6700, 0xab007261, 0x030001ab, 0x04000100,
    0x00000100, 0x00000000, 0x765f6700, 0x00327261, 0x030001ab, 0x04000100, 0x00000100, 0x00000000,
    0x00787400, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072,
    0x6c69706d, 0x31207265, 0x00312e30, 0x494c43ab, 0x00000434, 0x00000000, 0x4c584600, 0x00003c43,
    0x00000100, 0x10000400, 0x000002a0, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000200,
    0x00000400, 0x00000000, 0x00000400, 0x00000000, 0xf0f0f000, 0x0f0f0ff0, 0x00ffff0f, 0x00000100,
    0x00000200, 0x00000000, 0x00000100, 0x00000200, 0x00000000, 0x00357000, 0x000000f8, 0x43425844,
    0xf74bf893, 0x3d094d0b, 0xf013f99e, 0x99d86fff, 0x00000001, 0x000000f8, 0x00000003, 0x0000002c,
    0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001,
    0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000,
    0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369,
    0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072,
    0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x0000003c, 0x00000001, 0xa0800004,
    0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000004, 0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000,
    0x00000001, 0x00000002, 0x00000000, 0x4c003670, 0x44000001, 0x0f434258, 0xd38746b5, 0x34bf74fe,
    0x9d061641, 0x01dbbd02, 0x4c000000, 0x03000001, 0x2c000000, 0xa8000000, 0xb4000000, 0x43000000,
    0x74424154, 0x1c000000, 0x4b000000, 0x00000000, 0x01465804, 0x1c000000, 0x00000000, 0x48000001,
    0x30000000, 0x02000000, 0x01000000, 0x38000000, 0x00000000, 0x67000000, 0x7261765f, 0x01abab00,
    0x01000300, 0x01000400, 0x00000000, 0x74000000, 0x694d0078, 0x736f7263, 0x2074666f, 0x20295228,
    0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x2e303120, 0x43ab0031, 0x0434494c,
    0x00000000, 0x46000000, 0x90434c58, 0x04000000, 0x01000000, 0x01103000, 0x00000000, 0x02000000,
    0x00000000, 0x00000000, 0x04000000, 0x00000000, 0x01000000, 0x01103000, 0x00000000, 0x02000000,
    0x01000000, 0x00000000, 0x04000000, 0x01000000, 0x01000000, 0x01103000, 0x00000000, 0x02000000,
    0x02000000, 0x00000000, 0x04000000, 0x02000000, 0x01000000, 0x01103000, 0x00000000, 0x02000000,
    0x03000000, 0x00000000, 0x04000000, 0x03000000, 0xf0000000, 0x0ff0f0f0, 0xff0f0f0f, 0x010000ff,
    0x02000000, 0x00000000, 0x01000000, 0x02000000, 0x00000000, 0x70000000, 0x00ec0037, 0x58440000,
    0xaa424342, 0x4a779667, 0x419ccee2, 0x743a2086, 0x0001e1df, 0x00ec0000, 0x00030000, 0x002c0000,
    0x00a80000, 0x00b40000, 0x54430000, 0x00744241, 0x001c0000, 0x004b0000, 0x04000000, 0x00014658,
    0x001c0000, 0x01000000, 0x00480000, 0x00300000, 0x00020000, 0x00010000, 0x00380000, 0x00000000,
    0x5f670000, 0x00726176, 0x0001abab, 0x00010003, 0x00010004, 0x00000000, 0x78740000, 0x63694d00,
    0x6f736f72, 0x28207466, 0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f, 0x2072656c,
    0x312e3031, 0x4c43ab00, 0x00043449, 0x00000000, 0x58460000, 0x0030434c, 0x00010000, 0x00040000,
    0x00011040, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00040000, 0x00000000, 0xf0f00000,
    0x0f0ff0f0, 0xffff0f0f, 0x00010000, 0x00020000, 0x00000000, 0x00010000, 0x00020000, 0x00000000,
    0x38700000, 0x0001f800, 0x42584400, 0x6df99043, 0xe623cb12, 0xb7d81780, 0x4894a558, 0x000001ba,
    0x0001f800, 0x00000300, 0x00002c00, 0x0000a800, 0x0000c400, 0x41544300, 0x00007442, 0x00001c00,
    0x00004b00, 0x58040000, 0x00000146, 0x00001c00, 0x00010000, 0x00004800, 0x00003000, 0x00000200,
    0x00000100, 0x00003800, 0x00000000, 0x765f6700, 0x00337261, 0x020001ab, 0x04000100, 0x00000100,
    0x00000000, 0x00787400, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168,
    0x6f432072, 0x6c69706d, 0x31207265, 0x00312e30, 0x494c43ab, 0x00001434, 0x00000400, 0xcccccd00,
    0x0000003d, 0x00000000, 0x00000000, 0x4c584600, 0x00012c43, 0x00000700, 0xe0000100, 0x00000221,
    0x00000000, 0x00000200, 0x00000100, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000700,
    0x00000000, 0x00000100, 0x00000112, 0x00000000, 0x00000700, 0x00000000, 0x00000000, 0x00000700,
    0x00000400, 0x00000100, 0x00000113, 0x00000000, 0x00000700, 0x00000400, 0x00000000, 0x00000700,
    0x00000000, 0x50000100, 0x00000220, 0x00000000, 0x00000700, 0x00000000, 0x00000000, 0x00000100,
    0x00000000, 0x00000000, 0x00000400, 0x00000000, 0x50000100, 0x00000220, 0x00000000, 0x00000700,
    0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000400, 0x00000100, 0x50000100,
    0x00000220, 0x00000000, 0x00000700, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000,
    0x00000400, 0x00000200, 0x50000100, 0x00000220, 0x00000000, 0x00000700, 0x00000000, 0x00000000,
    0x00000100, 0x00000000, 0x00000000, 0x00000400, 0x00000300, 0xf0f0f000, 0x0f0f0ff0, 0x00ffff0f,
    0x00000100, 0x00000200, 0x00000000, 0x00000100, 0x00000200, 0x00000000, 0x00397000, 0x00000178,
    0x43425844, 0x453449ff, 0x2e103a44, 0x8883c1ca, 0x78e0dcb1, 0x00000001, 0x00000178, 0x00000003,
    0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400,
    0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038,
    0x00000000, 0x61765f67, 0xab003472, 0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874,
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
    0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x000000bc, 0x00000005,
    0x21a00001, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000000, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000001, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000002, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002,
    0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00303170, 0x000001cc, 0x43425844, 0x18677c9b,
    0xf5e472af, 0x9ba288a2, 0xcca43bb2, 0x00000001, 0x000001cc, 0x00000003, 0x0000002c, 0x000000a8,
    0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c,
    0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67,
    0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000110, 0x00000005, 0x13700001, 0x00000001,
    0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x30100001, 0x00000003,
    0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002,
    0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x30100001, 0x00000003, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002, 0x00000002, 0x00000000,
    0x00000004, 0x00000001, 0x30100001, 0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x00000000,
    0x00000002, 0x00000001, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000004, 0x00000002,
    0x30100001, 0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000002, 0x00000001,
    0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f,
    0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00313170,
    0x00000178, 0x43425844, 0xb2dec0d7, 0x6d936ace, 0x1e31a035, 0x7de038f5, 0x00000001, 0x00000178,
    0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b,
    0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001,
    0x00000038, 0x00000000, 0x61765f67, 0xab003472, 0x00020001, 0x00040001, 0x00000001, 0x00000000,
    0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320,
    0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x000000bc,
    0x00000005, 0x21f00001, 0x00000002, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x13100001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x13100001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x13100001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x13100001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001,
    0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00323170, 0x00000178, 0x43425844,
    0x0334f480, 0xa1c4caca, 0x9e1735e8, 0x1d8a5fb0, 0x00000001, 0x00000178, 0x00000003, 0x0000002c,
    0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001,
    0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000,
    0x61765f67, 0xab003472, 0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369,
    0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072,
    0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x000000bc, 0x00000005, 0x22000001,
    0x00000002, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000007, 0x00000000, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000,
    0x00000004, 0x00000000, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000,
    0x00000004, 0x00000001, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000,
    0x00000004, 0x00000002, 0x13100001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000,
    0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000,
    0x00000001, 0x00000002, 0x00000000, 0x00333170, 0x00000178, 0x43425844, 0x98cbead4, 0xa96cee69,
    0xfa67fbf6, 0x10832177, 0x00000001, 0x00000178, 0x00000003, 0x0000002c, 0x000000a8, 0x000000b4,
    0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100,
    0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xab003372,
    0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820,
    0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43,
    0x00000004, 0x00000000, 0x434c5846, 0x000000bc, 0x00000005, 0x21d00001, 0x00000002, 0x00000000,
    0x00000002, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000000,
    0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000,
    0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000001,
    0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000002,
    0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000003,
    0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002,
    0x00000000, 0x00343170, 0x00000178, 0x43425844, 0xd7c8865b, 0xe0b95177, 0xcad7d001, 0x106ac5ac,
    0x00000001, 0x00000178, 0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074,
    0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030,
    0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xab003472, 0x00020001, 0x00040001,
    0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c,
    0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000,
    0x434c5846, 0x000000bc, 0x00000005, 0x21600001, 0x00000002, 0x00000000, 0x00000002, 0x00000001,
    0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x13100001, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x13100001, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x13100001, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x13100001, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f,
    0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00353170,
    0x0000014c, 0x43425844, 0x858202fa, 0x6eea0ef1, 0x035697d7, 0x8937d809, 0x00000001, 0x0000014c,
    0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b,
    0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001,
    0x00000038, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000,
    0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320,
    0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000090,
    0x00000004, 0x10a00001, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004,
    0x00000000, 0x10a00001, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004,
    0x00000001, 0x10a00001, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004,
    0x00000002, 0x10a00001, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004,
    0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001,
    0x00000002, 0x00000000, 0x00363170, 0x0000014c, 0x43425844, 0x27003e76, 0x45cf2121, 0x9b3336ae,
    0xca07856b, 0x00000001, 0x0000014c, 0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443,
    0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048,
    0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001,
    0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
    0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004,
    0x00000000, 0x434c5846, 0x00000090, 0x00000004, 0x10b00001, 0x00000001, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x10b00001, 0x00000001, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x10b00001, 0x00000001, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x10b00001, 0x00000001, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001,
    0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00373170, 0x0000014c, 0x43425844,
    0x7a9aaef0, 0xcc3049d9, 0x1c45c020, 0xa01af792, 0x00000001, 0x0000014c, 0x00000003, 0x0000002c,
    0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001,
    0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000,
    0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369,
    0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072,
    0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000090, 0x00000004, 0x10c00001,
    0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x10c00001,
    0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x10c00001,
    0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x10c00001,
    0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0,
    0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000,
    0x00383170, 0x0000017c, 0x43425844, 0xb9755ca9, 0x574af4c2, 0x92daa707, 0xdab0053b, 0x00000001,
    0x0000017c, 0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c,
    0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002,
    0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001,
    0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
    0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846,
    0x000000c0, 0x00000004, 0x20600001, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000002, 0x00000001, 0x00000000, 0x00000004, 0x00000000, 0x20600001, 0x00000002, 0x00000000,
    0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000004, 0x00000001,
    0x20600001, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000001,
    0x00000000, 0x00000004, 0x00000002, 0x20600001, 0x00000002, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f,
    0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00393170,
    0x000002d0, 0x43425844, 0xca0a64ed, 0x1e48bebb, 0x50b61480, 0x81af66f1, 0x00000001, 0x000002d0,
    0x00000003, 0x0000002c, 0x000000a8, 0x000000c4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b,
    0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001,
    0x00000038, 0x00000000, 0x61765f67, 0xab003372, 0x00020001, 0x00040001, 0x00000001, 0x00000000,
    0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320,
    0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000014, 0x00000004, 0x80000000, 0x00000000,
    0x00000000, 0x00000000, 0x434c5846, 0x00000204, 0x0000000d, 0x12000001, 0x00000001, 0x00000000,
    0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x21e00001, 0x00000002, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000004,
    0x12000001, 0x00000001, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x21e00001, 0x00000002, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000002, 0x00000001,
    0x00000000, 0x00000007, 0x00000005, 0x21a00001, 0x00000002, 0x00000000, 0x00000007, 0x00000004,
    0x00000000, 0x00000007, 0x00000005, 0x00000000, 0x00000007, 0x00000000, 0x12000001, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000007, 0x00000004, 0x23300001, 0x00000002,
    0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000007,
    0x00000001, 0x23000001, 0x00000002, 0x00000000, 0x00000007, 0x00000001, 0x00000000, 0x00000001,
    0x00000000, 0x00000000, 0x00000007, 0x00000005, 0x30100001, 0x00000003, 0x00000000, 0x00000007,
    0x00000005, 0x00000000, 0x00000007, 0x00000004, 0x00000000, 0x00000007, 0x00000000, 0x00000000,
    0x00000007, 0x00000008, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000000,
    0x00000004, 0x00000000, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000000,
    0x00000004, 0x00000001, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000000,
    0x00000004, 0x00000002, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000000,
    0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000,
    0x00000001, 0x00000002, 0x00000000, 0x00303270, 0x000002e0, 0x43425844, 0x561ee53c, 0x9807d928,
    0x8a3e8c4e, 0x78f7bcb7, 0x00000001, 0x000002e0, 0x00000003, 0x0000002c, 0x000000a8, 0x000000c4,
    0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100,
    0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xab003372,
    0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820,
    0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43,
    0x00000014, 0x00000004, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x434c5846, 0x00000214,
    0x0000000c, 0x21200001, 0x00000002, 0x00000000, 0x00000002, 0x00000003, 0x00000000, 0x00000002,
    0x00000002, 0x00000000, 0x00000007, 0x00000000, 0x23000001, 0x00000002, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000000, 0x00000007, 0x00000004, 0x21300001,
    0x00000002, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000007, 0x00000000, 0x30100001, 0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x00000000,
    0x00000007, 0x00000004, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00000007, 0x00000008,
    0x21100001, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000002,
    0x00000000, 0x00000007, 0x00000000, 0x30100001, 0x00000003, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000000, 0x00000007,
    0x00000004, 0x21000001, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002,
    0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x30100001, 0x00000003, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000007, 0x00000004, 0x00000000, 0x00000007, 0x00000008, 0x00000000,
    0x00000007, 0x0000000c, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x0000000c, 0x00000000,
    0x00000004, 0x00000000, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x0000000c, 0x00000000,
    0x00000004, 0x00000001, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x0000000c, 0x00000000,
    0x00000004, 0x00000002, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x0000000c, 0x00000000,
    0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000,
    0x00000001, 0x00000002, 0x00000000, 0x00313270, 0x0000024c, 0x43425844, 0x2c53dbe2, 0x46a9f629,
    0xb72e1a1a, 0xf1302ff3, 0x00000001, 0x0000024c, 0x00000003, 0x0000002c, 0x000000a8, 0x000000c4,
    0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100,
    0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xab003472,
    0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820,
    0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43,
    0x00000014, 0x00000004, 0x3f800000, 0x40000000, 0x00000000, 0x00000000, 0x434c5846, 0x00000180,
    0x00000007, 0x21500001, 0x00000002, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002,
    0x00000002, 0x00000000, 0x00000007, 0x00000000, 0x30100001, 0x00000003, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000004, 0x21400001, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000002, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x30100001, 0x00000003, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00000007, 0x00000004,
    0x00000000, 0x00000004, 0x00000000, 0x30100001, 0x00000003, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00000007, 0x00000004, 0x00000000, 0x00000004,
    0x00000001, 0x30100001, 0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000001,
    0x00000002, 0x00000000, 0x00000007, 0x00000004, 0x00000000, 0x00000004, 0x00000002, 0x30100001,
    0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000000,
    0x00000007, 0x00000004, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff,
    0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00323270, 0x000000ec,
    0x43425844, 0xc95f99d5, 0x429f3bc7, 0xe63b88c0, 0x6af83630, 0x00000001, 0x000000ec, 0x00000003,
    0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400,
    0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038,
    0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874,
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
    0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000030, 0x00000001,
    0x13a00004, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000000,
    0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002,
    0x00000000, 0x00333270, 0x000000ec, 0x43425844, 0x522b9c04, 0xb45214ce, 0x80b49b27, 0x3c625e1f,
    0x00000001, 0x000000ec, 0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074,
    0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030,
    0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001,
    0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c,
    0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000,
    0x434c5846, 0x00000030, 0x00000001, 0x13900004, 0x00000001, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000004, 0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002,
    0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00343270, 0x000001a4, 0x43425844, 0x2d57898e,
    0xcbaa523b, 0xa6a27e6f, 0xb91c2543, 0x00000001, 0x000001a4, 0x00000003, 0x0000002c, 0x000000d4,
    0x000000e0, 0x42415443, 0x000000a0, 0x0000001c, 0x00000077, 0x46580400, 0x00000002, 0x0000001c,
    0x00000100, 0x00000074, 0x00000044, 0x00000002, 0x00000001, 0x0000004c, 0x00000000, 0x0000005c,
    0x00010002, 0x00000001, 0x00000064, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001,
    0x00000001, 0x00000000, 0x61765f67, 0xab003272, 0x00030001, 0x00040001, 0x00000001, 0x00000000,
    0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320,
    0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x000000bc,
    0x00000005, 0x50000004, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002,
    0x00000004, 0x00000000, 0x00000007, 0x00000000, 0x10000001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x10000001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x10000001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x10000001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001,
    0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00353270, 0x000001a4, 0x43425844,
    0x290c4bc4, 0x2befa5b6, 0x3846720d, 0x91ce4617, 0x00000001, 0x000001a4, 0x00000003, 0x0000002c,
    0x000000d4, 0x000000e0, 0x42415443, 0x000000a0, 0x0000001c, 0x00000077, 0x46580400, 0x00000002,
    0x0000001c, 0x00000100, 0x00000074, 0x00000044, 0x00000002, 0x00000001, 0x0000004c, 0x00000000,
    0x0000005c, 0x00010002, 0x00000001, 0x00000064, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001,
    0x00040001, 0x00000001, 0x00000000, 0x61765f67, 0xab003272, 0x00030001, 0x00040001, 0x00000001,
    0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
    0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846,
    0x000000bc, 0x00000005, 0x50000003, 0x00000002, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
    0x00000002, 0x00000004, 0x00000000, 0x00000007, 0x00000000, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff,
    0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00363270, 0x000001ec,
    0x43425844, 0xd1c3fbc0, 0xc234ebce, 0x0829e8d7, 0x81c3fc5c, 0x00000001, 0x000001ec, 0x00000003,
    0x0000002c, 0x000000d4, 0x000000e0, 0x42415443, 0x000000a0, 0x0000001c, 0x00000077, 0x46580400,
    0x00000002, 0x0000001c, 0x00000100, 0x00000074, 0x00000044, 0x00000002, 0x00000001, 0x0000004c,
    0x00000000, 0x0000005c, 0x00010002, 0x00000001, 0x00000064, 0x00000000, 0x61765f67, 0xabab0072,
    0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x61765f67, 0xab003272, 0x00030001, 0x00040001,
    0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c,
    0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000,
    0x434c5846, 0x00000104, 0x00000005, 0x70e00001, 0x00000008, 0x00000000, 0x00000002, 0x00000003,
    0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000002, 0x00000004, 0x00000000, 0x00000002, 0x00000005, 0x00000000,
    0x00000002, 0x00000006, 0x00000000, 0x00000002, 0x00000007, 0x00000000, 0x00000007, 0x00000000,
    0x10000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000,
    0x10000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000001,
    0x10000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000002,
    0x10000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000003,
    0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002,
    0x00000000, 0x00373270, 0x000001d4, 0x43425844, 0x8d6a0e48, 0x9e27e0a9, 0x0f3eba37, 0x93932040,
    0x00000001, 0x000001d4, 0x00000003, 0x0000002c, 0x000000d4, 0x000000e0, 0x42415443, 0x000000a0,
    0x0000001c, 0x00000077, 0x46580400, 0x00000002, 0x0000001c, 0x00000100, 0x00000074, 0x00000044,
    0x00000002, 0x00000001, 0x0000004c, 0x00000000, 0x0000005c, 0x00010002, 0x00000001, 0x00000064,
    0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x61765f67,
    0xab003272, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x000000ec, 0x00000005, 0x70e00001, 0x00000006,
    0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002,
    0x00000000, 0x00000000, 0x00000002, 0x00000005, 0x00000000, 0x00000002, 0x00000006, 0x00000000,
    0x00000002, 0x00000007, 0x00000000, 0x00000007, 0x00000000, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x10000001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff,
    0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00383270, 0x00000178,
    0x43425844, 0x40465071, 0x33ca1cb9, 0x8d1d7192, 0xedd86abd, 0x00000001, 0x00000178, 0x00000003,
    0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400,
    0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038,
    0x00000000, 0x61765f67, 0xab003372, 0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874,
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
    0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x000000bc, 0x00000005,
    0x21900001, 0x00000002, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000007, 0x00000000, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000000, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000001, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000002, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002,
    0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00393270, 0x000000ec, 0x43425844, 0xc5784377,
    0x16c9ce18, 0x67a07021, 0xb9eaebe9, 0x00000001, 0x000000ec, 0x00000003, 0x0000002c, 0x000000a8,
    0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c,
    0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67,
    0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000030, 0x00000001, 0x11200004, 0x00000001,
    0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0xf0f0f0f0, 0x0f0f0f0f,
    0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00303370,
    0x0000014c, 0x43425844, 0x36d77f1e, 0xfb12f04e, 0x9c616c93, 0x0ab9004d, 0x00000001, 0x0000014c,
    0x00000003, 0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b,
    0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001,
    0x00000038, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000,
    0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320,
    0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000090,
    0x00000004, 0x10700001, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000004,
    0x00000000, 0x10700001, 0x00000001, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000004,
    0x00000001, 0x10700001, 0x00000001, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000004,
    0x00000002, 0x10700001, 0x00000001, 0x00000000, 0x00000002, 0x00000003, 0x00000000, 0x00000004,
    0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001,
    0x00000002, 0x00000000, 0x00313370, 0x00000188, 0x43425844, 0xdadb15b4, 0x2d2e9c74, 0x5c105708,
    0x9218f7c5, 0x00000001, 0x00000188, 0x00000003, 0x0000002c, 0x000000a8, 0x000000c4, 0x42415443,
    0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c, 0x00000100, 0x00000048,
    0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67, 0xabab0072, 0x00030001,
    0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
    0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x34494c43, 0x00000014,
    0x00000004, 0x3f317218, 0x00000000, 0x00000000, 0x00000000, 0x434c5846, 0x000000bc, 0x00000005,
    0x10600001, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000000,
    0x10600001, 0x00000001, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000007, 0x00000001,
    0x10600001, 0x00000001, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000007, 0x00000002,
    0x10600001, 0x00000001, 0x00000000, 0x00000002, 0x00000003, 0x00000000, 0x00000007, 0x00000003,
    0xa0500004, 0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000004, 0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002,
    0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00323370, 0x00000188, 0x43425844, 0xcc3b2c7b,
    0x0ab3141b, 0xea88d5b8, 0x99b314cc, 0x00000001, 0x00000188, 0x00000003, 0x0000002c, 0x000000a8,
    0x000000c4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001, 0x0000001c,
    0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000, 0x61765f67,
    0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x34494c43, 0x00000014, 0x00000004, 0x3fb8aa3b, 0x00000000, 0x00000000, 0x00000000, 0x434c5846,
    0x000000bc, 0x00000005, 0xa0500004, 0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
    0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x10500001, 0x00000001, 0x00000000,
    0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x10500001, 0x00000001, 0x00000000,
    0x00000007, 0x00000001, 0x00000000, 0x00000004, 0x00000001, 0x10500001, 0x00000001, 0x00000000,
    0x00000007, 0x00000002, 0x00000000, 0x00000004, 0x00000002, 0x10500001, 0x00000001, 0x00000000,
    0x00000007, 0x00000003, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff,
    0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00333370, 0x0000010c,
    0x43425844, 0x13228b05, 0x212f3cde, 0x9538c252, 0xedacc5f2, 0x00000001, 0x0000010c, 0x00000003,
    0x0000002c, 0x000000a8, 0x000000b4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400,
    0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038,
    0x00000000, 0x61765f67, 0xab003372, 0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874,
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
    0x30312072, 0xab00312e, 0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x00000050, 0x00000002,
    0x12100004, 0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000007, 0x00000000,
    0x13000004, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000000,
    0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002,
    0x00000000, 0x00343370, 0x00000144, 0x43425844, 0x74c24633, 0x7c8b5796, 0x94f5cf48, 0x026f6022,
    0x00000001, 0x00000144, 0x00000003, 0x0000002c, 0x000000d4, 0x000000e0, 0x42415443, 0x000000a0,
    0x0000001c, 0x00000077, 0x46580400, 0x00000002, 0x0000001c, 0x00000100, 0x00000074, 0x00000044,
    0x00000002, 0x00000001, 0x0000004c, 0x00000000, 0x0000005c, 0x00010002, 0x00000001, 0x00000064,
    0x00000000, 0x61765f67, 0xab003372, 0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x61765f67,
    0xab003472, 0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369, 0x74666f73,
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e,
    0x34494c43, 0x00000004, 0x00000000, 0x434c5846, 0x0000005c, 0x00000002, 0x23100004, 0x00000002,
    0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000004, 0x00000000, 0x00000007,
    0x00000000, 0x13100004, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004,
    0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001,
    0x00000002, 0x00000000, 0x00353370, 0x0000020c, 0x43425844, 0xbc818fd3, 0x08f516ed, 0x45b19da0,
    0x6741e36e, 0x00000001, 0x0000020c, 0x00000003, 0x0000002c, 0x000000d4, 0x000000f0, 0x42415443,
    0x000000a0, 0x0000001c, 0x00000077, 0x46580400, 0x00000002, 0x0000001c, 0x00000100, 0x00000074,
    0x00000044, 0x00000002, 0x00000001, 0x0000004c, 0x00000000, 0x0000005c, 0x00010002, 0x00000001,
    0x00000064, 0x00000000, 0x61765f67, 0xab003372, 0x00020001, 0x00040001, 0x00000001, 0x00000000,
    0x61765f67, 0xab003472, 0x00020001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369,
    0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072,
    0xab00312e, 0x34494c43, 0x00000014, 0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x434c5846, 0x00000114, 0x00000007, 0x23600001, 0x00000002, 0x00000000, 0x00000002, 0x00000004,
    0x00000000, 0x00000002, 0x00000005, 0x00000000, 0x00000007, 0x00000000, 0x13100001, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x23500001, 0x00000002,
    0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x13000001, 0x00000001, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000004,
    0x00000000, 0x23400001, 0x00000002, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000002,
    0x00000003, 0x00000000, 0x00000007, 0x00000000, 0x13000001, 0x00000001, 0x00000000, 0x00000007,
    0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x10000001, 0x00000001, 0x00000000, 0x00000001,
    0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff, 0x00000001,
    0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00363370, 0x00000188, 0x43425844,
    0x0fa50e48, 0x77568ea3, 0x5a5f1569, 0x9a0b8508, 0x00000001, 0x00000188, 0x00000003, 0x0000002c,
    0x000000a8, 0x000000c4, 0x42415443, 0x00000074, 0x0000001c, 0x0000004b, 0x46580400, 0x00000001,
    0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00000002, 0x00000001, 0x00000038, 0x00000000,
    0x61765f67, 0xabab0072, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x4d007874, 0x6f726369,
    0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072,
    0xab00312e, 0x34494c43, 0x00000014, 0x00000004, 0x3e9a209b, 0x00000000, 0x00000000, 0x00000000,
    0x434c5846, 0x000000bc, 0x00000005, 0x10600001, 0x00000001, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000004, 0x00000000, 0x10600001, 0x00000001, 0x00000000, 0x00000002, 0x00000001,
    0x00000000, 0x00000007, 0x00000000, 0x20500001, 0x00000002, 0x00000000, 0x00000007, 0x00000000,
    0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x10500001, 0x00000001,
    0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x00000004, 0x00000002, 0x10000001, 0x00000001,
    0x00000000, 0x00000001, 0x00000001, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f,
    0x0000ffff, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000, 0x00000004,
    0x00000040, 0x00000000, 0x00000004, 0xffffffff, 0x00000000, 0x00000030, 0x00000014, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000036, 0x00000014, 0x00000000, 0x00000010,
    0x00000000, 0x00000000, 0x00000000, 0x0000005e, 0x00000042, 0x00000000, 0x00000020, 0x00000000,
    0x00000000, 0x00000000, 0x00000087, 0x0000006b, 0x00000000, 0x00000030, 0x00000000, 0x00000000,
    0x00000000, 0x000000bc, 0x000000a0, 0x00000000, 0xffffffff, 0x00000001, 0x00000019, 0x00000000,
    0x00000006, 0x000000c5, 0x00000000, 0x000001b5, 0x00000025, 0x00000000, 0x000001ba, 0x00000003,
    0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x000001bd, 0x0000000b, 0x00000000, 0x00000001,
    0x00000405, 0x00000002, 0x00000000, 0x00000001, 0x00000411, 0x0000041d, 0x00000003, 0x00000000,
    0x0000000a, 0x00000000, 0x00000006, 0x00000420, 0x0000000b, 0x00000000, 0x00000001, 0x00000574,
    0x00000002, 0x00000000, 0x00000001, 0x00000580, 0x0000058c, 0x00000003, 0x00000000, 0x0000000a,
    0x00000000, 0x00000006, 0x0000058f, 0x0000000b, 0x00000000, 0x00000001, 0x000007bf, 0x00000002,
    0x00000000, 0x00000001, 0x000007cb, 0x000007d7, 0x00000003, 0x00000000, 0x0000000a, 0x00000000,
    0x00000006, 0x000007da, 0x0000000b, 0x00000000, 0x00000001, 0x00000902, 0x00000002, 0x00000000,
    0x00000001, 0x0000090e, 0x0000091a, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006,
    0x0000091d, 0x0000000b, 0x00000000, 0x00000001, 0x00000a45, 0x00000002, 0x00000000, 0x00000001,
    0x00000a51, 0x00000a5d, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00000a60,
    0x0000000b, 0x00000000, 0x00000001, 0x00000b5c, 0x00000002, 0x00000000, 0x00000001, 0x00000b68,
    0x00000b74, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00000b77, 0x0000000b,
    0x00000000, 0x00000001, 0x00000cc7, 0x00000002, 0x00000000, 0x00000001, 0x00000cd3, 0x00000cdf,
    0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00000ce2, 0x0000000b, 0x00000000,
    0x00000001, 0x00000dd2, 0x00000002, 0x00000000, 0x00000001, 0x00000dde, 0x00000dea, 0x00000003,
    0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00000ded, 0x0000000b, 0x00000000, 0x00000001,
    0x00000fe9, 0x00000002, 0x00000000, 0x00000001, 0x00000ff5, 0x00001001, 0x00000003, 0x00000000,
    0x0000000a, 0x00000000, 0x00000006, 0x00001004, 0x0000000b, 0x00000000, 0x00000001, 0x00001180,
    0x00000002, 0x00000000, 0x00000001, 0x0000118c, 0x00001198, 0x00000003, 0x00000000, 0x0000000a,
    0x00000000, 0x00000006, 0x0000119c, 0x0000000b, 0x00000000, 0x00000001, 0x0000136c, 0x00000002,
    0x00000000, 0x00000001, 0x00001378, 0x00001384, 0x00000003, 0x00000000, 0x0000000a, 0x00000000,
    0x00000006, 0x00001388, 0x0000000b, 0x00000000, 0x00000001, 0x00001504, 0x00000002, 0x00000000,
    0x00000001, 0x00001510, 0x0000151c, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006,
    0x00001520, 0x0000000b, 0x00000000, 0x00000001, 0x0000169c, 0x00000002, 0x00000000, 0x00000001,
    0x000016a8, 0x000016b4, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x000016b8,
    0x0000000b, 0x00000000, 0x00000001, 0x00001834, 0x00000002, 0x00000000, 0x00000001, 0x00001840,
    0x0000184c, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00001850, 0x0000000b,
    0x00000000, 0x00000001, 0x000019cc, 0x00000002, 0x00000000, 0x00000001, 0x000019d8, 0x000019e4,
    0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x000019e8, 0x0000000b, 0x00000000,
    0x00000001, 0x00001b38, 0x00000002, 0x00000000, 0x00000001, 0x00001b44, 0x00001b50, 0x00000003,
    0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00001b54, 0x0000000b, 0x00000000, 0x00000001,
    0x00001ca4, 0x00000002, 0x00000000, 0x00000001, 0x00001cb0, 0x00001cbc, 0x00000003, 0x00000000,
    0x0000000a, 0x00000000, 0x00000006, 0x00001cc0, 0x0000000b, 0x00000000, 0x00000001, 0x00001e10,
    0x00000002, 0x00000000, 0x00000001, 0x00001e1c, 0x00001e28, 0x00000003, 0x00000000, 0x0000000a,
    0x00000000, 0x00000006, 0x00001e2c, 0x0000000b, 0x00000000, 0x00000001, 0x00001fac, 0x00000002,
    0x00000000, 0x00000001, 0x00001fb8, 0x00001fc4, 0x00000003, 0x00000000, 0x0000000a, 0x00000000,
    0x00000006, 0x00001fc8, 0x0000000b, 0x00000000, 0x00000001, 0x0000229c, 0x00000002, 0x00000000,
    0x00000001, 0x000022a8, 0x000022b4, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006,
    0x000022b8, 0x0000000b, 0x00000000, 0x00000001, 0x0000259c, 0x00000002, 0x00000000, 0x00000001,
    0x000025a8, 0x000025b4, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x000025b8,
    0x0000000b, 0x00000000, 0x00000001, 0x00002808, 0x00000002, 0x00000000, 0x00000001, 0x00002814,
    0x00002820, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00002824, 0x0000000b,
    0x00000000, 0x00000001, 0x00002914, 0x00000002, 0x00000000, 0x00000001, 0x00002920, 0x0000292c,
    0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00002930, 0x0000000b, 0x00000000,
    0x00000001, 0x00002a20, 0x00000002, 0x00000000, 0x00000001, 0x00002a2c, 0x00002a38, 0x00000003,
    0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00002a3c, 0x0000000b, 0x00000000, 0x00000001,
    0x00002be4, 0x00000002, 0x00000000, 0x00000001, 0x00002bf0, 0x00002bfc, 0x00000003, 0x00000000,
    0x0000000a, 0x00000000, 0x00000006, 0x00002c00, 0x0000000b, 0x00000000, 0x00000001, 0x00002da8,
    0x00000002, 0x00000000, 0x00000001, 0x00002db4, 0x00002dc0, 0x00000003, 0x00000000, 0x0000000a,
    0x00000000, 0x00000006, 0x00002dc4, 0x0000000b, 0x00000000, 0x00000001, 0x00002fb4, 0x00000002,
    0x00000000, 0x00000001, 0x00002fc0, 0x00002fcc, 0x00000003, 0x00000000, 0x0000000a, 0x00000000,
    0x00000006, 0x00002fd0, 0x0000000b, 0x00000000, 0x00000001, 0x000031a8, 0x00000002, 0x00000000,
    0x00000001, 0x000031b4, 0x000031c0, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006,
    0x000031c4, 0x0000000b, 0x00000000, 0x00000001, 0x00003340, 0x00000002, 0x00000000, 0x00000001,
    0x0000334c, 0x00003358, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x0000335c,
    0x0000000b, 0x00000000, 0x00000001, 0x0000344c, 0x00000002, 0x00000000, 0x00000001, 0x00003458,
    0x00003464, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x00003468, 0x0000000b,
    0x00000000, 0x00000001, 0x000035b8, 0x00000002, 0x00000000, 0x00000001, 0x000035c4, 0x000035d0,
    0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x000035d4, 0x0000000b, 0x00000000,
    0x00000001, 0x00003760, 0x00000002, 0x00000000, 0x00000001, 0x0000376c, 0x00003778, 0x00000003,
    0x00000000, 0x0000000a, 0x00000000, 0x00000006, 0x0000377c, 0x0000000b, 0x00000000, 0x00000001,
    0x00003908, 0x00000002, 0x00000000, 0x00000001, 0x00003914, 0x00003920, 0x00000003, 0x00000000,
    0x0000000a, 0x00000000, 0x00000006, 0x00003924, 0x0000000b, 0x00000000, 0x00000001, 0x00003a34,
    0x00000002, 0x00000000, 0x00000001, 0x00003a40, 0x00003a4c, 0x00000003, 0x00000000, 0x0000000a,
    0x00000000, 0x00000006, 0x00003a50, 0x0000000b, 0x00000000, 0x00000001, 0x00003b98, 0x00000002,
    0x00000000, 0x00000001, 0x00003ba4, 0x00003bb0, 0x00000003, 0x00000000, 0x0000000a, 0x00000000,
    0x00000006, 0x00003bb4, 0x0000000b, 0x00000000, 0x00000001, 0x00003dc4, 0x00000002, 0x00000000,
    0x00000001, 0x00003dd0, 0x00003ddc, 0x00000003, 0x00000000, 0x0000000a, 0x00000000, 0x00000006,
    0x00003de0, 0x0000000b, 0x00000000, 0x00000001, 0x00003f6c, 0x00000002, 0x00000000, 0x00000001,
    0x00003f78,
};

static float get_frc(float v)
{
    return v - floor(v);
}

static void test_effect_value_expression(void)
{
    ID3D10EffectVectorVariable *g_var, *g_var2, *g_var3, *g_var4;
    ID3D10EffectDepthStencilVariable *ds;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    ID3D10BlendState *blend_state;
    float f[4], blend_factor[4];
    ID3D10EffectTechnique *t;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    ID3D10Effect *effect;
    ID3D10Device *device;
    UINT sample_mask;
    unsigned int idx;
    ULONG refcount;
    HRESULT hr;
    int i[4];

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_value_expression, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    t = effect->lpVtbl->GetTechniqueByName(effect, "tech");
    ok(t->lpVtbl->IsValid(t), "Expected valid technique.\n");

    v = effect->lpVtbl->GetVariableByName(effect, "g_var");
    g_var = v->lpVtbl->AsVector(v);
    ok(g_var->lpVtbl->IsValid(g_var), "Expected valid vector variable.\n");
    v = effect->lpVtbl->GetVariableByName(effect, "g_var2");
    g_var2 = v->lpVtbl->AsVector(v);
    ok(g_var2->lpVtbl->IsValid(g_var2), "Expected valid vector variable.\n");
    v = effect->lpVtbl->GetVariableByName(effect, "g_var3");
    g_var3 = v->lpVtbl->AsVector(v);
    ok(g_var3->lpVtbl->IsValid(g_var3), "Expected valid vector variable.\n");
    v = effect->lpVtbl->GetVariableByName(effect, "g_var4");
    g_var4 = v->lpVtbl->AsVector(v);
    ok(g_var4->lpVtbl->IsValid(g_var4), "Expected valid vector variable.\n");

    /* min/max */
    pass = t->lpVtbl->GetPassByName(t, "p3");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 0.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    set_vec4(f, 2.0f, 1.1f, 0.1f, -3.0f);
    hr = g_var2->lpVtbl->SetFloatVector(g_var2, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 1.0f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 1.0f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 0.1f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == -3.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    pass = t->lpVtbl->GetPassByName(t, "p4");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 2.0f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 1.1f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 1.0f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 1.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* div */
    pass = t->lpVtbl->GetPassByName(t, "p5");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 1.0f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 1.0f / 2.0f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 1.0f / 3.0f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 1.0f / 4.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* rcp */
    pass = t->lpVtbl->GetPassByName(t, "p6");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 2.0f, 3.0f, 4.0f, 5.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 1.0f / 2.0f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 1.0f / 3.0f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 1.0f / 4.0f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 1.0f / 5.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* frc */
    pass = t->lpVtbl->GetPassByName(t, "p7");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 0.0f, 3.1f, -4.2f, 0.1f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == get_frc(f[idx]), "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* Mutable state objects. */
    v = effect->lpVtbl->GetVariableByName(effect, "ds_state");
    ds = v->lpVtbl->AsDepthStencil(v);

    set_vec4(f, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ds->lpVtbl->GetBackingStore(ds, 0, &ds_desc);
    ok(ds_desc.StencilEnable == 0xffffffff, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);

    set_vec4(f, 0.0f, 2.0f, 3.0f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ds->lpVtbl->GetBackingStore(ds, 0, &ds_desc);
    ok(!ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);

    set_vec4(f, -0.1f, 2.0f, 3.0f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ds->lpVtbl->GetBackingStore(ds, 0, &ds_desc);
    ok(ds_desc.StencilEnable == 0xffffffff, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);

    set_vec4(f, 0.0f, 2.0f, 3.0f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ds->lpVtbl->GetBackingStore(ds, 0, &ds_desc);
    ok(!ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);

    set_vec4(f, NAN, 2.0f, 3.0f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ds->lpVtbl->GetBackingStore(ds, 0, &ds_desc);
    ok(ds_desc.StencilEnable == 0xffffffff, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);

    /* itof */
    pass = t->lpVtbl->GetPassByName(t, "p8");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, -4.0f, -3.0f, 3.0f, 4.0f);
    hr = g_var3->lpVtbl->SetFloatVector(g_var3, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 0.3f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* udiv */
    pass = t->lpVtbl->GetPassByName(t, "p9");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 2.0f, 0.0f, 3.0f, 4.0f);
    hr = g_var3->lpVtbl->SetFloatVector(g_var3, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == (float)UINT_MAX, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* movc */
    pass = t->lpVtbl->GetPassByName(t, "p10");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 0.0f, 1.1f, 2.2f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 2.2f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, 0.1f, 0.1f, 0.2f, 4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 0.1f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* umin/umax */
    pass = t->lpVtbl->GetPassByName(t, "p11");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 3, 2, 0, 0);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 2.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    pass = t->lpVtbl->GetPassByName(t, "p12");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 2, 5, 0, 0);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 5.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* imin */
    pass = t->lpVtbl->GetPassByName(t, "p13");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 3, 5, 0, 0);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 3.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* iadd */
    pass = t->lpVtbl->GetPassByName(t, "p14");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 3, 5, 0, 0);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 8.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* asin */
    pass = t->lpVtbl->GetPassByName(t, "p15");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 0.1234567f, 0.0f, 0.0f, 0.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(blend_factor[0] == asinf(f[0]), "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);

    /* acos */
    pass = t->lpVtbl->GetPassByName(t, "p16");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 0.1234567f, 0.0f, 0.0f, 0.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(blend_factor[0] == acosf(f[0]), "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);

    /* atan */
    pass = t->lpVtbl->GetPassByName(t, "p17");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 0.1234567f, 0.0f, 0.0f, 0.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(blend_factor[0] == atanf(f[0]), "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);

    /* atan2 */
    pass = t->lpVtbl->GetPassByName(t, "p18");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 0.1234567f, 0.7654321f, 0.0f, 0.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(blend_factor[0] == atan2f(f[0], f[1]), "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);

    /* Integer division */
    pass = t->lpVtbl->GetPassByName(t, "p19");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 10, 5, 0, 0);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 2.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* Signed integer comparison. */
    pass = t->lpVtbl->GetPassByName(t, "p20");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 1, 5, 6, 5);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 1.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_int4(i, 2, 1, 2, 5);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 2.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* Unsigned integer comparison. */
    pass = t->lpVtbl->GetPassByName(t, "p21");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 6, 5, 7, 1);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 0.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_int4(i, 2, 5, 7, 1);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 1.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* ceil() */
    pass = t->lpVtbl->GetPassByName(t, "p22");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 1.1f, -2.3f, 3.5f, -4.5f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 2.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, -2.3f, 1.1f, 3.5f, -4.5f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -2.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, 3.5f, -2.3f, 1.1f, -4.5f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 4.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, -4.5f, -2.3f, 3.5f, 1.1f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -4.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* floor() */
    pass = t->lpVtbl->GetPassByName(t, "p23");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 1.1f, -2.3f, 3.5f, -4.5f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 1.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, -2.3f, 1.1f, 3.5f, -4.5f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -3.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, 3.5f, -2.3f, 1.1f, -4.5f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 3.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_vec4(f, -4.5f, -2.3f, 3.5f, 1.1f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -5.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* dot(), 4-component vectors */
    pass = t->lpVtbl->GetPassByName(t, "p24");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 1.0f, -2.0f, 3.0f, -4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    set_vec4(f, 5.0f, 6.0f, -7.0f, 8.0f);
    hr = g_var2->lpVtbl->SetFloatVector(g_var2, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -60.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* dot(), 3-component vectors */
    pass = t->lpVtbl->GetPassByName(t, "p25");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 1.0f, -2.0f, 3.0f, -4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    set_vec4(f, 5.0f, 6.0f, -7.0f, 8.0f);
    hr = g_var2->lpVtbl->SetFloatVector(g_var2, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -28.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* dot(), 4-component vectors with swizzles */
    pass = t->lpVtbl->GetPassByName(t, "p26");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 1.0f, -2.0f, 3.0f, -4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    set_vec4(f, 5.0f, 6.0f, -7.0f, 8.0f);
    hr = g_var2->lpVtbl->SetFloatVector(g_var2, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 20.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* dot(), 3-component vectors with swizzles */
    pass = t->lpVtbl->GetPassByName(t, "p27");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 1.0f, -2.0f, 3.0f, -4.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    set_vec4(f, 5.0f, 6.0f, -7.0f, 8.0f);
    hr = g_var2->lpVtbl->SetFloatVector(g_var2, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 40.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* Signed integer multiplication */
    pass = t->lpVtbl->GetPassByName(t, "p28");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 3, -2, 5, -4);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -6.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* sqrt() */
    pass = t->lpVtbl->GetPassByName(t, "p29");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 4.0f, 0.0f, 1.0f, 64.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 2.0f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 0.0f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 1.0f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 8.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* rsqrt() */
    pass = t->lpVtbl->GetPassByName(t, "p30");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 4.0f, 9.0f, 1.0f, 64.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 0.5f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 1.0f / 3.0f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 1.0f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.125f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* log() */
    pass = t->lpVtbl->GetPassByName(t, "p31");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 4.0f, 9.0f, 0.0f, -2.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == logf(fabsf(f[0])), "Got unexpected blend_factor %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == logf(fabsf(f[1])), "Got unexpected blend_factor %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 0.0f, "Got unexpected blend_factor %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == logf(fabsf(f[3])), "Got unexpected blend_factor %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* exp() */
    pass = t->lpVtbl->GetPassByName(t, "p32");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 4.0f, 1.0f, 0.0f, -2.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == expf(f[idx]), "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* binary complement */
    pass = t->lpVtbl->GetPassByName(t, "p33");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 0, 1, 2, 3);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -1.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_int4(i, 1, 0, 2, 3);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == -2.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* bitwise 'or' */
    pass = t->lpVtbl->GetPassByName(t, "p34");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 1, 2, 3, 4);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_int4(i, 0x10, 0x20, 0x30, 0x40);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 17.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_int4(i, 0x20, 0x10, 0x30, 0x40);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    for (idx = 0; idx < ARRAY_SIZE(blend_factor); ++idx)
        ok(blend_factor[idx] == 33.0f, "Got unexpected blend_factor[%u] %.8e.\n", idx, blend_factor[idx]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* Shifts */
    pass = t->lpVtbl->GetPassByName(t, "p35");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_int4(i, 10, 1, 20, 2);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_int4(i, 10, 2, 0, 0);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 5.0f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 80.0f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 2.0f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    set_int4(i, 40, 35, 20, 37);
    hr = g_var3->lpVtbl->SetIntVector(g_var3, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_int4(i, 10, 66, 0, 0);
    hr = g_var4->lpVtbl->SetIntVector(g_var4, i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == 5.0f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 640.0f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 2.0f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    /* log2(), log10(), exp2() */
    pass = t->lpVtbl->GetPassByName(t, "p36");
    ok(pass->lpVtbl->IsValid(pass), "Expected valid pass.\n");

    set_vec4(f, 4.0f, 9.0f, 3.0f, 0.0f);
    hr = g_var->lpVtbl->SetFloatVector(g_var, f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ok(!blend_state, "Unexpected blend state %p.\n", blend_state);
    ok(blend_factor[0] == logf(f[0]) / logf(2.0f), "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == log10f(f[1]), "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == exp2f(f[2]), "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.0f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(!sample_mask, "Got unexpected sample_mask %#x.\n", sample_mask);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_effect_fx_4_1(void)
{
    static const char source[] =
        "technique10 tech0\n"
        "{\n"
        "   pass pass0 {}\n"
        "};";
    ID3D10Effect *effect;
    ID3D10Device *device;
    ID3D10Blob *blob;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = D3DCompile(source, sizeof(source), NULL, NULL, NULL, "main", "fx_4_1", 0, 0, &blob, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = create_effect(ID3D10Blob_GetBufferPointer(blob), 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    effect->lpVtbl->Release(effect);

    ID3D10Blob_Release(blob);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

#if 0
BlendState blend_state
{
    srcblend = one;
};
#endif
static DWORD fx_4_1_test_blend_state[] =
{
    0x43425844, 0xe4566da7, 0x2242fb47, 0xa5924d09, 0x8280296f, 0x00000001, 0x000001a7, 0x00000001,
    0x00000024, 0x30315846, 0x0000017b, 0xfeff1011, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000097, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6e656c42,
    0x61745364, 0x04006574, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02000000,
    0x62000000, 0x646e656c, 0x6174735f, 0x01006574, 0x02000000, 0x02000000, 0x01000000, 0x02000000,
    0x02000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000,
    0x02000000, 0x02000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000, 0x02000000, 0x02000000,
    0x01000000, 0x02000000, 0x02000000, 0x2b000000, 0x0f000000, 0x00000000, 0xff000000, 0x08ffffff,
    0x26000000, 0x00000000, 0x01000000, 0x37000000, 0x26000000, 0x01000000, 0x01000000, 0x43000000,
    0x26000000, 0x02000000, 0x01000000, 0x4f000000, 0x26000000, 0x03000000, 0x01000000, 0x5b000000,
    0x26000000, 0x04000000, 0x01000000, 0x67000000, 0x26000000, 0x05000000, 0x01000000, 0x73000000,
    0x26000000, 0x06000000, 0x01000000, 0x7f000000, 0x26000000, 0x07000000, 0x01000000, 0x8b000000,
    0x00000000, 0x00000000,
};

static void test_effect_fx_4_1_blend_state(void)
{
    ID3D10Effect *effect = NULL;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_4_1_test_blend_state, 0, device, NULL, &effect);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_effect_compiler(void)
{
    static char empty_effect[] =
        "technique10 {};";
    static char empty_buffer[] =
        "cbuffer cb1 { float4 m1; }\n"
        "cbuffer cb2 { }\n"
        "technique10 {};";

    D3D10_EFFECT_VARIABLE_DESC var_desc;
    ID3D10EffectConstantBuffer *cb;
    D3D10_EFFECT_DESC desc;
    ID3D10Device *device;
    ID3D10Effect *effect;
    ID3D10Blob *blob;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = D3D10CompileEffectFromMemory(empty_effect, sizeof(empty_effect), NULL, NULL, NULL, 0, 0,
            &blob, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = create_effect(ID3D10Blob_GetBufferPointer(blob), 0, device, NULL, &effect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3D10Effect_GetDesc(effect, &desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(desc.Techniques == 1, "Unexpected technique count %u.\n", desc.Techniques);
    ok(desc.ConstantBuffers == 1, "Unexpected buffer count %u.\n", desc.ConstantBuffers);

    cb = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "$Globals"), "Unexpected variable name %s.\n", var_desc.Name);

    ID3D10Effect_Release(effect);
    ID3D10Blob_Release(blob);

    /* Empty user buffers. */
    hr = D3D10CompileEffectFromMemory(empty_buffer, sizeof(empty_buffer), NULL, NULL, NULL, 0, 0,
            &blob, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = create_effect(ID3D10Blob_GetBufferPointer(blob), 0, device, NULL, &effect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3D10Effect_GetDesc(effect, &desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(desc.Techniques == 1, "Unexpected technique count %u.\n", desc.Techniques);
    ok(desc.ConstantBuffers == 3, "Unexpected buffer count %u.\n", desc.ConstantBuffers);

    cb = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "$Globals"), "Unexpected variable name %s.\n", var_desc.Name);

    cb = effect->lpVtbl->GetConstantBufferByIndex(effect, 1);
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "cb1"), "Unexpected variable name %s.\n", var_desc.Name);

    cb = effect->lpVtbl->GetConstantBufferByIndex(effect, 2);
    hr = cb->lpVtbl->GetDesc(cb, &var_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(var_desc.Name, "cb2"), "Unexpected variable name %s.\n", var_desc.Name);

    ID3D10Effect_Release(effect);
    ID3D10Blob_Release(blob);

    ID3D10Device_Release(device);
}

START_TEST(effect)
{
    test_effect_constant_buffer_type();
    test_effect_variable_type();
    test_effect_variable_member();
    test_effect_variable_element();
    test_effect_variable_type_class();
    test_effect_constant_buffer_stride();
    test_effect_local_shader();
    test_effect_get_variable_by();
    test_effect_state_groups();
    test_effect_state_group_defaults();
    test_effect_scalar_variable();
    test_effect_vector_variable();
    test_effect_matrix_variable();
    test_effect_resource_variable();
    test_effect_annotations();
    test_effect_optimize();
    test_effect_shader_description();
    test_effect_shader_object();
    test_effect_pool();
    test_effect_default_variable_value();
    test_effect_raw_value();
    test_effect_dynamic_numeric_field();
    test_effect_index_expression();
    test_effect_value_expression();
    test_effect_fx_4_1();
    test_effect_fx_4_1_blend_state();
    test_effect_compiler();
}
