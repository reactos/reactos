/*
 * Copyright 2012 Christian Costa
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
#include "d3dx9xof.h"

static const char templates_bad_file_type1[] =
"xOf 0302txt 0064\n";

static const char templates_bad_file_version[] =
"xof 0102txt 0064\n";

static const char templates_bad_file_type2[] =
"xof 0302foo 0064\n";

static const char templates_bad_file_float_size[] =
"xof 0302txt 0050\n";

static const char templates_parse_error[] =
"xof 0302txt 0064"
"foobar;\n";

static const char templates[] =
"xof 0302txt 0064"
"template Header"
"{"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>"
"WORD major;"
"WORD minor;"
"DWORD flags;"
"}\n";

static char objects[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 2; 3;\n"
"}\n";

static char object_noname[] =
"xof 0302txt 0064\n"
"Header\n"
"{\n"
"1; 2; 3;\n"
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

static void test_templates(void)
{
    ID3DXFile *d3dxfile;
    HRESULT ret;

    ret = D3DXFileCreate(NULL);
    ok(ret == E_POINTER, "D3DXCreateFile returned %#lx, expected %#lx\n", ret, E_POINTER);

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#lx\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_type1, sizeof(templates_bad_file_type1) - 1);
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#lx, expected %#lx\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_version, sizeof(templates_bad_file_version) - 1);
    ok(ret == D3DXFERR_BADFILEVERSION, "RegisterTemplates returned %#lx, expected %#lx\n", ret, D3DXFERR_BADFILEVERSION);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_type2, sizeof(templates_bad_file_type2) - 1);
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#lx, expected %#lx\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_float_size, sizeof(templates_bad_file_float_size) - 1);
    ok(ret == D3DXFERR_BADFILEFLOATSIZE, "RegisterTemplates returned %#lx, expected %#lx\n", ret, D3DXFERR_BADFILEFLOATSIZE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_parse_error, sizeof(templates_parse_error) - 1);
    ok(ret == D3DXFERR_PARSEERROR, "RegisterTemplates returned %#lx, expected %#lx\n", ret, D3DXFERR_PARSEERROR);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#lx\n", ret);

    d3dxfile->lpVtbl->Release(d3dxfile);
}

static void test_lock_unlock(void)
{
    ID3DXFile *d3dxfile;
    D3DXF_FILELOADMEMORY memory;
    ID3DXFileEnumObject *enum_object;
    ID3DXFileData *data_object;
    const void *data;
    SIZE_T size;
    HRESULT ret;

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#lx\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#lx\n", ret);

    memory.lpMemory = objects;
    memory.dSize = sizeof(objects) - 1;

    ret = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &memory, D3DXF_FILELOAD_FROMMEMORY, &enum_object);
    ok(ret == S_OK, "CreateEnumObject failed with %#lx\n", ret);

    ret = enum_object->lpVtbl->GetChild(enum_object, 0, &data_object);
    ok(ret == S_OK, "GetChild failed with %#lx\n", ret);

    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#lx\n", ret);
    ret = data_object->lpVtbl->Lock(data_object, &size, &data);
    ok(ret == S_OK, "Lock failed with %#lx\n", ret);
    ret = data_object->lpVtbl->Lock(data_object, &size, &data);
    ok(ret == S_OK, "Lock failed with %#lx\n", ret);
    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#lx\n", ret);
    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#lx\n", ret);

    data_object->lpVtbl->Release(data_object);
    enum_object->lpVtbl->Release(enum_object);
    d3dxfile->lpVtbl->Release(d3dxfile);
}

static void test_getname(void)
{
    ID3DXFile *d3dxfile;
    D3DXF_FILELOADMEMORY memory;
    ID3DXFileEnumObject *enum_object;
    ID3DXFileData *data_object;
    SIZE_T length;
    char name[100];
    HRESULT ret;

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#lx\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#lx\n", ret);

    /* Check object with name */
    memory.lpMemory = objects;
    memory.dSize = sizeof(objects) - 1;
    ret = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &memory, D3DXF_FILELOAD_FROMMEMORY, &enum_object);
    ok(ret == S_OK, "CreateEnumObject failed with %#lx\n", ret);
    ret = enum_object->lpVtbl->GetChild(enum_object, 0, &data_object);
    ok(ret == S_OK, "GetChild failed with %#lx\n", ret);

    ret = data_object->lpVtbl->GetName(data_object, NULL, NULL);
    ok(ret == D3DXFERR_BADVALUE, "GetName returned %#lx, expected %#lx\n", ret, D3DXFERR_BADVALUE);
    ret = data_object->lpVtbl->GetName(data_object, name, NULL);
    ok(ret == D3DXFERR_BADVALUE, "GetName returned %#lx, expected %#lx\n", ret, D3DXFERR_BADVALUE);
    ret = data_object->lpVtbl->GetName(data_object, NULL, &length);
    ok(ret == S_OK, "GetName failed with %#lx\n", ret);
    ok(length == 7, "Returned length should be 7 instead of %Id\n", length);
    length = sizeof(name);
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret == S_OK, "GetName failed with %#lx\n", ret);
    ok(length == 7, "Returned length should be 7 instead of %Id\n", length);
    ok(!strcmp(name, "Object"), "Returned string should be 'Object' instead of '%s'\n", name);
    length = 3;
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret== D3DXFERR_BADVALUE, "GetName returned %#lx, expected %#lx\n", ret, D3DXFERR_BADVALUE);

    data_object->lpVtbl->Release(data_object);
    enum_object->lpVtbl->Release(enum_object);

    /* Check object without name */
    memory.lpMemory = object_noname;
    memory.dSize = sizeof(object_noname) - 1;
    ret = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &memory, D3DXF_FILELOAD_FROMMEMORY, &enum_object);
    ok(ret == S_OK, "CreateEnumObject failed with %#lx\n", ret);
    ret = enum_object->lpVtbl->GetChild(enum_object, 0, &data_object);
    ok(ret == S_OK, "GetChild failed with %#lx\n", ret);

    /* Contrary to d3dxof, d3dx9_36 returns an empty string with a null byte when no name is available.
     * If the input size is 0, it returns a length of 1 without touching the buffer */
    ret = data_object->lpVtbl->GetName(data_object, NULL, &length);
    ok(ret == S_OK, "GetName failed with %#lx\n", ret);
    ok(length == 1, "Returned length should be 1 instead of %Id\n", length);
    length = 0;
    name[0] = 0x7f;
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret == S_OK, "GetName failed with %#lx\n", ret);
    ok(length == 1, "Returned length should be 1 instead of %Id\n", length);
    ok(name[0] == 0x7f, "First character is %#x instead of 0x7f\n", name[0]);
    length = sizeof(name);
    name[0] = 0x7f;
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret == S_OK, "GetName failed with %#lx\n", ret);
    ok(length == 1, "Returned length should be 1 instead of %Id\n", length);
    ok(name[0] == 0, "First character is %#x instead of 0x00\n", name[0]);

    data_object->lpVtbl->Release(data_object);
    enum_object->lpVtbl->Release(enum_object);
    d3dxfile->lpVtbl->Release(d3dxfile);
}

static void test_type_index_color(void)
{
    ID3DXFile *d3dxfile;
    HRESULT ret;

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#lx\n", ret);

    /* Test that 'indexColor' can be used (same as IndexedColor in standard templates) and is case sensitive */
    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, template_using_index_color_lower, sizeof(template_using_index_color_lower) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#lx\n", ret);
    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, template_using_index_color_upper, sizeof(template_using_index_color_upper) - 1);
    ok(ret == D3DXFERR_PARSEERROR, "RegisterTemplates returned %#lx instead of %#lx\n", ret, D3DXFERR_PARSEERROR);

    d3dxfile->lpVtbl->Release(d3dxfile);
}

START_TEST(xfile)
{
    test_templates();
    test_lock_unlock();
    test_getname();
    test_type_index_color();
}
