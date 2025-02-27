/*
 * Some unit tests for d3dxof
 *
 * Copyright (C) 2008, 2013 Christian Costa
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

#include <stdio.h>

#include "wine/test.h"
#include "initguid.h"
#include "dxfile.h"
#include "rmxftmpl.h"

#define I2(x) x,0
#define I4(x) x,0,0,0

#define TOKEN_NAME         I2(1)
#define TOKEN_STRING       I2(2)
#define TOKEN_INTEGER      I2(3)
#define TOKEN_INTEGER_LIST I2(6)
#define TOKEN_OBRACE       I2(10)
#define TOKEN_CBRACE       I2(11)
#define TOKEN_COMMA        I2(19)
#define TOKEN_SEMICOLON    I2(20)

#define SEMICOLON_5X TOKEN_SEMICOLON, TOKEN_SEMICOLON, TOKEN_SEMICOLON, TOKEN_SEMICOLON, TOKEN_SEMICOLON

static HMODULE hd3dxof;
static HRESULT (WINAPI *pDirectXFileCreate)(LPDIRECTXFILE*);

static char template[] =
"xof 0302txt 0064\n"
"template Header\n"
"{\n"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>\n"
"WORD major;\n"
"WORD minor;\n"
"DWORD flags;\n"
"}\n";

/* Same version as above compressed with mszip */
static char compressed_template[] =
"xof 0302tzip0064\x71\x00\x00\x00\x61\x00\x5a\x00"
"\x43\x4B\x2B\x49\xCD\x2D\xC8\x49\x2C\x49\x55\xF0\x48\x4D\x4C\x49"
"\x2D\xE2\xAA\xE6\xB2\x31\x76\xB1\x30\x72\x74\x32\x31\xD6\x35\x33"
"\x72\x71\xD4\x35\x34\x74\x76\xD3\x75\x74\x32\xB6\xD4\x35\x30\x30"
"\x32\x70\x74\x33\x37\x74\x35\x31\x36\xB6\xE3\x0A\xF7\x0F\x72\x51"
"\xC8\x4D\xCC\xCA\x2F\xB2\x86\xB2\x33\xF3\x40\x6C\x17\x30\x27\x2D"
"\x27\x31\xBD\xD8\x9A\xAB\x96\x8B\x0B\x00";

static char object[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 2; 3;\n"
"}\n";

/* Same version as above compressed with mszip */
static char compressed_object[] =
"xof 0302tzip0064\x2c\x00\x00\x00\x1c\x00\x20\x00"
"\x43\x4b\xf3\x48\x4d\x4c\x49\x2d\x52\xf0\x4f\xca\x4a\x4d\x2e\xe1"
"\xaa\xe6\x32\xb4\x56\x30\xb2\x56\x30\xb6\xe6\xaa\xe5\xe2\x02\x00";

static char empty_txt_file[]  = "xof 0302txt 0064";
static char empty_bin_file[]  = "xof 0302bin 0064";
/* MSZip data is generated with the command "MAKECAB.EXE /D Compress=ON /D CompressionType=MSZip file packed"
 * Data in cab is after the filename (null terminated) and the 32-bit checksum:
 * size (16-bit), packed_size (16-bit) and compressed data (with leading 16-bit CK signature)
 * for each MSZIP chunk whose decompressed size cannot exceed 32768 bytes
 * Data in x files is preceded by the size (32-bit) of the decompressed file including the xof header (16 bytes)
 * It does not seem possible to generate an MSZip data chunk with no byte, so put just 1 byte here */
/* "\n" packed with MSZip => no text */
static char empty_tzip_file[] = "xof 0302tzip0064\x11\x00\x00\x00\x01\x00\x05\x00\x43\x4b\xe3\x02\x00";
/* "\n" packed with MSZip => no token (token are 16-bit and there is only 1 byte) */
static char empty_bzip_file[] = "xof 0302bzip0064\x11\x00\x00\x00\x01\x00\x05\x00\x43\x4b\xe3\x02\x00";
static char empty_cmp_file[]  = "xof 0302cmp 0064";
static char empty_xxxx_file[] = "xof 0302xxxx0064";

static char templates_bad_file_type1[]      = "xOf 0302txt 0064\n";
static char templates_bad_file_version[]    = "xof 0102txt 0064\n";
static char templates_bad_file_type2[]      = "xof 0302foo 0064\n";
static char templates_bad_file_float_size[] = "xof 0302txt 0050\n";

static char templates_parse_error[] =
"xof 0302txt 0064"
"foobar;\n";

static char object_noname[] =
"xof 0302txt 0064\n"
"Header\n"
"{\n"
"1; 2; 3;\n"
"}\n";

static char template_syntax_array_mixed[] =
"xof 0302txt 0064\n"
"template Buffer\n"
"{\n"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>\n"
"DWORD num_elem;\n"
"array DWORD value[num_elem];\n"
"DWORD dummy;\n"
"}\n";

static char object_syntax_empty_array[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"0;\n"
"1234;\n"
"}\n";

static char object_syntax_semicolon_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3;\n"
"0; 1; 2;\n"
"5;\n"
"}\n";

static char object_syntax_semicolon_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(0), TOKEN_SEMICOLON, TOKEN_INTEGER, I4(1), TOKEN_SEMICOLON, TOKEN_INTEGER, I4(2), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(5), TOKEN_SEMICOLON,
TOKEN_CBRACE
};

static char object_syntax_comma_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3,\n"
"0, 1, 2,\n"
"5,\n"
"}\n";

static char object_syntax_comma_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), TOKEN_COMMA,
TOKEN_INTEGER, I4(0), TOKEN_COMMA, TOKEN_INTEGER, I4(1), TOKEN_COMMA, TOKEN_INTEGER, I4(2), TOKEN_COMMA,
TOKEN_INTEGER, I4(5), TOKEN_COMMA,
TOKEN_CBRACE
};

