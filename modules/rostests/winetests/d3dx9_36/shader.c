/*
 * Copyright 2008 Luis Busquets
 * Copyright 2011 Travis Athougies
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

#include "wine/test.h"
#include "d3dx9.h"

#define FCC_TEXT MAKEFOURCC('T','E','X','T')
#define FCC_CTAB MAKEFOURCC('C','T','A','B')

static const DWORD shader_zero[] = {0x0};

static const DWORD shader_invalid[] = {0xeeee0100};

static const DWORD shader_empty[] = {0xfffe0200, 0x0000ffff};

static const DWORD simple_fx[] = {0x46580000, 0x0002fffe, FCC_TEXT, 0x00000000, 0x0000ffff};

static const DWORD simple_tx[] = {0x54580000, 0x0002fffe, FCC_TEXT, 0x00000000, 0x0000ffff};

static const DWORD simple_7ffe[] = {0x7ffe0000, 0x0002fffe, FCC_TEXT, 0x00000000, 0x0000ffff};

static const DWORD simple_7fff[] = {0x7fff0000, 0x0002fffe, FCC_TEXT, 0x00000000, 0x0000ffff};

static const DWORD simple_vs[] = {
    0xfffe0101,                                                             /* vs_1_1                       */
    0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position0 v0             */
    0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000,                         /* dp4 oPos.x, v0, c0           */
    0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001,                         /* dp4 oPos.y, v0, c1           */
    0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002,                         /* dp4 oPos.z, v0, c2           */
    0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003,                         /* dp4 oPos.w, v0, c3           */
    0x0000ffff};                                                            /* END                          */

static const DWORD simple_ps[] = {
    0xffff0101,                                                             /* ps_1_1                       */
    0x00000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
    0x00000042, 0xb00f0000,                                                 /* tex t0                       */
    0x00000008, 0x800f0000, 0xa0e40001, 0xa0e40000,                         /* dp3 r0, c1, c0               */
    0x00000005, 0x800f0000, 0x90e40000, 0x80e40000,                         /* mul r0, v0, r0               */
    0x00000005, 0x800f0000, 0xb0e40000, 0x80e40000,                         /* mul r0, t0, r0               */
    0x0000ffff};                                                            /* END                          */

static const DWORD shader_with_ctab[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0002fffe, FCC_TEXT,   0x00000000,                                     /* TEXT comment                 */
    0x0008fffe, FCC_CTAB,   0x0000001c, 0x00000010, 0xfffe0300, 0x00000000, /* CTAB comment                 */
                0x00000000, 0x00000000, 0x00000000,
    0x0004fffe, FCC_TEXT,   0x00000000, 0x00000000, 0x00000000,             /* TEXT comment                 */
    0x0000ffff};                                                            /* END                          */

static const DWORD shader_with_invalid_ctab[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0005fffe, FCC_CTAB,                                                   /* CTAB comment                 */
                0x0000001c, 0x000000a9, 0xfffe0300,
                0x00000000, 0x00000000,
    0x0000ffff};                                                            /* END                          */

static const DWORD shader_with_ctab_constants[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x002efffe, FCC_CTAB,                                                   /* CTAB comment                 */
    0x0000001c, 0x000000a4, 0xfffe0300, 0x00000003, 0x0000001c, 0x20008100, /* Header                       */
    0x0000009c,
    0x00000058, 0x00070002, 0x00000001, 0x00000064, 0x00000000,             /* Constant 1 desc              */
    0x00000074, 0x00000002, 0x00000004, 0x00000080, 0x00000000,             /* Constant 2 desc              */
    0x00000090, 0x00040002, 0x00000003, 0x00000080, 0x00000000,             /* Constant 3 desc              */
    0x736e6f43, 0x746e6174, 0xabab0031,                                     /* Constant 1 name string       */
    0x00030001, 0x00040001, 0x00000001, 0x00000000,                         /* Constant 1 type desc         */
    0x736e6f43, 0x746e6174, 0xabab0032,                                     /* Constant 2 name string       */
    0x00030003, 0x00040004, 0x00000001, 0x00000000,                         /* Constant 2 & 3 type desc     */
    0x736e6f43, 0x746e6174, 0xabab0033,                                     /* Constant 3 name string       */
    0x335f7376, 0xab00305f,                                                 /* Target name string           */
    0x656e6957, 0x6f727020, 0x7463656a, 0xababab00,                         /* Creator name string          */
    0x0000ffff};                                                            /* END                          */

static const DWORD ctab_basic[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0040fffe, FCC_CTAB,                                                   /* CTAB comment                 */
    0x0000001c, 0x000000ec, 0xfffe0300, 0x00000005, 0x0000001c, 0x20008100, /* Header                       */
    0x000000e4,
    0x00000080, 0x00060002, 0x00000001, 0x00000084, 0x00000000,             /* Constant 1 desc (f)          */
    0x00000094, 0x00070002, 0x00000001, 0x00000098, 0x00000000,             /* Constant 2 desc (f4)         */
    0x000000A8, 0x00040002, 0x00000001, 0x000000AC, 0x00000000,             /* Constant 3 desc (i)          */
    0x000000BC, 0x00050002, 0x00000001, 0x000000C0, 0x00000000,             /* Constant 4 desc (i4)         */
    0x000000D0, 0x00000002, 0x00000004, 0x000000D4, 0x00000000,             /* Constant 5 desc (mvp)        */
    0xabab0066, 0x00030000, 0x00010001, 0x00000001, 0x00000000,             /* Constant 1 name/type desc    */
    0xab003466, 0x00030001, 0x00040001, 0x00000001, 0x00000000,             /* Constant 2 name/type desc    */
    0xabab0069, 0x00020000, 0x00010001, 0x00000001, 0x00000000,             /* Constant 3 name/type desc    */
    0xab003469, 0x00020001, 0x00040001, 0x00000001, 0x00000000,             /* Constant 4 name/type desc    */
    0x0070766d, 0x00030003, 0x00040004, 0x00000001, 0x00000000,             /* Constant 5 name/type desc    */
    0x335f7376, 0xab00305f,                                                 /* Target name string           */
    0x656e6957, 0x6f727020, 0x7463656a, 0xababab00,                         /* Creator name string          */
    0x0000ffff};                                                            /* END                          */

static const D3DXCONSTANT_DESC ctab_basic_expected[] = {
    {"mvp", D3DXRS_FLOAT4, 0, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 1, 0, 64, NULL},
    {"i",   D3DXRS_FLOAT4, 4, 1, D3DXPC_SCALAR,         D3DXPT_INT,   1, 1, 1, 0,  4, NULL},
    {"i4",  D3DXRS_FLOAT4, 5, 1, D3DXPC_VECTOR,         D3DXPT_INT,   1, 4, 1, 0, 16, NULL},
    {"f",   D3DXRS_FLOAT4, 6, 1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1, 1, 1, 0,  4, NULL},
    {"f4",  D3DXRS_FLOAT4, 7, 1, D3DXPC_VECTOR,         D3DXPT_FLOAT, 1, 4, 1, 0, 16, NULL}};

static const DWORD ctab_matrices[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0032fffe, FCC_CTAB,                                                   /* CTAB comment                 */
    0x0000001c, 0x000000b4, 0xfffe0300, 0x00000003, 0x0000001c, 0x20008100, /* Header                       */
    0x000000ac,
    0x00000058, 0x00070002, 0x00000001, 0x00000064, 0x00000000,             /* Constant 1 desc (fmatrix3x1) */
    0x00000074, 0x00000002, 0x00000004, 0x00000080, 0x00000000,             /* Constant 2 desc (fmatrix4x4) */
    0x00000090, 0x00040002, 0x00000002, 0x0000009c, 0x00000000,             /* Constant 3 desc (imatrix2x3) */
    0x74616D66, 0x33786972, 0xab003178,                                     /* Constant 1 name              */
    0x00030003, 0x00010003, 0x00000001, 0x00000000,                         /* Constant 1 type desc         */
    0x74616D66, 0x34786972, 0xab003478,                                     /* Constant 2 name              */
    0x00030003, 0x00040004, 0x00000001, 0x00000000,                         /* Constant 2 type desc         */
    0x74616D69, 0x32786972, 0xab003378,                                     /* Constant 3 name              */
    0x00020002, 0x00030002, 0x00000001, 0x00000000,                         /* Constant 3 type desc         */
    0x335f7376, 0xab00305f,                                                 /* Target name string           */
    0x656e6957, 0x6f727020, 0x7463656a, 0xababab00,                         /* Creator name string          */
    0x0000ffff};                                                            /* END                          */

static const D3DXCONSTANT_DESC ctab_matrices_expected[] = {
    {"fmatrix4x4", D3DXRS_FLOAT4, 0, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 1, 0, 64, NULL},
    {"imatrix2x3", D3DXRS_FLOAT4, 4, 2, D3DXPC_MATRIX_ROWS,    D3DXPT_INT,   2, 3, 1, 0, 24, NULL},
    {"fmatrix3x1", D3DXRS_FLOAT4, 7, 1, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 1, 1, 0, 12, NULL},
};

static const DWORD ctab_matrices2[] = {
    0xfffe0200,                                                             /* vs_2_0                        */
    0x0058fffe, FCC_CTAB,                                                   /* CTAB comment                  */
    0x0000001c, 0x0000012b, 0xfffe0200, 0x00000006, 0x0000001c, 0x00000100, /* Header                        */
    0x00000124,
    0x00000094, 0x00070002, 0x00000003, 0x0000009c, 0x00000000,             /* Constant 1 desc (c2x3)        */
    0x000000ac, 0x000d0002, 0x00000002, 0x000000b4, 0x00000000,             /* Constant 2 desc (c3x2)        */
    0x000000c4, 0x000a0002, 0x00000003, 0x000000cc, 0x00000000,             /* Constant 3 desc (c3x3)        */
    0x000000dc, 0x000f0002, 0x00000002, 0x000000e4, 0x00000000,             /* Constant 4 desc (r2x3)        */
    0x000000f4, 0x00040002, 0x00000003, 0x000000fc, 0x00000000,             /* Constant 5 desc (r3x2)        */
    0x0000010c, 0x00000002, 0x00000004, 0x00000114, 0x00000000,             /* Constant 6 desc (r4x4)        */
    0x33783263, 0xababab00,                                                 /* Constant 1 name               */
    0x00030003, 0x00030002, 0x00000001, 0x00000000,                         /* Constant 1 type desc          */
    0x32783363, 0xababab00,                                                 /* Constant 2 name               */
    0x00030003, 0x00020003, 0x00000001, 0x00000000,                         /* Constant 2 type desc          */
    0x33783363, 0xababab00,                                                 /* Constant 3 name               */
    0x00030003, 0x00030003, 0x00000001, 0x00000000,                         /* Constant 3 type desc          */
    0x33783272, 0xababab00,                                                 /* Constant 4 name               */
    0x00030002, 0x00030002, 0x00000001, 0x00000000,                         /* Constant 4 type desc          */
    0x32783372, 0xababab00,                                                 /* Constant 5 name               */
    0x00030002, 0x00020003, 0x00000001, 0x00000000,                         /* Constant 5 type desc          */
    0x34783472, 0xababab00,                                                 /* Constant 6 name               */
    0x00030002, 0x00040004, 0x00000001, 0x00000000,                         /* Constant 6 type desc          */
    0x325f7376, 0x4100305f, 0x41414141, 0x00414141,                         /* Target and Creator name       */
    0x0000ffff};                                                            /* END                           */

static const D3DXCONSTANT_DESC ctab_matrices2_expected[] = {
    {"c2x3", D3DXRS_FLOAT4,  7, 3, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 2, 3, 1, 0, 24, NULL},
    {"c3x2", D3DXRS_FLOAT4, 13, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL},
    {"c3x3", D3DXRS_FLOAT4, 10, 3, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 3, 1, 0, 36, NULL},
    {"r2x3", D3DXRS_FLOAT4, 15, 2, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 2, 3, 1, 0, 24, NULL},
    {"r3x2", D3DXRS_FLOAT4,  4, 3, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL},
    {"r4x4", D3DXRS_FLOAT4,  0, 4, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 4, 4, 1, 0, 64, NULL}};

static const DWORD ctab_arrays[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0052fffe, FCC_CTAB,                                                   /* CTAB comment                 */
    0x0000001c, 0x0000013c, 0xfffe0300, 0x00000006, 0x0000001c, 0x20008100, /* Header                       */
    0x00000134,
    0x00000094, 0x000E0002, 0x00000002, 0x0000009c, 0x00000000,             /* Constant 1 desc (barray)     */
    0x000000ac, 0x00100002, 0x00000002, 0x000000b8, 0x00000000,             /* Constant 2 desc (bvecarray)  */
    0x000000c8, 0x00080002, 0x00000004, 0x000000d0, 0x00000000,             /* Constant 3 desc (farray)     */
    0x000000e0, 0x00000002, 0x00000008, 0x000000ec, 0x00000000,             /* Constant 4 desc (fmtxarray)  */
    0x000000fc, 0x000C0002, 0x00000002, 0x00000108, 0x00000000,             /* Constant 5 desc (fvecarray)  */
    0x00000118, 0x00120002, 0x00000001, 0x00000124, 0x00000000,             /* Constant 6 desc (ivecarray)  */
    0x72726162, 0xab007961,                                                 /* Constant 1 name              */
    0x00010000, 0x00010001, 0x00000002, 0x00000000,                         /* Constant 1 type desc         */
    0x63657662, 0x61727261, 0xabab0079,                                     /* Constant 2 name              */
    0x00010001, 0x00030001, 0x00000003, 0x00000000,                         /* Constant 2 type desc         */
    0x72726166, 0xab007961,                                                 /* Constant 3 name              */
    0x00030000, 0x00010001, 0x00000004, 0x00000000,                         /* constant 3 type desc         */
    0x78746d66, 0x61727261, 0xabab0079,                                     /* Constant 4 name              */
    0x00030002, 0x00040004, 0x00000002, 0x00000000,                         /* Constant 4 type desc         */
    0x63657666, 0x61727261, 0xabab0079,                                     /* Constant 5 name              */
    0x00030001, 0x00040001, 0x00000002, 0x00000000,                         /* Constant 5 type desc         */
    0x63657669, 0x61727261, 0xabab0079,                                     /* Constant 6 name              */
    0x00020001, 0x00040001, 0x00000001, 0x00000000,                         /* Constant 6 type desc         */
    0x335f7376, 0xab00305f,                                                 /* Target name string           */
    0x656e6957, 0x6f727020, 0x7463656a, 0xababab00,                         /* Creator name string          */
    0x0000ffff};                                                            /* END                          */

static const D3DXCONSTANT_DESC ctab_arrays_expected[] = {
    {"fmtxarray", D3DXRS_FLOAT4,  0, 8, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 4, 2, 0, 128, NULL},
    {"farray",    D3DXRS_FLOAT4,  8, 4, D3DXPC_SCALAR,      D3DXPT_FLOAT, 1, 1, 4, 0,  16, NULL},
    {"fvecarray", D3DXRS_FLOAT4, 12, 2, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 4, 2, 0,  32, NULL},
    {"barray",    D3DXRS_FLOAT4, 14, 2, D3DXPC_SCALAR,      D3DXPT_BOOL,  1, 1, 2, 0,   8, NULL},
    {"bvecarray", D3DXRS_FLOAT4, 16, 2, D3DXPC_VECTOR,      D3DXPT_BOOL,  1, 3, 3, 0,  36, NULL},
    {"ivecarray", D3DXRS_FLOAT4, 18, 1, D3DXPC_VECTOR,      D3DXPT_INT,   1, 4, 1, 0,  16, NULL}};

static const DWORD ctab_with_default_values[] = {
    0xfffe0200,                                                 /* vs_2_0 */
    0x007bfffe, FCC_CTAB,                                       /* CTAB comment */
    0x0000001c, 0x000001b7, 0xfffe0200, 0x00000005, 0x0000001c, /* header */
    0x00000100, 0x000001b0,
    0x00000080, 0x00080002, 0x00000003, 0x00000084, 0x00000094, /* constant 1 desc (arr) */
    0x000000c4, 0x000c0002, 0x00000001, 0x000000c8, 0x000000d8, /* constant 2 desc (flt) */
    0x000000e8, 0x00040002, 0x00000004, 0x000000f0, 0x00000100, /* constant 3 desc (mat3) */
    0x00000140, 0x00000002, 0x00000004, 0x000000f0, 0x00000148, /* constant 4 desc (mat4) */
    0x00000188, 0x000b0002, 0x00000001, 0x00000190, 0x000001a0, /* constant 5 desc (vec4) */
    0x00727261,                                                 /* constant 1 name */
    0x00030000, 0x00010001, 0x00000003, 0x00000000,             /* constant 1 type desc */
    0x42c80000, 0x00000000, 0x00000000, 0x00000000,             /* constant 1 default value */
    0x43480000, 0x00000000, 0x00000000, 0x00000000,
    0x43960000, 0x00000000, 0x00000000, 0x00000000,
    0x00746c66,                                                 /* constant 2 name */
    0x00030000, 0x00010001, 0x00000001, 0x00000000,             /* constant 2 type desc */
    0x411fd70a, 0x00000000, 0x00000000, 0x00000000,             /* constant 2 default value */
    0x3374616d,                                                 /* constant 3 name */
    0xababab00,
    0x00030003, 0x00040004, 0x00000001, 0x00000000,             /* constant 3 & 4 type desc */
    0x41300000, 0x425c0000, 0x42c60000, 0x44a42000,             /* constat 3 default value */
    0x41b00000, 0x42840000, 0x447c8000, 0x44b0c000,
    0x42040000, 0x429a0000, 0x448ae000, 0x44bd6000,
    0x42300000, 0x42b00000, 0x44978000, 0x44ca0000,
    0x3474616d,                                                 /* constant 4 name */
    0xababab00,
    0x3f800000, 0x40a00000, 0x41100000, 0x41500000,             /* constant 4 default value */
    0x40000000, 0x40c00000, 0x41200000, 0x41600000,
    0x40400000, 0x40e00000, 0x41300000, 0x41700000,
    0x40800000, 0x41000000, 0x41400000, 0x41800000,
    0x34636576,                                                 /* constant 5 name */
    0xababab00,
    0x00030001, 0x00040001, 0x00000001, 0x00000000,             /* constant 5 type desc */
    0x41200000, 0x41a00000, 0x41f00000, 0x42200000,             /* constant 5 default value */
    0x325f7376, 0x4d004141, 0x41414141, 0x00000000,             /* target & creator string */
    0x0000ffff};                                                /* END */

static const float mat4_default_value[] = {1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16};
static const float mat3_default_value[] = {11, 55, 99, 1313, 22, 66, 1010, 1414, 33, 77, 1111, 1515, 44, 88, 1212, 1616};
static const float arr_default_value[] = {100, 0, 0, 0, 200, 0, 0, 0, 300, 0, 0, 0};
static const float vec4_default_value[] = {10, 20, 30, 40};
static const float flt_default_value[] = {9.99, 0, 0, 0};

static const D3DXCONSTANT_DESC ctab_with_default_values_expected[] = {
    {"mat4", D3DXRS_FLOAT4,  0, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 1, 0, 64, mat4_default_value},
    {"mat3", D3DXRS_FLOAT4,  4, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 1, 0, 64, mat3_default_value},
    {"arr",  D3DXRS_FLOAT4,  8, 3, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1, 1, 3, 0, 12, arr_default_value},
    {"vec4", D3DXRS_FLOAT4, 11, 1, D3DXPC_VECTOR,         D3DXPT_FLOAT, 1, 4, 1, 0, 16, vec4_default_value},
    {"flt",  D3DXRS_FLOAT4, 12, 1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1, 1, 1, 0,  4, flt_default_value}};

static const DWORD ctab_samplers[] = {
    0xfffe0300,                                                             /* vs_3_0                        */
    0x0032fffe, FCC_CTAB,                                                   /* CTAB comment                  */
    0x0000001c, 0x000000b4, 0xfffe0300, 0x00000003, 0x0000001c, 0x20008100, /* Header                        */
    0x000000ac,
    0x00000058, 0x00020002, 0x00000001, 0x00000064, 0x00000000,             /* Constant 1 desc (notsampler)  */
    0x00000074, 0x00000003, 0x00000001, 0x00000080, 0x00000000,             /* Constant 2 desc (sampler1)    */
    0x00000090, 0x00030003, 0x00000001, 0x0000009c, 0x00000000,             /* Constant 3 desc (sampler2)    */
    0x73746f6e, 0x6c706d61, 0xab007265,                                     /* Constant 1 name               */
    0x00030001, 0x00040001, 0x00000001, 0x00000000,                         /* Constant 1 type desc          */
    0x706d6173, 0x3172656c, 0xababab00,                                     /* Constant 2 name               */
    0x000c0004, 0x00010001, 0x00000001, 0x00000000,                         /* Constant 2 type desc          */
    0x706d6173, 0x3272656c, 0xababab00,                                     /* Constant 3 name               */
    0x000d0004, 0x00010001, 0x00000001, 0x00000000,                         /* Constant 3 type desc          */
    0x335f7376, 0xab00305f,                                                 /* Target name string            */
    0x656e6957, 0x6f727020, 0x7463656a, 0xababab00,                         /* Creator name string           */
    0x0000ffff};                                                            /* END                           */

static const D3DXCONSTANT_DESC ctab_samplers_expected[] = {
    {"sampler1",   D3DXRS_SAMPLER, 0, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER2D, 1, 1, 1, 0, 4,  NULL},
    {"sampler2",   D3DXRS_SAMPLER, 3, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER3D, 1, 1, 1, 0, 4,  NULL},
    {"notsampler", D3DXRS_FLOAT4,  2, 1, D3DXPC_VECTOR, D3DXPT_FLOAT,     1, 4, 1, 0, 16, NULL}};

static const DWORD fx_shader_with_ctab[] =
{
    0x46580200,                                                             /* FX20                     */
    0x002efffe, FCC_CTAB,                                                   /* CTAB comment             */
    0x0000001c, 0x000000a4, 0xfffe0300, 0x00000003, 0x0000001c, 0x20008100, /* Header                   */
    0x0000009c,
    0x00000058, 0x00070002, 0x00000001, 0x00000064, 0x00000000,             /* Constant 1 desc          */
    0x00000074, 0x00000002, 0x00000004, 0x00000080, 0x00000000,             /* Constant 2 desc          */
    0x00000090, 0x00040002, 0x00000003, 0x00000080, 0x00000000,             /* Constant 3 desc          */
    0x736e6f43, 0x746e6174, 0xabab0031,                                     /* Constant 1 name string   */
    0x00030001, 0x00040001, 0x00000001, 0x00000000,                         /* Constant 1 type desc     */
    0x736e6f43, 0x746e6174, 0xabab0032,                                     /* Constant 2 name string   */
    0x00030003, 0x00040004, 0x00000001, 0x00000000,                         /* Constant 2 & 3 type desc */
    0x736e6f43, 0x746e6174, 0xabab0033,                                     /* Constant 3 name string   */
    0x335f7376, 0xab00305f,                                                 /* Target name string       */
    0x656e6957, 0x6f727020, 0x7463656a, 0xababab00,                         /* Creator name string      */
    0x0000ffff                                                              /* END                      */
};

static void test_get_shader_size(void)
{
    UINT shader_size, expected;

    shader_size = D3DXGetShaderSize(simple_vs);
    expected = sizeof(simple_vs);
    ok(shader_size == expected, "Got shader size %u, expected %u\n", shader_size, expected);

    shader_size = D3DXGetShaderSize(simple_ps);
    expected = sizeof(simple_ps);
    ok(shader_size == expected, "Got shader size %u, expected %u\n", shader_size, expected);

    shader_size = D3DXGetShaderSize(NULL);
    ok(shader_size == 0, "Got shader size %u, expected 0\n", shader_size);
}

static void test_get_shader_version(void)
{
    DWORD shader_version;

    shader_version = D3DXGetShaderVersion(simple_vs);
    ok(shader_version == D3DVS_VERSION(1, 1), "Got shader version 0x%08x, expected 0x%08x\n",
            shader_version, D3DVS_VERSION(1, 1));

    shader_version = D3DXGetShaderVersion(simple_ps);
    ok(shader_version == D3DPS_VERSION(1, 1), "Got shader version 0x%08x, expected 0x%08x\n",
            shader_version, D3DPS_VERSION(1, 1));

    shader_version = D3DXGetShaderVersion(NULL);
    ok(shader_version == 0, "Got shader version 0x%08x, expected 0\n", shader_version);
}

static void test_find_shader_comment(void)
{
    const void *data = (void *)0xdeadbeef;
    HRESULT hr;
    UINT size = 100;

    hr = D3DXFindShaderComment(NULL, MAKEFOURCC('C','T','A','B'), &data, &size);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
    ok(!data, "Got %p, expected NULL\n", data);
    ok(!size, "Got %u, expected 0\n", size);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('C','T','A','B'), NULL, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(size == 28, "Got %u, expected 28\n", size);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('C','T','A','B'), &data, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(data == shader_with_ctab + 6, "Got result %p, expected %p\n", data, shader_with_ctab + 6);

    hr = D3DXFindShaderComment(shader_with_ctab, 0, &data, &size);
    ok(hr == S_FALSE, "Got result %x, expected 1 (S_FALSE)\n", hr);
    ok(!data, "Got %p, expected NULL\n", data);
    ok(!size, "Got %u, expected 0\n", size);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('X','X','X','X'), &data, &size);
    ok(hr == S_FALSE, "Got result %x, expected 1 (S_FALSE)\n", hr);
    ok(!data, "Got %p, expected NULL\n", data);
    ok(!size, "Got %u, expected 0\n", size);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('C','T','A','B'), &data, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(data == shader_with_ctab + 6, "Got result %p, expected %p\n", data, shader_with_ctab + 6);
    ok(size == 28, "Got result %u, expected 28\n", size);

    hr = D3DXFindShaderComment(shader_zero, MAKEFOURCC('C','T','A','B'), &data, &size);
    ok(hr == D3DXERR_INVALIDDATA, "Got result %x, expected %x (D3DXERR_INVALIDDATA)\n", hr, D3DXERR_INVALIDDATA);
    ok(!data, "Got %p, expected NULL\n", data);
    ok(!size, "Got %u, expected 0\n", size);

    hr = D3DXFindShaderComment(shader_invalid, MAKEFOURCC('C','T','A','B'), &data, &size);
    ok(hr == D3DXERR_INVALIDDATA, "Got result %x, expected %x (D3DXERR_INVALIDDATA)\n", hr, D3DXERR_INVALIDDATA);
    ok(!data, "Got %p, expected NULL\n", data);
    ok(!size, "Got %u, expected 0\n", size);

    hr = D3DXFindShaderComment(shader_empty, MAKEFOURCC('C','T','A','B'), &data, &size);
    ok(hr == S_FALSE, "Got result %x, expected %x (S_FALSE)\n", hr, S_FALSE);
    ok(!data, "Got %p, expected NULL\n", data);
    ok(!size, "Got %u, expected 0\n", size);

    hr = D3DXFindShaderComment(simple_fx, FCC_TEXT, &data, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(data == simple_fx + 3, "Got result %p, expected %p\n", data, simple_fx + 3);
    ok(size == 4, "Got result %u, expected 4\n", size);

    hr = D3DXFindShaderComment(simple_tx, FCC_TEXT, &data, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(data == simple_tx + 3, "Got result %p, expected %p\n", data, simple_tx + 3);
    ok(size == 4, "Got result %u, expected 4\n", size);

    hr = D3DXFindShaderComment(simple_7ffe, FCC_TEXT, &data, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(data == simple_7ffe + 3, "Got result %p, expected %p\n", data, simple_7ffe + 3);
    ok(size == 4, "Got result %u, expected 4\n", size);

    hr = D3DXFindShaderComment(simple_7fff, FCC_TEXT, &data, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(data == simple_7fff + 3, "Got result %p, expected %p\n", data, simple_7fff + 3);
    ok(size == 4, "Got result %u, expected 4\n", size);
}

static void test_get_shader_constant_table_ex(void)
{
    ID3DXConstantTable *constant_table;
    HRESULT hr;
    void *data;
    DWORD size;
    D3DXCONSTANTTABLE_DESC desc;

    constant_table = (ID3DXConstantTable *)0xdeadbeef;
    hr = D3DXGetShaderConstantTableEx(NULL, 0, &constant_table);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
    ok(constant_table == NULL, "D3DXGetShaderConstantTableEx() failed, got %p\n", constant_table);

    constant_table = (ID3DXConstantTable *)0xdeadbeef;
    hr = D3DXGetShaderConstantTableEx(shader_zero, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    ok(constant_table == NULL, "D3DXGetShaderConstantTableEx() failed, got %p\n", constant_table);

    constant_table = (ID3DXConstantTable *)0xdeadbeef;
    hr = D3DXGetShaderConstantTableEx(shader_invalid, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    ok(constant_table == NULL, "D3DXGetShaderConstantTableEx() failed, got %p\n", constant_table);

    constant_table = (ID3DXConstantTable *)0xdeadbeef;
    hr = D3DXGetShaderConstantTableEx(shader_empty, 0, &constant_table);
    ok(hr == D3DXERR_INVALIDDATA, "Got result %x, expected %x (D3DXERR_INVALIDDATA)\n", hr, D3DXERR_INVALIDDATA);
    ok(constant_table == NULL, "D3DXGetShaderConstantTableEx() failed, got %p\n", constant_table);

    /* No CTAB data */
    constant_table = (ID3DXConstantTable *)0xdeadbeef;
    hr = D3DXGetShaderConstantTableEx(simple_ps, 0, &constant_table);
    ok(hr == D3DXERR_INVALIDDATA, "Got result %x, expected %x (D3DXERR_INVALIDDATA)\n", hr, D3DXERR_INVALIDDATA);
    ok(constant_table == NULL, "D3DXGetShaderConstantTableEx() failed, got %p\n", constant_table);

    /* With invalid CTAB data */
    hr = D3DXGetShaderConstantTableEx(shader_with_invalid_ctab, 0, &constant_table);
    ok(hr == D3DXERR_INVALIDDATA || broken(hr == D3D_OK), /* winxp 64-bit, w2k3 64-bit */
       "Got result %x, expected %x (D3DXERR_INVALIDDATA)\n", hr, D3DXERR_INVALIDDATA);
    if (constant_table) ID3DXConstantTable_Release(constant_table);

    hr = D3DXGetShaderConstantTableEx(simple_fx, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK).\n", hr);
    ok(!constant_table, "D3DXGetShaderConstantTableEx() returned a non-NULL constant table.\n");

    hr = D3DXGetShaderConstantTableEx(simple_tx, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK).\n", hr);
    ok(!constant_table, "D3DXGetShaderConstantTableEx() returned a non-NULL constant table.\n");

    hr = D3DXGetShaderConstantTableEx(shader_with_ctab, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(constant_table != NULL, "D3DXGetShaderConstantTableEx() failed, got NULL\n");

    if (constant_table)
    {
        size = ID3DXConstantTable_GetBufferSize(constant_table);
        ok(size == 28, "Got result %x, expected 28\n", size);

        data = ID3DXConstantTable_GetBufferPointer(constant_table);
        ok(!memcmp(data, shader_with_ctab + 6, size), "Retrieved wrong CTAB data\n");

        hr = ID3DXConstantTable_GetDesc(constant_table, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXConstantTable_GetDesc(constant_table, &desc);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(desc.Creator == (const char *)data + 0x10, "Got result %p, expected %p\n",
                desc.Creator, (const char *)data + 0x10);
        ok(desc.Version == D3DVS_VERSION(3, 0), "Got result %x, expected %x\n", desc.Version, D3DVS_VERSION(3, 0));
        ok(desc.Constants == 0, "Got result %x, expected 0\n", desc.Constants);

        ID3DXConstantTable_Release(constant_table);
    }

    hr = D3DXGetShaderConstantTableEx(shader_with_ctab_constants, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(constant_table != NULL, "D3DXGetShaderConstantTableEx() failed, got NULL\n");

    if (constant_table)
    {
        D3DXHANDLE constant;
        D3DXCONSTANT_DESC constant_desc;
        D3DXCONSTANT_DESC constant_desc_save;
        UINT nb;

        /* Test GetDesc */
        hr = ID3DXConstantTable_GetDesc(constant_table, &desc);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(desc.Creator, "Wine project"), "Got result '%s', expected 'Wine project'\n", desc.Creator);
        ok(desc.Version == D3DVS_VERSION(3, 0), "Got result %x, expected %x\n", desc.Version, D3DVS_VERSION(3, 0));
        ok(desc.Constants == 3, "Got result %x, expected 3\n", desc.Constants);

        /* Test GetConstant */
        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 0);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(constant_desc.Name, "Constant1"), "Got result '%s', expected 'Constant1'\n",
            constant_desc.Name);
        ok(constant_desc.Class == D3DXPC_VECTOR, "Got result %x, expected %u (D3DXPC_VECTOR)\n",
            constant_desc.Class, D3DXPC_VECTOR);
        ok(constant_desc.Type == D3DXPT_FLOAT, "Got result %x, expected %u (D3DXPT_FLOAT)\n",
            constant_desc.Type, D3DXPT_FLOAT);
        ok(constant_desc.Rows == 1, "Got result %x, expected 1\n", constant_desc.Rows);
        ok(constant_desc.Columns == 4, "Got result %x, expected 4\n", constant_desc.Columns);

        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 1);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(constant_desc.Name, "Constant2"), "Got result '%s', expected 'Constant2'\n",
            constant_desc.Name);
        ok(constant_desc.Class == D3DXPC_MATRIX_COLUMNS, "Got result %x, expected %u (D3DXPC_MATRIX_COLUMNS)\n",
            constant_desc.Class, D3DXPC_MATRIX_COLUMNS);
        ok(constant_desc.Type == D3DXPT_FLOAT, "Got result %x, expected %u (D3DXPT_FLOAT)\n",
            constant_desc.Type, D3DXPT_FLOAT);
        ok(constant_desc.Rows == 4, "Got result %x, expected 1\n", constant_desc.Rows);
        ok(constant_desc.Columns == 4, "Got result %x, expected 4\n", constant_desc.Columns);

        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 2);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(constant_desc.Name, "Constant3"), "Got result '%s', expected 'Constant3'\n",
            constant_desc.Name);
        ok(constant_desc.Class == D3DXPC_MATRIX_COLUMNS, "Got result %x, expected %u (D3DXPC_MATRIX_COLUMNS)\n",
            constant_desc.Class, D3DXPC_MATRIX_COLUMNS);
        ok(constant_desc.Type == D3DXPT_FLOAT, "Got result %x, expected %u (D3DXPT_FLOAT)\n",
            constant_desc.Type, D3DXPT_FLOAT);
        ok(constant_desc.Rows == 4, "Got result %x, expected 1\n", constant_desc.Rows);
        ok(constant_desc.Columns == 4, "Got result %x, expected 4\n", constant_desc.Columns);
        constant_desc_save = constant_desc; /* For GetConstantDesc test */

        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 3);
        ok(constant == NULL, "Got result %p, expected NULL\n", constant);

        /* Test GetConstantByName */
        constant = ID3DXConstantTable_GetConstantByName(constant_table, NULL, "Constant unknown");
        ok(constant == NULL, "Got result %p, expected NULL\n", constant);
        constant = ID3DXConstantTable_GetConstantByName(constant_table, NULL, "Constant3");
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!memcmp(&constant_desc, &constant_desc_save, sizeof(D3DXCONSTANT_DESC)), "Got different constant data\n");

        /* Test GetConstantDesc */
        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 0);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, NULL, &constant_desc, &nb);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, NULL, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, NULL);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, "Constant unknown", &constant_desc, &nb);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, "Constant3", &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!memcmp(&constant_desc, &constant_desc_save, sizeof(D3DXCONSTANT_DESC)), "Got different constant data\n");

        ID3DXConstantTable_Release(constant_table);
    }

    hr = D3DXGetShaderConstantTableEx(fx_shader_with_ctab, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK).\n", hr);
    ok(!constant_table, "D3DXGetShaderConstantTableEx() returned a non-NULL constant table.\n");
}

