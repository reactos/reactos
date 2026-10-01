/*
 * Direct3D blob file
 *
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
 *
 */

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

struct d3dcompiler_blob
{
    ID3DBlob ID3DBlob_iface;
    LONG refcount;

    SIZE_T size;
    void *data;
};

static inline struct d3dcompiler_blob *impl_from_ID3DBlob(ID3DBlob *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_blob, ID3DBlob_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3dcompiler_blob_QueryInterface(ID3DBlob *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10Blob)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_blob_AddRef(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);
    ULONG refcount = InterlockedIncrement(&blob->refcount);

    TRACE("%p increasing refcount to %lu.\n", blob, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_blob_Release(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);
    ULONG refcount = InterlockedDecrement(&blob->refcount);

    TRACE("%p decreasing refcount to %lu.\n", blob, refcount);

    if (!refcount)
    {
        free(blob->data);
        free(blob);
    }

    return refcount;
}

/* ID3DBlob methods */

static void * STDMETHODCALLTYPE d3dcompiler_blob_GetBufferPointer(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);

    TRACE("iface %p\n", iface);

    return blob->data;
}

static SIZE_T STDMETHODCALLTYPE d3dcompiler_blob_GetBufferSize(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);

    TRACE("iface %p\n", iface);

    return blob->size;
}

static const struct ID3D10BlobVtbl d3dcompiler_blob_vtbl =
{
    /* IUnknown methods */
    d3dcompiler_blob_QueryInterface,
    d3dcompiler_blob_AddRef,
    d3dcompiler_blob_Release,
    /* ID3DBlob methods */
    d3dcompiler_blob_GetBufferPointer,
    d3dcompiler_blob_GetBufferSize,
};