static char object_syntax_multi_semicolons_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3;;;;;\n"
"0;;;;; 1;;;;; 2;;;;;\n"
"5;;;;;\n"
"}\n";

static char object_syntax_multi_semicolons_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), SEMICOLON_5X,
TOKEN_INTEGER, I4(0), SEMICOLON_5X, TOKEN_INTEGER, I4(1), SEMICOLON_5X, TOKEN_INTEGER, I4(2), SEMICOLON_5X,
TOKEN_INTEGER, I4(5), SEMICOLON_5X,
TOKEN_CBRACE
};

static char object_syntax_multi_commas_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3;\n"
"0, 1,, 2;\n"
"5;\n"
"}\n";

static char object_syntax_multi_commas_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(0), TOKEN_COMMA, TOKEN_INTEGER, I4(1), TOKEN_COMMA, TOKEN_COMMA, TOKEN_INTEGER, I4(2), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(5), TOKEN_SEMICOLON,
TOKEN_CBRACE
};

static char object_syntax_multi_semicolons_and_comma_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3;;;;;,\n"
"0;;;;;, 1;;;;;, 2;;;;;,\n"
"5;;;;;,\n"
"}\n";

static char object_syntax_multi_semicolons_and_comma_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), SEMICOLON_5X, TOKEN_COMMA,
TOKEN_INTEGER, I4(0), SEMICOLON_5X, TOKEN_COMMA, TOKEN_INTEGER, I4(1), SEMICOLON_5X, TOKEN_COMMA, TOKEN_INTEGER, I4(2), SEMICOLON_5X, TOKEN_COMMA,
TOKEN_INTEGER, I4(5), SEMICOLON_5X, TOKEN_COMMA,
TOKEN_CBRACE
};

static char object_syntax_comma_and_semicolon_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3;\n"
"0, 1,; 2;\n"
"5;\n"
"}\n";

static char object_syntax_comma_and_semicolon_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(0), TOKEN_COMMA, TOKEN_INTEGER, I4(1), TOKEN_COMMA, TOKEN_SEMICOLON, TOKEN_INTEGER, I4(2), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(5), TOKEN_SEMICOLON,
TOKEN_CBRACE
};

static char object_syntax_no_ending_separator_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3;\n"
"0, 1, 2;\n"
"5\n"
"}\n";

static char object_syntax_no_ending_separator_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(0), TOKEN_COMMA, TOKEN_INTEGER, I4(1), TOKEN_COMMA, TOKEN_INTEGER, I4(2), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(5),
TOKEN_CBRACE
};

static char object_syntax_array_no_separator_txt[] =
"xof 0302txt 0064\n"
"Buffer\n"
"{\n"
"3;\n"
"0 1 2;\n"
"5;\n"
"}\n";

static char object_syntax_array_no_separator_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(0), TOKEN_INTEGER, I4(1), TOKEN_INTEGER, I4(2), TOKEN_SEMICOLON,
TOKEN_INTEGER, I4(5), TOKEN_SEMICOLON,
TOKEN_CBRACE
};

static char object_syntax_full_integer_list_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER, I4(3), TOKEN_SEMICOLON,
TOKEN_INTEGER_LIST, I4(3), I4(0), I4(1), I4(2),
TOKEN_INTEGER, I4(5), TOKEN_SEMICOLON,
TOKEN_CBRACE
};