static void test_constant_table(const char *test_name, const DWORD *ctable_fn,
        const D3DXCONSTANT_DESC *expecteds, UINT count)
{
    UINT i;
    ID3DXConstantTable *ctable;

    HRESULT res;

    /* Get the constant table from the shader itself */
    res = D3DXGetShaderConstantTable(ctable_fn, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed on %s: got %08x\n", test_name, res);

    for (i = 0; i < count; i++)
    {
        const D3DXCONSTANT_DESC *expected = &expecteds[i];
        D3DXHANDLE const_handle;
        D3DXCONSTANT_DESC actual;
        UINT pCount = 1;

        const_handle = ID3DXConstantTable_GetConstantByName(ctable, NULL, expected->Name);

        res = ID3DXConstantTable_GetConstantDesc(ctable, const_handle, &actual, &pCount);
        ok(SUCCEEDED(res), "%s in %s: ID3DXConstantTable_GetConstantDesc returned %08x\n", expected->Name,
                test_name, res);
        ok(pCount == 1, "%s in %s: Got more or less descriptions: %d\n", expected->Name, test_name, pCount);

        ok(strcmp(actual.Name, expected->Name) == 0,
           "%s in %s: Got different names: Got %s, expected %s\n", expected->Name,
           test_name, actual.Name, expected->Name);
        ok(actual.RegisterSet == expected->RegisterSet,
           "%s in %s: Got different register sets: Got %d, expected %d\n",
           expected->Name, test_name, actual.RegisterSet, expected->RegisterSet);
        ok(actual.RegisterIndex == expected->RegisterIndex,
           "%s in %s: Got different register indices: Got %d, expected %d\n",
           expected->Name, test_name, actual.RegisterIndex, expected->RegisterIndex);
        ok(actual.RegisterCount == expected->RegisterCount,
           "%s in %s: Got different register counts: Got %d, expected %d\n",
           expected->Name, test_name, actual.RegisterCount, expected->RegisterCount);
        ok(actual.Class == expected->Class,
           "%s in %s: Got different classes: Got %d, expected %d\n", expected->Name,
           test_name, actual.Class, expected->Class);
        ok(actual.Type == expected->Type,
           "%s in %s: Got different types: Got %d, expected %d\n", expected->Name,
           test_name, actual.Type, expected->Type);
        ok(actual.Rows == expected->Rows && actual.Columns == expected->Columns,
           "%s in %s: Got different dimensions: Got (%d, %d), expected (%d, %d)\n",
           expected->Name, test_name, actual.Rows, actual.Columns, expected->Rows,
           expected->Columns);
        ok(actual.Elements == expected->Elements,
           "%s in %s: Got different element count: Got %d, expected %d\n",
           expected->Name, test_name, actual.Elements, expected->Elements);
        ok(actual.StructMembers == expected->StructMembers,
           "%s in %s: Got different struct member count: Got %d, expected %d\n",
           expected->Name, test_name, actual.StructMembers, expected->StructMembers);
        ok(actual.Bytes == expected->Bytes,
           "%s in %s: Got different byte count: Got %d, expected %d\n",
           expected->Name, test_name, actual.Bytes, expected->Bytes);

        if (!expected->DefaultValue)
        {
            ok(actual.DefaultValue == NULL,
                "%s in %s: Got different default value: expected NULL\n",
                expected->Name, test_name);
        }
        else
        {
            ok(actual.DefaultValue != NULL,
                "%s in %s: Got different default value: expected non-NULL\n",
                expected->Name, test_name);
            ok(memcmp(actual.DefaultValue, expected->DefaultValue, expected->Bytes) == 0,
                "%s in %s: Got different default value\n", expected->Name, test_name);
        }
    }

    /* Finally, release the constant table */
    ID3DXConstantTable_Release(ctable);
}

static void test_constant_tables(void)
{
    test_constant_table("test_basic", ctab_basic, ctab_basic_expected,
            ARRAY_SIZE(ctab_basic_expected));
    test_constant_table("test_matrices", ctab_matrices, ctab_matrices_expected,
            ARRAY_SIZE(ctab_matrices_expected));
    test_constant_table("test_matrices2", ctab_matrices2, ctab_matrices2_expected,
            ARRAY_SIZE(ctab_matrices2_expected));
    test_constant_table("test_arrays", ctab_arrays, ctab_arrays_expected,
            ARRAY_SIZE(ctab_arrays_expected));
    test_constant_table("test_default_values", ctab_with_default_values, ctab_with_default_values_expected,
            ARRAY_SIZE(ctab_with_default_values_expected));
    test_constant_table("test_samplers", ctab_samplers, ctab_samplers_expected,
            ARRAY_SIZE(ctab_samplers_expected));
}

static void test_setting_basic_table(IDirect3DDevice9 *device)
{
    static const D3DXMATRIX mvp = {{{
        0.514f, 0.626f, 0.804f, 0.786f,
        0.238f, 0.956f, 0.374f, 0.483f,
        0.109f, 0.586f, 0.900f, 0.255f,
        0.898f, 0.411f, 0.932f, 0.275f}}};
    static const D3DXVECTOR4 f4 = {0.350f, 0.526f, 0.925f, 0.021f};
    static const float f = 0.12543f;
    static const int i = 321;
    static const D3DXMATRIX *matrix_pointer[] = {&mvp};

    ID3DXConstantTable *ctable;

    HRESULT res;
    float out[16];
    ULONG refcnt;

    /* Get the constant table from the shader itself */
    res = D3DXGetShaderConstantTable(ctab_basic, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got 0x%08x\n", res);

    /* Set constants */
    res = ID3DXConstantTable_SetMatrix(ctable, device, "mvp", &mvp);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable mvp: got 0x%08x\n", res);

    res = ID3DXConstantTable_SetInt(ctable, device, "i", i + 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetInt failed on variable i: got 0x%08x\n", res);

    /* Check that setting i again will overwrite the previous value */
    res = ID3DXConstantTable_SetInt(ctable, device, "i", i);
    ok(res == D3D_OK, "ID3DXConstantTable_SetInt failed on variable i: got 0x%08x\n", res);

    res = ID3DXConstantTable_SetFloat(ctable, device, "f", f);
    ok(res == D3D_OK, "ID3DXConstantTable_SetFloat failed on variable f: got 0x%08x\n", res);

    res = ID3DXConstantTable_SetVector(ctable, device, "f4", &f4);
    ok(res == D3D_OK, "ID3DXConstantTable_SetVector failed on variable f4: got 0x%08x\n", res);

    /* Get constants back and validate */
    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(out[0] == S(U(mvp))._11 && out[4] == S(U(mvp))._12 && out[8] == S(U(mvp))._13 && out[12] == S(U(mvp))._14,
            "The first row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[4], out[8], out[12], S(U(mvp))._11, S(U(mvp))._12, S(U(mvp))._13, S(U(mvp))._14);
    ok(out[1] == S(U(mvp))._21 && out[5] == S(U(mvp))._22 && out[9] == S(U(mvp))._23 && out[13] == S(U(mvp))._24,
            "The second row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[1], out[5], out[9], out[13], S(U(mvp))._21, S(U(mvp))._22, S(U(mvp))._23, S(U(mvp))._24);
    ok(out[2] == S(U(mvp))._31 && out[6] == S(U(mvp))._32 && out[10] == S(U(mvp))._33 && out[14] == S(U(mvp))._34,
            "The third row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[2], out[6], out[10], out[14], S(U(mvp))._31, S(U(mvp))._32, S(U(mvp))._33, S(U(mvp))._34);
    ok(out[3] == S(U(mvp))._41 && out[7] == S(U(mvp))._42 && out[11] == S(U(mvp))._43 && out[15] == S(U(mvp))._44,
            "The fourth row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[3], out[7], out[11], out[15], S(U(mvp))._41, S(U(mvp))._42, S(U(mvp))._43, S(U(mvp))._44);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 4, out, 1);
    ok(out[0] == (float)i && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable i was not set correctly, out={%f, %f, %f, %f}, should be {%d, 0.0, 0.0, 0.0}\n",
            out[0], out[1], out[2], out[3], i);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 6, out, 1);
    ok(out[0] == f && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable f was not set correctly, out={%f, %f, %f, %f}, should be {%f, 0.0, 0.0, 0.0}\n",
            out[0], out[1], out[2], out[3], f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(memcmp(out, &f4, sizeof(f4)) == 0,
            "The variable f4 was not set correctly, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], f4.x, f4.y, f4.z, f4.w);

    /* Finally test using a set* function for one type to set a variable of another type (should succeed) */
    res = ID3DXConstantTable_SetVector(ctable, device, "f", &f4);
    ok(res == D3D_OK, "ID3DXConstantTable_SetVector failed on variable f: 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 6, out, 1);
    ok(out[0] == f4.x && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable f was not set correctly by ID3DXConstantTable_SetVector, got %f, should be %f\n",
            out[0], f4.x);

    memset(out, 0, sizeof(out));
    IDirect3DDevice9_SetVertexShaderConstantF(device, 6, out, 1);
    res = ID3DXConstantTable_SetMatrix(ctable, device, "f", &mvp);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable f: 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 6, out, 1);
    ok(out[0] == S(U(mvp))._11 && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable f was not set correctly by ID3DXConstantTable_SetMatrix, got %f, should be %f\n",
            out[0], S(U(mvp))._11);

    /* Clear registers */
    memset(out, 0, sizeof(out));
    IDirect3DDevice9_SetVertexShaderConstantF(device, 0, out, 4);
    IDirect3DDevice9_SetVertexShaderConstantF(device, 6, out, 1);
    IDirect3DDevice9_SetVertexShaderConstantF(device, 7, out, 1);

    /* SetVector shouldn't change the value of a matrix constant */
    res = ID3DXConstantTable_SetVector(ctable, device, "mvp", &f4);
    ok(res == D3D_OK, "ID3DXConstantTable_SetVector failed on variable f: 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == 0.0f && out[5] == 0.0f && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == 0.0f && out[9] == 0.0f && out[10] == 0.0f && out[11] == 0.0f
            && out[12] == 0.0f && out[13] == 0.0f && out[14] == 0.0f && out[15] == 0.0f,
            "The variable mvp was not set correctly by ID3DXConstantTable_SetVector, "
            "got {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f %f; %f, %f, %f, %f}, "
            "should be all 0.0f\n",
            out[0], out[1], out[2], out[3],
            out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11],
            out[12], out[13], out[14], out[15]);

    res = ID3DXConstantTable_SetFloat(ctable, device, "mvp", f);
    ok(res == D3D_OK, "ID3DXConstantTable_SetFloat failed on variable mvp: 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == 0.0f && out[5] == 0.0f && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == 0.0f && out[9] == 0.0f && out[10] == 0.0f && out[11] == 0.0f
            && out[12] == 0.0f && out[13] == 0.0f && out[14] == 0.0f && out[15] == 0.0f,
            "The variable mvp was not set correctly by ID3DXConstantTable_SetFloat, "
            "got {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f %f; %f, %f, %f, %f}, "
            "should be all 0.0f\n",
            out[0], out[1], out[2], out[3],
            out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11],
            out[12], out[13], out[14], out[15]);

    res = ID3DXConstantTable_SetFloat(ctable, device, "f4", f);
    ok(res == D3D_OK, "ID3DXConstantTable_SetFloat failed on variable f4: 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable f4 was not set correctly by ID3DXConstantTable_SetFloat, "
            "got {%f, %f, %f, %f}, should be all 0.0f\n",
            out[0], out[1], out[2], out[3]);

    res = ID3DXConstantTable_SetMatrixTranspose(ctable, device, "f", &mvp);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTranspose failed on variable f: 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 6, out, 1);
    ok(out[0] == S(U(mvp))._11 && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable f was not set correctly by ID3DXConstantTable_SetMatrixTranspose, got %f, should be %f\n",
            out[0], S(U(mvp))._11);

    res = ID3DXConstantTable_SetMatrixTranspose(ctable, device, "f4", &mvp);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTranspose failed on variable f4: 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(out[0] == S(U(mvp))._11 && out[1] == S(U(mvp))._21 && out[2] == S(U(mvp))._31 && out[3] == S(U(mvp))._41,
            "The variable f4 was not set correctly by ID3DXConstantTable_SetMatrixTranspose, "
            "got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            S(U(mvp))._11, S(U(mvp))._21, S(U(mvp))._31, S(U(mvp))._41);

    memset(out, 0, sizeof(out));
    IDirect3DDevice9_SetVertexShaderConstantF(device, 6, out, 1);
    res = ID3DXConstantTable_SetMatrixPointerArray(ctable, device, "f", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixPointerArray failed on variable f: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 6, out, 1);
    ok(out[0] == S(U(mvp))._11 && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable f was not set correctly by ID3DXConstantTable_SetMatrixPointerArray, "
            "got %f, should be %f\n",
            out[0], S(U(mvp))._11);

    res = ID3DXConstantTable_SetMatrixPointerArray(ctable, device, "f4", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixPointerArray failed on variable f4: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(out[0] == S(U(mvp))._11 && out[1] == S(U(mvp))._12 && out[2] == S(U(mvp))._13 && out[3] == S(U(mvp))._14,
            "The variable f4 was not set correctly by ID3DXConstantTable_SetMatrixPointerArray, "
            "got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            S(U(mvp))._11, S(U(mvp))._12, S(U(mvp))._13, S(U(mvp))._14);

    memset(out, 0, sizeof(out));
    IDirect3DDevice9_SetVertexShaderConstantF(device, 6, out, 1);
    res = ID3DXConstantTable_SetMatrixTransposePointerArray(ctable, device, "f", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTransposePointerArray failed on variable f: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 6, out, 1);
    ok(out[0] == S(U(mvp))._11 && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
            "The variable f was not set correctly by ID3DXConstantTable_SetMatrixTransposePointerArray, "
            "got %f, should be %f\n",
            out[0], S(U(mvp))._11);

    res = ID3DXConstantTable_SetMatrixTransposePointerArray(ctable, device, "f4", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTransposePointerArray failed on variable f4: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(out[0] == S(U(mvp))._11 && out[1] == S(U(mvp))._21 && out[2] == S(U(mvp))._31 && out[3] == S(U(mvp))._41,
            "The variable f4 was not set correctly by ID3DXConstantTable_SetMatrixTransposePointerArray, "
            "got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            S(U(mvp))._11, S(U(mvp))._21, S(U(mvp))._31, S(U(mvp))._41);

    refcnt = ID3DXConstantTable_Release(ctable);
    ok(refcnt == 0, "The constant table reference count was %u, should be 0\n", refcnt);
}

static void test_setting_matrices_table(IDirect3DDevice9 *device)
{
    static const D3DXMATRIX fmatrix =
        {{{2.001f, 1.502f, 9.003f, 1.004f,
           5.005f, 3.006f, 3.007f, 6.008f,
           9.009f, 5.010f, 7.011f, 1.012f,
           5.013f, 5.014f, 5.015f, 9.016f}}};
    static const D3DXMATRIX *matrix_pointer[] = {&fmatrix};

    ID3DXConstantTable *ctable;

    HRESULT res;
    float out[32];

    res = D3DXGetShaderConstantTable(ctab_matrices, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "imatrix2x3", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable imatrix2x3: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "fmatrix3x1", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable fmatrix3x1: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 4, out, 2);
    ok(out[0] == (int)S(U(fmatrix))._11 && out[1] == (int)S(U(fmatrix))._12 && out[2] == (int)S(U(fmatrix))._13
            && out[3] == 0
            && out[4] == (int)S(U(fmatrix))._21 && out[5] == (int)S(U(fmatrix))._22 && out[6] == (int)S(U(fmatrix))._23
            && out[7] == 0,
            "The variable imatrix2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%d, %d, %d, %d; %d, %d, %d, %d}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            (int)S(U(fmatrix))._11, (int)S(U(fmatrix))._12, (int)S(U(fmatrix))._13, 0,
            (int)S(U(fmatrix))._21, (int)S(U(fmatrix))._22, (int)S(U(fmatrix))._23, 0);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._21 && out[2] == S(U(fmatrix))._31 && out[3] == 0.0f,
            "The variable fmatrix3x1 was not set correctly, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            S(U(fmatrix))._11, S(U(fmatrix))._21, S(U(fmatrix))._31, 0.0f);

    ID3DXConstantTable_Release(ctable);

    res = D3DXGetShaderConstantTable(ctab_matrices2, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %#x\n", res);

    /* SetMatrix */
    res = ID3DXConstantTable_SetMatrix(ctable, device, "c2x3", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable c2x3: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "r2x3", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable r2x3: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "c3x2", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable c3x2: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "r3x2", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable r3x2: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "c3x3", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable c3x3: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 3);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._21 && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._12 && out[5] == S(U(fmatrix))._22 && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == S(U(fmatrix))._13 && out[9] == S(U(fmatrix))._23 && out[10] == 0.0f && out[11] == 0.0f,
            "The variable c2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11],
            S(U(fmatrix))._11, S(U(fmatrix))._21, 0.0f, 0.0f,
            S(U(fmatrix))._12, S(U(fmatrix))._22, 0.0f, 0.0f,
            S(U(fmatrix))._13, S(U(fmatrix))._23, 0.0f, 0.0f);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "r4x4", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed on variable r4x4: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 15, out, 2);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._12 && out[2] == S(U(fmatrix))._13 && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._21 && out[5] == S(U(fmatrix))._22 && out[6] == S(U(fmatrix))._23 && out[7] == 0.0f,
            "The variable r2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            S(U(fmatrix))._11, S(U(fmatrix))._12, S(U(fmatrix))._13, 0.0f,
            S(U(fmatrix))._21, S(U(fmatrix))._22, S(U(fmatrix))._23, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 13, out, 2);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._21 && out[2] == S(U(fmatrix))._31 && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._12 && out[5] == S(U(fmatrix))._22 && out[6] == S(U(fmatrix))._32 && out[7] == 0.0f,
            "The variable c3x2 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            S(U(fmatrix))._11, S(U(fmatrix))._21, S(U(fmatrix))._31, 0.0f,
            S(U(fmatrix))._12, S(U(fmatrix))._22, S(U(fmatrix))._32, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 4, out, 3);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._12 && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._21 && out[5] == S(U(fmatrix))._22 && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == S(U(fmatrix))._31 && out[9] == S(U(fmatrix))._32 && out[10] == 0.0f && out[11] == 0.0f,
            "The variable r3x2 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9], out[10], out[11],
            S(U(fmatrix))._11, S(U(fmatrix))._12, 0.0f, 0.0f,
            S(U(fmatrix))._21, S(U(fmatrix))._22, 0.0f, 0.0f,
            S(U(fmatrix))._31, S(U(fmatrix))._32, 0.0f, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 10, out, 3);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._21 && out[2] == S(U(fmatrix))._31 && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._12 && out[5] == S(U(fmatrix))._22 && out[6] == S(U(fmatrix))._32 && out[7] == 0.0f
            && out[8] == S(U(fmatrix))._13 && out[9] == S(U(fmatrix))._23 && out[10] == S(U(fmatrix))._33 && out[11] == 0.0f,
            "The variable c3x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9], out[10], out[11],
            S(U(fmatrix))._11, S(U(fmatrix))._21, S(U(fmatrix))._31, 0.0f,
            S(U(fmatrix))._12, S(U(fmatrix))._22, S(U(fmatrix))._32, 0.0f,
            S(U(fmatrix))._13, S(U(fmatrix))._23, S(U(fmatrix))._33, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._12 && out[2] == S(U(fmatrix))._13 && out[3] == S(U(fmatrix))._14
            && out[4] == S(U(fmatrix))._21 && out[5] == S(U(fmatrix))._22 && out[6] == S(U(fmatrix))._23 && out[7] == S(U(fmatrix))._24
            && out[8] == S(U(fmatrix))._31 && out[9] == S(U(fmatrix))._32 && out[10] == S(U(fmatrix))._33 && out[11] == S(U(fmatrix))._34
            && out[12] == S(U(fmatrix))._41 && out[13] == S(U(fmatrix))._42 && out[14] == S(U(fmatrix))._43 && out[15] == S(U(fmatrix))._44,
            "The variable r4x4 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11], out[12], out[13], out[14], out[15],
            S(U(fmatrix))._11, S(U(fmatrix))._12, S(U(fmatrix))._13, S(U(fmatrix))._14,
            S(U(fmatrix))._21, S(U(fmatrix))._22, S(U(fmatrix))._23, S(U(fmatrix))._24,
            S(U(fmatrix))._31, S(U(fmatrix))._32, S(U(fmatrix))._33, S(U(fmatrix))._34,
            S(U(fmatrix))._41, S(U(fmatrix))._42, S(U(fmatrix))._43, S(U(fmatrix))._44);

    /* SetMatrixTranspose */
    res = ID3DXConstantTable_SetMatrixTranspose(ctable, device, "c2x3", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTranspose failed on variable c2x3: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrixTranspose(ctable, device, "r2x3", &fmatrix);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTranspose failed on variable r2x3: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 3);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._12 && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._21 && out[5] == S(U(fmatrix))._22 && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == S(U(fmatrix))._31 && out[9] == S(U(fmatrix))._32 && out[10] == 0.0f && out[11] == 0.0f,
            "The variable c2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11],
            S(U(fmatrix))._11, S(U(fmatrix))._12, 0.0f, 0.0f,
            S(U(fmatrix))._21, S(U(fmatrix))._22, 0.0f, 0.0f,
            S(U(fmatrix))._31, S(U(fmatrix))._32, 0.0f, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 15, out, 2);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._21 && out[2] == S(U(fmatrix))._31 && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._12 && out[5] == S(U(fmatrix))._22 && out[6] == S(U(fmatrix))._32 && out[7] == 0.0f,
            "The variable r2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            S(U(fmatrix))._11, S(U(fmatrix))._21, S(U(fmatrix))._31, 0.0f,
            S(U(fmatrix))._12, S(U(fmatrix))._22, S(U(fmatrix))._32, 0.0f);

    /* SetMatrixPointerArray */
    res = ID3DXConstantTable_SetMatrixPointerArray(ctable, device, "c2x3", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixPointerArray failed on variable c2x3: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrixPointerArray(ctable, device, "r2x3", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixPointerArray failed on variable r2x3: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 3);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._21 && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._12 && out[5] == S(U(fmatrix))._22 && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == S(U(fmatrix))._13 && out[9] == S(U(fmatrix))._23 && out[10] == 0.0f && out[11] == 0.0f,
            "The variable c2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11],
            S(U(fmatrix))._11, S(U(fmatrix))._21, 0.0f, 0.0f,
            S(U(fmatrix))._12, S(U(fmatrix))._22, 0.0f, 0.0f,
            S(U(fmatrix))._13, S(U(fmatrix))._23, 0.0f, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 15, out, 2);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._12 && out[2] == S(U(fmatrix))._13 && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._21 && out[5] == S(U(fmatrix))._22 && out[6] == S(U(fmatrix))._23 && out[7] == 0.0f,
            "The variable r2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            S(U(fmatrix))._11, S(U(fmatrix))._12, S(U(fmatrix))._13, 0.0f,
            S(U(fmatrix))._21, S(U(fmatrix))._22, S(U(fmatrix))._23, 0.0f);

    /* SetMatrixTransposePointerArray */
    res = ID3DXConstantTable_SetMatrixTransposePointerArray(ctable, device, "c2x3", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTransposePointerArray failed on variable c2x3: got %#x\n", res);

    res = ID3DXConstantTable_SetMatrixTransposePointerArray(ctable, device, "r2x3", matrix_pointer, 1);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixTransposePointerArray failed on variable r2x3: got %#x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 3);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._12 && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._21 && out[5] == S(U(fmatrix))._22 && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == S(U(fmatrix))._31 && out[9] == S(U(fmatrix))._32 && out[10] == 0.0f && out[11] == 0.0f,
            "The variable c2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3],
            out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11],
            S(U(fmatrix))._11, S(U(fmatrix))._12, 0.0f, 0.0f,
            S(U(fmatrix))._21, S(U(fmatrix))._22, 0.0f, 0.0f,
            S(U(fmatrix))._31, S(U(fmatrix))._32, 0.0f, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 15, out, 2);
    ok(out[0] == S(U(fmatrix))._11 && out[1] == S(U(fmatrix))._21 && out[2] == S(U(fmatrix))._31 && out[3] == 0.0f
            && out[4] == S(U(fmatrix))._12 && out[5] == S(U(fmatrix))._22 && out[6] == S(U(fmatrix))._32 && out[7] == 0.0f,
            "The variable r2x3 was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            S(U(fmatrix))._11, S(U(fmatrix))._21, S(U(fmatrix))._31, 0.0f,
            S(U(fmatrix))._12, S(U(fmatrix))._22, S(U(fmatrix))._32, 0.0f);

    ID3DXConstantTable_Release(ctable);
}

static void test_setting_arrays_table(IDirect3DDevice9 *device)
{
    static const float farray[8] = {
        0.005f, 0.745f, 0.973f, 0.264f,
        0.010f, 0.020f, 0.030f, 0.040f};
    static const D3DXMATRIX fmtxarray[2] = {
        {{{0.001f, 0.002f, 0.003f, 0.004f,
           0.005f, 0.006f, 0.007f, 0.008f,
           0.009f, 0.010f, 0.011f, 0.012f,
           0.013f, 0.014f, 0.015f, 0.016f}}},
        {{{0.010f, 0.020f, 0.030f, 0.040f,
           0.050f, 0.060f, 0.070f, 0.080f,
           0.090f, 0.100f, 0.110f, 0.120f,
           0.130f, 0.140f, 0.150f, 0.160f}}}};
    static const int iarray[4] = {1, 2, 3, 4};
    static const D3DXVECTOR4 fvecarray[2] = {
        {0.745f, 0.997f, 0.353f, 0.237f},
        {0.060f, 0.455f, 0.333f, 0.983f}};
    static BOOL barray[4] = {FALSE, 100, TRUE, TRUE};

    ID3DXConstantTable *ctable;

    HRESULT res;
    float out[32];
    ULONG refcnt;

    /* Clear registers */
    memset(out, 0, sizeof(out));
    IDirect3DDevice9_SetVertexShaderConstantF(device,  8, out, 4);
    IDirect3DDevice9_SetVertexShaderConstantF(device, 12, out, 4);

    /* Get the constant table from the shader */
    res = D3DXGetShaderConstantTable(ctab_arrays, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got 0x%08x\n", res);

    /* Set constants */

    /* Make sure that we cannot set registers that do not belong to this constant */
    res = ID3DXConstantTable_SetFloatArray(ctable, device, "farray", farray, 8);
    ok(res == D3D_OK, "ID3DXConstantTable_SetFloatArray failed: got 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 8, out, 8);
    ok(out[0] == farray[0] && out[4] == farray[1] && out[8] == farray[2] && out[12] == farray[3],
            "The in-bounds elements of the array were not set, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[4], out[8], out[12], farray[0], farray[1], farray[2], farray[3]);
    ok(out[16] == 0.0f && out[20] == 0.0f && out[24] == 0.0f && out[28] == 0.0f,
            "The excess elements of the array were set, out={%f, %f, %f, %f}, should be all 0.0f\n",
            out[16], out[20], out[24], out[28]);

    /* ivecarray takes up only 1 register, but a matrix takes up 4, so no elements should be set */
    res = ID3DXConstantTable_SetMatrix(ctable, device, "ivecarray", &fmtxarray[0]);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed: got 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 18, out, 4);
    ok(out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f,
       "The array was set, out={%f, %f, %f, %f}, should be all 0.0f\n", out[0], out[1], out[2], out[3]);

    /* Try setting an integer array to an array declared as a float array */
    res = ID3DXConstantTable_SetIntArray(ctable, device, "farray", iarray, 4);
    ok(res == D3D_OK, "ID3DXConstantTable_SetIntArray failed: got 0x%08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 8, out, 4);
    ok(out[0] == iarray[0] && out[4] == iarray[1] && out[8] == iarray[2] && out[12] == iarray[3],
           "SetIntArray did not properly set a float array: out={%f, %f, %f, %f}, should be {%d, %d, %d, %d}\n",
            out[0], out[4], out[8], out[12], iarray[0], iarray[1], iarray[2], iarray[3]);

    res = ID3DXConstantTable_SetFloatArray(ctable, device, "farray", farray, 4);
    ok(res == D3D_OK, "ID3DXConstantTable_SetFloatArray failed: got x0%08x\n", res);

    res = ID3DXConstantTable_SetVectorArray(ctable, device, "fvecarray", fvecarray, 2);
    ok(res == D3D_OK, "ID3DXConstantTable_SetVectorArray failed: got 0x%08x\n", res);

    res = ID3DXConstantTable_SetMatrixArray(ctable, device, "fmtxarray", fmtxarray, 2);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrixArray failed: got 0x%08x\n", res);

    res = ID3DXConstantTable_SetBoolArray(ctable, device, "barray", barray, 2);
    ok(res == D3D_OK, "ID3DXConstantTable_SetBoolArray failed: got 0x%08x\n", res);

    /* Read back constants */
    IDirect3DDevice9_GetVertexShaderConstantF(device, 8, out, 4);
    ok(out[0] == farray[0] && out[4] == farray[1] && out[8] == farray[2] && out[12] == farray[3],
            "The variable farray was not set correctly, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[4], out[8], out[12], farray[0], farray[1], farray[2], farray[3]);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 12, out, 2);
    ok(out[0] == fvecarray[0].x && out[1] == fvecarray[0].y && out[2] == fvecarray[0].z && out[3] == fvecarray[0].w &&
            out[4] == fvecarray[1].x && out[5] == fvecarray[1].y && out[6] == fvecarray[1].z && out[7] == fvecarray[1].w,
            "The variable fvecarray was not set correctly, out={{%f, %f, %f, %f}, {%f, %f, %f, %f}}, should be "
            "{{%f, %f, %f, %f}, {%f, %f, %f, %f}}\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            fvecarray[0].x, fvecarray[0].y, fvecarray[0].z, fvecarray[0].w, fvecarray[1].x, fvecarray[1].y,
            fvecarray[1].z, fvecarray[1].w);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 14, out, 2);
    ok(out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == 1.0f && out[5] == 0.0f && out[6] == 0.0f && out[7] == 0.0f,
            "The variable barray was not set correctly, out={%f, %f %f, %f; %f, %f, %f, %f}, should be {%f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 8);
    /* Just check a few elements in each matrix to make sure fmtxarray was set row-major */
    ok(out[0] == S(U(fmtxarray[0]))._11 && out[1] == S(U(fmtxarray[0]))._12 && out[2] == S(U(fmtxarray[0]))._13 && out[3] == S(U(fmtxarray[0]))._14,
           "The variable fmtxarray was not set row-major, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
           out[0], out[1], out[2], out[3], S(U(fmtxarray[0]))._11, S(U(fmtxarray[0]))._12, S(U(fmtxarray[0]))._13, S(U(fmtxarray[0]))._14);
    ok(out[16] == S(U(fmtxarray[1]))._11 && out[17] == S(U(fmtxarray[1]))._12 && out[18] == S(U(fmtxarray[1]))._13 && out[19] == S(U(fmtxarray[1]))._14,
           "The variable fmtxarray was not set row-major, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
           out[16], out[17], out[18], out[19], S(U(fmtxarray[1]))._11, S(U(fmtxarray[1]))._12, S(U(fmtxarray[1]))._13, S(U(fmtxarray[1]))._14);

    refcnt = ID3DXConstantTable_Release(ctable);
    ok(refcnt == 0, "The constant table reference count was %u, should be 0\n", refcnt);
}

static void test_SetDefaults(IDirect3DDevice9 *device)
{
    static const D3DXMATRIX mvp = {{{
        0.51f, 0.62f, 0.80f, 0.78f,
        0.23f, 0.95f, 0.37f, 0.48f,
        0.10f, 0.58f, 0.90f, 0.25f,
        0.89f, 0.41f, 0.93f, 0.27f}}};
    static const D3DXVECTOR4 f4 = {0.2f, 0.4f, 0.8f, 1.2f};

    float out[16];

    HRESULT res;
    ID3DXConstantTable *ctable;

    res = D3DXGetShaderConstantTable(ctab_basic, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %08x\n", res);

    res = ID3DXConstantTable_SetVector(ctable, device, "f4", &f4);
    ok(res == D3D_OK, "ID3DXConstantTable_SetVector failed: got %08x\n", res);

    res = ID3DXConstantTable_SetMatrix(ctable, device, "mvp", &mvp);
    ok(res == D3D_OK, "ID3DXConstantTable_SetMatrix failed: got %08x\n", res);

    res = ID3DXConstantTable_SetDefaults(ctable, device);
    ok(res == D3D_OK, "ID3dXConstantTable_SetDefaults failed: got %08x\n", res);

    /* SetDefaults doesn't change constants without default values */
    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(out[0] == S(U(mvp))._11 && out[4] == S(U(mvp))._12 && out[8] == S(U(mvp))._13 && out[12] == S(U(mvp))._14,
            "The first row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[4], out[8], out[12], S(U(mvp))._11, S(U(mvp))._12, S(U(mvp))._13, S(U(mvp))._14);
    ok(out[1] == S(U(mvp))._21 && out[5] == S(U(mvp))._22 && out[9] == S(U(mvp))._23 && out[13] == S(U(mvp))._24,
            "The second row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[1], out[5], out[9], out[13], S(U(mvp))._21, S(U(mvp))._22, S(U(mvp))._23, S(U(mvp))._24);
    ok(out[2] == S(U(mvp))._31 && out[6] == S(U(mvp))._32 && out[10] == S(U(mvp))._33 && out[14] == S(U(mvp))._34,
            "The third row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[2], out[6], out[10], out[14], S(U(mvp))._31, S(U(mvp))._32, S(U(mvp))._33, S(U(mvp))._34);
    ok(out[3] == S(U(mvp))._41 && out[7] == S(U(mvp))._42 && out[11] == S(U(mvp))._43 && out[15] == S(U(mvp))._44,
            "The fourth row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[3], out[7], out[11], out[15], S(U(mvp))._41, S(U(mvp))._42, S(U(mvp))._43, S(U(mvp))._44);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(memcmp(out, &f4, sizeof(f4)) == 0,
            "The variable f4 was not set correctly, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], f4.x, f4.y, f4.z, f4.w);

    ID3DXConstantTable_Release(ctable);

    res = D3DXGetShaderConstantTable(ctab_with_default_values, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %08x\n", res);

    res = ID3DXConstantTable_SetDefaults(ctable, device);
    ok(res == D3D_OK, "ID3DXConstantTable_SetDefaults failed: got %08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(memcmp(out, mat4_default_value, sizeof(mat4_default_value)) == 0,
            "The variable mat4 was not set correctly to default value\n");

    IDirect3DDevice9_GetVertexShaderConstantF(device, 4, out, 4);
    ok(memcmp(out, mat3_default_value, sizeof(mat3_default_value)) == 0,
            "The variable mat3 was not set correctly to default value\n");

    IDirect3DDevice9_GetVertexShaderConstantF(device, 8, out, 3);
    ok(memcmp(out, arr_default_value, sizeof(arr_default_value)) == 0,
        "The variable array was not set correctly to default value\n");

    IDirect3DDevice9_GetVertexShaderConstantF(device, 11, out, 1);
    ok(memcmp(out, vec4_default_value, sizeof(vec4_default_value)) == 0,
        "The variable vec4 was not set correctly to default value\n");

    IDirect3DDevice9_GetVertexShaderConstantF(device, 12, out, 1);
    ok(memcmp(out, flt_default_value, sizeof(flt_default_value)) == 0,
        "The variable flt was not set correctly to default value\n");

    ID3DXConstantTable_Release(ctable);
}

static void test_SetValue(IDirect3DDevice9 *device)
{
    static const D3DXMATRIX mvp = {{{
        0.51f, 0.62f, 0.80f, 0.78f,
        0.23f, 0.95f, 0.37f, 0.48f,
        0.10f, 0.58f, 0.90f, 0.25f,
        0.89f, 0.41f, 0.93f, 0.27f}}};
    static const D3DXVECTOR4 f4 = {0.2f, 0.4f, 0.8f, 1.2f};
    static const FLOAT arr[] = {0.33f, 0.55f, 0.96f, 1.00f,
                                1.00f, 1.00f, 1.00f, 1.00f,
                                1.00f, 1.00f, 1.00f, 1.00f};
    static int imatrix[] = {1, 2, 3, 4, 5, 6};
    static float fmatrix[] = {1.1f, 2.2f, 3.3f, 4.4f};
    static BOOL barray[] = {TRUE, FALSE};
    static float fvecarray[] = {9.1f, 9.2f, 9.3f, 9.4f, 9.5f, 9.6f, 9.7f, 9.8f};
    static float farray[] = {2.2f, 3.3f};

    static const float def[16] = {5.5f, 5.5f, 5.5f, 5.5f,
                                  5.5f, 5.5f, 5.5f, 5.5f,
                                  5.5f, 5.5f, 5.5f, 5.5f,
                                  5.5f, 5.5f, 5.5f, 5.5f};
    float out[16];

    HRESULT res;
    ID3DXConstantTable *ctable;

    res = D3DXGetShaderConstantTable(ctab_basic, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %08x\n", res);

    IDirect3DDevice9_SetVertexShaderConstantF(device, 7, def, 1);

    /* SetValue called with 0 bytes size doesn't change value */
    res = ID3DXConstantTable_SetValue(ctable, device, "f4", &f4, 0);
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(memcmp(out, def, sizeof(f4)) == 0,
            "The variable f4 was not set correctly, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], def[0], def[1], def[2], def[3]);

    res = ID3DXConstantTable_SetValue(ctable, device, "f4", &f4, sizeof(f4));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 1);
    ok(memcmp(out, &f4, sizeof(f4)) == 0,
            "The variable f4 was not set correctly, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], f4.x, f4.y, f4.z, f4.w);

    IDirect3DDevice9_SetVertexShaderConstantF(device, 0, def, 4);

    /* SetValue called with size smaller than constant size doesn't change value */
    res = ID3DXConstantTable_SetValue(ctable, device, "mvp", &mvp, sizeof(mvp) / 2);
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue returned %08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(memcmp(out, def, sizeof(def)) == 0,
            "The variable mvp was not set correctly, out={%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[4], out[ 8], out[12],
            out[1], out[5], out[ 9], out[13],
            out[2], out[6], out[10], out[14],
            out[3], out[7], out[11], out[15],
            def[0], def[4], def[ 8], def[12],
            def[1], def[5], def[ 9], def[13],
            def[2], def[6], def[10], def[14],
            def[3], def[7], def[11], def[15]);

    res = ID3DXConstantTable_SetValue(ctable, device, "mvp", &mvp, sizeof(mvp));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 0, out, 4);
    ok(out[0] == S(U(mvp))._11 && out[4] == S(U(mvp))._12 && out[8] == S(U(mvp))._13 && out[12] == S(U(mvp))._14,
            "The first row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[4], out[8], out[12], S(U(mvp))._11, S(U(mvp))._12, S(U(mvp))._13, S(U(mvp))._14);
    ok(out[1] == S(U(mvp))._21 && out[5] == S(U(mvp))._22 && out[9] == S(U(mvp))._23 && out[13] == S(U(mvp))._24,
            "The second row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[1], out[5], out[9], out[13], S(U(mvp))._21, S(U(mvp))._22, S(U(mvp))._23, S(U(mvp))._24);
    ok(out[2] == S(U(mvp))._31 && out[6] == S(U(mvp))._32 && out[10] == S(U(mvp))._33 && out[14] == S(U(mvp))._34,
            "The third row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[2], out[6], out[10], out[14], S(U(mvp))._31, S(U(mvp))._32, S(U(mvp))._33, S(U(mvp))._34);
    ok(out[3] == S(U(mvp))._41 && out[7] == S(U(mvp))._42 && out[11] == S(U(mvp))._43 && out[15] == S(U(mvp))._44,
            "The fourth row of mvp was not set correctly, got {%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[3], out[7], out[11], out[15], S(U(mvp))._41, S(U(mvp))._42, S(U(mvp))._43, S(U(mvp))._44);

    ID3DXConstantTable_Release(ctable);

    res = D3DXGetShaderConstantTable(ctab_with_default_values, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %08x\n", res);

    res = ID3DXConstantTable_SetValue(ctable, device, "arr", arr, sizeof(arr));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 8, out, 3);
    ok(out[0] == arr[0] && out[4] == arr[1] && out[8] == arr[2]
            && out[1] == 0 &&  out[2] == 0 && out[3] == 0 && out[5] == 0 && out[6] == 0 && out[7] == 0
            && out[9] == 0 && out[10] == 0 && out[11] == 0,
            "The variable arr was not set correctly, out={%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f}, "
            "should be {0.33, 0, 0, 0, 0.55, 0, 0, 0, 0.96, 0, 0, 0}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9], out[10], out[11]);

    ID3DXConstantTable_Release(ctable);

    res = D3DXGetShaderConstantTable(ctab_matrices, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %08x\n", res);

    res = ID3DXConstantTable_SetValue(ctable, device, "fmatrix3x1", fmatrix, sizeof(fmatrix));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    res = ID3DXConstantTable_SetValue(ctable, device, "imatrix2x3", imatrix, sizeof(imatrix));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 4, out, 2);
    ok(out[0] == imatrix[0] && out[1] == imatrix[1] && out[2] == imatrix[2] && out[3] == 0.0f
            && out[4] == imatrix[3] && out[5] == imatrix[4] && out[6] == imatrix[5] && out[7] == 0.0f,
            "The variable imatrix2x3 was not set correctly, out={%f, %f, %f, %f, %f, %f, %f, %f}, "
            "should be {%d, %d, %d, 0, %d, %d, %d, 0}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            imatrix[0], imatrix[1], imatrix[2], imatrix[3], imatrix[4], imatrix[5]);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 7, out, 2);
    ok(out[0] == fmatrix[0] && out[1] == fmatrix[1] && out[2] == fmatrix[2] && out[3] == 0.0f,
            "The variable fmatrix3x1 was not set correctly, out={%f, %f, %f, %f}, should be {%f, %f, %f, %f}\n",
            out[0], out[1] ,out[2], out[4],
            fmatrix[0], fmatrix[1], fmatrix[2], 0.0f);

    ID3DXConstantTable_Release(ctable);

    res = D3DXGetShaderConstantTable(ctab_arrays, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed: got %08x\n", res);

    res = ID3DXConstantTable_SetValue(ctable, device, "barray", barray, sizeof(barray));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    res = ID3DXConstantTable_SetValue(ctable, device, "fvecarray", fvecarray, sizeof(fvecarray));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    IDirect3DDevice9_SetVertexShaderConstantF(device, 8, def, 4);
    res = ID3DXConstantTable_SetValue(ctable, device, "farray", farray, sizeof(farray));
    ok(res == D3D_OK, "ID3DXConstantTable_SetValue failed: got %08x\n", res);

    /* 2 elements of farray were set */
    IDirect3DDevice9_GetVertexShaderConstantF(device, 8, out, 4);
    ok(out[0] == farray[0] && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == farray[1] && out[5] == 0.0f && out[6] == 0.0f && out[7] == 0.0f
            && out[8] == def[8] && out[9] == def[9] && out[10] == def[10] && out[11] == def[11]
            && out[12] == def[12] && out[13] == def[13] && out[14] == def[14] && out[15] == def[15],
            "The variable farray was not set correctly, should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f; %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            out[8], out[9], out[10], out[11], out[12], out[13], out[14], out[15],
            farray[0], 0.0f, 0.0f, 0.0f,
            farray[1], 0.0f, 0.0f, 0.0f,
            def[8], def[9], def[10], def[11],
            def[12], def[13], def[14], def[15]);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 12, out, 2);
    ok(out[0] == fvecarray[0] && out[1] == fvecarray[1] && out[2] == fvecarray[2] && out[3] == fvecarray[3]
            && out[4] == fvecarray[4] && out[5] == fvecarray[5] && out[6] == fvecarray[6] && out[7] == fvecarray[7],
            "The variable fvecarray was not set correctly, out ={%f, %f, %f, %f, %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f, %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            fvecarray[0], fvecarray[1], fvecarray[2], fvecarray[3], fvecarray[4], fvecarray[5], fvecarray[6], fvecarray[7]);

    IDirect3DDevice9_GetVertexShaderConstantF(device, 14, out, 2);
    ok(out[0] == 1.0f && out[1] == 0.0f && out[2] == 0.0f && out[3] == 0.0f
            && out[4] == 0.0f && out[5] == 0.0f && out[6] == 0.0f && out[7] == 0.0f,
            "The variable barray was not set correctly, out={%f, %f, %f, %f, %f, %f, %f, %f}, "
            "should be {%f, %f, %f, %f, %f, %f, %f, %f}\n",
            out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    ID3DXConstantTable_Release(ctable);
}

static void test_setting_constants(void)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;
    ULONG refcnt;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    if (!(d3d = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_setting_basic_table(device);
    test_setting_matrices_table(device);
    test_setting_arrays_table(device);
    test_SetDefaults(device);
    test_SetValue(device);

    /* Release resources */
    refcnt = IDirect3DDevice9_Release(device);
    ok(refcnt == 0, "The Direct3D device reference count was %u, should be 0\n", refcnt);

    refcnt = IDirect3D9_Release(d3d);
    ok(refcnt == 0, "The Direct3D object reference count was %u, should be 0\n", refcnt);

    if (wnd) DestroyWindow(wnd);
}

static void test_get_sampler_index(void)
{
    ID3DXConstantTable *ctable;

    HRESULT res;
    UINT index;

    ULONG refcnt;

    res = D3DXGetShaderConstantTable(ctab_samplers, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed on ctab_samplers: got %08x\n", res);

    index = ID3DXConstantTable_GetSamplerIndex(ctable, "sampler1");
    ok(index == 0, "ID3DXConstantTable_GetSamplerIndex returned wrong index: Got %d, expected 0\n", index);

    index = ID3DXConstantTable_GetSamplerIndex(ctable, "sampler2");
    ok(index == 3, "ID3DXConstantTable_GetSamplerIndex returned wrong index: Got %d, expected 3\n", index);

    index = ID3DXConstantTable_GetSamplerIndex(ctable, "nonexistent");
    ok(index == -1, "ID3DXConstantTable_GetSamplerIndex found nonexistent sampler: Got %d\n",
            index);

    index = ID3DXConstantTable_GetSamplerIndex(ctable, "notsampler");
    ok(index == -1, "ID3DXConstantTable_GetSamplerIndex succeeded on non-sampler constant: Got %d\n",
            index);

    refcnt = ID3DXConstantTable_Release(ctable);
    ok(refcnt == 0, "The ID3DXConstantTable reference count was %u, should be 0\n", refcnt);
}

/*
 * fxc.exe /Tps_3_0
 */
#if 0
sampler s;
sampler1D s1D;
sampler2D s2D;
sampler3D s3D;
samplerCUBE scube;
float4 init;
float4 main(float3 tex : TEXCOORD0) : COLOR
{
    float4 tmp = init;
    tmp = tmp + tex1D(s1D, tex.x);
    tmp = tmp + tex1D(s1D, tex.y);
    tmp = tmp + tex3D(s3D, tex.xyz);
    tmp = tmp + tex1D(s, tex.x);
    tmp = tmp + tex2D(s2D, tex.xy);
    tmp = tmp + texCUBE(scube, tex.xyz);
    return tmp;
}
#endif
static const DWORD get_shader_samplers_blob[] =
{
    0xffff0300,                                                             /* ps_3_0                        */
    0x0054fffe, FCC_CTAB,                                                   /* CTAB comment                  */
    0x0000001c, 0x0000011b, 0xffff0300, 0x00000006, 0x0000001c, 0x00000100, /* Header                        */
    0x00000114,
    0x00000094, 0x00000002, 0x00000001, 0x0000009c, 0x00000000,             /* Constant 1 desc (init)        */
    0x000000ac, 0x00040003, 0x00000001, 0x000000b0, 0x00000000,             /* Constant 2 desc (s)           */
    0x000000c0, 0x00000003, 0x00000001, 0x000000c4, 0x00000000,             /* Constant 3 desc (s1D)         */
    0x000000d4, 0x00010003, 0x00000001, 0x000000d8, 0x00000000,             /* Constant 4 desc (s2D)         */
    0x000000e8, 0x00030003, 0x00000001, 0x000000ec, 0x00000000,             /* Constant 5 desc (s3D)         */
    0x000000fc, 0x00020003, 0x00000001, 0x00000104, 0x00000000,             /* Constant 6 desc (scube)       */
    0x74696e69, 0xababab00,                                                 /* Constant 1 name               */
    0x00030001, 0x00040001, 0x00000001, 0x00000000,                         /* Constant 1 type desc          */
    0xabab0073,                                                             /* Constant 2 name               */
    0x000c0004, 0x00010001, 0x00000001, 0x00000000,                         /* Constant 2 type desc          */
    0x00443173,                                                             /* Constant 3 name               */
    0x000b0004, 0x00010001, 0x00000001, 0x00000000,                         /* Constant 3 type desc          */
    0x00443273,                                                             /* Constant 4 name               */
    0x000c0004, 0x00010001, 0x00000001, 0x00000000,                         /* Constant 4 type desc          */
    0x00443373,                                                             /* Constant 5 name               */
    0x000d0004, 0x00010001, 0x00000001, 0x00000000,                         /* Constant 5 type desc          */
    0x62756373, 0xabab0065,                                                 /* Constant 6 name               */
    0x000e0004, 0x00010001, 0x00000001, 0x00000000,                         /* Constant 6 type desc          */
    0x335f7370, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, /* Target/Creator name string    */
    0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932,
    0x332e3235, 0x00313131,
    0x0200001f, 0x80000005, 0x90070000, 0x0200001f, 0x90000000, 0xa00f0800, /* shader                        */
    0x0200001f, 0x90000000, 0xa00f0801, 0x0200001f, 0x98000000, 0xa00f0802,
    0x0200001f, 0xa0000000, 0xa00f0803, 0x0200001f, 0x90000000, 0xa00f0804,
    0x03000042, 0x800f0000, 0x90e40000, 0xa0e40800, 0x03000002, 0x800f0000,
    0x80e40000, 0xa0e40000, 0x03000042, 0x800f0001, 0x90550000, 0xa0e40800,
    0x03000002, 0x800f0000, 0x80e40000, 0x80e40001, 0x03000042, 0x800f0001,
    0x90e40000, 0xa0e40803, 0x03000002, 0x800f0000, 0x80e40000, 0x80e40001,
    0x03000042, 0x800f0001, 0x90e40000, 0xa0e40804, 0x03000002, 0x800f0000,
    0x80e40000, 0x80e40001, 0x03000042, 0x800f0001, 0x90e40000, 0xa0e40801,
    0x03000002, 0x800f0000, 0x80e40000, 0x80e40001, 0x03000042, 0x800f0001,
    0x90e40000, 0xa0e40802, 0x03000002, 0x800f0800, 0x80e40000, 0x80e40001,
    0x0000ffff,                                                             /* END                           */
};

static void test_get_shader_samplers(void)
{
    const char *samplers[16] = {NULL}; /* maximum number of sampler registers v/ps 3.0 = 16 */
    const char *sampler_orig;
    UINT count = 2;
    HRESULT hr;

if (0)
{
    /* crashes if bytecode is NULL */
    hr = D3DXGetShaderSamplers(NULL, NULL, &count);
    ok(hr == D3D_OK, "D3DXGetShaderSamplers failed, got %x, expected %x\n", hr, D3D_OK);
}

    hr = D3DXGetShaderSamplers(get_shader_samplers_blob, NULL, NULL);
    ok(hr == D3D_OK, "D3DXGetShaderSamplers failed, got %x, expected %x\n", hr, D3D_OK);

    samplers[5] = "dummy";

    hr = D3DXGetShaderSamplers(get_shader_samplers_blob, samplers, NULL);
    ok(hr == D3D_OK, "D3DXGetShaderSamplers failed, got %x, expected %x\n", hr, D3D_OK);

    /* check that sampler points to shader blob */
    sampler_orig = (const char *)&get_shader_samplers_blob[0x2e];
    ok(sampler_orig == samplers[0], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[0], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x33];
    ok(sampler_orig == samplers[1], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[1], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x38];
    ok(sampler_orig == samplers[2], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[2], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x3d];
    ok(sampler_orig == samplers[3], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[3], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x42];
    ok(sampler_orig == samplers[4], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[4], sampler_orig);

    ok(!strcmp(samplers[5], "dummy"), "D3DXGetShaderSamplers failed, got \"%s\", expected \"%s\"\n", samplers[5], "dummy");

    /* reset samplers */
    memset(samplers, 0, sizeof(samplers));
    samplers[5] = "dummy";

    hr = D3DXGetShaderSamplers(get_shader_samplers_blob, NULL, &count);
    ok(hr == D3D_OK, "D3DXGetShaderSamplers failed, got %x, expected %x\n", hr, D3D_OK);
    ok(count == 5, "D3DXGetShaderSamplers failed, got %u, expected %u\n", count, 5);

    hr = D3DXGetShaderSamplers(get_shader_samplers_blob, samplers, &count);
    ok(hr == D3D_OK, "D3DXGetShaderSamplers failed, got %x, expected %x\n", hr, D3D_OK);
    ok(count == 5, "D3DXGetShaderSamplers failed, got %u, expected %u\n", count, 5);

    /* check that sampler points to shader blob */
    sampler_orig = (const char *)&get_shader_samplers_blob[0x2e];
    ok(sampler_orig == samplers[0], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[0], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x33];
    ok(sampler_orig == samplers[1], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[1], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x38];
    ok(sampler_orig == samplers[2], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[2], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x3d];
    ok(sampler_orig == samplers[3], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[3], sampler_orig);

    sampler_orig = (const char *)&get_shader_samplers_blob[0x42];
    ok(sampler_orig == samplers[4], "D3DXGetShaderSamplers failed, got %p, expected %p\n", samplers[4], sampler_orig);

    ok(!strcmp(samplers[5], "dummy"), "D3DXGetShaderSamplers failed, got \"%s\", expected \"%s\"\n", samplers[5], "dummy");

    /* check without ctab */
    hr = D3DXGetShaderSamplers(simple_vs, samplers, &count);
    ok(hr == D3D_OK, "D3DXGetShaderSamplers failed, got %x, expected %x\n", hr, D3D_OK);
    ok(count == 0, "D3DXGetShaderSamplers failed, got %u, expected %u\n", count, 0);

    /* check invalid ctab */
    hr = D3DXGetShaderSamplers(shader_with_invalid_ctab, samplers, &count);
    ok(hr == D3D_OK, "D3DXGetShaderSamplers failed, got %x, expected %x\n", hr, D3D_OK);
    ok(count == 0, "D3DXGetShaderSamplers failed, got %u, expected %u\n", count, 0);
}

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
float f = {1.1f}, f_2[2] = {2.1f, 2.2f};
struct {float f; int i;} s = {3.1f, 31},
s_2[2] = {{4.1f, 41}, {4.2f, 42}},
s_3[3] = {{5.1f, 51}, {5.2f, 52}, {5.3f, 53}};
struct {int i1; int i2; float2 f_2; row_major float3x1 r[2];}
p[2] = {{11, 12, {13.1, 14.1}, {{3.11, 3.21, 3.31}, {3.41, 3.51, 3.61}}},
        {15, 16, {17.1, 18.1}, {{4.11, 4.21, 4.31}, {4.41, 4.51, 4.61}}}};
int i[1] = {6};
float2x3 f23[2] = {{0.11, 0.21, 0.31, 0.41, 0.51, 0.61}, {0.12, 0.22, 0.32, 0.42, 0.52, 0.62}};
float3x2 f32[2] = {{1.11, 1.21, 1.31, 1.41, 1.51, 1.61}, {1.12, 1.22, 1.32, 1.42, 1.52, 1.62}};
float3 v[2] = {{2.11, 2.21, 2.31}, {2.41, 2.51, 2.61}};
row_major float3x1 r31[2] = {{3.11, 3.21, 3.31}, {3.41, 3.51, 3.61}};
row_major float1x3 r13[2] = {{4.11, 4.21, 4.31}, {4.41, 4.51, 4.61}};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0.0f;
    tmp.zyw = v[1] + r13[1] + r31[1] + p[1].r[1];
    tmp.x += f * f_2[1] * pos.x * p[1].f_2.y;
    tmp.y += s.f * pos.y * s_2[0].i;
    tmp.z += s_3[0].f * pos.z * s_3[2].f * i[0] * f23[1]._11 * f32[1]._32;
    return tmp;
}
#endif
static const DWORD test_get_shader_constant_variables_blob[] =
{
0xfffe0300, 0x0185fffe, 0x42415443, 0x0000001c, 0x000005df, 0xfffe0300, 0x0000000c, 0x0000001c,
0x00000100, 0x000005d8, 0x0000010c, 0x002d0002, 0x00000001, 0x00000110, 0x00000120, 0x00000130,
0x001d0002, 0x00000004, 0x00000134, 0x00000144, 0x000001a4, 0x00210002, 0x00000004, 0x000001a8,
0x000001b8, 0x000001f8, 0x00250002, 0x00000002, 0x000001fc, 0x0000020c, 0x0000022c, 0x002f0002,
0x00000001, 0x00000230, 0x00000240, 0x00000250, 0x00000002, 0x00000012, 0x000002b0, 0x000002c0,
0x000003e0, 0x002b0002, 0x00000002, 0x000003e4, 0x000003f4, 0x00000414, 0x00120002, 0x00000006,
0x00000418, 0x00000428, 0x00000488, 0x002e0002, 0x00000001, 0x000004ac, 0x000004bc, 0x000004dc,
0x00270002, 0x00000002, 0x000004e0, 0x000004f0, 0x00000530, 0x00180002, 0x00000005, 0x00000534,
0x00000544, 0x000005a4, 0x00290002, 0x00000002, 0x000005a8, 0x000005b8, 0xabab0066, 0x00030000,
0x00010001, 0x00000001, 0x00000000, 0x3f8ccccd, 0x00000000, 0x00000000, 0x00000000, 0x00333266,
0x00030003, 0x00030002, 0x00000002, 0x00000000, 0x3de147ae, 0x3ed1eb85, 0x00000000, 0x00000000,
0x3e570a3d, 0x3f028f5c, 0x00000000, 0x00000000, 0x3e9eb852, 0x3f1c28f6, 0x00000000, 0x00000000,
0x3df5c28f, 0x3ed70a3d, 0x00000000, 0x00000000, 0x3e6147ae, 0x3f051eb8, 0x00000000, 0x00000000,
0x3ea3d70a, 0x3f1eb852, 0x00000000, 0x00000000, 0x00323366, 0x00030003, 0x00020003, 0x00000002,
0x00000000, 0x3f8e147b, 0x3fa7ae14, 0x3fc147ae, 0x00000000, 0x3f9ae148, 0x3fb47ae1, 0x3fce147b,
0x00000000, 0x3f8f5c29, 0x3fa8f5c3, 0x3fc28f5c, 0x00000000, 0x3f9c28f6, 0x3fb5c28f, 0x3fcf5c29,
0x00000000, 0x00325f66, 0x00030000, 0x00010001, 0x00000002, 0x00000000, 0x40066666, 0x00000000,
0x00000000, 0x00000000, 0x400ccccd, 0x00000000, 0x00000000, 0x00000000, 0xabab0069, 0x00020000,
0x00010001, 0x00000001, 0x00000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x31690070,
0xababab00, 0x00020000, 0x00010001, 0x00000001, 0x00000000, 0xab003269, 0x00030001, 0x00020001,
0x00000001, 0x00000000, 0xabab0072, 0x00030002, 0x00010003, 0x00000002, 0x00000000, 0x00000252,
0x00000258, 0x00000268, 0x00000258, 0x000001f8, 0x0000026c, 0x0000027c, 0x00000280, 0x00000005,
0x000a0001, 0x00040002, 0x00000290, 0x41300000, 0x00000000, 0x00000000, 0x00000000, 0x41400000,
0x00000000, 0x00000000, 0x00000000, 0x4151999a, 0x4161999a, 0x00000000, 0x00000000, 0x40470a3d,
0x00000000, 0x00000000, 0x00000000, 0x404d70a4, 0x00000000, 0x00000000, 0x00000000, 0x4053d70a,
0x00000000, 0x00000000, 0x00000000, 0x405a3d71, 0x00000000, 0x00000000, 0x00000000, 0x4060a3d7,
0x00000000, 0x00000000, 0x00000000, 0x40670a3d, 0x00000000, 0x00000000, 0x00000000, 0x41700000,
0x00000000, 0x00000000, 0x00000000, 0x41800000, 0x00000000, 0x00000000, 0x00000000, 0x4188cccd,
0x4190cccd, 0x00000000, 0x00000000, 0x4083851f, 0x00000000, 0x00000000, 0x00000000, 0x4086b852,
0x00000000, 0x00000000, 0x00000000, 0x4089eb85, 0x00000000, 0x00000000, 0x00000000, 0x408d1eb8,
0x00000000, 0x00000000, 0x00000000, 0x409051ec, 0x00000000, 0x00000000, 0x00000000, 0x4093851f,
0x00000000, 0x00000000, 0x00000000, 0x00333172, 0x00030002, 0x00030001, 0x00000002, 0x00000000,
0x4083851f, 0x4086b852, 0x4089eb85, 0x00000000, 0x408d1eb8, 0x409051ec, 0x4093851f, 0x00000000,
0x00313372, 0x00030002, 0x00010003, 0x00000002, 0x00000000, 0x40470a3d, 0x00000000, 0x00000000,
0x00000000, 0x404d70a4, 0x00000000, 0x00000000, 0x00000000, 0x4053d70a, 0x00000000, 0x00000000,
0x00000000, 0x405a3d71, 0x00000000, 0x00000000, 0x00000000, 0x4060a3d7, 0x00000000, 0x00000000,
0x00000000, 0x40670a3d, 0x00000000, 0x00000000, 0x00000000, 0xabab0073, 0x00030000, 0x00010001,
0x00000001, 0x00000000, 0x0000010c, 0x0000048c, 0x0000022c, 0x00000258, 0x00000005, 0x00020001,
0x00020001, 0x0000049c, 0x40466666, 0x00000000, 0x00000000, 0x00000000, 0x41f80000, 0x00000000,
0x00000000, 0x00000000, 0x00325f73, 0x00000005, 0x00020001, 0x00020002, 0x0000049c, 0x40833333,
0x00000000, 0x00000000, 0x00000000, 0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x40866666,
0x00000000, 0x00000000, 0x00000000, 0x42280000, 0x00000000, 0x00000000, 0x00000000, 0x00335f73,
0x00000005, 0x00020001, 0x00020003, 0x0000049c, 0x40a33333, 0x00000000, 0x00000000, 0x00000000,
0x424c0000, 0x00000000, 0x00000000, 0x00000000, 0x40a66666, 0x00000000, 0x00000000, 0x00000000,
0x42500000, 0x00000000, 0x00000000, 0x00000000, 0x40a9999a, 0x00000000, 0x00000000, 0x00000000,
0x42540000, 0x00000000, 0x00000000, 0x00000000, 0xabab0076, 0x00030001, 0x00030001, 0x00000002,
0x00000000, 0x40070a3d, 0x400d70a4, 0x4013d70a, 0x00000000, 0x401a3d71, 0x4020a3d7, 0x40270a3d,
0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c,
0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131, 0x0200001f,
0x80000000, 0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x02000001, 0x80070000, 0xa0e4002a,
0x03000002, 0x80070000, 0x80e40000, 0xa0e4002c, 0x03000002, 0x80040000, 0x80aa0000, 0xa0000017,
0x03000002, 0xe0080000, 0x80aa0000, 0xa0000011, 0x02000001, 0x80010001, 0xa000002d, 0x03000005,
0x80040000, 0x80000001, 0xa0000026, 0x03000005, 0x80040000, 0x80aa0000, 0x90000000, 0x03000005,
0xe0010000, 0x80aa0000, 0xa055000b, 0x03000002, 0x80020000, 0x80550000, 0xa0000016, 0x03000002,
0x80010000, 0x80000000, 0xa0000015, 0x03000002, 0x80010000, 0x80000000, 0xa000000f, 0x03000002,
0x80020000, 0x80550000, 0xa0000010, 0x03000005, 0x80040000, 0xa000002e, 0x90550000, 0x04000004,
0xe0020000, 0x80aa0000, 0xa0000028, 0x80550000, 0x03000005, 0x80020000, 0xa0000018, 0x90aa0000,
0x03000005, 0x80020000, 0x80550000, 0xa000001c, 0x03000005, 0x80020000, 0x80550000, 0xa000002f,
0x03000005, 0x80020000, 0x80550000, 0xa0000020, 0x04000004, 0xe0040000, 0x80550000, 0xa0aa0024,
0x80000000, 0x0000ffff,
};

const struct
{
    const char *fullname;
    D3DXCONSTANT_DESC desc;
    UINT ctaboffset;
}
test_get_shader_constant_variables_data[] =
{
    {"f",         {"f",   D3DXRS_FLOAT4, 45,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL},  72},
    {"f23",       {"f23", D3DXRS_FLOAT4, 29,  4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 2,  3, 2, 0, 48, NULL},  81},
    {"f23[0]",    {"f23", D3DXRS_FLOAT4, 29,  3, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 2,  3, 1, 0, 24, NULL},  81},
    {"f23[1]",    {"f23", D3DXRS_FLOAT4, 32,  1, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 2,  3, 1, 0, 24, NULL},  93},
    {"f32",       {"f32", D3DXRS_FLOAT4, 33,  4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3,  2, 2, 0, 48, NULL}, 110},
    {"f32[0]",    {"f32", D3DXRS_FLOAT4, 33,  2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3,  2, 1, 0, 24, NULL}, 110},
    {"f32[1]",    {"f32", D3DXRS_FLOAT4, 35,  2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3,  2, 1, 0, 24, NULL}, 118},
    {"f_2",       {"f_2", D3DXRS_FLOAT4, 37,  2, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 2, 0,  8, NULL}, 131},
    {"f_2[0]",    {"f_2", D3DXRS_FLOAT4, 37,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 131},
    {"f_2[1]",    {"f_2", D3DXRS_FLOAT4, 38,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 135},
    {"i",         {"i",   D3DXRS_FLOAT4, 47,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 144},
    {"i[0]",      {"i",   D3DXRS_FLOAT4, 47,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 144},
    {"p",         {"p",   D3DXRS_FLOAT4,  0, 18, D3DXPC_STRUCT,         D3DXPT_VOID,  1, 10, 2, 4, 80, NULL}, 176},
    {"p[0]",      {"p",   D3DXRS_FLOAT4,  0,  9, D3DXPC_STRUCT,         D3DXPT_VOID,  1, 10, 1, 4, 40, NULL}, 176},
    {"p[0].i1",   {"i1",  D3DXRS_FLOAT4,  0,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 176},
    {"p[0].i2",   {"i2",  D3DXRS_FLOAT4,  1,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 180},
    {"p[0].f_2",  {"f_2", D3DXRS_FLOAT4,  2,  1, D3DXPC_VECTOR,         D3DXPT_FLOAT, 1,  2, 1, 0,  8, NULL}, 184},
    {"p[0].r",    {"r",   D3DXRS_FLOAT4,  3,  6, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 2, 0, 24, NULL}, 188},
    {"p[0].r[0]", {"r",   D3DXRS_FLOAT4,  3,  3, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 1, 0, 12, NULL}, 188},
    {"p[0].r[1]", {"r",   D3DXRS_FLOAT4,  6,  3, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 1, 0, 12, NULL}, 200},
    {"p[1]",      {"p",   D3DXRS_FLOAT4,  9,  9, D3DXPC_STRUCT,         D3DXPT_VOID,  1, 10, 1, 4, 40, NULL}, 212},
    {"p[1].i1",   {"i1",  D3DXRS_FLOAT4,  9,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 212},
    {"p[1].i2",   {"i2",  D3DXRS_FLOAT4, 10,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 216},
    {"p[1].f_2",  {"f_2", D3DXRS_FLOAT4, 11,  1, D3DXPC_VECTOR,         D3DXPT_FLOAT, 1,  2, 1, 0,  8, NULL}, 220},
    {"p[1].r",    {"r",   D3DXRS_FLOAT4, 12,  6, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 2, 0, 24, NULL}, 224},
    {"p[1].r[0]", {"r",   D3DXRS_FLOAT4, 12,  3, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 1, 0, 12, NULL}, 224},
    {"p[1].r[1]", {"r",   D3DXRS_FLOAT4, 15,  3, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 1, 0, 12, NULL}, 236},
    {"r13",       {"r13", D3DXRS_FLOAT4, 43,  2, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 1,  3, 2, 0, 24, NULL}, 253},
    {"r13[0]",    {"r13", D3DXRS_FLOAT4, 43,  1, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 1,  3, 1, 0, 12, NULL}, 253},
    {"r13[1]",    {"r13", D3DXRS_FLOAT4, 44,  1, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 1,  3, 1, 0, 12, NULL}, 257},
    {"r31",       {"r31", D3DXRS_FLOAT4, 18,  6, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 2, 0, 24, NULL}, 266},
    {"r31[0]",    {"r31", D3DXRS_FLOAT4, 18,  3, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 1, 0, 12, NULL}, 266},
    {"r31[1]",    {"r31", D3DXRS_FLOAT4, 21,  3, D3DXPC_MATRIX_ROWS,    D3DXPT_FLOAT, 3,  1, 1, 0, 12, NULL}, 278},
    {"s",         {"s",   D3DXRS_FLOAT4, 46,  1, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 1, 2,  8, NULL}, 303},
    {"s.f",       {"f",   D3DXRS_FLOAT4, 46,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 303},
    {"s.i",       {"i",   D3DXRS_FLOAT4, 47,  0, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 307},
    {"s_2",       {"s_2", D3DXRS_FLOAT4, 39,  2, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 2, 2, 16, NULL}, 316},
    {"s_2[0]",    {"s_2", D3DXRS_FLOAT4, 39,  2, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 1, 2,  8, NULL}, 316},
    {"s_2[0].f",  {"f",   D3DXRS_FLOAT4, 39,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 316},
    {"s_2[0].i",  {"i",   D3DXRS_FLOAT4, 40,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 320},
    {"s_2[1]",    {"s_2", D3DXRS_FLOAT4, 41,  0, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 1, 2,  8, NULL}, 324},
    {"s_2[1].f",  {"f",   D3DXRS_FLOAT4, 41,  0, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 324},
    {"s_2[1].i",  {"i",   D3DXRS_FLOAT4, 41,  0, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 328},
    {"s_3",       {"s_3", D3DXRS_FLOAT4, 24,  5, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 3, 2, 24, NULL}, 337},
    {"s_3[0]",    {"s_3", D3DXRS_FLOAT4, 24,  2, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 1, 2,  8, NULL}, 337},
    {"s_3[0].f",  {"f",   D3DXRS_FLOAT4, 24,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 337},
    {"s_3[0].i",  {"i",   D3DXRS_FLOAT4, 25,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 341},
    {"s_3[1]",    {"s_3", D3DXRS_FLOAT4, 26,  2, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 1, 2,  8, NULL}, 345},
    {"s_3[1].f",  {"f",   D3DXRS_FLOAT4, 26,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 345},
    {"s_3[1].i",  {"i",   D3DXRS_FLOAT4, 27,  1, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 349},
    {"s_3[2]",    {"s_3", D3DXRS_FLOAT4, 28,  1, D3DXPC_STRUCT,         D3DXPT_VOID,  1,  2, 1, 2,  8, NULL}, 353},
    {"s_3[2].f",  {"f",   D3DXRS_FLOAT4, 28,  1, D3DXPC_SCALAR,         D3DXPT_FLOAT, 1,  1, 1, 0,  4, NULL}, 353},
    {"s_3[2].i",  {"i",   D3DXRS_FLOAT4, 29,  0, D3DXPC_SCALAR,         D3DXPT_INT,   1,  1, 1, 0,  4, NULL}, 357},
    {"v",         {"v",   D3DXRS_FLOAT4, 41,  2, D3DXPC_VECTOR,         D3DXPT_FLOAT, 1,  3, 2, 0, 24, NULL}, 366},
    {"v[0]",      {"v",   D3DXRS_FLOAT4, 41,  1, D3DXPC_VECTOR,         D3DXPT_FLOAT, 1,  3, 1, 0, 12, NULL}, 366},
    {"v[1]",      {"v",   D3DXRS_FLOAT4, 42,  1, D3DXPC_VECTOR,         D3DXPT_FLOAT, 1,  3, 1, 0, 12, NULL}, 370},
};

static void test_get_shader_constant_variables(void)
{
    ID3DXConstantTable *ctable;
    HRESULT hr;
    ULONG count;
    UINT i;
    UINT nr = 1;
    D3DXHANDLE constant, element;
    D3DXCONSTANT_DESC desc;
    DWORD *ctab;

    hr = D3DXGetShaderConstantTable(test_get_shader_constant_variables_blob, &ctable);
    ok(hr == D3D_OK, "D3DXGetShaderConstantTable failed, got %08x, expected %08x\n", hr, D3D_OK);

    ctab = ID3DXConstantTable_GetBufferPointer(ctable);
    ok(ctab[0] == test_get_shader_constant_variables_blob[3], "ID3DXConstantTable_GetBufferPointer failed\n");

    for (i = 0; i < ARRAY_SIZE(test_get_shader_constant_variables_data); ++i)
    {
        const char *fullname = test_get_shader_constant_variables_data[i].fullname;
        const D3DXCONSTANT_DESC *expected_desc = &test_get_shader_constant_variables_data[i].desc;
        UINT ctaboffset = test_get_shader_constant_variables_data[i].ctaboffset;

        constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, fullname);
        ok(constant != NULL, "GetConstantByName \"%s\" failed\n", fullname);

        hr = ID3DXConstantTable_GetConstantDesc(ctable, constant, &desc, &nr);
        ok(hr == D3D_OK, "GetConstantDesc \"%s\" failed, got %08x, expected %08x\n", fullname, hr, D3D_OK);

        ok(!strcmp(expected_desc->Name, desc.Name), "GetConstantDesc \"%s\" failed, got \"%s\", expected \"%s\"\n",
                fullname, desc.Name, expected_desc->Name);
        ok(expected_desc->RegisterSet == desc.RegisterSet, "GetConstantDesc \"%s\" failed, got %#x, expected %#x\n",
                fullname, desc.RegisterSet, expected_desc->RegisterSet);
        ok(expected_desc->RegisterIndex == desc.RegisterIndex, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                fullname, desc.RegisterIndex, expected_desc->RegisterIndex);
        ok(expected_desc->RegisterCount == desc.RegisterCount, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                fullname, desc.RegisterCount, expected_desc->RegisterCount);
        ok(expected_desc->Class == desc.Class, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                fullname, desc.Class, expected_desc->Class);
        ok(expected_desc->Type == desc.Type, "GetConstantDesc \"%s\" failed, got %#x, expected %#x\n",
                fullname, desc.Type, expected_desc->Type);
        ok(expected_desc->Rows == desc.Rows, "GetConstantDesc \"%s\" failed, got %#x, expected %#x\n",
                fullname, desc.Rows, expected_desc->Rows);
        ok(expected_desc->Columns == desc.Columns, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                fullname, desc.Columns, expected_desc->Columns);
        ok(expected_desc->Elements == desc.Elements, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                fullname, desc.Elements, expected_desc->Elements);
        ok(expected_desc->StructMembers == desc.StructMembers, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                fullname, desc.StructMembers, expected_desc->StructMembers);
        ok(expected_desc->Bytes == desc.Bytes, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                fullname, desc.Bytes, expected_desc->Bytes);
        ok(ctaboffset == (DWORD *)desc.DefaultValue - ctab, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
           fullname, (UINT)((DWORD *)desc.DefaultValue - ctab), ctaboffset);
    }

    element = ID3DXConstantTable_GetConstantElement(ctable, NULL, 0);
    ok(element == NULL, "GetConstantElement failed\n");

    constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, "i");
    ok(constant != NULL, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstantByName(ctable, NULL, "i[0]");
    ok(constant == element, "GetConstantByName failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstantElement(ctable, "i", 0);
    ok(element == constant, "GetConstantElement failed, got %p, expected %p\n", element, constant);

    constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f");
    ok(constant != NULL, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstant(ctable, NULL, 0);
    ok(element == constant, "GetConstant failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstant(ctable, "invalid", 0);
    ok(element == NULL, "GetConstant failed\n");

    element = ID3DXConstantTable_GetConstant(ctable, "f", 0);
    ok(element == NULL, "GetConstant failed\n");

    element = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f[0]");
    ok(constant == element, "GetConstantByName failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f[1]");
    ok(NULL == element, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f[0][0]");
    ok(constant == element, "GetConstantByName failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f.");
    ok(element == NULL, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstantElement(ctable, "f", 0);
    ok(element == constant, "GetConstantElement failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstantElement(ctable, "f", 1);
    ok(element == NULL, "GetConstantElement failed\n");

    constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f_2[0]");
    ok(constant != NULL, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f_2");
    ok(element != constant, "GetConstantByName failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstantElement(ctable, "f_2", 0);
    ok(element == constant, "GetConstantElement failed, got %p, expected %p\n", element, constant);

    constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, "f_2[1]");
    ok(constant != NULL, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstantElement(ctable, "f_2", 1);
    ok(element == constant, "GetConstantElement failed, got %p, expected %p\n", element, constant);

    constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, "s_2[0].f");
    ok(constant != NULL, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstant(ctable, "s_2[0]", 0);
    ok(element == constant, "GetConstant failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstantByName(ctable, "s_2[0]", "f");
    ok(element == constant, "GetConstantByName failed, got %p, expected %p\n", element, constant);

    element = ID3DXConstantTable_GetConstantByName(ctable, "s_2[0]", "invalid");
    ok(element == NULL, "GetConstantByName failed\n");

    constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, "s_2[0]");
    ok(constant != NULL, "GetConstantByName failed\n");

    element = ID3DXConstantTable_GetConstantElement(ctable, "s_2[0]", 0);
    ok(constant == element, "GetConstantByName failed, got %p, expected %p\n", element, constant);

    count = ID3DXConstantTable_Release(ctable);
    ok(count == 0, "Release failed, got %u, expected %u\n", count, 0);
}

#define REGISTER_OUTPUT_SIZE 48

enum Type {SetFloat, SetInt, SetBool, SetIntArray, SetBoolArray, SetFloatArray, SetMatrix,
    SetMatrixTranspose, SetMatrixArray, SetMatrixTransposeArray, SetVector, SetVectorArray,
    SetValue, SetMatrixPointerArray, SetMatrixTransposePointerArray};

struct registerset_test
{
    enum Type type;
    UINT in_index;
    UINT in_count_min;
    UINT in_count_max;
    UINT out_count;
    DWORD out[REGISTER_OUTPUT_SIZE];
};

struct registerset_constants
{
    const char *fullname;
    D3DXCONSTANT_DESC desc;
    UINT ctaboffset;
};

static const DWORD registerset_test_input[][REGISTER_OUTPUT_SIZE] =
{
    /* float */
    {0x40000123, 0x00000000, 0x40800123, 0x40a00123,
    0x40c00123, 0x40e00123, 0x41000123, 0x41100123,
    0x41200123, 0x41300123, 0x41400123, 0x41500123,
    0x41600123, 0x41700123, 0x41800123, 0x41900123,
    0x41a00123, 0x41b00123, 0x41c00123, 0x41d00123,
    0x00000000, 0x41f00123, 0x42000123, 0x42100123,
    0x00000000, 0x42300123, 0x42400123, 0x42500123,
    0x42600123, 0x42700123, 0x42800123, 0x42900123,
    0x43000123, 0x43100123, 0x43200123, 0x43300123,
    0x43400123, 0x43500123, 0x43600123, 0x43700123,
    0x43800123, 0x43900123, 0x43a00123, 0x43b00123,
    0x43c00123, 0x43d00123, 0x43e00123, 0x43f00123},
    /* int */
    {0x00000002, 0x00000003, 0x00000004, 0x00000005,
    0x00000000, 0x00000007, 0x00000008, 0x00000009,
    0x0000000a, 0x0000000b, 0x0000000c, 0x0000000d,
    0x0000000e, 0x0000000f, 0x00000010, 0x00000011,
    0x00000012, 0x00000000, 0x00000000, 0x00000015,
    0x00000016, 0x00000017, 0x00000018, 0x00000019,
    0x0000001a, 0x0000001b, 0x0000001c, 0x0000001d,
    0x0000001e, 0x0000001f, 0x00000020, 0x00000021,
    0x00000022, 0x00000023, 0x00000024, 0x00000025,
    0x00000026, 0x00000027, 0x00000028, 0x00000029,
    0x0000002a, 0x0000002b, 0x0000002c, 0x0000002d,
    0x0000002e, 0x0000002f, 0x00000030, 0x00000031},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
bool b = 1;
int n = 8;
float f = 5.1;
int nf = 11;
bool bf = 1;
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (b) for (i = 0; i < n; i++) tmp.x += pos.z * f * nf;
    else for (i = 0; i < n; i++) tmp.y += pos.y * f * bf;
    return tmp;
}
#endif
static const DWORD registerset_blob_scalar[] =
{
0xfffe0300, 0x0051fffe, 0x42415443, 0x0000001c, 0x0000010f, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x00000108, 0x00000080, 0x00000000, 0x00000001, 0x00000084, 0x00000094, 0x00000098,
0x00020002, 0x00000001, 0x00000084, 0x0000009c, 0x000000ac, 0x00000002, 0x00000001, 0x000000b0,
0x000000c0, 0x000000d0, 0x00000001, 0x00000001, 0x000000d4, 0x000000e4, 0x000000f4, 0x00010002,
0x00000001, 0x000000d4, 0x000000f8, 0xabab0062, 0x00010000, 0x00010001, 0x00000001, 0x00000000,
0xffffffff, 0xab006662, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xabab0066, 0x00030000,
0x00010001, 0x00000001, 0x00000000, 0x40a33333, 0x00000000, 0x00000000, 0x00000000, 0xabab006e,
0x00020000, 0x00010001, 0x00000001, 0x00000000, 0x00000008, 0x00000000, 0x00000001, 0x00000000,
0xab00666e, 0x41300000, 0x00000000, 0x00000000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369,
0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072,
0x392e3932, 0x332e3235, 0x00313131, 0x05000051, 0xa00f0003, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x01000028,
0xe0e40800, 0x03000005, 0x80010000, 0xa0000000, 0x90aa0000, 0x02000001, 0x80010001, 0xa0000003,
0x01000026, 0xf0e40000, 0x04000004, 0x80010001, 0x80000000, 0xa0000001, 0x80000001, 0x00000027,
0x02000001, 0x80020001, 0xa0000003, 0x0000002a, 0x03000005, 0x80010000, 0xa0000000, 0x90550000,
0x02000001, 0x80020001, 0xa0000003, 0x01000026, 0xf0e40000, 0x04000004, 0x80020001, 0x80000000,
0xa0000002, 0x80550001, 0x00000027, 0x02000001, 0x80010001, 0xa0000003, 0x0000002b, 0x02000001,
0xe0030000, 0x80e40001, 0x02000001, 0xe00c0000, 0xa0000003, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_scalar_float[] =
{
    {"f", {"f", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4, NULL}, 48},
};

static const struct registerset_test registerset_test_scalar_float[] =
{
    {SetFloat, 0, 0, 0, 4, {0x40000123}},
    {SetInt, 1, 0, 0, 4, {0x40000000}},
    {SetBool, 1, 0, 0, 4, {0x3f800000}},
    {SetIntArray},
    {SetIntArray, 1, 1, REGISTER_OUTPUT_SIZE, 4, {0x40000000}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, REGISTER_OUTPUT_SIZE, 4, {0x3f800000}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, REGISTER_OUTPUT_SIZE, 4, {0x40000123}},
    {SetValue, 0, 0, 3},
    {SetValue, 0, 4, REGISTER_OUTPUT_SIZE * 4, 4, {0x40000123}},
    {SetVector, 0, 0, 0, 4, {0x40000123}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4, {0x40000123}},
    {SetMatrix, 0, 0, 0, 4, {0x40000123},},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123}},
    {SetMatrixTranspose, 0, 0, 0, 4, {0x40000123},},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123}},
};

static const struct registerset_constants registerset_constants_scalar_int[] =
{
    {"n", {"n", D3DXRS_INT4, 0, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4, NULL}, 57},
};

static const struct registerset_test registerset_test_scalar_int[] =
{
    {SetFloat, 0, 0, 0, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetInt, 1, 0, 0, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetBool, 1, 0, 0, 4,
        {0x00000001, 0x00000000, 0x00000001}},
    {SetIntArray},
    {SetIntArray, 1, 1, REGISTER_OUTPUT_SIZE, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, REGISTER_OUTPUT_SIZE, 4,
        {0x00000001, 0x00000000, 0x00000001}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, REGISTER_OUTPUT_SIZE, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, REGISTER_OUTPUT_SIZE * 4, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetVector, 0, 0, 0, 4,
        {0x00000002, 0x00000000, 0x00000001},},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetMatrix, 0, 0, 0, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000002, 0x00000000, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000002, 0x00000000, 0x00000001}},
};

static const struct registerset_constants registerset_constants_scalar_int_float[] =
{
    {"nf", {"nf", D3DXRS_FLOAT4, 1, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4, NULL}, 62},
};

static const struct registerset_test registerset_test_scalar_int_float[] =
{
    {SetFloat, 0, 0, 0, 4, {0x40000000}},
    {SetInt, 1, 0, 0, 4, {0x40000000}},
    {SetBool, 1, 0, 0, 4, {0x3f800000}},
    {SetIntArray},
    {SetIntArray, 1, 1, REGISTER_OUTPUT_SIZE, 4, {0x40000000}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, REGISTER_OUTPUT_SIZE, 4, {0x3f800000}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, REGISTER_OUTPUT_SIZE, 4, {0x40000000}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, REGISTER_OUTPUT_SIZE * 4, 4, {0x40000000}},
    {SetVector, 0, 0, 0, 4, {0x40000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4, {0x40000000}},
    {SetMatrix, 0, 0, 0, 4, {0x40000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000}},
    {SetMatrixTranspose, 0, 0, 0, 4, {0x40000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000}},
};

static const struct registerset_constants registerset_constants_scalar_bool_float[] =
{
    {"bf", {"bf", D3DXRS_FLOAT4, 2, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0, 4, NULL}, 39},
};

static const struct registerset_test registerset_test_scalar_bool_float[] =
{
    {SetFloat, 0, 0, 0, 4, {0x3f800000}},
    {SetInt, 1, 0, 0, 4, {0x3f800000}},
    {SetBool, 1, 0, 0, 4, {0x3f800000}},
    {SetIntArray},
    {SetIntArray, 1, 1, REGISTER_OUTPUT_SIZE, 4, {0x3f800000}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, REGISTER_OUTPUT_SIZE, 4, {0x3f800000}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, REGISTER_OUTPUT_SIZE, 4, {0x3f800000}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, REGISTER_OUTPUT_SIZE * 4, 4, {0x3f800000}},
    {SetVector, 0, 0, 0, 4, {0x3f800000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4, {0x3f800000}},
    {SetMatrix, 0, 0, 0, 4, {0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 4, {0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000}},
};

static const struct registerset_constants registerset_constants_scalar_bool[] =
{
    {"b", {"b", D3DXRS_BOOL, 0, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0, 4, NULL}, 37},
};

static const struct registerset_test registerset_test_scalar_bool[] =
{
    {SetFloat, 0, 0, 0, 1, {0x00000001}},
    {SetInt, 1, 0, 0, 1, {0x00000001}},
    {SetBool, 1, 0, 0, 1, {0x00000002}},
    {SetIntArray},
    {SetIntArray, 1, 1, REGISTER_OUTPUT_SIZE, 1, {0x00000001}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, REGISTER_OUTPUT_SIZE, 1, {0x00000002}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, REGISTER_OUTPUT_SIZE, 1, {0x00000001}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, REGISTER_OUTPUT_SIZE * 4, 1, {0x00000002}},
    {SetVector, 0, 0, 0, 1, {0x00000001}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 1, {0x00000001}},
    {SetMatrix, 0, 0, 0, 1, {0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 1, {0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 1, {0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 1, {0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 1, {0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 1, {0x00000001}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
bool ab[2] = {1, 0};
int an[2] = {32, 33};
float af[2] = {3.1, 3.2};
int anf[2] = {14, 15};
bool abf[2] = {1, 1};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (ab[1]) for (i = 0; i < an[1]; i++) tmp.x += pos.z * af[0] * anf[1];
    else for (i = 0; i < an[0]; i++) tmp.y += pos.y * af[1] * abf[1];
    return tmp;
}
#endif
static const DWORD registerset_blob_scalar_array[] =
{
0xfffe0300, 0x006afffe, 0x42415443, 0x0000001c, 0x00000173, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x0000016c, 0x00000080, 0x00000000, 0x00000002, 0x00000084, 0x00000094, 0x0000009c,
0x00040002, 0x00000002, 0x000000a0, 0x000000b0, 0x000000d0, 0x00000002, 0x00000002, 0x000000d4,
0x000000e4, 0x00000104, 0x00000001, 0x00000002, 0x00000108, 0x00000118, 0x00000138, 0x00020002,
0x00000002, 0x0000013c, 0x0000014c, 0xab006261, 0x00010000, 0x00010001, 0x00000002, 0x00000000,
0xffffffff, 0x00000000, 0x00666261, 0x00010000, 0x00010001, 0x00000002, 0x00000000, 0x3f800000,
0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xab006661,
0x00030000, 0x00010001, 0x00000002, 0x00000000, 0x40466666, 0x00000000, 0x00000000, 0x00000000,
0x404ccccd, 0x00000000, 0x00000000, 0x00000000, 0xab006e61, 0x00020000, 0x00010001, 0x00000002,
0x00000000, 0x00000020, 0x00000000, 0x00000001, 0x00000000, 0x00000021, 0x00000000, 0x00000001,
0x00000000, 0x00666e61, 0x00020000, 0x00010001, 0x00000002, 0x00000000, 0x41600000, 0x00000000,
0x00000000, 0x00000000, 0x41700000, 0x00000000, 0x00000000, 0x00000000, 0x335f7376, 0x4d00305f,
0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
0x2e392072, 0x392e3932, 0x332e3235, 0x00313131, 0x05000051, 0xa00f0006, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000,
0x01000028, 0xe0e40801, 0x03000005, 0x80010000, 0xa0000000, 0x90aa0000, 0x02000001, 0x80010001,
0xa0000006, 0x01000026, 0xf0e40001, 0x04000004, 0x80010001, 0x80000000, 0xa0000003, 0x80000001,
0x00000027, 0x02000001, 0x80020001, 0xa0000006, 0x0000002a, 0x03000005, 0x80010000, 0xa0000001,
0x90550000, 0x02000001, 0x80020001, 0xa0000006, 0x01000026, 0xf0e40000, 0x04000004, 0x80020001,
0x80000000, 0xa0000005, 0x80550001, 0x00000027, 0x02000001, 0x80010001, 0xa0000006, 0x0000002b,
0x02000001, 0xe0030000, 0x80e40001, 0x02000001, 0xe00c0000, 0xa0000006, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_scalar_array_float[] =
{
    {"af",    {"af", D3DXRS_FLOAT4, 0, 2, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 2, 0, 8, NULL}, 57},
    {"af[0]", {"af", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4, NULL}, 57},
    {"af[1]", {"af", D3DXRS_FLOAT4, 1, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4, NULL}, 61},
};

static const struct registerset_test registerset_test_scalar_array_float[] =
{
    {SetFloat, 0, 0, 0, 4, {0x40000123}},
    {SetInt, 1, 0, 0, 4, {0x40000000}},
    {SetBool, 1, 0, 0, 4, {0x3f800000}},
    {SetIntArray},
    {SetIntArray, 1, 1, 1, 4, {0x40000000}},
    {SetIntArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, 1, 4, {0x3f800000}},
    {SetBoolArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, 1, 4, {0x40000123}},
    {SetFloatArray, 0, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetValue, 0, 0, 3},
    {SetValue, 0, 4, 7, 4, {0x40000123}},
    {SetValue, 0, 8, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123}},
};

static const struct registerset_constants registerset_constants_scalar_array_int[] =
{
    {"an",    {"an", D3DXRS_INT4, 0, 2, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 2, 0, 8, NULL}, 70},
    {"an[0]", {"an", D3DXRS_INT4, 0, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4, NULL}, 70},
    {"an[1]", {"an", D3DXRS_INT4, 1, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4, NULL}, 74},
};

static const struct registerset_test registerset_test_scalar_array_int[] =
{
    {SetFloat, 0, 0, 0, 4, {0x00000002, 0x00000000, 0x00000001}},
    {SetInt, 1, 0, 0, 4, {0x00000002, 0x00000000, 0x00000001}},
    {SetBool, 1, 0, 0, 4, {0x00000001, 0x00000000, 0x00000001}},
    {SetIntArray},
    {SetIntArray, 1, 1, 1, 4, {0x00000002, 0x00000000, 0x00000001}},
    {SetIntArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000000, 0x00000001}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, 1, 4, {0x00000001, 0x00000000, 0x00000001}},
    {SetBoolArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, 1, 4, {0x00000002, 0x00000000, 0x00000001}},
    {SetFloatArray, 0, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, 7, 4, {0x00000002, 0x00000000, 0x00000001}},
    {SetValue, 1, 8, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000000, 0x00000001}},
    {SetVector, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetMatrix, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000000, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000000, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000000, 0x00000001}},
};

static const struct registerset_constants registerset_constants_scalar_array_bool[] =
{
    {"ab",    {"ab", D3DXRS_BOOL, 0, 2, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 2, 0, 8, NULL}, 37},
    {"ab[0]", {"ab", D3DXRS_BOOL, 0, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0, 4, NULL}, 37},
    {"ab[1]", {"ab", D3DXRS_BOOL, 1, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0, 4, NULL}, 38},
};

static const struct registerset_test registerset_test_scalar_array_bool[] =
{
    {SetFloat, 0, 0, 0, 1, {0x00000001}},
    {SetInt, 1, 0, 0, 1, {0x00000001}},
    {SetBool, 1, 0, 0, 1, {0x00000002}},
    {SetIntArray},
    {SetIntArray, 1, 1, 1, 1, {0x00000001}},
    {SetIntArray, 1, 2, REGISTER_OUTPUT_SIZE, 2, {0x00000001, 0x00000001}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, 1, 1, {0x00000002}},
    {SetBoolArray, 1, 2, REGISTER_OUTPUT_SIZE, 2, {0x00000002, 0x00000003}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, 1, 1, {0x00000001}},
    {SetFloatArray, 0, 2, REGISTER_OUTPUT_SIZE, 2, {0x00000001, 0x00000000}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, 7, 1, {0x00000002}},
    {SetValue, 1, 8, REGISTER_OUTPUT_SIZE * 4, 2, {0x00000002, 0x00000003}},
    {SetVector, 0, 0, 0, 2, {0x00000001, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 2, {0x00000001, 0x00000000}},
    {SetMatrix, 0, 0, 0, 2, {0x00000001, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 2, {0x00000001, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 2, {0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 2, {0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 2, {0x00000001, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 2, {0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_scalar_array_bool_float[] =
{
    {"abf",    {"abf", D3DXRS_FLOAT4, 4, 2, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 2, 0, 8, NULL}, 44},
    {"abf[0]", {"abf", D3DXRS_FLOAT4, 4, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0, 4, NULL}, 44},
    {"abf[1]", {"abf", D3DXRS_FLOAT4, 5, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0, 4, NULL}, 48},
};

static const struct registerset_test registerset_test_scalar_array_bool_float[] =
{
    {SetFloat, 0, 0, 0, 4, {0x3f800000}},
    {SetInt, 1, 0, 0, 4, {0x3f800000}},
    {SetBool, 1, 0, 0, 4, {0x3f800000}},
    {SetIntArray},
    {SetIntArray, 1, 1, 1, 4, {0x3f800000}},
    {SetIntArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, 1, 4, {0x3f800000}},
    {SetBoolArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, 1, 4, {0x3f800000}},
    {SetFloatArray, 0, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, 7, 4, {0x3f800000}},
    {SetValue, 1, 8, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetVector, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetVectorArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
};

static const struct registerset_constants registerset_constants_scalar_array_int_float[] =
{
    {"anf",    {"anf", D3DXRS_FLOAT4, 2, 2, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 2, 0, 8, NULL}, 83},
    {"anf[0]", {"anf", D3DXRS_FLOAT4, 2, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4, NULL}, 83},
    {"anf[1]", {"anf", D3DXRS_FLOAT4, 3, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4, NULL}, 87},
};

static const struct registerset_test registerset_test_scalar_array_int_float[] =
{
    {SetFloat, 0, 0, 0, 4, {0x40000000}},
    {SetInt, 1, 0, 0, 4, {0x40000000}},
    {SetBool, 1, 0, 0, 4, {0x3f800000}},
    {SetIntArray},
    {SetIntArray, 1, 1, 1, 4, {0x40000000}},
    {SetIntArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000}},
    {SetBoolArray},
    {SetBoolArray, 1, 1, 1, 4, {0x3f800000}},
    {SetBoolArray, 1, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetFloatArray},
    {SetFloatArray, 0, 1, 1, 4, {0x40000000}},
    {SetFloatArray, 0, 2, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetValue, 1, 0, 3},
    {SetValue, 1, 4, 7, 4, {0x40000000}},
    {SetValue, 1, 8, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000}},
    {SetVector, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
bool3 vb = {1, 0, 1};
int3 vn = {7, 8, 9};
float3 vf = {5.1, 5.2, 5.3};
int3 vnf = {11, 85, 62};
bool3 vbf = {1, 1, 1};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (vb.z) for (i = 0; i < vn.z; i++) tmp.x += pos.z * vf.z * vnf.z;
    else for (i = 0; i < vn.y; i++) tmp.y += pos.y * vf.y * vbf.z;
    return tmp;
}
#endif
static const DWORD registerset_blob_vector[] =
{
0xfffe0300, 0x0053fffe, 0x42415443, 0x0000001c, 0x00000117, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x00000110, 0x00000080, 0x00000000, 0x00000003, 0x00000084, 0x00000094, 0x000000a0,
0x00020002, 0x00000001, 0x00000084, 0x000000a4, 0x000000b4, 0x00000002, 0x00000001, 0x000000b8,
0x000000c8, 0x000000d8, 0x00000001, 0x00000003, 0x000000dc, 0x000000ec, 0x000000fc, 0x00010002,
0x00000001, 0x000000dc, 0x00000100, 0xab006276, 0x00010001, 0x00030001, 0x00000001, 0x00000000,
0xffffffff, 0x00000000, 0xffffffff, 0x00666276, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
0xab006676, 0x00030001, 0x00030001, 0x00000001, 0x00000000, 0x40a33333, 0x40a66666, 0x40a9999a,
0x00000000, 0xab006e76, 0x00020001, 0x00030001, 0x00000001, 0x00000000, 0x00000007, 0x00000008,
0x00000009, 0x00000000, 0x00666e76, 0x41300000, 0x42aa0000, 0x42780000, 0x00000000, 0x335f7376,
0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320,
0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131, 0x05000051, 0xa00f0003, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f, 0x80000000,
0xe00f0000, 0x01000028, 0xe0e40802, 0x03000005, 0x80010000, 0xa0aa0000, 0x90aa0000, 0x02000001,
0x80010001, 0xa0000003, 0x01000026, 0xf0e40002, 0x04000004, 0x80010001, 0x80000000, 0xa0aa0001,
0x80000001, 0x00000027, 0x02000001, 0x80020001, 0xa0000003, 0x0000002a, 0x03000005, 0x80010000,
0xa0550000, 0x90550000, 0x02000001, 0x80020001, 0xa0000003, 0x01000026, 0xf0e40001, 0x04000004,
0x80020001, 0x80000000, 0xa0aa0002, 0x80550001, 0x00000027, 0x02000001, 0x80010001, 0xa0000003,
0x0000002b, 0x02000001, 0xe0030000, 0x80e40001, 0x02000001, 0xe00c0000, 0xa0000003, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_vector_float[] =
{
    {"vf", {"vf", D3DXRS_FLOAT4, 0, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 1, 0, 12, NULL}, 50},
};

static const struct registerset_test registerset_test_vector_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x40000000, 0x40400000, 0x40800000}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, REGISTER_OUTPUT_SIZE, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetValue, 0, 0, 11},
    {SetValue, 0, 12, REGISTER_OUTPUT_SIZE * 4, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetVector, 0, 0, 0, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetMatrix, 0, 0, 0, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetMatrixTranspose, 0, 0, 0, 4, {0x40000123, 0x40c00123, 0x41200123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123, 0x40c00123, 0x41200123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000123, 0x40c00123, 0x41200123}},
};

static const struct registerset_constants registerset_constants_vector_int[] =
{
    {"vn", {"vn", D3DXRS_INT4, 0, 3, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 1, 0, 12, NULL}, 59},
};

static const struct registerset_test registerset_test_vector_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x00000002, 0x00000003, 0x00000004}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x00000001, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, REGISTER_OUTPUT_SIZE, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, REGISTER_OUTPUT_SIZE * 4, 4, {0x00000002, 0x00000003, 0x00000004}},
    {SetVector, 0, 0, 0, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetMatrix, 0, 0, 0, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetMatrixTranspose, 0, 0, 0, 4, {0x00000002, 0x00000006, 0x0000000a}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x00000002, 0x00000006, 0x0000000a}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x00000002, 0x00000006, 0x0000000a}},
};

static const struct registerset_constants registerset_constants_vector_bool[] =
{
    {"vb", {"vb", D3DXRS_BOOL, 0, 3, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 37},
};

static const struct registerset_test registerset_test_vector_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, REGISTER_OUTPUT_SIZE, 3, {0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, REGISTER_OUTPUT_SIZE, 3, {0x00000002, 0x00000003, 0x00000004}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, REGISTER_OUTPUT_SIZE, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, REGISTER_OUTPUT_SIZE * 4, 3, {0x00000002, 0x00000003, 0x00000004}},
    {SetVector, 0, 0, 0, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetMatrix, 0, 0, 0, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 3, {0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 3, {0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 3, {0x00000001, 0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_vector_bool_float[] =
{
    {"vbf", {"vbf", D3DXRS_FLOAT4, 2, 1, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 41},
};

static const struct registerset_test registerset_test_vector_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, REGISTER_OUTPUT_SIZE, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, REGISTER_OUTPUT_SIZE * 4, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetVector, 0, 0, 0, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetMatrix, 0, 0, 0, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
};

static const struct registerset_constants registerset_constants_vector_int_float[] =
{
    {"vnf", {"vnf", D3DXRS_FLOAT4, 1, 1, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 1, 0, 12, NULL}, 64},
};

static const struct registerset_test registerset_test_vector_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x40000000, 0x40400000, 0x40800000}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, REGISTER_OUTPUT_SIZE, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, REGISTER_OUTPUT_SIZE, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, REGISTER_OUTPUT_SIZE * 4, 4, {0x40000000, 0x40400000, 0x40800000}},
    {SetVector, 0, 0, 0, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetMatrix, 0, 0, 0, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetMatrixTranspose, 0, 0, 0, 4, {0x40000000, 0x40c00000, 0x41200000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000, 0x40c00000, 0x41200000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4, {0x40000000, 0x40c00000, 0x41200000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
bool3 vab[2] = {1, 0, 1, 1, 0, 1};
int3 van[2] = {70, 80, 90, 100, 110, 120};
float3 vaf[2] = {55.1, 55.2, 55.3, 55.4, 55.5, 55.6};
int3 vanf[2] = {130, 140, 150, 160, 170, 180};
bool3 vabf[2] = {1, 1, 1, 1, 1, 1};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (vab[1].z) for (i = 0; i < van[1].z; i++) tmp.x += pos.z * vaf[1].z * vanf[1].z;
    else for (i = 0; i < van[1].y; i++) tmp.y += pos.y * vaf[0].y * vabf[1].z;
    return tmp;
}
#endif
static const DWORD registerset_blob_vector_array[] =
{
0xfffe0300, 0x0070fffe, 0x42415443, 0x0000001c, 0x0000018b, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x00000184, 0x00000080, 0x00000000, 0x00000006, 0x00000084, 0x00000094, 0x000000ac,
0x00040002, 0x00000002, 0x000000b4, 0x000000c4, 0x000000e4, 0x00000002, 0x00000002, 0x000000e8,
0x000000f8, 0x00000118, 0x00000001, 0x00000006, 0x0000011c, 0x0000012c, 0x0000014c, 0x00020002,
0x00000002, 0x00000154, 0x00000164, 0x00626176, 0x00010001, 0x00030001, 0x00000002, 0x00000000,
0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0x66626176, 0xababab00,
0x00010001, 0x00030001, 0x00000002, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00666176, 0x00030001, 0x00030001, 0x00000002,
0x00000000, 0x425c6666, 0x425ccccd, 0x425d3333, 0x00000000, 0x425d999a, 0x425e0000, 0x425e6666,
0x00000000, 0x006e6176, 0x00020001, 0x00030001, 0x00000002, 0x00000000, 0x00000046, 0x00000050,
0x0000005a, 0x00000000, 0x00000064, 0x0000006e, 0x00000078, 0x00000000, 0x666e6176, 0xababab00,
0x00020001, 0x00030001, 0x00000002, 0x00000000, 0x43020000, 0x430c0000, 0x43160000, 0x00000000,
0x43200000, 0x432a0000, 0x43340000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73,
0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932,
0x332e3235, 0x00313131, 0x05000051, 0xa00f0006, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x0200001f, 0x80000000, 0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x01000028, 0xe0e40805,
0x03000005, 0x80010000, 0xa0aa0001, 0x90aa0000, 0x02000001, 0x80010001, 0xa0000006, 0x01000026,
0xf0e40005, 0x04000004, 0x80010001, 0x80000000, 0xa0aa0003, 0x80000001, 0x00000027, 0x02000001,
0x80020001, 0xa0000006, 0x0000002a, 0x03000005, 0x80010000, 0xa0550000, 0x90550000, 0x02000001,
0x80020001, 0xa0000006, 0x01000026, 0xf0e40004, 0x04000004, 0x80020001, 0x80000000, 0xa0aa0005,
0x80550001, 0x00000027, 0x02000001, 0x80010001, 0xa0000006, 0x0000002b, 0x02000001, 0xe0030000,
0x80e40001, 0x02000001, 0xe00c0000, 0xa0000006, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_vector_array_float[] =
{
    {"vaf",    {"vaf", D3DXRS_FLOAT4, 0, 2, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 2, 0, 24, NULL}, 62},
    {"vaf[0]", {"vaf", D3DXRS_FLOAT4, 0, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 1, 0, 12, NULL}, 62},
    {"vaf[1]", {"vaf", D3DXRS_FLOAT4, 1, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 1, 0, 12, NULL}, 66},
};

static const struct registerset_test registerset_test_vector_array_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, 5, 4, {0x40000000, 0x40400000, 0x40800000}},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x40400000, 0x40800000, 0x00000000, 0x40a00000, 0x00000000, 0x40e00000}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, 5, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, 5, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40a00123, 0x40c00123, 0x40e00123}},
    {SetValue, 0, 0, 11},
    {SetValue, 0, 12, 23, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetValue, 0, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40a00123, 0x40c00123, 0x40e00123}},
    {SetVector, 0, 0, 0, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 4, {0x40000123, 0x00000000, 0x40800123}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
};

static const struct registerset_constants registerset_constants_vector_array_int[] =
{
    {"van",    {"van", D3DXRS_INT4, 0, 6, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 2, 0, 24, NULL}, 75},
    {"van[0]", {"van", D3DXRS_INT4, 0, 1, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 1, 0, 12, NULL}, 75},
    {"van[1]", {"van", D3DXRS_INT4, 1, 1, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 1, 0, 12, NULL}, 79},
};

static const struct registerset_test registerset_test_vector_array_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, 5, 4, {0x00000002, 0x00000003, 0x00000004}},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000003, 0x00000004, 0x00000000, 0x00000005, 0x00000000, 0x00000007}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, 5, 4, {0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, 5, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000005, 0x00000006, 0x00000007}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, 23, 4, {0x00000002, 0x00000003, 0x00000004}},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x00000002, 0x00000003, 0x00000004, 0x00000000, 0x00000005, 0x00000000, 0x00000007}},
    {SetVector, 0, 0, 0, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetVectorArray, 0, 0, 0},
    {SetVectorArray, 0, 1, 1, 4, {0x00000002, 0x00000000, 0x00000004}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrix, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
};

static const struct registerset_constants registerset_constants_vector_array_bool[] =
{
    {"vab",    {"vab", D3DXRS_BOOL, 0, 6, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 2, 0, 24, NULL}, 37},
    {"vab[0]", {"vab", D3DXRS_BOOL, 0, 3, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 37},
    {"vab[1]", {"vab", D3DXRS_BOOL, 3, 3, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 40},
};

static const struct registerset_test registerset_test_vector_array_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, 5, 3, {0x00000001, 0x00000001, 0x00000001}},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001}},
    {SetBoolArray, 1, 0},
    {SetBoolArray, 1, 3, 5, 3, {0x00000002, 0x00000003, 0x00000004}},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, 5, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, 23, 3, {0x00000002, 0x00000003, 0x00000004}},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetVector, 0, 0, 0, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 3, {0x00000001, 0x00000000, 0x00000001}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrix, 0, 0, 0, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_vector_array_bool_float[] =
{
    {"vabf",    {"vabf", D3DXRS_FLOAT4, 4, 2, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 2, 0, 24, NULL}, 49},
    {"vabf[0]", {"vabf", D3DXRS_FLOAT4, 4, 1, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 49},
    {"vabf[1]", {"vabf", D3DXRS_FLOAT4, 5, 1, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 53},
};

static const struct registerset_test registerset_test_vector_array_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, 5, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, 5, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 2},
    {SetFloatArray, 0, 3, 5, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, 23, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetVector, 0, 0, 0, 4, {0x3f800000, 0x00000000, 0x3f800000},},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 4, {0x3f800000, 0x00000000, 0x3f800000}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
};

static const struct registerset_constants registerset_constants_vector_array_int_float[] =
{
    {"vanf",    {"vanf", D3DXRS_FLOAT4, 2, 2, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 2, 0, 24, NULL}, 89},
    {"vanf[0]", {"vanf", D3DXRS_FLOAT4, 2, 1, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 1, 0, 12, NULL}, 89},
    {"vanf[1]", {"vanf", D3DXRS_FLOAT4, 3, 1, D3DXPC_VECTOR, D3DXPT_INT, 1, 3, 1, 0, 12, NULL}, 93},
};

static const struct registerset_test registerset_test_vector_array_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 2},
    {SetIntArray, 1, 3, 5, 4, {0x40000000, 0x40400000, 0x40800000}},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x40400000, 0x40800000, 0x00000000, 0x40a00000, 0x00000000, 0x40e00000}},
    {SetBoolArray, 1, 0, 2},
    {SetBoolArray, 1, 3, 5, 4, {0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetFloatArray, 2, 0, 2,},
    {SetFloatArray, 0, 3, 5, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40a00000, 0x40c00000, 0x40e00000}},
    {SetValue, 1, 0, 11},
    {SetValue, 1, 12, 23, 4, {0x40000000, 0x40400000, 0x40800000}},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000000, 0x40400000, 0x40800000, 0x00000000, 0x40a00000, 0x00000000, 0x40e00000}},
    {SetVector, 0, 0, 0, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 4, {0x40000000, 0x00000000, 0x40800000}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
bool3x2 cb = {1, 0, 1, 1, 0, 1};
int3x2 cn = {4, 5, 6, 7, 8, 9};
float3x2 cf = {15.1, 15.2, 15.3, 15.4, 15.5, 15.6};
bool3x2 cbf = {1, 1, 0, 1, 0, 1};
int3x2 cnf = {30, 31, 33, 32, 34, 36};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (cb._32) for (i = 0; i < cn._31; i++) tmp.x += pos.z * cf._31 * cbf._32;
    else for (i = 0; i < cn._32; i++) tmp.y += pos.y * cf._32 * cnf._32;
    return tmp;
}
#endif
static const DWORD registerset_blob_column[] =
{
0xfffe0300, 0x0066fffe, 0x42415443, 0x0000001c, 0x00000163, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x0000015c, 0x00000080, 0x00000000, 0x00000006, 0x00000084, 0x00000094, 0x000000ac,
0x00020002, 0x00000002, 0x00000084, 0x000000b0, 0x000000d0, 0x00000002, 0x00000002, 0x000000d4,
0x000000e4, 0x00000104, 0x00000001, 0x00000006, 0x00000108, 0x00000118, 0x00000138, 0x00040002,
0x00000002, 0x00000108, 0x0000013c, 0xab006263, 0x00010003, 0x00020003, 0x00000001, 0x00000000,
0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0x00666263, 0x3f800000,
0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0xab006663,
0x00030003, 0x00020003, 0x00000001, 0x00000000, 0x4171999a, 0x4174cccd, 0x41780000, 0x00000000,
0x41733333, 0x41766666, 0x4179999a, 0x00000000, 0xab006e63, 0x00020003, 0x00020003, 0x00000001,
0x00000000, 0x00000004, 0x00000006, 0x00000008, 0x00000000, 0x00000005, 0x00000007, 0x00000009,
0x00000000, 0x00666e63, 0x41f00000, 0x42040000, 0x42080000, 0x00000000, 0x41f80000, 0x42000000,
0x42100000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,
0x05000051, 0xa00f0006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000,
0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x01000028, 0xe0e40805, 0x03000005, 0x80010000,
0xa0aa0000, 0x90aa0000, 0x02000001, 0x80010001, 0xa0000006, 0x01000026, 0xf0e40004, 0x04000004,
0x80010001, 0x80000000, 0xa0aa0003, 0x80000001, 0x00000027, 0x02000001, 0x80020001, 0xa0000006,
0x0000002a, 0x03000005, 0x80010000, 0xa0aa0001, 0x90550000, 0x02000001, 0x80020001, 0xa0000006,
0x01000026, 0xf0e40005, 0x04000004, 0x80020001, 0x80000000, 0xa0aa0005, 0x80550001, 0x00000027,
0x02000001, 0x80010001, 0xa0000006, 0x0000002b, 0x02000001, 0xe0030000, 0x80e40001, 0x02000001,
0xe00c0000, 0xa0000006, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_column_float[] =
{
    {"cf", {"cf", D3DXRS_FLOAT4, 0, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL}, 57},
};

static const struct registerset_test registerset_test_column_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000123, 0x40800123, 0x40c00123, 0x00000000, 0x00000000, 0x40a00123, 0x40e00123}},
    {SetValue, 0, 0, 23},
    {SetValue, 0, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000123, 0x40800123, 0x40c00123, 0x00000000, 0x00000000, 0x40a00123, 0x40e00123}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
};

static const struct registerset_constants registerset_constants_column_int[] =
{
    {"cn", {"cn", D3DXRS_INT4, 0, 6, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 70},
};

static const struct registerset_test registerset_test_column_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000004, 0x00000000, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x00000001, 0x00000001, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000004, 0x00000006, 0x00000000, 0x00000000, 0x00000005, 0x00000007}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x00000002, 0x00000004, 0x00000000, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrix, 0, 0, 0, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
};

static const struct registerset_constants registerset_constants_column_bool[] =
{
    {"cb", {"cb", D3DXRS_BOOL, 0, 6, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 37},
};

static const struct registerset_test registerset_test_column_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 6,
        {0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrix, 0, 0, 0, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_column_int_float[] =
{
    {"cnf", {"cnf", D3DXRS_FLOAT4, 4, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 79},
};

static const struct registerset_test registerset_test_column_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x40800000, 0x40c00000, 0x00000000, 0x00000000, 0x40a00000, 0x40e00000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetValue, 0, 0, 23},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
};

static const struct registerset_constants registerset_constants_column_bool_float[] =
{
    {"cbf", {"cbf", D3DXRS_FLOAT4, 2, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 44},
};

static const struct registerset_test registerset_test_column_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
bool3x2 cab[2] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1};
int3x2 can[2] = {14, 15, 16, 71, 18, 19, 55, 63, 96, 96, 97, 13};
float3x2 caf[2] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 1.2, 1.3, 1.4};
bool3x2 cabf[2] = {1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1};
int3x2 canf[2] = {300, 301, 303, 302, 304, 306, 350, 365, 654, 612, 326, 999};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (cab[1]._32) for (i = 0; i < can[1]._31; i++) tmp.x += pos.z * caf[1]._31 * cabf[1]._32;
    else for (i = 0; i < can[1]._32; i++) tmp.y += pos.y * caf[1]._32 * canf[1]._32;
    return tmp;
}
#endif
static const DWORD registerset_blob_column_array[] =
{
0xfffe0300, 0x0096fffe, 0x42415443, 0x0000001c, 0x00000223, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x0000021c, 0x00000080, 0x00000000, 0x0000000c, 0x00000084, 0x00000094, 0x000000c4,
0x00040002, 0x00000004, 0x000000cc, 0x000000dc, 0x0000011c, 0x00000002, 0x00000004, 0x00000120,
0x00000130, 0x00000170, 0x00000001, 0x0000000c, 0x00000174, 0x00000184, 0x000001c4, 0x00080002,
0x00000004, 0x000001cc, 0x000001dc, 0x00626163, 0x00010003, 0x00020003, 0x00000002, 0x00000000,
0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000,
0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0x66626163, 0xababab00, 0x00010003, 0x00020003,
0x00000002, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000,
0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000,
0x3f800000, 0x00000000, 0x00666163, 0x00030003, 0x00020003, 0x00000002, 0x00000000, 0x3f8ccccd,
0x40533333, 0x40b00000, 0x00000000, 0x400ccccd, 0x408ccccd, 0x40d33333, 0x00000000, 0x40f66666,
0x411e6666, 0x3fa66666, 0x00000000, 0x410ccccd, 0x3f99999a, 0x3fb33333, 0x00000000, 0x006e6163,
0x00020003, 0x00020003, 0x00000002, 0x00000000, 0x0000000e, 0x00000010, 0x00000012, 0x00000000,
0x0000000f, 0x00000047, 0x00000013, 0x00000000, 0x00000037, 0x00000060, 0x00000061, 0x00000000,
0x0000003f, 0x00000060, 0x0000000d, 0x00000000, 0x666e6163, 0xababab00, 0x00020003, 0x00020003,
0x00000002, 0x00000000, 0x43960000, 0x43978000, 0x43980000, 0x00000000, 0x43968000, 0x43970000,
0x43990000, 0x00000000, 0x43af0000, 0x44238000, 0x43a30000, 0x00000000, 0x43b68000, 0x44190000,
0x4479c000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,
0x05000051, 0xa00f000c, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000,
0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x01000028, 0xe0e4080b, 0x03000005, 0x80010000,
0xa0aa0002, 0x90aa0000, 0x02000001, 0x80010001, 0xa000000c, 0x01000026, 0xf0e4000a, 0x04000004,
0x80010001, 0x80000000, 0xa0aa0007, 0x80000001, 0x00000027, 0x02000001, 0x80020001, 0xa000000c,
0x0000002a, 0x03000005, 0x80010000, 0xa0aa0003, 0x90550000, 0x02000001, 0x80020001, 0xa000000c,
0x01000026, 0xf0e4000b, 0x04000004, 0x80020001, 0x80000000, 0xa0aa000b, 0x80550001, 0x00000027,
0x02000001, 0x80010001, 0xa000000c, 0x0000002b, 0x02000001, 0xe0030000, 0x80e40001, 0x02000001,
0xe00c0000, 0xa000000c, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_column_array_float[] =
{
    {"caf",    {"caf", D3DXRS_FLOAT4, 0, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 2, 2, 0, 48, NULL}, 76},
    {"caf[0]", {"caf", D3DXRS_FLOAT4, 0, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL}, 76},
    {"caf[1]", {"caf", D3DXRS_FLOAT4, 2, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL}, 84},
};

static const struct registerset_test registerset_test_column_array_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 8,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000, 0x00000000,
        0x41000000, 0x41200000, 0x41400000, 0x00000000, 0x41100000, 0x41300000, 0x41500000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 8,
        {0x40000123, 0x40800123, 0x40c00123, 0x00000000, 0x00000000, 0x40a00123, 0x40e00123}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x40000123, 0x40800123, 0x40c00123, 0x00000000, 0x00000000, 0x40a00123, 0x40e00123, 0x00000000,
        0x41000123, 0x41200123, 0x41400123, 0x00000000, 0x41100123, 0x41300123, 0x41500123}},
    {SetValue, 0, 0, 23},
    {SetValue, 0, 24, 47, 8,
        {0x40000123, 0x40800123, 0x40c00123, 0x00000000, 0x00000000, 0x40a00123, 0x40e00123}},
    {SetValue, 0, 48, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x40000123, 0x40800123, 0x40c00123, 0x00000000, 0x00000000, 0x40a00123, 0x40e00123, 0x00000000,
        0x41000123, 0x41200123, 0x41400123, 0x00000000, 0x41100123, 0x41300123, 0x41500123}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, 3, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetVectorArray, 0, 4, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x41600123, 0x41a00123, 0x00000000, 0x00000000, 0x41700123, 0x41b00123, 0x41f00123}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x41a00123, 0x00000000, 0x00000000, 0x00000000, 0x41b00123, 0x41f00123, 0x42300123}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41a00123, 0x41b00123, 0x41c00123, 0x00000000, 0x00000000, 0x41f00123, 0x42000123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x41900123, 0x41d00123, 0x42100123, 0x00000000, 0x41a00123, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 8,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41900123, 0x41a00123, 0x41b00123, 0x00000000, 0x41d00123, 0x00000000, 0x41f00123}},
};

static const struct registerset_constants registerset_constants_column_array_int[] =
{
    {"can",    {"can", D3DXRS_INT4, 0, 12, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 2, 0, 48, NULL},  97},
    {"can[0]", {"can", D3DXRS_INT4, 0,  2, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL},  97},
    {"can[1]", {"can", D3DXRS_INT4, 2,  2, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 105},
};

static const struct registerset_test registerset_test_column_array_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 8,
        {0x00000002, 0x00000004, 0x00000000, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x00000002, 0x00000004, 0x00000000, 0x00000000, 0x00000003, 0x00000005, 0x00000007, 0x00000000,
        0x00000008, 0x0000000a, 0x0000000c, 0x00000000, 0x00000009, 0x0000000b, 0x0000000d}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 8,
        {0x00000001, 0x00000001, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x00000001, 0x00000001, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
        0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 8,
        {0x00000002, 0x00000004, 0x00000006, 0x00000000, 0x00000000, 0x00000005, 0x00000007}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x00000002, 0x00000004, 0x00000006, 0x00000000, 0x00000000, 0x00000005, 0x00000007, 0x00000000,
        0x00000008, 0x0000000a, 0x0000000c, 0x00000000, 0x00000009, 0x0000000b, 0x0000000d}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 8,
        {0x00000002, 0x00000004, 0x00000000, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x00000002, 0x00000004, 0x00000000, 0x00000000, 0x00000003, 0x00000005, 0x00000007, 0x00000000,
        0x00000008, 0x0000000a, 0x0000000c, 0x00000000, 0x00000009, 0x0000000b, 0x0000000d}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, 3, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetVectorArray, 0, 4, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b, 0x00000000,
        0x0000000e, 0x00000014, 0x00000000, 0x00000000, 0x0000000f, 0x00000016, 0x0000001e}},
    {SetMatrix, 0, 0, 0, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b, 0x00000000,
        0x00000014, 0x00000000, 0x00000000, 0x00000000, 0x00000016, 0x0000001e, 0x0000002c}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008, 0x00000000,
        0x00000014, 0x00000016, 0x00000018, 0x00000000, 0x00000000, 0x0000001e, 0x00000020}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 8,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000006, 0x0000000a, 0x00000000, 0x00000000, 0x00000007, 0x0000000b, 0x00000000,
        0x00000012, 0x0000001a, 0x00000024, 0x00000000, 0x00000014, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 8,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000000, 0x00000004, 0x00000000, 0x00000006, 0x00000007, 0x00000008, 0x00000000,
        0x00000012, 0x00000014, 0x00000016, 0x00000000, 0x0000001a, 0x00000000, 0x0000001e}},
};

static const struct registerset_constants registerset_constants_column_array_bool[] =
{
    {"cab",    {"cab", D3DXRS_BOOL, 0, 12, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 2, 0, 48, NULL}, 37},
    {"cab[0]", {"cab", D3DXRS_BOOL, 0,  6, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 37},
    {"cab[1]", {"cab", D3DXRS_BOOL, 6,  6, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 43},
};

static const struct registerset_test registerset_test_column_array_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 12,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 6,
        {0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 12,
        {0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x00000005, 0x00000007, 0x00000008, 0x0000000a,
        0x0000000c, 0x00000009, 0x0000000b, 0x0000000d}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 12,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 6,
        {0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x00000005, 0x00000007}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 12,
        {0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x00000005, 0x00000007, 0x00000008, 0x0000000a,
        0x0000000c, 0x00000009, 0x0000000b, 0x0000000d}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, 3, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetVectorArray, 0, 4, REGISTER_OUTPUT_SIZE / 4, 12,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrix, 0, 0, 0, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
        0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000001, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000001, 0x00000000, 0x00000001}},
};

static const struct registerset_constants registerset_constants_column_array_int_float[] =
{
    {"canf",    {"canf", D3DXRS_FLOAT4,  8, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 2, 0, 48, NULL}, 119},
    {"canf[0]", {"canf", D3DXRS_FLOAT4,  8, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 119},
    {"canf[1]", {"canf", D3DXRS_FLOAT4, 10, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 127},
};

static const struct registerset_test registerset_test_column_array_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 8,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000, 0x00000000,
        0x41000000, 0x41200000, 0x41400000, 0x00000000, 0x41100000, 0x41300000, 0x41500000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 8,
        {0x40000000, 0x40800000, 0x40c00000, 0x00000000, 0x00000000, 0x40a00000, 0x40e00000}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x40000000, 0x40800000, 0x40c00000, 0x00000000, 0x00000000, 0x40a00000, 0x40e00000, 0x00000000,
        0x41000000, 0x41200000, 0x41400000, 0x00000000, 0x41100000, 0x41300000, 0x41500000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 8,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x40000000, 0x40800000, 0x00000000, 0x00000000, 0x40400000, 0x40a00000, 0x40e00000, 0x00000000,
        0x41000000, 0x41200000, 0x41400000, 0x00000000, 0x41100000, 0x41300000, 0x41500000}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},/*16*/
    {SetVectorArray, 0, 2, 3, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetVectorArray, 0, 4, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000, 0x00000000,
        0x41600000, 0x41a00000, 0x00000000, 0x00000000, 0x41700000, 0x41b00000, 0x41f00000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000, 0x00000000,
        0x41a00000, 0x00000000, 0x00000000, 0x00000000, 0x41b00000, 0x41f00000, 0x42300000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000, 0x00000000,
        0x41a00000, 0x41b00000, 0x41c00000, 0x00000000, 0x00000000, 0x41f00000, 0x42000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 8,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000000, 0x40c00000, 0x41200000, 0x00000000, 0x00000000, 0x40e00000, 0x41300000, 0x00000000,
        0x41900000, 0x41d00000, 0x42100000, 0x00000000, 0x41a00000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 8,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000000, 0x00000000, 0x40800000, 0x00000000, 0x40c00000, 0x40e00000, 0x41000000, 0x00000000,
        0x41900000, 0x41a00000, 0x41b00000, 0x00000000, 0x41d00000, 0x00000000, 0x41f00000}},
};

static const struct registerset_constants registerset_constants_column_array_bool_float[] =
{
    {"cabf",    {"cabf", D3DXRS_FLOAT4, 4, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 2, 0, 48, NULL}, 55},
    {"cabf[0]", {"cabf", D3DXRS_FLOAT4, 4, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 55},
    {"cabf[1]", {"cabf", D3DXRS_FLOAT4, 6, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 63},
};

static const struct registerset_test registerset_test_column_array_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 8,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, 3, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetVectorArray, 0, 4, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 8,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 8,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
row_major bool3x2 rb = {1, 1, 0, 0, 1, 1};
row_major int3x2 rn = {80, 81, 82, 83, 84, 85};
row_major float3x2 rf = {95.1, 95.2, 95.3, 95.4, 95.5, 95.6};
row_major bool3x2 rbf = {1, 1, 1, 1, 0, 1};
row_major int3x2 rnf = {37, 13, 98, 54, 77, 36};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (rb._32) for (i = 0; i < rn._31; i++) tmp.x += pos.z * rf._31 * rbf._32;
    else for (i = 0; i < rn._32; i++) tmp.y += pos.y * rf._32 * rnf._32;
    return tmp;
}
#endif
static const DWORD registerset_blob_row[] =
{
0xfffe0300, 0x0076fffe, 0x42415443, 0x0000001c, 0x000001a3, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x0000019c, 0x00000080, 0x00000000, 0x00000006, 0x00000084, 0x00000094, 0x000000ac,
0x00030002, 0x00000003, 0x00000084, 0x000000b0, 0x000000e0, 0x00000002, 0x00000003, 0x000000e4,
0x000000f4, 0x00000124, 0x00000001, 0x00000006, 0x00000128, 0x00000138, 0x00000168, 0x00060002,
0x00000003, 0x00000128, 0x0000016c, 0xab006272, 0x00010002, 0x00020003, 0x00000001, 0x00000000,
0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0x00666272, 0x3f800000,
0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,
0x3f800000, 0x00000000, 0x00000000, 0xab006672, 0x00030002, 0x00020003, 0x00000001, 0x00000000,
0x42be3333, 0x42be6666, 0x00000000, 0x00000000, 0x42be999a, 0x42becccd, 0x00000000, 0x00000000,
0x42bf0000, 0x42bf3333, 0x00000000, 0x00000000, 0xab006e72, 0x00020002, 0x00020003, 0x00000001,
0x00000000, 0x00000050, 0x00000051, 0x00000001, 0x00000000, 0x00000052, 0x00000053, 0x00000001,
0x00000000, 0x00000054, 0x00000055, 0x00000001, 0x00000000, 0x00666e72, 0x42140000, 0x41500000,
0x00000000, 0x00000000, 0x42c40000, 0x42580000, 0x00000000, 0x00000000, 0x429a0000, 0x42100000,
0x00000000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,
0x05000051, 0xa00f0009, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000,
0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x01000028, 0xe0e40805, 0x03000005, 0x80010000,
0xa0000002, 0x90aa0000, 0x02000001, 0x80010001, 0xa0000009, 0x01000026, 0xf0e40004, 0x04000004,
0x80010001, 0x80000000, 0xa0550005, 0x80000001, 0x00000027, 0x02000001, 0x80020001, 0xa0000009,
0x0000002a, 0x03000005, 0x80010000, 0xa0550002, 0x90550000, 0x02000001, 0x80020001, 0xa0000009,
0x01000026, 0xf0e40005, 0x04000004, 0x80020001, 0x80000000, 0xa0550008, 0x80550001, 0x00000027,
0x02000001, 0x80010001, 0xa0000009, 0x0000002b, 0x02000001, 0xe0030000, 0x80e40001, 0x02000001,
0xe00c0000, 0xa0000009, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_row_float[] =
{
    {"rf", {"rf", D3DXRS_FLOAT4, 0, 3, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL}, 61},
};

static const struct registerset_test registerset_test_row_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000, 0x00000000,
        0x40c00123, 0x40e00123}},
    {SetValue, 0, 0, 23},
    {SetValue, 0, 24, REGISTER_OUTPUT_SIZE * 4, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000, 0x00000000,
        0x40c00123, 0x40e00123}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, REGISTER_OUTPUT_SIZE / 4, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetMatrix, 0, 0, 0, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123}},
};

static const struct registerset_constants registerset_constants_row_int[] =
{
    {"rn", {"rn", D3DXRS_INT4, 0, 6, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 78},
};

static const struct registerset_test registerset_test_row_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x00000002, 0x00000003, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000000, 0x00000007, 0x00000001}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
        0x00000000, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000006, 0x00000007, 0x00000001}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 12,
        {0x00000002, 0x00000003, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000000, 0x00000007, 0x00000001}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, REGISTER_OUTPUT_SIZE / 4, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetMatrix, 0, 0, 0, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001}},
};

static const struct registerset_constants registerset_constants_row_bool[] =
{
    {"rb", {"rb", D3DXRS_BOOL, 0, 6, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 37},
};

static const struct registerset_test registerset_test_row_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 0, 0, 5,},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001}},
    {SetBoolArray, 0, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetValue, 0, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, REGISTER_OUTPUT_SIZE / 4, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrix, 0, 0, 0, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001},},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_row_int_float[] =
{
    {"rnf", {"rnf", D3DXRS_FLOAT4, 6, 3, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 91},
};

static const struct registerset_test registerset_test_row_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x40c00000, 0x40e00000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 12,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, REGISTER_OUTPUT_SIZE / 4, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetMatrix, 0, 0, 0, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000}},
};

static const struct registerset_constants registerset_constants_row_bool_float[] =
{
    {"rbf", {"rbf", D3DXRS_FLOAT4, 3, 3, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 44},
};

static const struct registerset_test registerset_test_row_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, REGISTER_OUTPUT_SIZE / 4, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrix, 0, 0, 0, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
row_major bool3x2 rab[2] = {1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1};
row_major int3x2 ran[2] = {4, 5, 6, 1, 8, 1, 5, 3, 9, 6, 7, 3};
row_major float3x2 raf[2] = {1.5, 2.8, 3.3, 4.9, 5.9, 6.8, 7.9, 8.5, 9.4, 1.3, 1.2, 1.1};
row_major bool3x2 rabf[2] = {1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1};
row_major int3x2 ranf[2] = {35, 40, 60, 80, 70, 56, 37, 13, 98, 54, 77, 36};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (rab[1]._32) for (i = 0; i < ran[1]._31; i++) tmp.x += pos.z * raf[1]._31 * rabf[1]._32;
    else for (i = 0; i < ran[1]._32; i++) tmp.y += pos.y * raf[1]._32 * ranf[1]._32;
    return tmp;
}
#endif
static const DWORD registerset_blob_row_array[] =
{
0xfffe0300, 0x00b6fffe, 0x42415443, 0x0000001c, 0x000002a3, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x0000029c, 0x00000080, 0x00000000, 0x0000000c, 0x00000084, 0x00000094, 0x000000c4,
0x00060002, 0x00000006, 0x000000cc, 0x000000dc, 0x0000013c, 0x00000002, 0x00000006, 0x00000140,
0x00000150, 0x000001b0, 0x00000001, 0x0000000c, 0x000001b4, 0x000001c4, 0x00000224, 0x000c0002,
0x00000006, 0x0000022c, 0x0000023c, 0x00626172, 0x00010002, 0x00020003, 0x00000002, 0x00000000,
0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0x66626172, 0xababab00, 0x00010002, 0x00020003,
0x00000002, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000,
0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
0x00000000, 0x00000000, 0x00666172, 0x00030002, 0x00020003, 0x00000002, 0x00000000, 0x3fc00000,
0x40333333, 0x00000000, 0x00000000, 0x40533333, 0x409ccccd, 0x00000000, 0x00000000, 0x40bccccd,
0x40d9999a, 0x00000000, 0x00000000, 0x40fccccd, 0x41080000, 0x00000000, 0x00000000, 0x41166666,
0x3fa66666, 0x00000000, 0x00000000, 0x3f99999a, 0x3f8ccccd, 0x00000000, 0x00000000, 0x006e6172,
0x00020002, 0x00020003, 0x00000002, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
0x00000006, 0x00000001, 0x00000001, 0x00000000, 0x00000008, 0x00000001, 0x00000001, 0x00000000,
0x00000005, 0x00000003, 0x00000001, 0x00000000, 0x00000009, 0x00000006, 0x00000001, 0x00000000,
0x00000007, 0x00000003, 0x00000001, 0x00000000, 0x666e6172, 0xababab00, 0x00020002, 0x00020003,
0x00000002, 0x00000000, 0x420c0000, 0x42200000, 0x00000000, 0x00000000, 0x42700000, 0x42a00000,
0x00000000, 0x00000000, 0x428c0000, 0x42600000, 0x00000000, 0x00000000, 0x42140000, 0x41500000,
0x00000000, 0x00000000, 0x42c40000, 0x42580000, 0x00000000, 0x00000000, 0x429a0000, 0x42100000,
0x00000000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,
0x05000051, 0xa00f0012, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000,
0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x01000028, 0xe0e4080b, 0x03000005, 0x80010000,
0xa0000005, 0x90aa0000, 0x02000001, 0x80010001, 0xa0000012, 0x01000026, 0xf0e4000a, 0x04000004,
0x80010001, 0x80000000, 0xa055000b, 0x80000001, 0x00000027, 0x02000001, 0x80020001, 0xa0000012,
0x0000002a, 0x03000005, 0x80010000, 0xa0550005, 0x90550000, 0x02000001, 0x80020001, 0xa0000012,
0x01000026, 0xf0e4000b, 0x04000004, 0x80020001, 0x80000000, 0xa0550011, 0x80550001, 0x00000027,
0x02000001, 0x80010001, 0xa0000012, 0x0000002b, 0x02000001, 0xe0030000, 0x80e40001, 0x02000001,
0xe00c0000, 0xa0000012, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_row_array_float[] =
{
    {"raf",    {"raf", D3DXRS_FLOAT4, 0, 6, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 2, 2, 0, 48, NULL}, 84},
    {"raf[0]", {"raf", D3DXRS_FLOAT4, 0, 3, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL}, 84},
    {"raf[1]", {"raf", D3DXRS_FLOAT4, 3, 3, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 2, 1, 0, 24, NULL}, 96},
};

static const struct registerset_test registerset_test_row_array_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 12,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000, 0x00000000, 0x00000000, 0x41000000, 0x41100000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000, 0x00000000, 0x00000000, 0x41400000, 0x41500000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000, 0x00000000,
        0x40c00123, 0x40e00123}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000, 0x00000000,
        0x40c00123, 0x40e00123, 0x00000000, 0x00000000, 0x41000123, 0x41100123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123, 0x00000000, 0x00000000, 0x41400123, 0x41500123}},
    {SetValue, 0, 0, 23},
    {SetValue, 0, 24, 47, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000, 0x00000000,
        0x40c00123, 0x40e00123}},
    {SetValue, 0, 48, REGISTER_OUTPUT_SIZE * 4, 24,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000, 0x00000000,
        0x40c00123, 0x40e00123, 0x00000000, 0x00000000, 0x41000123, 0x41100123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123, 0x00000000, 0x00000000, 0x41400123, 0x41500123}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, 5, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetVectorArray, 0, 6, REGISTER_OUTPUT_SIZE / 4, 24,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123, 0x00000000, 0x00000000, 0x41600123, 0x41700123, 0x00000000, 0x00000000,
        0x41a00123, 0x41b00123, 0x00000000, 0x00000000, 0x00000000, 0x41f00123}},
    {SetMatrix, 0, 0, 0, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123, 0x00000000, 0x00000000, 0x41a00123, 0x41b00123, 0x00000000, 0x00000000,
        0x00000000, 0x41f00123, 0x00000000, 0x00000000, 0x00000000, 0x42300123}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 12,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123, 0x00000000, 0x00000000, 0x41a00123, 0x00000000, 0x00000000, 0x00000000,
        0x41b00123, 0x41f00123, 0x00000000, 0x00000000, 0x41c00123, 0x42000123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 12,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x40c00123, 0x40e00123, 0x00000000, 0x00000000,
        0x41200123, 0x41300123, 0x00000000, 0x00000000, 0x41900123, 0x41a00123, 0x00000000, 0x00000000,
        0x41d00123, 0x00000000, 0x00000000, 0x00000000, 0x42100123, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 12,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000123, 0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x00000000, 0x00000000,
        0x40800123, 0x41000123, 0x00000000, 0x00000000, 0x41900123, 0x41d00123, 0x00000000, 0x00000000,
        0x41a00123, 0x00000000, 0x00000000, 0x00000000, 0x41b00123, 0x41f00123}},
};

static const struct registerset_constants registerset_constants_row_array_int[] =
{
    {"ran",    {"ran", D3DXRS_INT4, 0, 12, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 2, 0, 48, NULL}, 113},
    {"ran[0]", {"ran", D3DXRS_INT4, 0,  3, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 113},
    {"ran[1]", {"ran", D3DXRS_INT4, 3,  3, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 125},
};

static const struct registerset_test registerset_test_row_array_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 12,
        {0x00000002, 0x00000003, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000000, 0x00000007, 0x00000001}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x00000002, 0x00000003, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000000, 0x00000007, 0x00000001, 0x00000000, 0x00000008, 0x00000009, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001, 0x00000000, 0x0000000c, 0x0000000d, 0x00000001}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 12,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
        0x00000000, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
        0x00000000, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
        0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000006, 0x00000007, 0x00000001}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000006, 0x00000007, 0x00000001, 0x00000000, 0x00000008, 0x00000009, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001, 0x00000000, 0x0000000c, 0x0000000d, 0x00000001}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 12,
        {0x00000002, 0x00000003, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000000, 0x00000007, 0x00000001}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 24,
        {0x00000002, 0x00000003, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
        0x00000000, 0x00000007, 0x00000001, 0x00000000, 0x00000008, 0x00000009, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001, 0x00000000, 0x0000000c, 0x0000000d, 0x00000001}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, 5, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetVectorArray, 0, 6, REGISTER_OUTPUT_SIZE / 4, 24,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001, 0x00000000, 0x0000000e, 0x0000000f, 0x00000001, 0x00000000,
        0x00000014, 0x00000016, 0x00000001, 0x00000000, 0x00000000, 0x0000001e, 0x00000001}},
    {SetMatrix, 0, 0, 0, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001, 0x00000000, 0x00000014, 0x00000016, 0x00000001, 0x00000000,
        0x00000000, 0x0000001e, 0x00000001, 0x00000000, 0x00000000, 0x0000002c, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 12,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001, 0x00000000, 0x00000014, 0x00000000, 0x00000001, 0x00000000,
        0x00000016, 0x0000001e, 0x00000001, 0x00000000, 0x00000018, 0x00000020, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 12,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000006, 0x00000007, 0x00000001, 0x00000000,
        0x0000000a, 0x0000000b, 0x00000001, 0x00000000, 0x00000012, 0x00000014, 0x00000001, 0x00000000,
        0x0000001a, 0x00000000, 0x00000001, 0x00000000, 0x00000024, 0x00000000, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 12,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x00000002, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000007, 0x00000001, 0x00000000,
        0x00000004, 0x00000008, 0x00000001, 0x00000000, 0x00000012, 0x0000001a, 0x00000001, 0x00000000,
        0x00000014, 0x00000000, 0x00000001, 0x00000000, 0x00000016, 0x0000001e, 0x00000001}},
};

