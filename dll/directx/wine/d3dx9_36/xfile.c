/*
 * Copyright (C) 2012 Christian Costa
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


#include "d3dx9_private.h"
#include "d3dx9xof.h"
#undef MAKE_DDHRESULT
#include "dxfile.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

static HRESULT error_dxfile_to_d3dxfile(HRESULT error)
{
    switch (error)
    {
        case DXFILEERR_BADFILETYPE:
            return D3DXFERR_BADFILETYPE;
        case DXFILEERR_BADFILEVERSION:
            return D3DXFERR_BADFILEVERSION;
        case DXFILEERR_BADFILEFLOATSIZE:
            return D3DXFERR_BADFILEFLOATSIZE;
        case DXFILEERR_PARSEERROR:
            return D3DXFERR_PARSEERROR;
        case DXFILEERR_BADVALUE:
            return D3DXFERR_BADVALUE;
        default:
            FIXME("Cannot map error %#lx.\n", error);
            return E_FAIL;
    }
}

struct d3dx9_file
{
    ID3DXFile ID3DXFile_iface;
    LONG ref;
    IDirectXFile *dxfile;
};

struct d3dx9_file_enum_object
{
    ID3DXFileEnumObject ID3DXFileEnumObject_iface;
    LONG ref;
    unsigned int child_count;
    ID3DXFileData **children;
};

struct d3dx9_file_data
{
    ID3DXFileData ID3DXFileData_iface;
    LONG ref;
    BOOL reference;
    IDirectXFileData *dxfile_data;
    unsigned int child_count;
    ID3DXFileData **children;
};

static inline struct d3dx9_file *impl_from_ID3DXFile(ID3DXFile *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_file, ID3DXFile_iface);
}

static inline struct d3dx9_file_enum_object *impl_from_ID3DXFileEnumObject(ID3DXFileEnumObject *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_file_enum_object, ID3DXFileEnumObject_iface);
}

static inline struct d3dx9_file_data *impl_from_ID3DXFileData(ID3DXFileData *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_file_data, ID3DXFileData_iface);
}

static HRESULT WINAPI d3dx9_file_data_QueryInterface(ID3DXFileData *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXFileData)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("Interface %s not found.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_file_data_AddRef(ID3DXFileData *iface)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);
    ULONG refcount = InterlockedIncrement(&file_data->ref);

    TRACE("%p increasing refcount to %lu.\n", file_data, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_file_data_Release(ID3DXFileData *iface)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);
    ULONG refcount = InterlockedDecrement(&file_data->ref);

    TRACE("%p decreasing refcount to %lu.\n", file_data, refcount);

    if (!refcount)
    {
        ULONG i;

        for (i = 0; i < file_data->child_count; ++i)
        {
            ID3DXFileData *child = file_data->children[i];
            child->lpVtbl->Release(child);
        }
        free(file_data->children);
        IDirectXFileData_Release(file_data->dxfile_data);
        free(file_data);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_file_data_GetEnum(ID3DXFileData *iface, ID3DXFileEnumObject **enum_object)
{
    FIXME("iface %p, enum_object %p stub!\n", iface, enum_object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_file_data_GetName(ID3DXFileData *iface, char *name, SIZE_T *size)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);
    DWORD dxfile_size;
    HRESULT ret;

    TRACE("iface %p, name %p, size %p.\n", iface, name, size);

    if (!size)
        return D3DXFERR_BADVALUE;

    dxfile_size = *size;

    ret = IDirectXFileData_GetName(file_data->dxfile_data, name, &dxfile_size);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    if (!dxfile_size)
    {
        /* Contrary to d3dxof, d3dx9_36 returns an empty string with a null byte when no name is available.
         * If the input size is 0, it returns a length of 1 without touching the buffer */
        dxfile_size = 1;
        if (name && *size)
            name[0] = 0;
    }

    *size = dxfile_size;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_data_GetId(ID3DXFileData *iface, GUID *guid)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);
    HRESULT ret;

    TRACE("iface %p, guid %p.\n", iface, guid);

    if (!guid)
        return E_POINTER;

    ret = IDirectXFileData_GetId(file_data->dxfile_data, guid);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_data_Lock(ID3DXFileData *iface, SIZE_T *size, const void **data)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);
    DWORD dxfile_size;
    HRESULT ret;

    TRACE("iface %p, size %p, data %p.\n", iface, size, data);

    if (!size || !data)
        return E_POINTER;

    ret = IDirectXFileData_GetData(file_data->dxfile_data, NULL, &dxfile_size, (void **)data);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    *size = dxfile_size;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_data_Unlock(ID3DXFileData *iface)
{
    TRACE("iface %p.\n", iface);

    /* Nothing to do */

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_data_GetType(ID3DXFileData *iface, GUID *guid)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);
    const GUID *dxfile_guid;
    HRESULT ret;

    TRACE("iface %p, guid %p.\n", iface, guid);

    ret = IDirectXFileData_GetType(file_data->dxfile_data, &dxfile_guid);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    *guid = *dxfile_guid;

    return S_OK;
}

