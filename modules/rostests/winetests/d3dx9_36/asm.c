/*
 * Copyright (C) 2008 Stefan Dösinger
 * Copyright (C) 2009 Matteo Bruni
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

#include <d3dx9.h>

#include "resources.h"

static char temp_path[MAX_PATH];

static BOOL create_file(const char *filename, const char *data, const unsigned int size, char *out_path)
{
    DWORD written;
    HANDLE hfile;
    char path[MAX_PATH];

    if (!*temp_path)
        GetTempPathA(sizeof(temp_path), temp_path);

    strcpy(path, temp_path);
    strcat(path, filename);
    hfile = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hfile == INVALID_HANDLE_VALUE)
        return FALSE;

    if (WriteFile(hfile, data, size, &written, NULL))
    {
        CloseHandle(hfile);

        if (out_path)
            strcpy(out_path, path);
        return TRUE;
    }

    CloseHandle(hfile);
    return FALSE;
}

static void delete_file(const char *filename)
{
    char path[MAX_PATH];

    strcpy(path, temp_path);
    strcat(path, filename);
    DeleteFileA(path);
}

static BOOL create_directory(const char *name)
{
    char path[MAX_PATH];

    strcpy(path, temp_path);
    strcat(path, name);
    return CreateDirectoryA(path, NULL);
}

static void delete_directory(const char *name)
{
    char path[MAX_PATH];

    strcpy(path, temp_path);
    strcat(path, name);
    RemoveDirectoryA(path);
}

static HRESULT WINAPI testD3DXInclude_open(ID3DXInclude *iface, D3DXINCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    char *buffer;
    static const char shader[] =
            "#include \"incl.vsh\"\n"
            "mov REGISTER, v0\n";
    static const char include[] = "#define REGISTER r0\nvs.1.1\n";
    static const char include2[] = "#include \"incl3.vsh\"\n";
    static const char include3[] = "vs.1.1\n";

    trace("filename %s.\n", filename);
    trace("parent_data %p: %s.\n", parent_data, parent_data ? (char *)parent_data : "(null)");

    if (!strcmp(filename, "shader.vsh"))
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(shader));
        memcpy(buffer, shader, sizeof(shader));
        *bytes = sizeof(shader);
    }
    else if (!strcmp(filename, "incl.vsh"))
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(include));
        memcpy(buffer, include, sizeof(include));
        *bytes = sizeof(include);
        /* This is included from the first D3DXAssembleShader with non-null ID3DXInclude test
         * (parent_data == NULL) and from shader.vsh / shader[] (with matching parent_data).
         * Allow both cases. */
        ok(parent_data == NULL || !strncmp(shader, parent_data, strlen(shader)), "wrong parent_data value\n");
    }
    else if (!strcmp(filename, "incl2.vsh"))
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(include2));
        memcpy(buffer, include2, sizeof(include2));
        *bytes = sizeof(include2);
    }
    else if (!strcmp(filename, "incl3.vsh"))
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(include3));
        memcpy(buffer, include3, sizeof(include3));
        *bytes = sizeof(include3);
        /* Also check for the correct parent_data content */
        ok(parent_data != NULL && !strncmp(include2, parent_data, strlen(include2)), "wrong parent_data value\n");
    }
    else if (!strcmp(filename, "include/incl3.vsh"))
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(include));
        memcpy(buffer, include, sizeof(include));
        *bytes = sizeof(include);
        ok(!parent_data, "wrong parent_data value\n");
    }
    else
    {
        ok(0, "Unexpected #include for file %s.\n", filename);
        return D3DERR_INVALIDCALL;
    }
    *data = buffer;
    return S_OK;
}

static HRESULT WINAPI testD3DXInclude_close(ID3DXInclude *iface, const void *data)
{
    HeapFree(GetProcessHeap(), 0, (void *)data);
    return S_OK;
}

static const struct ID3DXIncludeVtbl D3DXInclude_Vtbl = {
    testD3DXInclude_open,
    testD3DXInclude_close
};

struct D3DXIncludeImpl {
    ID3DXInclude ID3DXInclude_iface;
};

