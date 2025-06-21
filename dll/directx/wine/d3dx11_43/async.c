/*
 * Copyright 2016 Matteo Bruni for CodeWeavers
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

#include "d3dx11.h"
#include "d3dcompiler.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct asyncdataloader
{
    ID3DX11DataLoader ID3DX11DataLoader_iface;

    union
    {
        struct
        {
            WCHAR *path;
        } file;
        struct
        {
            HMODULE module;
            HRSRC rsrc;
        } resource;
    } u;
    void *data;
    SIZE_T size;
};

static inline struct asyncdataloader *impl_from_ID3DX11DataLoader(ID3DX11DataLoader *iface)
{
    return CONTAINING_RECORD(iface, struct asyncdataloader, ID3DX11DataLoader_iface);
}

static HRESULT WINAPI memorydataloader_Load(ID3DX11DataLoader *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static HRESULT WINAPI memorydataloader_Decompress(ID3DX11DataLoader *iface, void **data, SIZE_T *size)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);

    TRACE("iface %p, data %p, size %p.\n", iface, data, size);

    *data = loader->data;
    *size = loader->size;

    return S_OK;
}

static HRESULT WINAPI memorydataloader_Destroy(ID3DX11DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);

    TRACE("iface %p.\n", iface);

    free(loader);
    return S_OK;
}

static const ID3DX11DataLoaderVtbl memorydataloadervtbl =
{
    memorydataloader_Load,
    memorydataloader_Decompress,
    memorydataloader_Destroy
};

static HRESULT WINAPI filedataloader_Load(ID3DX11DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);
    DWORD size, read_len;
    HANDLE file;
    void *data;
    BOOL ret;

    TRACE("iface %p.\n", iface);

    /* Always buffer file contents, even if Load() was already called. */
    file = CreateFileW(loader->u.file.path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return D3D11_ERROR_FILE_NOT_FOUND;

    size = GetFileSize(file, NULL);
    data = malloc(size);
    if (!data)
    {
        CloseHandle(file);
        return E_OUTOFMEMORY;
    }

    ret = ReadFile(file, data, size, &read_len, NULL);
    CloseHandle(file);
    if (!ret)
    {
        ERR("Failed to read file contents.\n");
        free(data);
        return E_FAIL;
    }

    free(loader->data);
    loader->data = data;
    loader->size = size;

    return S_OK;
}

static HRESULT WINAPI filedataloader_Decompress(ID3DX11DataLoader *iface, void **data, SIZE_T *size)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);

    TRACE("iface %p, data %p, size %p.\n", iface, data, size);

    if (!loader->data)
        return E_FAIL;

    *data = loader->data;
    *size = loader->size;

    return S_OK;
}

static HRESULT WINAPI filedataloader_Destroy(ID3DX11DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);

    TRACE("iface %p.\n", iface);

    free(loader->u.file.path);
    free(loader->data);
    free(loader);

    return S_OK;
}

static const ID3DX11DataLoaderVtbl filedataloadervtbl =
{
    filedataloader_Load,
    filedataloader_Decompress,
    filedataloader_Destroy
};

static HRESULT WINAPI resourcedataloader_Load(ID3DX11DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);
    HGLOBAL hglobal;

    TRACE("iface %p.\n", iface);

    if (loader->data)
        return S_OK;

    hglobal = LoadResource(loader->u.resource.module, loader->u.resource.rsrc);
    if (!hglobal)
    {
        ERR("Failed to load resource.\n");
        return E_FAIL;
    }

    loader->data = LockResource(hglobal);
    loader->size = SizeofResource(loader->u.resource.module, loader->u.resource.rsrc);

    return S_OK;
}

static HRESULT WINAPI resourcedataloader_Decompress(ID3DX11DataLoader *iface, void **data, SIZE_T *size)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);

    TRACE("iface %p, data %p, size %p.\n", iface, data, size);

    if (!loader->data)
        return E_FAIL;

    *data = loader->data;
    *size = loader->size;

    return S_OK;
}

static HRESULT WINAPI resourcedataloader_Destroy(ID3DX11DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX11DataLoader(iface);

    TRACE("iface %p.\n", iface);

    free(loader);

    return S_OK;
}

static const ID3DX11DataLoaderVtbl resourcedataloadervtbl =
{
    resourcedataloader_Load,
    resourcedataloader_Decompress,
    resourcedataloader_Destroy
};

HRESULT WINAPI D3DX11CompileFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *entry_point,
        const char *target, UINT sflags, UINT eflags, ID3DX11ThreadPump *pump, ID3D10Blob **shader,
        ID3D10Blob **error_messages, HRESULT *hresult)
{
    TRACE("data %s, data_size %Iu, filename %s, defines %p, include %p, entry_point %s, target %s, "
            "sflags %#x, eflags %#x, pump %p, shader %p, error_messages %p, hresult %p.\n",
            debugstr_an(data, data_size), data_size, debugstr_a(filename), defines, include,
            debugstr_a(entry_point), debugstr_a(target), sflags, eflags, pump, shader,
            error_messages, hresult);

    if (pump)
        FIXME("Unimplemented ID3DX11ThreadPump handling.\n");

    return D3DCompile(data, data_size, filename, defines, include, entry_point, target,
            sflags, eflags, shader, error_messages);
}