static const struct registerset_constants registerset_constants_row_array_bool[] =
{
    {"rab",    {"rab", D3DXRS_BOOL, 0, 12, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 2, 0, 48, NULL}, 37},
    {"rab[0]", {"rab", D3DXRS_BOOL, 0,  6, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 37},
    {"rab[1]", {"rab", D3DXRS_BOOL, 6,  6, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 43},
};

static const struct registerset_test registerset_test_row_array_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5,},
    {SetIntArray, 1, 6, 11, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 12,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 12,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007, 0x00000008, 0x00000009,
        0x0000000a, 0x0000000b, 0x0000000c, 0x0000000d}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 12,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 12,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007, 0x00000008, 0x00000009,
        0x0000000a, 0x0000000b, 0x0000000c, 0x0000000d}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, 5, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetVectorArray, 0, 6, REGISTER_OUTPUT_SIZE / 4, 12,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000001, 0x00000000, 0x00000001}},
    {SetMatrix, 0, 0, 0, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000000, 0x00000001, 0x00000000, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
        0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000000, 0x00000001, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 6,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 12,
        {0x00000001, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x00000001, 0x00000000, 0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_row_array_int_float[] =
{
    {"ranf",    {"ranf", D3DXRS_FLOAT4, 12, 6, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 2, 0, 48, NULL}, 143},
    {"ranf[0]", {"ranf", D3DXRS_FLOAT4, 12, 3, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 143},
    {"ranf[1]", {"ranf", D3DXRS_FLOAT4, 15, 3, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3, 2, 1, 0, 24, NULL}, 155},
};

static const struct registerset_test registerset_test_row_array_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 12,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000,}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000, 0x00000000, 0x00000000, 0x41000000, 0x41100000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000, 0x00000000, 0x00000000, 0x41400000, 0x41500000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x40c00000, 0x40e00000}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x40c00000, 0x40e00000, 0x00000000, 0x00000000, 0x41000000, 0x41100000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000, 0x00000000, 0x00000000, 0x41400000, 0x41500000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 12,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 24,
        {0x40000000, 0x40400000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000, 0x00000000,
        0x00000000, 0x40e00000, 0x00000000, 0x00000000, 0x41000000, 0x41100000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000, 0x00000000, 0x00000000, 0x41400000, 0x41500000}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, 5, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetVectorArray, 0, 6, REGISTER_OUTPUT_SIZE / 4, 24,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000, 0x00000000, 0x00000000, 0x41600000, 0x41700000, 0x00000000, 0x00000000,
        0x41a00000, 0x41b00000, 0x00000000, 0x00000000, 0x00000000, 0x41f00000}},
    {SetMatrix, 0, 0, 0, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000, 0x00000000, 0x00000000, 0x41a00000, 0x41b00000, 0x00000000, 0x00000000,
        0x00000000, 0x41f00000, 0x00000000, 0x00000000, 0x00000000, 0x42300000}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 12,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000, 0x00000000, 0x00000000, 0x41a00000, 0x00000000, 0x00000000, 0x00000000,
        0x41b00000, 0x41f00000, 0x00000000, 0x00000000, 0x41c00000, 0x42000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 12,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40c00000, 0x40e00000, 0x00000000, 0x00000000,
        0x41200000, 0x41300000, 0x00000000, 0x00000000, 0x41900000, 0x41a00000, 0x00000000, 0x00000000,
        0x41d00000, 0x00000000, 0x00000000, 0x00000000, 0x42100000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 12,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x40000000, 0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x00000000, 0x00000000,
        0x40800000, 0x41000000, 0x00000000, 0x00000000, 0x41900000, 0x41d00000, 0x00000000, 0x00000000,
        0x41a00000, 0x00000000, 0x00000000, 0x00000000, 0x41b00000, 0x41f00000}},
};

