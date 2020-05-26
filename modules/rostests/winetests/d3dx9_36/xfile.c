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

#include <stdio.h>

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
    ok(ret == E_POINTER, "D3DXCreateFile returned %#x, expected %#x\n", ret, E_POINTER);

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_type1, sizeof(templates_bad_file_type1) - 1);
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_version, sizeof(templates_bad_file_version) - 1);
    ok(ret == D3DXFERR_BADFILEVERSION, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILEVERSION);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_type2, sizeof(templates_bad_file_type2) - 1);
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_float_size, sizeof(templates_bad_file_float_size) - 1);
    ok(ret == D3DXFERR_BADFILEFLOATSIZE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILEFLOATSIZE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_parse_error, sizeof(templates_parse_error) - 1);
    ok(ret == D3DXFERR_PARSEERROR, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_PARSEERROR);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#x\n", ret);

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
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#x\n", ret);

    memory.lpMemory = objects;
    memory.dSize = sizeof(objects) - 1;

    ret = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &memory, D3DXF_FILELOAD_FROMMEMORY, &enum_object);
    ok(ret == S_OK, "CreateEnumObject failed with %#x\n", ret);

    ret = enum_object->lpVtbl->GetChild(enum_object, 0, &data_object);
    ok(ret == S_OK, "GetChild failed with %#x\n", ret);

    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Lock(data_object, &size, &data);
    ok(ret == S_OK, "Lock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Lock(data_object, &size, &data);
    ok(ret == S_OK, "Lock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#x\n", ret);

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
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#x\n", ret);

    /* Check object with name */
    memory.lpMemory = objects;
    memory.dSize = sizeof(objects) - 1;
    ret = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &memory, D3DXF_FILELOAD_FROMMEMORY, &enum_object);
    ok(ret == S_OK, "CreateEnumObject failed with %#x\n", ret);
    ret = enum_object->lpVtbl->GetChild(enum_object, 0, &data_object);
    ok(ret == S_OK, "GetChild failed with %#x\n", ret);

    ret = data_object->lpVtbl->GetName(data_object, NULL, NULL);
    ok(ret == D3DXFERR_BADVALUE, "GetName returned %#x, expected %#x\n", ret, D3DXFERR_BADVALUE);
    ret = data_object->lpVtbl->GetName(data_object, name, NULL);
    ok(ret == D3DXFERR_BADVALUE, "GetName returned %#x, expected %#x\n", ret, D3DXFERR_BADVALUE);
    ret = data_object->lpVtbl->GetName(data_object, NULL, &length);
    ok(ret == S_OK, "GetName failed with %#x\n", ret);
    ok(length == 7, "Returned length should be 7 instead of %ld\n", length);
    length = sizeof(name);
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret == S_OK, "GetName failed with %#x\n", ret);
    ok(length == 7, "Returned length should be 7 instead of %ld\n", length);
    ok(!strcmp(name, "Object"), "Returned string should be 'Object' instead of '%s'\n", name);
    length = 3;
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret== D3DXFERR_BADVALUE, "GetName returned %#x, expected %#x\n", ret, D3DXFERR_BADVALUE);

    data_object->lpVtbl->Release(data_object);
    enum_object->lpVtbl->Release(enum_object);

    /* Check object without name */
    memory.lpMemory = object_noname;
    memory.dSize = sizeof(object_noname) - 1;
    ret = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &memory, D3DXF_FILELOAD_FROMMEMORY, &enum_object);
    ok(ret == S_OK, "CreateEnumObject failed with %#x\n", ret);
    ret = enum_object->lpVtbl->GetChild(enum_object, 0, &data_object);
    ok(ret == S_OK, "GetChild failed with %#x\n", ret);

    /* Contrary to d3dxof, d3dx9_36 returns an empty string with a null byte when no name is available.
     * If the input size is 0, it returns a length of 1 without touching the buffer */
    ret = data_object->lpVtbl->GetName(data_object, NULL, &length);
    ok(ret == S_OK, "GetName failed with %#x\n", ret);
    ok(length == 1, "Returned length should be 1 instead of %ld\n", length);
    length = 0;
    name[0] = 0x7f;
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret == S_OK, "GetName failed with %#x\n", ret);
    ok(length == 1, "Returned length should be 1 instead of %ld\n", length);
    ok(name[0] == 0x7f, "First character is %#x instead of 0x7f\n", name[0]);
    length = sizeof(name);
    name[0] = 0x7f;
    ret = data_object->lpVtbl->GetName(data_object, name, &length);
    ok(ret == S_OK, "GetName failed with %#x\n", ret);
    ok(length == 1, "Returned length should be 1 instead of %ld\n", length);
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
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    /* Test that 'indexColor' can be used (same as IndexedColor in standard templates) and is case sensitive */
    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, template_using_index_color_lower, sizeof(template_using_index_color_lower) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#x\n", ret);
    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, template_using_index_color_upper, sizeof(template_using_index_color_upper) - 1);
    ok(ret == D3DXFERR_PARSEERROR, "RegisterTemplates returned %#x instead of %#x\n", ret, D3DXFERR_PARSEERROR);

    d3dxfile->lpVtbl->Release(d3dxfile);
}