static char object_syntax_mixed_integer_list_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER_LIST, I4(5), I4(3), I4(0), I4(1), I4(2), I4(5),
TOKEN_CBRACE
};

static char object_syntax_integer_list_semicolon_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER_LIST, I4(5), I4(3), I4(0), I4(1), I4(2), I4(5), TOKEN_SEMICOLON,
TOKEN_CBRACE
};

static char object_syntax_integer_list_comma_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(6), /* name */ 'B','u','f','f','e','r', TOKEN_OBRACE,
TOKEN_INTEGER_LIST, I4(5), I4(3), I4(0), I4(1), I4(2), I4(5), TOKEN_COMMA,
TOKEN_CBRACE
};

static char template_syntax_string[] =
"xof 0302txt 0064\n"
"template Filename\n"
"{\n"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>\n"
"STRING filename;\n"
"}\n";

static char object_syntax_string_normal[] =
"xof 0302txt 0064\n"
"Filename\n"
"{\n"
"\"foobar\";\n"
"}\n";

static char object_syntax_string_with_separator[] =
"xof 0302txt 0064\n"
"Filename\n"
"{\n"
"\"foo;bar\";\n"
"}\n";

static char object_syntax_string_bin[] = {
'x','o','f',' ','0','3','0','2','b','i','n',' ','0','0','6','4',
TOKEN_NAME, /* size */ I4(8), /* name */ 'F','i','l','e','n','a','m','e', TOKEN_OBRACE,
TOKEN_STRING, /* size */ I4(6), /* string */ 'f','o','o','b','a','r', TOKEN_SEMICOLON,
TOKEN_CBRACE
};

static char templates_complex_object[] =
"xof 0302txt 0064\n"
"template Vector\n"
"{\n"
"<3D82AB5E-62DA-11CF-AB39-0020AF71E433>\n"
"FLOAT x;\n"
"FLOAT y;\n"
"FLOAT z;\n"
"}\n"
"template MeshFace\n"
"{\n"
"<3D82AB5F-62DA-11CF-AB39-0020AF71E433>\n"
"DWORD nFaceVertexIndices;\n"
"array DWORD faceVertexIndices[nFaceVertexIndices];\n"
"}\n"
"template Mesh\n"
"{\n"
"<3D82AB44-62DA-11CF-AB39-0020AF71E433>\n"
"DWORD nVertices;\n"
"array Vector vertices[nVertices];\n"
"DWORD nFaces;\n"
"array MeshFace faces[nFaces];\n"
"[...]\n"
"}\n";

static char object_complex[] =
"xof 0302txt 0064\n"
"Mesh Object\n"
"{\n"
"4;;;,\n"
"1.0;;;, 0.0;;;, 0.0;;;,\n"
"0.0;;;, 1.0;;;, 0.0;;;,\n"
"0.0;;;, 0.0;;;, 1.0;;;,\n"
"1.0;;;, 1.0;;;, 1.0;;;,\n"
"3;;;,\n"
"3;;;, 0;;;, 1;;;, 2;;;,\n"
"3;;;, 1;;;, 2;;;, 3;;;,\n"
"3;;;, 3;;;, 1;;;, 2;;;,\n"
"}\n";

static char template_using_index_color_lower[] =
"xof 0302txt 0064\n"
"template MeshVertexColors\n"
"{\n"
"<1630B821-7842-11cf-8F52-0040333594A3>\n"
"DWORD nVertexColors;\n"
"array indexColor vertexColors[nVertexColors];\n"
"}\n";

static char template_using_index_color_upper[] =
"xof 0302txt 0064\n"
"template MeshVertexColors\n"
"{\n"
"<1630B821-7842-11cf-8F52-0040333594A3>\n"
"DWORD nVertexColors;\n"
"array IndexColor vertexColors[nVertexColors];\n"
"}\n";