static const struct registerset_constants registerset_constants_row_array_bool_float[] =
{
    {"rabf",    {"rabf", D3DXRS_FLOAT4, 6, 6, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 2, 0, 48, NULL}, 55},
    {"rabf[0]", {"rabf", D3DXRS_FLOAT4, 6, 3, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 55},
    {"rabf[1]", {"rabf", D3DXRS_FLOAT4, 9, 3, D3DXPC_MATRIX_ROWS, D3DXPT_BOOL, 3, 2, 1, 0, 24, NULL}, 67},
};

static const struct registerset_test registerset_test_row_array_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, 11, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetIntArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, 11, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetBoolArray, 1, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, 11, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetFloatArray, 0, 12, REGISTER_OUTPUT_SIZE, 24,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, 47, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000}},
    {SetValue, 1, 48, REGISTER_OUTPUT_SIZE * 4, 24,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetVector},
    {SetVectorArray, 0, 0, 2},
    {SetVectorArray, 0, 3, 5, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetVectorArray, 0, 6, REGISTER_OUTPUT_SIZE / 4, 24,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetMatrix, 0, 0, 0, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixTransposeArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 12,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixPointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 12,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000}},
    {SetMatrixTransposePointerArray, 0, 2, REGISTER_OUTPUT_SIZE / 16, 24,
        {0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
struct {bool b; bool3 vb;} sb = {1, 1, 0, 1};
struct {int n; int3 vn;} sn = {11, 12, 13, 14};
struct {float f; float3 vf;} sf = {1.1f, 2.2f, 3.3f, 4.4f};
struct {int nf; int3 vnf;} snf = {31, 32, 33, 34};
struct {bool bf; bool3 vbf;} sbf = {1, 0, 0, 1};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (sb.vb.z) for (i = 0; i < sn.n; i++) tmp.x += pos.z * sf.vf.x * snf.vnf.z;
    else for (i = 0; i < sn.vn.z; i++) tmp.y += pos.y * sf.vf.z * sbf.vbf.y;
    return tmp;
}
#endif
static const DWORD registerset_blob_struct[] =
{
0xfffe0300, 0x00a2fffe, 0x42415443, 0x0000001c, 0x00000253, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x0000024c, 0x00000080, 0x00000000, 0x00000004, 0x000000bc, 0x000000cc, 0x000000dc,
0x00040002, 0x00000002, 0x000000f8, 0x00000108, 0x00000128, 0x00000002, 0x00000002, 0x00000164,
0x00000174, 0x00000194, 0x00000001, 0x00000004, 0x000001d0, 0x000001e0, 0x00000200, 0x00020002,
0x00000002, 0x0000021c, 0x0000022c, 0x62006273, 0xababab00, 0x00010000, 0x00010001, 0x00000001,
0x00000000, 0xab006276, 0x00010001, 0x00030001, 0x00000001, 0x00000000, 0x00000083, 0x00000088,
0x00000098, 0x0000009c, 0x00000005, 0x00040001, 0x00020001, 0x000000ac, 0xffffffff, 0xffffffff,
0x00000000, 0xffffffff, 0x00666273, 0x76006662, 0xab006662, 0x000000e0, 0x00000088, 0x000000e3,
0x0000009c, 0x00000005, 0x00040001, 0x00020001, 0x000000e8, 0x3f800000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x66006673, 0xababab00, 0x00030000,
0x00010001, 0x00000001, 0x00000000, 0xab006676, 0x00030001, 0x00030001, 0x00000001, 0x00000000,
0x0000012b, 0x00000130, 0x00000140, 0x00000144, 0x00000005, 0x00040001, 0x00020001, 0x00000154,
0x3f8ccccd, 0x00000000, 0x00000000, 0x00000000, 0x400ccccd, 0x40533333, 0x408ccccd, 0x00000000,
0x6e006e73, 0xababab00, 0x00020000, 0x00010001, 0x00000001, 0x00000000, 0xab006e76, 0x00020001,
0x00030001, 0x00000001, 0x00000000, 0x00000197, 0x0000019c, 0x000001ac, 0x000001b0, 0x00000005,
0x00040001, 0x00020001, 0x000001c0, 0x0000000b, 0x00000000, 0x00000001, 0x00000000, 0x0000000c,
0x0000000d, 0x0000000e, 0x00000000, 0x00666e73, 0x7600666e, 0xab00666e, 0x00000204, 0x0000019c,
0x00000207, 0x000001b0, 0x00000005, 0x00040001, 0x00020001, 0x0000020c, 0x41f80000, 0x00000000,
0x00000000, 0x00000000, 0x42000000, 0x42040000, 0x42080000, 0x00000000, 0x335f7376, 0x4d00305f,
0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,
0x2e392072, 0x392e3932, 0x332e3235, 0x00313131, 0x05000051, 0xa00f0006, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000,
0x01000028, 0xe0e40803, 0x03000005, 0x80010000, 0xa0000001, 0x90aa0000, 0x02000001, 0x80010001,
0xa0000006, 0x01000026, 0xf0e40000, 0x04000004, 0x80010001, 0x80000000, 0xa0aa0003, 0x80000001,
0x00000027, 0x02000001, 0x80020001, 0xa0000006, 0x0000002a, 0x03000005, 0x80010000, 0xa0aa0001,
0x90550000, 0x02000001, 0x80020001, 0xa0000006, 0x01000026, 0xf0e40003, 0x04000004, 0x80020001,
0x80000000, 0xa0550005, 0x80550001, 0x00000027, 0x02000001, 0x80010001, 0xa0000006, 0x0000002b,
0x02000001, 0xe0030000, 0x80e40001, 0x02000001, 0xe00c0000, 0xa0000006, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_struct_float[] =
{
    {"sf",    {"sf", D3DXRS_FLOAT4, 0, 2, D3DXPC_STRUCT, D3DXPT_VOID,  1, 4, 1, 2, 16, NULL}, 93},
    {"sf.f",  {"f",  D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0,  4, NULL}, 93},
    {"sf.vf", {"vf", D3DXRS_FLOAT4, 1, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 1, 0, 12, NULL}, 97},
};

static const struct registerset_test registerset_test_struct_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetValue, 0, 0, 15},
    {SetValue, 0, 16, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
};

static const struct registerset_constants registerset_constants_struct_int[] =
{
    {"sn",    {"sn", D3DXRS_INT4, 0, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 120},
    {"sn.n",  {"n",  D3DXRS_INT4, 0, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 120},
    {"sn.vn", {"vn", D3DXRS_INT4, 1, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 124},
};

static const struct registerset_test registerset_test_struct_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000004, 0x00000005, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000004, 0x00000005, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
};

static const struct registerset_constants registerset_constants_struct_bool[] =
{
    {"sb",    {"sb", D3DXRS_BOOL, 0, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 51},
    {"sb.b",  {"b",  D3DXRS_BOOL, 0, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 51},
    {"sb.vb", {"vb", D3DXRS_BOOL, 1, 3, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 52},
};

static const struct registerset_test registerset_test_struct_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, REGISTER_OUTPUT_SIZE, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, REGISTER_OUTPUT_SIZE, 4,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, REGISTER_OUTPUT_SIZE, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, REGISTER_OUTPUT_SIZE * 4, 4,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005}},
    {SetVector, 0, 0, 0, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrix, 0, 0, 0, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_struct_int_float[] =
{
    {"snf",     {"snf", D3DXRS_FLOAT4, 2, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 139},
    {"snf.nf",  {"nf",  D3DXRS_FLOAT4, 2, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 139},
    {"snf.vnf", {"vnf", D3DXRS_FLOAT4, 3, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 143},
};

static const struct registerset_test registerset_test_struct_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
};

static const struct registerset_constants registerset_constants_struct_bool_float[] =
{
    {"sbf",     {"sbf", D3DXRS_FLOAT4, 4, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 66},
    {"sbf.bf",  {"bf",  D3DXRS_FLOAT4, 4, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 66},
    {"sbf.vbf", {"vbf", D3DXRS_FLOAT4, 5, 1, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 70},
};

static const struct registerset_test registerset_test_struct_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
struct {bool b; bool3 vb;} sab[2] = {1, 1, 0, 1, 0, 1, 0, 1};
struct {int n; int3 vn;} san[2] = {21, 22, 23, 24, 25, 26, 27, 28};
struct {float f; float3 vf;} saf[2] = {1.1f, 2.1f, 3.1f, 4.1f, 5.1f, 6.1f, 7.1f, 8.1f};
struct {int nf; int3 vnf;} sanf[2] = {41, 0, 43, 44, 41, 42, 43, 44};
struct {bool bf; bool3 vbf;} sabf[2] = {1, 0, 0, 1, 1, 1, 0, 1};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (sab[1].vb.z) for (i = 0; i < san[1].n; i++) tmp.x += pos.z * saf[1].vf.x * sanf[1].vnf.z;
    else for (i = 0; i < san[1].vn.z; i++) tmp.y += pos.y * saf[1].vf.z * sabf[1].vbf.y;
    return tmp;
}
#endif
static const DWORD registerset_blob_struct_array[] =
{
0xfffe0300, 0x00c6fffe, 0x42415443, 0x0000001c, 0x000002e3, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x000002dc, 0x00000080, 0x00000000, 0x00000008, 0x000000bc, 0x000000cc, 0x000000ec,
0x00080002, 0x00000004, 0x00000108, 0x00000118, 0x00000158, 0x00000002, 0x00000004, 0x00000194,
0x000001a4, 0x000001e4, 0x00000001, 0x00000008, 0x00000220, 0x00000230, 0x00000270, 0x00040002,
0x00000004, 0x0000028c, 0x0000029c, 0x00626173, 0xabab0062, 0x00010000, 0x00010001, 0x00000001,
0x00000000, 0xab006276, 0x00010001, 0x00030001, 0x00000001, 0x00000000, 0x00000084, 0x00000088,
0x00000098, 0x0000009c, 0x00000005, 0x00040001, 0x00020002, 0x000000ac, 0xffffffff, 0xffffffff,
0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0x66626173, 0x00666200,
0x00666276, 0x000000f1, 0x00000088, 0x000000f4, 0x0000009c, 0x00000005, 0x00040001, 0x00020002,
0x000000f8, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000,
0x00000000, 0x00666173, 0xabab0066, 0x00030000, 0x00010001, 0x00000001, 0x00000000, 0xab006676,
0x00030001, 0x00030001, 0x00000001, 0x00000000, 0x0000015c, 0x00000160, 0x00000170, 0x00000174,
0x00000005, 0x00040001, 0x00020002, 0x00000184, 0x3f8ccccd, 0x00000000, 0x00000000, 0x00000000,
0x40066666, 0x40466666, 0x40833333, 0x00000000, 0x40a33333, 0x00000000, 0x00000000, 0x00000000,
0x40c33333, 0x40e33333, 0x4101999a, 0x00000000, 0x006e6173, 0xabab006e, 0x00020000, 0x00010001,
0x00000001, 0x00000000, 0xab006e76, 0x00020001, 0x00030001, 0x00000001, 0x00000000, 0x000001e8,
0x000001ec, 0x000001fc, 0x00000200, 0x00000005, 0x00040001, 0x00020002, 0x00000210, 0x00000015,
0x00000000, 0x00000001, 0x00000000, 0x00000016, 0x00000017, 0x00000018, 0x00000000, 0x00000019,
0x00000000, 0x00000001, 0x00000000, 0x0000001a, 0x0000001b, 0x0000001c, 0x00000000, 0x666e6173,
0x00666e00, 0x00666e76, 0x00000275, 0x000001ec, 0x00000278, 0x00000200, 0x00000005, 0x00040001,
0x00020002, 0x0000027c, 0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x422c0000,
0x42300000, 0x00000000, 0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x42280000, 0x422c0000,
0x42300000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,
0x05000051, 0xa00f000c, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000,
0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x01000028, 0xe0e40807, 0x03000005, 0x80010000,
0xa0000003, 0x90aa0000, 0x02000001, 0x80010001, 0xa000000c, 0x01000026, 0xf0e40004, 0x04000004,
0x80010001, 0x80000000, 0xa0aa0007, 0x80000001, 0x00000027, 0x02000001, 0x80020001, 0xa000000c,
0x0000002a, 0x03000005, 0x80010000, 0xa0aa0003, 0x90550000, 0x02000001, 0x80020001, 0xa000000c,
0x01000026, 0xf0e40007, 0x04000004, 0x80020001, 0x80000000, 0xa055000b, 0x80550001, 0x00000027,
0x02000001, 0x80010001, 0xa000000c, 0x0000002b, 0x02000001, 0xe0030000, 0x80e40001, 0x02000001,
0xe00c0000, 0xa000000c, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_struct_array_float[] =
{
    {"saf",       {"saf", D3DXRS_FLOAT4, 0, 4, D3DXPC_STRUCT, D3DXPT_VOID,  1, 4, 2, 2, 32, NULL}, 105},
    {"saf[0]",    {"saf", D3DXRS_FLOAT4, 0, 2, D3DXPC_STRUCT, D3DXPT_VOID,  1, 4, 1, 2, 16, NULL}, 105},
    {"saf[0].f",  {"f",   D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0,  4, NULL}, 105},
    {"saf[0].vf", {"vf",  D3DXRS_FLOAT4, 1, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 1, 0, 12, NULL}, 109},
    {"saf[1]",    {"saf", D3DXRS_FLOAT4, 2, 2, D3DXPC_STRUCT, D3DXPT_VOID,  1, 4, 1, 2, 16, NULL}, 113},
    {"saf[1].f",  {"f",   D3DXRS_FLOAT4, 2, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0,  4, NULL}, 113},
    {"saf[1].vf", {"vf",  D3DXRS_FLOAT4, 3, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 1, 0, 12, NULL}, 117},
};

static const struct registerset_test registerset_test_struct_array_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, 7, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetIntArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x41000000, 0x41100000, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, 7, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetBoolArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, 7, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetFloatArray, 0, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x41000123, 0x41100123, 0x00000000}},
    {SetValue, 0, 0, 15},
    {SetValue, 0, 16, 31, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetValue, 0, 32, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x41000123, 0x41100123, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 8,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800123, 0x40a00123, 0x00000000}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x41000123, 0x41100123, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x41000123, 0x41100123, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40c00123, 0x00000000, 0x00000000, 0x00000000, 0x40e00123, 0x41000123, 0x41100123, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8},
};

static const struct registerset_constants registerset_constants_struct_array_int[] =
{
    {"san",       {"san", D3DXRS_INT4, 0, 8, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 2, 2, 32, NULL}, 140},
    {"san[0]",    {"san", D3DXRS_INT4, 0, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 140},
    {"san[0].n",  {"n",   D3DXRS_INT4, 0, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 140},
    {"san[0].vn", {"vn",  D3DXRS_INT4, 1, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 144},
    {"san[1]",    {"san", D3DXRS_INT4, 2, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 148},
    {"san[1].n",  {"n",   D3DXRS_INT4, 2, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 148},
    {"san[1].vn", {"vn",  D3DXRS_INT4, 3, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 152},
};

static const struct registerset_test registerset_test_struct_array_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, 7, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000004, 0x00000005, 0x00000000}},
    {SetIntArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000009, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, 7, 8,
        {0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000}},
    {SetBoolArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, 7, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetFloatArray, 0, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000006, 0x00000000, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000009, 0x00000000}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, 31, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000004, 0x00000005, 0x00000000}},
    {SetValue, 1, 32, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000009, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 8,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000000}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x00000006, 0x00000000, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000009, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x00000006, 0x00000000, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000009, 0x00000000},},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000006, 0x00000000, 0x00000001, 0x00000000, 0x00000007, 0x00000008, 0x00000009, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
};

static const struct registerset_constants registerset_constants_struct_array_bool[] =
{
    {"sab",       {"sab", D3DXRS_BOOL, 0, 8, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 2, 2, 32, NULL}, 51},
    {"sab[0]",    {"sab", D3DXRS_BOOL, 0, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 51},
    {"sab[0].b",  {"b",   D3DXRS_BOOL, 0, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 51},
    {"sab[0].vb", {"vb",  D3DXRS_BOOL, 1, 3, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 52},
    {"sab[1]",    {"sab", D3DXRS_BOOL, 4, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 55},
    {"sab[1].b",  {"b",   D3DXRS_BOOL, 4, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 55},
    {"sab[1].vb", {"vb",  D3DXRS_BOOL, 5, 3, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 56},
};

static const struct registerset_test registerset_test_struct_array_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, 7, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetIntArray, 1, 8, REGISTER_OUTPUT_SIZE, 4,
        {0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, 7, 4,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005}},
    {SetBoolArray, 1, 8, REGISTER_OUTPUT_SIZE, 4,
        {0x00000000, 0x00000007, 0x00000008, 0x00000009}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, 7, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetFloatArray, 0, 8, REGISTER_OUTPUT_SIZE, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, 31, 4,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005}},
    {SetValue, 1, 32, REGISTER_OUTPUT_SIZE * 4, 4,
        {0x00000000, 0x00000007, 0x00000008, 0x00000009}},
    {SetVector, 0, 0, 0, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 4,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrix, 0, 0, 0, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 4,
        {0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000000, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 4,
        {0x00000000, 0x00000001, 0x00000001, 0x00000001}},
};

static const struct registerset_constants registerset_constants_struct_array_int_float[] =
{
    {"sanf",        {"sanf", D3DXRS_FLOAT4, 4, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 2, 2, 32, NULL}, 167},
    {"sanf[0]",     {"sanf", D3DXRS_FLOAT4, 4, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 167},
    {"sanf[0].nf",  {"nf",   D3DXRS_FLOAT4, 4, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 167},
    {"sanf[0].vnf", {"vnf",  D3DXRS_FLOAT4, 5, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 171},
    {"sanf[1]",     {"sanf", D3DXRS_FLOAT4, 6, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 175},
    {"sanf[1].nf",  {"nf",   D3DXRS_FLOAT4, 6, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 175},
    {"sanf[1].vnf", {"vnf",  D3DXRS_FLOAT4, 7, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 179},
};

static const struct registerset_test registerset_test_struct_array_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, 7, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetIntArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x41000000, 0x41100000, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, 7, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetBoolArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, 7, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetFloatArray, 0, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x41000000, 0x41100000, 0x00000000}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, 31, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetValue, 1, 32, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x41000000, 0x41100000, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 8,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40800000, 0x40a00000, 0x00000000}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x41000000, 0x41100000, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x41000000, 0x41100000, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x40c00000, 0x00000000, 0x00000000, 0x00000000, 0x40e00000, 0x41000000, 0x41100000, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8},
};

static const struct registerset_constants registerset_constants_struct_array_bool_float[] =
{
    {"sabf",        {"sabf", D3DXRS_FLOAT4,  8, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 2, 2, 32, NULL}, 70},
    {"sabf[0]",     {"sabf", D3DXRS_FLOAT4,  8, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 70},
    {"sabf[0].bf",  {"bf",   D3DXRS_FLOAT4,  8, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 70},
    {"sabf[0].vbf", {"vbf",  D3DXRS_FLOAT4,  9, 1, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 74},
    {"sabf[1]",     {"sabf", D3DXRS_FLOAT4, 10, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 78},
    {"sabf[1].bf",  {"bf",   D3DXRS_FLOAT4, 10, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 78},
    {"sabf[1].vbf", {"vbf",  D3DXRS_FLOAT4, 11, 1, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 82},
};

static const struct registerset_test registerset_test_struct_array_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 3},
    {SetIntArray, 1, 4, 7, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetIntArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetBoolArray, 1, 0, 3},
    {SetBoolArray, 1, 4, 7, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetBoolArray, 1, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetFloatArray, 0, 0, 3},
    {SetFloatArray, 0, 4, 7, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetFloatArray, 0, 8, REGISTER_OUTPUT_SIZE, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetValue, 1, 0, 15},
    {SetValue, 1, 16, 31, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetValue, 1, 32, REGISTER_OUTPUT_SIZE * 4, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetVector, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetVectorArray},
    {SetVectorArray, 0, 1, 1, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetMatrix, 0, 0, 0, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000}},
    {SetMatrixTranspose, 0, 0, 0, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8,
        {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 8},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
struct {bool b; struct {bool b; bool3 vb;} s; bool b1;} ssb = {1, 1, 0, 1, 1, 0};
struct {int n; struct {int n; int3 vn;} s; int n1;} ssn = {71, 72, 73, 74, 75, 76};
struct {float f; struct {float f; float3 vf;} s; float f1;} ssf = {1.1f, 2.1f, 3.1f, 4.1f, 5.1f, 6.1f};
struct {int nf; struct {int nf; int3 vnf;} s; int nf1;} ssnf = {41, 0, 43, 44, 41, 42};
struct {bool bf; struct {bool bf; bool3 vbf;} s; bool bf1;} ssbf = {1, 0, 0, 1, 1, 0};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int i;
    if (ssb.b1) for (i = 0; i < ssn.n1; i++) tmp.x += pos.z * ssf.f1 * ssnf.nf1;
    else for (i = 0; i < ssn.s.vn.z; i++) tmp.y += pos.y * ssf.s.vf.z * ssbf.bf1;
    return tmp;
}
#endif
static const DWORD registerset_blob_struct_struct[] =
{
0xfffe0300, 0x00fcfffe, 0x42415443, 0x0000001c, 0x000003bb, 0xfffe0300, 0x00000005, 0x0000001c,
0x00000100, 0x000003b4, 0x00000080, 0x00000000, 0x00000006, 0x000000ec, 0x000000fc, 0x00000114,
0x00080002, 0x00000004, 0x0000015c, 0x0000016c, 0x000001ac, 0x00000002, 0x00000004, 0x00000214,
0x00000224, 0x00000264, 0x00000001, 0x00000006, 0x000002cc, 0x000002dc, 0x0000031c, 0x00040002,
0x00000004, 0x00000364, 0x00000374, 0x00627373, 0xabab0062, 0x00010000, 0x00010001, 0x00000001,
0x00000000, 0x62760073, 0xababab00, 0x00010001, 0x00030001, 0x00000001, 0x00000000, 0x00000084,
0x00000088, 0x0000009a, 0x000000a0, 0x00000005, 0x00040001, 0x00020001, 0x000000b0, 0xab003162,
0x00000084, 0x00000088, 0x00000098, 0x000000c0, 0x000000d0, 0x00000088, 0x00000005, 0x00060001,
0x00030001, 0x000000d4, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000,
0x66627373, 0x00666200, 0x00666276, 0x00000119, 0x00000088, 0x0000011c, 0x000000a0, 0x00000005,
0x00040001, 0x00020001, 0x00000120, 0x00316662, 0x00000119, 0x00000088, 0x00000098, 0x00000130,
0x00000140, 0x00000088, 0x00000005, 0x00060001, 0x00030001, 0x00000144, 0x3f800000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00667373, 0xabab0066,
0x00030000, 0x00010001, 0x00000001, 0x00000000, 0xab006676, 0x00030001, 0x00030001, 0x00000001,
0x00000000, 0x000001b0, 0x000001b4, 0x000001c4, 0x000001c8, 0x00000005, 0x00040001, 0x00020001,
0x000001d8, 0xab003166, 0x000001b0, 0x000001b4, 0x00000098, 0x000001e8, 0x000001f8, 0x000001b4,
0x00000005, 0x00060001, 0x00030001, 0x000001fc, 0x3f8ccccd, 0x00000000, 0x00000000, 0x00000000,
0x40066666, 0x00000000, 0x00000000, 0x00000000, 0x40466666, 0x40833333, 0x40a33333, 0x00000000,
0x40c33333, 0x00000000, 0x00000000, 0x00000000, 0x006e7373, 0xabab006e, 0x00020000, 0x00010001,
0x00000001, 0x00000000, 0xab006e76, 0x00020001, 0x00030001, 0x00000001, 0x00000000, 0x00000268,
0x0000026c, 0x0000027c, 0x00000280, 0x00000005, 0x00040001, 0x00020001, 0x00000290, 0xab00316e,
0x00000268, 0x0000026c, 0x00000098, 0x000002a0, 0x000002b0, 0x0000026c, 0x00000005, 0x00060001,
0x00030001, 0x000002b4, 0x00000047, 0x00000000, 0x00000001, 0x00000000, 0x00000048, 0x00000000,
0x00000001, 0x00000000, 0x00000049, 0x0000004a, 0x0000004b, 0x00000000, 0x0000004c, 0x00000000,
0x00000001, 0x00000000, 0x666e7373, 0x00666e00, 0x00666e76, 0x00000321, 0x0000026c, 0x00000324,
0x00000280, 0x00000005, 0x00040001, 0x00020001, 0x00000328, 0x0031666e, 0x00000321, 0x0000026c,
0x00000098, 0x00000338, 0x00000348, 0x0000026c, 0x00000005, 0x00060001, 0x00030001, 0x0000034c,
0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x422c0000, 0x42300000, 0x42240000, 0x00000000, 0x42280000, 0x00000000, 0x00000000, 0x00000000,
0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131, 0x05000051, 0xa00f000c,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f,
0x80000000, 0xe00f0000, 0x01000028, 0xe0e40805, 0x03000005, 0x80010000, 0xa0000003, 0x90aa0000,
0x02000001, 0x80010001, 0xa000000c, 0x01000026, 0xf0e40005, 0x04000004, 0x80010001, 0x80000000,
0xa0000007, 0x80000001, 0x00000027, 0x02000001, 0x80020001, 0xa000000c, 0x0000002a, 0x03000005,
0x80010000, 0xa0aa0002, 0x90550000, 0x02000001, 0x80020001, 0xa000000c, 0x01000026, 0xf0e40004,
0x04000004, 0x80020001, 0x80000000, 0xa000000b, 0x80550001, 0x00000027, 0x02000001, 0x80010001,
0xa000000c, 0x0000002b, 0x02000001, 0xe0030000, 0x80e40001, 0x02000001, 0xe00c0000, 0xa000000c,
0x0000ffff,
};

static const struct registerset_constants registerset_constants_struct_struct_float[] =
{
    {"ssf",      {"ssf", D3DXRS_FLOAT4, 0, 4, D3DXPC_STRUCT, D3DXPT_VOID,  1, 6, 1, 3, 24, NULL}, 137},
    {"ssf.f",    {"f",   D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0,  4, NULL}, 137},
    {"ssf.s",    {"s",   D3DXRS_FLOAT4, 1, 2, D3DXPC_STRUCT, D3DXPT_VOID,  1, 4, 1, 2, 16, NULL}, 141},
    {"ssf.s.f",  {"f",   D3DXRS_FLOAT4, 1, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0,  4, NULL}, 141},
    {"ssf.s.vf", {"vf",  D3DXRS_FLOAT4, 2, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 1, 0, 12, NULL}, 145},
    {"ssf.f1",   {"f1",  D3DXRS_FLOAT4, 3, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0,  4, NULL}, 149},
};

static const struct registerset_test registerset_test_struct_struct_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x00000000, 0x00000000, 0x00000000,
        0x40800000, 0x40a00000, 0x00000000, 0x00000000, 0x40e00000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800123, 0x40a00123, 0x40c00123, 0x00000000, 0x40e00123}},
    {SetValue, 0, 0, 23},
    {SetValue, 0, 24, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800123, 0x40a00123, 0x40c00123, 0x00000000, 0x40e00123}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800123, 0x40a00123, 0x40c00123, 0x00000000, 0x40e00123}},
    {SetMatrix, 0, 0, 0, 16,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800123, 0x40a00123, 0x40c00123, 0x00000000, 0x40e00123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000123, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800123, 0x40a00123, 0x40c00123, 0x00000000, 0x40e00123}},
    {SetMatrixTranspose, 0, 0, 0, 16, {0x40000123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x40000123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x40000123}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x40000123}},
};

static const struct registerset_constants registerset_constants_struct_struct_int[] =
{
    {"ssn",      {"ssn", D3DXRS_INT4, 0, 6, D3DXPC_STRUCT, D3DXPT_VOID, 1, 6, 1, 3, 24, NULL}, 183},
    {"ssn.n",    {"n",   D3DXRS_INT4, 0, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 183},
    {"ssn.s",    {"s",   D3DXRS_INT4, 1, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 187},
    {"ssn.s.n",  {"n",   D3DXRS_INT4, 1, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 187},
    {"ssn.s.vn", {"vn",  D3DXRS_INT4, 2, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 191},
    {"ssn.n1",   {"n1",  D3DXRS_INT4, 3, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 195},
};

static const struct registerset_test registerset_test_struct_struct_int[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000000, 0x00000001, 0x00000000,
        0x00000004, 0x00000005, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x00000001}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000001, 0x00000000,
        0x00000001, 0x00000001, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000001}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000004, 0x00000005, 0x00000006, 0x00000000, 0x00000007, 0x00000000, 0x00000001}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000003, 0x00000000, 0x00000001, 0x00000000,
        0x00000004, 0x00000005, 0x00000000, 0x00000000, 0x00000007, 0x00000000, 0x00000001}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000004, 0x00000005, 0x00000006, 0x00000000, 0x00000007, 0x00000000, 0x00000001}},
    {SetMatrix, 0, 0, 0, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000004, 0x00000005, 0x00000006, 0x00000000, 0x00000007, 0x00000000, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000004, 0x00000005, 0x00000006, 0x00000000, 0x00000007, 0x00000000, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001}},
};

static const struct registerset_constants registerset_constants_struct_struct_bool[] =
{
    {"ssb",      {"ssb", D3DXRS_BOOL, 0, 6, D3DXPC_STRUCT, D3DXPT_VOID, 1, 6, 1, 3, 24, NULL}, 63},
    {"ssb.b",    {"b",   D3DXRS_BOOL, 0, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 63},
    {"ssb.s",    {"s",   D3DXRS_BOOL, 1, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 64},
    {"ssb.s.b",  {"b",   D3DXRS_BOOL, 1, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 64},
    {"ssb.s.vb", {"vb",  D3DXRS_BOOL, 2, 3, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL}, 65},
    {"ssb.b1",   {"b1",  D3DXRS_BOOL, 5, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 68},
};

static const struct registerset_test registerset_test_struct_struct_bool[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 6,
        {0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000000, 0x00000007}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrix, 0, 0, 0, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTranspose, 0, 0, 0, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 6,
        {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000001}},
};

static const struct registerset_constants registerset_constants_struct_struct_int_float[] =
{
    {"ssnf",       {"ssnf", D3DXRS_FLOAT4, 4, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 6, 1, 3, 24, NULL}, 221},
    {"ssnf.nf",    {"nf",   D3DXRS_FLOAT4, 4, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 221},
    {"ssnf.s",     {"s",    D3DXRS_FLOAT4, 5, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL}, 225},
    {"ssnf.s.nf",  {"nf",   D3DXRS_FLOAT4, 5, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 225},
    {"ssnf.s.vnf", {"vnf",  D3DXRS_FLOAT4, 6, 1, D3DXPC_VECTOR, D3DXPT_INT,  1, 3, 1, 0, 12, NULL}, 229},
    {"ssnf.nf1",   {"nf1",  D3DXRS_FLOAT4, 7, 1, D3DXPC_SCALAR, D3DXPT_INT,  1, 1, 1, 0,  4, NULL}, 233},
};

static const struct registerset_test registerset_test_struct_struct_int_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x00000000, 0x00000000, 0x00000000,
        0x40800000, 0x40a00000, 0x00000000, 0x00000000, 0x40e00000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800000, 0x40a00000, 0x40c00000, 0x00000000, 0x40e00000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x40400000, 0x00000000, 0x00000000, 0x00000000,
        0x40800000, 0x40a00000, 0x00000000, 0x00000000, 0x40e00000}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800000, 0x40a00000, 0x40c00000, 0x00000000, 0x40e00000}},
    {SetMatrix, 0, 0, 0, 16,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800000, 0x40a00000, 0x40c00000, 0x00000000, 0x40e00000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x40800000, 0x40a00000, 0x40c00000, 0x00000000, 0x40e00000}},
    {SetMatrixTranspose, 0, 0, 0, 16, {0x40000000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x40000000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x40000000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x40000000}},
};

static const struct registerset_constants registerset_constants_struct_struct_bool_float[] =
{
    {"ssbf",       {"ssbf", D3DXRS_FLOAT4,  8, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 6, 1, 3, 24, NULL},  91},
    {"ssbf.bf",    {"bf",   D3DXRS_FLOAT4,  8, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL},  91},
    {"ssbf.s",     {"s",    D3DXRS_FLOAT4,  9, 2, D3DXPC_STRUCT, D3DXPT_VOID, 1, 4, 1, 2, 16, NULL},  95},
    {"ssbf.s.bf",  {"bf",   D3DXRS_FLOAT4,  9, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL},  95},
    {"ssbf.s.vbf", {"vbf",  D3DXRS_FLOAT4, 10, 1, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 1, 0, 12, NULL},  99},
    {"ssbf.bf1",   {"bf1",  D3DXRS_FLOAT4, 11, 1, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 1, 0,  4, NULL}, 103},
};

static const struct registerset_test registerset_test_struct_struct_bool_float[] =
{
    {SetInt},
    {SetBool},
    {SetFloat},
    {SetIntArray, 1, 0, 5},
    {SetIntArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetBoolArray, 1, 0, 5},
    {SetBoolArray, 1, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetFloatArray, 0, 0, 5},
    {SetFloatArray, 0, 6, REGISTER_OUTPUT_SIZE, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetValue, 1, 0, 23},
    {SetValue, 1, 24, REGISTER_OUTPUT_SIZE * 4, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000}},
    {SetVector},
    {SetVectorArray, 0, 0, 1},
    {SetVectorArray, 0, 2, REGISTER_OUTPUT_SIZE / 4, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetMatrix, 0, 0, 0, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16,
        {0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000}},
    {SetMatrixTranspose, 0, 0, 0, 16, {0x3f800000}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x3f800000}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x3f800000}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, REGISTER_OUTPUT_SIZE / 16, 16, {0x3f800000}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
row_major int3x2 ran[2] = {4, 5, 6, 1, 8, 1, 2, 3, 4, 7, 9, 1};
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    int k;
    for (k = 0; k < ran[1]._21; k++)
        tmp.y += pos.y + tmp.x;
    return tmp;
}
#endif
static const DWORD registerset_blob_special_int[] =
{
0xfffe0300, 0x0038fffe, 0x42415443, 0x0000001c, 0x000000ab, 0xfffe0300, 0x00000001, 0x0000001c,
0x00000100, 0x000000a4, 0x00000030, 0x00000001, 0x00000009, 0x00000034, 0x00000044, 0x006e6172,
0x00020002, 0x00020003, 0x00000002, 0x00000000, 0x00000004, 0x00000005, 0x00000001, 0x00000000,
0x00000006, 0x00000001, 0x00000001, 0x00000000, 0x00000008, 0x00000001, 0x00000001, 0x00000000,
0x00000002, 0x00000003, 0x00000001, 0x00000000, 0x00000004, 0x00000007, 0x00000001, 0x00000000,
0x00000009, 0x00000001, 0x00000001, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73,
0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932,
0x332e3235, 0x00313131, 0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x0200001f, 0x80000000, 0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x02000001, 0x80010000,
0xa0000000, 0x01000026, 0xf0e40008, 0x03000002, 0x80010000, 0x80000000, 0x90550000, 0x00000027,
0x02000001, 0xe0020000, 0x80000000, 0x02000001, 0xe00d0000, 0xa0000000, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_special_int[] =
{
    {"ran",    {"ran", D3DXRS_INT4, 0, 9, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3,  2, 2, 0, 48, NULL}, 17},
    {"ran[0]", {"ran", D3DXRS_INT4, 0, 3, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3,  2, 1, 0, 24, NULL}, 17},
    {"ran[1]", {"ran", D3DXRS_INT4, 3, 3, D3DXPC_MATRIX_ROWS, D3DXPT_INT, 3,  2, 1, 0, 24, NULL}, 29},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
float3 vaf[10];
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    tmp.y += pos.y + vaf[8].x;
    return tmp;
}
#endif
static const DWORD registerset_blob_bigvec[] =
{
0xfffe0300, 0x0020fffe, 0x42415443, 0x0000001c, 0x0000004b, 0xfffe0300, 0x00000001, 0x0000001c,
0x00000100, 0x00000044, 0x00000030, 0x00000002, 0x00000009, 0x00000034, 0x00000000, 0x00666176,
0x00030001, 0x00030001, 0x0000000a, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73,
0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932,
0x332e3235, 0x00313131, 0x05000051, 0xa00f0009, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x0200001f, 0x80000000, 0x900f0000, 0x0200001f, 0x80000000, 0xe00f0000, 0x03000002, 0xe0020000,
0xa0000008, 0x90550000, 0x02000001, 0xe00d0000, 0xa0000009, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_bigvec_float[] =
{
    {"vaf",    {"vaf", D3DXRS_FLOAT4, 0, 9, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3, 10, 0, 120, NULL}, 0},
    {"vaf[0]", {"vaf", D3DXRS_FLOAT4, 0, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[1]", {"vaf", D3DXRS_FLOAT4, 1, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[2]", {"vaf", D3DXRS_FLOAT4, 2, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[3]", {"vaf", D3DXRS_FLOAT4, 3, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[4]", {"vaf", D3DXRS_FLOAT4, 4, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[5]", {"vaf", D3DXRS_FLOAT4, 5, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[6]", {"vaf", D3DXRS_FLOAT4, 6, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[7]", {"vaf", D3DXRS_FLOAT4, 7, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[8]", {"vaf", D3DXRS_FLOAT4, 8, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
    {"vaf[9]", {"vaf", D3DXRS_FLOAT4, 9, 0, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 3,  1, 0,  12, NULL}, 0},
};

static const struct registerset_test registerset_test_bigvec_float[] =
{
    {SetMatrix, 0, 0, 0, 16,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41200123, 0x41300123, 0x41400123, 0x00000000, 0x41600123, 0x41700123, 0x41800123}},
    {SetMatrixArray},
    {SetMatrixArray, 0, 1, 1, 16,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41200123, 0x41300123, 0x41400123, 0x00000000, 0x41600123, 0x41700123, 0x41800123}},
    {SetMatrixArray, 0, 2, 2, 32,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41200123, 0x41300123, 0x41400123, 0x00000000, 0x41600123, 0x41700123, 0x41800123, 0x00000000,
        0x41a00123, 0x41b00123, 0x41c00123, 0x00000000, 0x00000000, 0x41f00123, 0x42000123, 0x00000000,
        0x00000000, 0x42300123, 0x42400123, 0x00000000, 0x42600123, 0x42700123, 0x42800123}},
    {SetMatrixArray, 0, 3, REGISTER_OUTPUT_SIZE / 16, 36,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41200123, 0x41300123, 0x41400123, 0x00000000, 0x41600123, 0x41700123, 0x41800123, 0x00000000,
        0x41a00123, 0x41b00123, 0x41c00123, 0x00000000, 0x00000000, 0x41f00123, 0x42000123, 0x00000000,
        0x00000000, 0x42300123, 0x42400123, 0x00000000, 0x42600123, 0x42700123, 0x42800123, 0x00000000,
        0x43000123, 0x43100123, 0x43200123}},
    {SetMatrixTranspose, 0, 0, 0, 16,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x40800123, 0x41000123, 0x41400123, 0x00000000, 0x40a00123, 0x41100123, 0x41500123}},
    {SetMatrixTransposeArray},
    {SetMatrixTransposeArray, 0, 1, 1, 16,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x40800123, 0x41000123, 0x41400123, 0x00000000, 0x40a00123, 0x41100123, 0x41500123}},
    {SetMatrixTransposeArray, 0, 2, 2, 32,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x40800123, 0x41000123, 0x41400123, 0x00000000, 0x40a00123, 0x41100123, 0x41500123, 0x00000000,
        0x41a00123, 0x00000000, 0x00000000, 0x00000000, 0x41b00123, 0x41f00123, 0x42300123, 0x00000000,
        0x41c00123, 0x42000123, 0x42400123, 0x00000000, 0x41d00123, 0x42100123, 0x42500123}},
    {SetMatrixTransposeArray, 0, 3, REGISTER_OUTPUT_SIZE / 16, 36,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x40800123, 0x41000123, 0x41400123, 0x00000000, 0x40a00123, 0x41100123, 0x41500123, 0x00000000,
        0x41a00123, 0x00000000, 0x00000000, 0x00000000, 0x41b00123, 0x41f00123, 0x42300123, 0x00000000,
        0x41c00123, 0x42000123, 0x42400123, 0x00000000, 0x41d00123, 0x42100123, 0x42500123, 0x00000000,
        0x43000123, 0x43400123, 0x43800123}},
    {SetMatrixPointerArray},
    {SetMatrixPointerArray, 0, 1, 1, 16,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41200123, 0x41300123, 0x41400123, 0x00000000, 0x41600123, 0x41700123, 0x41800123}},
    {SetMatrixPointerArray, 0, 2, 2, 32,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41200123, 0x41300123, 0x41400123, 0x00000000, 0x41600123, 0x41700123, 0x41800123, 0x00000000,
        0x41900123, 0x41a00123, 0x41b00123, 0x00000000, 0x41d00123, 0x00000000, 0x41f00123, 0x00000000,
        0x42100123, 0x00000000, 0x42300123, 0x00000000, 0x42500123, 0x42600123, 0x42700123}},
    {SetMatrixPointerArray, 0, 3, REGISTER_OUTPUT_SIZE / 16, 36,
        {0x40000123, 0x00000000, 0x40800123, 0x00000000, 0x40c00123, 0x40e00123, 0x41000123, 0x00000000,
        0x41200123, 0x41300123, 0x41400123, 0x00000000, 0x41600123, 0x41700123, 0x41800123, 0x00000000,
        0x41900123, 0x41a00123, 0x41b00123, 0x00000000, 0x41d00123, 0x00000000, 0x41f00123, 0x00000000,
        0x42100123, 0x00000000, 0x42300123, 0x00000000, 0x42500123, 0x42600123, 0x42700123, 0x00000000,
        0x42800123, 0x42900123, 0x43000123}},
    {SetMatrixTransposePointerArray},
    {SetMatrixTransposePointerArray, 0, 1, 1, 16,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x40800123, 0x41000123, 0x41400123, 0x00000000, 0x40a00123, 0x41100123, 0x41500123}},
    {SetMatrixTransposePointerArray, 0, 2, 2, 32,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x40800123, 0x41000123, 0x41400123, 0x00000000, 0x40a00123, 0x41100123, 0x41500123, 0x00000000,
        0x41900123, 0x41d00123, 0x42100123, 0x00000000, 0x41a00123, 0x00000000, 0x00000000, 0x00000000,
        0x41b00123, 0x41f00123, 0x42300123, 0x00000000, 0x41c00123, 0x42000123, 0x42400123}},
    {SetMatrixTransposePointerArray, 0, 3, REGISTER_OUTPUT_SIZE / 16, 36,
        {0x40000123, 0x40c00123, 0x41200123, 0x00000000, 0x00000000, 0x40e00123, 0x41300123, 0x00000000,
        0x40800123, 0x41000123, 0x41400123, 0x00000000, 0x40a00123, 0x41100123, 0x41500123, 0x00000000,
        0x41900123, 0x41d00123, 0x42100123, 0x00000000, 0x41a00123, 0x00000000, 0x00000000, 0x00000000,
        0x41b00123, 0x41f00123, 0x42300123, 0x00000000, 0x41c00123, 0x42000123, 0x42400123, 0x00000000,
        0x42800123, 0x43200123, 0x43600123}},
};

