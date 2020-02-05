/*
 * Copyright 2008 Luis Busquets
 * Copyright 2009 Matteo Bruni
 * Copyright 2010, 2013, 2016 Christian Costa
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

#include "config.h"
#include "wine/port.h"
#include <stdio.h>

#include "d3dx9_private.h"
#include "d3dcommon.h"
#include "d3dcompiler.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/* This function is not declared in the SDK headers yet. */
HRESULT WINAPI D3DAssemble(const void *data, SIZE_T datasize, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, UINT flags,
        ID3DBlob **shader, ID3DBlob **error_messages);

static inline BOOL is_valid_bytecode(DWORD token)
{
    return (token & 0xfffe0000) == 0xfffe0000;
}

const char * WINAPI D3DXGetPixelShaderProfile(struct IDirect3DDevice9 *device)
{
    D3DCAPS9 caps;

    TRACE("device %p\n", device);

    if (!device) return NULL;

    IDirect3DDevice9_GetDeviceCaps(device,&caps);

    switch (caps.PixelShaderVersion)
    {
    case D3DPS_VERSION(1, 1):
        return "ps_1_1";

    case D3DPS_VERSION(1, 2):
        return "ps_1_2";

    case D3DPS_VERSION(1, 3):
        return "ps_1_3";

    case D3DPS_VERSION(1, 4):
        return "ps_1_4";

    case D3DPS_VERSION(2, 0):
        if ((caps.PS20Caps.NumTemps>=22)                          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_ARBITRARYSWIZZLE)     &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_GRADIENTINSTRUCTIONS) &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_PREDICATION)          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NODEPENDENTREADLIMIT) &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))
        {
            return "ps_2_a";
        }
        if ((caps.PS20Caps.NumTemps>=32)                          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))
        {
            return "ps_2_b";
        }
        return "ps_2_0";

    case D3DPS_VERSION(3, 0):
        return "ps_3_0";
    }

    return NULL;
}

