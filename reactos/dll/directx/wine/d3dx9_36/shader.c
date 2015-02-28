/*
 * Copyright 2008 Luis Busquets
 * Copyright 2009 Matteo Bruni
 * Copyright 2010, 2013 Christian Costa
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

#include "d3dx9_36_private.h"
#include "d3dcompiler.h"


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

static CRITICAL_SECTION from_file_mutex;
static CRITICAL_SECTION_DEBUG from_file_mutex_debug =
{
    0, 0, &from_file_mutex,
    {
        &from_file_mutex_debug.ProcessLocksList,
        &from_file_mutex_debug.ProcessLocksList
    },
    0, 0, {(DWORD_PTR)(__FILE__ ": from_file_mutex")}
};
static CRITICAL_SECTION from_file_mutex = {&from_file_mutex_debug, -1, 0, 0, 0, 0};

/* D3DXInclude private implementation, used to implement
 * D3DXAssembleShaderFromFile() from D3DXAssembleShader(). */
/* To be able to correctly resolve include search paths we have to store the
 * pathname of each include file. We store the pathname pointer right before
 * the file data. */
static HRESULT WINAPI d3dincludefromfile_open(ID3DXInclude *iface, D3DXINCLUDE_TYPE include_type,
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

    TRACE("Looking up for include file %s, parent %s\n", debugstr_a(filename), debugstr_a(parent_name));

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

static HRESULT WINAPI d3dincludefromfile_close(ID3DXInclude *iface, const void *data)
{
    HeapFree(GetProcessHeap(), 0, *((char **)data - 1));
    HeapFree(GetProcessHeap(), 0, (char **)data - 1);
    if (main_file_data == data)
        main_file_data = NULL;
    return S_OK;
}

static const struct ID3DXIncludeVtbl D3DXInclude_Vtbl = {
    d3dincludefromfile_open,
    d3dincludefromfile_close
};

struct D3DXIncludeImpl {
    ID3DXInclude ID3DXInclude_iface;
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
    struct D3DXIncludeImpl includefromfile;
    char *filename_a;

    TRACE("filename %s, defines %p, include %p, flags %#x, shader %p, error_messages %p.\n",
            debugstr_w(filename), defines, include, flags, shader, error_messages);

    if(!include)
    {
        includefromfile.ID3DXInclude_iface.lpVtbl = &D3DXInclude_Vtbl;
        include = &includefromfile.ID3DXInclude_iface;
    }

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    filename_a = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
    if (!filename_a)
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);

    EnterCriticalSection(&from_file_mutex);
    hr = ID3DXInclude_Open(include, D3D_INCLUDE_LOCAL, filename_a, NULL, &buffer, &len);
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
    struct D3DXIncludeImpl includefromfile;
    char *filename_a;

    TRACE("filename %s, defines %p, include %p, entrypoint %s, profile %s, "
            "flags %#x, shader %p, error_messages %p, constant_table %p.\n",
            debugstr_w(filename), defines, include, debugstr_a(entrypoint), debugstr_a(profile),
            flags, shader, error_messages, constant_table);

    if (!include)
    {
        includefromfile.ID3DXInclude_iface.lpVtbl = &D3DXInclude_Vtbl;
        include = &includefromfile.ID3DXInclude_iface;
    }

    filename_len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    filename_a = HeapAlloc(GetProcessHeap(), 0, filename_len * sizeof(char));
    if (!filename_a)
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, filename_len, NULL, NULL);

    EnterCriticalSection(&from_file_mutex);
    hr = ID3DXInclude_Open(include, D3D_INCLUDE_LOCAL, filename_a, NULL, &buffer, &len);
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
    struct D3DXIncludeImpl includefromfile;
    char *filename_a;

    TRACE("filename %s, defines %p, include %p, shader %p, error_messages %p.\n",
            debugstr_w(filename), defines, include, shader, error_messages);

    if (!include)
    {
        includefromfile.ID3DXInclude_iface.lpVtbl = &D3DXInclude_Vtbl;
        include = &includefromfile.ID3DXInclude_iface;
    }

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    filename_a = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
    if (!filename_a)
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);

    EnterCriticalSection(&from_file_mutex);
    hr = ID3DXInclude_Open(include, D3D_INCLUDE_LOCAL, filename_a, NULL, &buffer, &len);
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