static void init_function_pointers(void)
{
    /* We have to use LoadLibrary as no d3dxof functions are referenced directly */
    hd3dxof = LoadLibraryA("d3dxof.dll");

    pDirectXFileCreate = (void *)GetProcAddress(hd3dxof, "DirectXFileCreate");
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static void test_refcount(void)
{
    HRESULT hr;
    ULONG ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpdxfeo;
    LPDIRECTXFILEDATA lpdxfd;
    DXFILELOADMEMORY dxflm;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    ref = get_refcount(lpDirectXFile);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);
    ref = IDirectXFile_AddRef(lpDirectXFile);
    ok(ref == 2, "Unexpected refcount %ld.\n", ref);
    ref = IDirectXFile_Release(lpDirectXFile);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template, sizeof(template) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    dxflm.lpMemory = &object;
    dxflm.dSize = sizeof(object) - 1;
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ref = get_refcount(lpDirectXFile);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);
    ref = get_refcount(lpdxfeo);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);

    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ref = get_refcount(lpDirectXFile);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);
    ref = get_refcount(lpdxfeo);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);
    /* Enum object gets references to all top level objects */
    ref = get_refcount(lpdxfd);
    ok(ref == 2, "Unexpected refcount %ld.\n", ref);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
    /* Nothing changes for all other objects */
    ref = get_refcount(lpdxfeo);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);
    ref = get_refcount(lpdxfd);
    ok(ref == 2, "Unexpected refcount %ld.\n", ref);

    ref = IDirectXFileEnumObject_Release(lpdxfeo);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
    /* Enum object releases references to all top level objects */
    ref = get_refcount(lpdxfd);
    ok(ref == 1, "Unexpected refcount %ld.\n", ref);

    ref = IDirectXFileData_Release(lpdxfd);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
}

static void test_CreateEnumObject(void)
{
    HRESULT hr;
    ULONG ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpdxfeo;
    LPDIRECTXFILEDATA lpdxfd;
    DXFILELOADMEMORY dxflm;
    BYTE* pdata;
    DWORD size;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template, sizeof(template) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    dxflm.lpMemory = &object;
    dxflm.dSize = sizeof(object) - 1;
    /* Check that only lowest 4 bits are relevant in DXFILELOADOPTIONS */
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, 0xFFFFFFF0 + DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Get all data (szMember == NULL) */
    hr = IDirectXFileData_GetData(lpdxfd, NULL, &size, (void**)&pdata);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    ok(size == 8, "Unexpected data size %lu.\n", size);
    ok((*((WORD*)pdata) == 1) && (*((WORD*)(pdata+2)) == 2) && (*((DWORD*)(pdata+4)) == 3), "Retrieved data is wrong\n");

    /* Get only "major" member (szMember == "major") */
    hr = IDirectXFileData_GetData(lpdxfd, "major", &size, (void**)&pdata);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    ok(size == 2, "Unexpected data size %lu.\n", size);
    ok(*((WORD*)pdata) == 1, "Retrieved data is wrong (%u instead of 1)\n", *((WORD*)pdata));

    /* Get only "minor" member (szMember == "minor") */
    hr = IDirectXFileData_GetData(lpdxfd, "minor", &size, (void**)&pdata);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    ok(size == 2, "Unexpected data size %lu.\n", size);
    ok(*((WORD*)pdata) == 2, "Retrieved data is wrong (%u instead of 2)\n", *((WORD*)pdata));

    /* Get only "flags" member (szMember == "flags") */
    hr = IDirectXFileData_GetData(lpdxfd, "flags", &size, (void**)&pdata);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    ok(size == 4, "Unexpected data size %lu.\n", size);
    ok(*((WORD*)pdata) == 3, "Retrieved data is wrong (%u instead of 3)\n", *((WORD*)pdata));

    /* Try to get not existing member (szMember == "unknown") */
    hr = IDirectXFileData_GetData(lpdxfd, "unknown", &size, (void**)&pdata);
    ok(hr == DXFILEERR_BADDATAREFERENCE, "Unexpected hr %#lx.\n", hr);

    ref = IDirectXFileEnumObject_Release(lpdxfeo);
    ok(!ref, "Unexpected refcount %ld.\n", ref);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(!ref, "Unexpected refcount %ld.\n", ref);

    ref = IDirectXFileData_Release(lpdxfd);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
}