static HRESULT d3dcompiler_blob_init(struct d3dcompiler_blob *blob, SIZE_T data_size)
{
    blob->ID3DBlob_iface.lpVtbl = &d3dcompiler_blob_vtbl;
    blob->refcount = 1;
    blob->size = data_size;

    blob->data = calloc(1, data_size);
    if (!blob->data)
    {
        ERR("Failed to allocate D3D blob data memory\n");
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT WINAPI D3DCreateBlob(SIZE_T data_size, ID3DBlob **blob)
{
    struct d3dcompiler_blob *object;
    HRESULT hr;

    TRACE("data_size %Iu, blob %p.\n", data_size, blob);

    if (!blob)
    {
        WARN("Invalid blob specified.\n");
        return D3DERR_INVALIDCALL;
    }

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = d3dcompiler_blob_init(object, data_size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize blob, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    *blob = &object->ID3DBlob_iface;

    TRACE("Created ID3DBlob %p\n", *blob);

    return S_OK;
}

static BOOL check_blob_part(DWORD tag, D3D_BLOB_PART part)
{
    BOOL add = FALSE;

    switch(part)
    {
        case D3D_BLOB_INPUT_SIGNATURE_BLOB:
            if (tag == TAG_ISGN) add = TRUE;
            break;

        case D3D_BLOB_OUTPUT_SIGNATURE_BLOB:
            if (tag == TAG_OSGN || tag == TAG_OSG5) add = TRUE;
            break;

        case D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB:
            if (tag == TAG_ISGN || tag == TAG_OSGN || tag == TAG_OSG5) add = TRUE;
            break;

        case D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB:
            if (tag == TAG_PCSG) add = TRUE;
            break;

        case D3D_BLOB_ALL_SIGNATURE_BLOB:
            if (tag == TAG_ISGN || tag == TAG_OSGN || tag == TAG_OSG5 || tag == TAG_PCSG) add = TRUE;
            break;

        case D3D_BLOB_DEBUG_INFO:
            if (tag == TAG_SDBG) add = TRUE;
            break;

        case D3D_BLOB_LEGACY_SHADER:
            if (tag == TAG_Aon9) add = TRUE;
            break;

        case D3D_BLOB_XNA_PREPASS_SHADER:
            if (tag == TAG_XNAP) add = TRUE;
            break;

        case D3D_BLOB_XNA_SHADER:
            if (tag == TAG_XNAS) add = TRUE;
            break;

        default:
            FIXME("Unhandled D3D_BLOB_PART %s.\n", debug_d3dcompiler_d3d_blob_part(part));
            break;
    }

    TRACE("%s tag %s\n", add ? "Add" : "Skip", debugstr_fourcc(tag));

    return add;
}

static HRESULT d3dcompiler_get_blob_part(const void *data, SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob)
{
    const struct vkd3d_shader_code src_dxbc = {.code = data, .size = data_size};
    struct vkd3d_shader_dxbc_section_desc *sections;
    struct vkd3d_shader_dxbc_desc src_dxbc_desc;
    struct vkd3d_shader_code dst_dxbc;
    unsigned int section_count, i;
    HRESULT hr;
    int ret;

    if (!data || data_size < DXBC_HEADER_SIZE || flags || !blob)
    {
        WARN("Invalid arguments: data %p, data_size %Iu, flags %#x, blob %p.\n", data, data_size, flags, blob);
        return D3DERR_INVALIDCALL;
    }

    if (part > D3D_BLOB_TEST_COMPILE_PERF
            || (part < D3D_BLOB_TEST_ALTERNATE_SHADER && part > D3D_BLOB_XNA_SHADER))
    {
        WARN("Invalid D3D_BLOB_PART: part %s\n", debug_d3dcompiler_d3d_blob_part(part));
        return D3DERR_INVALIDCALL;
    }

    if ((ret = vkd3d_shader_parse_dxbc(&src_dxbc, 0, &src_dxbc_desc, NULL)) < 0)
    {
        WARN("Failed to parse source data, ret %d.\n", ret);
        return E_FAIL;
    }

    if (!(sections = calloc(src_dxbc_desc.section_count, sizeof(*sections))))
    {
        ERR("Failed to allocate sections memory.\n");
        vkd3d_shader_free_dxbc(&src_dxbc_desc);
        return E_OUTOFMEMORY;
    }

    for (i = 0, section_count = 0; i < src_dxbc_desc.section_count; ++i)
    {
        const struct vkd3d_shader_dxbc_section_desc *src_section = &src_dxbc_desc.sections[i];

        if (check_blob_part(src_section->tag, part))
            sections[section_count++] = *src_section;
    }

    switch(part)
    {
        case D3D_BLOB_INPUT_SIGNATURE_BLOB:
        case D3D_BLOB_OUTPUT_SIGNATURE_BLOB:
        case D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB:
        case D3D_BLOB_DEBUG_INFO:
        case D3D_BLOB_LEGACY_SHADER:
        case D3D_BLOB_XNA_PREPASS_SHADER:
        case D3D_BLOB_XNA_SHADER:
            if (section_count != 1)
                section_count = 0;
            break;

        case D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB:
            if (section_count != 2)
                section_count = 0;
            break;

        case D3D_BLOB_ALL_SIGNATURE_BLOB:
            if (section_count != 3)
                section_count = 0;
            break;

        default:
            FIXME("Unhandled D3D_BLOB_PART %s.\n", debug_d3dcompiler_d3d_blob_part(part));
            break;
    }

    if (!section_count)
    {
        WARN("Nothing to write into the blob.\n");
        hr = E_FAIL;
        goto done;
    }

    /* some parts aren't full DXBCs, they contain only the data */
    if (section_count == 1 && (part == D3D_BLOB_DEBUG_INFO || part == D3D_BLOB_LEGACY_SHADER
            || part == D3D_BLOB_XNA_PREPASS_SHADER || part == D3D_BLOB_XNA_SHADER))
    {
        dst_dxbc = sections[0].data;
    }
    else if ((ret = vkd3d_shader_serialize_dxbc(section_count, sections, &dst_dxbc, NULL) < 0))
    {
        WARN("Failed to serialise DXBC, ret %d.\n", ret);
        hr = E_FAIL;
        goto done;
    }

    if (FAILED(hr = D3DCreateBlob(dst_dxbc.size, blob)))
        WARN("Failed to create blob, hr %#lx.\n", hr);
    else
        memcpy(ID3D10Blob_GetBufferPointer(*blob), dst_dxbc.code, dst_dxbc.size);
    if (dst_dxbc.code != sections[0].data.code)
        vkd3d_shader_free_shader_code(&dst_dxbc);

done:
    free(sections);
    vkd3d_shader_free_dxbc(&src_dxbc_desc);

    return hr;
}

static BOOL check_blob_strip(DWORD tag, UINT flags)
{
    BOOL add = TRUE;

    if (flags & D3DCOMPILER_STRIP_TEST_BLOBS) FIXME("Unhandled flag D3DCOMPILER_STRIP_TEST_BLOBS.\n");

    switch(tag)
    {
        case TAG_RDEF:
        case TAG_STAT:
            if (flags & D3DCOMPILER_STRIP_REFLECTION_DATA) add = FALSE;
            break;

        case TAG_SDBG:
            if (flags & D3DCOMPILER_STRIP_DEBUG_INFO) add = FALSE;
            break;

        default:
            break;
    }

    TRACE("%s tag %s\n", add ? "Add" : "Skip", debugstr_fourcc(tag));

    return add;
}

static HRESULT d3dcompiler_strip_shader(const void *data, SIZE_T data_size, UINT flags, ID3DBlob **blob)
{
    const struct vkd3d_shader_code src_dxbc = {.code = data, .size = data_size};
    struct vkd3d_shader_dxbc_section_desc *sections;
    struct vkd3d_shader_dxbc_desc src_dxbc_desc;
    struct vkd3d_shader_code dst_dxbc;
    unsigned int section_count, i;
    HRESULT hr;
    int ret;

    if (!blob)
    {
        WARN("NULL for blob specified\n");
        return E_FAIL;
    }

    if (!data || data_size < DXBC_HEADER_SIZE)
    {
        WARN("Invalid arguments: data %p, data_size %Iu.\n", data, data_size);
        return D3DERR_INVALIDCALL;
    }

    if ((ret = vkd3d_shader_parse_dxbc(&src_dxbc, 0, &src_dxbc_desc, NULL)) < 0)
    {
        WARN("Failed to parse source data, ret %d.\n", ret);
        return E_FAIL;
    }

    /* src_dxbc.count >= dst_dxbc.count */
    if (!(sections = calloc(src_dxbc_desc.section_count, sizeof(*sections))))
    {
        ERR("Failed to allocate sections memory.\n");
        vkd3d_shader_free_dxbc(&src_dxbc_desc);
        return E_OUTOFMEMORY;
    }

    for (i = 0, section_count = 0; i < src_dxbc_desc.section_count; ++i)
    {
        const struct vkd3d_shader_dxbc_section_desc *src_section = &src_dxbc_desc.sections[i];

        if (check_blob_strip(src_section->tag, flags))
            sections[section_count++] = *src_section;
    }

    if ((ret = vkd3d_shader_serialize_dxbc(section_count, sections, &dst_dxbc, NULL) < 0))
    {
        WARN("Failed to serialise DXBC, ret %d.\n", ret);
        hr = E_FAIL;
        goto done;
    }

    if (FAILED(hr = D3DCreateBlob(dst_dxbc.size, blob)))
        WARN("Failed to create blob, hr %#lx.\n", hr);
    else
        memcpy(ID3D10Blob_GetBufferPointer(*blob), dst_dxbc.code, dst_dxbc.size);
    vkd3d_shader_free_shader_code(&dst_dxbc);

done:
    free(sections);
    vkd3d_shader_free_dxbc(&src_dxbc_desc);

    return hr;
}

HRESULT WINAPI D3DGetBlobPart(const void *data, SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob)
{
    TRACE("data %p, data_size %Iu, part %s, flags %#x, blob %p.\n", data,
           data_size, debug_d3dcompiler_d3d_blob_part(part), flags, blob);

    return d3dcompiler_get_blob_part(data, data_size, part, flags, blob);
}

HRESULT WINAPI D3DGetInputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %Iu, blob %p.\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %Iu, blob %p.\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetInputAndOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %Iu, blob %p.\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetDebugInfo(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %Iu, blob %p.\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_DEBUG_INFO, 0, blob);
}

HRESULT WINAPI D3DStripShader(const void *data, SIZE_T data_size, UINT flags, ID3D10Blob **blob)
{
    TRACE("data %p, data_size %Iu, flags %#x, blob %p.\n", data, data_size, flags, blob);

    return d3dcompiler_strip_shader(data, data_size, flags, blob);
}

HRESULT WINAPI D3DReadFileToBlob(const WCHAR *filename, ID3DBlob **contents)
{
    struct d3dcompiler_blob *object;
    SIZE_T data_size;
    DWORD read_size;
    HANDLE file;
    HRESULT hr;

    TRACE("filename %s, contents %p.\n", debugstr_w(filename), contents);

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    data_size = GetFileSize(file, NULL);
    if (data_size == INVALID_FILE_SIZE)
    {
        CloseHandle(file);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        CloseHandle(file);
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = d3dcompiler_blob_init(object, data_size)))
    {
        WARN("Failed to initialise blob, hr %#lx.\n", hr);
        CloseHandle(file);
        free(object);
        return hr;
    }

    if (!ReadFile(file, object->data, data_size, &read_size, NULL) || (read_size != data_size))
    {
        WARN("Failed to read file contents.\n");
        CloseHandle(file);
        free(object->data);
        free(object);
        return E_FAIL;
    }
    CloseHandle(file);
    object->size = read_size;

    *contents = &object->ID3DBlob_iface;

    TRACE("Returning ID3DBlob %p.\n", *contents);

    return S_OK;
}

HRESULT WINAPI D3DWriteBlobToFile(ID3DBlob *blob, const WCHAR *filename, BOOL overwrite)
{
    DWORD written_size;
    SIZE_T data_size;
    HANDLE file;
    BOOL ret;

    TRACE("blob %p, filename %s, overwrite %#x.\n", blob, debugstr_w(filename), overwrite);

    file = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, overwrite ? CREATE_ALWAYS : CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    data_size = ID3D10Blob_GetBufferSize(blob);
    ret = WriteFile(file, ID3D10Blob_GetBufferPointer(blob), data_size, &written_size, NULL);
    CloseHandle(file);
    if (!ret || data_size != written_size)
    {
        WARN("Failed to write blob contents.\n");
        return E_FAIL;
    }

    return S_OK;
}
