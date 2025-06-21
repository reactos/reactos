/*
 * Implementation of DirectX File Interfaces
 *
 * Copyright 2004, 2008, 2010 Christian Costa
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

#include "d3dxof_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dxof);
WINE_DECLARE_DEBUG_CHANNEL(d3dxof_dump);

static const struct IDirectXFileVtbl IDirectXFile_Vtbl;
static const struct IDirectXFileBinaryVtbl IDirectXFileBinary_Vtbl;
static const struct IDirectXFileDataVtbl IDirectXFileData_Vtbl;
static const struct IDirectXFileDataReferenceVtbl IDirectXFileDataReference_Vtbl;
static const struct IDirectXFileEnumObjectVtbl IDirectXFileEnumObject_Vtbl;
static const struct IDirectXFileSaveObjectVtbl IDirectXFileSaveObject_Vtbl;

static HRESULT IDirectXFileDataReferenceImpl_Create(IDirectXFileDataReferenceImpl** ppObj);
static HRESULT IDirectXFileEnumObjectImpl_Create(IDirectXFileEnumObjectImpl** ppObj);
static HRESULT IDirectXFileSaveObjectImpl_Create(IDirectXFileSaveObjectImpl** ppObj);

#define TOKEN_DWORD       41
#define TOKEN_FLOAT       42

HRESULT IDirectXFileImpl_Create(IUnknown* pUnkOuter, LPVOID* ppObj)
{
    IDirectXFileImpl* object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileImpl));
    if (!object)
        return DXFILEERR_BADALLOC;

    object->IDirectXFile_iface.lpVtbl = &IDirectXFile_Vtbl;
    object->ref = 1;

    /* Reserve first template to handle the case sensitive legacy type indexColor */
    object->nb_xtemplates = 1;
    strcpy(object->xtemplates[0].name, "indexColor");
    object->xtemplates[0].nb_members = 2;
    object->xtemplates[0].members[0].type = TOKEN_DWORD;
    object->xtemplates[0].members[0].nb_dims = 0;
    object->xtemplates[0].members[1].type = TOKEN_FLOAT;
    object->xtemplates[0].members[1].nb_dims = 1;
    object->xtemplates[0].members[1].dim_fixed[0] = TRUE;
    object->xtemplates[0].members[1].dim_value[0] = 4;

    *ppObj = &object->IDirectXFile_iface;

    return S_OK;
}

static inline IDirectXFileImpl *impl_from_IDirectXFile(IDirectXFile *iface)
{
    return CONTAINING_RECORD(iface, IDirectXFileImpl, IDirectXFile_iface);
}