static void test_file_types(void)
{
    HRESULT hr;
    LPDIRECTXFILE dxfile = NULL;
    LPDIRECTXFILEENUMOBJECT enum_object;
    DXFILELOADMEMORY lminfo;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_txt_file, sizeof(empty_txt_file) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_bin_file, sizeof(empty_bin_file) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_tzip_file, sizeof(empty_tzip_file) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_bzip_file, sizeof(empty_bzip_file) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_cmp_file, sizeof(empty_cmp_file) - 1);
    ok(hr == DXFILEERR_BADFILETYPE, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_xxxx_file, sizeof(empty_xxxx_file) - 1);
    ok(hr == DXFILEERR_BADFILETYPE, "Unexpected hr %#lx.\n", hr);

    lminfo.lpMemory = empty_txt_file;
    lminfo.dSize = sizeof(empty_txt_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_bin_file;
    lminfo.dSize = sizeof(empty_bin_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_tzip_file;
    lminfo.dSize = sizeof(empty_tzip_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_bzip_file;
    lminfo.dSize = sizeof(empty_bzip_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_cmp_file;
    lminfo.dSize = sizeof(empty_cmp_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILEERR_BADFILETYPE, "Unexpected hr %#lx.\n", hr);

    lminfo.lpMemory = empty_xxxx_file;
    lminfo.dSize = sizeof(empty_xxxx_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILEERR_BADFILETYPE, "Unexpected hr %#lx.\n", hr);

    IDirectXFile_Release(dxfile);
}

static void test_templates(void)
{
    IDirectXFile *dxfile = NULL;
    HRESULT hr;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, templates_bad_file_type1, sizeof(templates_bad_file_type1) - 1);
    ok(hr == DXFILEERR_BADFILETYPE, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, templates_bad_file_version, sizeof(templates_bad_file_version) - 1);
    ok(hr == DXFILEERR_BADFILEVERSION, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, templates_bad_file_type2, sizeof(templates_bad_file_type2) - 1);
    ok(hr == DXFILEERR_BADFILETYPE, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, templates_bad_file_float_size, sizeof(templates_bad_file_float_size) - 1);
    ok(hr == DXFILEERR_BADFILEFLOATSIZE, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, templates_parse_error, sizeof(templates_parse_error) - 1);
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    IDirectXFile_Release(dxfile);
}

static void test_compressed_files(void)
{
    HRESULT hr;
    LPDIRECTXFILE dxfile = NULL;
    LPDIRECTXFILEENUMOBJECT enum_object;
    LPDIRECTXFILEDATA file_data;
    DXFILELOADMEMORY lminfo;
    BYTE* data;
    DWORD size;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, compressed_template, sizeof(compressed_template) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    lminfo.lpMemory = compressed_object;
    lminfo.dSize = sizeof(compressed_object) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFileEnumObject_GetNextDataObject(enum_object, &file_data);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFileData_GetData(file_data, NULL, &size, (void**)&data);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    ok(size == 8, "Retrieved data size is wrong\n");
    ok((*((WORD*)data) == 1) && (*((WORD*)(data+2)) == 2) && (*((DWORD*)(data+4)) == 3), "Retrieved data is wrong\n");

    IDirectXFileData_Release(file_data);
    IDirectXFileEnumObject_Release(enum_object);
    IDirectXFile_Release(dxfile);
}

static void test_getname(void)
{
    HRESULT hr;
    ULONG ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpdxfeo;
    LPDIRECTXFILEDATA lpdxfd;
    DXFILELOADMEMORY dxflm;
    char name[100];
    DWORD length;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template, sizeof(template) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Check object with name */
    dxflm.lpMemory = &object;
    dxflm.dSize = sizeof(object) - 1;
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFileData_GetName(lpdxfd, NULL, NULL);
    ok(hr == DXFILEERR_BADVALUE, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetName(lpdxfd, name, NULL);
    ok(hr == DXFILEERR_BADVALUE, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetName(lpdxfd, NULL, &length);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 7, "Unexpected length %lu.\n", length);
    length = sizeof(name);
    hr = IDirectXFileData_GetName(lpdxfd, name, &length);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 7, "Unexpected length %lu.\n", length);
    ok(!strcmp(name, "Object"), "Returned string should be 'Object' instead of '%s'\n", name);
    length = 3;
    hr = IDirectXFileData_GetName(lpdxfd, name, &length);
    ok(hr == DXFILEERR_BADVALUE, "Unexpected hr %#lx.\n", hr);

    ref = IDirectXFileEnumObject_Release(lpdxfeo);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
    ref = IDirectXFileData_Release(lpdxfd);
    ok(!ref, "Unexpected refcount %ld.\n", ref);

    /* Check object without name */
    dxflm.lpMemory = &object_noname;
    dxflm.dSize = sizeof(object_noname) - 1;
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFileData_GetName(lpdxfd, NULL, &length);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(!length, "Unexpected length %lu.\n", length);
    length = 0;
    name[0] = 0x7f;
    hr = IDirectXFileData_GetName(lpdxfd, name, &length);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(!length, "Unexpected length %lu.\n", length);
    ok(name[0] == 0x7f, "First character is %#x instead of 0x7f\n", name[0]);
    length = sizeof(name);
    name[0] = 0x7f;
    hr = IDirectXFileData_GetName(lpdxfd, name, &length);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(!length, "Unexpected length %lu.\n", length);
    ok(name[0] == 0, "First character is %#x instead of 0x00\n", name[0]);

    ref = IDirectXFileEnumObject_Release(lpdxfeo);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
    ref = IDirectXFileData_Release(lpdxfd);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
    ref = IDirectXFile_Release(lpDirectXFile);
    ok(!ref, "Unexpected refcount %ld.\n", ref);
}

static void test_syntax(void)
{
    HRESULT hr;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpdxfeo;
    LPDIRECTXFILEDATA lpdxfd;
    DXFILELOADMEMORY dxflm;
    DWORD size;
    char** string;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template_syntax_array_mixed, sizeof(template_syntax_array_mixed) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test empty array */
    dxflm.lpMemory = &object_syntax_empty_array;
    dxflm.dSize = sizeof(object_syntax_empty_array) - 1;
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == DXFILE_OK)
        IDirectXFileData_Release(lpdxfd);
    IDirectXFileEnumObject_Release(lpdxfeo);

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template_syntax_string, sizeof(template_syntax_string) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test normal string */
    dxflm.lpMemory = &object_syntax_string_normal;
    dxflm.dSize = sizeof(object_syntax_string_normal) - 1;
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetData(lpdxfd, NULL, &size, (void**)&string);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == sizeof(char*), "Got wrong data size %ld.\n", size);
    ok(!strcmp(*string, "foobar"), "Got string %s, expected foobar\n", *string);
    if (hr == DXFILE_OK)
        IDirectXFileData_Release(lpdxfd);
    IDirectXFileEnumObject_Release(lpdxfeo);

    /* Test string containing separator character */
    dxflm.lpMemory = &object_syntax_string_with_separator;
    dxflm.dSize = sizeof(object_syntax_string_with_separator) - 1;
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetData(lpdxfd, NULL, &size, (void**)&string);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == sizeof(char*), "Got wrong data size %ld.\n", size);
    ok(!strcmp(*string, "foo;bar"), "Got string %s, expected foo;bar\n", *string);
    if (hr == DXFILE_OK)
        IDirectXFileData_Release(lpdxfd);
    IDirectXFileEnumObject_Release(lpdxfeo);

    /* Test string in binary mode */
    dxflm.lpMemory = &object_syntax_string_bin;
    dxflm.dSize = sizeof(object_syntax_string_bin);
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetData(lpdxfd, NULL, &size, (void**)&string);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == sizeof(char*), "Got wrong data size %ld.\n", size);
    ok(!strcmp(*string, "foobar"), "Got string %s, expected foobar\n", *string);
    if (hr == DXFILE_OK)
        IDirectXFileData_Release(lpdxfd);
    IDirectXFileEnumObject_Release(lpdxfeo);

    IDirectXFile_Release(lpDirectXFile);
}

