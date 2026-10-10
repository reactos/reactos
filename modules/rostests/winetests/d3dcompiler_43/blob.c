/*
 * Copyright 2010 Rico Schüller
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

/*
 * Nearly all compiler functions need the shader blob and the size. The size
 * is always located at DWORD #6 in the shader blob (blob[6]).
 * The functions are e.g.: D3DGet*SignatureBlob, D3DReflect
 */

#define COBJMACROS
#include "d3dcompiler.h"
#include "wine/test.h"

/*
 * This doesn't belong here, but for some functions it is possible to return that value,
 * see http://msdn.microsoft.com/en-us/library/bb205278%28v=VS.85%29.aspx
 * The original definition is in D3DX10core.h.
 */
#define D3DERR_INVALIDCALL 0x8876086c

#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_Aon9 MAKE_TAG('A', 'o', 'n', '9')
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_ISGN MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSGN MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_PCSG MAKE_TAG('P', 'C', 'S', 'G')
#define TAG_RDEF MAKE_TAG('R', 'D', 'E', 'F')
#define TAG_SDBG MAKE_TAG('S', 'D', 'B', 'G')
#define TAG_SHDR MAKE_TAG('S', 'H', 'D', 'R')
#define TAG_SHEX MAKE_TAG('S', 'H', 'E', 'X')
#define TAG_STAT MAKE_TAG('S', 'T', 'A', 'T')
#define TAG_XNAP MAKE_TAG('X', 'N', 'A', 'P')
#define TAG_XNAS MAKE_TAG('X', 'N', 'A', 'S')