UINT WINAPI D3DXGetShaderSize(const DWORD *byte_code)
{
    const DWORD *ptr = byte_code;

    TRACE("byte_code %p\n", byte_code);

    if (!ptr) return 0;

    /* Look for the END token, skipping the VERSION token */
    while (*++ptr != D3DSIO_END)
    {
        /* Skip comments */
        if ((*ptr & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
        {
            ptr += ((*ptr & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT);
        }
    }
    ++ptr;

    /* Return the shader size in bytes */
    return (ptr - byte_code) * sizeof(*ptr);
}

DWORD WINAPI D3DXGetShaderVersion(const DWORD *byte_code)
{
    TRACE("byte_code %p\n", byte_code);

    return byte_code ? *byte_code : 0;
}

const char * WINAPI D3DXGetVertexShaderProfile(struct IDirect3DDevice9 *device)
{
    D3DCAPS9 caps;

    TRACE("device %p\n", device);

    if (!device) return NULL;

    IDirect3DDevice9_GetDeviceCaps(device,&caps);

    switch (caps.VertexShaderVersion)
    {
    case D3DVS_VERSION(1, 1):
        return "vs_1_1";
    case D3DVS_VERSION(2, 0):
        if ((caps.VS20Caps.NumTemps>=13) &&
            (caps.VS20Caps.DynamicFlowControlDepth==24) &&
            (caps.VS20Caps.Caps&D3DPS20CAPS_PREDICATION))
        {
            return "vs_2_a";
        }
        return "vs_2_0";
    case D3DVS_VERSION(3, 0):
        return "vs_3_0";
    }

    return NULL;
}

HRESULT WINAPI D3DXFindShaderComment(const DWORD *byte_code, DWORD fourcc, const void **data, UINT *size)
{
    const DWORD *ptr = byte_code;
    DWORD version;

    TRACE("byte_code %p, fourcc %x, data %p, size %p\n", byte_code, fourcc, data, size);

    if (data) *data = NULL;
    if (size) *size = 0;

    if (!byte_code) return D3DERR_INVALIDCALL;

    version = *ptr >> 16;
    if (version != 0x4658         /* FX */
            && version != 0x5458  /* TX */
            && version != 0x7ffe
            && version != 0x7fff
            && version != 0xfffe  /* VS */
            && version != 0xffff) /* PS */
    {
        WARN("Invalid data supplied\n");
        return D3DXERR_INVALIDDATA;
    }

    while (*++ptr != D3DSIO_END)
    {
        /* Check if it is a comment */
        if ((*ptr & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
        {
            DWORD comment_size = (*ptr & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;

            /* Check if this is the comment we are looking for */
            if (*(ptr + 1) == fourcc)
            {
                UINT ctab_size = (comment_size - 1) * sizeof(DWORD);
                const void *ctab_data = ptr + 2;
                if (size)
                    *size = ctab_size;
                if (data)
                    *data = ctab_data;
                TRACE("Returning comment data at %p with size %d\n", ctab_data, ctab_size);
                return D3D_OK;
            }
            ptr += comment_size;
        }
    }

    return S_FALSE;
}

HRESULT WINAPI D3DXAssembleShader(const char *data, UINT data_len, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    HRESULT hr;

    TRACE("data %p, data_len %u, defines %p, include %p, flags %#x, shader %p, error_messages %p\n",
          data, data_len, defines, include, flags, shader, error_messages);

    /* Forward to d3dcompiler: the parameter types aren't really different,
       the actual data types are equivalent */
    hr = D3DAssemble(data, data_len, NULL, (D3D_SHADER_MACRO *)defines,
                     (ID3DInclude *)include, flags, (ID3DBlob **)shader,
                     (ID3DBlob **)error_messages);

    if(hr == E_FAIL) hr = D3DXERR_INVALIDDATA;
    return hr;
}

static const void *main_file_data;

static CRITICAL_SECTION_DEBUG from_file_mutex_debug =
{
    0, 0, &from_file_mutex,
    {
        &from_file_mutex_debug.ProcessLocksList,
        &from_file_mutex_debug.ProcessLocksList
    },
    0, 0, {(DWORD_PTR)(__FILE__ ": from_file_mutex")}
};
CRITICAL_SECTION from_file_mutex = {&from_file_mutex_debug, -1, 0, 0, 0, 0};

/* D3DXInclude private implementation, used to implement
 * D3DXAssembleShaderFromFile() from D3DXAssembleShader(). */
/* To be able to correctly resolve include search paths we have to store the
 * pathname of each include file. We store the pathname pointer right before
 * the file data. */
static HRESULT WINAPI d3dx_include_from_file_open(ID3DXInclude *iface, D3DXINCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    const char *p, *parent_name = "";
    char *pathname = NULL, *ptr;
    char **buffer = NULL;
    HANDLE file;
    UINT size;

    if (parent_data)
    {
        parent_name = *((const char **)parent_data - 1);
    }
    else
    {
        if (main_file_data)
            parent_name = *((const char **)main_file_data - 1);
    }

    TRACE("Looking up include file %s, parent %s.\n", debugstr_a(filename), debugstr_a(parent_name));

    if ((p = strrchr(parent_name, '\\')))
        ++p;
    else
        p = parent_name;
    pathname = HeapAlloc(GetProcessHeap(), 0, (p - parent_name) + strlen(filename) + 1);
    if(!pathname)
        return HRESULT_FROM_WIN32(GetLastError());

    memcpy(pathname, parent_name, p - parent_name);
    strcpy(pathname + (p - parent_name), filename);
    ptr = pathname + (p - parent_name);
    while (*ptr)
    {
        if (*ptr == '/')
            *ptr = '\\';
        ++ptr;
    }

    file = CreateFileA(pathname, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(file == INVALID_HANDLE_VALUE)
        goto error;

    TRACE("Include file found at pathname = %s\n", debugstr_a(pathname));

    size = GetFileSize(file, NULL);
    if(size == INVALID_FILE_SIZE)
        goto error;

    buffer = HeapAlloc(GetProcessHeap(), 0, size + sizeof(char *));
    if(!buffer)
        goto error;
    *buffer = pathname;
    if(!ReadFile(file, buffer + 1, size, bytes, NULL))
        goto error;

    *data = buffer + 1;
    if (!main_file_data)
        main_file_data = *data;

    CloseHandle(file);
    return S_OK;

error:
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, pathname);
    HeapFree(GetProcessHeap(), 0, buffer);
    return HRESULT_FROM_WIN32(GetLastError());
}

static HRESULT WINAPI d3dx_include_from_file_close(ID3DXInclude *iface, const void *data)
{
    HeapFree(GetProcessHeap(), 0, *((char **)data - 1));
    HeapFree(GetProcessHeap(), 0, (char **)data - 1);
    if (main_file_data == data)
        main_file_data = NULL;
    return S_OK;
}

const struct ID3DXIncludeVtbl d3dx_include_from_file_vtbl =
{
    d3dx_include_from_file_open,
    d3dx_include_from_file_close
};

HRESULT WINAPI D3DXAssembleShaderFromFileA(const char *filename, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    WCHAR *filename_w;
    DWORD len;
    HRESULT ret;

    TRACE("filename %s, defines %p, include %p, flags %#x, shader %p, error_messages %p.\n",
            debugstr_a(filename), defines, include, flags, shader, error_messages);

    if (!filename) return D3DXERR_INVALIDDATA;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename_w) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    ret = D3DXAssembleShaderFromFileW(filename_w, defines, include, flags, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, filename_w);
    return ret;
}

HRESULT WINAPI D3DXAssembleShaderFromFileW(const WCHAR *filename, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    const void *buffer;
    DWORD len;
    HRESULT hr;
    struct d3dx_include_from_file include_from_file;
    char *filename_a;

    TRACE("filename %s, defines %p, include %p, flags %#x, shader %p, error_messages %p.\n",
            debugstr_w(filename), defines, include, flags, shader, error_messages);

    if(!include)
    {
        include_from_file.ID3DXInclude_iface.lpVtbl = &d3dx_include_from_file_vtbl;
        include = &include_from_file.ID3DXInclude_iface;
    }

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    filename_a = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
    if (!filename_a)
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);

    EnterCriticalSection(&from_file_mutex);
    hr = ID3DXInclude_Open(include, D3DXINC_LOCAL, filename_a, NULL, &buffer, &len);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&from_file_mutex);
        HeapFree(GetProcessHeap(), 0, filename_a);
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DXAssembleShader(buffer, len, defines, include, flags, shader, error_messages);

    ID3DXInclude_Close(include, buffer);
    LeaveCriticalSection(&from_file_mutex);
    HeapFree(GetProcessHeap(), 0, filename_a);
    return hr;
}

HRESULT WINAPI D3DXAssembleShaderFromResourceA(HMODULE module, const char *resource, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    void *buffer;
    HRSRC res;
    DWORD len;

    TRACE("module %p, resource %s, defines %p, include %p, flags %#x, shader %p, error_messages %p.\n",
            module, debugstr_a(resource), defines, include, flags, shader, error_messages);

    if (!(res = FindResourceA(module, resource, (const char *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, &buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXAssembleShader(buffer, len, defines, include, flags,
                              shader, error_messages);
}

HRESULT WINAPI D3DXAssembleShaderFromResourceW(HMODULE module, const WCHAR *resource, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    void *buffer;
    HRSRC res;
    DWORD len;

    TRACE("module %p, resource %s, defines %p, include %p, flags %#x, shader %p, error_messages %p.\n",
            module, debugstr_w(resource), defines, include, flags, shader, error_messages);

    if (!(res = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, &buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXAssembleShader(buffer, len, defines, include, flags,
                              shader, error_messages);
}

HRESULT WINAPI D3DXCompileShader(const char *data, UINT length, const D3DXMACRO *defines,
        ID3DXInclude *include, const char *function, const char *profile, DWORD flags,
        ID3DXBuffer **shader, ID3DXBuffer **error_msgs, ID3DXConstantTable **constant_table)
{
    HRESULT hr;

    TRACE("data %s, length %u, defines %p, include %p, function %s, profile %s, "
            "flags %#x, shader %p, error_msgs %p, constant_table %p.\n",
            debugstr_a(data), length, defines, include, debugstr_a(function), debugstr_a(profile),
            flags, shader, error_msgs, constant_table);

    hr = D3DCompile(data, length, NULL, (D3D_SHADER_MACRO *)defines, (ID3DInclude *)include,
                    function, profile, flags, 0, (ID3DBlob **)shader, (ID3DBlob **)error_msgs);

    if (SUCCEEDED(hr) && constant_table)
    {
        hr = D3DXGetShaderConstantTable(ID3DXBuffer_GetBufferPointer(*shader), constant_table);
        if (FAILED(hr))
        {
            ID3DXBuffer_Release(*shader);
            *shader = NULL;
        }
    }

    /* Filter out D3DCompile warning messages that are not present with D3DCompileShader */
    if (SUCCEEDED(hr) && error_msgs && *error_msgs)
    {
        char *messages = ID3DXBuffer_GetBufferPointer(*error_msgs);
        DWORD size     = ID3DXBuffer_GetBufferSize(*error_msgs);

        /* Ensure messages are null terminated for safe processing */
        if (size) messages[size - 1] = 0;

        while (size > 1)
        {
            char *prev, *next;

            /* Warning has the form "warning X3206: ... implicit truncation of vector type"
               but we only search for "X3206:" in case d3dcompiler_43 has localization */
            prev = next = strstr(messages, "X3206:");
            if (!prev) break;

            /* get pointer to beginning and end of current line */
            while (prev > messages && *(prev - 1) != '\n') prev--;
            while (next < messages + size - 1 && *next != '\n') next++;
            if (next < messages + size - 1 && *next == '\n') next++;

            memmove(prev, next, messages + size - next);
            size -= (next - prev);
        }

        /* Only return a buffer if the resulting string is not empty as some apps depend on that */
        if (size <= 1)
        {
            ID3DXBuffer_Release(*error_msgs);
            *error_msgs = NULL;
        }
    }

    return hr;
}

HRESULT WINAPI D3DXCompileShaderFromFileA(const char *filename, const D3DXMACRO *defines,
        ID3DXInclude *include, const char *entrypoint, const char *profile, DWORD flags,
        ID3DXBuffer **shader, ID3DXBuffer **error_messages, ID3DXConstantTable **constant_table)
{
    WCHAR *filename_w;
    DWORD len;
    HRESULT ret;

    TRACE("filename %s, defines %p, include %p, entrypoint %s, profile %s, "
            "flags %#x, shader %p, error_messages %p, constant_table %p.\n",
            debugstr_a(filename), defines, include, debugstr_a(entrypoint),
            debugstr_a(profile), flags, shader, error_messages, constant_table);

    if (!filename) return D3DXERR_INVALIDDATA;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename_w) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    ret = D3DXCompileShaderFromFileW(filename_w, defines, include,
                                     entrypoint, profile, flags,
                                     shader, error_messages, constant_table);

    HeapFree(GetProcessHeap(), 0, filename_w);
    return ret;
}

HRESULT WINAPI D3DXCompileShaderFromFileW(const WCHAR *filename, const D3DXMACRO *defines,
        ID3DXInclude *include, const char *entrypoint, const char *profile, DWORD flags,
        ID3DXBuffer **shader, ID3DXBuffer **error_messages, ID3DXConstantTable **constant_table)
{
    const void *buffer;
    DWORD len, filename_len;
    HRESULT hr;
    struct d3dx_include_from_file include_from_file;
    char *filename_a;

    TRACE("filename %s, defines %p, include %p, entrypoint %s, profile %s, "
            "flags %#x, shader %p, error_messages %p, constant_table %p.\n",
            debugstr_w(filename), defines, include, debugstr_a(entrypoint), debugstr_a(profile),
            flags, shader, error_messages, constant_table);

    if (!include)
    {
        include_from_file.ID3DXInclude_iface.lpVtbl = &d3dx_include_from_file_vtbl;
        include = &include_from_file.ID3DXInclude_iface;
    }

    filename_len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    filename_a = HeapAlloc(GetProcessHeap(), 0, filename_len * sizeof(char));
    if (!filename_a)
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, filename_len, NULL, NULL);

    EnterCriticalSection(&from_file_mutex);
    hr = ID3DXInclude_Open(include, D3DXINC_LOCAL, filename_a, NULL, &buffer, &len);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&from_file_mutex);
        HeapFree(GetProcessHeap(), 0, filename_a);
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DCompile(buffer, len, filename_a, (const D3D_SHADER_MACRO *)defines,
                    (ID3DInclude *)include, entrypoint, profile, flags, 0,
                    (ID3DBlob **)shader, (ID3DBlob **)error_messages);

    if (SUCCEEDED(hr) && constant_table)
        hr = D3DXGetShaderConstantTable(ID3DXBuffer_GetBufferPointer(*shader),
                                        constant_table);

    ID3DXInclude_Close(include, buffer);
    LeaveCriticalSection(&from_file_mutex);
    HeapFree(GetProcessHeap(), 0, filename_a);
    return hr;
}

HRESULT WINAPI D3DXCompileShaderFromResourceA(HMODULE module, const char *resource, const D3DXMACRO *defines,
        ID3DXInclude *include, const char *entrypoint, const char *profile, DWORD flags,
        ID3DXBuffer **shader, ID3DXBuffer **error_messages, ID3DXConstantTable **constant_table)
{
    void *buffer;
    HRSRC res;
    DWORD len;

    TRACE("module %p, resource %s, defines %p, include %p, entrypoint %s, profile %s, "
            "flags %#x, shader %p, error_messages %p, constant_table %p.\n",
            module, debugstr_a(resource), defines, include, debugstr_a(entrypoint), debugstr_a(profile),
            flags, shader, error_messages, constant_table);

    if (!(res = FindResourceA(module, resource, (const char *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, &buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXCompileShader(buffer, len, defines, include, entrypoint, profile,
                             flags, shader, error_messages, constant_table);
}

HRESULT WINAPI D3DXCompileShaderFromResourceW(HMODULE module, const WCHAR *resource, const D3DXMACRO *defines,
        ID3DXInclude *include, const char *entrypoint, const char *profile, DWORD flags,
        ID3DXBuffer **shader, ID3DXBuffer **error_messages, ID3DXConstantTable **constant_table)
{
    void *buffer;
    HRSRC res;
    DWORD len;

    TRACE("module %p, resource %s, defines %p, include %p, entrypoint %s, profile %s, "
            "flags %#x, shader %p, error_messages %p, constant_table %p.\n",
            module, debugstr_w(resource), defines, include, debugstr_a(entrypoint), debugstr_a(profile),
            flags, shader, error_messages, constant_table);

    if (!(res = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, &buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXCompileShader(buffer, len, defines, include, entrypoint, profile,
                             flags, shader, error_messages, constant_table);
}

HRESULT WINAPI D3DXPreprocessShader(const char *data, UINT data_len, const D3DXMACRO *defines,
        ID3DXInclude *include, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    TRACE("data %s, data_len %u, defines %p, include %p, shader %p, error_messages %p.\n",
            debugstr_a(data), data_len, defines, include, shader, error_messages);

    return D3DPreprocess(data, data_len, NULL,
                         (const D3D_SHADER_MACRO *)defines, (ID3DInclude *)include,
                         (ID3DBlob **)shader, (ID3DBlob **)error_messages);
}

HRESULT WINAPI D3DXPreprocessShaderFromFileA(const char *filename, const D3DXMACRO *defines,
        ID3DXInclude *include, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    WCHAR *filename_w = NULL;
    DWORD len;
    HRESULT ret;

    TRACE("filename %s, defines %p, include %p, shader %p, error_messages %p.\n",
            debugstr_a(filename), defines, include, shader, error_messages);

    if (!filename) return D3DXERR_INVALIDDATA;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename_w) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    ret = D3DXPreprocessShaderFromFileW(filename_w, defines, include, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, filename_w);
    return ret;
}

HRESULT WINAPI D3DXPreprocessShaderFromFileW(const WCHAR *filename, const D3DXMACRO *defines,
        ID3DXInclude *include, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    const void *buffer;
    DWORD len;
    HRESULT hr;
    struct d3dx_include_from_file include_from_file;
    char *filename_a;

    TRACE("filename %s, defines %p, include %p, shader %p, error_messages %p.\n",
            debugstr_w(filename), defines, include, shader, error_messages);

    if (!include)
    {
        include_from_file.ID3DXInclude_iface.lpVtbl = &d3dx_include_from_file_vtbl;
        include = &include_from_file.ID3DXInclude_iface;
    }

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    filename_a = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
    if (!filename_a)
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);

    EnterCriticalSection(&from_file_mutex);
    hr = ID3DXInclude_Open(include, D3DXINC_LOCAL, filename_a, NULL, &buffer, &len);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&from_file_mutex);
        HeapFree(GetProcessHeap(), 0, filename_a);
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DPreprocess(buffer, len, NULL,
                       (const D3D_SHADER_MACRO *)defines,
                       (ID3DInclude *) include,
                       (ID3DBlob **)shader, (ID3DBlob **)error_messages);

    ID3DXInclude_Close(include, buffer);
    LeaveCriticalSection(&from_file_mutex);
    HeapFree(GetProcessHeap(), 0, filename_a);
    return hr;
}

HRESULT WINAPI D3DXPreprocessShaderFromResourceA(HMODULE module, const char *resource, const D3DXMACRO *defines,
        ID3DXInclude *include, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    void *buffer;
    HRSRC res;
    DWORD len;

    TRACE("module %p, resource %s, defines %p, include %p, shader %p, error_messages %p.\n",
            module, debugstr_a(resource), defines, include, shader, error_messages);

    if (!(res = FindResourceA(module, resource, (const char *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, &buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXPreprocessShader(buffer, len, defines, include,
                                shader, error_messages);
}

HRESULT WINAPI D3DXPreprocessShaderFromResourceW(HMODULE module, const WCHAR *resource, const D3DXMACRO *defines,
        ID3DXInclude *include, ID3DXBuffer **shader, ID3DXBuffer **error_messages)
{
    void *buffer;
    HRSRC res;
    DWORD len;

    TRACE("module %p, resource %s, defines %p, include %p, shader %p, error_messages %p.\n",
            module, debugstr_w(resource), defines, include, shader, error_messages);

    if (!(res = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, &buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXPreprocessShader(buffer, len, defines, include,
                                shader, error_messages);

}

struct ID3DXConstantTableImpl {
    ID3DXConstantTable ID3DXConstantTable_iface;
    LONG ref;
    char *ctab;
    DWORD size;
    D3DXCONSTANTTABLE_DESC desc;
    struct ctab_constant *constants;
};

static void free_constant(struct ctab_constant *constant)
{
    if (constant->constants)
    {
        UINT i, count = constant->desc.Elements > 1 ? constant->desc.Elements : constant->desc.StructMembers;

        for (i = 0; i < count; ++i)
        {
            free_constant(&constant->constants[i]);
        }
        HeapFree(GetProcessHeap(), 0, constant->constants);
    }
}

static void free_constant_table(struct ID3DXConstantTableImpl *table)
{
    if (table->constants)
    {
        UINT i;

        for (i = 0; i < table->desc.Constants; ++i)
        {
            free_constant(&table->constants[i]);
        }
        HeapFree(GetProcessHeap(), 0, table->constants);
    }
    HeapFree(GetProcessHeap(), 0, table->ctab);
}

static inline struct ID3DXConstantTableImpl *impl_from_ID3DXConstantTable(ID3DXConstantTable *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXConstantTableImpl, ID3DXConstantTable_iface);
}

static inline BOOL is_vertex_shader(DWORD version)
{
    return (version & 0xffff0000) == 0xfffe0000;
}

static inline D3DXHANDLE handle_from_constant(struct ctab_constant *constant)
{
    return (D3DXHANDLE)constant;
}

static struct ctab_constant *get_constant_by_name(struct ID3DXConstantTableImpl *table,
        struct ctab_constant *constant, const char *name);

static struct ctab_constant *get_constant_element_by_name(struct ctab_constant *constant, const char *name)
{
    const char *part;
    UINT element;

    TRACE("constant %p, name %s\n", constant, debugstr_a(name));

    if (!name || !*name) return NULL;

    element = atoi(name);
    part = strchr(name, ']') + 1;

    if (constant->desc.Elements > element)
    {
        struct ctab_constant *c = constant->constants ? &constant->constants[element] : constant;

        switch (*part++)
        {
            case '.':
                return get_constant_by_name(NULL, c, part);

            case '[':
                return get_constant_element_by_name(c, part);

            case '\0':
                TRACE("Returning parameter %p\n", c);
                return c;

            default:
                FIXME("Unhandled case \"%c\"\n", *--part);
                break;
        }
    }

    TRACE("Constant not found\n");
    return NULL;
}

static struct ctab_constant *get_constant_by_name(struct ID3DXConstantTableImpl *table,
        struct ctab_constant *constant, const char *name)
{
    UINT i, count, length;
    struct ctab_constant *handles;
    const char *part;

    TRACE("table %p, constant %p, name %s\n", table, constant, debugstr_a(name));

    if (!name || !*name) return NULL;

    if (!constant)
    {
        count = table->desc.Constants;
        handles = table->constants;
    }
    else
    {
        count = constant->desc.StructMembers;
        handles = constant->constants;
    }

    length = strcspn(name, "[.");
    part = name + length;

    for (i = 0; i < count; i++)
    {
        if (strlen(handles[i].desc.Name) == length && !strncmp(handles[i].desc.Name, name, length))
        {
            switch (*part++)
            {
                case '.':
                    return get_constant_by_name(NULL, &handles[i], part);

                case '[':
                    return get_constant_element_by_name(&handles[i], part);

                default:
                    TRACE("Returning parameter %p\n", &handles[i]);
                    return &handles[i];
            }
        }
    }

    TRACE("Constant not found\n");
    return NULL;
}

static struct ctab_constant *is_valid_sub_constant(struct ctab_constant *parent, D3DXHANDLE handle)
{
    struct ctab_constant *c;
    UINT i, count;

    /* all variable have at least elements = 1, but not always elements */
    if (!parent->constants) return NULL;

    count = parent->desc.Elements > 1 ? parent->desc.Elements : parent->desc.StructMembers;
    for (i = 0; i < count; ++i)
    {
        if (handle_from_constant(&parent->constants[i]) == handle)
            return &parent->constants[i];

        c = is_valid_sub_constant(&parent->constants[i], handle);
        if (c) return c;
    }

    return NULL;
}

static inline struct ctab_constant *get_valid_constant(struct ID3DXConstantTableImpl *table, D3DXHANDLE handle)
{
    struct ctab_constant *c;
    UINT i;

    if (!handle) return NULL;

    for (i = 0; i < table->desc.Constants; ++i)
    {
        if (handle_from_constant(&table->constants[i]) == handle)
            return &table->constants[i];

        c = is_valid_sub_constant(&table->constants[i], handle);
        if (c) return c;
    }

    return get_constant_by_name(table, NULL, handle);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXConstantTableImpl_QueryInterface(ID3DXConstantTable *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXBuffer) ||
        IsEqualGUID(riid, &IID_ID3DXConstantTable))
    {
        ID3DXConstantTable_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("Interface %s not found.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXConstantTableImpl_AddRef(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(): AddRef from %d\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXConstantTableImpl_Release(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %d\n", This, ref + 1);

    if (!ref)
    {
        free_constant_table(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBuffer methods ***/
static void * WINAPI ID3DXConstantTableImpl_GetBufferPointer(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *table = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p.\n", iface);

    return table->ctab;
}

static DWORD WINAPI ID3DXConstantTableImpl_GetBufferSize(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->()\n", This);

    return This->size;
}

/*** ID3DXConstantTable methods ***/
static HRESULT WINAPI ID3DXConstantTableImpl_GetDesc(ID3DXConstantTable *iface, D3DXCONSTANTTABLE_DESC *desc)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p)\n", This, desc);

    if (!desc)
        return D3DERR_INVALIDCALL;

    *desc = This->desc;

    return D3D_OK;
}

const struct ctab_constant *d3dx_shader_get_ctab_constant(ID3DXConstantTable *iface, D3DXHANDLE constant)
{
    struct ID3DXConstantTableImpl *ctab = impl_from_ID3DXConstantTable(iface);

    return get_valid_constant(ctab, constant);
}

static HRESULT WINAPI ID3DXConstantTableImpl_GetConstantDesc(ID3DXConstantTable *iface, D3DXHANDLE constant,
                                                             D3DXCONSTANT_DESC *desc, UINT *count)
{
    struct ID3DXConstantTableImpl *ctab = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(ctab, constant);

    TRACE("(%p)->(%p, %p, %p)\n", ctab, constant, desc, count);

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    if (desc) *desc = c->desc;
    if (count) *count = 1;

    return D3D_OK;
}

static UINT WINAPI ID3DXConstantTableImpl_GetSamplerIndex(ID3DXConstantTable *iface, D3DXHANDLE constant)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("(%p)->(%p)\n", This, constant);

    if (!c || c->desc.RegisterSet != D3DXRS_SAMPLER)
    {
        WARN("Invalid argument specified\n");
        return (UINT)-1;
    }

    TRACE("Returning RegisterIndex %u\n", c->desc.RegisterIndex);
    return c->desc.RegisterIndex;
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstant(ID3DXConstantTable *iface, D3DXHANDLE constant, UINT index)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c;

    TRACE("(%p)->(%p, %d)\n", This, constant, index);

    if (constant)
    {
        c = get_valid_constant(This, constant);
        if (c && index < c->desc.StructMembers)
        {
            c = &c->constants[index];
            TRACE("Returning constant %p\n", c);
            return handle_from_constant(c);
        }
    }
    else
    {
        if (index < This->desc.Constants)
        {
            c = &This->constants[index];
            TRACE("Returning constant %p\n", c);
            return handle_from_constant(c);
        }
    }

    WARN("Index out of range\n");
    return NULL;
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstantByName(ID3DXConstantTable *iface,
        D3DXHANDLE constant, const char *name)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("iface %p, constant %p, name %s.\n", iface, constant, debugstr_a(name));

    c = get_constant_by_name(This, c, name);
    TRACE("Returning constant %p\n", c);

    return handle_from_constant(c);
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstantElement(ID3DXConstantTable *iface, D3DXHANDLE constant, UINT index)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("(%p)->(%p, %d)\n", This, constant, index);

    if (c && index < c->desc.Elements)
    {
        if (c->desc.Elements > 1) c = &c->constants[index];
        TRACE("Returning constant %p\n", c);
        return handle_from_constant(c);
    }

    WARN("Invalid argument specified\n");
    return NULL;
}

static inline DWORD get_index(const void **indata, UINT index, BOOL is_pointer)
{
    if (!indata)
        return 0;

    if (is_pointer)
        return ((DWORD **)indata)[index / 16][index % 16];

    return (*((DWORD **)indata))[index];
}

static UINT set(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device, struct ctab_constant *constant,
        const void **indata, D3DXPARAMETER_TYPE intype, UINT *size, UINT incol, D3DXPARAMETER_CLASS inclass, UINT index,
        BOOL is_pointer)
{
    D3DXCONSTANT_DESC *desc = &constant->desc;
    UINT l, i, regcount = 1, regsize = 1, cin = 1, rin = 1, ret, last = 0;
    DWORD tmp;

    /* size too small to set anything */
    if (*size < desc->Rows * desc->Columns)
    {
        *size = 0;
        return 0;
    }

    /* D3DXPC_STRUCT is somewhat special */
    if (desc->Class == D3DXPC_STRUCT)
    {
        /*
         * Struct array sets the last complete input to the first struct element, all other
         * elements are not set.
         * E.g.: struct {int i;} s1[2];
         *       SetValue(device, "s1", [1, 2], 8) => s1 = {2, x};
         *
         *       struct {int i; int n} s2[2];
         *       SetValue(device, "s2", [1, 2, 3, 4, 5], 20) => s1 = {{3, 4}, {x, x}};
         */
        if (desc->Elements > 1)
        {
            UINT offset = *size / (desc->Rows * desc->Columns) - 1;

            offset = min(desc->Elements - 1, offset);
            last = offset * desc->Rows * desc->Columns;

            if ((is_pointer || inclass == D3DXPC_MATRIX_ROWS) && desc->RegisterSet != D3DXRS_BOOL)
            {
                set(table, device, &constant->constants[0], NULL, intype, size, incol, inclass, 0, is_pointer);
            }
            else
            {
                last += set(table, device, &constant->constants[0], indata, intype, size, incol, inclass,
                        index + last, is_pointer);
            }
        }
        else
        {
            /*
             * D3DXRS_BOOL is always set. As there are only 16 bools and there are
             * exactly 16 input values, use matrix transpose.
             */
            if (inclass == D3DXPC_MATRIX_ROWS && desc->RegisterSet == D3DXRS_BOOL)
            {
                D3DXMATRIX mat, *m, min;
                D3DXMatrixTranspose(&mat, &min);

                if (is_pointer)
                    min = *(D3DXMATRIX *)(indata[index / 16]);
                else
                    min = **(D3DXMATRIX **)indata;

                D3DXMatrixTranspose(&mat, &min);
                m = &mat;
                for (i = 0; i < desc->StructMembers; ++i)
                {
                    last += set(table, device, &constant->constants[i], (const void **)&m, intype, size, incol,
                            D3DXPC_SCALAR, index + last, is_pointer);
                }
            }
            /*
             * For pointers or for matrix rows, only the first member is set.
             * All other members are set to 0. This is not true for D3DXRS_BOOL.
             * E.g.: struct {int i; int n} s;
             *       SetValue(device, "s", [1, 2], 8) => s = {1, 0};
             */
            else if ((is_pointer || inclass == D3DXPC_MATRIX_ROWS) && desc->RegisterSet != D3DXRS_BOOL)
            {
                last = set(table, device, &constant->constants[0], indata, intype, size, incol, inclass,
                        index + last, is_pointer);

                for (i = 1; i < desc->StructMembers; ++i)
                {
                    set(table, device, &constant->constants[i], NULL, intype, size, incol, inclass, 0, is_pointer);
                }
            }
            else
            {
                for (i = 0; i < desc->StructMembers; ++i)
                {
                    last += set(table, device, &constant->constants[i], indata, intype, size, incol, D3DXPC_SCALAR,
                            index + last, is_pointer);
                }
            }
        }

        return last;
    }

    /* elements */
    if (desc->Elements > 1)
    {
        for (i = 0; i < desc->Elements && *size > 0; ++i)
        {
            last += set(table, device, &constant->constants[i], indata, intype, size, incol, inclass,
                    index + last, is_pointer);

            /* adjust the vector size for matrix rows */
            if (inclass == D3DXPC_MATRIX_ROWS && desc->Class == D3DXPC_VECTOR && (i % 4) == 3)
            {
                last += 12;
                *size = *size < 12 ? 0 : *size - 12;
            }
        }

        return last;
    }

    switch (desc->Class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
            regcount = min(desc->RegisterCount, desc->Rows);
            if (inclass == D3DXPC_MATRIX_ROWS) cin = incol;
            else rin = incol;
            regsize = desc->Columns;
            break;

        case D3DXPC_MATRIX_COLUMNS:
            regcount = min(desc->RegisterCount, desc->Columns);
            if (inclass == D3DXPC_MATRIX_ROWS) rin = incol;
            else cin = incol;
            regsize = desc->Rows;
            break;

        default:
            FIXME("Unhandled variable class %s\n", debug_d3dxparameter_class(desc->Class));
            return 0;
    }

    /* specific stuff for different in types */
    switch (inclass)
    {
        case D3DXPC_SCALAR:
            ret = desc->Columns * desc->Rows;
            *size -= desc->Columns * desc->Rows;
            break;

        case D3DXPC_VECTOR:
            switch (desc->Class)
            {
                case D3DXPC_MATRIX_ROWS:
                    if (*size < regcount * 4)
                    {
                        *size = 0;
                        return 0;
                    }
                    ret = 4 * regcount;
                    *size -= 4 * regcount;
                    break;

                case D3DXPC_MATRIX_COLUMNS:
                    ret = 4 * regsize;
                    *size -= 4 * regcount;
                    break;

                case D3DXPC_SCALAR:
                    ret = 1;
                    *size -= ret;
                    break;

                case D3DXPC_VECTOR:
                    ret = 4;
                    *size -= ret;
                    break;

                default:
                    FIXME("Unhandled variable class %s\n", debug_d3dxparameter_class(desc->Class));
                    return 0;
            }
            break;

        case D3DXPC_MATRIX_ROWS:
            switch (desc->Class)
            {
                case D3DXPC_MATRIX_ROWS:
                case D3DXPC_MATRIX_COLUMNS:
                    if (*size < 16)
                    {
                        *size = 0;
                        return 0;
                    }
                    ret = 16;
                    break;

                case D3DXPC_SCALAR:
                    ret = 4;
                    break;

                case D3DXPC_VECTOR:
                    ret = 1;
                    break;

                default:
                    FIXME("Unhandled variable class %s\n", debug_d3dxparameter_class(desc->Class));
                    return 0;
            }
            *size -= ret;
            break;

        case D3DXPC_MATRIX_COLUMNS:
            switch (desc->Class)
            {
                case D3DXPC_MATRIX_ROWS:
                case D3DXPC_MATRIX_COLUMNS:
                    if (*size < 16)
                    {
                        *size = 0;
                        return 0;
                    }
                    ret = 16;
                    break;

                case D3DXPC_SCALAR:
                    ret = 1;
                    break;

                case D3DXPC_VECTOR:
                    ret = 4;
                    break;

                default:
                    FIXME("Unhandled variable class %s\n", debug_d3dxparameter_class(desc->Class));
                    return 0;
            }
            *size -= ret;
            break;

        default:
            FIXME("Unhandled variable class %s\n", debug_d3dxparameter_class(inclass));
            return 0;
    }

    /* set the registers */
    switch (desc->RegisterSet)
    {
        case D3DXRS_BOOL:
            regcount = min(desc->RegisterCount, desc->Columns * desc->Rows);
            l = 0;
            for (i = 0; i < regcount; ++i)
            {
                BOOL out;
                DWORD t = get_index(indata, index + i / regsize * rin + l * cin, is_pointer);

                set_number(&tmp, desc->Type, &t, intype);
                set_number(&out, D3DXPT_BOOL, &tmp, desc->Type);
                if (is_vertex_shader(table->desc.Version))
                    IDirect3DDevice9_SetVertexShaderConstantB(device, desc->RegisterIndex + i, &out, 1);
                else
                    IDirect3DDevice9_SetPixelShaderConstantB(device, desc->RegisterIndex + i, &out, 1);

                if (++l >= regsize) l = 0;
            }
            return ret;

        case D3DXRS_INT4:
            for (i = 0; i < regcount; ++i)
            {
                INT vec[4] = {0, 0, 1, 0};

                for (l = 0; l < regsize; ++l)
                {
                    DWORD t = get_index(indata, index + i * rin + l * cin, is_pointer);

                    set_number(&tmp, desc->Type, &t, intype);
                    set_number(&vec[l], D3DXPT_INT, &tmp, desc->Type);
                }
                if (is_vertex_shader(table->desc.Version))
                    IDirect3DDevice9_SetVertexShaderConstantI(device, desc->RegisterIndex + i, vec, 1);
                else
                    IDirect3DDevice9_SetPixelShaderConstantI(device, desc->RegisterIndex + i, vec, 1);
            }
            return ret;

        case D3DXRS_FLOAT4:
            for (i = 0; i < regcount; ++i)
            {
                FLOAT vec[4] = {0};

                for (l = 0; l < regsize; ++l)
                {
                    DWORD t = get_index(indata, index + i * rin + l * cin, is_pointer);

                    set_number(&tmp, desc->Type, &t, intype);
                    set_number(&vec[l], D3DXPT_FLOAT, &tmp, desc->Type);
                }
                if (is_vertex_shader(table->desc.Version))
                    IDirect3DDevice9_SetVertexShaderConstantF(device, desc->RegisterIndex + i, vec, 1);
                else
                    IDirect3DDevice9_SetPixelShaderConstantF(device, desc->RegisterIndex + i, vec, 1);
            }
            return ret;

        default:
            FIXME("Unhandled register set %s\n", debug_d3dxparameter_registerset(desc->RegisterSet));
            return 0;
    }
}

static HRESULT set_scalar(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device, D3DXHANDLE constant,
        const void *indata, D3DXPARAMETER_TYPE intype)
{
    struct ctab_constant *c = get_valid_constant(table, constant);
    UINT count = 1;

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    switch (c->desc.Class)
    {
        case D3DXPC_SCALAR:
            set(table, device, c, &indata, intype, &count, c->desc.Columns, D3DXPC_SCALAR, 0, FALSE);
            return D3D_OK;

        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
        case D3DXPC_STRUCT:
            return D3D_OK;

        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(c->desc.Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT set_scalar_array(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device, D3DXHANDLE constant,
        const void *indata, UINT count, D3DXPARAMETER_TYPE intype)
{
    struct ctab_constant *c = get_valid_constant(table, constant);

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    switch (c->desc.Class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
        case D3DXPC_STRUCT:
            set(table, device, c, &indata, intype, &count, c->desc.Columns, D3DXPC_SCALAR, 0, FALSE);
            return D3D_OK;

        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(c->desc.Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT set_vector(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device, D3DXHANDLE constant,
        const void *indata, D3DXPARAMETER_TYPE intype)
{
    struct ctab_constant *c = get_valid_constant(table, constant);
    UINT count = 4;

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    switch (c->desc.Class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_STRUCT:
            set(table, device, c, &indata, intype, &count, 4, D3DXPC_VECTOR, 0, FALSE);
            return D3D_OK;

        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
            return D3D_OK;

        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(c->desc.Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT set_vector_array(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device, D3DXHANDLE constant,
        const void *indata, UINT count, D3DXPARAMETER_TYPE intype)
{
    struct ctab_constant *c = get_valid_constant(table, constant);

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    switch (c->desc.Class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
        case D3DXPC_STRUCT:
            count *= 4;
            set(table, device, c, &indata, intype, &count, 4, D3DXPC_VECTOR, 0, FALSE);
            return D3D_OK;

        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(c->desc.Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT set_matrix_array(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device, D3DXHANDLE constant,
        const void *indata, UINT count, BOOL transpose)
{
    struct ctab_constant *c = get_valid_constant(table, constant);

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    switch (c->desc.Class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
        case D3DXPC_STRUCT:
            count *= 16;
            set(table, device, c, &indata, D3DXPT_FLOAT, &count, 4,
                    transpose ? D3DXPC_MATRIX_ROWS : D3DXPC_MATRIX_COLUMNS, 0, FALSE);
            return D3D_OK;

        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(c->desc.Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT set_matrix_pointer_array(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device,
        D3DXHANDLE constant, const void **indata, UINT count, BOOL transpose)
{
    struct ctab_constant *c = get_valid_constant(table, constant);

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    switch (c->desc.Class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
        case D3DXPC_STRUCT:
            count *= 16;
            set(table, device, c, indata, D3DXPT_FLOAT, &count, 4,
                    transpose ? D3DXPC_MATRIX_ROWS : D3DXPC_MATRIX_COLUMNS, 0, TRUE);
            return D3D_OK;

        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(c->desc.Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetDefaults(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    UINT i;

    TRACE("iface %p, device %p\n", iface, device);

    if (!device)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    for (i = 0; i < This->desc.Constants; i++)
    {
        D3DXCONSTANT_DESC *desc = &This->constants[i].desc;
        HRESULT hr;

        if (!desc->DefaultValue)
            continue;

        switch (desc->RegisterSet)
        {
            case D3DXRS_BOOL:
                if (is_vertex_shader(This->desc.Version))
                    hr = IDirect3DDevice9_SetVertexShaderConstantB(device, desc->RegisterIndex, desc->DefaultValue,
                            desc->RegisterCount);
                else
                    hr = IDirect3DDevice9_SetPixelShaderConstantB(device, desc->RegisterIndex, desc->DefaultValue,
                            desc->RegisterCount);
                break;

            case D3DXRS_INT4:
                if (is_vertex_shader(This->desc.Version))
                    hr = IDirect3DDevice9_SetVertexShaderConstantI(device, desc->RegisterIndex, desc->DefaultValue,
                            desc->RegisterCount);
                else
                    hr = IDirect3DDevice9_SetPixelShaderConstantI(device, desc->RegisterIndex, desc->DefaultValue,
                        desc->RegisterCount);
                break;

            case D3DXRS_FLOAT4:
                if (is_vertex_shader(This->desc.Version))
                    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, desc->RegisterIndex, desc->DefaultValue,
                            desc->RegisterCount);
                else
                    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, desc->RegisterIndex, desc->DefaultValue,
                        desc->RegisterCount);
                break;

            default:
                FIXME("Unhandled register set %s\n", debug_d3dxparameter_registerset(desc->RegisterSet));
                hr = E_NOTIMPL;
                break;
        }

        if (hr != D3D_OK)
            return hr;
    }

    return D3D_OK;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetValue(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const void *data, unsigned int bytes)
{
    struct ID3DXConstantTableImpl *table = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(table, constant);
    D3DXCONSTANT_DESC *desc;

    TRACE("iface %p, device %p, constant %p, data %p, bytes %u\n", iface, device, constant, data, bytes);

    if (!device || !c || !data)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    desc = &c->desc;

    switch (desc->Class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
        case D3DXPC_STRUCT:
            bytes /= 4;
            set(table, device, c, &data, desc->Type, &bytes, desc->Columns, D3DXPC_SCALAR, 0, FALSE);
            return D3D_OK;

        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(desc->Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetBool(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, BOOL b)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, b %d\n", iface, device, constant, b);

    return set_scalar(This, device, constant, &b, D3DXPT_BOOL);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetBoolArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const BOOL *b, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, b %p, count %d\n", iface, device, constant, b, count);

    return set_scalar_array(This, device, constant, b, count, D3DXPT_BOOL);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetInt(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, INT n)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, n %d\n", iface, device, constant, n);

    return set_scalar(This, device, constant, &n, D3DXPT_INT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetIntArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const INT *n, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, n %p, count %d\n", iface, device, constant, n, count);

    return set_scalar_array(This, device, constant, n, count, D3DXPT_INT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetFloat(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, float f)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, f %f\n", iface, device, constant, f);

    return set_scalar(This, device, constant, &f, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetFloatArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const float *f, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, f %p, count %d\n", iface, device, constant, f, count);

    return set_scalar_array(This, device, constant, f, count, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetVector(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXVECTOR4 *vector)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, vector %p\n", iface, device, constant, vector);

    return set_vector(This, device, constant, vector, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetVectorArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, vector %p, count %u\n", iface, device, constant, vector, count);

    return set_vector_array(This, device, constant, vector, count, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrix(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXMATRIX *matrix)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, matrix %p\n", iface, device, constant, matrix);

    return set_matrix_array(This, device, constant, matrix, 1, FALSE);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, matrix %p, count %u\n", iface, device, constant, matrix, count);

    return set_matrix_array(This, device, constant, matrix, count, FALSE);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixPointerArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, matrix %p, count %u)\n", iface, device, constant, matrix, count);

    return set_matrix_pointer_array(This, device, constant, (const void **)matrix, count, FALSE);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTranspose(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXMATRIX *matrix)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, matrix %p\n", iface, device, constant, matrix);

    return set_matrix_array(This, device, constant, matrix, 1, TRUE);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTransposeArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, matrix %p, count %u\n", iface, device, constant, matrix, count);

    return set_matrix_array(This, device, constant, matrix, count, TRUE);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTransposePointerArray(struct ID3DXConstantTable *iface,
        struct IDirect3DDevice9 *device, D3DXHANDLE constant, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("iface %p, device %p, constant %p, matrix %p, count %u)\n", iface, device, constant, matrix, count);

    return set_matrix_pointer_array(This, device, constant, (const void **)matrix, count, TRUE);
}

static const struct ID3DXConstantTableVtbl ID3DXConstantTable_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXConstantTableImpl_QueryInterface,
    ID3DXConstantTableImpl_AddRef,
    ID3DXConstantTableImpl_Release,
    /*** ID3DXBuffer methods ***/
    ID3DXConstantTableImpl_GetBufferPointer,
    ID3DXConstantTableImpl_GetBufferSize,
    /*** ID3DXConstantTable methods ***/
    ID3DXConstantTableImpl_GetDesc,
    ID3DXConstantTableImpl_GetConstantDesc,
    ID3DXConstantTableImpl_GetSamplerIndex,
    ID3DXConstantTableImpl_GetConstant,
    ID3DXConstantTableImpl_GetConstantByName,
    ID3DXConstantTableImpl_GetConstantElement,
    ID3DXConstantTableImpl_SetDefaults,
    ID3DXConstantTableImpl_SetValue,
    ID3DXConstantTableImpl_SetBool,
    ID3DXConstantTableImpl_SetBoolArray,
    ID3DXConstantTableImpl_SetInt,
    ID3DXConstantTableImpl_SetIntArray,
    ID3DXConstantTableImpl_SetFloat,
    ID3DXConstantTableImpl_SetFloatArray,
    ID3DXConstantTableImpl_SetVector,
    ID3DXConstantTableImpl_SetVectorArray,
    ID3DXConstantTableImpl_SetMatrix,
    ID3DXConstantTableImpl_SetMatrixArray,
    ID3DXConstantTableImpl_SetMatrixPointerArray,
    ID3DXConstantTableImpl_SetMatrixTranspose,
    ID3DXConstantTableImpl_SetMatrixTransposeArray,
    ID3DXConstantTableImpl_SetMatrixTransposePointerArray
};

static HRESULT parse_ctab_constant_type(const char *ctab, DWORD typeoffset, struct ctab_constant *constant,
        BOOL is_element, WORD index, WORD max_index, DWORD *offset, DWORD nameoffset, UINT regset)
{
    const D3DXSHADER_TYPEINFO *type = (LPD3DXSHADER_TYPEINFO)(ctab + typeoffset);
    const D3DXSHADER_STRUCTMEMBERINFO *memberinfo = NULL;
    HRESULT hr = D3D_OK;
    UINT i, count = 0;
    WORD size = 0;

    constant->desc.DefaultValue = offset ? ctab + *offset : NULL;
    constant->desc.Class = type->Class;
    constant->desc.Type = type->Type;
    constant->desc.Rows = type->Rows;
    constant->desc.Columns = type->Columns;
    constant->desc.Elements = is_element ? 1 : type->Elements;
    constant->desc.StructMembers = type->StructMembers;
    constant->desc.Name = ctab + nameoffset;
    constant->desc.RegisterSet = regset;
    constant->desc.RegisterIndex = index;

    TRACE("name %s, elements %u, index %u, defaultvalue %p, regset %s\n", constant->desc.Name,
            constant->desc.Elements, index, constant->desc.DefaultValue,
            debug_d3dxparameter_registerset(regset));
    TRACE("class %s, type %s, rows %d, columns %d, elements %d, struct_members %d\n",
            debug_d3dxparameter_class(type->Class), debug_d3dxparameter_type(type->Type),
            type->Rows, type->Columns, type->Elements, type->StructMembers);

    if (type->Elements > 1 && !is_element)
    {
        count = type->Elements;
    }
    else if ((type->Class == D3DXPC_STRUCT) && type->StructMembers)
    {
        memberinfo = (D3DXSHADER_STRUCTMEMBERINFO*)(ctab + type->StructMemberInfo);
        count = type->StructMembers;
    }

    if (count)
    {
        constant->constants = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*constant->constants) * count);
        if (!constant->constants)
        {
             ERR("Out of memory\n");
             hr = E_OUTOFMEMORY;
             goto error;
        }

        for (i = 0; i < count; ++i)
        {
            hr = parse_ctab_constant_type(ctab, memberinfo ? memberinfo[i].TypeInfo : typeoffset,
                    &constant->constants[i], memberinfo == NULL, index + size, max_index, offset,
                    memberinfo ? memberinfo[i].Name : nameoffset, regset);
            if (hr != D3D_OK)
                goto error;

            size += constant->constants[i].desc.RegisterCount;
        }
    }
    else
    {
        WORD offsetdiff = type->Columns * type->Rows;
        BOOL fail = FALSE;

        size = type->Columns * type->Rows;

        switch (regset)
        {
            case D3DXRS_BOOL:
                fail = type->Class != D3DXPC_SCALAR && type->Class != D3DXPC_VECTOR
                        && type->Class != D3DXPC_MATRIX_ROWS && type->Class != D3DXPC_MATRIX_COLUMNS;
                break;

            case D3DXRS_FLOAT4:
            case D3DXRS_INT4:
                switch (type->Class)
                {
                    case D3DXPC_VECTOR:
                        size = 1;
                        /* fall through */
                    case D3DXPC_SCALAR:
                        offsetdiff = type->Rows * 4;
                        break;

                    case D3DXPC_MATRIX_ROWS:
                        offsetdiff = type->Rows * 4;
                        size = type->Rows;
                        break;

                    case D3DXPC_MATRIX_COLUMNS:
                        offsetdiff = type->Columns * 4;
                        size = type->Columns;
                        break;

                    default:
                        fail = TRUE;
                        break;
                }
                break;

            case D3DXRS_SAMPLER:
                size = 1;
                fail = type->Class != D3DXPC_OBJECT;
                break;

            default:
                fail = TRUE;
                break;
        }

        if (fail)
        {
            FIXME("Unhandled register set %s, type class %s\n", debug_d3dxparameter_registerset(regset),
                    debug_d3dxparameter_class(type->Class));
        }

        /* offset in bytes => offsetdiff * sizeof(DWORD) */
        if (offset) *offset += offsetdiff * 4;
    }

    constant->desc.RegisterCount = max(0, min(max_index - index, size));
    constant->desc.Bytes = 4 * constant->desc.Elements * type->Rows * type->Columns;

    return D3D_OK;

error:
    if (constant->constants)
    {
        for (i = 0; i < count; ++i)
        {
            free_constant(&constant->constants[i]);
        }
        HeapFree(GetProcessHeap(), 0, constant->constants);
        constant->constants = NULL;
    }

    return hr;
}

HRESULT WINAPI D3DXGetShaderConstantTableEx(const DWORD *byte_code, DWORD flags, ID3DXConstantTable **constant_table)
{
    struct ID3DXConstantTableImpl *object = NULL;
    const void *data;
    HRESULT hr;
    UINT size;
    const D3DXSHADER_CONSTANTTABLE *ctab_header;
    const D3DXSHADER_CONSTANTINFO *constant_info;
    DWORD i;

    TRACE("byte_code %p, flags %x, constant_table %p\n", byte_code, flags, constant_table);

    if (constant_table) *constant_table = NULL;

    if (!byte_code || !constant_table)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    if (!is_valid_bytecode(*byte_code))
    {
        WARN("Invalid byte_code specified.\n");
        return D3D_OK;
    }

    if (flags) FIXME("Flags (%#x) are not handled, yet!\n", flags);

    hr = D3DXFindShaderComment(byte_code, MAKEFOURCC('C','T','A','B'), &data, &size);
    if (hr != D3D_OK)
    {
        WARN("CTAB not found.\n");
        return D3DXERR_INVALIDDATA;
    }

    if (size < sizeof(*ctab_header))
    {
        WARN("Invalid CTAB size.\n");
        return D3DXERR_INVALIDDATA;
    }

    ctab_header = (const D3DXSHADER_CONSTANTTABLE *)data;
    if (ctab_header->Size != sizeof(*ctab_header))
    {
        WARN("Invalid D3DXSHADER_CONSTANTTABLE size.\n");
        return D3DXERR_INVALIDDATA;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXConstantTable_iface.lpVtbl = &ID3DXConstantTable_Vtbl;
    object->ref = 1;

    object->ctab = HeapAlloc(GetProcessHeap(), 0, size);
    if (!object->ctab)
    {
        ERR("Out of memory\n");
        HeapFree(GetProcessHeap(), 0, object);
        return E_OUTOFMEMORY;
    }
    object->size = size;
    memcpy(object->ctab, data, object->size);

    object->desc.Creator = ctab_header->Creator ? object->ctab + ctab_header->Creator : NULL;
    object->desc.Version = ctab_header->Version;
    object->desc.Constants = ctab_header->Constants;
    TRACE("Creator %s, Version %x, Constants %u, Target %s\n",
            debugstr_a(object->desc.Creator), object->desc.Version, object->desc.Constants,
            debugstr_a(ctab_header->Target ? object->ctab + ctab_header->Target : NULL));

    object->constants = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                  sizeof(*object->constants) * object->desc.Constants);
    if (!object->constants)
    {
         ERR("Out of memory\n");
         hr = E_OUTOFMEMORY;
         goto error;
    }

    constant_info = (const D3DXSHADER_CONSTANTINFO *)(object->ctab + ctab_header->ConstantInfo);
    for (i = 0; i < ctab_header->Constants; i++)
    {
        DWORD offset = constant_info[i].DefaultValue;

        hr = parse_ctab_constant_type(object->ctab, constant_info[i].TypeInfo,
                &object->constants[i], FALSE, constant_info[i].RegisterIndex,
                constant_info[i].RegisterIndex + constant_info[i].RegisterCount,
                offset ? &offset : NULL, constant_info[i].Name, constant_info[i].RegisterSet);
        if (hr != D3D_OK)
            goto error;

        /*
         * Set the register count, it may differ for D3DXRS_INT4, because somehow
         * it makes the assumption that the register size is 1 instead of 4, so the
         * count is 4 times bigger. This holds true only for toplevel shader
         * constants. The count of elements and members is always based on a
         * register size of 4.
         */
        if (object->constants[i].desc.RegisterSet == D3DXRS_INT4)
        {
            object->constants[i].desc.RegisterCount = constant_info[i].RegisterCount;
        }
        object->constants[i].constantinfo_reserved = constant_info[i].Reserved;
    }

    *constant_table = &object->ID3DXConstantTable_iface;

    return D3D_OK;

error:
    free_constant_table(object);
    HeapFree(GetProcessHeap(), 0, object);

    return hr;
}

HRESULT WINAPI D3DXGetShaderConstantTable(const DWORD *byte_code, ID3DXConstantTable **constant_table)
{
    TRACE("(%p, %p): Forwarded to D3DXGetShaderConstantTableEx\n", byte_code, constant_table);

    return D3DXGetShaderConstantTableEx(byte_code, 0, constant_table);
}

HRESULT WINAPI D3DXCreateFragmentLinker(IDirect3DDevice9 *device, UINT size, ID3DXFragmentLinker **linker)
{
    FIXME("device %p, size %u, linker %p: stub.\n", device, size, linker);

    if (linker)
        *linker = NULL;


    return E_NOTIMPL;
}

HRESULT WINAPI D3DXCreateFragmentLinkerEx(IDirect3DDevice9 *device, UINT size, DWORD flags, ID3DXFragmentLinker **linker)
{
    FIXME("device %p, size %u, flags %#x, linker %p: stub.\n", device, size, flags, linker);

    if (linker)
        *linker = NULL;

    return E_NOTIMPL;
}

HRESULT WINAPI D3DXGetShaderSamplers(const DWORD *byte_code, const char **samplers, UINT *count)
{
    UINT i, sampler_count = 0;
    UINT size;
    const char *data;
    const D3DXSHADER_CONSTANTTABLE *ctab_header;
    const D3DXSHADER_CONSTANTINFO *constant_info;

    TRACE("byte_code %p, samplers %p, count %p\n", byte_code, samplers, count);

    if (count) *count = 0;

    if (D3DXFindShaderComment(byte_code, MAKEFOURCC('C','T','A','B'), (const void **)&data, &size) != D3D_OK)
        return D3D_OK;

    if (size < sizeof(*ctab_header)) return D3D_OK;

    ctab_header = (const D3DXSHADER_CONSTANTTABLE *)data;
    if (ctab_header->Size != sizeof(*ctab_header)) return D3D_OK;

    constant_info = (const D3DXSHADER_CONSTANTINFO *)(data + ctab_header->ConstantInfo);
    for (i = 0; i < ctab_header->Constants; i++)
    {
        const D3DXSHADER_TYPEINFO *type;

        TRACE("name = %s\n", data + constant_info[i].Name);

        type = (const D3DXSHADER_TYPEINFO *)(data + constant_info[i].TypeInfo);

        if (type->Type == D3DXPT_SAMPLER
                || type->Type == D3DXPT_SAMPLER1D
                || type->Type == D3DXPT_SAMPLER2D
                || type->Type == D3DXPT_SAMPLER3D
                || type->Type == D3DXPT_SAMPLERCUBE)
        {
            if (samplers) samplers[sampler_count] = data + constant_info[i].Name;

            ++sampler_count;
        }
    }

    TRACE("Found %u samplers\n", sampler_count);

    if (count) *count = sampler_count;

    return D3D_OK;
}


static const char *decl_usage[] = { "position", "blendweight", "blendindices", "normal", "psize", "texcoord",
                                    "tangent", "binormal", "tessfactor", "positiont", "color" };

static const char *tex_type[] = { "", "1d", "2d", "cube", "volume" };

static int add_modifier(char *buffer, DWORD param)
{
    char *buf = buffer;
    DWORD dst_mod = param & D3DSP_DSTMOD_MASK;

    if (dst_mod & D3DSPDM_SATURATE)
        buf += sprintf(buf, "_sat");
    if (dst_mod & D3DSPDM_PARTIALPRECISION)
        buf += sprintf(buf, "_pp");
    if (dst_mod & D3DSPDM_MSAMPCENTROID)
        buf += sprintf(buf, "_centroid");

    return buf - buffer;
}

static int add_register(char *buffer, DWORD param, BOOL dst, BOOL ps)
{
    char *buf = buffer;
    DWORD reg_type = ((param & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2)
                   | ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);
    DWORD reg_num = param & D3DSP_REGNUM_MASK;

    if (reg_type == D3DSPR_INPUT)
        buf += sprintf(buf, "v%d", reg_num);
    else if (reg_type == D3DSPR_CONST)
        buf += sprintf(buf, "c%d", reg_num);
    else if (reg_type == D3DSPR_TEMP)
        buf += sprintf(buf, "r%d", reg_num);
    else if (reg_type == D3DSPR_ADDR)
        buf += sprintf(buf, "%s%d", ps ? "t" : "a", reg_num);
    else if (reg_type == D3DSPR_SAMPLER)
        buf += sprintf(buf, "s%d", reg_num);
    else if (reg_type == D3DSPR_RASTOUT)
        buf += sprintf(buf, "oPos");
    else if (reg_type == D3DSPR_COLOROUT)
        buf += sprintf(buf, "oC%d", reg_num);
    else if (reg_type == D3DSPR_TEXCRDOUT)
        buf += sprintf(buf, "oT%d", reg_num);
    else if (reg_type == D3DSPR_ATTROUT)
        buf += sprintf(buf, "oD%d", reg_num);
    else
        buf += sprintf(buf, "? (%d)", reg_type);

    if (dst)
    {
        if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL)
        {
            buf += sprintf(buf, ".%s%s%s%s", param & D3DSP_WRITEMASK_0 ? "x" : "",
                                             param & D3DSP_WRITEMASK_1 ? "y" : "",
                                             param & D3DSP_WRITEMASK_2 ? "z" : "",
                                             param & D3DSP_WRITEMASK_3 ? "w" : "");
        }
    }
    else
    {
        if ((param & D3DVS_SWIZZLE_MASK) != D3DVS_NOSWIZZLE)
        {
            if ( ((param & D3DSP_SWIZZLE_MASK) == (D3DVS_X_X | D3DVS_Y_X | D3DVS_Z_X | D3DVS_W_X)) ||
                 ((param & D3DSP_SWIZZLE_MASK) == (D3DVS_X_Y | D3DVS_Y_Y | D3DVS_Z_Y | D3DVS_W_Y)) ||
                 ((param & D3DSP_SWIZZLE_MASK) == (D3DVS_X_Z | D3DVS_Y_Z | D3DVS_Z_Z | D3DVS_W_Z)) ||
                 ((param & D3DSP_SWIZZLE_MASK) == (D3DVS_X_W | D3DVS_Y_W | D3DVS_Z_W | D3DVS_W_W)) )
                buf += sprintf(buf, ".%c", 'w' + (((param >> D3DVS_SWIZZLE_SHIFT) + 1) & 0x3));
            else
                buf += sprintf(buf, ".%c%c%c%c", 'w' + (((param >> (D3DVS_SWIZZLE_SHIFT+0)) + 1) & 0x3),
                                                 'w' + (((param >> (D3DVS_SWIZZLE_SHIFT+2)) + 1) & 0x3),
                                                 'w' + (((param >> (D3DVS_SWIZZLE_SHIFT+4)) + 1) & 0x3),
                                                 'w' + (((param >> (D3DVS_SWIZZLE_SHIFT+6)) + 1) & 0x3));
        }
    }

    return buf - buffer;
}

struct instr_info
{
    DWORD opcode;
    const char *name;
    int length;
    int (*function)(const struct instr_info *info, DWORD **ptr, char *buffer, BOOL ps);
    WORD min_version;
    WORD max_version;
};

static int instr_comment(const struct instr_info *info, DWORD **ptr, char *buffer, BOOL ps)
{
    *ptr += 1 + ((**ptr & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT);
    return 0;
}

static int instr_def(const struct instr_info *info, DWORD **ptr, char *buffer, BOOL ps)
{
    int len = sprintf(buffer, "    def c%d, %g, %g, %g, %g\n", *(*ptr+1) & D3DSP_REGNUM_MASK,
                      (double)*(float*)(*ptr+2), (double)*(float*)(*ptr+3),
                      (double)*(float*)(*ptr+4), (double)*(float*)(*ptr+5));
    *ptr += 6;
    return len;
}

static int instr_dcl(const struct instr_info *info, DWORD **ptr, char *buffer, BOOL ps)
{
    DWORD param1 = *++*ptr;
    DWORD param2 = *++*ptr;
    DWORD usage = (param1 & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT;
    DWORD usage_index = (param1 & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;
    char *buf = buffer;

    buf += sprintf(buf, "    dcl");
    if (ps)
    {
        if (param1 & D3DSP_TEXTURETYPE_MASK)
            buf += sprintf(buf, "_%s", (usage <= D3DSTT_VOLUME) ?
                tex_type[(param1 & D3DSP_TEXTURETYPE_MASK) >> D3DSP_TEXTURETYPE_SHIFT] : "???");
    }
    else
    {
        buf += sprintf(buf, "_%s", (usage <= D3DDECLUSAGE_COLOR) ? decl_usage[usage] : "???");
        if (usage_index)
            buf += sprintf(buf, "%d", usage_index);
    }

    buf += add_modifier(buf, param2);
    buf += sprintf(buf, " ");
    buf += add_register(buf, param2, TRUE, TRUE);
    buf += sprintf(buf, "\n");
    (*ptr)++;
    return buf - buffer;
}

static int instr_generic(const struct instr_info *info, DWORD **ptr, char *buffer, BOOL ps)
{
    char *buf = buffer;
    int j;

    buf += sprintf(buf, "    %s", info->name);
    (*ptr)++;

    if (info->length)
    {
        buf += add_modifier(buf, **ptr);

        for (j = 0; j < info->length; j++)
        {
            buf += sprintf(buf, "%s ", j ? "," : "");

            if ((j != 0) && ((**ptr & D3DSP_SRCMOD_MASK) != D3DSPSM_NONE))
            {
                if ((**ptr & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG)
                    buf += sprintf(buf, "-");
                else
                    buf += sprintf(buf, "*");
            }

            buf += add_register(buf, **ptr, j == 0, ps);

            if (*(*ptr)++ & D3DVS_ADDRESSMODE_MASK)
            {
                buf += sprintf(buf, "[");
                buf += add_register(buf, **ptr, FALSE, FALSE);
                buf += sprintf(buf, "]");
                (*ptr)++;
            }
        }
    }
    buf += sprintf(buf, "\n");
    return buf - buffer;
}

const struct instr_info instructions[] =
{
    { D3DSIO_NOP,          "nop",           0, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_MOV,          "mov",           2, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_ADD,          "add",           3, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_SUB,          "sub",           3, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_MAD,          "mad",           4, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_MUL,          "mul",           3, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_RCP,          "rcp",           2, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_RSQ,          "rsq",           2, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_DP3,          "dp3",           3, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_DP4,          "dp4",           3, instr_generic, 0x0100, 0xFFFF }, /* >= 1.2 for PS */
    { D3DSIO_MIN,          "min",           3, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_MAX,          "max",           3, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_SLT,          "slt",           3, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_SGE,          "sge",           3, instr_generic, 0x0100, 0xFFFF }, /* VS only */
    { D3DSIO_EXP,          "exp",           2, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_LOG,          "log",           2, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_LIT,          "lit",           2, instr_generic, 0x0100, 0xFFFF }, /* VS only */
    { D3DSIO_DST,          "dst",           3, instr_generic, 0x0100, 0xFFFF }, /* VS only */
    { D3DSIO_LRP,          "lrp",           4, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for VS */
    { D3DSIO_FRC,          "frc",           2, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_M4x4,         "m4x4",          3, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_M4x3,         "m4x3",          3, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_M3x4,         "m3x4",          3, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_M3x3,         "m3x3",          3, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_M3x2,         "m3x2",          3, instr_generic, 0x0100, 0xFFFF }, /* >= 2.0 for PS */
    { D3DSIO_CALL,         "call",          1, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_CALLNZ,       "callnz",        2, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_LOOP,         "loop",          2, instr_generic, 0x0200, 0xFFFF }, /* >= 3.0 for PS */
    { D3DSIO_RET,          "ret",           0, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_ENDLOOP,      "endloop",       1, instr_generic, 0x0200, 0xFFFF }, /* >= 3.0 for PS */
    { D3DSIO_LABEL,        "label",         1, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_DCL,          "dcl",           1, instr_dcl,     0x0100, 0xFFFF },
    { D3DSIO_POW,          "pow",           3, instr_generic, 0x0200, 0xFFFF },
    { D3DSIO_CRS,          "crs",           3, instr_generic, 0x0200, 0xFFFF },
    { D3DSIO_SGN,          "sgn",           4, instr_generic, 0x0200, 0xFFFF }, /* VS only */
    { D3DSIO_ABS,          "abs",           2, instr_generic, 0x0200, 0xFFFF },
    { D3DSIO_NRM,          "nrm",           2, instr_generic, 0x0200, 0xFFFF },
    { D3DSIO_SINCOS,       "sincos",        4, instr_generic, 0x0200, 0x02FF },
    { D3DSIO_SINCOS,       "sincos",        2, instr_generic, 0x0300, 0xFFFF },
    { D3DSIO_REP,          "rep",           1, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_ENDREP,       "endrep",        0, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_IF,           "if",            1, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_IFC,          "if_comp",       2, instr_generic, 0x0200, 0xFFFF },
    { D3DSIO_ELSE,         "else",          0, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_ENDIF,        "endif",         0, instr_generic, 0x0200, 0xFFFF }, /* >= 2.a for PS */
    { D3DSIO_BREAK,        "break",         0, instr_generic, 0x0201, 0xFFFF },
    { D3DSIO_BREAKC,       "break_comp",    2, instr_generic, 0x0201, 0xFFFF },
    { D3DSIO_MOVA,         "mova",          2, instr_generic, 0x0200, 0xFFFF }, /* VS only */
    { D3DSIO_DEFB,         "defb",          2, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_DEFI,         "defi",          2, instr_generic, 0x0100, 0xFFFF },
    { D3DSIO_TEXCOORD,     "texcoord",      1, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXCOORD,     "texcrd",        2, instr_generic, 0x0104, 0x0104 }, /* PS only */
    { D3DSIO_TEXKILL,      "texkill",       1, instr_generic, 0x0100, 0xFFFF }, /* PS only */
    { D3DSIO_TEX,          "tex",           1, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEX,          "texld",         2, instr_generic, 0x0104, 0x0104 }, /* PS only */
    { D3DSIO_TEX,          "texld",         3, instr_generic, 0x0200, 0xFFFF }, /* PS only */
    { D3DSIO_TEXBEM,       "texbem",        2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXBEML,      "texbeml",       2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXREG2AR,    "texreg2ar",     2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXREG2GB,    "texreg2gb",     2, instr_generic, 0x0102, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x2PAD,   "texm3x2pad",    2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x2TEX,   "texm3x2tex",    2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x3PAD,   "texm3x3pad",    2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x3TEX,   "texm3x3tex",    2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x3DIFF,  "texm3x3diff",   2, instr_generic, 0x0100, 0xFFFF }, /* PS only - Not documented */
    { D3DSIO_TEXM3x3SPEC,  "texm3x3spec",   3, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x3VSPEC, "texm3x3vspec",  2, instr_generic, 0x0100, 0x0103 }, /* PS only */
    { D3DSIO_EXPP,         "expp",          2, instr_generic, 0x0100, 0xFFFF }, /* VS only */
    { D3DSIO_LOGP,         "logp",          2, instr_generic, 0x0100, 0xFFFF }, /* VS only */
    { D3DSIO_CND,          "cnd",           4, instr_generic, 0x0100, 0x0104 }, /* PS only */
    { D3DSIO_DEF,          "def",           5, instr_def,     0x0100, 0xFFFF },
    { D3DSIO_TEXREG2RGB,   "texreg2rgb",    2, instr_generic, 0x0102, 0x0103 }, /* PS only */
    { D3DSIO_TEXDP3TEX,    "texdp3tex",     2, instr_generic, 0x0102, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x2DEPTH, "texm3x2depth",  2, instr_generic, 0x0103, 0x0103 }, /* PS only */
    { D3DSIO_TEXDP3,       "texdp3",        2, instr_generic, 0x0102, 0x0103 }, /* PS only */
    { D3DSIO_TEXM3x3,      "texm3x3",       2, instr_generic, 0x0102, 0x0103 }, /* PS only */
    { D3DSIO_TEXDEPTH,     "texdepth",      1, instr_generic, 0x0104, 0x0104 }, /* PS only */
    { D3DSIO_CMP,          "cmp",           4, instr_generic, 0x0102, 0xFFFF }, /* PS only */
    { D3DSIO_BEM,          "bem",           3, instr_generic, 0x0104, 0x0104 }, /* PS only */
    { D3DSIO_DP2ADD,       "dp2add",        4, instr_generic, 0x0200, 0xFFFF }, /* PS only */
    { D3DSIO_DSX,          "dsx",           2, instr_generic, 0x0201, 0xFFFF }, /* PS only */
    { D3DSIO_DSY,          "dsy",           2, instr_generic, 0x0201, 0xFFFF }, /* PS only */
    { D3DSIO_TEXLDD,       "texldd",        5, instr_generic, 0x0201, 0xFFFF }, /* PS only - not existing for 2.b */
    { D3DSIO_SETP,         "setp_comp",     3, instr_generic, 0x0201, 0xFFFF },
    { D3DSIO_TEXLDL,       "texldl",        3, instr_generic, 0x0300, 0xFFFF },
    { D3DSIO_BREAKP,       "breakp",        1, instr_generic, 0x0201, 0xFFFF },
    { D3DSIO_PHASE,        "phase",         0, instr_generic, 0x0104, 0x0104 },  /* PS only */
    { D3DSIO_COMMENT,      "",              0, instr_comment, 0x0100, 0xFFFF }
};

HRESULT WINAPI D3DXDisassembleShader(const DWORD *shader, BOOL colorcode, const char *comments, ID3DXBuffer **disassembly)
{
    DWORD *ptr = (DWORD *)shader;
    char *buffer, *buf;
    UINT capacity = 4096;
    BOOL ps;
    WORD version;
    HRESULT hr;

    TRACE("%p %d %s %p\n", shader, colorcode, debugstr_a(comments), disassembly);

    if (!shader || !disassembly)
        return D3DERR_INVALIDCALL;

    buf = buffer = HeapAlloc(GetProcessHeap(), 0, capacity);
    if (!buffer)
        return E_OUTOFMEMORY;

    ps = (*ptr >> 16) & 1;
    version = *ptr & 0xFFFF;
    buf += sprintf(buf, "    %s_%d_%d\n", ps ? "ps" : "vs", D3DSHADER_VERSION_MAJOR(*ptr), D3DSHADER_VERSION_MINOR(*ptr));
    ptr++;

    while (*ptr != D3DSIO_END)
    {
        DWORD index;

        if ((buf - buffer + 128) > capacity)
        {
            UINT count = buf - buffer;
            char *new_buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, capacity * 2);
            if (!new_buffer)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                return E_OUTOFMEMORY;
            }
            capacity *= 2;
            buffer = new_buffer;
            buf = buffer + count;
        }

        for (index = 0; index < sizeof(instructions)/sizeof(instructions[0]); index++)
            if (((*ptr & D3DSI_OPCODE_MASK) == instructions[index].opcode) &&
                (version >= instructions[index].min_version) && (version <= instructions[index].max_version))
                break;

        if (index != sizeof(instructions)/sizeof(instructions[0]))
        {
            buf += instructions[index].function(&(instructions[index]), &ptr, buf, ps);
        }
        else
        {
            buf += sprintf(buf, "    ??? (Unknown opcode %x)\n", *ptr);
            while (*++ptr & (1u << 31));
        }
    }

    hr = D3DXCreateBuffer(buf - buffer + 1 , disassembly);
    if (SUCCEEDED(hr))
        strcpy(ID3DXBuffer_GetBufferPointer(*disassembly), buffer);
    HeapFree(GetProcessHeap(), 0, buffer);

    return hr;
}

struct d3dx9_texture_shader
{
    ID3DXTextureShader ID3DXTextureShader_iface;
    LONG ref;
};

static inline struct d3dx9_texture_shader *impl_from_ID3DXTextureShader(ID3DXTextureShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_texture_shader, ID3DXTextureShader_iface);
}

static HRESULT WINAPI d3dx9_texture_shader_QueryInterface(ID3DXTextureShader *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXTextureShader))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return D3D_OK;
    }

    WARN("Interface %s not found.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_texture_shader_AddRef(ID3DXTextureShader *iface)
{
    struct d3dx9_texture_shader *texture_shader = impl_from_ID3DXTextureShader(iface);
    ULONG refcount = InterlockedIncrement(&texture_shader->ref);

    TRACE("%p increasing refcount to %u.\n", texture_shader, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_texture_shader_Release(ID3DXTextureShader *iface)
{
    struct d3dx9_texture_shader *texture_shader = impl_from_ID3DXTextureShader(iface);
    ULONG refcount = InterlockedDecrement(&texture_shader->ref);

    TRACE("%p decreasing refcount to %u.\n", texture_shader, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, texture_shader);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_texture_shader_GetFunction(ID3DXTextureShader *iface, struct ID3DXBuffer **function)
{
    FIXME("iface %p, function %p stub.\n", iface, function);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_GetConstantBuffer(ID3DXTextureShader *iface, struct ID3DXBuffer **constant_buffer)
{
    FIXME("iface %p, constant_buffer %p stub.\n", iface, constant_buffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_GetDesc(ID3DXTextureShader *iface, D3DXCONSTANTTABLE_DESC *desc)
{
    FIXME("iface %p, desc %p stub.\n", iface, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_GetConstantDesc(ID3DXTextureShader *iface, D3DXHANDLE constant, D3DXCONSTANT_DESC *constant_desc, UINT *count)
{
    FIXME("iface %p, constant %p, constant_desc %p, count %p stub.\n", iface, constant, constant_desc, count);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI d3dx9_texture_shader_GetConstant(ID3DXTextureShader *iface, D3DXHANDLE constant, UINT index)
{
    FIXME("iface %p, constant %p, index %u stub.\n", iface, constant, index);

    return NULL;
}

static D3DXHANDLE WINAPI d3dx9_texture_shader_GetConstantByName(ID3DXTextureShader *iface, D3DXHANDLE constant, const char *name)
{
    FIXME("iface %p, constant %p, name %s stub.\n", iface, constant, debugstr_a(name));

    return NULL;
}

static D3DXHANDLE WINAPI d3dx9_texture_shader_GetConstantElement(ID3DXTextureShader *iface, D3DXHANDLE constant, UINT index)
{
    FIXME("iface %p, constant %p, index %u stub.\n", iface, constant, index);

    return NULL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetDefaults(ID3DXTextureShader *iface)
{
    FIXME("iface %p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetValue(ID3DXTextureShader *iface, D3DXHANDLE constant, const void *data, UINT bytes)
{
    FIXME("iface %p, constant %p, data %p, bytes %u stub.\n", iface, constant, data, bytes);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetBool(ID3DXTextureShader *iface, D3DXHANDLE constant, BOOL b)
{
    FIXME("iface %p, constant %p, b %u stub.\n", iface, constant, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetBoolArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const BOOL *b, UINT count)
{
    FIXME("iface %p, constant %p, b %p, count %u stub.\n", iface, constant, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetInt(ID3DXTextureShader *iface, D3DXHANDLE constant, INT n)
{
    FIXME("iface %p, constant %p, n %d stub.\n", iface, constant, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetIntArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const INT *n, UINT count)
{
    FIXME("iface %p, constant %p, n %p, count %u stub.\n", iface, constant, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetFloat(ID3DXTextureShader *iface, D3DXHANDLE constant, FLOAT f)
{
    FIXME("iface %p, constant %p, f %f stub.\n", iface, constant, f);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetFloatArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const FLOAT *f, UINT count)
{
    FIXME("iface %p, constant %p, f %p, count %u stub.\n", iface, constant, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetVector(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXVECTOR4 *vector)
{
    FIXME("iface %p, constant %p, vector %p stub.\n", iface, constant, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetVectorArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXVECTOR4 *vector, UINT count)
{
    FIXME("iface %p, constant %p, vector %p, count %u stub.\n", iface, constant, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetMatrix(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXMATRIX *matrix)
{
    FIXME("iface %p, constant %p, matrix %p stub.\n", iface, constant, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetMatrixArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXMATRIX *matrix, UINT count)
{
    FIXME("iface %p, constant %p, matrix %p, count %u stub.\n", iface, constant, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetMatrixPointerArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXMATRIX **matrix, UINT count)
{
    FIXME("iface %p, constant %p, matrix %p, count %u stub.\n", iface, constant, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetMatrixTranspose(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXMATRIX *matrix)
{
    FIXME("iface %p, constant %p, matrix %p stub.\n", iface, constant, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetMatrixTransposeArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXMATRIX *matrix, UINT count)
{
    FIXME("iface %p, constant %p, matrix %p, count %u stub.\n", iface, constant, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_texture_shader_SetMatrixTransposePointerArray(ID3DXTextureShader *iface, D3DXHANDLE constant, const D3DXMATRIX **matrix, UINT count)
{
    FIXME("iface %p, constant %p, matrix %p, count %u stub.\n", iface, constant, matrix, count);

    return E_NOTIMPL;
}

static const struct ID3DXTextureShaderVtbl d3dx9_texture_shader_vtbl =
{
    /*** IUnknown methods ***/
    d3dx9_texture_shader_QueryInterface,
    d3dx9_texture_shader_AddRef,
    d3dx9_texture_shader_Release,
    /*** ID3DXTextureShader methods ***/
    d3dx9_texture_shader_GetFunction,
    d3dx9_texture_shader_GetConstantBuffer,
    d3dx9_texture_shader_GetDesc,
    d3dx9_texture_shader_GetConstantDesc,
    d3dx9_texture_shader_GetConstant,
    d3dx9_texture_shader_GetConstantByName,
    d3dx9_texture_shader_GetConstantElement,
    d3dx9_texture_shader_SetDefaults,
    d3dx9_texture_shader_SetValue,
    d3dx9_texture_shader_SetBool,
    d3dx9_texture_shader_SetBoolArray,
    d3dx9_texture_shader_SetInt,
    d3dx9_texture_shader_SetIntArray,
    d3dx9_texture_shader_SetFloat,
    d3dx9_texture_shader_SetFloatArray,
    d3dx9_texture_shader_SetVector,
    d3dx9_texture_shader_SetVectorArray,
    d3dx9_texture_shader_SetMatrix,
    d3dx9_texture_shader_SetMatrixArray,
    d3dx9_texture_shader_SetMatrixPointerArray,
    d3dx9_texture_shader_SetMatrixTranspose,
    d3dx9_texture_shader_SetMatrixTransposeArray,
    d3dx9_texture_shader_SetMatrixTransposePointerArray
};

HRESULT WINAPI D3DXCreateTextureShader(const DWORD *function, ID3DXTextureShader **texture_shader)
{
    struct d3dx9_texture_shader *object;

    TRACE("function %p, texture_shader %p.\n", function, texture_shader);

    if (!function || !texture_shader)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXTextureShader_iface.lpVtbl = &d3dx9_texture_shader_vtbl;
    object->ref = 1;

    *texture_shader = &object->ID3DXTextureShader_iface;

    return D3D_OK;
}

static unsigned int get_instr_length(const DWORD *byte_code, unsigned int major, unsigned int minor)
{
    DWORD opcode = *byte_code & 0xffff;
    unsigned int len = 0;

    if (opcode == D3DSIO_COMMENT)
        return (*byte_code & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;

    if (major > 1)
        return (*byte_code & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;

    switch (opcode)
    {
        case D3DSIO_END:
            ERR("Unexpected END token.\n");
            return 0;
        case D3DSIO_DEF:
        case D3DSIO_DEFI:
            return 5;
        case D3DSIO_DEFB:
            return 2;
        default:
            ++byte_code;
            while (*byte_code & 0x80000000)
            {
                ++byte_code;
                ++len;
            }
    }

    return len;
}

static HRESULT get_shader_semantics(const DWORD *byte_code, D3DXSEMANTIC *semantics, UINT *count, BOOL output)
{
    static const D3DDECLUSAGE regtype_usage[] =
    {
        D3DDECLUSAGE_COLOR,
        D3DDECLUSAGE_COLOR,
        0,
        D3DDECLUSAGE_TEXCOORD,
        0,
        D3DDECLUSAGE_COLOR,
        D3DDECLUSAGE_TEXCOORD,
        0,
        0,
        D3DDECLUSAGE_DEPTH
    };
    static const D3DDECLUSAGE rast_usage[] =
    {
        D3DDECLUSAGE_POSITION,
        D3DDECLUSAGE_FOG,
        D3DDECLUSAGE_PSIZE
    };
    DWORD reg_type, usage, index, version_token = *byte_code;
    BOOL is_ps = version_token >> 16 == 0xffff;
    unsigned int major, minor, i = 0, j;
    BYTE colors = 0, rastout = 0;
    BOOL has_dcl, depth = 0;
    WORD texcoords = 0;

    if ((version_token & 0xffff0000) != 0xfffe0000 && (version_token & 0xffff0000) != 0xffff0000)
        return D3DXERR_INVALIDDATA;

    major = version_token >> 8 & 0xff;
    minor = version_token & 0xff;

    TRACE("%s shader, version %u.%u.\n", is_ps ? "Pixel" : "Vertex", major, minor);
    ++byte_code;

    has_dcl = (!is_ps && (!output || major == 3)) || (is_ps && !output && major >= 2);

    while (*byte_code != D3DSIO_END)
    {
        if (has_dcl && (*byte_code & 0xffff) == D3DSIO_DCL)
        {
            DWORD usage_token = byte_code[1];
            DWORD reg = byte_code[2];

            reg_type = ((reg & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT)
                    | ((reg & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2);

            if (is_ps && !output && major == 2)
            {
                /* dcl with no explicit usage, look at the register. */
                reg_type = ((reg & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT)
                        | ((reg & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2);
                index = reg & D3DSP_REGNUM_MASK;
                if (reg_type >= ARRAY_SIZE(regtype_usage))
                {
                    WARN("Invalid register type %u.\n", reg_type);
                    reg_type = 0;
                }
                usage = regtype_usage[reg_type];
                if (semantics)
                {
                    semantics[i].Usage = usage;
                    semantics[i].UsageIndex = index;
                }
                ++i;
            }
            else if ((!output && reg_type == D3DSPR_INPUT) || (output && reg_type == D3DSPR_OUTPUT))
            {
                if (semantics)
                {
                    semantics[i].Usage =
                            (usage_token & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT;
                    semantics[i].UsageIndex =
                            (usage_token & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;
                }
                ++i;
            }
            byte_code += 3;
        }
        else if (!has_dcl)
        {
            unsigned int len = get_instr_length(byte_code, major, minor) + 1;

            switch (*byte_code & 0xffff)
            {
                case D3DSIO_COMMENT:
                case D3DSIO_DEF:
                case D3DSIO_DEFB:
                case D3DSIO_DEFI:
                    byte_code += len;
                    break;
                default:
                    ++byte_code;
                    while (*byte_code & 0x80000000)
                    {
                        reg_type = ((*byte_code & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT)
                                | ((*byte_code & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2);
                        index = *byte_code & D3DSP_REGNUM_MASK;

                        if ((reg_type == D3DSPR_TEMP && is_ps && major == 1)
                                || (reg_type == D3DSPR_INPUT && is_ps)
                                || (reg_type == D3DSPR_TEXTURE && is_ps && !output)
                                || reg_type == D3DSPR_RASTOUT
                                || reg_type == D3DSPR_ATTROUT
                                || reg_type == D3DSPR_OUTPUT
                                || reg_type == D3DSPR_DEPTHOUT)
                        {
                            if (reg_type == D3DSPR_RASTOUT)
                                rastout |= 1u << index;
                            else if (reg_type == D3DSPR_DEPTHOUT)
                                depth = TRUE;
                            else if (reg_type == D3DSPR_TEXTURE || reg_type == D3DSPR_OUTPUT)
                                texcoords |= 1u << index;
                            else
                                colors |= 1u << index;
                        }
                        ++byte_code;
                    }
            }
        }
        else
        {
            byte_code += get_instr_length(byte_code, major, minor) + 1;
        }
    }

    if (!has_dcl)
    {
        i = j = 0;
        while (texcoords)
        {
            if (texcoords & 1)
            {
                if (semantics)
                {
                    semantics[i].Usage = D3DDECLUSAGE_TEXCOORD;
                    semantics[i].UsageIndex = j;
                }
                ++i;
            }
            texcoords >>= 1;
            ++j;
        }
        j = 0;
        while (colors)
        {
            if (colors & 1)
            {
                if (semantics)
                {
                    semantics[i].Usage = D3DDECLUSAGE_COLOR;
                    semantics[i].UsageIndex = j;
                }
                ++i;
            }
            colors >>= 1;
            ++j;
        }
        j = 0;
        while (rastout)
        {
            if (rastout & 1)
            {
                if (j >= ARRAY_SIZE(rast_usage))
                {
                    WARN("Invalid RASTOUT register index.\n");
                    usage = 0;
                }
                else
                {
                    usage = rast_usage[j];
                }
                if (semantics)
                {
                    semantics[i].Usage = usage;
                    semantics[i].UsageIndex = 0;
                }
                ++i;
            }
            rastout >>= 1;
            ++j;
        }
        if (depth)
        {
            if (semantics)
            {
                semantics[i].Usage = D3DDECLUSAGE_DEPTH;
                semantics[i].UsageIndex = 0;
            }
            ++i;
        }
    }

    if (count)
        *count = i;

    return D3D_OK;
}

HRESULT WINAPI D3DXGetShaderInputSemantics(const DWORD *byte_code, D3DXSEMANTIC *semantics, UINT *count)
{
    TRACE("byte_code %p, semantics %p, count %p.\n", byte_code, semantics, count);

    return get_shader_semantics(byte_code, semantics, count, FALSE);
}

HRESULT WINAPI D3DXGetShaderOutputSemantics(const DWORD *byte_code, D3DXSEMANTIC *semantics, UINT *count)
{
    TRACE("byte_code %p, semantics %p, count %p.\n", byte_code, semantics, count);

    return get_shader_semantics(byte_code, semantics, count, TRUE);
}