static HRESULT WINAPI IDirectXFileImpl_QueryInterface(IDirectXFile *iface, REFIID riid, void **out)
{
    IDirectXFileImpl *object = impl_from_IDirectXFile(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirectXFile))
    {
        *out = &object->IDirectXFile_iface;
    }
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI IDirectXFileImpl_AddRef(IDirectXFile *iface)
{
    IDirectXFileImpl *object = impl_from_IDirectXFile(iface);
    ULONG refcount = InterlockedIncrement(&object->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI IDirectXFileImpl_Release(IDirectXFile *iface)
{
    IDirectXFileImpl *object = impl_from_IDirectXFile(iface);
    ULONG refcount = InterlockedDecrement(&object->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, object);

    return refcount;
}

/*** IDirectXFile methods ***/
static HRESULT WINAPI IDirectXFileImpl_CreateEnumObject(IDirectXFile* iface, LPVOID pvSource, DXFILELOADOPTIONS dwLoadOptions, LPDIRECTXFILEENUMOBJECT* ppEnumObj)
{
  IDirectXFileImpl *This = impl_from_IDirectXFile(iface);
  IDirectXFileEnumObjectImpl* object;
  HRESULT hr;
  LPBYTE file_buffer;
  DWORD file_size;
  DWORD bytes_written;

  TRACE("iface %p, source %p, load_options %#lx, out %p.\n", iface, pvSource, dwLoadOptions, ppEnumObj);

  if (!ppEnumObj)
    return DXFILEERR_BADVALUE;

  /* Only lowest 4 bits are relevant in DXFILELOADOPTIONS */
  dwLoadOptions &= 0xF;

  hr = IDirectXFileEnumObjectImpl_Create(&object);
  if (FAILED(hr))
    return hr;

  if (dwLoadOptions == DXFILELOAD_FROMFILE)
  {
    HANDLE hFile, file_mapping;

    TRACE("Open source file '%s'\n", (char*)pvSource);

    hFile = CreateFileA(pvSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
      TRACE("File '%s' not found\n", (char*)pvSource);
      return DXFILEERR_FILENOTFOUND;
    }

    file_size = GetFileSize(hFile, NULL);

    file_mapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);
    if (!file_mapping)
    {
      hr = DXFILEERR_BADFILETYPE;
      goto error;
    }

    object->mapped_memory = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(file_mapping);
    if (!object->mapped_memory)
    {
      hr = DXFILEERR_BADFILETYPE;
      goto error;
    }
    file_buffer = object->mapped_memory;
  }
  else if (dwLoadOptions == DXFILELOAD_FROMRESOURCE)
  {
    HRSRC resource_info;
    HGLOBAL resource_data;
    LPDXFILELOADRESOURCE lpdxflr = pvSource;

    TRACE("Source in resource (module = %p, name = %s, type = %s)\n", lpdxflr->hModule, debugstr_a(lpdxflr->lpName), debugstr_a(lpdxflr->lpType));

    resource_info = FindResourceA(lpdxflr->hModule, lpdxflr->lpName, lpdxflr->lpType);
    if (!resource_info)
    {
      hr = DXFILEERR_RESOURCENOTFOUND;
      goto error;
    }

    file_size = SizeofResource(lpdxflr->hModule, resource_info);

    resource_data = LoadResource(lpdxflr->hModule, resource_info);
    if (!resource_data)
    {
      hr = DXFILEERR_BADRESOURCE;
      goto error;
    }

    file_buffer = LockResource(resource_data);
    if (!file_buffer)
    {
      hr = DXFILEERR_BADRESOURCE;
      goto error;
    }
  }
  else if (dwLoadOptions == DXFILELOAD_FROMMEMORY)
  {
    LPDXFILELOADMEMORY lpdxflm = pvSource;

    TRACE("Source in memory at %p with size %ld.\n", lpdxflm->lpMemory, lpdxflm->dSize);

    file_buffer = lpdxflm->lpMemory;
    file_size = lpdxflm->dSize;
  }
  else
  {
    FIXME("Source type %ld not handled yet.\n", dwLoadOptions);
    hr = DXFILEERR_NOTDONEYET;
    goto error;
  }

  TRACE("File size is %lu bytes.\n", file_size);

  if (TRACE_ON(d3dxof_dump))
  {
    static USHORT num;
    char tmp[12];
    HANDLE file;
    sprintf(tmp, "file%05u.x", num++);

    file = CreateFileA(tmp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
      WriteFile(file, file_buffer, file_size, &bytes_written, NULL);
      CloseHandle(file);
    }
  }

  object->pDirectXFile = This;

  object->buf.pdxf = This;
  object->buf.token_present = FALSE;
  object->buf.buffer = file_buffer;
  object->buf.rem_bytes = file_size;
  hr = parse_header(&object->buf, &object->decomp_buffer);
  if (FAILED(hr))
    goto error;

  /* Check if there are templates defined before the object */
  if (!parse_templates(&object->buf, TRUE))
  {
    hr = DXFILEERR_PARSEERROR;
    goto error;
  }

  if (TRACE_ON(d3dxof))
  {
    ULONG i;
    TRACE("Registered templates (%lu):\n", This->nb_xtemplates);
    for (i = 1; i < This->nb_xtemplates; i++)
      TRACE("%s - %s\n", This->xtemplates[i].name, debugstr_guid(&This->xtemplates[i].class_id));
  }

  *ppEnumObj = &object->IDirectXFileEnumObject_iface;

  return DXFILE_OK;

error:
  IDirectXFileEnumObject_Release(&object->IDirectXFileEnumObject_iface);
  *ppEnumObj = NULL;

  return hr;
}

static HRESULT WINAPI IDirectXFileImpl_CreateSaveObject(IDirectXFile *iface, const char *filename,
        DXFILEFORMAT format, LPDIRECTXFILESAVEOBJECT *out)
{
    IDirectXFileSaveObjectImpl *object;
    HRESULT hr;

    FIXME("iface %p, filename %s, format %lu, out %p partial stub!\n", iface, debugstr_a(filename),
            format, out);

    if (!filename || !out)
        return E_POINTER;

    hr = IDirectXFileSaveObjectImpl_Create(&object);
    if (SUCCEEDED(hr))
        *out = &object->IDirectXFileSaveObject_iface;
    return hr;
}

static HRESULT WINAPI IDirectXFileImpl_RegisterTemplates(IDirectXFile* iface, LPVOID pvData, DWORD cbSize)
{
  IDirectXFileImpl *This = impl_from_IDirectXFile(iface);
  parse_buffer buf;
  HRESULT hr;
  LPBYTE decomp_buffer = NULL;
  DWORD bytes_written;

  ZeroMemory(&buf, sizeof(buf));
  buf.buffer = pvData;
  buf.rem_bytes = cbSize;
  buf.pdxf = This;

  TRACE("iface %p, data %p, size %lu.\n", iface, pvData, cbSize);

  if (!pvData)
    return DXFILEERR_BADVALUE;

  if (TRACE_ON(d3dxof_dump))
  {
    static USHORT num;
    char tmp[16];
    HANDLE file;
    sprintf(tmp, "template%05u.x", num++);

    file = CreateFileA(tmp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
      WriteFile(file, pvData, cbSize, &bytes_written, NULL);
      CloseHandle(file);
    }
  }

  hr = parse_header(&buf, &decomp_buffer);
  if (FAILED(hr))
    goto cleanup;

  if (!parse_templates(&buf, FALSE))
  {
    hr = DXFILEERR_PARSEERROR;
    goto cleanup;
  }

  if (TRACE_ON(d3dxof))
  {
      unsigned int i;

      TRACE("Registered templates (%lu):\n", This->nb_xtemplates);
      for (i = 1; i < This->nb_xtemplates; ++i)
          TRACE("%s - %s\n", This->xtemplates[i].name, debugstr_guid(&This->xtemplates[i].class_id));
  }

  hr = DXFILE_OK;
cleanup:
  HeapFree(GetProcessHeap(), 0, decomp_buffer);
  return hr;
}

static const IDirectXFileVtbl IDirectXFile_Vtbl =
{
  IDirectXFileImpl_QueryInterface,
  IDirectXFileImpl_AddRef,
  IDirectXFileImpl_Release,
  IDirectXFileImpl_CreateEnumObject,
  IDirectXFileImpl_CreateSaveObject,
  IDirectXFileImpl_RegisterTemplates
};

static HRESULT IDirectXFileBinaryImpl_Create(IDirectXFileBinaryImpl** ppObj)
{
    IDirectXFileBinaryImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileBinaryImpl));
    if (!object)
        return DXFILEERR_BADALLOC;

    object->IDirectXFileBinary_iface.lpVtbl = &IDirectXFileBinary_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return DXFILE_OK;
}