/*
 * fxc.exe /Tvs_3_0
 */
#if 0
float4x4 cf = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8};
float f = 33.33;
float4 main(float4 pos : POSITION) : POSITION
{
    float4 tmp = 0;
    tmp.y += cf._22;
    tmp.z = f;
    return tmp;
}
#endif
static const DWORD registerset_blob_matrix_column_clamp[] =
{
0xfffe0300, 0x003efffe, 0x42415443, 0x0000001c, 0x000000c3, 0xfffe0300, 0x00000002, 0x0000001c,
0x00000100, 0x000000bc, 0x00000044, 0x00000002, 0x00000002, 0x00000048, 0x00000058, 0x00000098,
0x00020002, 0x00000001, 0x0000009c, 0x000000ac, 0xab006663, 0x00030003, 0x00040004, 0x00000001,
0x00000000, 0x3f8ccccd, 0x40b00000, 0x411e6666, 0x3fc00000, 0x400ccccd, 0x40d33333, 0x3f99999a,
0x3fcccccd, 0x40533333, 0x40f66666, 0x3fa66666, 0x3fd9999a, 0x408ccccd, 0x410ccccd, 0x3fb33333,
0x3fe66666, 0xabab0066, 0x00030000, 0x00010001, 0x00000001, 0x00000000, 0x420551ec, 0x00000000,
0x00000000, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,
0x05000051, 0xa00f0003, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x0200001f, 0x80000000,
0xe00f0000, 0x02000001, 0x80020000, 0xa0550001, 0x03000005, 0xe00b0000, 0x80550000, 0xa0240003,
0x02000001, 0xe0040000, 0xa0000002, 0x0000ffff,
};