static HRESULT test_buffer_object(IDirectXFile *dxfile, char* object_data, DWORD object_size)
{
    HRESULT hr, ret;
    IDirectXFileEnumObject *enum_object;
    IDirectXFileData *file_data;
    DXFILELOADMEMORY load_info;
    DWORD size;
    const DWORD values[] = { 3, 0, 1, 2, 5 };
    DWORD* array;

    load_info.lpMemory = object_data;
    load_info.dSize = object_size;
    hr = IDirectXFile_CreateEnumObject(dxfile, &load_info, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    ret = IDirectXFileEnumObject_GetNextDataObject(enum_object, &file_data);
    if (ret == DXFILE_OK)
    {
        hr = IDirectXFileData_GetData(file_data, NULL, &size, (void**)&array);
        ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
        ok(size == sizeof(values), "Got wrong data size %ld.\n", size);
        ok(!memcmp(array, values, sizeof(values)), "Got values [%lu, %lu, %lu, %lu, %lu], expected [%lu, %lu, %lu, %lu, %lu]\n",
           array[0], array[1], array[2], array[3], array[4], values[0], values[1], values[2], values[3], values[4]);
        IDirectXFileData_Release(file_data);
    }
    IDirectXFileEnumObject_Release(enum_object);

    return ret;
}

static void test_syntax_semicolon_comma(void)
{
    IDirectXFile *dxfile = NULL;
    HRESULT hr;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, template_syntax_array_mixed, sizeof(template_syntax_array_mixed) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test semicolon separators in text mode */
    hr = test_buffer_object(dxfile, object_syntax_semicolon_txt, sizeof(object_syntax_semicolon_txt) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    /* Test semicolon separators in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_semicolon_bin, sizeof(object_syntax_semicolon_bin));
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test comma separators in text mode */
    hr = test_buffer_object(dxfile, object_syntax_comma_txt, sizeof(object_syntax_comma_txt) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    /* Test comma separators in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_comma_bin, sizeof(object_syntax_comma_bin));
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test multi-semicolons separators in text mode */
    hr = test_buffer_object(dxfile, object_syntax_multi_semicolons_txt, sizeof(object_syntax_multi_semicolons_txt) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    /* Test multi-semicolons separators in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_multi_semicolons_bin, sizeof(object_syntax_multi_semicolons_bin));
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test multi-commas separators in text mode */
    hr = test_buffer_object(dxfile, object_syntax_multi_commas_txt, sizeof(object_syntax_multi_semicolons_txt) - 1);
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);
    /* Test multi-commas separators in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_multi_commas_bin, sizeof(object_syntax_multi_semicolons_bin));
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    /* Test multi-semicolons + single comma separators in text mode */
    hr = test_buffer_object(dxfile, object_syntax_multi_semicolons_and_comma_txt, sizeof(object_syntax_multi_semicolons_and_comma_txt) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    /* Test multi-semicolons + single comma separators in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_multi_semicolons_and_comma_bin, sizeof(object_syntax_multi_semicolons_and_comma_bin));
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test comma + semicolon separators in text mode */
    hr = test_buffer_object(dxfile, object_syntax_comma_and_semicolon_txt, sizeof(object_syntax_comma_and_semicolon_txt) - 1);
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);
    /* Test comma + semicolon separators in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_comma_and_semicolon_bin, sizeof(object_syntax_comma_and_semicolon_bin));
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    /* Test no ending separator in text mode */
    hr = test_buffer_object(dxfile, object_syntax_no_ending_separator_txt, sizeof(object_syntax_no_ending_separator_txt) - 1);
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);
    /* Test no ending separator in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_no_ending_separator_bin, sizeof(object_syntax_no_ending_separator_bin));
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    /* Test no array separator in text mode */
    hr = test_buffer_object(dxfile, object_syntax_array_no_separator_txt, sizeof(object_syntax_array_no_separator_txt) - 1);
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);
    /* Test no array separator in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_array_no_separator_bin, sizeof(object_syntax_array_no_separator_bin));
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    /* Test object with a single integer list in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_full_integer_list_bin, sizeof(object_syntax_full_integer_list_bin));
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test object with mixed integer list and integers + single comma separators in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_mixed_integer_list_bin, sizeof(object_syntax_mixed_integer_list_bin));
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test integer list followed by a semicolon in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_integer_list_semicolon_bin, sizeof(object_syntax_integer_list_semicolon_bin));
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    /* Test integer list followed by a comma in binary mode */
    hr = test_buffer_object(dxfile, object_syntax_integer_list_comma_bin, sizeof(object_syntax_integer_list_comma_bin));
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    IDirectXFile_Release(dxfile);
}

static void test_complex_object(void)
{
    IDirectXFile *dxfile = NULL;
    IDirectXFileEnumObject *enum_object;
    IDirectXFileData *file_data;
    DXFILELOADMEMORY load_info;
    HRESULT hr;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, templates_complex_object, sizeof(templates_complex_object) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    load_info.lpMemory = object_complex;
    load_info.dSize = sizeof(object_complex) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &load_info, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileEnumObject_GetNextDataObject(enum_object, &file_data);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    IDirectXFileData_Release(file_data);
    IDirectXFileEnumObject_Release(enum_object);
    IDirectXFile_Release(dxfile);
}

static void test_standard_templates(void)
{
    IDirectXFile *dxfile = NULL;
    HRESULT hr;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    IDirectXFile_Release(dxfile);
}

static void test_type_index_color(void)
{
    IDirectXFile *dxfile = NULL;
    HRESULT hr;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    /* Test that 'indexColor' can be used (same as IndexedColor in standard templates) and is case sensitive */
    hr = IDirectXFile_RegisterTemplates(dxfile, template_using_index_color_lower, sizeof(template_using_index_color_lower) - 1);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFile_RegisterTemplates(dxfile, template_using_index_color_upper, sizeof(template_using_index_color_upper) - 1);
    ok(hr == DXFILEERR_PARSEERROR, "Unexpected hr %#lx.\n", hr);

    IDirectXFile_Release(dxfile);
}

/* Set it to 1 to expand the string when dumping the object. This is useful when there is
 * only one string in a sub-object (very common). Use with care, this may lead to a crash. */
#define EXPAND_STRING 0

static void process_data(LPDIRECTXFILEDATA lpDirectXFileData, int level)
{
    HRESULT hr;
    char name[100];
    GUID clsid;
    const GUID *clsid_type = NULL;
    DWORD len = 100;
    LPDIRECTXFILEOBJECT pChildObj;
    int i;
    int j = 0;
    LPBYTE pData;
    DWORD k, size;

    hr = IDirectXFileData_GetId(lpDirectXFileData, &clsid);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetName(lpDirectXFileData, name, &len);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetType(lpDirectXFileData, &clsid_type);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectXFileData_GetData(lpDirectXFileData, NULL, &size, (void**)&pData);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < level; i++)
        printf("  ");
    printf("Found object '%s' - %s - %s - %ld\n",
           len ? name : "", wine_dbgstr_guid(&clsid), wine_dbgstr_guid(clsid_type), size);

    if (EXPAND_STRING && size == 4)
    {
        char * str = *(char**)pData;
        printf("string %s\n", str);
    }
    else if (size)
    {
        for (k = 0; k < size; k++)
        {
            if (k && !(k%16))
                printf("\n");
            printf("%02x ", pData[k]);
        }
        printf("\n");
    }

    level++;

    while (SUCCEEDED(hr = IDirectXFileData_GetNextObject(lpDirectXFileData, &pChildObj)))
    {
        LPDIRECTXFILEDATA p1;
        LPDIRECTXFILEDATAREFERENCE p2;
        LPDIRECTXFILEBINARY p3;
        j++;

        hr = IDirectXFileObject_QueryInterface(pChildObj, &IID_IDirectXFileData, (void **) &p1);
        if (SUCCEEDED(hr))
        {
            for (i = 0; i < level; i++)
                printf("  ");
            printf("Found Data (%d)\n", j);
            process_data(p1, level);
            IDirectXFileData_Release(p1);
        }
        hr = IDirectXFileObject_QueryInterface(pChildObj, &IID_IDirectXFileDataReference, (void **) &p2);
        if (SUCCEEDED(hr))
        {
            LPDIRECTXFILEDATA pfdo;
            for (i = 0; i < level; i++)
                printf("  ");
            printf("Found Data Reference (%d)\n", j);
if (0)
{
            hr = IDirectXFileDataReference_GetId(lpDirectXFileData, &clsid);
            ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
            hr = IDirectXFileDataReference_GetName(lpDirectXFileData, name, &len);
            ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);
}
            IDirectXFileDataReference_Resolve(p2, &pfdo);
            process_data(pfdo, level);
            IDirectXFileData_Release(pfdo);
            IDirectXFileDataReference_Release(p2);
        }
        hr = IDirectXFileObject_QueryInterface(pChildObj, &IID_IDirectXFileBinary, (void **) &p3);
        if (SUCCEEDED(hr))
        {
            for (i = 0; i < level; i++)
                printf("  ");
            printf("Found Binary (%d)\n", j);
            IDirectXFileBinary_Release(p3);
        }
        IDirectXFileObject_Release(pChildObj);
    }

    ok(hr == DXFILE_OK || hr == DXFILEERR_NOMOREOBJECTS, "Unexpected hr %#lx.\n", hr);
}