#if D3D_COMPILER_VERSION >= 43
static void test_create_blob(void)
{
    ID3D10Blob *blob;
    HRESULT hr;
    ULONG refcount;

    hr = D3DCreateBlob(1, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DCreateBlob(0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DCreateBlob(0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
}

static const D3D_BLOB_PART parts[] =
{
   D3D_BLOB_INPUT_SIGNATURE_BLOB, D3D_BLOB_OUTPUT_SIGNATURE_BLOB, D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB,
   D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB, D3D_BLOB_ALL_SIGNATURE_BLOB, D3D_BLOB_DEBUG_INFO,
   D3D_BLOB_LEGACY_SHADER, D3D_BLOB_XNA_PREPASS_SHADER, D3D_BLOB_XNA_SHADER,
   D3D_BLOB_TEST_ALTERNATE_SHADER, D3D_BLOB_TEST_COMPILE_DETAILS, D3D_BLOB_TEST_COMPILE_PERF
};

/*
 * test_blob_part - fxc.exe /E VS /Tvs_4_0_level_9_0 /Fx
 */
#if 0
float4 VS(float4 position : POSITION, float4 pos : SV_POSITION) : SV_POSITION
{
  return position;
}
#endif
static DWORD test_blob_part[] = {
0x43425844, 0x0ef2a70f, 0x6a548011, 0x91ff9409, 0x9973a43d, 0x00000001, 0x000002e0, 0x00000008,
0x00000040, 0x0000008c, 0x000000d8, 0x0000013c, 0x00000180, 0x000001fc, 0x00000254, 0x000002ac,
0x53414e58, 0x00000044, 0x00000044, 0xfffe0200, 0x00000020, 0x00000024, 0x00240000, 0x00240000,
0x00240000, 0x00240000, 0x00240000, 0xfffe0200, 0x0200001f, 0x80000005, 0x900f0000, 0x02000001,
0xc00f0000, 0x80e40000, 0x0000ffff, 0x50414e58, 0x00000044, 0x00000044, 0xfffe0200, 0x00000020,
0x00000024, 0x00240000, 0x00240000, 0x00240000, 0x00240000, 0x00240000, 0xfffe0200, 0x0200001f,
0x80000005, 0x900f0000, 0x02000001, 0xc00f0000, 0x80e40000, 0x0000ffff, 0x396e6f41, 0x0000005c,
0x0000005c, 0xfffe0200, 0x00000034, 0x00000028, 0x00240000, 0x00240000, 0x00240000, 0x00240000,
0x00240001, 0x00000000, 0xfffe0200, 0x0200001f, 0x80000005, 0x900f0000, 0x04000004, 0xc0030000,
0x90ff0000, 0xa0e40000, 0x90e40000, 0x02000001, 0xc00c0000, 0x90e40000, 0x0000ffff, 0x52444853,
0x0000003c, 0x00010040, 0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2,
0x00000000, 0x00000001, 0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
0x54415453, 0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x46454452,
0x00000050, 0x00000000, 0x00000000, 0x00000000, 0x0000001c, 0xfffe0400, 0x00000100, 0x0000001c,
0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d,
0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000050, 0x00000002,
0x00000008, 0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041,
0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x49534f50, 0x4e4f4954, 0x5f565300,
0x49534f50, 0x4e4f4954, 0xababab00, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
};

static void test_get_blob_part(void)
{
    ID3DBlob *blob, *blob2;
    HRESULT hr, expected;
    ULONG refcount;
    DWORD *dword;
    SIZE_T size;
    UINT i;

#if D3D_COMPILER_VERSION >= 46
    expected = D3DERR_INVALIDCALL;
#else
    expected = E_FAIL;
#endif

    hr = D3DCreateBlob(1, &blob2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    blob = blob2;

    /* invalid cases */
    hr = D3DGetBlobPart(NULL, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(NULL, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(NULL, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DGetBlobPart(NULL, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DGetBlobPart(test_blob_part, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(test_blob_part, 7 * sizeof(DWORD), D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(test_blob_part, 8 * sizeof(DWORD), D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
#if D3D_COMPILER_VERSION >= 46
    todo_wine
#endif
    ok(hr == expected, "Got unexpected hr %#lx.\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DGetBlobPart(test_blob_part, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 1, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], 0xffffffff, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    refcount = ID3D10Blob_Release(blob2);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_INPUT_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 124, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_ISGN == *(dword + 9), "ISGN got %#lx, expected %#lx.\n", *(dword + 9), TAG_ISGN);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);

        if (parts[i] == D3D_BLOB_INPUT_SIGNATURE_BLOB)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            refcount = ID3D10Blob_Release(blob2);
            ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
        }
        else
        {
            ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
        }
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_OUTPUT_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_OUTPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 88, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_OSGN == *(dword + 9), "OSGN got %#lx, expected %#lx.\n", *(dword + 9), TAG_OSGN);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);

        if (parts[i] == D3D_BLOB_OUTPUT_SIGNATURE_BLOB)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            refcount = ID3D10Blob_Release(blob2);
            ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
        }
        else
        {
            ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
        }
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 180, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_ISGN == *(dword + 10), "ISGN got %#lx, expected %#lx.\n", *(dword + 10), TAG_ISGN);
    ok(TAG_OSGN == *(dword + 32), "OSGN got %#lx, expected %#lx.\n", *(dword + 32), TAG_OSGN);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);

        if (parts[i] == D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB
                || parts[i] == D3D_BLOB_INPUT_SIGNATURE_BLOB
                || parts[i] == D3D_BLOB_OUTPUT_SIGNATURE_BLOB)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            refcount = ID3D10Blob_Release(blob2);
            ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
        }
        else
        {
            ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
        }
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* D3D_BLOB_ALL_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_ALL_SIGNATURE_BLOB, 0, &blob);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* D3D_BLOB_DEBUG_INFO */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_DEBUG_INFO, 0, &blob);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* D3D_BLOB_LEGACY_SHADER */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_LEGACY_SHADER, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 92, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(test_blob_part[0] != *dword, "DXBC failed got %#lx.\n", *dword);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        /* There isn't a full DXBC blob returned for D3D_BLOB_LEGACY_SHADER */
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);
#if D3D_COMPILER_VERSION >= 46
        todo_wine
#endif
        ok(hr == expected, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected);
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_XNA_PREPASS_SHADER */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_XNA_PREPASS_SHADER, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 68, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(test_blob_part[0] != *dword, "DXBC failed got %#lx.\n", *dword);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        /* There isn't a full DXBC blob returned for D3D_BLOB_XNA_PREPASS_SHADER */
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);
#if D3D_COMPILER_VERSION >= 46
        todo_wine
#endif
        ok(hr == expected, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected);
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_XNA_SHADER */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_XNA_SHADER, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 68, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(test_blob_part[0] != *dword, "DXBC failed got %#lx.\n", *dword);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        /* There isn't a full DXBC blob returned for D3D_BLOB_XNA_SHADER */
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);
#if D3D_COMPILER_VERSION >= 46
        todo_wine
#endif
        ok(hr == expected, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected);
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* check corner cases for D3DStripShader */
    hr = D3DStripShader(test_blob_part, test_blob_part[6], 0xffffffff, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    hr = D3DStripShader(test_blob_part, test_blob_part[6], 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    hr = D3DStripShader(NULL, test_blob_part[6], 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DStripShader(test_blob_part, 7 * sizeof(DWORD), 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DStripShader(test_blob_part, 8 * sizeof(DWORD), 0, &blob);
#if D3D_COMPILER_VERSION >= 46
    todo_wine
#endif
    ok(hr == expected, "Got unexpected hr %#lx.\n", hr);

    hr = D3DStripShader(test_blob_part, test_blob_part[6], 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DStripShader(NULL, test_blob_part[6], 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DStripShader(test_blob_part, 0, 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* D3DCOMPILER_STRIP_DEBUG_INFO */
    hr = D3DStripShader(test_blob_part, test_blob_part[6], D3DCOMPILER_STRIP_DEBUG_INFO, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 736, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_XNAS == *(dword + 16), "XNAS got %#lx, expected %#lx.\n", *(dword + 16), TAG_XNAS);
    ok(TAG_XNAP == *(dword + 35), "XNAP got %#lx, expected %#lx.\n", *(dword + 35), TAG_XNAP);
    ok(TAG_Aon9 == *(dword + 54), "Aon9 got %#lx, expected %#lx.\n", *(dword + 54), TAG_Aon9);
    ok(TAG_SHDR == *(dword + 79), "SHDR got %#lx, expected %#lx.\n", *(dword + 79), TAG_SHDR);
    ok(TAG_STAT == *(dword + 96), "STAT got %#lx, expected %#lx.\n", *(dword + 96), TAG_STAT);
    ok(TAG_RDEF == *(dword + 127), "RDEF got %#lx, expected %#lx.\n", *(dword + 127), TAG_RDEF);
    ok(TAG_ISGN == *(dword + 149), "ISGN got %#lx, expected %#lx.\n", *(dword + 149), TAG_ISGN);
    ok(TAG_OSGN == *(dword + 171), "OSGN got %#lx, expected %#lx.\n", *(dword + 171), TAG_OSGN);

    hr = D3DGetBlobPart(dword, size, D3D_BLOB_DEBUG_INFO, 0, &blob2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3DCOMPILER_STRIP_REFLECTION_DATA */
    hr = D3DStripShader(test_blob_part, test_blob_part[6], D3DCOMPILER_STRIP_REFLECTION_DATA, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 516, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_XNAS == *(dword + 14), "XNAS got %#lx, expected %#lx.\n", *(dword + 14), TAG_XNAS);
    ok(TAG_XNAP == *(dword + 33), "XNAP got %#lx, expected %#lx.\n", *(dword + 33), TAG_XNAP);
    ok(TAG_Aon9 == *(dword + 52), "Aon9 got %#lx, expected %#lx.\n", *(dword + 52), TAG_Aon9);
    ok(TAG_SHDR == *(dword + 77), "SHDR got %#lx, expected %#lx.\n", *(dword + 77), TAG_SHDR);
    ok(TAG_ISGN == *(dword + 94), "ISGN got %#lx, expected %#lx.\n", *(dword + 94), TAG_ISGN);
    ok(TAG_OSGN == *(dword + 116), "OSGN got %#lx, expected %#lx.\n", *(dword + 116), TAG_OSGN);

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
}

/*
 * test_blob_part2 - fxc.exe /E BHS /Ths_5_0 /Zi
 */
#if 0
struct VSO { float3 p : POSITION; };
struct HSDO { float e[4] : SV_TessFactor; float i[2] : SV_InsideTessFactor; };
struct HSO { float3 p : BEZIERPOS; };
HSDO BCHS( InputPatch<VSO, 8> ip, uint PatchID : SV_PrimitiveID )
{
    HSDO res;
    res.e[0] = res.e[1] = res.e[2] = res.e[3] = 1.0f;
    res.i[0] = res.i[1] = 1.0f;
    return res;
}
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(8)]
[patchconstantfunc("BCHS")]
HSO BHS( InputPatch<VSO, 8> p, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID )
{
    HSO res;
    res.p = p[i].p;
    return res;
}
#endif
static DWORD test_blob_part2[] = {
0x43425844, 0xa9d455ae, 0x4cf9c0df, 0x4cf806dc, 0xc57a8c2c, 0x00000001, 0x0000139b, 0x00000007,
0x0000003c, 0x000000b4, 0x000000e8, 0x0000011c, 0x000001e0, 0x00000320, 0x000003bc, 0x46454452,
0x00000070, 0x00000000, 0x00000000, 0x00000000, 0x0000003c, 0x48530500, 0x00000101, 0x0000003c,
0x31314452, 0x0000003c, 0x00000018, 0x00000020, 0x00000028, 0x00000024, 0x0000000c, 0x00000000,
0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d,
0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x0000002c, 0x00000001,
0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000707, 0x49534f50,
0x4e4f4954, 0xababab00, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000,
0x00000000, 0x00000003, 0x00000000, 0x00000807, 0x495a4542, 0x4f505245, 0xabab0053, 0x47534350,
0x000000bc, 0x00000006, 0x00000008, 0x00000098, 0x00000000, 0x0000000b, 0x00000003, 0x00000000,
0x00000e01, 0x00000098, 0x00000001, 0x0000000b, 0x00000003, 0x00000001, 0x00000e01, 0x00000098,
0x00000002, 0x0000000b, 0x00000003, 0x00000002, 0x00000e01, 0x00000098, 0x00000003, 0x0000000b,
0x00000003, 0x00000003, 0x00000e01, 0x000000a6, 0x00000000, 0x0000000c, 0x00000003, 0x00000004,
0x00000e01, 0x000000a6, 0x00000001, 0x0000000c, 0x00000003, 0x00000005, 0x00000e01, 0x545f5653,
0x46737365, 0x6f746361, 0x56530072, 0x736e495f, 0x54656469, 0x46737365, 0x6f746361, 0xabab0072,
0x58454853, 0x00000138, 0x00030050, 0x0000004e, 0x01000071, 0x01004093, 0x01004094, 0x01001895,
0x01000896, 0x01001897, 0x0100086a, 0x01000073, 0x02000099, 0x00000004, 0x0200005f, 0x00017000,
0x04000067, 0x00102012, 0x00000000, 0x0000000b, 0x04000067, 0x00102012, 0x00000001, 0x0000000c,
0x04000067, 0x00102012, 0x00000002, 0x0000000d, 0x04000067, 0x00102012, 0x00000003, 0x0000000e,
0x02000068, 0x00000001, 0x0400005b, 0x00102012, 0x00000000, 0x00000004, 0x04000036, 0x00100012,
0x00000000, 0x0001700a, 0x06000036, 0x00902012, 0x0010000a, 0x00000000, 0x00004001, 0x3f800000,
0x0100003e, 0x01000073, 0x02000099, 0x00000002, 0x0200005f, 0x00017000, 0x04000067, 0x00102012,
0x00000004, 0x0000000f, 0x04000067, 0x00102012, 0x00000005, 0x00000010, 0x02000068, 0x00000001,
0x0400005b, 0x00102012, 0x00000004, 0x00000002, 0x04000036, 0x00100012, 0x00000000, 0x0001700a,
0x07000036, 0x00d02012, 0x00000004, 0x0010000a, 0x00000000, 0x00004001, 0x3f800000, 0x0100003e,
0x54415453, 0x00000094, 0x00000006, 0x00000001, 0x00000000, 0x00000005, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0000000f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000008, 0x00000003, 0x00000001, 0x00000003, 0x00000000, 0x00000000, 0x00000000, 0x47424453,
0x00000fd7, 0x00000054, 0x000002d5, 0x00000306, 0x0000030a, 0x00000101, 0x00000001, 0x00000000,
0x00000006, 0x00000010, 0x00000006, 0x00000958, 0x00000000, 0x000009e8, 0x00000008, 0x000009e8,
0x00000006, 0x00000a88, 0x00000007, 0x00000b00, 0x00000c34, 0x00000c64, 0x00000000, 0x0000004a,
0x0000004a, 0x0000026a, 0x00000000, 0x00000036, 0x00000001, 0x00000004, 0x00000000, 0xffffffff,
0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000003, 0x00000000,
0x00000003, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000007,
0x00000000, 0x00000003, 0x00000024, 0x00000000, 0x00000000, 0x00000001, 0x00000036, 0x00000001,
0x00000001, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000,
0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x00000003, 0x00000024, 0x00000000, 0x00000000,
0x00000002, 0x0000003e, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x00000003,
0x00000024, 0x00000000, 0x00000000, 0x00000003, 0x00000036, 0x00000001, 0x00000004, 0x00000000,
0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000001,
0x00000000, 0x00000001, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000007, 0x00000000, 0x00000003, 0x00000024, 0x00000000, 0x00000000, 0x00000004, 0x00000036,
0x00000001, 0x00000001, 0x00000004, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff,
0x00000004, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x00000003, 0x00000024, 0x00000000,
0x00000000, 0x00000005, 0x0000003e, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000007, 0x00000000,
0x00000003, 0x00000024, 0x00000000, 0x00000000, 0x00000006, 0x00000003, 0xffffffff, 0xffffffff,
0x00000003, 0x00000000, 0x00000006, 0x00000003, 0xffffffff, 0xffffffff, 0x00000003, 0x00000001,
0x00000006, 0x00000003, 0xffffffff, 0xffffffff, 0x00000003, 0x00000002, 0x00000006, 0x00000003,
0xffffffff, 0xffffffff, 0x00000003, 0x00000003, 0x00000006, 0x00000003, 0xffffffff, 0xffffffff,
0x00000003, 0x00000004, 0x00000006, 0x00000003, 0xffffffff, 0xffffffff, 0x00000003, 0x00000005,
0x00000000, 0x00000002, 0x00000014, 0x00000007, 0x000002c1, 0x00000000, 0x00000002, 0x00000030,
0x00000007, 0x000002c8, 0x00000000, 0x00000004, 0x0000001e, 0x00000002, 0x00000102, 0x00000000,
0x00000004, 0x00000027, 0x00000007, 0x0000010b, 0x00000000, 0x00000006, 0x00000009, 0x00000003,
0x00000131, 0x00000000, 0x00000001, 0x00000014, 0x00000006, 0x000002cf, 0x00000000, 0x00000004,
0x00000005, 0x00000004, 0x000000e9, 0x00000000, 0x00000009, 0x00000004, 0x00000000, 0x00000000,
0x00000003, 0x00000051, 0x00000003, 0x00000001, 0x00000000, 0x00000003, 0x00000076, 0x00000004,
0x00000002, 0x00000004, 0x00000000, 0x000002b4, 0x00000007, 0x00000001, 0x0000000c, 0x00000003,
0x00000076, 0x00000004, 0x00000002, 0x00000004, 0x00000001, 0x000002bb, 0x00000006, 0x00000001,
0x00000010, 0x00000004, 0x000000e9, 0x00000004, 0x00000003, 0x00000014, 0x00000005, 0x00000000,
0x00000001, 0x00000001, 0x00000003, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000003,
0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0xffffffff, 0x00000001,
0x00000014, 0x00000004, 0x00000004, 0xffffffff, 0x00000001, 0x00000000, 0x00000000, 0x00000001,
0x00000001, 0xffffffff, 0x00000001, 0x00000008, 0x00000004, 0x00000002, 0xffffffff, 0x00000006,
0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
0x00000006, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000001, 0x00000020, 0x0000000c, 0x00000018, 0xffffffff, 0x00000003, 0x00000000, 0x00000000,
0x00000001, 0x00000001, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xffffffff,
0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000000,
0x00000000, 0x00000006, 0xffffffff, 0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000006,
0x00000004, 0x00000005, 0x00000006, 0x00000008, 0x00000004, 0x00000005, 0x00000002, 0x505c3a43,
0x72676f72, 0x656d6d61, 0x63694d5c, 0x6f736f72, 0x44207466, 0x63657269, 0x53205874, 0x28204b44,
0x656e754a, 0x31303220, 0x555c2930, 0x696c6974, 0x73656974, 0x6e69625c, 0x3638785c, 0x6165685c,
0x2e726564, 0x74737866, 0x74637572, 0x4f535620, 0x66207b20, 0x74616f6c, 0x20702033, 0x4f50203a,
0x49544953, 0x203b4e4f, 0x730a3b7d, 0x63757274, 0x53482074, 0x7b204f44, 0x6f6c6620, 0x65207461,
0x205d345b, 0x5653203a, 0x7365545f, 0x63614673, 0x3b726f74, 0x6f6c6620, 0x69207461, 0x205d325b,
0x5653203a, 0x736e495f, 0x54656469, 0x46737365, 0x6f746361, 0x7d203b72, 0x74730a3b, 0x74637572,
0x4f534820, 0x66207b20, 0x74616f6c, 0x20702033, 0x4542203a, 0x5245495a, 0x3b534f50, 0x0a3b7d20,
0x4f445348, 0x48434220, 0x49202853, 0x7475706e, 0x63746150, 0x53563c68, 0x38202c4f, 0x7069203e,
0x6975202c, 0x5020746e, 0x68637461, 0x3a204449, 0x5f565320, 0x6d697250, 0x76697469, 0x20444965,
0x0a7b0a29, 0x20202020, 0x4f445348, 0x73657220, 0x20200a3b, 0x65722020, 0x5b652e73, 0x3d205d30,
0x73657220, 0x315b652e, 0x203d205d, 0x2e736572, 0x5d325b65, 0x72203d20, 0x652e7365, 0x205d335b,
0x2e31203d, 0x0a3b6630, 0x20202020, 0x2e736572, 0x5d305b69, 0x72203d20, 0x692e7365, 0x205d315b,
0x2e31203d, 0x0a3b6630, 0x20202020, 0x75746572, 0x72206e72, 0x0a3b7365, 0x645b0a7d, 0x69616d6f,
0x7122286e, 0x22646175, 0x5b0a5d29, 0x74726170, 0x6f697469, 0x676e696e, 0x6e692228, 0x65676574,
0x5d292272, 0x756f5b0a, 0x74757074, 0x6f706f74, 0x79676f6c, 0x72742228, 0x676e6169, 0x635f656c,
0x5d292277, 0x756f5b0a, 0x74757074, 0x746e6f63, 0x706c6f72, 0x746e696f, 0x29382873, 0x705b0a5d,
0x68637461, 0x736e6f63, 0x746e6174, 0x636e7566, 0x43422228, 0x29225348, 0x53480a5d, 0x4842204f,
0x49202853, 0x7475706e, 0x63746150, 0x53563c68, 0x38202c4f, 0x2c70203e, 0x6e697520, 0x20692074,
0x5653203a, 0x74754f5f, 0x43747570, 0x72746e6f, 0x6f506c6f, 0x49746e69, 0x75202c44, 0x20746e69,
0x63746150, 0x20444968, 0x5653203a, 0x6972505f, 0x6974696d, 0x44496576, 0x7b0a2920, 0x2020200a,
0x4f534820, 0x73657220, 0x20200a3b, 0x65722020, 0x20702e73, 0x5b70203d, 0x702e5d69, 0x20200a3b,
0x65722020, 0x6e727574, 0x73657220, 0x0a7d0a3b, 0x626f6c47, 0x4c736c61, 0x6c61636f, 0x44534873,
0x653a3a4f, 0x4f445348, 0x56693a3a, 0x3a3a4f53, 0x63694d70, 0x6f736f72, 0x28207466, 0x48202952,
0x204c534c, 0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x39322e39, 0x3235392e, 0x3131332e,
0x48420031, 0x73680053, 0x305f355f, 0x6e6f6320, 0x6c6f7274, 0x696f7020, 0x0000746e,
};

static void test_get_blob_part2(void)
{
    ID3DBlob *blob, *blob2;
    HRESULT hr, expected;
    ULONG refcount;
    DWORD *dword;
    SIZE_T size;
    UINT i;

#if D3D_COMPILER_VERSION >= 46
    expected = D3DERR_INVALIDCALL;
#else
    expected = E_FAIL;
#endif

    /* D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part2, test_blob_part2[6], D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 232, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_PCSG == *(dword + 9), "PCSG got %#lx, expected %#lx.\n", *(dword + 9), TAG_PCSG);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);

        if (parts[i] == D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            refcount = ID3D10Blob_Release(blob2);
            ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
        }
        else
        {
            ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
        }
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_ALL_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part2, test_blob_part2[6], D3D_BLOB_ALL_SIGNATURE_BLOB, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 344, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_ISGN == *(dword + 11), "ISGN got %#lx, expected %#lx.\n", *(dword + 11), TAG_ISGN);
    ok(TAG_OSGN == *(dword + 24), "OSGN got %#lx, expected %#lx.\n", *(dword + 24), TAG_OSGN);
    ok(TAG_PCSG == *(dword + 37), "PCSG got %#lx, expected %#lx.\n", *(dword + 37), TAG_PCSG);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);

        if (parts[i] == D3D_BLOB_ALL_SIGNATURE_BLOB
                || parts[i] == D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB
                || parts[i] == D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB
                || parts[i] == D3D_BLOB_INPUT_SIGNATURE_BLOB
                || parts[i] == D3D_BLOB_OUTPUT_SIGNATURE_BLOB)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            refcount = ID3D10Blob_Release(blob2);
            ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
        }
        else
        {
            ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
        }
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_DEBUG_INFO */
    hr = D3DGetBlobPart(test_blob_part2, test_blob_part2[6], D3D_BLOB_DEBUG_INFO, 0, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 4055, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC != *dword, "DXBC failed got %#lx.\n", *dword);

    for (i = 0; i < ARRAY_SIZE(parts); i++)
    {
        /* There isn't a full DXBC blob returned for D3D_BLOB_DEBUG_INFO */
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);
#if D3D_COMPILER_VERSION >= 46
        todo_wine
#endif
        ok(hr == expected, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected);
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3D_BLOB_LEGACY_SHADER */
    hr = D3DGetBlobPart(test_blob_part2, test_blob_part2[6], D3D_BLOB_LEGACY_SHADER, 0, &blob);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* D3D_BLOB_XNA_PREPASS_SHADER */
    hr = D3DGetBlobPart(test_blob_part2, test_blob_part2[6], D3D_BLOB_XNA_PREPASS_SHADER, 0, &blob);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* D3D_BLOB_XNA_SHADER */
    hr = D3DGetBlobPart(test_blob_part2, test_blob_part2[6], D3D_BLOB_XNA_SHADER, 0, &blob);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* D3DCOMPILER_STRIP_DEBUG_INFO */
    hr = D3DStripShader(test_blob_part2, test_blob_part2[6], D3DCOMPILER_STRIP_DEBUG_INFO, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 952, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_RDEF == *(dword + 14), "RDEF got %#lx, expected %#lx.\n", *(dword + 14), TAG_RDEF);
    ok(TAG_ISGN == *(dword + 44), "ISGN got %#lx, expected %#lx.\n", *(dword + 44), TAG_ISGN);
    ok(TAG_OSGN == *(dword + 57), "OSGN got %#lx, expected %#lx.\n", *(dword + 57), TAG_OSGN);
    ok(TAG_PCSG == *(dword + 70), "PCSG got %#lx, expected %#lx.\n", *(dword + 70), TAG_PCSG);
    ok(TAG_SHEX == *(dword + 119), "SHEX got %#lx, expected %#lx.\n", *(dword + 119), TAG_SHEX);
    ok(TAG_STAT == *(dword + 199), "STAT got %#lx, expected %#lx.\n", *(dword + 199), TAG_STAT);

    hr = D3DGetBlobPart(dword, size, D3D_BLOB_DEBUG_INFO, 0, &blob2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);

    /* D3DCOMPILER_STRIP_REFLECTION_DATA */
    hr = D3DStripShader(test_blob_part2, test_blob_part2[6], D3DCOMPILER_STRIP_REFLECTION_DATA, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 4735, "Got unexpected size %Iu.\n", size);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#lx, expected %#lx.\n", *dword, TAG_DXBC);
    ok(TAG_ISGN == *(dword + 13), "ISGN got %#lx, expected %#lx.\n", *(dword + 13), TAG_ISGN);
    ok(TAG_OSGN == *(dword + 26), "OSGN got %#lx, expected %#lx.\n", *(dword + 26), TAG_OSGN);
    ok(TAG_PCSG == *(dword + 39), "PCSG got %#lx, expected %#lx.\n", *(dword + 39), TAG_PCSG);
    ok(TAG_SHEX == *(dword + 88), "SHEX got %#lx, expected %#lx.\n", *(dword + 88), TAG_SHEX);
    ok(TAG_SDBG == *(dword + 168), "SDBG got %#lx, expected %#lx.\n", *(dword + 168), TAG_SDBG);

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %lu references left.\n", refcount);
}

#if D3D_COMPILER_VERSION >= 46
static BOOL create_file(WCHAR *filename, const DWORD *data, DWORD data_size)
{
    static WCHAR temp_dir[MAX_PATH];
    DWORD written;
    HANDLE file;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    GetTempFileNameW(temp_dir, NULL, 0, filename);
    file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    if (data)
    {
        WriteFile(file, data, data_size, &written, NULL);
        if (written != data_size)
        {
            CloseHandle(file);
            DeleteFileW(filename);
            return FALSE;
        }
    }
    CloseHandle(file);
    return TRUE;
}

/* test_cso_data - fxc.exe file.hlsl /Fo file.cso */
static const DWORD test_cso_data[] =
{
#if 0
    struct PSInput
    {
        float4 value : SV_POSITION;
    };

    PSInput main(float4 position : POSITION)
    {
        PSInput result;
        result.value = position;
        return result;
    }
#endif
    0xfffe0200, 0x0014fffe, 0x42415443, 0x0000001c, 0x00000023, 0xfffe0200, 0x00000000, 0x00000000,
    0x00000100, 0x0000001c, 0x325f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
    0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x0200001f, 0x80000000,
    0x900f0000, 0x02000001, 0xc00f0000, 0x90e40000, 0x0000ffff
};

static void test_D3DReadFileToBlob(void)
{
    WCHAR filename[MAX_PATH] = {'n','o','n','e','x','i','s','t','e','n','t',0};
    ID3DBlob *blob = NULL;
    SIZE_T data_size;
    DWORD *data;
    HRESULT hr;

    hr = D3DReadFileToBlob(filename, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    hr = D3DReadFileToBlob(filename, &blob);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    if (0)
    {
        /* Crashes on Windows. */
        create_file(filename, test_cso_data, ARRAY_SIZE(test_cso_data));
        D3DReadFileToBlob(filename, NULL);
        DeleteFileW(filename);
    }

    if (!create_file(filename, NULL, 0))
    {
        win_skip("File creation failed.\n");
        return;
    }
    hr = D3DReadFileToBlob(filename, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    data_size = ID3D10Blob_GetBufferSize(blob);
    ok(!data_size, "Got unexpected data size.\n");
    DeleteFileW(filename);
    ID3D10Blob_Release(blob);

    if (!create_file(filename, test_cso_data, ARRAY_SIZE(test_cso_data)))
    {
        win_skip("File creation failed.\n");
        return;
    }
    hr = D3DReadFileToBlob(filename, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    data_size = ID3D10Blob_GetBufferSize(blob);
    ok(data_size == ARRAY_SIZE(test_cso_data), "Got unexpected data size.\n");
    data = ID3D10Blob_GetBufferPointer(blob);
    ok(!memcmp(data, test_cso_data, ARRAY_SIZE(test_cso_data)), "Got unexpected data.\n");
    DeleteFileW(filename);
    ID3D10Blob_Release(blob);
}

static void test_D3DWriteBlobToFile(void)
{
    WCHAR temp_dir[MAX_PATH], filename[MAX_PATH];
    ID3DBlob *blob;
    HRESULT hr;

    GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    GetTempFileNameW(temp_dir, NULL, 0, filename);

    hr = D3DCreateBlob(16, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DWriteBlobToFile(blob, filename, FALSE);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "Got unexpected hr %#lx.\n", hr);

    hr = D3DWriteBlobToFile(blob, filename, TRUE);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    DeleteFileW(filename);

    hr = D3DWriteBlobToFile(blob, filename, FALSE);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DWriteBlobToFile(blob, filename, FALSE);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "Got unexpected hr %#lx.\n", hr);

    DeleteFileW(filename);

    ID3D10Blob_Release(blob);
}
#endif
#endif

START_TEST(blob)
{
#if D3D_COMPILER_VERSION >= 43
    test_create_blob();
    test_get_blob_part();
    test_get_blob_part2();
#if D3D_COMPILER_VERSION >= 46
    test_D3DReadFileToBlob();
    test_D3DWriteBlobToFile();
#endif
#endif
}