static BOOL WINAPI d3dx9_file_data_IsReference(ID3DXFileData *iface)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);

    TRACE("iface %p.\n", iface);

    return file_data->reference;
}

static HRESULT WINAPI d3dx9_file_data_GetChildren(ID3DXFileData *iface, SIZE_T *children)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);

    TRACE("iface %p, children %p.\n", iface, children);

    if (!children)
        return E_POINTER;

    *children = file_data->child_count;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_data_GetChild(ID3DXFileData *iface, SIZE_T id, ID3DXFileData **object)
{
    struct d3dx9_file_data *file_data = impl_from_ID3DXFileData(iface);

    TRACE("iface %p, id %#Ix, object %p.\n", iface, id, object);

    if (!object)
        return E_POINTER;

    *object = file_data->children[id];
    (*object)->lpVtbl->AddRef(*object);

    return S_OK;
}

static const ID3DXFileDataVtbl d3dx9_file_data_vtbl =
{
    d3dx9_file_data_QueryInterface,
    d3dx9_file_data_AddRef,
    d3dx9_file_data_Release,
    d3dx9_file_data_GetEnum,
    d3dx9_file_data_GetName,
    d3dx9_file_data_GetId,
    d3dx9_file_data_Lock,
    d3dx9_file_data_Unlock,
    d3dx9_file_data_GetType,
    d3dx9_file_data_IsReference,
    d3dx9_file_data_GetChildren,
    d3dx9_file_data_GetChild,
};