static inline IDirectXFileBinaryImpl *impl_from_IDirectXFileBinary(IDirectXFileBinary *iface)
{
    return CONTAINING_RECORD(iface, IDirectXFileBinaryImpl, IDirectXFileBinary_iface);
}

static HRESULT WINAPI IDirectXFileBinaryImpl_QueryInterface(IDirectXFileBinary *iface,
        REFIID riid, void **out)
{
    IDirectXFileBinaryImpl *object = impl_from_IDirectXFileBinary(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirectXFileObject)
            || IsEqualGUID(riid, &IID_IDirectXFileBinary))
    {
        *out = &object->IDirectXFileBinary_iface;
    }
    else
    {
        *out = NULL;
        /* Do not print an error for interfaces that can be queried to
         * retrieve the type of the object */
        if (!IsEqualGUID(riid, &IID_IDirectXFileData) && !IsEqualGUID(riid, &IID_IDirectXFileDataReference))
            WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI IDirectXFileBinaryImpl_AddRef(IDirectXFileBinary *iface)
{
    IDirectXFileBinaryImpl *object = impl_from_IDirectXFileBinary(iface);
    ULONG refcount = InterlockedIncrement(&object->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI IDirectXFileBinaryImpl_Release(IDirectXFileBinary *iface)
{
    IDirectXFileBinaryImpl *object = impl_from_IDirectXFileBinary(iface);
    ULONG refcount = InterlockedDecrement(&object->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, object);

    return refcount;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetName(IDirectXFileBinary *iface, char *name,
        DWORD *length)
{
    FIXME("iface %p, name %p, length %p stub!\n", iface, name, length);

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetId(IDirectXFileBinary *iface, GUID *guid)
{
    FIXME("iface %p, guid %p stub!\n", iface, guid);

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetSize(IDirectXFileBinary *iface, DWORD *size)
{
    FIXME("iface %p, size %p stub!\n", iface, size);

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetMimeType(IDirectXFileBinary *iface, LPCSTR *mimetype)
{
    FIXME("iface %p, mimetype %p stub!\n", iface, mimetype);

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_Read(IDirectXFileBinary *iface, void *data,
        DWORD size, DWORD *count)
{
    FIXME("iface %p, data %p, size %lu, count %p stub!\n", iface, data, size, count);

    return DXFILEERR_BADVALUE;
}

static const IDirectXFileBinaryVtbl IDirectXFileBinary_Vtbl =
{
    IDirectXFileBinaryImpl_QueryInterface,
    IDirectXFileBinaryImpl_AddRef,
    IDirectXFileBinaryImpl_Release,
    IDirectXFileBinaryImpl_GetName,
    IDirectXFileBinaryImpl_GetId,
    IDirectXFileBinaryImpl_GetSize,
    IDirectXFileBinaryImpl_GetMimeType,
    IDirectXFileBinaryImpl_Read
};

static HRESULT IDirectXFileDataImpl_Create(IDirectXFileDataImpl** ppObj)
{
    IDirectXFileDataImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataImpl));
    if (!object)
        return DXFILEERR_BADALLOC;

    object->IDirectXFileData_iface.lpVtbl = &IDirectXFileData_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return S_OK;
}

static inline IDirectXFileDataImpl *impl_from_IDirectXFileData(IDirectXFileData *iface)
{
    return CONTAINING_RECORD(iface, IDirectXFileDataImpl, IDirectXFileData_iface);
}

static HRESULT WINAPI IDirectXFileDataImpl_QueryInterface(IDirectXFileData *iface, REFIID riid,
        void **out)
{
    IDirectXFileDataImpl *data = impl_from_IDirectXFileData(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirectXFileObject)
            || IsEqualGUID(riid, &IID_IDirectXFileData))
    {
        *out = &data->IDirectXFileData_iface;
    }
    else
    {
        *out = NULL;
        /* Do not print an error for interfaces that can be queried to
         * retrieve the type of the object */
        if (!IsEqualGUID(riid, &IID_IDirectXFileBinary) && !IsEqualGUID(riid, &IID_IDirectXFileDataReference))
            WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI IDirectXFileDataImpl_AddRef(IDirectXFileData *iface)
{
    IDirectXFileDataImpl *data = impl_from_IDirectXFileData(iface);
    ULONG refcount = InterlockedIncrement(&data->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI IDirectXFileDataImpl_Release(IDirectXFileData *iface)
{
    IDirectXFileDataImpl *data = impl_from_IDirectXFileData(iface);
    ULONG refcount = InterlockedDecrement(&data->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (!data->level && !data->from_ref)
        {
            HeapFree(GetProcessHeap(), 0, data->pstrings);
            if (data->pobj)
            {
                HeapFree(GetProcessHeap(), 0, data->pobj->pdata);
                HeapFree(GetProcessHeap(), 0, data->pobj);
            }
        }
        HeapFree(GetProcessHeap(), 0, data);
    }

    return refcount;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetName(IDirectXFileData* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)
{
  IDirectXFileDataImpl *This = impl_from_IDirectXFileData(iface);
  DWORD len;

  TRACE("(%p/%p)->(%p,%p)\n", This, iface, pstrNameBuf, pdwBufLen);

  if (!pdwBufLen)
    return DXFILEERR_BADVALUE;

  len = strlen(This->pobj->name);
  if (len)
    len++;

  if (pstrNameBuf) {
    if (*pdwBufLen < len)
      return DXFILEERR_BADVALUE;
    CopyMemory(pstrNameBuf, This->pobj->name, len);
    /* Even if we return a size of 0, an empty string with a null byte must be returned */
    if (*pdwBufLen && !len)
      pstrNameBuf[0] = 0;
  }
  *pdwBufLen = len;

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetId(IDirectXFileData* iface, LPGUID pGuid)
{
  IDirectXFileDataImpl *This = impl_from_IDirectXFileData(iface);

  TRACE("(%p/%p)->(%p)\n", This, iface, pGuid);

  if (!pGuid)
    return DXFILEERR_BADVALUE;

  *pGuid = This->pobj->class_id;

  return DXFILE_OK;
}

/*** IDirectXFileData methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetData(IDirectXFileData* iface, LPCSTR szMember, DWORD* pcbSize, void** ppvData)
{
  IDirectXFileDataImpl *This = impl_from_IDirectXFileData(iface);

  TRACE("(%p/%p)->(%s,%p,%p)\n", This, iface, debugstr_a(szMember), pcbSize, ppvData);

  if (!pcbSize || !ppvData)
    return DXFILEERR_BADVALUE;

  if (szMember)
  {
    ULONG i;
    for (i = 0; i < This->pobj->nb_members; i++)
      if (!strcmp(This->pobj->members[i].name, szMember))
        break;
    if (i == This->pobj->nb_members)
    {
      WARN("Unknown member '%s'\n", szMember);
      return DXFILEERR_BADDATAREFERENCE;
    }
    *pcbSize = This->pobj->members[i].size;
    *ppvData = This->pobj->root->pdata + This->pobj->members[i].start;
  }
  else
  {
    *pcbSize = This->pobj->size;
    *ppvData = This->pobj->root->pdata + This->pobj->pos_data;
  }

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetType(IDirectXFileData* iface, const GUID** pguid)
{
  IDirectXFileDataImpl *This = impl_from_IDirectXFileData(iface);
  static GUID guid;

  TRACE("(%p/%p)->(%p)\n", This, iface, pguid);

  if (!pguid)
    return DXFILEERR_BADVALUE;

  guid = This->pobj->type;
  *pguid = &guid;

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetNextObject(IDirectXFileData* iface, LPDIRECTXFILEOBJECT* ppChildObj)
{
  HRESULT hr;
  IDirectXFileDataImpl *This = impl_from_IDirectXFileData(iface);

  TRACE("(%p/%p)->(%p)\n", This, iface, ppChildObj);

  if (This->cur_enum_object >= This->pobj->nb_children)
  {
    *ppChildObj = NULL;
    return DXFILEERR_NOMOREOBJECTS;
  }

  if (This->from_ref && (This->level >= 1))
  {
    /* Only 2 levels can be enumerated if the object is obtained from a reference */
    *ppChildObj = NULL;
    return DXFILEERR_NOMOREOBJECTS;
  }

  if (This->pobj->children[This->cur_enum_object]->binary)
  {
    IDirectXFileBinaryImpl *object;

    hr = IDirectXFileBinaryImpl_Create(&object);
    if (FAILED(hr))
      return hr;

    *ppChildObj = (LPDIRECTXFILEOBJECT)&object->IDirectXFileBinary_iface;
  }
  else if (This->pobj->children[This->cur_enum_object]->ptarget)
  {
    IDirectXFileDataReferenceImpl *object;

    hr = IDirectXFileDataReferenceImpl_Create(&object);
    if (FAILED(hr))
      return hr;

    object->ptarget = This->pobj->children[This->cur_enum_object++]->ptarget;

    *ppChildObj = (LPDIRECTXFILEOBJECT)&object->IDirectXFileDataReference_iface;
  }
  else
  {
    IDirectXFileDataImpl *object;

    hr = IDirectXFileDataImpl_Create(&object);
    if (FAILED(hr))
      return hr;

    object->pobj = This->pobj->children[This->cur_enum_object++];
    object->cur_enum_object = 0;
    object->from_ref = This->from_ref;
    object->level = This->level + 1;

    *ppChildObj = (LPDIRECTXFILEOBJECT)&object->IDirectXFileData_iface;
  }

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddDataObject(IDirectXFileData *iface, LPDIRECTXFILEDATA data)
{
    FIXME("iface %p, data %p stub!\n", iface, data);

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddDataReference(IDirectXFileData *iface,
        const char *reference, const GUID *guidref)
{
    FIXME("iface %p, reference %s, guid reference %s stub!\n", iface, debugstr_a(reference),
            debugstr_guid(guidref));

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddBinaryObject(IDirectXFileData *iface, const char *name,
        const GUID *guid, const char *mimetype, void *data, DWORD size)
{
    FIXME("iface %p, name %s, guid %s, mimetype %s, data %p, size %lu stub!\n", iface, debugstr_a(name),
            debugstr_guid(guid), debugstr_a(mimetype), data, size);

    return DXFILEERR_BADVALUE;
}

static const IDirectXFileDataVtbl IDirectXFileData_Vtbl =
{
    IDirectXFileDataImpl_QueryInterface,
    IDirectXFileDataImpl_AddRef,
    IDirectXFileDataImpl_Release,
    IDirectXFileDataImpl_GetName,
    IDirectXFileDataImpl_GetId,
    IDirectXFileDataImpl_GetData,
    IDirectXFileDataImpl_GetType,
    IDirectXFileDataImpl_GetNextObject,
    IDirectXFileDataImpl_AddDataObject,
    IDirectXFileDataImpl_AddDataReference,
    IDirectXFileDataImpl_AddBinaryObject
};

static HRESULT IDirectXFileDataReferenceImpl_Create(IDirectXFileDataReferenceImpl** ppObj)
{
    IDirectXFileDataReferenceImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataReferenceImpl));
    if (!object)
        return DXFILEERR_BADALLOC;

    object->IDirectXFileDataReference_iface.lpVtbl = &IDirectXFileDataReference_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return S_OK;
}

static inline IDirectXFileDataReferenceImpl *impl_from_IDirectXFileDataReference(IDirectXFileDataReference *iface)
{
    return CONTAINING_RECORD(iface, IDirectXFileDataReferenceImpl, IDirectXFileDataReference_iface);
}

static HRESULT WINAPI IDirectXFileDataReferenceImpl_QueryInterface(IDirectXFileDataReference *iface,
        REFIID riid, void **out)
{
    IDirectXFileDataReferenceImpl *object = impl_from_IDirectXFileDataReference(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirectXFileObject)
            || IsEqualGUID(riid, &IID_IDirectXFileDataReference))
    {
        *out = &object->IDirectXFileDataReference_iface;
    }
    else
    {
        /* Do not print an error for interfaces that can be queried to
         * retrieve the type of the object */
        *out = NULL;
        if (!IsEqualGUID(riid, &IID_IDirectXFileData) && !IsEqualGUID(riid, &IID_IDirectXFileBinary))
            WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI IDirectXFileDataReferenceImpl_AddRef(IDirectXFileDataReference *iface)
{
    IDirectXFileDataReferenceImpl *object = impl_from_IDirectXFileDataReference(iface);
    ULONG refcount = InterlockedIncrement(&object->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI IDirectXFileDataReferenceImpl_Release(IDirectXFileDataReference *iface)
{
    IDirectXFileDataReferenceImpl *object = impl_from_IDirectXFileDataReference(iface);
    ULONG refcount = InterlockedDecrement(&object->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, object);

    return refcount;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_GetName(IDirectXFileDataReference* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)
{
  IDirectXFileDataReferenceImpl *This = impl_from_IDirectXFileDataReference(iface);
  DWORD len;

  TRACE("(%p/%p)->(%p,%p)\n", This, iface, pstrNameBuf, pdwBufLen);

  if (!pdwBufLen)
    return DXFILEERR_BADVALUE;

  len = strlen(This->ptarget->name);
  if (len)
    len++;

  if (pstrNameBuf) {
    if (*pdwBufLen < len)
      return DXFILEERR_BADVALUE;
    CopyMemory(pstrNameBuf, This->ptarget->name, len);
    /* Even if we return a size of 0, an empty string with a null byte must be returned */
    if (*pdwBufLen && !len)
      pstrNameBuf[0] = 0;
  }
  *pdwBufLen = len;

  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileDataReferenceImpl_GetId(IDirectXFileDataReference *iface,
        GUID *guid)
{
    IDirectXFileDataReferenceImpl *object = impl_from_IDirectXFileDataReference(iface);

    TRACE("iface %p, guid %p.\n", iface, guid);

    if (!guid)
        return DXFILEERR_BADVALUE;

    *guid = object->ptarget->class_id;

    return DXFILE_OK;
}

/*** IDirectXFileDataReference ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_Resolve(IDirectXFileDataReference* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileDataReferenceImpl *This = impl_from_IDirectXFileDataReference(iface);
  IDirectXFileDataImpl *object;
  HRESULT hr;

  TRACE("(%p/%p)->(%p)\n", This, iface, ppDataObj);

  if (!ppDataObj)
    return DXFILEERR_BADVALUE;

  hr = IDirectXFileDataImpl_Create(&object);
  if (FAILED(hr))
    return hr;

  object->pobj = This->ptarget;
  object->cur_enum_object = 0;
  object->level = 0;
  object->from_ref = TRUE;

  *ppDataObj = &object->IDirectXFileData_iface;

  return DXFILE_OK;
}

static const IDirectXFileDataReferenceVtbl IDirectXFileDataReference_Vtbl =
{
    IDirectXFileDataReferenceImpl_QueryInterface,
    IDirectXFileDataReferenceImpl_AddRef,
    IDirectXFileDataReferenceImpl_Release,
    IDirectXFileDataReferenceImpl_GetName,
    IDirectXFileDataReferenceImpl_GetId,
    IDirectXFileDataReferenceImpl_Resolve
};

static HRESULT IDirectXFileEnumObjectImpl_Create(IDirectXFileEnumObjectImpl** ppObj)
{
    IDirectXFileEnumObjectImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileEnumObjectImpl));
    if (!object)
        return DXFILEERR_BADALLOC;

    object->IDirectXFileEnumObject_iface.lpVtbl = &IDirectXFileEnumObject_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return S_OK;
}

static inline IDirectXFileEnumObjectImpl *impl_from_IDirectXFileEnumObject(IDirectXFileEnumObject *iface)
{
    return CONTAINING_RECORD(iface, IDirectXFileEnumObjectImpl, IDirectXFileEnumObject_iface);
}

static HRESULT WINAPI IDirectXFileEnumObjectImpl_QueryInterface(IDirectXFileEnumObject *iface,
        REFIID riid, void **out)
{
    IDirectXFileEnumObjectImpl *object = impl_from_IDirectXFileEnumObject(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirectXFileEnumObject))
    {
        *out = &object->IDirectXFileEnumObject_iface;
    }
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI IDirectXFileEnumObjectImpl_AddRef(IDirectXFileEnumObject *iface)
{
    IDirectXFileEnumObjectImpl *object = impl_from_IDirectXFileEnumObject(iface);
    ULONG refcount = InterlockedIncrement(&object->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI IDirectXFileEnumObjectImpl_Release(IDirectXFileEnumObject *iface)
{
    IDirectXFileEnumObjectImpl *object = impl_from_IDirectXFileEnumObject(iface);
    ULONG refcount = InterlockedDecrement(&object->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        unsigned int i;

        for (i = 0; i < object->nb_xobjects; ++i)
            IDirectXFileData_Release(object->pRefObjects[i]);
        if (object->mapped_memory)
            UnmapViewOfFile(object->mapped_memory);
        HeapFree(GetProcessHeap(), 0, object->decomp_buffer);
        HeapFree(GetProcessHeap(), 0, object);
    }

    return refcount;
}

/*** IDirectXFileEnumObject methods ***/
static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetNextDataObject(IDirectXFileEnumObject* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = impl_from_IDirectXFileEnumObject(iface);
  IDirectXFileDataImpl* object;
  HRESULT hr;

  if (!ppDataObj)
    return E_POINTER;

  *ppDataObj = NULL;

  TRACE("(%p/%p)->(%p)\n", This, iface, ppDataObj);

  if (This->nb_xobjects >= MAX_OBJECTS)
  {
    ERR("Too many objects\n");
    return DXFILEERR_NOMOREOBJECTS;
  }

  /* Check if there are templates defined before the object */
  if (!parse_templates(&This->buf, TRUE))
    return DXFILEERR_PARSEERROR;

  if (!This->buf.rem_bytes)
    return DXFILEERR_NOMOREOBJECTS;

  hr = IDirectXFileDataImpl_Create(&object);
  if (FAILED(hr))
    return hr;

  object->pobj = HeapAlloc(GetProcessHeap(), 0, sizeof(xobject)*MAX_SUBOBJECTS);
  if (!object->pobj)
  {
    hr = DXFILEERR_BADALLOC;
    goto error;
  }

  object->pstrings = HeapAlloc(GetProcessHeap(), 0, MAX_STRINGS_BUFFER);
  if (!object->pstrings)
  {
    hr = DXFILEERR_BADALLOC;
    goto error;
  }

  object->cur_enum_object = 0;
  object->level = 0;
  object->from_ref = FALSE;

  This->buf.pxo_globals = This->xobjects;
  This->buf.nb_pxo_globals = This->nb_xobjects;
  This->buf.level = 0;
  This->buf.pdata = NULL;
  This->buf.capacity = 0;
  This->buf.cur_pos_data = 0;
  This->buf.cur_pstrings = This->buf.pstrings = object->pstrings;
  This->buf.pxo = This->xobjects[This->nb_xobjects] = This->buf.pxo_tab = object->pobj;
  This->buf.pxo->pdata = NULL;
  This->buf.pxo->nb_subobjects = 1;

  if (!parse_object(&This->buf))
  {
    WARN("Object is not correct\n");
    hr = DXFILEERR_PARSEERROR;
    goto error;
  }

  *ppDataObj = &object->IDirectXFileData_iface;

  /* Get a reference to created object */
  This->pRefObjects[This->nb_xobjects] = &object->IDirectXFileData_iface;
  IDirectXFileData_AddRef(This->pRefObjects[This->nb_xobjects]);

  This->nb_xobjects++;

  return DXFILE_OK;

error:

  IDirectXFileData_Release(&object->IDirectXFileData_iface);

  return hr;
}

static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetDataObjectById(IDirectXFileEnumObject *iface,
        REFGUID guid, LPDIRECTXFILEDATA *data)
{
    FIXME("iface %p, guid %s, data %p stub!\n", iface, debugstr_guid(guid), data);

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetDataObjectByName(IDirectXFileEnumObject *iface,
        const char *name, LPDIRECTXFILEDATA *data)
{
    FIXME("iface %p, name %s, data %p stub!\n", iface, debugstr_a(name), data);

    return DXFILEERR_BADVALUE;
}

static const IDirectXFileEnumObjectVtbl IDirectXFileEnumObject_Vtbl =
{
    IDirectXFileEnumObjectImpl_QueryInterface,
    IDirectXFileEnumObjectImpl_AddRef,
    IDirectXFileEnumObjectImpl_Release,
    IDirectXFileEnumObjectImpl_GetNextDataObject,
    IDirectXFileEnumObjectImpl_GetDataObjectById,
    IDirectXFileEnumObjectImpl_GetDataObjectByName
};

static HRESULT IDirectXFileSaveObjectImpl_Create(IDirectXFileSaveObjectImpl** ppObj)
{
    IDirectXFileSaveObjectImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileSaveObjectImpl));
    if (!object)
        return DXFILEERR_BADALLOC;

    object->IDirectXFileSaveObject_iface.lpVtbl = &IDirectXFileSaveObject_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return S_OK;
}

static inline IDirectXFileSaveObjectImpl *impl_from_IDirectXFileSaveObject(IDirectXFileSaveObject *iface)
{
    return CONTAINING_RECORD(iface, IDirectXFileSaveObjectImpl, IDirectXFileSaveObject_iface);
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_QueryInterface(IDirectXFileSaveObject *iface,
        REFIID riid, void **out)
{
    IDirectXFileSaveObjectImpl *object = impl_from_IDirectXFileSaveObject(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirectXFileSaveObject))
    {
        *out = &object->IDirectXFileSaveObject_iface;
    }
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI IDirectXFileSaveObjectImpl_AddRef(IDirectXFileSaveObject *iface)
{
    IDirectXFileSaveObjectImpl *object = impl_from_IDirectXFileSaveObject(iface);
    ULONG refcount = InterlockedIncrement(&object->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI IDirectXFileSaveObjectImpl_Release(IDirectXFileSaveObject *iface)
{
    IDirectXFileSaveObjectImpl *object = impl_from_IDirectXFileSaveObject(iface);
    ULONG refcount = InterlockedDecrement(&object->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, object);

    return refcount;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_SaveTemplates(IDirectXFileSaveObject *iface,
        DWORD count, const GUID **templates)
{
    FIXME("iface %p, count %lu, templates %p stub!\n", iface, count, templates);

    return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_CreateDataObject(IDirectXFileSaveObject *iface, REFGUID template,
        const char *name, const GUID *guid, DWORD size, void *data, LPDIRECTXFILEDATA *dataobj)
{
    FIXME("iface %p, template %s, name %s, guid %s, size %lu, data %p, dataobj %p stub!\n",
            iface, debugstr_guid(template), debugstr_a(name), debugstr_guid(guid), size, data, dataobj);

    return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_SaveData(IDirectXFileSaveObject *iface, LPDIRECTXFILEDATA data)
{
    FIXME("iface %p, data %p stub!\n", iface, data);

    return DXFILEERR_BADVALUE;
}

static const IDirectXFileSaveObjectVtbl IDirectXFileSaveObject_Vtbl =
{
    IDirectXFileSaveObjectImpl_QueryInterface,
    IDirectXFileSaveObjectImpl_AddRef,
    IDirectXFileSaveObjectImpl_Release,
    IDirectXFileSaveObjectImpl_SaveTemplates,
    IDirectXFileSaveObjectImpl_CreateDataObject,
    IDirectXFileSaveObjectImpl_SaveData
};