struct ctab_constant {
    D3DXCONSTANT_DESC desc;
    struct ctab_constant *constants;
};

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

static HRESULT WINAPI ID3DXConstantTableImpl_GetConstantDesc(ID3DXConstantTable *iface, D3DXHANDLE constant,
                                                             D3DXCONSTANT_DESC *desc, UINT *count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("(%p)->(%p, %p, %p)\n", This, constant, desc, count);

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

            if ((is_pointer || (!is_pointer && inclass == D3DXPC_MATRIX_ROWS)) && desc->RegisterSet != D3DXRS_BOOL)
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
            else if ((is_pointer || (!is_pointer && inclass == D3DXPC_MATRIX_ROWS)) && desc->RegisterSet != D3DXRS_BOOL)
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

HRESULT WINAPI D3DXDisassembleShader(const DWORD *shader, BOOL colorcode, const char *comments, ID3DXBuffer **disassembly)
{
   FIXME("%p %d %s %p: stub\n", shader, colorcode, debugstr_a(comments), disassembly);
   return E_OUTOFMEMORY;
}

const DWORD* skip_instruction(const DWORD *byte_code, UINT shader_model)
{
    TRACE("Shader model %u\n", shader_model);

    /* Handle all special instructions whose arguments may contain D3DSIO_DCL */
    if ((*byte_code & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
    {
        byte_code += 1 + ((*byte_code & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT);
    }
    else if (shader_model >= 2)
    {
        byte_code += 1 + ((*byte_code & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT);
    }
    else if ((*byte_code & D3DSI_OPCODE_MASK) == D3DSIO_DEF)
    {
        byte_code += 1 + 5;
    }
    else
    {
        /* Handle remaining safe instructions */
        while (*++byte_code & (1 << 31));
    }

    return byte_code;
}

static UINT get_shader_semantics(const DWORD *byte_code, D3DXSEMANTIC *semantics, BOOL input)
{
    const DWORD *ptr = byte_code;
    UINT shader_model = (*ptr >> 8) & 0xff;
    UINT i = 0;

    TRACE("Shader version: %#x\n", *ptr);
    ptr++;

    while (*ptr != D3DSIO_END)
    {
        if (*ptr & (1 << 31))
        {
            FIXME("Opcode expected but got %#x\n", *ptr);
            return 0;
        }
        else if ((*ptr & D3DSI_OPCODE_MASK) == D3DSIO_DCL)
        {
            DWORD param1 = *++ptr;
            DWORD param2 = *++ptr;
            DWORD usage = param1 & 0x1f;
            DWORD usage_index = (param1 >> 16) & 0xf;
            DWORD reg_type = (((param2 >> 11) & 0x3) << 3) | ((param2 >> 28) & 0x7);

            TRACE("D3DSIO_DCL param1: %#x, param2: %#x, usage: %u, usage_index: %u, reg_type: %u\n",
                   param1, param2, usage, usage_index, reg_type);

            if ((input && (reg_type == D3DSPR_INPUT)) || (!input && (reg_type == D3DSPR_OUTPUT)))
            {
                if (semantics)
                {
                    semantics[i].Usage = usage;
                    semantics[i].UsageIndex = usage_index;
                }
                i++;
            }

            ptr++;
        }
        else
        {
            ptr = skip_instruction(ptr, shader_model);
        }
    }

    return i;
}

HRESULT WINAPI D3DXGetShaderInputSemantics(const DWORD *byte_code, D3DXSEMANTIC *semantics, UINT *count)
{
    UINT nb_semantics;

    TRACE("byte_code %p, semantics %p, count %p\n", byte_code, semantics, count);

    if (!byte_code)
        return D3DERR_INVALIDCALL;

    nb_semantics = get_shader_semantics(byte_code, semantics, TRUE);

    if (count)
        *count = nb_semantics;

    return D3D_OK;
}


HRESULT WINAPI D3DXGetShaderOutputSemantics(const DWORD *byte_code, D3DXSEMANTIC *semantics, UINT *count)
{
    UINT nb_semantics;

    TRACE("byte_code %p, semantics %p, count %p\n", byte_code, semantics, count);

    if (!byte_code)
        return D3DERR_INVALIDCALL;

    nb_semantics = get_shader_semantics(byte_code, semantics, FALSE);

    if (count)
        *count = nb_semantics;

    return D3D_OK;
}