static const struct registerset_constants registerset_constants_matrix_column_clamp[] =
{
    {"cf", {"cf", D3DXRS_FLOAT4, 0, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 1, 0, 64, NULL}, 0},
    {"f", {"f", D3DXRS_FLOAT4, 2, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4, NULL}, 0},
};

static const struct registerset_test registerset_test_matrix_column_clamp[] =
{
    {SetMatrix, 0, 0, 0, 8,
        {0x40000123, 0x40c00123, 0x41200123, 0x41600123, 0x00000000, 0x40e00123, 0x41300123, 0x41700123}},
};

static const struct
{
    const char *name;
    const char *var;
    UINT start;
    D3DXREGISTER_SET regset;
    const DWORD *blob;
    const struct registerset_test *tests;
    UINT test_count;
    const struct registerset_constants *constants;
    UINT constant_count;
}
registerset_data[] =
{
    /* scalar */
    {"float", "f", 0, D3DXRS_FLOAT4, registerset_blob_scalar,
        registerset_test_scalar_float, ARRAY_SIZE(registerset_test_scalar_float),
        registerset_constants_scalar_float, ARRAY_SIZE(registerset_constants_scalar_float)},
    {"int", "n", 0, D3DXRS_INT4, registerset_blob_scalar,
        registerset_test_scalar_int, ARRAY_SIZE(registerset_test_scalar_int),
        registerset_constants_scalar_int, ARRAY_SIZE(registerset_constants_scalar_int)},
    {"int float", "nf", 4, D3DXRS_FLOAT4, registerset_blob_scalar,
        registerset_test_scalar_int_float, ARRAY_SIZE(registerset_test_scalar_int_float),
        registerset_constants_scalar_int_float, ARRAY_SIZE(registerset_constants_scalar_int_float)},
    {"bool float", "bf", 8, D3DXRS_FLOAT4, registerset_blob_scalar,
        registerset_test_scalar_bool_float, ARRAY_SIZE(registerset_test_scalar_bool_float),
        registerset_constants_scalar_bool_float, ARRAY_SIZE(registerset_constants_scalar_bool_float)},
    {"bool", "b", 0, D3DXRS_BOOL, registerset_blob_scalar,
        registerset_test_scalar_bool, ARRAY_SIZE(registerset_test_scalar_bool),
        registerset_constants_scalar_bool, ARRAY_SIZE(registerset_constants_scalar_bool)},
    /* scalar array */
    {"float [2]", "af", 0, D3DXRS_FLOAT4, registerset_blob_scalar_array,
        registerset_test_scalar_array_float, ARRAY_SIZE(registerset_test_scalar_array_float),
        registerset_constants_scalar_array_float, ARRAY_SIZE(registerset_constants_scalar_array_float)},
    {"int [2]", "an", 0, D3DXRS_INT4, registerset_blob_scalar_array,
        registerset_test_scalar_array_int, ARRAY_SIZE(registerset_test_scalar_array_int),
        registerset_constants_scalar_array_int, ARRAY_SIZE(registerset_constants_scalar_array_int)},
    {"bool [2]", "ab", 0, D3DXRS_BOOL, registerset_blob_scalar_array,
        registerset_test_scalar_array_bool, ARRAY_SIZE(registerset_test_scalar_array_bool),
        registerset_constants_scalar_array_bool, ARRAY_SIZE(registerset_constants_scalar_array_bool)},
    {"int float [2]", "anf", 8, D3DXRS_FLOAT4, registerset_blob_scalar_array,
        registerset_test_scalar_array_int_float, ARRAY_SIZE(registerset_test_scalar_array_int_float),
        registerset_constants_scalar_array_int_float, ARRAY_SIZE(registerset_constants_scalar_array_int_float)},
    {"bool float [2]", "abf", 16, D3DXRS_FLOAT4, registerset_blob_scalar_array,
        registerset_test_scalar_array_bool_float, ARRAY_SIZE(registerset_test_scalar_array_bool_float),
        registerset_constants_scalar_array_bool_float, ARRAY_SIZE(registerset_constants_scalar_array_bool_float)},
    /* vector */
    {"float 3", "vf", 0, D3DXRS_FLOAT4, registerset_blob_vector,
        registerset_test_vector_float, ARRAY_SIZE(registerset_test_vector_float),
        registerset_constants_vector_float, ARRAY_SIZE(registerset_constants_vector_float)},
    {"int 3", "vn", 0, D3DXRS_INT4, registerset_blob_vector,
        registerset_test_vector_int, ARRAY_SIZE(registerset_test_vector_int),
        registerset_constants_vector_int, ARRAY_SIZE(registerset_constants_vector_int)},
    {"bool 3", "vb", 0, D3DXRS_BOOL, registerset_blob_vector,
        registerset_test_vector_bool, ARRAY_SIZE(registerset_test_vector_bool),
        registerset_constants_vector_bool, ARRAY_SIZE(registerset_constants_vector_bool)},
    {"bool float 3", "vbf", 8, D3DXRS_FLOAT4, registerset_blob_vector,
        registerset_test_vector_bool_float, ARRAY_SIZE(registerset_test_vector_bool_float),
        registerset_constants_vector_bool_float, ARRAY_SIZE(registerset_constants_vector_bool_float)},
    {"int float 3", "vnf", 4, D3DXRS_FLOAT4, registerset_blob_vector,
        registerset_test_vector_int_float, ARRAY_SIZE(registerset_test_vector_int_float),
        registerset_constants_vector_int_float, ARRAY_SIZE(registerset_constants_vector_int_float)},
    /* vector array */
    {"float 3 [2]", "vaf", 0, D3DXRS_FLOAT4, registerset_blob_vector_array,
        registerset_test_vector_array_float, ARRAY_SIZE(registerset_test_vector_array_float),
        registerset_constants_vector_array_float, ARRAY_SIZE(registerset_constants_vector_array_float)},
    {"int 3 [2]", "van", 0, D3DXRS_INT4, registerset_blob_vector_array,
        registerset_test_vector_array_int, ARRAY_SIZE(registerset_test_vector_array_int),
        registerset_constants_vector_array_int, ARRAY_SIZE(registerset_constants_vector_array_int)},
    {"bool 3 [2]", "vab", 0, D3DXRS_BOOL, registerset_blob_vector_array,
        registerset_test_vector_array_bool, ARRAY_SIZE(registerset_test_vector_array_bool),
        registerset_constants_vector_array_bool, ARRAY_SIZE(registerset_constants_vector_array_bool)},
    {"bool float 3 [2]", "vabf", 16, D3DXRS_FLOAT4, registerset_blob_vector_array,
        registerset_test_vector_array_bool_float, ARRAY_SIZE(registerset_test_vector_array_bool_float),
        registerset_constants_vector_array_bool_float, ARRAY_SIZE(registerset_constants_vector_array_bool_float)},
    {"int float 3 [2]", "vanf", 8, D3DXRS_FLOAT4, registerset_blob_vector_array,
        registerset_test_vector_array_int_float, ARRAY_SIZE(registerset_test_vector_array_int_float),
        registerset_constants_vector_array_int_float, ARRAY_SIZE(registerset_constants_vector_array_int_float)},
    /* matrix column */
    {"float c3x2", "cf", 0, D3DXRS_FLOAT4, registerset_blob_column,
        registerset_test_column_float, ARRAY_SIZE(registerset_test_column_float),
        registerset_constants_column_float, ARRAY_SIZE(registerset_constants_column_float)},
    {"int c3x2", "cn", 0, D3DXRS_INT4, registerset_blob_column,
        registerset_test_column_int, ARRAY_SIZE(registerset_test_column_int),
        registerset_constants_column_int, ARRAY_SIZE(registerset_constants_column_int)},
    {"bool c3x2", "cb", 0, D3DXRS_BOOL, registerset_blob_column,
        registerset_test_column_bool, ARRAY_SIZE(registerset_test_column_bool),
        registerset_constants_column_bool, ARRAY_SIZE(registerset_constants_column_bool)},
    {"bool float c3x2", "cbf", 8, D3DXRS_FLOAT4, registerset_blob_column,
        registerset_test_column_bool_float, ARRAY_SIZE(registerset_test_column_bool_float),
        registerset_constants_column_bool_float, ARRAY_SIZE(registerset_constants_column_bool_float)},
    {"int float c3x2", "cnf", 16, D3DXRS_FLOAT4, registerset_blob_column,
        registerset_test_column_int_float, ARRAY_SIZE(registerset_test_column_int_float),
        registerset_constants_column_int_float, ARRAY_SIZE(registerset_constants_column_int_float)},
    /* matrix column array */
    {"float c3x2 [2]", "caf", 0, D3DXRS_FLOAT4, registerset_blob_column_array,
        registerset_test_column_array_float, ARRAY_SIZE(registerset_test_column_array_float),
        registerset_constants_column_array_float, ARRAY_SIZE(registerset_constants_column_array_float)},
    {"int c3x2 [2]", "can", 0, D3DXRS_INT4, registerset_blob_column_array,
        registerset_test_column_array_int, ARRAY_SIZE(registerset_test_column_array_int),
        registerset_constants_column_array_int, ARRAY_SIZE(registerset_constants_column_array_int)},
    {"bool c3x2 [2]", "cab", 0, D3DXRS_BOOL, registerset_blob_column_array,
        registerset_test_column_array_bool, ARRAY_SIZE(registerset_test_column_array_bool),
        registerset_constants_column_array_bool, ARRAY_SIZE(registerset_constants_column_array_bool)},
    {"bool float c3x2 [2]", "cabf", 16, D3DXRS_FLOAT4, registerset_blob_column_array,
        registerset_test_column_array_bool_float, ARRAY_SIZE(registerset_test_column_array_bool_float),
        registerset_constants_column_array_bool_float, ARRAY_SIZE(registerset_constants_column_array_bool_float)},
    {"int float c3x2 [2]", "canf", 32, D3DXRS_FLOAT4, registerset_blob_column_array,
        registerset_test_column_array_int_float, ARRAY_SIZE(registerset_test_column_array_int_float),
        registerset_constants_column_array_int_float, ARRAY_SIZE(registerset_constants_column_array_int_float)},
    /* matrix row */
    {"float r3x2", "rf", 0, D3DXRS_FLOAT4, registerset_blob_row,
        registerset_test_row_float, ARRAY_SIZE(registerset_test_row_float),
        registerset_constants_row_float, ARRAY_SIZE(registerset_constants_row_float)},
    {"int r3x2", "rn", 0, D3DXRS_INT4, registerset_blob_row,
        registerset_test_row_int, ARRAY_SIZE(registerset_test_row_int),
        registerset_constants_row_int, ARRAY_SIZE(registerset_constants_row_int)},
    {"bool r3x2", "rb", 0, D3DXRS_BOOL, registerset_blob_row,
        registerset_test_row_bool, ARRAY_SIZE(registerset_test_row_bool),
        registerset_constants_row_bool, ARRAY_SIZE(registerset_constants_row_bool)},
    {"bool float r3x2", "rbf", 12, D3DXRS_FLOAT4, registerset_blob_row,
        registerset_test_row_bool_float, ARRAY_SIZE(registerset_test_row_bool_float),
        registerset_constants_row_bool_float, ARRAY_SIZE(registerset_constants_row_bool_float)},
    {"int float r3x2", "rnf", 24, D3DXRS_FLOAT4, registerset_blob_row,
        registerset_test_row_int_float, ARRAY_SIZE(registerset_test_row_int_float),
        registerset_constants_row_int_float, ARRAY_SIZE(registerset_constants_row_int_float)},
    /* matrix row array */
    {"float 3x2 [2]", "raf", 0, D3DXRS_FLOAT4, registerset_blob_row_array,
        registerset_test_row_array_float, ARRAY_SIZE(registerset_test_row_array_float),
        registerset_constants_row_array_float, ARRAY_SIZE(registerset_constants_row_array_float)},
    {"int 3x2 [2]", "ran", 0, D3DXRS_INT4, registerset_blob_row_array,
        registerset_test_row_array_int, ARRAY_SIZE(registerset_test_row_array_int),
        registerset_constants_row_array_int, ARRAY_SIZE(registerset_constants_row_array_int)},
    {"bool 3x2 [2]", "rab", 0, D3DXRS_BOOL, registerset_blob_row_array,
        registerset_test_row_array_bool, ARRAY_SIZE(registerset_test_row_array_bool),
        registerset_constants_row_array_bool, ARRAY_SIZE(registerset_constants_row_array_bool)},
    {"bool float 3x2 [2]", "rabf", 24, D3DXRS_FLOAT4, registerset_blob_row_array,
        registerset_test_row_array_bool_float, ARRAY_SIZE(registerset_test_row_array_bool_float),
        registerset_constants_row_array_bool_float, ARRAY_SIZE(registerset_constants_row_array_bool_float)},
    {"int float 3x2 [2]", "ranf", 48, D3DXRS_FLOAT4, registerset_blob_row_array,
        registerset_test_row_array_int_float, ARRAY_SIZE(registerset_test_row_array_int_float),
        registerset_constants_row_array_int_float, ARRAY_SIZE(registerset_constants_row_array_int_float)},
    /* struct */
    {"struct float", "sf", 0, D3DXRS_FLOAT4, registerset_blob_struct,
        registerset_test_struct_float, ARRAY_SIZE(registerset_test_struct_float),
        registerset_constants_struct_float, ARRAY_SIZE(registerset_constants_struct_float)},
    {"struct int", "sn", 0, D3DXRS_INT4, registerset_blob_struct,
        registerset_test_struct_int, ARRAY_SIZE(registerset_test_struct_int),
        registerset_constants_struct_int, ARRAY_SIZE(registerset_constants_struct_int)},
    {"struct bool", "sb", 0, D3DXRS_BOOL, registerset_blob_struct,
        registerset_test_struct_bool, ARRAY_SIZE(registerset_test_struct_bool),
        registerset_constants_struct_bool, ARRAY_SIZE(registerset_constants_struct_bool)},
    {"struct bool float", "sbf", 16, D3DXRS_FLOAT4, registerset_blob_struct,
        registerset_test_struct_bool_float, ARRAY_SIZE(registerset_test_struct_bool_float),
        registerset_constants_struct_bool_float, ARRAY_SIZE(registerset_constants_struct_bool_float)},
    {"struct int float", "snf", 8, D3DXRS_FLOAT4, registerset_blob_struct,
        registerset_test_struct_int_float, ARRAY_SIZE(registerset_test_struct_int_float),
        registerset_constants_struct_int_float, ARRAY_SIZE(registerset_constants_struct_int_float)},
    /* struct array */
    {"struct float [2]", "saf", 0, D3DXRS_FLOAT4, registerset_blob_struct_array,
        registerset_test_struct_array_float, ARRAY_SIZE(registerset_test_struct_array_float),
        registerset_constants_struct_array_float, ARRAY_SIZE(registerset_constants_struct_array_float)},
    {"struct int [2]", "san", 0, D3DXRS_INT4, registerset_blob_struct_array,
        registerset_test_struct_array_int, ARRAY_SIZE(registerset_test_struct_array_int),
        registerset_constants_struct_array_int, ARRAY_SIZE(registerset_constants_struct_array_int)},
    {"struct bool [2]", "sab", 0, D3DXRS_BOOL, registerset_blob_struct_array,
        registerset_test_struct_array_bool, ARRAY_SIZE(registerset_test_struct_array_bool),
        registerset_constants_struct_array_bool, ARRAY_SIZE(registerset_constants_struct_array_bool)},
    {"struct bool float [2]", "sabf", 32, D3DXRS_FLOAT4, registerset_blob_struct_array,
        registerset_test_struct_array_bool_float, ARRAY_SIZE(registerset_test_struct_array_bool_float),
        registerset_constants_struct_array_bool_float, ARRAY_SIZE(registerset_constants_struct_array_bool_float)},
    {"struct int float [2]", "sanf", 16, D3DXRS_FLOAT4, registerset_blob_struct_array,
        registerset_test_struct_array_int_float, ARRAY_SIZE(registerset_test_struct_array_int_float),
        registerset_constants_struct_array_int_float, ARRAY_SIZE(registerset_constants_struct_array_int_float)},
    /* struct struct */
    {"struct struct float", "ssf", 0, D3DXRS_FLOAT4, registerset_blob_struct_struct,
        registerset_test_struct_struct_float, ARRAY_SIZE(registerset_test_struct_struct_float),
        registerset_constants_struct_struct_float, ARRAY_SIZE(registerset_constants_struct_struct_float)},
    {"struct struct int", "ssn", 0, D3DXRS_INT4, registerset_blob_struct_struct,
        registerset_test_struct_struct_int, ARRAY_SIZE(registerset_test_struct_struct_int),
        registerset_constants_struct_struct_int, ARRAY_SIZE(registerset_constants_struct_struct_int)},
    {"struct struct bool", "ssb", 0, D3DXRS_BOOL, registerset_blob_struct_struct,
        registerset_test_struct_struct_bool, ARRAY_SIZE(registerset_test_struct_struct_bool),
        registerset_constants_struct_struct_bool, ARRAY_SIZE(registerset_constants_struct_struct_bool)},
    {"struct struct bool float", "ssbf", 32, D3DXRS_FLOAT4, registerset_blob_struct_struct,
        registerset_test_struct_struct_bool_float, ARRAY_SIZE(registerset_test_struct_struct_bool_float),
        registerset_constants_struct_struct_bool_float, ARRAY_SIZE(registerset_constants_struct_struct_bool_float)},
    {"struct struct int float", "ssnf", 16, D3DXRS_FLOAT4, registerset_blob_struct_struct,
        registerset_test_struct_struct_int_float, ARRAY_SIZE(registerset_test_struct_struct_int_float),
        registerset_constants_struct_struct_int_float, ARRAY_SIZE(registerset_constants_struct_struct_int_float)},
    /* special */
    {"int ran", "ran", 0, D3DXRS_INT4, registerset_blob_special_int, NULL, 0,
        registerset_constants_special_int, ARRAY_SIZE(registerset_constants_special_int)},
    {"bigvec", "vaf", 0, D3DXRS_FLOAT4, registerset_blob_bigvec,
        registerset_test_bigvec_float, ARRAY_SIZE(registerset_test_bigvec_float),
        registerset_constants_bigvec_float, ARRAY_SIZE(registerset_constants_bigvec_float)},
    {"cf", "cf", 0, D3DXRS_FLOAT4, registerset_blob_matrix_column_clamp,
        registerset_test_matrix_column_clamp, ARRAY_SIZE(registerset_test_matrix_column_clamp),
        registerset_constants_matrix_column_clamp, ARRAY_SIZE(registerset_constants_matrix_column_clamp)},
};

