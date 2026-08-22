/*
 * Copyright (C) 2010 Matteo Bruni
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
#define CONST_VTABLE
#include "wine/test.h"

#include <d3d9types.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>

/* TODO: maybe this is defined in some header file,
   perhaps with a different name? */
#define D3DXERR_INVALIDDATA                      0x88760b59

static HRESULT (WINAPI *pD3DAssemble)(const void *data, SIZE_T datasize, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, UINT flags,
        ID3DBlob **shader, ID3DBlob **error_messages);

struct shader_test {
    const char *text;
    const DWORD bytes[128];
};

static void dump_shader(DWORD *shader) {
    unsigned int i = 0, j = 0;
    do {
        trace("0x%08lx ", shader[i]);
        j++;
        i++;
        if(j == 6) trace("\n");
    } while(shader[i - 1] != D3DSIO_END);
    if(j != 6) trace("\n");
}

static void exec_tests(const char *name, struct shader_test tests[], unsigned int count) {
    HRESULT hr;
    DWORD *res;
    unsigned int i, j;
    BOOL diff;
    ID3DBlob *shader, *messages;

    for(i = 0; i < count; i++) {
        /* D3DAssemble sets messages to 0 if there aren't error messages */
        messages = NULL;
        hr = pD3DAssemble(tests[i].text, strlen(tests[i].text), NULL, NULL,
                NULL, D3DCOMPILE_SKIP_VALIDATION, &shader, &messages);
        ok(hr == S_OK, "Test %s, shader %u: D3DAssemble failed with error %#lx - %ld.\n", name, i, hr, hr & 0xffff);
        if(messages) {
            trace("D3DAssemble messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
            ID3D10Blob_Release(messages);
        }
        if(FAILED(hr)) continue;

        j = 0;
        diff = FALSE;
        res = ID3D10Blob_GetBufferPointer(shader);
        while(res[j] != D3DSIO_END && tests[i].bytes[j] != D3DSIO_END) {
            if(res[j] != tests[i].bytes[j]) diff = TRUE;
            j++;
        };
        /* Both must have an end token */
        if(res[j] != tests[i].bytes[j]) diff = TRUE;

        if(diff) {
            ok(FALSE, "Test %s, shader %d: Generated code differs\n", name, i);
            dump_shader(res);
        }
        ID3D10Blob_Release(shader);
    }
}

static void preproc_test(void) {
    struct shader_test tests[] = {
        {   /* shader 0 */
            "vs.1.1\r\n"
            "//some comments\r\n"
            "//other comments\n"
            "; yet another comment\r\n"
            "add r0, r0, r1\n",
            {0xfffe0101, 0x00000002, 0x800f0000, 0x80e40000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 1 */
            "#line 1 \"vertex.vsh\"\n"
            "vs.1.1\n",
            {0xfffe0101, 0x0000ffff}
        },
        {   /* shader 2 */
            "#define REG 1 + 2 +\\\n"
            "3 + 4\n"
            "vs.1.1\n"
            "mov r0, c0[ REG ]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e4000a, 0x0000ffff}
        },
    };

    exec_tests("preproc", tests, ARRAY_SIZE(tests));
}

static void ps_1_1_test(void) {
    struct shader_test tests[] = {
        {   /* shader 0 */
            "ps.1.1\r\n"
            "tex t0\r\n"
            "add r0.rgb, r0, r1\r\n"
            "+mov r0.a, t0\r\n",
            {0xffff0101, 0x00000042, 0xb00f0000, 0x00000002, 0x80070000, 0x80e40000,
             0x80e40001, 0x40000001, 0x80080000, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 1 */
            "ps.1.1\n"
            "mov_d4 r0, r1\n",
            {0xffff0101, 0x00000001, 0x8e0f0000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 2 */
            "ps_1_1\n"
            "def c2, 0, 0., 0, 0.\n",
            {0xffff0101, 0x00000051, 0xa00f0002, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x0000ffff}
        },
    };

    exec_tests("ps_1_1", tests, ARRAY_SIZE(tests));
}

static void vs_1_1_test(void) {
    struct shader_test tests[] = {
        /* Basic instruction tests */
        {   /* shader 0 */
            "vs_1_1\n"
            "add r0, r1, r2\n",
            {0xfffe0101, 0x00000002, 0x800f0000, 0x80e40001, 0x80e40002, 0x0000ffff}
        },
        {   /* shader 1 */
            "vs_1_1\n"
            "nop\n",
            {0xfffe0101, 0x00000000, 0x0000ffff}
        },
        /* Output register tests */
        {   /* shader 2 */
            "vs_1_1\n"
            "mov oPos, c0\n",
            {0xfffe0101, 0x00000001, 0xc00f0000, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 3 */
            "vs_1_1\n"
            "mov oT0, c0\n",
            {0xfffe0101, 0x00000001, 0xe00f0000, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 4 */
            "vs_1_1\n"
            "mov oT5, c0\n",
            {0xfffe0101, 0x00000001, 0xe00f0005, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 5 */
            "vs_1_1\n"
            "mov oD0, c0\n",
            {0xfffe0101, 0x00000001, 0xd00f0000, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 6 */
            "vs_1_1\n"
            "mov oD1, c0\n",
            {0xfffe0101, 0x00000001, 0xd00f0001, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 7 */
            "vs_1_1\n"
            "mov oFog, c0.x\n",
            {0xfffe0101, 0x00000001, 0xc00f0001, 0xa0000000, 0x0000ffff}
        },
        {   /* shader 8 */
            "vs_1_1\n"
            "mov oPts, c0.x\n",
            {0xfffe0101, 0x00000001, 0xc00f0002, 0xa0000000, 0x0000ffff}
        },
        /* A bunch of tests for declarations */
        {   /* shader 9 */
            "vs_1_1\n"
            "dcl_position0 v0",
            {0xfffe0101, 0x0000001f, 0x80000000, 0x900f0000, 0x0000ffff}
        },
        {   /* shader 10 */
            "vs_1_1\n"
            "dcl_position v1",
            {0xfffe0101, 0x0000001f, 0x80000000, 0x900f0001, 0x0000ffff}
        },
        {   /* shader 11 */
            "vs_1_1\n"
            "dcl_normal12 v15",
            {0xfffe0101, 0x0000001f, 0x800c0003, 0x900f000f, 0x0000ffff}
        },
        {   /* shader 12 */
            "vs_1_1\n"
            "add r0, v0, v1\n",
            {0xfffe0101, 0x00000002, 0x800f0000, 0x90e40000, 0x90e40001, 0x0000ffff}
        },
        {   /* shader 13 */
            "vs_1_1\n"
            "def c12, 0, -1, -0.5, 1024\n",
            {0xfffe0101, 0x00000051, 0xa00f000c, 0x00000000, 0xbf800000, 0xbf000000,
             0x44800000, 0x0000ffff},
        },
        {   /* shader 14: writemasks, swizzles */
            "vs_1_1\n"
            "dp4 r0.xw, r1.wzyx, r2.xxww\n",
            {0xfffe0101, 0x00000009, 0x80090000, 0x801b0001, 0x80f00002, 0x0000ffff}
        },
        {   /* shader 15: negation input modifier. Other modifiers not supprted in vs_1_1 */
            "vs_1_1\n"
            "add r0, -r0.x, -r1\n",
            {0xfffe0101, 0x00000002, 0x800f0000, 0x81000000, 0x81e40001, 0x0000ffff}
        },
        {   /* shader 16: relative addressing */
            "vs_1_1\n"
            "mov r0, c0[a0.x]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e42000, 0x0000ffff}
        },
        {   /* shader 17: relative addressing */
            "vs_1_1\n"
            "mov r0, c1[a0.x + 2]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e42003, 0x0000ffff}
        },
        {   /* shader 18 */
            "vs_1_1\n"
            "def c0, 1.0f, 1.0f, 1.0f, 0.5f\n",
            {0xfffe0101, 0x00000051, 0xa00f0000, 0x3f800000, 0x3f800000, 0x3f800000,
             0x3f000000, 0x0000ffff}
        },
        /* Other relative addressing tests */
        {   /* shader 19 */
            "vs_1_1\n"
            "mov r0, c[ a0.x + 12 ]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e4200c, 0x0000ffff}
        },
        {   /* shader 20 */
            "vs_1_1\n"
            "mov r0, c[ 2 + a0.x ]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e42002, 0x0000ffff}
        },
        {   /* shader 21 */
            "vs_1_1\n"
            "mov r0, c[ 2 + a0.x + 12 ]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e4200e, 0x0000ffff}
        },
        {   /* shader 22 */
            "vs_1_1\n"
            "mov r0, c[ 2 + 10 + 12 ]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e40018, 0x0000ffff}
        },
        {   /* shader 23 */
            "vs_1_1\n"
            "mov r0, c4[ 2 ]\n",
            {0xfffe0101, 0x00000001, 0x800f0000, 0xa0e40006, 0x0000ffff}
        },
        {   /* shader 24 */
            "vs_1_1\n"
            "rcp r0, v0.x\n",
            {0xfffe0101, 0x00000006, 0x800f0000, 0x90000000, 0x0000ffff}
        },
        {   /* shader 25 */
            "vs.1.1\n"
            "rsq r0, v0.x\n",
            {0xfffe0101, 0x00000007, 0x800f0000, 0x90000000, 0x0000ffff}
        },
    };

    exec_tests("vs_1_1", tests, ARRAY_SIZE(tests));
}

static void ps_1_3_test(void) {
    struct shader_test tests[] = {
        /* Basic instruction tests */
        {   /* shader 0 */
            "ps_1_3\n"
            "mov r0, r1\n",
            {0xffff0103, 0x00000001, 0x800f0000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 1 */
            "ps_1_3\n"
            "add r0, r1, r0\n",
            {0xffff0103, 0x00000002, 0x800f0000, 0x80e40001, 0x80e40000, 0x0000ffff}
        },
        /* Color interpolator tests */
        {   /* shader 2 */
            "ps_1_3\n"
            "mov r0, v0\n",
            {0xffff0103, 0x00000001, 0x800f0000, 0x90e40000, 0x0000ffff}
        },
        {   /* shader 3 */
            "ps_1_3\n"
            "mov r0, v1\n",
            {0xffff0103, 0x00000001, 0x800f0000, 0x90e40001, 0x0000ffff}
        },
        /* Texture sampling instructions */
        {   /* shader 4 */
            "ps_1_3\n"
            "tex t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x0000ffff}
        },
        {   /* shader 5 */
            "ps_1_3\n"
            "tex t0\n"
            "texreg2ar t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000045, 0xb00f0001, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 6 */
            "ps_1_3\n"
            "tex t0\n"
            "texreg2gb t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000046, 0xb00f0001, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 7 */
            "ps_1_3\n"
            "tex t0\n"
            "texreg2rgb t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000052, 0xb00f0001, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 8 */
            "ps_1_3\n"
            "cnd r0, r1, r0, v0\n",
            {0xffff0103, 0x00000050, 0x800f0000, 0x80e40001, 0x80e40000, 0x90e40000,
             0x0000ffff}
        },
        {   /* shader 9 */
            "ps_1_3\n"
            "cmp r0, r1, r0, v0\n",
            {0xffff0103, 0x00000058, 0x800f0000, 0x80e40001, 0x80e40000, 0x90e40000,
             0x0000ffff}
        },
        {   /* shader 10 */
            "ps_1_3\n"
            "texkill t0\n",
            {0xffff0103, 0x00000041, 0xb00f0000, 0x0000ffff}
        },
        {   /* shader 11 */
            "ps_1_3\n"
            "tex t0\n"
            "texm3x2pad t1, t0\n"
            "texm3x2tex t2, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000047, 0xb00f0001, 0xb0e40000,
             0x00000048, 0xb00f0002, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 12 */
            "ps_1_3\n"
            "tex t0\n"
            "texm3x2pad t1, t0\n"
            "texm3x2depth t2, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000047, 0xb00f0001, 0xb0e40000,
             0x00000054, 0xb00f0002, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 13 */
            "ps_1_3\n"
            "tex t0\n"
            "texbem t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000043, 0xb00f0001, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 14 */
            "ps_1_3\n"
            "tex t0\n"
            "texbeml t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000044, 0xb00f0001, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 15 */
            "ps_1_3\n"
            "tex t0\n"
            "texdp3tex t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000053, 0xb00f0001, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 16 */
            "ps_1_3\n"
            "tex t0\n"
            "texdp3 t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000055, 0xb00f0001, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 17 */
            "ps_1_3\n"
            "tex t0\n"
            "texm3x3pad t1, t0\n"
            "texm3x3pad t2, t0\n"
            "texm3x3tex t3, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000049, 0xb00f0001, 0xb0e40000,
             0x00000049, 0xb00f0002, 0xb0e40000, 0x0000004a, 0xb00f0003, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 18 */
            "ps_1_3\n"
            "tex t0\n"
            "texm3x3pad t1, t0\n"
            "texm3x3pad t2, t0\n"
            "texm3x3 t3, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000049, 0xb00f0001, 0xb0e40000,
             0x00000049, 0xb00f0002, 0xb0e40000, 0x00000056, 0xb00f0003, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 19 */
            "ps_1_3\n"
            "tex t0\n"
            "texm3x3pad t1, t0\n"
            "texm3x3pad t2, t0\n"
            "texm3x3spec t3, t0, c0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000049, 0xb00f0001, 0xb0e40000,
             0x00000049, 0xb00f0002, 0xb0e40000, 0x0000004c, 0xb00f0003, 0xb0e40000,
             0xa0e40000, 0x0000ffff}
        },
        {   /* shader 20 */
            "ps_1_3\n"
            "tex t0\n"
            "texm3x3pad t1, t0\n"
            "texm3x3pad t2, t0\n"
            "texm3x3vspec t3, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000049, 0xb00f0001, 0xb0e40000,
             0x00000049, 0xb00f0002, 0xb0e40000, 0x0000004d, 0xb00f0003, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 21 */
            "ps_1_3\n"
            "texcoord t0\n",
            {0xffff0103, 0x00000040, 0xb00f0000, 0x0000ffff}
        },
        /* Modifiers, shifts */
        {   /* shader 22 */
            "ps_1_3\n"
            "mov_x2_sat r0, 1 - r1\n",
            {0xffff0103, 0x00000001, 0x811f0000, 0x86e40001, 0x0000ffff}
        },
        {   /* shader 23 */
            "ps_1_3\n"
            "mov_d8 r0, -r1\n",
            {0xffff0103, 0x00000001, 0x8d0f0000, 0x81e40001, 0x0000ffff}
        },
        {   /* shader 24 */
            "ps_1_3\n"
            "mov_sat r0, r1_bx2\n",
            {0xffff0103, 0x00000001, 0x801f0000, 0x84e40001, 0x0000ffff}
        },
        {   /* shader 25 */
            "ps_1_3\n"
            "mov_sat r0, r1_bias\n",
            {0xffff0103, 0x00000001, 0x801f0000, 0x82e40001, 0x0000ffff}
        },
        {   /* shader 26 */
            "ps_1_3\n"
            "mov_sat r0, -r1_bias\n",
            {0xffff0103, 0x00000001, 0x801f0000, 0x83e40001, 0x0000ffff}
        },
        {   /* shader 27 */
            "ps_1_3\n"
            "mov_sat r0, -r1_bx2\n",
            {0xffff0103, 0x00000001, 0x801f0000, 0x85e40001, 0x0000ffff}
        },
        {   /* shader 28 */
            "ps_1_3\n"
            "mov_sat r0, -r1_x2\n",
            {0xffff0103, 0x00000001, 0x801f0000, 0x88e40001, 0x0000ffff}
        },
        {   /* shader 29 */
            "ps_1_3\n"
            "mov_x4_sat r0.a, -r1_bx2.a\n",
            {0xffff0103, 0x00000001, 0x82180000, 0x85ff0001, 0x0000ffff}
        },
        {   /* shader 30 */
            "ps_1_3\n"
            "texcoord_x2 t0\n",
            {0xffff0103, 0x00000040, 0xb10f0000, 0x0000ffff}
        },
        {   /* shader 31 */
            "ps_1_3\n"
            "tex_x2 t0\n",
            {0xffff0103, 0x00000042, 0xb10f0000, 0x0000ffff}
        },
        {   /* shader 32 */
            "ps_1_3\n"
            "texreg2ar_x4 t0, t1\n",
            {0xffff0103, 0x00000045, 0xb20f0000, 0xb0e40001, 0x0000ffff}
        },
        {   /* shader 33 */
            "ps_1_3\n"
            "texbem_d4 t1, t0\n",
            {0xffff0103, 0x00000043, 0xbe0f0001, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 34 */
            "ps_1_3\n"
            "tex t0\n"
            "texm3x3pad_x2 t1, t0\n"
            "texm3x3pad_x2 t2, t0\n"
            "texm3x3tex_x2 t3, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000049, 0xb10f0001, 0xb0e40000,
             0x00000049, 0xb10f0002, 0xb0e40000, 0x0000004a, 0xb10f0003, 0xb0e40000,
             0x0000ffff}
        },
        {   /* shader 35 */
            "ps.1.3\n"
            "tex t0\n"
            "texdp3tex_x8 t1, t0\n",
            {0xffff0103, 0x00000042, 0xb00f0000, 0x00000053, 0xb30f0001, 0xb0e40000,
             0x0000ffff}
        },
    };

    exec_tests("ps_1_3", tests, ARRAY_SIZE(tests));
}

static void ps_1_4_test(void) {
    struct shader_test tests[] = {
        /* Basic instruction tests */
        {   /* shader 0 */
            "ps_1_4\n"
            "mov r0, r1\n",
            {0xffff0104, 0x00000001, 0x800f0000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 1 */
            "ps_1_4\n"
            "mov r0, r5\n",
            {0xffff0104, 0x00000001, 0x800f0000, 0x80e40005, 0x0000ffff}
        },
        {   /* shader 2 */
            "ps_1_4\n"
            "mov r0, c7\n",
            {0xffff0104, 0x00000001, 0x800f0000, 0xa0e40007, 0x0000ffff}
        },
        {   /* shader 3 */
            "ps_1_4\n"
            "mov r0, v1\n",
            {0xffff0104, 0x00000001, 0x800f0000, 0x90e40001, 0x0000ffff}
        },
        {   /* shader 4 */
            "ps_1_4\n"
            "phase\n",
            {0xffff0104, 0x0000fffd, 0x0000ffff}
        },
        {   /* shader 5 */
            "ps_1_4\n"
            "texcrd r0, t0\n",
            {0xffff0104, 0x00000040, 0x800f0000, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 6 */
            "ps_1_4\n"
            "texcrd r4, t3\n",
            {0xffff0104, 0x00000040, 0x800f0004, 0xb0e40003, 0x0000ffff}
        },
        {   /* shader 7 */
            "ps_1_4\n"
            "texcrd_sat r4, t3\n",
            {0xffff0104, 0x00000040, 0x801f0004, 0xb0e40003, 0x0000ffff}
        },
        {   /* shader 8 */
            "ps_1_4\n"
            "texld r0, t0\n",
            {0xffff0104, 0x00000042, 0x800f0000, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 9 */
            "ps_1_4\n"
            "texld r1, t4\n",
            {0xffff0104, 0x00000042, 0x800f0001, 0xb0e40004, 0x0000ffff}
        },
        {   /* shader 10 */
            "ps_1_4\n"
            "texld r5, r0\n",
            {0xffff0104, 0x00000042, 0x800f0005, 0x80e40000, 0x0000ffff}
        },
        {   /* shader 11 */
            "ps_1_4\n"
            "texld r5, c0\n", /* Assembly succeeds, validation fails */
            {0xffff0104, 0x00000042, 0x800f0005, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 12 */
            "ps_1_4\n"
            "texld r5, r2_dz\n",
            {0xffff0104, 0x00000042, 0x800f0005, 0x89e40002, 0x0000ffff}
        },
        {   /* shader 13 */
            "ps_1_4\n"
            "bem r1.rg, c0, r0\n",
            {0xffff0104, 0x00000059, 0x80030001, 0xa0e40000, 0x80e40000, 0x0000ffff}
        },
        {   /* shader 14 */
            "ps_1_4\n"
            "texdepth r5\n",
            {0xffff0104, 0x00000057, 0x800f0005, 0x0000ffff}
        },
        {   /* shader 15 */
            "ps_1_4\n"
            "add r0, r1, r2_bx2\n",
            {0xffff0104, 0x00000002, 0x800f0000, 0x80e40001, 0x84e40002, 0x0000ffff}
        },
        {   /* shader 16 */
            "ps_1_4\n"
            "add_x4 r0, r1, r2\n",
            {0xffff0104, 0x00000002, 0x820f0000, 0x80e40001, 0x80e40002, 0x0000ffff}
        },
        {   /* shader 17 */
            "ps_1_4\n"
            "add r0.rgb, r1, r2\n"
            "+add r0.a, r1, r2\n",
            {0xffff0104, 0x00000002, 0x80070000, 0x80e40001, 0x80e40002, 0x40000002,
             0x80080000, 0x80e40001, 0x80e40002, 0x0000ffff}
        },
        {   /* shader 18 */
            "ps_1_4\n"
            "texdepth_x2 r5\n",
            {0xffff0104, 0x00000057, 0x810f0005, 0x0000ffff}
        },
        {   /* shader 18 */
            "ps.1.4\n"
            "bem_d2 r1, c0, r0\n",
            {0xffff0104, 0x00000059, 0x8f0f0001, 0xa0e40000, 0x80e40000, 0x0000ffff}
        },
    };

    exec_tests("ps_1_4", tests, ARRAY_SIZE(tests));
}

static void vs_2_0_test(void) {
    struct shader_test tests[] = {
        /* Basic instruction tests */
        {   /* shader 0 */
            "vs_2_0\n"
            "mov r0, r1\n",
            {0xfffe0200, 0x02000001, 0x800f0000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 1 */
            "vs_2_0\n"
            "lrp r0, v0, c0, r1\n",
            {0xfffe0200, 0x04000012, 0x800f0000, 0x90e40000, 0xa0e40000, 0x80e40001,
             0x0000ffff}
        },
        {   /* shader 2 */
            "vs_2_0\n"
            "dp4 oPos, v0, c0\n",
            {0xfffe0200, 0x03000009, 0xc00f0000, 0x90e40000, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 3 */
            "vs_2_0\n"
            "mov r0, c0[a0.x]\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa0e42000, 0xb0000000, 0x0000ffff}
        },
        {   /* shader 4 */
            "vs_2_0\n"
            "mov r0, c0[a0.y]\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa0e42000, 0xb0550000, 0x0000ffff}
        },
        {   /* shader 5 */
            "vs_2_0\n"
            "mov r0, c0[a0.z]\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa0e42000, 0xb0aa0000, 0x0000ffff}
        },
        {   /* shader 6 */
            "vs_2_0\n"
            "mov r0, c0[a0.w]\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa0e42000, 0xb0ff0000, 0x0000ffff}
        },
        {   /* shader 7 */
            "vs_2_0\n"
            "mov r0, c0[a0.w].x\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa0002000, 0xb0ff0000, 0x0000ffff}
        },
        {   /* shader 8 */
            "vs_2_0\n"
            "mov r0, -c0[a0.w+5].x\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa1002005, 0xb0ff0000, 0x0000ffff}
        },
        {   /* shader 9 */
            "vs_2_0\n"
            "mov r0, c0[a0]\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa0e42000, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 10 */
            "vs_2_0\n"
            "mov r0, c0[a0.xyww]\n",
            {0xfffe0200, 0x03000001, 0x800f0000, 0xa0e42000, 0xb0f40000, 0x0000ffff}
        },
        {   /* shader 11 */
            "vs_2_0\n"
            "add r0, c0[a0.x], c1[a0.y]\n", /* validation would fail on this line */
            {0xfffe0200, 0x05000002, 0x800f0000, 0xa0e42000, 0xb0000000, 0xa0e42001,
             0xb0550000, 0x0000ffff}
        },
        {   /* shader 12 */
            "vs_2_0\n"
            "rep i0\n"
            "endrep\n",
            {0xfffe0200, 0x01000026, 0xf0e40000, 0x00000027, 0x0000ffff}
        },
        {   /* shader 13 */
            "vs_2_0\n"
            "if b0\n"
            "else\n"
            "endif\n",
            {0xfffe0200, 0x01000028, 0xe0e40800, 0x0000002a, 0x0000002b, 0x0000ffff}
        },
        {   /* shader 14 */
            "vs_2_0\n"
            "loop aL, i0\n"
            "endloop\n",
            {0xfffe0200, 0x0200001b, 0xf0e40800, 0xf0e40000, 0x0000001d, 0x0000ffff}
        },
        {   /* shader 15 */
            "vs_2_0\n"
            "nrm r0, c0\n",
            {0xfffe0200, 0x02000024, 0x800f0000, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 16 */
            "vs_2_0\n"
            "crs r0, r1, r2\n",
            {0xfffe0200, 0x03000021, 0x800f0000, 0x80e40001, 0x80e40002, 0x0000ffff}
        },
        {   /* shader 17 */
            "vs_2_0\n"
            "sgn r0, r1, r2, r3\n",
            {0xfffe0200, 0x04000022, 0x800f0000, 0x80e40001, 0x80e40002, 0x80e40003,
             0x0000ffff}
        },
        {   /* shader 18 */
            "vs_2_0\n"
            "sincos r0, r1, r2, r3\n",
            {0xfffe0200, 0x04000025, 0x800f0000, 0x80e40001, 0x80e40002, 0x80e40003,
             0x0000ffff}
        },
        {   /* shader 19 */
            "vs_2_0\n"
            "pow r0, r1, r2\n",
            {0xfffe0200, 0x03000020, 0x800f0000, 0x80e40001, 0x80e40002, 0x0000ffff}
        },
        {   /* shader 20 */
            "vs_2_0\n"
            "mova a0.y, c0.z\n",
            {0xfffe0200, 0x0200002e, 0xb0020000, 0xa0aa0000, 0x0000ffff}
        },
        {   /* shader 21 */
            "vs_2_0\n"
            "defb b0, true\n"
            "defb b1, false\n",
            {0xfffe0200, 0x0200002f, 0xe00f0800, 0x00000001, 0x0200002f, 0xe00f0801,
             0x00000000, 0x0000ffff}
        },
        {   /* shader 22 */
            "vs_2_0\n"
            "defi i0, - 1, 1, 10, 0\n"
            "defi i1, 0, 40, 30, 10\n",
            {0xfffe0200, 0x05000030, 0xf00f0000, 0xffffffff, 0x00000001, 0x0000000a,
             0x00000000, 0x05000030, 0xf00f0001, 0x00000000, 0x00000028, 0x0000001e,
             0x0000000a, 0x0000ffff},
        },
        {   /* shader 23 */
            "vs_2_0\n"
            "loop aL, i0\n"
            "mov r0, c0[aL]\n"
            "endloop\n",
            {0xfffe0200, 0x0200001b, 0xf0e40800, 0xf0e40000, 0x03000001, 0x800f0000,
             0xa0e42000, 0xf0e40800, 0x0000001d, 0x0000ffff}
        },
        {   /* shader 24 */
            "vs_2_0\n"
            "call l0\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xfffe0200, 0x01000019, 0xa0e41000, 0x0000001c, 0x0100001e, 0xa0e41000,
             0x0000001c, 0x0000ffff}
        },
        {   /* shader 25 */
            "vs_2_0\n"
            "callnz l0, b0\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xfffe0200, 0x0200001a, 0xa0e41000, 0xe0e40800, 0x0000001c, 0x0100001e,
             0xa0e41000, 0x0000001c, 0x0000ffff}
        },
        {   /* shader 26 */
            "vs_2_0\n"
            "callnz l0, !b0\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xfffe0200, 0x0200001a, 0xa0e41000, 0xede40800, 0x0000001c, 0x0100001e,
             0xa0e41000, 0x0000001c, 0x0000ffff}
        },
        {   /* shader 27 */
            "vs_2_0\n"
            "if !b0\n"
            "else\n"
            "endif\n",
            {0xfffe0200, 0x01000028, 0xede40800, 0x0000002a, 0x0000002b, 0x0000ffff}
        },
        {   /* shader 28 */
            "vs_2_0\n"
            "call l3\n"
            "ret\n"
            "label l3\n"
            "ret\n",
            {0xfffe0200, 0x01000019, 0xa0e41003, 0x0000001c, 0x0100001e, 0xa0e41003, 0x0000001c, 0x0000ffff}
        },
        {   /* shader 29: labels up to 2047 are accepted even in vs_2_0 */
            "vs.2.0\n"
            "call l2047\n",
            {0xfffe0200, 0x01000019, 0xa0e417ff, 0x0000ffff}
        },
    };

    exec_tests("vs_2_0", tests, ARRAY_SIZE(tests));
}

static void vs_2_x_test(void) {
    struct shader_test tests[] = {
        {   /* shader 0 */
            "vs_2_x\n"
            "rep i0\n"
            "break\n"
            "endrep\n",
            {0xfffe0201, 0x01000026, 0xf0e40000, 0x0000002c, 0x00000027, 0x0000ffff}
        },
        {   /* shader 1 */
            "vs_2_x\n"
            "if_ge r0, r1\n"
            "endif\n",
            {0xfffe0201, 0x02030029, 0x80e40000, 0x80e40001, 0x0000002b, 0x0000ffff}
        },
        {   /* shader 2 */
            "vs_2_x\n"
            "rep i0\n"
            "break_ne r0, r1\n"
            "endrep",
            {0xfffe0201, 0x01000026, 0xf0e40000, 0x0205002d, 0x80e40000, 0x80e40001,
             0x00000027, 0x0000ffff}
        },

        /* predicates */
        {   /* shader 3 */
            "vs_2_x\n"
            "setp_gt p0, r0, r1\n"
            "(!p0) add r2, r2, r3\n",
            {0xfffe0201, 0x0301005e, 0xb00f1000, 0x80e40000, 0x80e40001, 0x14000002,
             0x800f0002, 0xbde41000, 0x80e40002, 0x80e40003, 0x0000ffff}
        },
        {   /* shader 4 */
            "vs_2_x\n"
            "if p0.x\n"
            "else\n"
            "endif\n",
            {0xfffe0201, 0x01000028, 0xb0001000, 0x0000002a, 0x0000002b, 0x0000ffff}
        },
        {   /* shader 5 */
            "vs_2_x\n"
            "callnz l0, !p0.z\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xfffe0201, 0x0200001a, 0xa0e41000, 0xbdaa1000, 0x0000001c,
             0x0100001e, 0xa0e41000, 0x0000001c, 0x0000ffff}
        },
        {   /* shader 6 */
            "vs.2.x\n"
            "rep i0\n"
            "breakp p0.w\n"
            "endrep\n",
            {0xfffe0201, 0x01000026, 0xf0e40000, 0x01000060, 0xb0ff1000,
             0x00000027, 0x0000ffff}
        },
    };

    exec_tests("vs_2_x", tests, ARRAY_SIZE(tests));
}

static void ps_2_0_test(void) {
    struct shader_test tests[] = {
        {   /* shader 0 */
            "ps_2_0\n"
            "dcl_2d s0\n",
            {0xffff0200, 0x0200001f, 0x90000000, 0xa00f0800, 0x0000ffff}
        },
        {   /* shader 1 */
            "ps_2_0\n"
            "dcl_cube s0\n",
            {0xffff0200, 0x0200001f, 0x98000000, 0xa00f0800, 0x0000ffff}
        },
        {   /* shader 2 */
            "ps_2_0\n"
            "dcl_volume s0\n",
            {0xffff0200, 0x0200001f, 0xa0000000, 0xa00f0800, 0x0000ffff}
        },
        {   /* shader 3 */
            "ps_2_0\n"
            "dcl_volume s0\n"
            "dcl_cube s1\n"
            "dcl_2d s2\n",
            {0xffff0200, 0x0200001f, 0xa0000000, 0xa00f0800, 0x0200001f, 0x98000000,
             0xa00f0801, 0x0200001f, 0x90000000, 0xa00f0802, 0x0000ffff}
        },
        {   /* shader 4 */
            "ps_2_0\n"
            "mov r0, t0\n",
            {0xffff0200, 0x02000001, 0x800f0000, 0xb0e40000, 0x0000ffff}
        },
        {   /* shader 5 */
            "ps_2_0\n"
            "dcl_2d s2\n"
            "texld r0, t1, s2\n",
            {0xffff0200, 0x0200001f, 0x90000000, 0xa00f0802, 0x03000042, 0x800f0000,
             0xb0e40001, 0xa0e40802, 0x0000ffff}
        },
        {   /* shader 6 */
            "ps_2_0\n"
            "texkill t0\n",
            {0xffff0200, 0x01000041, 0xb00f0000, 0x0000ffff}
        },
        {   /* shader 7 */
            "ps_2_0\n"
            "mov oC0, c0\n"
            "mov oC1, c1\n",
            {0xffff0200, 0x02000001, 0x800f0800, 0xa0e40000, 0x02000001, 0x800f0801,
             0xa0e40001, 0x0000ffff}
        },
        {   /* shader 8 */
            "ps_2_0\n"
            "mov oDepth, c0.x\n",
            {0xffff0200, 0x02000001, 0x900f0800, 0xa0000000, 0x0000ffff}
        },
        {   /* shader 9 */
            "ps_2_0\n"
            "dcl_2d s2\n"
            "texldp r0, t1, s2\n",
            {0xffff0200, 0x0200001f, 0x90000000, 0xa00f0802, 0x03010042, 0x800f0000,
             0xb0e40001, 0xa0e40802, 0x0000ffff}
        },
        {   /* shader 10 */
            "ps.2.0\n"
            "dcl_2d s2\n"
            "texldb r0, t1, s2\n",
            {0xffff0200, 0x0200001f, 0x90000000, 0xa00f0802, 0x03020042, 0x800f0000,
             0xb0e40001, 0xa0e40802, 0x0000ffff}
        },
    };

    exec_tests("ps_2_0", tests, ARRAY_SIZE(tests));
}

static void ps_2_x_test(void) {
    struct shader_test tests[] = {
        /* defb and defi are not supposed to work in ps_2_0 (even if defb actually works in ps_2_0 with native) */
        {   /* shader 0 */
            "ps_2_x\n"
            "defb b0, true\n"
            "defb b1, false\n",
            {0xffff0201, 0x0200002f, 0xe00f0800, 0x00000001, 0x0200002f, 0xe00f0801,
             0x00000000, 0x0000ffff}
        },
        {   /* shader 1 */
            "ps_2_x\n"
            "defi i0, -1, 1, 10, 0\n"
            "defi i1, 0, 40, 30, 10\n",
            {0xffff0201, 0x05000030, 0xf00f0000, 0xffffffff, 0x00000001, 0x0000000a,
             0x00000000, 0x05000030, 0xf00f0001, 0x00000000, 0x00000028, 0x0000001e,
             0x0000000a, 0x0000ffff},
        },
        {   /* shader 2 */
            "ps_2_x\n"
            "dsx r0, r0\n",
            {0xffff0201, 0x0200005b, 0x800f0000, 0x80e40000, 0x0000ffff}
        },
        {   /* shader 3 */
            "ps_2_x\n"
            "dsy r0, r0\n",
            {0xffff0201, 0x0200005c, 0x800f0000, 0x80e40000, 0x0000ffff}
        },
        {   /* shader 4 */
            "ps_2_x\n"
            "dcl_2d s2\n"
            "texldd r0, v1, s2, r3, r4\n",
            {0xffff0201, 0x0200001f, 0x90000000, 0xa00f0802, 0x0500005d, 0x800f0000,
             0x90e40001, 0xa0e40802, 0x80e40003, 0x80e40004, 0x0000ffff}
        },
        /* Static flow control tests */
        {   /* shader 5 */
            "ps_2_x\n"
            "call l0\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xffff0201, 0x01000019, 0xa0e41000, 0x0000001c, 0x0100001e, 0xa0e41000,
             0x0000001c, 0x0000ffff}
        },
        {   /* shader 6 */
            "ps_2_x\n"
            "callnz l0, b0\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xffff0201, 0x0200001a, 0xa0e41000, 0xe0e40800, 0x0000001c, 0x0100001e,
             0xa0e41000, 0x0000001c, 0x0000ffff}
        },
        {   /* shader 7 */
            "ps_2_x\n"
            "callnz l0, !b0\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xffff0201, 0x0200001a, 0xa0e41000, 0xede40800, 0x0000001c, 0x0100001e,
             0xa0e41000, 0x0000001c, 0x0000ffff}
        },
        {   /* shader 8 */
            "ps_2_x\n"
            "if !b0\n"
            "else\n"
            "endif\n",
            {0xffff0201, 0x01000028, 0xede40800, 0x0000002a, 0x0000002b, 0x0000ffff}
        },
        /* Dynamic flow control tests */
        {   /* shader 9 */
            "ps_2_x\n"
            "rep i0\n"
            "break\n"
            "endrep\n",
            {0xffff0201, 0x01000026, 0xf0e40000, 0x0000002c, 0x00000027, 0x0000ffff}
        },
        {   /* shader 10 */
            "ps_2_x\n"
            "if_ge r0, r1\n"
            "endif\n",
            {0xffff0201, 0x02030029, 0x80e40000, 0x80e40001, 0x0000002b, 0x0000ffff}
        },
        {   /* shader 11 */
            "ps_2_x\n"
            "rep i0\n"
            "break_ne r0, r1\n"
            "endrep",
            {0xffff0201, 0x01000026, 0xf0e40000, 0x0205002d, 0x80e40000, 0x80e40001,
             0x00000027, 0x0000ffff}
        },
        /* Predicates */
        {   /* shader 12 */
            "ps_2_x\n"
            "setp_gt p0, r0, r1\n"
            "(!p0) add r2, r2, r3\n",
            {0xffff0201, 0x0301005e, 0xb00f1000, 0x80e40000, 0x80e40001, 0x14000002,
             0x800f0002, 0xbde41000, 0x80e40002, 0x80e40003, 0x0000ffff}
        },
        {   /* shader 13 */
            "ps_2_x\n"
            "if p0.x\n"
            "else\n"
            "endif\n",
            {0xffff0201, 0x01000028, 0xb0001000, 0x0000002a, 0x0000002b, 0x0000ffff}
        },
        {   /* shader 14 */
            "ps_2_x\n"
            "callnz l0, !p0.z\n"
            "ret\n"
            "label l0\n"
            "ret\n",
            {0xffff0201, 0x0200001a, 0xa0e41000, 0xbdaa1000, 0x0000001c,
             0x0100001e, 0xa0e41000, 0x0000001c, 0x0000ffff}
        },
        {   /* shader 15 */
            "ps_2_x\n"
            "rep i0\n"
            "breakp p0.w\n"
            "endrep\n",
            {0xffff0201, 0x01000026, 0xf0e40000, 0x01000060, 0xb0ff1000,
             0x00000027, 0x0000ffff}
        },
        {   /* shader 16 */
            "ps.2.x\n"
            "call l2047\n"
            "ret\n"
            "label l2047\n"
            "ret\n",
            {0xffff0201, 0x01000019, 0xa0e417ff, 0x0000001c, 0x0100001e, 0xa0e417ff,
             0x0000001c, 0x0000ffff}
        },
    };

    exec_tests("ps_2_x", tests, ARRAY_SIZE(tests));
}

static void vs_3_0_test(void) {
    struct shader_test tests[] = {
        {   /* shader 0 */
            "vs_3_0\n"
            "mov r0, c0\n",
            {0xfffe0300, 0x02000001, 0x800f0000, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 1 */
            "vs_3_0\n"
            "dcl_2d s0\n",
            {0xfffe0300, 0x0200001f, 0x90000000, 0xa00f0800, 0x0000ffff}
        },
        {   /* shader 2 */
            "vs_3_0\n"
            "dcl_position o0\n",
            {0xfffe0300, 0x0200001f, 0x80000000, 0xe00f0000, 0x0000ffff}
        },
        {   /* shader 3 */
            "vs_3_0\n"
            "dcl_texcoord12 o11\n",
            {0xfffe0300, 0x0200001f, 0x800c0005, 0xe00f000b, 0x0000ffff}
        },
        {   /* shader 4 */
            "vs_3_0\n"
            "texldl r0, v0, s0\n",
            {0xfffe0300, 0x0300005f, 0x800f0000, 0x90e40000, 0xa0e40800, 0x0000ffff}
        },
        {   /* shader 5 */
            "vs_3_0\n"
            "mov r0, c0[aL]\n",
            {0xfffe0300, 0x03000001, 0x800f0000, 0xa0e42000, 0xf0e40800, 0x0000ffff}
        },
        {   /* shader 6 */
            "vs_3_0\n"
            "mov o[ a0.x + 12 ], r0\n",
            {0xfffe0300, 0x03000001, 0xe00f200c, 0xb0000000, 0x80e40000, 0x0000ffff}
        },
        {   /* shader 7 */
            "vs_3_0\n"
            "add_sat r0, r0, r1\n",
            {0xfffe0300, 0x03000002, 0x801f0000, 0x80e40000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 8 */
            "vs_3_0\n"
            "mov r2, r1_abs\n",
            {0xfffe0300, 0x02000001, 0x800f0002, 0x8be40001, 0x0000ffff}
        },
        {   /* shader 9 */
            "vs_3_0\n"
            "mov r2, r1.xygb\n",
            {0xfffe0300, 0x02000001, 0x800f0002, 0x80940001, 0x0000ffff}
        },
        {   /* shader 10 */
            "vs_3_0\n"
            "mov r2.xyb, r1\n",
            {0xfffe0300, 0x02000001, 0x80070002, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 11 */
            "vs_3_0\n"
            "mova_sat a0.x, r1\n",
            {0xfffe0300, 0x0200002e, 0xb0110000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 12 */
            "vs_3_0\n"
            "sincos r0, r1\n",
            {0xfffe0300, 0x02000025, 0x800f0000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 13 */
            "vs_3_0\n"
            "def c0, 1.0f, 1.0f, 1.0f, 0.5f\n",
            {0xfffe0300, 0x05000051, 0xa00f0000, 0x3f800000, 0x3f800000, 0x3f800000,
             0x3f000000, 0x0000ffff}
        },
        {   /* shader 14: no register number checks with relative addressing */
            "vs.3.0\n"
            "add r0, v20[aL], r2\n",
            {0xfffe0300, 0x04000002, 0x800f0000, 0x90e42014, 0xf0e40800, 0x80e40002,
             0x0000ffff}
        },
        {   /* shader 15 */
            "vs.3.0\n"
            "add r0, v0[aL + 1 + 3], r2\n",
            {0xfffe0300, 0x04000002, 0x800f0000, 0x90e42004, 0xf0e40800, 0x80e40002,
             0x0000ffff}
        },
    };

    exec_tests("vs_3_0", tests, ARRAY_SIZE(tests));
}

static void ps_3_0_test(void) {
    struct shader_test tests[] = {
        {   /* shader 0 */
            "ps_3_0\n"
            "mov r0, c0\n",
            {0xffff0300, 0x02000001, 0x800f0000, 0xa0e40000, 0x0000ffff}
        },
        {   /* shader 1 */
            "ps_3_0\n"
            "dcl_normal5 v0\n",
            {0xffff0300, 0x0200001f, 0x80050003, 0x900f0000, 0x0000ffff}
        },
        {   /* shader 2 */
            "ps_3_0\n"
            "mov r0, vPos\n",
            {0xffff0300, 0x02000001, 0x800f0000, 0x90e41000, 0x0000ffff}
        },
        {   /* shader 3 */
            "ps_3_0\n"
            "mov r0, vFace\n",
            {0xffff0300, 0x02000001, 0x800f0000, 0x90e41001, 0x0000ffff}
        },
        {   /* shader 4 */
            "ps_3_0\n"
            "mov r0, v[ aL + 12 ]\n",
            {0xffff0300, 0x03000001, 0x800f0000, 0x90e4200c, 0xf0e40800, 0x0000ffff}
        },
        {   /* shader 5 */
            "ps_3_0\n"
            "loop aL, i0\n"
            "mov r0, v0[aL]\n"
            "endloop\n",
            {0xffff0300, 0x0200001b, 0xf0e40800, 0xf0e40000, 0x03000001, 0x800f0000,
             0x90e42000, 0xf0e40800, 0x0000001d, 0x0000ffff}
        },
        {   /* shader 6 */
            "ps_3_0\n"
            "texldl r0, v0, s0\n",
            {0xffff0300, 0x0300005f, 0x800f0000, 0x90e40000, 0xa0e40800, 0x0000ffff}
        },
        {   /* shader 7 */
            "ps_3_0\n"
            "add_pp r0, r0, r1\n",
            {0xffff0300, 0x03000002, 0x802f0000, 0x80e40000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 8 */
            "ps_3_0\n"
            "dsx_sat r0, r1\n",
            {0xffff0300, 0x0200005b, 0x801f0000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 9 */
            "ps_3_0\n"
            "texldd_pp r0, r1, r2, r3, r4\n",
            {0xffff0300, 0x0500005d, 0x802f0000, 0x80e40001, 0x80e40002, 0x80e40003,
             0x80e40004, 0x0000ffff}
        },
        {   /* shader 10 */
            "ps_3_0\n"
            "texkill v0\n",
            {0xffff0300, 0x01000041, 0x900f0000, 0x0000ffff}
        },
        {   /* shader 11 */
            "ps_3_0\n"
            "add oC3, r0, r1\n",
            {0xffff0300, 0x03000002, 0x800f0803, 0x80e40000, 0x80e40001, 0x0000ffff}
        },
        {   /* shader 12 */
            "ps_3_0\n"
            "dcl_texcoord0_centroid v0\n",
            {0xffff0300, 0x0200001f, 0x80000005, 0x904f0000, 0x0000ffff}
        },
        {   /* shader 13 */
            "ps_3_0\n"
            "dcl_2d_centroid s0\n",
            {0xffff0300, 0x0200001f, 0x90000000, 0xa04f0800, 0x0000ffff}
        },
        {   /* shader 14 */
            "ps.3.0\n"
            "dcl_2d_pp s0\n",
            {0xffff0300, 0x0200001f, 0x90000000, 0xa02f0800, 0x0000ffff}
        },
    };

    exec_tests("ps_3_0", tests, ARRAY_SIZE(tests));
}

static void failure_test(void) {
    const char * tests[] = {
        /* shader 0: instruction modifier not allowed */
        "ps_3_0\n"
        "dcl_2d s2\n"
        "texldd_x2 r0, v1, s2, v3, v4\n",
        /* shader 1: coissue not supported in vertex shaders */
        "vs.1.1\r\n"
        "add r0.rgb, r0, r1\n"
        "+add r0.a, r0, r2\n",
        /* shader 2: coissue not supported in pixel shader version >= 2.0 */
        "ps_2_0\n"
        "texld r0, t0, s0\n"
        "add r0.rgb, r0, r1\n"
        "+add r0.a, r0, v1\n",
        /* shader 3: predicates not supported in vertex shader < 2.0 */
        "vs_1_1\n"
        "(p0) add r0, r0, v0\n",
        /* shader 4: register a0 doesn't exist in pixel shaders */
        "ps_3_0\n"
        "mov r0, v[ a0 + 12 ]\n",
        /* shader 5: s0 doesn't exist in vs_1_1 */
        "vs_1_1\n"
        "mov r0, s0\n",
        /* shader 6: aL is a scalar register, no swizzles allowed */
        "ps_3_0\n"
        "mov r0, v[ aL.x + 12 ]\n",
        /* shader 7: tn doesn't exist in ps_3_0 */
        "ps_3_0\n"
        "dcl_2d s2\n"
        "texldd r0, t1, s2, v3, v4\n",
        /* shader 8: two shift modifiers */
        "ps_1_3\n"
        "mov_x2_x2 r0, r1\n",
        /* shader 9: too many source registers for mov instruction */
        "vs_1_1\n"
        "mov r0, r1, r2\n",
        /* shader 10: invalid combination of negate and divide modifiers */
        "ps_1_4\n"
        "texld r5, -r2_dz\n",
        /* shader 11: complement modifier not allowed in >= PS 2 */
        "ps_2_0\n"
        "mov r2, 1 - r0\n",
        /* shader 12: invalid modifier */
        "vs_3_0\n"
        "mov r2, 2 - r0\n",
        /* shader 13: float value in relative addressing */
        "vs_3_0\n"
        "mov r2, c[ aL + 3.4 ]\n",
        /* shader 14: complement modifier not available in VS */
        "vs_3_0\n"
        "mov r2, 1 - r1\n",
        /* shader 15: _x2 modifier not available in VS */
        "vs_1_1\n"
        "mov r2, r1_x2\n",
        /* shader 16: _abs modifier not available in < VS 3.0 */
        "vs_1_1\n"
        "mov r2, r1_abs\n",
        /* shader 17: _x2 modifier not available in >= PS 2.0 */
        "ps_2_0\n"
        "mov r0, r1_x2\n",
        /* shader 18: wrong swizzle */
        "vs_2_0\n"
        "mov r0, r1.abcd\n",
        /* shader 19: wrong swizzle */
        "vs_2_0\n"
        "mov r0, r1.xyzwx\n",
        /* shader 20: wrong swizzle */
        "vs_2_0\n"
        "mov r0, r1.\n",
        /* shader 21: invalid writemask */
        "vs_2_0\n"
        "mov r0.xxyz, r1\n",
        /* shader 22: register r5 doesn't exist in PS < 1.4 */
        "ps_1_3\n"
        "mov r5, r0\n",
        /* shader 23: can't declare output registers in a pixel shader */
        "ps_3_0\n"
        "dcl_positiont o0\n",
        /* shader 24: _pp instruction modifier not allowed in vertex shaders */
        "vs_3_0\n"
        "add_pp r0, r0, r1\n",
        /* shader 25: _x4 instruction modified not allowed in > ps_1_x */
        "ps_3_0\n"
        "add_x4 r0, r0, r1\n",
        /* shader 26: there aren't oCx registers in ps_1_x */
        "ps_1_3\n"
        "add oC0, r0, r1\n",
        /* shader 27: oC3 is the max in >= ps_2_0 */
        "ps_3_0\n"
        "add oC4, r0, r1\n",
        /* shader 28: register v17 doesn't exist */
        "vs_3_0\n"
        "add r0, r0, v17\n",
        /* shader 29: register o13 doesn't exist */
        "vs_3_0\n"
        "add o13, r0, r1\n",
        /* shader 30: label > 2047 not allowed */
        "vs_3_0\n"
        "call l2048\n",
        /* shader 31: s20 register does not exist */
        "ps_3_0\n"
        "texld r0, r1, s20\n",
        /* shader 32: t5 not allowed in ps_1_3 */
        "ps_1_3\n"
        "tex t5\n",
        /* shader 33: no temporary registers relative addressing */
        "vs_3_0\n"
        "add r0, r0[ a0.x ], r1\n",
        /* shader 34: no input registers relative addressing in vs_2_0 */
        "vs_2_0\n"
        "add r0, v[ a0.x ], r1\n",
        /* shader 35: no aL register in ps_2_0 */
        "ps_2_0\n"
        "add r0, v[ aL ], r1\n",
        /* shader 36: no relative addressing in ps_2_0 */
        "ps_2_0\n"
        "add r0, v[ r0 ], r1\n",
        /* shader 37: no a0 register in ps_3_0 */
        "ps_3_0\n"
        "add r0, v[ a0.x ], r1\n",
        /* shader 38: only a0.x accepted in vs_1_1 */
        "vs_1_1\n"
        "mov r0, c0[ a0 ]\n",
        /* shader 39: invalid modifier for dcl instruction */
        "ps_3_0\n"
        "dcl_texcoord0_sat v0\n",
        /* shader 40: shift not allowed */
        "ps_3_0\n"
        "dcl_texcoord0_x2 v0\n",
        /* shader 41: no modifier allowed with dcl instruction in vs */
        "vs_3_0\n"
        "dcl_texcoord0_centroid v0\n",
        /* shader 42: no modifiers with vs dcl sampler instruction */
        "vs_3_0\n"
        "dcl_2d_pp s0\n",
        /* shader 43: */
        "ps_2_0\n"
        "texm3x3vspec t3, t0\n",
        /* shader 44: expression in defi not allowed */
        "vs_2_0\n"
        "defi i0, -1 - 1, 1, 10, 0\n"
        "defi i1, 0, 40, 30, 10\n",
        /* shader 45: '-' not allowed inside relative addressing operands */
        "vs.3.0\n"
        "add r0, v0[aL - 3 + 5], r2\n",
        /* shader 46: float constants in defi */
        "vs_2_0\n"
        "defi i0, 1.0, 1.1, 10.2, 0.3\n"
        "defi i1, 0, 40, 30, 10\n",
        /* shader 47: double '-' sign */
        "vs.3.0\n"
        "defi c0, -1, --1, 0, 0\n",
    };
    HRESULT hr;
    unsigned int i;
    ID3DBlob *shader, *messages;

    for(i = 0; i < ARRAY_SIZE(tests); i++)
    {
        shader = NULL;
        messages = NULL;
        hr = pD3DAssemble(tests[i], strlen(tests[i]), NULL, NULL, NULL, D3DCOMPILE_SKIP_VALIDATION, &shader, &messages);
        ok(hr == D3DXERR_INVALIDDATA, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        if (messages)
        {
            trace("D3DAssemble messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
            ID3D10Blob_Release(messages);
        }
        if(shader) {
            DWORD *res = ID3D10Blob_GetBufferPointer(shader);
            dump_shader(res);
            ID3D10Blob_Release(shader);
        }
    }
}

static HRESULT WINAPI testD3DInclude_open(ID3DInclude *iface, D3D_INCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    static const char include[] = "#define REGISTER r0\nvs.1.1\n";
    static const char include2[] = "#include \"incl3.vsh\"\n";
    static const char include3[] = "vs.1.1\n";
    static const char include4[] = "#include <incl3.vsh>\n";
    char *buffer;

    trace("include_type = %d, filename %s\n", include_type, filename);
    trace("parent_data (%p) -> %s\n", parent_data, parent_data ? (char *)parent_data : "(null)");

    if (!strcmp(filename, "incl.vsh"))
    {
        buffer = strdup(include);
        *bytes = sizeof(include);
        ok(!parent_data, "Wrong parent_data value.\n");
    }
    else if (!strcmp(filename, "incl2.vsh"))
    {
        buffer = strdup(include2);
        *bytes = sizeof(include2);
        ok(!parent_data, "Wrong parent_data value.\n");
        ok(include_type == D3D_INCLUDE_LOCAL, "Wrong include type %d.\n", include_type);
    }
    else if (!strcmp(filename, "incl3.vsh"))
    {
        buffer = strdup(include3);
        *bytes = sizeof(include3);
        /* Also check for the correct parent_data content */
        ok(parent_data != NULL
                && (!strncmp(include2, parent_data, strlen(include2))
                || !strncmp(include4, parent_data, strlen(include4))),
                "Wrong parent_data value.\n");
    }
    else if (!strcmp(filename, "incl4.vsh"))
    {
        buffer = strdup(include4);
        *bytes = sizeof(include4);
        ok(parent_data == NULL, "Wrong parent_data value.\n");
        ok(include_type == D3D_INCLUDE_SYSTEM, "Wrong include type %d.\n", include_type);
    }
    else if (!strcmp(filename, "includes/incl.vsh"))
    {
        buffer = strdup(include);
        *bytes = sizeof(include);
        ok(!parent_data, "Wrong parent_data value.\n");
    }
    else
    {
        ok(FALSE, "Unexpected file %s included.\n", filename);
        return E_FAIL;
    }

    *data = buffer;

    return S_OK;
}

static HRESULT WINAPI testD3DInclude_close(ID3DInclude *iface, const void *data)
{
    free((void *)data);
    return S_OK;
}

static const struct ID3DIncludeVtbl D3DInclude_Vtbl =
{
    testD3DInclude_open,
    testD3DInclude_close
};

struct D3DIncludeImpl {
    ID3DInclude ID3DInclude_iface;
};

static void assembleshader_test(void) {
    static const char test1[] =
    {
        "vs.1.1\n"
        "mov DEF2, v0\n"
    };
    static const char testshader[] =
    {
        "#include \"incl.vsh\"\n"
        "mov REGISTER, v0\n"
    };
    static const D3D_SHADER_MACRO defines[] =
    {
        {
            "DEF1", "10 + 15"
        },
        {
            "DEF2", "r0"
        },
        {
            NULL, NULL
        }
    };
    HRESULT hr;
    ID3DBlob *shader, *messages;
    struct D3DIncludeImpl include;

    /* defines test */
    shader = NULL;
    messages = NULL;
    hr = pD3DAssemble(test1, strlen(test1), NULL, defines, NULL, D3DCOMPILE_SKIP_VALIDATION, &shader, &messages);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DAssemble messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }
    if(shader) ID3D10Blob_Release(shader);

    /* NULL messages test */
    shader = NULL;
    hr = pD3DAssemble(test1, strlen(test1), NULL, defines, NULL, D3DCOMPILE_SKIP_VALIDATION, &shader, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (shader)
        ID3D10Blob_Release(shader);

    /* NULL shader test */
    messages = NULL;
    hr = pD3DAssemble(test1, strlen(test1), NULL, defines, NULL, D3DCOMPILE_SKIP_VALIDATION, NULL, &messages);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DAssemble messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }

    /* D3DInclude test */
    shader = NULL;
    messages = NULL;
    include.ID3DInclude_iface.lpVtbl = &D3DInclude_Vtbl;
    hr = pD3DAssemble(testshader, strlen(testshader), NULL, NULL,
            &include.ID3DInclude_iface, D3DCOMPILE_SKIP_VALIDATION, &shader, &messages);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DAssemble messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }
    if(shader) ID3D10Blob_Release(shader);

    /* NULL shader tests */
    shader = NULL;
    messages = NULL;
    hr = pD3DAssemble(NULL, 0, NULL, NULL, NULL, D3DCOMPILE_SKIP_VALIDATION, &shader, &messages);
    ok(hr == D3DXERR_INVALIDDATA, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DAssemble messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }
    if(shader) ID3D10Blob_Release(shader);
}

static void d3dpreprocess_test(void)
{
    static const char test1[] =
    {
        "vs.1.1\n"
        "mov DEF2, v0\n"
    };
    static const char quotation_marks_test[] =
    {
        "vs.1.1\n"
        "; ' comment\n"
        "; \" comment\n"
        "mov 0, v0\n"
    };
    static const char *include_test_shaders[] =
    {
        "#include \"incl.vsh\"\n"
        "mov REGISTER, v0\n",

        "#include \"incl2.vsh\"\n"
        "mov REGISTER, v0\n",

        "#include <incl.vsh>\n"
        "mov REGISTER, v0\n",

        "#include <incl4.vsh>\n"
        "mov REGISTER, v0\n",

        "#include \"includes/incl.vsh\"\n"
        "mov REGISTER, v0\n"
    };
    HRESULT hr;
    ID3DBlob *shader, *messages;
    static const D3D_SHADER_MACRO defines[] =
    {
        {
            "DEF1", "10 + 15"
        },
        {
            "DEF2", "r0"
        },
        {
            NULL, NULL
        }
    };
    struct D3DIncludeImpl include;
    unsigned int i;

    /* pDefines test */
    shader = NULL;
    messages = NULL;
    hr = D3DPreprocess(test1, strlen(test1), NULL, defines, NULL, &shader, &messages);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DPreprocess messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }
    if (shader) ID3D10Blob_Release(shader);

    /* NULL messages test */
    shader = NULL;
    hr = D3DPreprocess(test1, strlen(test1), NULL, defines, NULL, &shader, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (shader)
        ID3D10Blob_Release(shader);

    /* NULL shader test */
    messages = NULL;
    hr = D3DPreprocess(test1, strlen(test1), NULL, defines, NULL, NULL, &messages);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DPreprocess messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }

    /* quotation marks test */
    shader = NULL;
    messages = NULL;
    hr = D3DPreprocess(quotation_marks_test, strlen(quotation_marks_test), NULL, NULL, NULL, &shader, &messages);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DPreprocess messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }
    if (shader) ID3D10Blob_Release(shader);

    /* pInclude tests */
    include.ID3DInclude_iface.lpVtbl = &D3DInclude_Vtbl;
    for (i = 0; i < ARRAY_SIZE(include_test_shaders); ++i)
    {
        shader = NULL;
        messages = NULL;
        hr = D3DPreprocess(include_test_shaders[i], strlen(include_test_shaders[i]), NULL, NULL,
                &include.ID3DInclude_iface, &shader, &messages);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        if (messages)
        {
            trace("D3DPreprocess messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
            ID3D10Blob_Release(messages);
        }
        if (shader) ID3D10Blob_Release(shader);
    }

    /* NULL shader tests */
    shader = NULL;
    messages = NULL;
    hr = D3DPreprocess(NULL, 0, NULL, NULL, NULL, &shader, &messages);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    if (messages)
    {
        trace("D3DPreprocess messages:\n%s", (char *)ID3D10Blob_GetBufferPointer(messages));
        ID3D10Blob_Release(messages);
    }
    if (shader) ID3D10Blob_Release(shader);
}

static const DWORD vs_2_0[] =
{
    0xfffe0200,                         /* vs_2_0 */
    0x0200001f, 0x80000000, 0x900f0000, /* dcl_position v0 */
    0x0200001f, 0x80000003, 0x900f0001, /* dcl_normal v1 */
    0x0200001f, 0x8001000a, 0x900f0002, /* dcl_color1 v2 */
    0x0200001f, 0x80000005, 0x900f0003, /* dcl_texcoord0 v3 */
    0x02000001, 0xc00f0000, 0x90e40000, /* mov oPos, v0 */
    0x02000001, 0xd00f0001, 0x90e40002, /* mov oD1, v2 */
    0x02000001, 0xe0070000, 0x90e40003, /* mov oT0.xyz, v3 */
    0x02000001, 0xc00f0001, 0x90ff0002, /* mov oFog, v2.w */
    0x02000001, 0xc00f0002, 0x90ff0001, /* mov oPts, v1.w */
    0x0000ffff
};

static void test_disassemble_shader(void)
{
    ID3DBlob *blob;
    HRESULT hr;

    hr = D3DDisassemble(vs_2_0, 0, 0, NULL, &blob);
#if D3D_COMPILER_VERSION >= 46
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
#else
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
#endif

    hr = D3DDisassemble(vs_2_0, sizeof(vs_2_0), 0, NULL, &blob);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Blob_Release(blob);
}

START_TEST(asm)
{
    HMODULE d3dcompiler;
    char buffer[20];

    sprintf(buffer, "d3dcompiler_%d", D3D_COMPILER_VERSION);
    d3dcompiler = GetModuleHandleA(buffer);
    pD3DAssemble = (void *)GetProcAddress(d3dcompiler, "D3DAssemble");

    preproc_test();
    ps_1_1_test();
    vs_1_1_test();
    ps_1_3_test();
    ps_1_4_test();
    vs_2_0_test();
    vs_2_x_test();
    ps_2_0_test();
    ps_2_x_test();
    vs_3_0_test();
    ps_3_0_test();

    failure_test();

    assembleshader_test();

    d3dpreprocess_test();
    test_disassemble_shader();
}