/* Dump an X file 'objects.x' and its related templates file 'templates.x' if they are both presents
 * Useful for debug by comparing outputs from native and builtin dlls */
static void test_dump(void)
{
    HRESULT hr;
    ULONG ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpDirectXFileEnumObject = NULL;
    LPDIRECTXFILEDATA lpDirectXFileData = NULL;
    HANDLE hFile;
    LPVOID pvData = NULL;
    DWORD cbSize;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        goto exit;
    }

    /* Dump data only if there is an object and a template */
    hFile = CreateFileA("objects.x", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
      return;
    CloseHandle(hFile);

    hFile = CreateFileA("templates.x", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
      return;

    pvData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 10000);

    if (!ReadFile(hFile, pvData, 10000, &cbSize, NULL))
    {
      skip("Templates file is too big\n");
      goto exit;
    }

    printf("Load templates file (%ld bytes)\n", cbSize);

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, pvData, cbSize);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, (LPVOID)"objects.x", DXFILELOAD_FROMFILE, &lpDirectXFileEnumObject);
    ok(hr == DXFILE_OK, "Unexpected hr %#lx.\n", hr);

    while (SUCCEEDED(hr = IDirectXFileEnumObject_GetNextDataObject(lpDirectXFileEnumObject, &lpDirectXFileData)))
    {
        printf("\n");
        process_data(lpDirectXFileData, 0);
        IDirectXFileData_Release(lpDirectXFileData);
    }
    ok(hr == DXFILE_OK || hr == DXFILEERR_NOMOREOBJECTS, "Unexpected hr %#lx.\n", hr);

    ref = IDirectXFile_Release(lpDirectXFileEnumObject);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

    CloseHandle(hFile);

exit:
    HeapFree(GetProcessHeap(), 0, pvData);
}

START_TEST(d3dxof)
{
    init_function_pointers();

    test_refcount();
    test_CreateEnumObject();
    test_file_types();
    test_templates();
    test_compressed_files();
    test_getname();
    test_syntax();
    test_syntax_semicolon_comma();
    test_complex_object();
    test_standard_templates();
    test_type_index_color();
    test_dump();

    FreeLibrary(hd3dxof);
}