static void registerset_clear(IDirect3DDevice9 *device)
{
    DWORD zero[1024];
    HRESULT hr;

    memset(zero, 0xde, 4096);

    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, (FLOAT*)zero, 256);
    ok(hr == D3D_OK, "Clear failed, got %08x, expected %08x\n", hr, D3D_OK);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, (FLOAT*)zero, 224);
    ok(hr == D3D_OK, "Clear failed, got %08x, expected %08x\n", hr, D3D_OK);

    hr = IDirect3DDevice9_SetVertexShaderConstantB(device, 0, (BOOL*)zero, 16);
    ok(hr == D3D_OK, "Clear failed, got %08x, expected %08x\n", hr, D3D_OK);

    hr = IDirect3DDevice9_SetPixelShaderConstantB(device, 0, (BOOL*)zero, 16);
    ok(hr == D3D_OK, "Clear failed, got %08x, expected %08x\n", hr, D3D_OK);

    hr = IDirect3DDevice9_SetVertexShaderConstantI(device, 0, (INT*)zero, 16);
    ok(hr == D3D_OK, "Clear failed, got %08x, expected %08x\n", hr, D3D_OK);

    hr = IDirect3DDevice9_SetPixelShaderConstantI(device, 0, (INT*)zero, 16);
    ok(hr == D3D_OK, "Clear failed, got %08x, expected %08x\n", hr, D3D_OK);
}

static UINT registerset_compare(IDirect3DDevice9 *device, BOOL is_vs, D3DXREGISTER_SET regset,
        UINT start, UINT in_count, const DWORD *expected)
{
    DWORD ret[1024] = {0};
    HRESULT hr;
    UINT count = 1024, i, err = 0;

    memset(ret, 0xde, 4096);

    /* get shader constants */
    switch (regset)
    {
        case D3DXRS_BOOL:
            count = 16;
            if (is_vs) hr = IDirect3DDevice9_GetVertexShaderConstantB(device, 0, (BOOL*)ret, 16);
            else hr = IDirect3DDevice9_GetPixelShaderConstantB(device, 0, (BOOL*)ret, 16);
            ok(hr == D3D_OK, "Get*ShaderConstantB failed, got %08x\n", hr);
            break;

        case D3DXRS_INT4:
            count = 256;
            if (is_vs) hr = IDirect3DDevice9_GetVertexShaderConstantI(device, 0, (INT*)ret, 16);
            else hr = IDirect3DDevice9_GetPixelShaderConstantI(device, 0, (INT*)ret, 16);
            ok(hr == D3D_OK, "Get*ShaderConstantI failed, got %08x\n", hr);
            break;

        case D3DXRS_FLOAT4:
            if (is_vs) hr = IDirect3DDevice9_GetVertexShaderConstantF(device, 0, (FLOAT*)ret, 256);
            else
            {
                count = 896;
                hr = IDirect3DDevice9_GetPixelShaderConstantF(device, 0, (FLOAT*)ret, 224);
            }
            ok(hr == D3D_OK, "Get*ShaderConstantF failed, got %08x\n", hr);
            break;

        default:
            ok(0, "This should not happen!\n");
            break;
    }

    /* compare shader constants */
    for (i = 0; i < count; ++i)
    {
        DWORD value = 0xdededede;
        if (i >= start && i < start + in_count) value = expected[i - start];

        ok(ret[i] == value, "Get*ShaderConstant failed, %u got 0x%x(%f) expected 0x%x(%f)\n", i,
                ret[i], ((FLOAT *)ret)[i], value, *((FLOAT *)&value));
        if (ret[i] != value) err++;
    }

    return err;
}

static UINT registerset_compare_all(IDirect3DDevice9 *device, BOOL is_vs, D3DXREGISTER_SET regset,
        UINT start, UINT in_count, const DWORD *expected)
{
    D3DXREGISTER_SET regsets[] = {D3DXRS_BOOL, D3DXRS_INT4, D3DXRS_FLOAT4};
    UINT err = 0, i;

    for (i = 0; i < ARRAY_SIZE(regsets); i++)
    {
        if (regset == regsets[i])
            err += registerset_compare(device, is_vs, regset, start, in_count, expected);
        else
            err += registerset_compare(device, is_vs, regsets[i], 0, 0, NULL);

        err += registerset_compare(device, !is_vs, regsets[i], 0, 0, NULL);
    }

    return err;
}

static HRESULT registerset_apply(ID3DXConstantTable *ctable, IDirect3DDevice9 *device, D3DXHANDLE constant,
        UINT index, DWORD count, enum Type type)
{
    const DWORD *in = registerset_test_input[index];
    const D3DXMATRIX *inp[REGISTER_OUTPUT_SIZE / 16];
    unsigned int i;

    /* overlap, to see the difference between Array and PointerArray */
    for (i = 0; i < REGISTER_OUTPUT_SIZE / 16; i++)
    {
        inp[i] = (D3DXMATRIX *)&in[i * 15];
    }

    switch (type)
    {
        case SetInt:
            return ID3DXConstantTable_SetInt(ctable, device, constant, *((INT *)in));
        case SetFloat:
            return ID3DXConstantTable_SetFloat(ctable, device, constant, *((FLOAT *)in));
        case SetBool:
            return ID3DXConstantTable_SetBool(ctable, device, constant, *((BOOL *)in));
        case SetIntArray:
            return ID3DXConstantTable_SetIntArray(ctable, device, constant, (INT *)in, count);
        case SetBoolArray:
            return ID3DXConstantTable_SetBoolArray(ctable, device, constant, (BOOL *)in, count);
        case SetFloatArray:
            return ID3DXConstantTable_SetFloatArray(ctable, device, constant, (FLOAT *)in, count);
        case SetMatrix:
            return ID3DXConstantTable_SetMatrix(ctable, device, constant, (D3DXMATRIX *)in);
        case SetMatrixTranspose:
            return ID3DXConstantTable_SetMatrixTranspose(ctable, device, constant, (D3DXMATRIX *)in);
        case SetMatrixArray:
            return ID3DXConstantTable_SetMatrixArray(ctable, device, constant, (D3DXMATRIX *)in, count);
        case SetMatrixTransposeArray:
            return ID3DXConstantTable_SetMatrixTransposeArray(ctable, device, constant, (D3DXMATRIX *)in, count);
        case SetVector:
            return ID3DXConstantTable_SetVector(ctable, device, constant, (D3DXVECTOR4 *)in);
        case SetVectorArray:
            return ID3DXConstantTable_SetVectorArray(ctable, device, constant, (D3DXVECTOR4 *)in, count);
        case SetValue:
            return ID3DXConstantTable_SetValue(ctable, device, constant, in, count);
        case SetMatrixPointerArray:
            return ID3DXConstantTable_SetMatrixPointerArray(ctable, device, constant, inp, count);
        case SetMatrixTransposePointerArray:
            return ID3DXConstantTable_SetMatrixTransposePointerArray(ctable, device, constant, inp, count);
    }

    ok(0, "This should not happen!\n");
    return D3D_OK;
}