static void assembleshader_test(void)
{
    static const char test1[] =
        "vs.1.1\n"
        "mov DEF2, v0\n";
    static const char testincl[] =
        "#define REGISTER r0\n"
        "vs.1.1\n";
    static const char testshader[] =
        "#include \"incl.vsh\"\n"
        "mov REGISTER, v0\n";
    static const char testshader2[] =
        "#include \"incl2.vsh\"\n"
        "mov REGISTER, v0\n";
    static const char testshader3[] =
        "#include \"include/incl3.vsh\"\n"
        "mov REGISTER, v0\n";
    static const char testincl3[] =
        "#include \"incl4.vsh\"\n";
    static const char testincl4_ok[] =
        "#define REGISTER r0\n"
        "vs.1.1\n";
    static const char testincl4_wrong[] =
        "#error \"wrong include\"\n";
    HRESULT hr;
    ID3DXBuffer *shader, *messages;
    static const D3DXMACRO defines[] = {
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
    struct D3DXIncludeImpl include;
    char shader_vsh_path[MAX_PATH], shader3_vsh_path[MAX_PATH];
    static const WCHAR shader_filename_w[] = {'s','h','a','d','e','r','.','v','s','h',0};

    /* pDefines test */
    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShader(test1, strlen(test1),
                            defines, NULL, D3DXSHADER_SKIPVALIDATION,
                            &shader, &messages);
    ok(hr == D3D_OK, "pDefines test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* NULL messages test */
    shader = NULL;
    hr = D3DXAssembleShader(test1, strlen(test1),
                            defines, NULL, D3DXSHADER_SKIPVALIDATION,
                            &shader, NULL);
    ok(hr == D3D_OK, "NULL messages test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(shader) ID3DXBuffer_Release(shader);

    /* NULL shader test */
    messages = NULL;
    hr = D3DXAssembleShader(test1, strlen(test1),
                            defines, NULL, D3DXSHADER_SKIPVALIDATION,
                            NULL, &messages);
    ok(hr == D3D_OK, "NULL shader test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }

    /* pInclude test */
    shader = NULL;
    messages = NULL;
    include.ID3DXInclude_iface.lpVtbl = &D3DXInclude_Vtbl;
    hr = D3DXAssembleShader(testshader, strlen(testshader), NULL, &include.ID3DXInclude_iface,
                            D3DXSHADER_SKIPVALIDATION, &shader, &messages);
    ok(hr == D3D_OK, "pInclude test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* "unexpected #include file from memory" test */
    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShader(testshader, strlen(testshader),
                            NULL, NULL, D3DXSHADER_SKIPVALIDATION,
                            &shader, &messages);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXAssembleShader test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* recursive #include test */
    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShader(testshader2, strlen(testshader2), NULL, &include.ID3DXInclude_iface,
                            D3DXSHADER_SKIPVALIDATION, &shader, &messages);
    ok(hr == D3D_OK, "D3DXAssembleShader test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("recursive D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* #include with a path. */
    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShader(testshader3, strlen(testshader3), NULL, &include.ID3DXInclude_iface,
            D3DXSHADER_SKIPVALIDATION, &shader, &messages);
    ok(hr == D3D_OK, "D3DXAssembleShader test failed with error 0x%x - %d\n", hr, hr & 0x0000ffff);
    if (messages)
    {
        trace("Path search D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if (shader)
        ID3DXBuffer_Release(shader);

    if (create_file("shader.vsh", testshader, sizeof(testshader) - 1, shader_vsh_path))
    {
        create_file("incl.vsh", testincl, sizeof(testincl) - 1, NULL);

        /* D3DXAssembleShaderFromFile + #include test */
        shader = NULL;
        messages = NULL;
        hr = D3DXAssembleShaderFromFileA(shader_vsh_path,
                                         NULL, NULL, D3DXSHADER_SKIPVALIDATION,
                                         &shader, &messages);
        ok(hr == D3D_OK, "D3DXAssembleShaderFromFile test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
        if(messages) {
            trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if(shader) ID3DXBuffer_Release(shader);

        /* D3DXAssembleShaderFromFile + pInclude test */
        shader = NULL;
        messages = NULL;
        hr = D3DXAssembleShaderFromFileA("shader.vsh", NULL, &include.ID3DXInclude_iface,
                                         D3DXSHADER_SKIPVALIDATION, &shader, &messages);
        ok(hr == D3D_OK, "D3DXAssembleShaderFromFile + pInclude test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
        if(messages) {
            trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if(shader) ID3DXBuffer_Release(shader);

        create_file("shader3.vsh", testshader3, sizeof(testshader3) - 1, shader3_vsh_path);
        create_file("incl4.vsh", testincl4_wrong, sizeof(testincl4_wrong) - 1, NULL);
        if (create_directory("include"))
        {
            create_file("include\\incl3.vsh", testincl3, sizeof(testincl3) - 1, NULL);
            create_file("include\\incl4.vsh", testincl4_ok, sizeof(testincl4_ok) - 1, NULL);

            /* path search #include test */
            shader = NULL;
            messages = NULL;
            hr = D3DXAssembleShaderFromFileA(shader3_vsh_path, NULL, NULL,
                                             D3DXSHADER_SKIPVALIDATION,
                                             &shader, &messages);
            ok(hr == D3D_OK, "D3DXAssembleShaderFromFile path search test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
            if(messages) {
                trace("D3DXAssembleShaderFromFile path search messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
                ID3DXBuffer_Release(messages);
            }
            if(shader) ID3DXBuffer_Release(shader);
        } else skip("Couldn't create \"include\" directory\n");

        delete_file("shader.vsh");
        delete_file("incl.vsh");
        delete_file("shader3.vsh");
        delete_file("incl4.vsh");
        delete_file("include\\incl3.vsh");
        delete_file("include\\incl4.vsh");
        delete_directory("include");

        /* The main shader is also to be loaded through the ID3DXInclude object. */
        shader = NULL;
        messages = NULL;
        hr = D3DXAssembleShaderFromFileA("shader.vsh", NULL, &include.ID3DXInclude_iface,
                D3DXSHADER_SKIPVALIDATION, &shader, &messages);
        ok(hr == D3D_OK, "D3DXAssembleShaderFromFile + pInclude main shader test failed with error 0x%x - %d\n",
                hr, hr & 0x0000ffff);
        if (messages)
        {
            trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if (shader)
            ID3DXBuffer_Release(shader);

        shader = NULL;
        messages = NULL;
        hr = D3DXAssembleShaderFromFileW(shader_filename_w, NULL, &include.ID3DXInclude_iface,
                D3DXSHADER_SKIPVALIDATION, &shader, &messages);
        ok(hr == D3D_OK, "D3DXAssembleShaderFromFile + pInclude main shader test failed with error 0x%x - %d\n",
                hr, hr & 0x0000ffff);
        if (messages)
        {
            trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if (shader)
            ID3DXBuffer_Release(shader);
    } else skip("Couldn't create \"shader.vsh\"\n");

    /* NULL shader tests */
    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShader(NULL, 0,
                            NULL, NULL, D3DXSHADER_SKIPVALIDATION,
                            &shader, &messages);
    ok(hr == D3DXERR_INVALIDDATA, "NULL shader test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShaderFromFileA("nonexistent.vsh",
                                     NULL, NULL, D3DXSHADER_SKIPVALIDATION,
                                     &shader, &messages);
    ok(hr == D3DXERR_INVALIDDATA || hr == E_FAIL, /* I get this on WinXP */
        "D3DXAssembleShaderFromFile nonexistent file test failed with error 0x%x - %d\n",
        hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShaderFromFile messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* D3DXAssembleShaderFromResource test */
    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShaderFromResourceA(NULL, MAKEINTRESOURCEA(IDB_ASMSHADER),
                                         NULL, NULL, D3DXSHADER_SKIPVALIDATION,
                                         &shader, &messages);
    ok(hr == D3D_OK, "D3DXAssembleShaderFromResource test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShaderFromResource messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* D3DXAssembleShaderFromResource with missing shader resource test */
    shader = NULL;
    messages = NULL;
    hr = D3DXAssembleShaderFromResourceA(NULL, "notexisting",
                                         NULL, NULL, D3DXSHADER_SKIPVALIDATION,
                                         &shader, &messages);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXAssembleShaderFromResource NULL shader test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXAssembleShaderFromResource messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);
}

static void d3dxpreprocess_test(void)
{
    static const char testincl[] =
            "#define REGISTER r0\n"
            "vs.1.1\n";
    static const char testshader[] =
            "#include \"incl.vsh\"\n"
            "mov REGISTER, v0\n";
    static const char testshader3[] =
            "#include \"include/incl3.vsh\"\n"
            "mov REGISTER, v0\n";
    static const char testincl3[] =
            "#include \"incl4.vsh\"\n";
    static const char testincl4_ok[] =
            "#define REGISTER r0\n"
            "vs.1.1\n";
    static const char testincl4_wrong[] =
            "#error \"wrong include\"\n";
    HRESULT hr;
    ID3DXBuffer *shader, *messages;
    char shader_vsh_path[MAX_PATH], shader3_vsh_path[MAX_PATH];
    static struct D3DXIncludeImpl include = {{&D3DXInclude_Vtbl}};
    static const WCHAR shader_filename_w[] = {'s','h','a','d','e','r','.','v','s','h',0};

    if (create_file("shader.vsh", testshader, sizeof(testshader) - 1, shader_vsh_path))
    {
        create_file("incl.vsh", testincl, sizeof(testincl) - 1, NULL);
        create_file("shader3.vsh", testshader3, sizeof(testshader3) - 1, shader3_vsh_path);
        create_file("incl4.vsh", testincl4_wrong, sizeof(testincl4_wrong) - 1, NULL);
        if (create_directory("include"))
        {
            create_file("include\\incl3.vsh", testincl3, sizeof(testincl3) - 1, NULL);
            create_file("include\\incl4.vsh", testincl4_ok, sizeof(testincl4_ok) - 1, NULL);

            /* path search #include test */
            shader = NULL;
            messages = NULL;
            hr = D3DXPreprocessShaderFromFileA(shader3_vsh_path, NULL, NULL,
                                               &shader, &messages);
            ok(hr == D3D_OK, "D3DXPreprocessShaderFromFile path search test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
            if(messages) {
                trace("D3DXPreprocessShaderFromFile path search messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
                ID3DXBuffer_Release(messages);
            }
            if(shader) ID3DXBuffer_Release(shader);
        } else skip("Couldn't create \"include\" directory\n");

        /* D3DXPreprocessShaderFromFile + #include test */
        shader = NULL;
        messages = NULL;
        hr = D3DXPreprocessShaderFromFileA(shader_vsh_path,
                                           NULL, NULL,
                                           &shader, &messages);
        ok(hr == D3D_OK, "D3DXPreprocessShaderFromFile test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
        if(messages) {
            trace("D3DXPreprocessShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if(shader) ID3DXBuffer_Release(shader);

        /* D3DXPreprocessShaderFromFile + pInclude test */
        shader = NULL;
        messages = NULL;
        hr = D3DXPreprocessShaderFromFileA("shader.vsh", NULL, &include.ID3DXInclude_iface,
                                           &shader, &messages);
        ok(hr == D3D_OK, "D3DXPreprocessShaderFromFile + pInclude test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
        if(messages) {
            trace("D3DXPreprocessShader messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if(shader) ID3DXBuffer_Release(shader);

        delete_file("shader.vsh");
        delete_file("incl.vsh");
        delete_file("shader3.vsh");
        delete_file("incl4.vsh");
        delete_file("include\\incl3.vsh");
        delete_file("include\\incl4.vsh");
        delete_directory("include");

        /* The main shader is also to be loaded through the ID3DXInclude object. */
        shader = NULL;
        messages = NULL;
        hr = D3DXPreprocessShaderFromFileA("shader.vsh", NULL, &include.ID3DXInclude_iface,
                &shader, &messages);
        ok(hr == D3D_OK, "D3DXPreprocessShaderFromFile + pInclude main shader test failed with error 0x%x - %d\n",
                hr, hr & 0x0000ffff);
        if (messages)
        {
            trace("D3DXPreprocessShaderFromFile messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if (shader)
            ID3DXBuffer_Release(shader);

        shader = NULL;
        messages = NULL;
        hr = D3DXPreprocessShaderFromFileW(shader_filename_w, NULL, &include.ID3DXInclude_iface,
                &shader, &messages);
        ok(hr == D3D_OK, "D3DXPreprocessShaderFromFile + pInclude main shader test failed with error 0x%x - %d\n",
                hr, hr & 0x0000ffff);
        if (messages)
        {
            trace("D3DXPreprocessShaderFromFile messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
            ID3DXBuffer_Release(messages);
        }
        if (shader)
            ID3DXBuffer_Release(shader);
    } else skip("Couldn't create \"shader.vsh\"\n");

    /* NULL shader tests */
    shader = NULL;
    messages = NULL;
    hr = D3DXPreprocessShaderFromFileA("nonexistent.vsh",
                                       NULL, NULL,
                                       &shader, &messages);
    ok(hr == D3DXERR_INVALIDDATA || hr == E_FAIL, /* I get this on WinXP */
        "D3DXPreprocessShaderFromFile nonexistent file test failed with error 0x%x - %d\n",
        hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXPreprocessShaderFromFile messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* D3DXPreprocessShaderFromResource test */
    shader = NULL;
    messages = NULL;
    hr = D3DXPreprocessShaderFromResourceA(NULL, MAKEINTRESOURCEA(IDB_ASMSHADER),
                                           NULL, NULL,
                                           &shader, &messages);
    ok(hr == D3D_OK, "D3DXPreprocessShaderFromResource test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXPreprocessShaderFromResource messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);

    /* D3DXPreprocessShaderFromResource with missing shader resource test */
    shader = NULL;
    messages = NULL;
    hr = D3DXPreprocessShaderFromResourceA(NULL, "notexisting",
                                           NULL, NULL,
                                           &shader, &messages);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXPreprocessShaderFromResource NULL shader test failed with error 0x%x - %d\n", hr, hr & 0x0000FFFF);
    if(messages) {
        trace("D3DXPreprocessShaderFromResource messages:\n%s", (char *)ID3DXBuffer_GetBufferPointer(messages));
        ID3DXBuffer_Release(messages);
    }
    if(shader) ID3DXBuffer_Release(shader);
}

START_TEST(asm)
{
    assembleshader_test();

    d3dxpreprocess_test();
}