HRESULT WINAPI D3DX11CompileFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *entry_point, const char *target, UINT sflags, UINT eflags,
        ID3DX11ThreadPump *pump, ID3D10Blob **shader, ID3D10Blob **error_messages, HRESULT *hresult)
{
    WCHAR filename_w[MAX_PATH];

    TRACE("filename %s, defines %p, include %p, entry_point %s, target %s, sflags %#x, "
            "eflags %#x, pump %p, shader %p, error_messages %p, hresult %p.\n",
            debugstr_a(filename), defines, include, debugstr_a(entry_point), debugstr_a(target),
            sflags, eflags, pump, shader, error_messages, hresult);

    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, ARRAY_SIZE(filename_w));

    return D3DX11CompileFromFileW(filename_w, defines, include, entry_point, target,
            sflags, eflags, pump, shader, error_messages, hresult);
}

HRESULT WINAPI D3DX11CompileFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *entry_point, const char *target, UINT sflags, UINT eflags,
        ID3DX11ThreadPump *pump, ID3D10Blob **shader, ID3D10Blob **error_messages, HRESULT *hresult)
{
    HRESULT hr;

    TRACE("filename %s, defines %p, include %p, entry_point %s, target %s, sflags %#x, "
            "eflags %#x, pump %p, shader %p, error_messages %p, hresult %p.\n",
            debugstr_w(filename), defines, include, debugstr_a(entry_point), debugstr_a(target),
            sflags, eflags, pump, shader, error_messages, hresult);

    if (pump)
        FIXME("Unimplemented ID3DX11ThreadPump handling.\n");

    if (!include)
        include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    hr = D3DCompileFromFile(filename, defines, include, entry_point, target, sflags, eflags, shader, error_messages);
    if (hresult)
        *hresult = hr;

    return hr;
}

HRESULT WINAPI D3DX11CreateAsyncMemoryLoader(const void *data, SIZE_T data_size, ID3DX11DataLoader **loader)
{
    struct asyncdataloader *object;

    TRACE("data %p, data_size %Iu, loader %p.\n", data, data_size, loader);

    if (!data || !loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DX11DataLoader_iface.lpVtbl = &memorydataloadervtbl;
    object->data = (void *)data;
    object->size = data_size;

    *loader = &object->ID3DX11DataLoader_iface;

    return S_OK;
}

HRESULT WINAPI D3DX11CreateAsyncFileLoaderA(const char *filename, ID3DX11DataLoader **loader)
{
    WCHAR *filename_w;
    HRESULT hr;
    int len;

    TRACE("filename %s, loader %p.\n", debugstr_a(filename), loader);

    if (!filename || !loader)
        return E_FAIL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = malloc(len * sizeof(*filename_w));
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    hr = D3DX11CreateAsyncFileLoaderW(filename_w, loader);

    free(filename_w);

    return hr;
}

HRESULT WINAPI D3DX11CreateAsyncFileLoaderW(const WCHAR *filename, ID3DX11DataLoader **loader)
{
    struct asyncdataloader *object;

    TRACE("filename %s, loader %p.\n", debugstr_w(filename), loader);

    if (!filename || !loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DX11DataLoader_iface.lpVtbl = &filedataloadervtbl;
    object->u.file.path = malloc((lstrlenW(filename) + 1) * sizeof(WCHAR));
    if (!object->u.file.path)
    {
        free(object);
        return E_OUTOFMEMORY;
    }
    lstrcpyW(object->u.file.path, filename);
    object->data = NULL;
    object->size = 0;

    *loader = &object->ID3DX11DataLoader_iface;

    return S_OK;
}

HRESULT WINAPI D3DX11CreateAsyncResourceLoaderA(HMODULE module, const char *resource, ID3DX11DataLoader **loader)
{
    struct asyncdataloader *object;
    HRSRC rsrc;

    TRACE("module %p, resource %s, loader %p.\n", module, debugstr_a(resource), loader);

    if (!loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (!(rsrc = FindResourceA(module, resource, (const char *)RT_RCDATA)))
    {
        WARN("Failed to find resource.\n");
        free(object);
        return D3DX11_ERR_INVALID_DATA;
    }

    object->ID3DX11DataLoader_iface.lpVtbl = &resourcedataloadervtbl;
    object->u.resource.module = module;
    object->u.resource.rsrc = rsrc;
    object->data = NULL;
    object->size = 0;

    *loader = &object->ID3DX11DataLoader_iface;

    return S_OK;
}

HRESULT WINAPI D3DX11CreateAsyncResourceLoaderW(HMODULE module, const WCHAR *resource, ID3DX11DataLoader **loader)
{
    struct asyncdataloader *object;
    HRSRC rsrc;

    TRACE("module %p, resource %s, loader %p.\n", module, debugstr_w(resource), loader);

    if (!loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (!(rsrc = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA)))
    {
        WARN("Failed to find resource.\n");
        free(object);
        return D3DX11_ERR_INVALID_DATA;
    }

    object->ID3DX11DataLoader_iface.lpVtbl = &resourcedataloadervtbl;
    object->u.resource.module = module;
    object->u.resource.rsrc = rsrc;
    object->data = NULL;
    object->size = 0;

    *loader = &object->ID3DX11DataLoader_iface;

    return S_OK;
}