static void test_registerset(void)
{
    UINT k;
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;
    ULONG count;
    D3DCAPS9 caps;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    if (!(d3d = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion < D3DVS_VERSION(3, 0)
            || caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
    {
        skip("Skipping: Test requires VS >= 3 and PS >= 3.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    for (k = 0; k < ARRAY_SIZE(registerset_data); ++k)
    {
        const char *tablename = registerset_data[k].name;
        const char *name = registerset_data[k].var;
        ID3DXConstantTable *ctable;
        D3DXCONSTANTTABLE_DESC tdesc;
        D3DXHANDLE constant;
        UINT i;
        BOOL is_vs;
        DWORD *ctab;

        hr = D3DXGetShaderConstantTable(registerset_data[k].blob, &ctable);
        ok(hr == D3D_OK, "D3DXGetShaderConstantTable \"%s\" failed, got %08x, expected %08x\n", tablename, hr, D3D_OK);

        hr = ID3DXConstantTable_GetDesc(ctable, &tdesc);
        ok(hr == D3D_OK, "GetDesc \"%s\" failed, got %08x, expected %08x\n", tablename, hr, D3D_OK);

        ctab = ID3DXConstantTable_GetBufferPointer(ctable);
        ok(ctab[0] == registerset_data[k].blob[3], "ID3DXConstantTable_GetBufferPointer failed\n");

        is_vs = (tdesc.Version & 0xFFFF0000) == 0xFFFE0000;

        for (i = 0; i < registerset_data[k].constant_count; ++i)
        {
            const char *fullname = registerset_data[k].constants[i].fullname;
            const D3DXCONSTANT_DESC *expected_desc = &registerset_data[k].constants[i].desc;
            D3DXCONSTANT_DESC desc;
            UINT nr = 0;
            UINT ctaboffset = registerset_data[k].constants[i].ctaboffset;

            constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, fullname);
            ok(constant != NULL, "GetConstantByName \"%s\" failed\n", fullname);

            hr = ID3DXConstantTable_GetConstantDesc(ctable, constant, &desc, &nr);
            ok(hr == D3D_OK, "GetConstantDesc \"%s\" failed, got %08x, expected %08x\n", fullname, hr, D3D_OK);

            ok(!strcmp(expected_desc->Name, desc.Name), "GetConstantDesc \"%s\" failed, got \"%s\", expected \"%s\"\n",
                    fullname, desc.Name, expected_desc->Name);
            ok(expected_desc->RegisterSet == desc.RegisterSet, "GetConstantDesc \"%s\" failed, got %#x, expected %#x\n",
                    fullname, desc.RegisterSet, expected_desc->RegisterSet);
            ok(expected_desc->RegisterIndex == desc.RegisterIndex,
                    "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                    fullname, desc.RegisterIndex, expected_desc->RegisterIndex);
            ok(expected_desc->RegisterCount == desc.RegisterCount,
                    "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                    fullname, desc.RegisterCount, expected_desc->RegisterCount);
            ok(expected_desc->Class == desc.Class, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                    fullname, desc.Class, expected_desc->Class);
            ok(expected_desc->Type == desc.Type, "GetConstantDesc \"%s\" failed, got %#x, expected %#x\n",
                    fullname, desc.Type, expected_desc->Type);
            ok(expected_desc->Rows == desc.Rows, "GetConstantDesc \"%s\" failed, got %#x, expected %#x\n",
                    fullname, desc.Rows, expected_desc->Rows);
            ok(expected_desc->Columns == desc.Columns, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                    fullname, desc.Columns, expected_desc->Columns);
            ok(expected_desc->Elements == desc.Elements, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                    fullname, desc.Elements, expected_desc->Elements);
            ok(expected_desc->StructMembers == desc.StructMembers,
                    "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                    fullname, desc.StructMembers, expected_desc->StructMembers);
            ok(expected_desc->Bytes == desc.Bytes, "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                    fullname, desc.Bytes, expected_desc->Bytes);
            if (ctaboffset)
            {
                ok(ctaboffset == (DWORD *)desc.DefaultValue - ctab,
                        "GetConstantDesc \"%s\" failed, got %u, expected %u\n",
                        fullname, (UINT)((DWORD *)desc.DefaultValue - ctab), ctaboffset);
            }
        }

        constant = ID3DXConstantTable_GetConstantByName(ctable, NULL, name);
        ok(constant != NULL, "GetConstantByName \"%s\" \"%s\" failed\n", tablename, name);

        for (i = 0; i < registerset_data[k].test_count; ++i)
        {
            const struct registerset_test *test = &registerset_data[k].tests[i];
            UINT ret;

            registerset_clear(device);

            hr = registerset_apply(ctable, device, constant, test->in_index, test->in_count_min, test->type);
            ok(hr == D3D_OK, "Set* \"%s\" index %u, count %u failed, got %x, expected %x\n", tablename, i,
                    test->in_count_min, hr, D3D_OK);

            ret = registerset_compare_all(device, is_vs, registerset_data[k].regset,
                    registerset_data[k].start, test->out_count, test->out);
            ok(ret == 0, "Get*ShaderConstant \"%s\" index %u, count %u failed\n", tablename, i, test->in_count_min);

            if (test->in_count_max > test->in_count_min)
            {
                registerset_clear(device);

                hr = registerset_apply(ctable, device, constant, test->in_index, test->in_count_max, test->type);
                ok(hr == D3D_OK, "Set* \"%s\" index %u, count %u failed, got %x, expected %x\n", tablename, i,
                        test->in_count_max, hr, D3D_OK);

                ret = registerset_compare_all(device, is_vs, registerset_data[k].regset,
                        registerset_data[k].start, test->out_count, test->out);
                ok(ret == 0, "Get*ShaderConstant \"%s\" index %u, count %u failed\n", tablename, i, test->in_count_max);
            }
        }

        count = ID3DXConstantTable_Release(ctable);
        ok(count == 0, "Release \"%s\" failed, got %u, expected %u\n", tablename, count, 0);
    }

    /* Release resources */
    count = IDirect3DDevice9_Release(device);
    ok(count == 0, "The Direct3D device reference count was %u, should be 0\n", count);

    count = IDirect3D9_Release(d3d);
    ok(count == 0, "The Direct3D object reference count was %u, should be 0\n", count);

    if (wnd) DestroyWindow(wnd);
}

/*
 * For D3DXRS_INT4 (int_count, ints[]):
 *      Native seems to just set the following shader blob up to the register count, which in bad cases is up
 *      to 4 times larger than the actual correct value. This explanes where the "bogus" values for these cases
 *      come from. Somehow they forgot that the registers are INT4 and not INT.
 */
static const struct
{
    const char *name;
    const DWORD *blob;
    unsigned int float_count;
    unsigned int int_count;
    unsigned int bool_count;
    const DWORD floats[1024];
    const DWORD ints[256];
    const DWORD bools[16];
}
registerset_defaults_data[] =
{
    {"scalar", registerset_blob_scalar, 12, 4, 1,
        {0x40a33333, 0x00000000, 0x00000000, 0x00000000, 0x41300000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000},
        {0x00000008, 0x00000000, 0x00000001},
        {0xffffffff}},
    {"scalar array", registerset_blob_scalar_array, 24, 8, 2,
        {0x40466666, 0x00000000, 0x00000000, 0x00000000, 0x404ccccd, 0x00000000, 0x00000000, 0x00000000,
        0x41600000, 0x00000000, 0x00000000, 0x00000000, 0x41700000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000},
        {0x00000020, 0x00000000, 0x00000001, 0x00000000, 0x00000021, 0x00000000, 0x00000001, 0x00000000},
        {0xffffffff, 0x00000000}},
    {"vector", registerset_blob_vector, 12, 12, 3,
        {0x40a33333, 0x40a66666, 0x40a9999a, 0x00000000, 0x41300000, 0x42aa0000, 0x42780000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000},
        {0x00000007, 0x00000008, 0x00000009, 0x00000000, 0x00666e76, 0x41300000, 0x42aa0000, 0x42780000,
        0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369},
        {0xffffffff, 0x00000000, 0xffffffff}},
    {"vector array", registerset_blob_vector_array, 24, 24, 6,
        {0x425c6666, 0x425ccccd, 0x425d3333, 0x00000000, 0x425d999a, 0x425e0000, 0x425e6666, 0x00000000,
        0x43020000, 0x430c0000, 0x43160000, 0x00000000, 0x43200000, 0x432a0000, 0x43340000, 0x00000000,
        0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000},
        {0x00000046, 0x00000050, 0x0000005a, 0x00000000, 0x00000064, 0x0000006e, 0x00000078, 0x00000000,
        0x666e6176, 0xababab00, 0x00020001, 0x00030001, 0x00000002, 0x00000000, 0x43020000, 0x430c0000,
        0x43160000, 0x00000000, 0x43200000, 0x432a0000, 0x43340000, 0x00000000, 0x335f7376, 0x4d00305f},
        {0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff}},
    {"column", registerset_blob_column, 24, 24, 6,
        {0x4171999a, 0x4174cccd, 0x41780000, 0x00000000, 0x41733333, 0x41766666, 0x4179999a, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x41f00000, 0x42040000, 0x42080000, 0x00000000, 0x41f80000, 0x42000000, 0x42100000, 0x00000000},
        {0x00000004, 0x00000006, 0x00000008, 0x00000000, 0x00000005, 0x00000007, 0x00000009, 0x00000000,
        0x00666e63, 0x41f00000, 0x42040000, 0x42080000, 0x00000000, 0x41f80000, 0x42000000, 0x42100000,
        0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c},
        {0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff}},
    {"column array", registerset_blob_column_array, 48, 48, 12,
        {0x3f8ccccd, 0x40533333, 0x40b00000, 0x00000000, 0x400ccccd, 0x408ccccd, 0x40d33333, 0x00000000,
        0x40f66666, 0x411e6666, 0x3fa66666, 0x00000000, 0x410ccccd, 0x3f99999a, 0x3fb33333, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000,
        0x43960000, 0x43978000, 0x43980000, 0x00000000, 0x43968000, 0x43970000, 0x43990000, 0x00000000,
        0x43af0000, 0x44238000, 0x43a30000, 0x00000000, 0x43b68000, 0x44190000, 0x4479c000, 0x00000000},
        {0x0000000e, 0x00000010, 0x00000012, 0x00000000, 0x0000000f, 0x00000047, 0x00000013, 0x00000000,
        0x00000037, 0x00000060, 0x00000061, 0x00000000, 0x0000003f, 0x00000060, 0x0000000d, 0x00000000,
        0x666e6163, 0xababab00, 0x00020003, 0x00020003, 0x00000002, 0x00000000, 0x43960000, 0x43978000,
        0x43980000, 0x00000000, 0x43968000, 0x43970000, 0x43990000, 0x00000000, 0x43af0000, 0x44238000,
        0x43a30000, 0x00000000, 0x43b68000, 0x44190000, 0x4479c000, 0x00000000, 0x335f7376, 0x4d00305f,
        0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970},
        {0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000,
        0xffffffff, 0xffffffff, 0x00000000, 0xffffffff}},
    {"row", registerset_blob_row, 36, 24, 6,
        {0x42be3333, 0x42be6666, 0x00000000, 0x00000000, 0x42be999a, 0x42becccd, 0x00000000, 0x00000000,
        0x42bf0000, 0x42bf3333, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x42140000, 0x41500000, 0x00000000, 0x00000000, 0x42c40000, 0x42580000, 0x00000000, 0x00000000,
        0x429a0000, 0x42100000, 0x00000000, 0x00000000},
        {0x00000050, 0x00000051, 0x00000001, 0x00000000, 0x00000052, 0x00000053, 0x00000001, 0x00000000,
        0x00000054, 0x00000055, 0x00000001, 0x00000000, 0x00666e72, 0x42140000, 0x41500000, 0x00000000,
        0x00000000, 0x42c40000, 0x42580000, 0x00000000, 0x00000000, 0x429a0000, 0x42100000, 0x00000000},
        {0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff}},
    {"row array", registerset_blob_row_array, 72, 48, 12,
        {0x3fc00000, 0x40333333, 0x00000000, 0x00000000, 0x40533333, 0x409ccccd, 0x00000000, 0x00000000,
        0x40bccccd, 0x40d9999a, 0x00000000, 0x00000000, 0x40fccccd, 0x41080000, 0x00000000, 0x00000000,
        0x41166666, 0x3fa66666, 0x00000000, 0x00000000, 0x3f99999a, 0x3f8ccccd, 0x00000000, 0x00000000,
        0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000,
        0x420c0000, 0x42200000, 0x00000000, 0x00000000, 0x42700000, 0x42a00000, 0x00000000, 0x00000000,
        0x428c0000, 0x42600000, 0x00000000, 0x00000000, 0x42140000, 0x41500000, 0x00000000, 0x00000000,
        0x42c40000, 0x42580000, 0x00000000, 0x00000000, 0x429a0000, 0x42100000, 0x00000000, 0x00000000},
        {0x00000004, 0x00000005, 0x00000001, 0x00000000, 0x00000006, 0x00000001, 0x00000001, 0x00000000,
        0x00000008, 0x00000001, 0x00000001, 0x00000000, 0x00000005, 0x00000003, 0x00000001, 0x00000000,
        0x00000009, 0x00000006, 0x00000001, 0x00000000, 0x00000007, 0x00000003, 0x00000001, 0x00000000,
        0x666e6172, 0xababab00, 0x00020002, 0x00020003, 0x00000002, 0x00000000, 0x420c0000, 0x42200000,
        0x00000000, 0x00000000, 0x42700000, 0x42a00000, 0x00000000, 0x00000000, 0x428c0000, 0x42600000,
        0x00000000, 0x00000000, 0x42140000, 0x41500000, 0x00000000, 0x00000000, 0x42c40000, 0x42580000},
        {0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
        0xffffffff, 0xffffffff, 0x00000000, 0xffffffff}},
    {"struct", registerset_blob_struct, 24, 16, 4,
        {0x3f8ccccd, 0x00000000, 0x00000000, 0x00000000, 0x400ccccd, 0x40533333, 0x408ccccd, 0x00000000,
        0x41f80000, 0x00000000, 0x00000000, 0x00000000, 0x42000000, 0x42040000, 0x42080000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000},
        {0x0000000b, 0x00000000, 0x00000001, 0x00000000, 0x0000000c, 0x0000000d, 0x0000000e, 0x00000000,
        0x00666e73, 0x7600666e, 0xab00666e, 0x00000204, 0x0000019c, 0x00000207, 0x000001b0, 0x00000005},
        {0xffffffff, 0xffffffff, 0x00000000, 0xffffffff}},
    {"struct array", registerset_blob_struct_array, 48, 32, 8,
        {0x3f8ccccd, 0x00000000, 0x00000000, 0x00000000, 0x40066666, 0x40466666, 0x40833333, 0x00000000,
        0x40a33333, 0x00000000, 0x00000000, 0x00000000, 0x40c33333, 0x40e33333, 0x4101999a, 0x00000000,
        0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x422c0000, 0x42300000, 0x00000000,
        0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x42280000, 0x422c0000, 0x42300000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000},
        {0x00000015, 0x00000000, 0x00000001, 0x00000000, 0x00000016, 0x00000017, 0x00000018, 0x00000000,
        0x00000019, 0x00000000, 0x00000001, 0x00000000, 0x0000001a, 0x0000001b, 0x0000001c, 0x00000000,
        0x666e6173, 0x00666e00, 0x00666e76, 0x00000275, 0x000001ec, 0x00000278, 0x00000200, 0x00000005,
        0x00040001, 0x00020002, 0x0000027c, 0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
        {0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff}},
    {"struct struct", registerset_blob_struct_struct, 48, 24, 6,
        {0x3f8ccccd, 0x00000000, 0x00000000, 0x00000000, 0x40066666, 0x00000000, 0x00000000, 0x00000000,
        0x40466666, 0x40833333, 0x40a33333, 0x00000000, 0x40c33333, 0x00000000, 0x00000000, 0x00000000,
        0x42240000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x422c0000, 0x42300000, 0x42240000, 0x00000000, 0x42280000, 0x00000000, 0x00000000, 0x00000000,
        0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
        {0x00000047, 0x00000000, 0x00000001, 0x00000000, 0x00000048, 0x00000000, 0x00000001, 0x00000000,
        0x00000049, 0x0000004a, 0x0000004b, 0x00000000, 0x0000004c, 0x00000000, 0x00000001, 0x00000000,
        0x666e7373, 0x00666e00, 0x00666e76, 0x00000321, 0x0000026c, 0x00000324, 0x00000280, 0x00000005},
        {0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000}},
    {"int ran", registerset_blob_special_int, 0, 36, 0,
        {0x00000000},
        {0x00000004, 0x00000005, 0x00000001, 0x00000000, 0x00000006, 0x00000001, 0x00000001, 0x00000000,
        0x00000008, 0x00000001, 0x00000001, 0x00000000, 0x00000002, 0x00000003, 0x00000001, 0x00000000,
        0x00000004, 0x00000007, 0x00000001, 0x00000000, 0x00000009, 0x00000001, 0x00000001, 0x00000000,
        0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461,
        0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932},
        {0x00000000}},
    /* DefaultValue = NULL */
    {"big vector", registerset_blob_bigvec},
    {"matrix column clamp", registerset_blob_matrix_column_clamp, 12, 0, 0,
        {0x3f8ccccd, 0x40b00000, 0x411e6666, 0x3fc00000, 0x400ccccd, 0x40d33333, 0x3f99999a, 0x3fcccccd,
        0x420551ec, 0x00000000, 0x00000000, 0x00000000},
        {0x00000000},
        {0x00000000}},
};

static void test_registerset_defaults(void)
{
    UINT k;
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;
    ULONG count;
    D3DCAPS9 caps;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    if (!(d3d = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion < D3DVS_VERSION(3, 0)
            || caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
    {
        skip("Skipping: Test requires VS >= 3 and PS >= 3.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    for (k = 0; k < ARRAY_SIZE(registerset_defaults_data); ++k)
    {
        const char *tablename = registerset_defaults_data[k].name;
        ID3DXConstantTable *ctable;
        D3DXCONSTANTTABLE_DESC tdesc;
        BOOL is_vs;
        UINT ret;

        hr = D3DXGetShaderConstantTable(registerset_defaults_data[k].blob, &ctable);
        ok(hr == D3D_OK, "D3DXGetShaderConstantTable \"%s\" failed, got %08x, expected %08x\n", tablename, hr, D3D_OK);

        hr = ID3DXConstantTable_GetDesc(ctable, &tdesc);
        ok(hr == D3D_OK, "GetDesc \"%s\" failed, got %08x, expected %08x\n", tablename, hr, D3D_OK);

        is_vs = (tdesc.Version & 0xFFFF0000) == 0xFFFE0000;

        registerset_clear(device);

        hr = ID3DXConstantTable_SetDefaults(ctable, device);
        ok(hr == D3D_OK, "SetDefaults \"%s\" failed, got %08x, expected %08x\n", tablename, hr, D3D_OK);

        ret = registerset_compare(device, is_vs, D3DXRS_FLOAT4, 0, registerset_defaults_data[k].float_count,
                registerset_defaults_data[k].floats);
        ok(ret == 0, "Get*ShaderConstantF \"%s\" failed\n", tablename);

        ret = registerset_compare(device, is_vs, D3DXRS_INT4, 0, registerset_defaults_data[k].int_count,
                registerset_defaults_data[k].ints);
        ok(ret == 0, "Get*ShaderConstantI \"%s\" failed\n", tablename);

        ret = registerset_compare(device, is_vs, D3DXRS_BOOL, 0, registerset_defaults_data[k].bool_count,
                registerset_defaults_data[k].bools);
        ok(ret == 0, "Get*ShaderConstantB \"%s\" failed\n", tablename);

        count = ID3DXConstantTable_Release(ctable);
        ok(count == 0, "Release \"%s\" failed, got %u, expected %u\n", tablename, count, 0);
    }

    /* Release resources */
    count = IDirect3DDevice9_Release(device);
    ok(count == 0, "The Direct3D device reference count was %u, should be 0\n", count);

    count = IDirect3D9_Release(d3d);
    ok(count == 0, "The Direct3D object reference count was %u, should be 0\n", count);

    if (wnd) DestroyWindow(wnd);
}

static void test_shader_semantics(void)
{
    static const DWORD invalid_1[] =
    {
        0x00000200
    },
    invalid_2[] =
    {
        0xfffe0400
    },
    invalid_3[] =
    {
        0xfffe0000
    },
    vs_1_1[] =
    {
        0xfffe0101,                         /* vs_1_1 */
        0x0000001f, 0x80000000, 0x900f0000, /* dcl_position v0 */
        0x0000001f, 0x80000003, 0x900f0001, /* dcl_normal v1 */
        0x0000001f, 0x8001000a, 0x900f0002, /* dcl_color1 v2 */
        0x0000001f, 0x80000005, 0x900f0003, /* dcl_texcoord0 v3 */
        0x00000001, 0xc00f0000, 0x90e40000, /* mov oPos, v0 */
        0x00000001, 0xd00f0001, 0x90e40002, /* mov oD1, v2 */
        0x00000001, 0xe0070000, 0x90e40001, /* mov oT0.xyz, v1 */
        0x00000001, 0xc00f0001, 0x90ff0002, /* mov oFog, v2.w */
        0x00000001, 0xc00f0002, 0x90ff0001, /* mov oPts, v1.w */
        0x0000ffff
    },
    vs_2_0[] =
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
    },
    vs_3_0[] =
    {
        0xfffe0300,                         /* vs_3_0 */
        0x0002fffe, 0x0200000f, 0x00000000, /* comment */
        0x0200001f, 0x80000000, 0x900f0000, /* dcl_position v0 */
        0x0200001f, 0x80000003, 0x900f0001, /* dcl_normal v1 */
        0x0200001f, 0x8001000a, 0x900f0002, /* dcl_color1 v2 */
        0x0200001f, 0x80000005, 0x900f0003, /* dcl_texcoord0 v3 */
        0x0200001f, 0x80000000, 0xe00f0000, /* dcl_position o0 */
        0x0200001f, 0x8001000a, 0xe00f0001, /* dcl_color1 o1 */
        0x0200001f, 0x80000005, 0xe00f0002, /* dcl_texcoord0 o2 */
        0x0200001f, 0x8000000b, 0xe00f0003, /* dcl_fog o3 */
        0x0200001f, 0x80000004, 0xe00f0004, /* dcl_psize o4 */
        0x02000001, 0xe00f0000, 0x90e40000, /* mov o0, v0 */
        0x02000001, 0xe00f0001, 0x90e40002, /* mov o1, v2 */
        0x02000001, 0xe0070002, 0x90e40003, /* mov o2.xyz, v3 */
        0x02000001, 0xe00f0003, 0x90ff0002, /* mov o3, v2.w */
        0x02000001, 0xe00f0004, 0x90ff0001, /* mov o4, v1.w */
        0x0000ffff
    },
    ps_1_1[] =
    {
        0xffff0101,                                     /* ps_1_1 */
        0x00000042, 0xb00f0000,                         /* tex t0 */
        0x00000002, 0x800f0000, 0x90e40000, 0xb0e40000, /* add r0, v0, t0 */
        0x0000ffff
    },
    ps_2_0[] =
    {
        0xffff0200,                         /* ps_2_0 */
        0x0200001f, 0x80000000, 0x900f0000, /* dcl v0 */
        0x0200001f, 0x80000000, 0xb00f0000, /* dcl t0 */
        0x02000001, 0x800f0800, 0x90e40000, /* mov oC0, v0 */
        0x0000ffff
    },
    ps_3_0[] =
    {
        0xffff0300,                                                             /* ps_3_0 */
        0x0200001f, 0x8001000a, 0x900f0000,                                     /* dcl_color1 v0 */
        0x0200001f, 0x80000003, 0x900f0001,                                     /* dcl_normal v1 */
        0x0200001f, 0x80000005, 0x900f0002,                                     /* dcl_texcoord0 v2 */
        0x0200001f, 0x8000000b, 0x900f0003,                                     /* dcl_fog v3 */
        0x0200001f, 0x80000000, 0x90031000,                                     /* dcl vPos.xy */
        0x0200001f, 0x80000000, 0x900f1001,                                     /* dcl vFace */
        0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.0, 0.0, 0.0, 0.0 */
        0x02000001, 0x800f0000, 0x90e40000,                                     /* mov r0, v0 */
        0x03000002, 0x800f0800, 0x80e40000, 0x90e40003,                         /* add oC0, r0, v3 */
        0x02000001, 0x800f0001, 0x90e40001,                                     /* mov r1, v1 */
        0x03000005, 0x800f0801, 0x80e40001, 0x90e40002,                         /* mul oC1, r1, v2 */
        0x02000001, 0x800f0802, 0x90441000,                                     /* mov oC2, vPos.xyxy */
        0x04000058, 0x800f0803, 0x90e41001, 0x90e40000, 0xa0e40000,             /* cmp oC3, vFace, v0, c0 */
        0x02000001, 0x900f0800, 0x90ff0001,                                     /* mov oDepth, v1.w */
        0x0000ffff
    };
    static const struct
    {
        const DWORD *shader;
        D3DXSEMANTIC expected_input[MAXD3DDECLLENGTH];
        D3DXSEMANTIC expected_output[MAXD3DDECLLENGTH];
    }
    tests[] =
    {
        {vs_1_1, {{0, 0}, {3, 0}, {10, 1}, {5, 0}, {~0, ~0}}, {{5, 0}, {10, 1}, {0, 0}, {11, 0}, {4, 0}, {~0, ~0}}},
        {vs_2_0, {{0, 0}, {3, 0}, {10, 1}, {5, 0}, {~0, ~0}}, {{5, 0}, {10, 1}, {0, 0}, {11, 0}, {4, 0}, {~0, ~0}}},
        {vs_3_0, {{0, 0}, {3, 0}, {10, 1}, {5, 0}, {~0, ~0}}, {{0, 0}, {10, 1}, {5, 0}, {11, 0}, {4, 0}, {~0, ~0}}},
        {ps_1_1, {{5, 0}, {10, 0}, {~0, ~0}}, {{10, 0}, {~0, ~0}}},
        {ps_2_0, {{10, 0}, {5, 0}, {~0, ~0}}, {{10, 0}, {~0, ~0}}},
        {ps_3_0, {{10, 1}, {3,0}, {5, 0}, {11, 0}, {~0, ~0}}, {{10, 0}, {10, 1}, {10, 2}, {10, 3}, {12, 0}, {~0, ~0}}},
    };
    D3DXSEMANTIC semantics[MAXD3DDECLLENGTH];
    unsigned int count, count2;
    unsigned int i, j;
    HRESULT hr;

    hr = D3DXGetShaderInputSemantics(invalid_1, NULL, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "Unexpected hr %#x.\n", hr);
    hr = D3DXGetShaderInputSemantics(invalid_2, NULL, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
    hr = D3DXGetShaderInputSemantics(invalid_3, NULL, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);

    hr = D3DXGetShaderInputSemantics(vs_1_1, NULL, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
    hr = D3DXGetShaderInputSemantics(vs_1_1, semantics, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const DWORD *shader = tests[i].shader;

        hr = D3DXGetShaderInputSemantics(shader, NULL, &count);
        ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
        hr = D3DXGetShaderInputSemantics(shader, semantics, &count2);
        ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
        ok(count == count2, "Semantics count %u differs from previous count %u.\n", count2, count);
        for (j = 0; j < count; ++j)
        {
            ok(semantics[j].Usage == tests[i].expected_input[j].Usage
                    && semantics[j].UsageIndex == tests[i].expected_input[j].UsageIndex,
                    "Unexpected semantic %u, %u, test %u, idx %u.\n",
                    semantics[j].Usage, semantics[j].UsageIndex, i, j);
        }
        ok(tests[i].expected_input[j].Usage == ~0 && tests[i].expected_input[j].UsageIndex == ~0,
                "Unexpected semantics count %u.\n", count);
        hr = D3DXGetShaderOutputSemantics(shader, NULL, &count);
        ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
        hr = D3DXGetShaderOutputSemantics(shader, semantics, &count2);
        ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
        ok(count == count2, "Semantics count %u differs from previous count %u.\n", count2, count);
        for (j = 0; j < count; ++j)
        {
            ok(semantics[j].Usage == tests[i].expected_output[j].Usage
                    && semantics[j].UsageIndex == tests[i].expected_output[j].UsageIndex,
                    "Unexpected semantic %u, %u, test %u, idx %u.\n",
                    semantics[j].Usage, semantics[j].UsageIndex, i, j);
        }
        ok(tests[i].expected_output[j].Usage == ~0 && tests[i].expected_output[j].UsageIndex == ~0,
                "Unexpected semantics count %u.\n", count);
    }
}

static void test_fragment_linker(void)
{
    ID3DXFragmentLinker *linker;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, NULL, NULL, NULL, NULL);
    if (!(d3d = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create a D3D object.\n");
        DestroyWindow(window);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create a D3D device, hr %#x.\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = D3DXCreateFragmentLinker(device, 1024, &linker);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
    ok(!!linker, "Unexpected linker %p.\n", linker);
    linker->lpVtbl->Release(linker);

    hr = D3DXCreateFragmentLinkerEx(device, 1024, 0, &linker);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
    ok(!!linker, "Unexpected linker %p.\n", linker);
    linker->lpVtbl->Release(linker);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDirect3D9_Release(d3d);
    ok(!refcount, "The D3D object has %u references left.\n", refcount);
    DestroyWindow(window);
}

static const DWORD ps_tex[] = {
    0xffff0103,                                                             /* ps_1_3                       */
    0x00000042, 0xb00f0000,                                                 /* tex t0                       */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD ps_texld_1_4[] = {
    0xffff0104,                                                             /* ps_1_4                       */
    0x00000042, 0xb00f0000, 0xa0e40000,                                     /* texld t0, c0                 */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD ps_texld_2_0[] = {
    0xffff0200,                                                             /* ps_2_0                       */
    0x00000042, 0xb00f0000, 0xa0e40000, 0xa0e40001,                         /* texld t0, c0, c1             */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD ps_texcoord[] = {
    0xffff0103,                                                             /* ps_1_4                       */
    0x00000040, 0xb00f0000,                                                 /* texcoord t0                  */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD ps_texcrd[] = {
    0xffff0104,                                                             /* ps_2_0                       */
    0x00000040, 0xb00f0000, 0xa0e40000,                                     /* texcrd t0, c0                */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD ps_sincos_2_0[] = {
    0xffff0200,                                                             /* ps_2_0                       */
    0x00000025, 0xb00f0000, 0xa0e40000, 0xa0e40001, 0xa0e40002,             /* sincos t0, c0, c1, c2        */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD ps_sincos_3_0[] = {
    0xffff0300,                                                             /* ps_3_0                       */
    0x00000025, 0xb00f0000, 0xa0e40000,                                     /* sincos t0, c0                */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD vs_sincos_2_0[] = {
    0xfffe0200,                                                             /* vs_2_0                       */
    0x00000025, 0xb00f0000, 0xa0e40000, 0xa0e40001, 0xa0e40002,             /* sincos a0, c0, c1, c2        */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static const DWORD vs_sincos_3_0[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x00000025, 0xb00f0000, 0xa0e40000,                                     /* sincos a0, c0                */
    0x00000000,                                                             /* nop                          */
    0x0000ffff};

static void test_disassemble_shader(void)
{
    static const char disasm_vs[] = "    vs_1_1\n"
                                    "    dcl_position v0\n"
                                    "    dp4 oPos.x, v0, c0\n"
                                    "    dp4 oPos.y, v0, c1\n"
                                    "    dp4 oPos.z, v0, c2\n"
                                    "    dp4 oPos.w, v0, c3\n";
    static const char disasm_ps[] = "    ps_1_1\n"
                                    "    def c1, 1, 0, 0, 0\n"
                                    "    tex t0\n"
                                    "    dp3 r0, c1, c0\n"
                                    "    mul r0, v0, r0\n"
                                    "    mul r0, t0, r0\n";
    static const char disasm_ps_tex[] =        "    ps_1_3\n"
                                               "    tex t0\n"
                                               "    nop\n";
    static const char disasm_ps_texld_1_4[] =  "    ps_1_4\n"
                                               "    texld t0, c0\n"
                                               "    nop\n";
    static const char disasm_ps_texld_2_0[] =  "    ps_2_0\n"
                                               "    texld t0, c0, c1\n"
                                               "    nop\n";
    static const char disasm_ps_texcoord[] =   "    ps_1_3\n"
                                               "    texcoord t0\n"
                                               "    nop\n";
    static const char disasm_ps_texcrd[] =     "    ps_1_4\n"
                                               "    texcrd t0, c0\n"
                                               "    nop\n";
    static const char disasm_ps_sincos_2_0[] = "    ps_2_0\n"
                                               "    sincos t0, c0, c1, c2\n"
                                               "    nop\n";
    static const char disasm_ps_sincos_3_0[] = "    ps_3_0\n"
                                               "    sincos t0, c0\n"
                                               "    nop\n";
    static const char disasm_vs_sincos_2_0[] = "    vs_2_0\n"
                                               "    sincos a0, c0, c1, c2\n"
                                               "    nop\n";
    static const char disasm_vs_sincos_3_0[] = "    vs_3_0\n"
                                               "    sincos a0, c0\n"
                                               "    nop\n";
    ID3DXBuffer *disassembly;
    HRESULT ret;
    char *ptr;

    /* Check wrong parameters */
    ret = D3DXDisassembleShader(NULL, FALSE, NULL, NULL);
    ok(ret == D3DERR_INVALIDCALL, "Returned %#x, expected %#x\n", ret, D3DERR_INVALIDCALL);
    ret = D3DXDisassembleShader(NULL, FALSE, NULL, &disassembly);
    ok(ret == D3DERR_INVALIDCALL, "Returned %#x, expected %#x\n", ret, D3DERR_INVALIDCALL);
    ret = D3DXDisassembleShader(simple_vs, FALSE, NULL, NULL);
    ok(ret == D3DERR_INVALIDCALL, "Returned %#x, expected %#x\n", ret, D3DERR_INVALIDCALL);

    /* Test with vertex shader */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(simple_vs, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_vs, sizeof(disasm_vs) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_vs);
    ID3DXBuffer_Release(disassembly);

    /* Test with pixel shader */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(simple_ps, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps, sizeof(disasm_ps) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps);
    ID3DXBuffer_Release(disassembly);

    /* Test tex instruction with pixel shader 1.3 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(ps_tex, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps_tex, sizeof(disasm_ps_tex) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps_tex);
    ID3DXBuffer_Release(disassembly);

    /* Test texld instruction with pixel shader 1.4 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(ps_texld_1_4, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps_texld_1_4, sizeof(disasm_ps_texld_1_4) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps_texld_1_4);
    ID3DXBuffer_Release(disassembly);

    /* Test texld instruction with pixel shader 2.0 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(ps_texld_2_0, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps_texld_2_0, sizeof(disasm_ps_texld_2_0) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps_texld_2_0);
    ID3DXBuffer_Release(disassembly);

    /* Test texcoord instruction with pixel shader 1.3 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(ps_texcoord, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps_texcoord, sizeof(disasm_ps_texcoord) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps_texcoord);
    ID3DXBuffer_Release(disassembly);

    /* Test texcrd instruction with pixel shader 1.4 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(ps_texcrd, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps_texcrd, sizeof(disasm_ps_texcrd) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps_texcrd);
    ID3DXBuffer_Release(disassembly);

    /* Test sincos instruction pixel shader 2.0 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(ps_sincos_2_0, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps_sincos_2_0, sizeof(disasm_ps_sincos_2_0) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps_sincos_2_0);
    ID3DXBuffer_Release(disassembly);

    /* Test sincos instruction with pixel shader 3.0 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(ps_sincos_3_0, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_ps_sincos_3_0, sizeof(disasm_ps_sincos_3_0) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_ps_sincos_3_0);
    ID3DXBuffer_Release(disassembly);

    /* Test sincos instruction with pixel shader 2.0 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(vs_sincos_2_0, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_vs_sincos_2_0, sizeof(disasm_vs_sincos_2_0) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_vs_sincos_2_0);
    ID3DXBuffer_Release(disassembly);

    /* Test sincos instruction with pixel shader 3.0 */
    disassembly = (void *)0xdeadbeef;
    ret = D3DXDisassembleShader(vs_sincos_3_0, FALSE, NULL, &disassembly);
    ok(ret == D3D_OK, "Failed with %#x\n", ret);
    ptr = ID3DXBuffer_GetBufferPointer(disassembly);
    ok(!memcmp(ptr, disasm_vs_sincos_3_0, sizeof(disasm_vs_sincos_3_0) - 1), /* compare beginning */
       "Returned '%s', expected '%s'\n", ptr, disasm_vs_sincos_3_0);
    ID3DXBuffer_Release(disassembly);

}

START_TEST(shader)
{
    test_get_shader_size();
    test_get_shader_version();
    test_find_shader_comment();
    test_get_shader_constant_table_ex();
    test_constant_tables();
    test_setting_constants();
    test_get_sampler_index();
    test_get_shader_samplers();
    test_get_shader_constant_variables();
    test_registerset();
    test_registerset_defaults();
    test_shader_semantics();
    test_fragment_linker();
    test_disassemble_shader();
}