static HRESULT d3dx9_file_data_create(IDirectXFileObject *dxfile_object, ID3DXFileData **ret_iface)
{
    struct d3dx9_file_data *object;
    IDirectXFileObject *data_object;
    unsigned int children_array_size = 0;
    HRESULT ret;

    TRACE("dxfile_object %p, ret_iface %p.\n", dxfile_object, ret_iface);

    *ret_iface = NULL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXFileData_iface.lpVtbl = &d3dx9_file_data_vtbl;
    object->ref = 1;

    ret = IDirectXFileObject_QueryInterface(dxfile_object, &IID_IDirectXFileData, (void **)&object->dxfile_data);
    if (FAILED(ret))
    {
        IDirectXFileDataReference *reference;

        ret = IDirectXFileObject_QueryInterface(dxfile_object, &IID_IDirectXFileDataReference, (void **)&reference);
        if (SUCCEEDED(ret))
        {
            ret = IDirectXFileDataReference_Resolve(reference, &object->dxfile_data);
            IUnknown_Release(reference);
            if (FAILED(ret))
            {
                free(object);
                return E_FAIL;
            }
            object->reference = TRUE;
        }
        else
        {
            FIXME("Don't know what to do with binary object\n");
            free(object);
            return E_FAIL;
        }
    }

    while (SUCCEEDED(ret = IDirectXFileData_GetNextObject(object->dxfile_data, &data_object)))
    {
        if (object->child_count >= children_array_size)
        {
            ID3DXFileData **new_children;

            children_array_size = object->children ? children_array_size * 2 : 4;
            new_children = realloc(object->children, sizeof(*object->children) * children_array_size);
            if (!new_children)
            {
                ret = E_OUTOFMEMORY;
                break;
            }
            object->children = new_children;
        }
        ret = d3dx9_file_data_create(data_object, &object->children[object->child_count]);
        IUnknown_Release(data_object);
        if (FAILED(ret))
            break;
        ++object->child_count;
    }
    if (ret != DXFILEERR_NOMOREOBJECTS)
    {
        (&object->ID3DXFileData_iface)->lpVtbl->Release(&object->ID3DXFileData_iface);
        return ret;
    }
    if (object->children)
    {
        ID3DXFileData **new_children;

        new_children = realloc(object->children, sizeof(*object->children) * object->child_count);
        if (new_children)
            object->children = new_children;
    }

    TRACE("Found %u children.\n", object->child_count);

    *ret_iface = &object->ID3DXFileData_iface;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_enum_object_QueryInterface(ID3DXFileEnumObject *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXFileEnumObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_file_enum_object_AddRef(ID3DXFileEnumObject *iface)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);
    ULONG refcount = InterlockedIncrement(&file_enum->ref);

    TRACE("%p increasing refcount to %lu.\n", file_enum, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_file_enum_object_Release(ID3DXFileEnumObject *iface)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);
    ULONG refcount = InterlockedDecrement(&file_enum->ref);

    TRACE("%p decreasing refcount to %lu.\n", file_enum, refcount);

    if (!refcount)
    {
        ULONG i;

        for (i = 0; i < file_enum->child_count; ++i)
        {
            ID3DXFileData *child = file_enum->children[i];
            child->lpVtbl->Release(child);
        }
        free(file_enum->children);
        free(file_enum);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetFile(ID3DXFileEnumObject *iface, ID3DXFile **file)
{
    FIXME("iface %p, file %p stub!\n", iface, file);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetChildren(ID3DXFileEnumObject *iface, SIZE_T *children)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);

    TRACE("iface %p, children %p.\n", iface, children);

    if (!children)
        return E_POINTER;

    *children = file_enum->child_count;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetChild(ID3DXFileEnumObject *iface, SIZE_T id, ID3DXFileData **object)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);

    TRACE("iface %p, id %#Ix, object %p.\n", iface, id, object);

    if (!object)
        return E_POINTER;

    *object = file_enum->children[id];
    (*object)->lpVtbl->AddRef(*object);

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetDataObjectById(ID3DXFileEnumObject *iface,
        REFGUID guid, ID3DXFileData **object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetDataObjectByName(ID3DXFileEnumObject *iface,
        const char *name, ID3DXFileData **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static const ID3DXFileEnumObjectVtbl d3dx9_file_enum_object_vtbl =
{
    d3dx9_file_enum_object_QueryInterface,
    d3dx9_file_enum_object_AddRef,
    d3dx9_file_enum_object_Release,
    d3dx9_file_enum_object_GetFile,
    d3dx9_file_enum_object_GetChildren,
    d3dx9_file_enum_object_GetChild,
    d3dx9_file_enum_object_GetDataObjectById,
    d3dx9_file_enum_object_GetDataObjectByName,
};

static HRESULT WINAPI d3dx9_file_QueryInterface(ID3DXFile *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXFile)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_file_AddRef(ID3DXFile *iface)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    ULONG refcount = InterlockedIncrement(&file->ref);

    TRACE("%p increasing refcount to %lu.\n", file, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_file_Release(ID3DXFile *iface)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    ULONG refcount = InterlockedDecrement(&file->ref);

    TRACE("%p decreasing refcount to %lu.\n", file, refcount);

    if (!refcount)
    {
        IDirectXFile_Release(file->dxfile);
        free(file);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_file_CreateEnumObject(ID3DXFile *iface, const void *source,
        D3DXF_FILELOADOPTIONS options, ID3DXFileEnumObject **enum_object)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    struct d3dx9_file_enum_object *object;
    IDirectXFileEnumObject *dxfile_enum_object;
    void *dxfile_source;
    DXFILELOADOPTIONS dxfile_options;
    DXFILELOADRESOURCE dxfile_resource;
    DXFILELOADMEMORY dxfile_memory;
    IDirectXFileData *data_object;
    unsigned children_array_size = 0;
    HRESULT ret;

    TRACE("iface %p, source %p, options %#lx, enum_object %p.\n", iface, source, options, enum_object);

    if (!enum_object)
        return E_POINTER;

    *enum_object = NULL;

    if (options == D3DXF_FILELOAD_FROMFILE)
    {
        dxfile_source = (void*)source;
        dxfile_options = DXFILELOAD_FROMFILE;
    }
    else if (options == D3DXF_FILELOAD_FROMRESOURCE)
    {
        D3DXF_FILELOADRESOURCE *resource = (D3DXF_FILELOADRESOURCE*)source;

        dxfile_resource.hModule = resource->hModule;
        dxfile_resource.lpName = resource->lpName;
        dxfile_resource.lpType = resource->lpType;
        dxfile_source = &dxfile_resource;
        dxfile_options = DXFILELOAD_FROMRESOURCE;
    }
    else if (options == D3DXF_FILELOAD_FROMMEMORY)
    {
        D3DXF_FILELOADMEMORY *memory = (D3DXF_FILELOADMEMORY*)source;

        dxfile_memory.lpMemory = (void *)memory->lpMemory;
        dxfile_memory.dSize = memory->dSize;
        dxfile_source = &dxfile_memory;
        dxfile_options = DXFILELOAD_FROMMEMORY;
    }
    else
    {
        FIXME("Source type %lu not handled yet.\n", options);
        return E_NOTIMPL;
    }

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXFileEnumObject_iface.lpVtbl = &d3dx9_file_enum_object_vtbl;
    object->ref = 1;

    ret = IDirectXFile_CreateEnumObject(file->dxfile, dxfile_source, dxfile_options, &dxfile_enum_object);

    if (ret != S_OK)
    {
        free(object);
        return ret;
    }

    /* Fill enum object with top level data objects */
    while (SUCCEEDED(ret = IDirectXFileEnumObject_GetNextDataObject(dxfile_enum_object, &data_object)))
    {
        if (object->child_count >= children_array_size)
        {
            ID3DXFileData **new_children;

            children_array_size = object->children ? children_array_size * 2 : 4;
            new_children = realloc(object->children, sizeof(*object->children) * children_array_size);
            if (!new_children)
            {
                ret = E_OUTOFMEMORY;
                break;
            }
            object->children = new_children;
        }
        ret = d3dx9_file_data_create((IDirectXFileObject*)data_object,
                &object->children[object->child_count]);
        IUnknown_Release(data_object);
        if (FAILED(ret))
            break;
        ++object->child_count;
    }
    if (object->children)
    {
        ID3DXFileData **new_children;

        new_children = realloc(object->children, sizeof(*object->children) * object->child_count);
        if (new_children)
            object->children = new_children;
    }

    IDirectXFileEnumObject_Release(dxfile_enum_object);

    if (ret != DXFILEERR_NOMOREOBJECTS)
        WARN("Cannot get all top level data objects\n");

    TRACE("Found %u children.\n", object->child_count);

    *enum_object = &object->ID3DXFileEnumObject_iface;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_CreateSaveObject(ID3DXFile *iface, const void *data,
        D3DXF_FILESAVEOPTIONS options, D3DXF_FILEFORMAT format, ID3DXFileSaveObject **save_object)
{
    FIXME("iface %p, data %p, options %#lx, format %#lx, save_object %p stub!\n",
            iface, data, options, format, save_object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_file_RegisterTemplates(ID3DXFile *iface, const void *data, SIZE_T size)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    HRESULT ret;

    TRACE("iface %p, data %p, size %Iu.\n", iface, data, size);

    ret = IDirectXFile_RegisterTemplates(file->dxfile, (void *)data, size);
    if (ret != DXFILE_OK)
    {
        WARN("Error registering templates, hr %#lx.\n", ret);
        return error_dxfile_to_d3dxfile(ret);
    }

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_RegisterEnumTemplates(ID3DXFile *iface, ID3DXFileEnumObject *enum_object)
{
    FIXME("iface %p, enum_object %p stub!\n", iface, enum_object);

    return E_NOTIMPL;
}

static const ID3DXFileVtbl d3dx9_file_vtbl =
{
    d3dx9_file_QueryInterface,
    d3dx9_file_AddRef,
    d3dx9_file_Release,
    d3dx9_file_CreateEnumObject,
    d3dx9_file_CreateSaveObject,
    d3dx9_file_RegisterTemplates,
    d3dx9_file_RegisterEnumTemplates,
};

HRESULT WINAPI D3DXFileCreate(ID3DXFile **d3dxfile)
{
    struct d3dx9_file *object;
    HRESULT ret;

    TRACE("d3dxfile %p.\n", d3dxfile);

    if (!d3dxfile)
        return E_POINTER;

    *d3dxfile = NULL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    ret = DirectXFileCreate(&object->dxfile);
    if (ret != S_OK)
    {
        free(object);
        if (ret == E_OUTOFMEMORY)
            return ret;
        return E_FAIL;
    }

    object->ID3DXFile_iface.lpVtbl = &d3dx9_file_vtbl;
    object->ref = 1;

    *d3dxfile = &object->ID3DXFile_iface;

    return S_OK;
}