static void process_data(ID3DXFileData *xfile_data, int level)
{
    HRESULT ret;
    char name[100];
    GUID clsid;
    GUID clsid_type;
    SIZE_T len = sizeof(name);
    int i;
    const BYTE *data;
    SIZE_T size;
    SIZE_T children;

    ret = xfile_data->lpVtbl->GetId(xfile_data, &clsid);
    ok(ret == S_OK, "ID3DXFileData_GetId failed with %#x\n", ret);
    ret = xfile_data->lpVtbl->GetName(xfile_data, name, &len);
    ok(ret == S_OK, "ID3DXFileData_GetName failed with %#x\n", ret);
    ret = xfile_data->lpVtbl->GetType(xfile_data, &clsid_type);
    ok(ret == S_OK, "IDirectXFileData_GetType failed with %#x\n", ret);
    ret = xfile_data->lpVtbl->Lock(xfile_data, &size, (const void**)&data);
    ok(ret == S_OK, "IDirectXFileData_Lock failed with %#x\n", ret);

    for (i = 0; i < level; i++)
        printf("  ");

    printf("Found object '%s' - %s - %s - %lu\n",
           len ? name : "", wine_dbgstr_guid(&clsid), wine_dbgstr_guid(&clsid_type), size);

    if (size)
    {
        int j;
        for (j = 0; j < size; j++)
        {
            if (j && !(j%16))
                printf("\n");
            printf("%02x ", data[j]);
        }
        printf("\n");
    }

    ret = xfile_data->lpVtbl->Unlock(xfile_data);
    ok(ret == S_OK, "ID3DXFileData_Unlock failed with %#x\n", ret);

    ret = xfile_data->lpVtbl->GetChildren(xfile_data, &children);
    ok(ret == S_OK, "ID3DXFileData_GetChildren failed with %#x\n", ret);

    level++;

    for (i = 0; i < children; i++)
    {
        ID3DXFileData *child;
        int j;

        ret = xfile_data->lpVtbl->GetChild(xfile_data, i, &child);
        ok(ret == S_OK, "ID3DXFileData_GetChild failed with %#x\n", ret);
        for (j = 0; j < level; j++)
            printf("  ");
        if (child->lpVtbl->IsReference(child))
            printf("Found Data Reference (%d)\n", i + 1);
        else
            printf("Found Data (%d)\n", i + 1);

        process_data(child, level);

        child->lpVtbl->Release(child);
    }
}

/* Dump an X file 'objects.x' and its related templates file 'templates.x' if they are both presents
 * Useful for debug by comparing outputs from native and builtin dlls */
static void test_dump(void)
{
    HRESULT ret;
    ULONG ref;
    ID3DXFile *xfile = NULL;
    ID3DXFileEnumObject *xfile_enum_object = NULL;
    HANDLE file;
    void *data;
    DWORD size;
    SIZE_T children;
    int i;

    /* Dump data only if there is an object and a template */
    file = CreateFileA("objects.x", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;
    CloseHandle(file);

    file = CreateFileA("templates.x", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 10000);

    if (!ReadFile(file, data, 10000, &size, NULL))
    {
        skip("Templates file is too big\n");
        goto exit;
    }

    printf("Load templates file (%u bytes)\n", size);

    ret = D3DXFileCreate(&xfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    ret = xfile->lpVtbl->RegisterTemplates(xfile, data, size);
    ok(ret == S_OK, "ID3DXFileImpl_RegisterTemplates failed with %#x\n", ret);

    ret = xfile->lpVtbl->CreateEnumObject(xfile, (void*)"objects.x", D3DXF_FILELOAD_FROMFILE, &xfile_enum_object);
    ok(ret == S_OK, "ID3DXFile_CreateEnumObject failed with %#x\n", ret);

    ret = xfile_enum_object->lpVtbl->GetChildren(xfile_enum_object, &children);
    ok(ret == S_OK, "ID3DXFileEnumObject_GetChildren failed with %#x\n", ret);

    for (i = 0; i < children; i++)
    {
        ID3DXFileData *child;
        ret = xfile_enum_object->lpVtbl->GetChild(xfile_enum_object, i, &child);
        ok(ret == S_OK, "ID3DXFileEnumObject_GetChild failed with %#x\n", ret);
        printf("\n");
        process_data(child, 0);
        child->lpVtbl->Release(child);
    }

    ref = xfile_enum_object->lpVtbl->Release(xfile_enum_object);
    ok(ref == 0, "Got refcount %u, expected 0\n", ref);

    ref = xfile->lpVtbl->Release(xfile);
    ok(ref == 0, "Got refcount %u, expected 0\n", ref);


exit:
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, data);
}

START_TEST(xfile)
{
    test_templates();
    test_lock_unlock();
    test_getname();
    test_type_index_color();
    test_dump();
}
