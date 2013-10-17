/*
 * Copyright 2012 Alistair Leslie-Hughes
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

#include "config.h"
#include <stdarg.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "olectl.h"
#include "dispex.h"
#include "ntsecapi.h"
#include "scrrun.h"
#include "scrrun_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrrun);

struct folder {
    IFolder IFolder_iface;
    LONG ref;
};

struct file {
    IFile IFile_iface;
    LONG ref;

    WCHAR *path;
};

struct textstream {
    ITextStream ITextStream_iface;
    LONG ref;

    IOMode mode;
};

enum iotype {
    IORead,
    IOWrite
};

static inline struct folder *impl_from_IFolder(IFolder *iface)
{
    return CONTAINING_RECORD(iface, struct folder, IFolder_iface);
}

static inline struct file *impl_from_IFile(IFile *iface)
{
    return CONTAINING_RECORD(iface, struct file, IFile_iface);
}

static inline struct textstream *impl_from_ITextStream(ITextStream *iface)
{
    return CONTAINING_RECORD(iface, struct textstream, ITextStream_iface);
}

static inline HRESULT create_error(DWORD err)
{
    switch(err) {
    case ERROR_FILE_NOT_FOUND: return CTL_E_FILENOTFOUND;
    case ERROR_PATH_NOT_FOUND: return CTL_E_PATHNOTFOUND;
    case ERROR_ACCESS_DENIED: return CTL_E_PERMISSIONDENIED;
    case ERROR_FILE_EXISTS: return CTL_E_FILEALREADYEXISTS;
    case ERROR_ALREADY_EXISTS: return CTL_E_FILEALREADYEXISTS;
    default:
        FIXME("Unsupported error code: %d\n", err);
        return E_FAIL;
    }
}

static int textstream_check_iomode(struct textstream *This, enum iotype type)
{
    if (type == IORead)
        return This->mode == ForWriting || This->mode == ForAppending;
    else
        return 1;
}

static HRESULT WINAPI textstream_QueryInterface(ITextStream *iface, REFIID riid, void **obj)
{
    struct textstream *This = impl_from_ITextStream(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_ITextStream) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        ITextStream_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI textstream_AddRef(ITextStream *iface)
{
    struct textstream *This = impl_from_ITextStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI textstream_Release(ITextStream *iface)
{
    struct textstream *This = impl_from_ITextStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI textstream_GetTypeInfoCount(ITextStream *iface, UINT *pctinfo)
{
    struct textstream *This = impl_from_ITextStream(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI textstream_GetTypeInfo(ITextStream *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    struct textstream *This = impl_from_ITextStream(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(ITextStream_tid, ppTInfo);
}

static HRESULT WINAPI textstream_GetIDsOfNames(ITextStream *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    struct textstream *This = impl_from_ITextStream(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(ITextStream_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI textstream_Invoke(ITextStream *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct textstream *This = impl_from_ITextStream(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextStream_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI textstream_get_Line(ITextStream *iface, LONG *line)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, line);
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_get_Column(ITextStream *iface, LONG *column)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, column);
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_get_AtEndOfStream(ITextStream *iface, VARIANT_BOOL *eos)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, eos);
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_get_AtEndOfLine(ITextStream *iface, VARIANT_BOOL *eol)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, eol);
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_Read(ITextStream *iface, LONG len, BSTR *text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, text);

    if (textstream_check_iomode(This, IORead))
        return CTL_E_BADFILEMODE;

    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_ReadLine(ITextStream *iface, BSTR *text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, text);

    if (textstream_check_iomode(This, IORead))
        return CTL_E_BADFILEMODE;

    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_ReadAll(ITextStream *iface, BSTR *text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, text);

    if (textstream_check_iomode(This, IORead))
        return CTL_E_BADFILEMODE;

    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_Write(ITextStream *iface, BSTR text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(text));
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_WriteLine(ITextStream *iface, BSTR text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(text));
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_WriteBlankLines(ITextStream *iface, LONG lines)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%d): stub\n", This, lines);
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_Skip(ITextStream *iface, LONG count)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%d): stub\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_SkipLine(ITextStream *iface)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_Close(ITextStream *iface)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static const ITextStreamVtbl textstreamvtbl = {
    textstream_QueryInterface,
    textstream_AddRef,
    textstream_Release,
    textstream_GetTypeInfoCount,
    textstream_GetTypeInfo,
    textstream_GetIDsOfNames,
    textstream_Invoke,
    textstream_get_Line,
    textstream_get_Column,
    textstream_get_AtEndOfStream,
    textstream_get_AtEndOfLine,
    textstream_Read,
    textstream_ReadLine,
    textstream_ReadAll,
    textstream_Write,
    textstream_WriteLine,
    textstream_WriteBlankLines,
    textstream_Skip,
    textstream_SkipLine,
    textstream_Close
};

static HRESULT create_textstream(IOMode mode, ITextStream **ret)
{
    struct textstream *stream;

    stream = heap_alloc(sizeof(struct textstream));
    if (!stream) return E_OUTOFMEMORY;

    stream->ITextStream_iface.lpVtbl = &textstreamvtbl;
    stream->ref = 1;
    stream->mode = mode;

    *ret = &stream->ITextStream_iface;
    return S_OK;
}

static HRESULT WINAPI folder_QueryInterface(IFolder *iface, REFIID riid, void **obj)
{
    struct folder *This = impl_from_IFolder(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID( riid, &IID_IFolder ) ||
        IsEqualGUID( riid, &IID_IUnknown))
    {
        *obj = iface;
        IFolder_AddRef(iface);
    }
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI folder_AddRef(IFolder *iface)
{
    struct folder *This = impl_from_IFolder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI folder_Release(IFolder *iface)
{
    struct folder *This = impl_from_IFolder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI folder_GetTypeInfoCount(IFolder *iface, UINT *pctinfo)
{
    struct folder *This = impl_from_IFolder(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI folder_GetTypeInfo(IFolder *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    struct folder *This = impl_from_IFolder(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IFolder_tid, ppTInfo);
}

static HRESULT WINAPI folder_GetIDsOfNames(IFolder *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    struct folder *This = impl_from_IFolder(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IFolder_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI folder_Invoke(IFolder *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct folder *This = impl_from_IFolder(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IFolder_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI folder_get_Path(IFolder *iface, BSTR *path)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Name(IFolder *iface, BSTR *name)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_put_Name(IFolder *iface, BSTR name)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_ShortPath(IFolder *iface, BSTR *path)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_ShortName(IFolder *iface, BSTR *name)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Drive(IFolder *iface, IDrive **drive)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, drive);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_ParentFolder(IFolder *iface, IFolder **parent)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, parent);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Attributes(IFolder *iface, FileAttribute *attr)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_put_Attributes(IFolder *iface, FileAttribute attr)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(0x%x): stub\n", This, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_DateCreated(IFolder *iface, DATE *date)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, date);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_DateLastModified(IFolder *iface, DATE *date)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, date);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_DateLastAccessed(IFolder *iface, DATE *date)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, date);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Type(IFolder *iface, BSTR *type)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_Delete(IFolder *iface, VARIANT_BOOL force)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%x): stub\n", This, force);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_Copy(IFolder *iface, BSTR dest, VARIANT_BOOL overwrite)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s %x): stub\n", This, debugstr_w(dest), overwrite);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_Move(IFolder *iface, BSTR dest)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(dest));
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_IsRootFolder(IFolder *iface, VARIANT_BOOL *isroot)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, isroot);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Size(IFolder *iface, VARIANT *size)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_SubFolders(IFolder *iface, IFolderCollection **folders)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, folders);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Files(IFolder *iface, IFileCollection **files)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, files);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_CreateTextFile(IFolder *iface, BSTR filename, VARIANT_BOOL overwrite,
    VARIANT_BOOL unicode, ITextStream **stream)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s %x %x %p): stub\n", This, debugstr_w(filename), overwrite, unicode, stream);
    return E_NOTIMPL;
}

static const IFolderVtbl foldervtbl = {
    folder_QueryInterface,
    folder_AddRef,
    folder_Release,
    folder_GetTypeInfoCount,
    folder_GetTypeInfo,
    folder_GetIDsOfNames,
    folder_Invoke,
    folder_get_Path,
    folder_get_Name,
    folder_put_Name,
    folder_get_ShortPath,
    folder_get_ShortName,
    folder_get_Drive,
    folder_get_ParentFolder,
    folder_get_Attributes,
    folder_put_Attributes,
    folder_get_DateCreated,
    folder_get_DateLastModified,
    folder_get_DateLastAccessed,
    folder_get_Type,
    folder_Delete,
    folder_Copy,
    folder_Move,
    folder_get_IsRootFolder,
    folder_get_Size,
    folder_get_SubFolders,
    folder_get_Files,
    folder_CreateTextFile
};

static HRESULT create_folder(IFolder **folder)
{
    struct folder *This;

    This = heap_alloc(sizeof(struct folder));
    if (!This) return E_OUTOFMEMORY;

    This->IFolder_iface.lpVtbl = &foldervtbl;
    This->ref = 1;

    *folder = &This->IFolder_iface;

    return S_OK;
}

static HRESULT WINAPI file_QueryInterface(IFile *iface, REFIID riid, void **obj)
{
    struct file *This = impl_from_IFile(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IFile) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IFile_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI file_AddRef(IFile *iface)
{
    struct file *This = impl_from_IFile(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI file_Release(IFile *iface)
{
    struct file *This = impl_from_IFile(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This->path);

    return ref;
}

static HRESULT WINAPI file_GetTypeInfoCount(IFile *iface, UINT *pctinfo)
{
    struct file *This = impl_from_IFile(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI file_GetTypeInfo(IFile *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    struct file *This = impl_from_IFile(iface);

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IFile_tid, ppTInfo);
}

static HRESULT WINAPI file_GetIDsOfNames(IFile *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct file *This = impl_from_IFile(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IFile_tid, &typeinfo);
    if(SUCCEEDED(hr)) {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI file_Invoke(IFile *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct file *This = impl_from_IFile(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IFile_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI file_get_Path(IFile *iface, BSTR *pbstrPath)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pbstrPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_Name(IFile *iface, BSTR *pbstrName)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_put_Name(IFile *iface, BSTR pbstrName)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pbstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_ShortPath(IFile *iface, BSTR *pbstrPath)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pbstrPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_ShortName(IFile *iface, BSTR *pbstrName)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_Drive(IFile *iface, IDrive **ppdrive)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, ppdrive);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_ParentFolder(IFile *iface, IFolder **ppfolder)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, ppfolder);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_Attributes(IFile *iface, FileAttribute *pfa)
{
    struct file *This = impl_from_IFile(iface);
    DWORD fa;

    TRACE("(%p)->(%p)\n", This, pfa);

    if(!pfa)
        return E_POINTER;

    fa = GetFileAttributesW(This->path);
    if(fa == INVALID_FILE_ATTRIBUTES)
        return create_error(GetLastError());

    *pfa = fa & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |
            FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE |
            FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_COMPRESSED);
    return S_OK;
}

static HRESULT WINAPI file_put_Attributes(IFile *iface, FileAttribute pfa)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%x)\n", This, pfa);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_DateCreated(IFile *iface, DATE *pdate)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pdate);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_DateLastModified(IFile *iface, DATE *pdate)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pdate);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_DateLastAccessed(IFile *iface, DATE *pdate)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pdate);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_get_Size(IFile *iface, VARIANT *pvarSize)
{
    struct file *This = impl_from_IFile(iface);
    WIN32_FIND_DATAW fd;
    HANDLE f;

    TRACE("(%p)->(%p)\n", This, pvarSize);

    if(!pvarSize)
        return E_POINTER;

    f = FindFirstFileW(This->path, &fd);
    if(f == INVALID_HANDLE_VALUE)
        return create_error(GetLastError());
    FindClose(f);

    if(fd.nFileSizeHigh || fd.nFileSizeLow>INT_MAX) {
        V_VT(pvarSize) = VT_R8;
        V_R8(pvarSize) = ((ULONGLONG)fd.nFileSizeHigh<<32) + fd.nFileSizeLow;
    }else {
        V_VT(pvarSize) = VT_I4;
        V_I4(pvarSize) = fd.nFileSizeLow;
    }
    return S_OK;
}

static HRESULT WINAPI file_get_Type(IFile *iface, BSTR *pbstrType)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%p)\n", This, pbstrType);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_Delete(IFile *iface, VARIANT_BOOL Force)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%x)\n", This, Force);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_Copy(IFile *iface, BSTR Destination, VARIANT_BOOL OverWriteFiles)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(Destination), OverWriteFiles);
    return E_NOTIMPL;
}

static HRESULT WINAPI file_Move(IFile *iface, BSTR Destination)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(Destination));
    return E_NOTIMPL;
}

static HRESULT WINAPI file_OpenAsTextStream(IFile *iface, IOMode IOMode, Tristate Format, ITextStream **ppts)
{
    struct file *This = impl_from_IFile(iface);
    FIXME("(%p)->(%x %x %p)\n", This, IOMode, Format, ppts);
    return E_NOTIMPL;
}

static const IFileVtbl file_vtbl = {
    file_QueryInterface,
    file_AddRef,
    file_Release,
    file_GetTypeInfoCount,
    file_GetTypeInfo,
    file_GetIDsOfNames,
    file_Invoke,
    file_get_Path,
    file_get_Name,
    file_put_Name,
    file_get_ShortPath,
    file_get_ShortName,
    file_get_Drive,
    file_get_ParentFolder,
    file_get_Attributes,
    file_put_Attributes,
    file_get_DateCreated,
    file_get_DateLastModified,
    file_get_DateLastAccessed,
    file_get_Size,
    file_get_Type,
    file_Delete,
    file_Copy,
    file_Move,
    file_OpenAsTextStream
};

static HRESULT create_file(BSTR path, IFile **file)
{
    struct file *f;
    DWORD len, attrs;

    *file = NULL;

    f = heap_alloc(sizeof(struct file));
    if(!f)
        return E_OUTOFMEMORY;

    f->IFile_iface.lpVtbl = &file_vtbl;
    f->ref = 1;

    len = GetFullPathNameW(path, 0, NULL, NULL);
    if(!len) {
        heap_free(f);
        return E_FAIL;
    }

    f->path = heap_alloc(len*sizeof(WCHAR));
    if(!f->path) {
        heap_free(f);
        return E_OUTOFMEMORY;
    }

    if(!GetFullPathNameW(path, len, f->path, NULL)) {
        heap_free(f->path);
        heap_free(f);
        return E_FAIL;
    }

    if(path[len-1]=='/' || path[len-1]=='\\')
        path[len-1] = 0;

    attrs = GetFileAttributesW(f->path);
    if(attrs==INVALID_FILE_ATTRIBUTES ||
            (attrs&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE))) {
        heap_free(f->path);
        heap_free(f);
        return create_error(GetLastError());
    }

    *file = &f->IFile_iface;
    return S_OK;
}

static HRESULT WINAPI filesys_QueryInterface(IFileSystem3 *iface, REFIID riid, void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IFileSystem3 ) ||
         IsEqualGUID( riid, &IID_IFileSystem ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IDispatchEx ))
    {
        TRACE("Interface IDispatchEx not supported - returning NULL\n");
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    else if ( IsEqualGUID( riid, &IID_IObjectWithSite ))
    {
        TRACE("Interface IObjectWithSite not supported - returning NULL\n");
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IFileSystem3_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI filesys_AddRef(IFileSystem3 *iface)
{
    TRACE("%p\n", iface);

    return 2;
}

static ULONG WINAPI filesys_Release(IFileSystem3 *iface)
{
    TRACE("%p\n", iface);

    return 1;
}

static HRESULT WINAPI filesys_GetTypeInfoCount(IFileSystem3 *iface, UINT *pctinfo)
{
    TRACE("(%p)->(%p)\n", iface, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI filesys_GetTypeInfo(IFileSystem3 *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("(%p)->(%u %u %p)\n", iface, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IFileSystem3_tid, ppTInfo);
}

static HRESULT WINAPI filesys_GetIDsOfNames(IFileSystem3 *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", iface, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IFileSystem3_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI filesys_Invoke(IFileSystem3 *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", iface, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IFileSystem3_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI filesys_get_Drives(IFileSystem3 *iface, IDriveCollection **ppdrives)
{
    FIXME("%p %p\n", iface, ppdrives);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_BuildPath(IFileSystem3 *iface, BSTR Path,
                                            BSTR Name, BSTR *pbstrResult)
{
    FIXME("%p %s %s %p\n", iface, debugstr_w(Path), debugstr_w(Name), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetDriveName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static inline DWORD get_parent_folder_name(const WCHAR *path, DWORD len)
{
    int i;

    if(!path)
        return 0;

    for(i=len-1; i>=0; i--)
        if(path[i]!='/' && path[i]!='\\')
            break;

    for(; i>=0; i--)
        if(path[i]=='/' || path[i]=='\\')
            break;

    for(; i>=0; i--)
        if(path[i]!='/' && path[i]!='\\')
            break;

    if(i < 0)
        return 0;

    if(path[i]==':' && i==1)
        i++;
    return i+1;
}

static HRESULT WINAPI filesys_GetParentFolderName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    DWORD len;

    TRACE("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    if(!pbstrResult)
        return E_POINTER;

    len = get_parent_folder_name(Path, SysStringLen(Path));
    if(!len) {
        *pbstrResult = NULL;
        return S_OK;
    }

    *pbstrResult = SysAllocStringLen(Path, len);
    if(!*pbstrResult)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI filesys_GetFileName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    int i, end;

    TRACE("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    if(!pbstrResult)
        return E_POINTER;

    if(!Path) {
        *pbstrResult = NULL;
        return S_OK;
    }

    for(end=strlenW(Path)-1; end>=0; end--)
        if(Path[end]!='/' && Path[end]!='\\')
            break;

    for(i=end; i>=0; i--)
        if(Path[i]=='/' || Path[i]=='\\')
            break;
    i++;

    if(i>end || (i==0 && end==1 && Path[1]==':')) {
        *pbstrResult = NULL;
        return S_OK;
    }

    *pbstrResult = SysAllocStringLen(Path+i, end-i+1);
    if(!*pbstrResult)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI filesys_GetBaseName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    int i, end;

    TRACE("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    if(!pbstrResult)
        return E_POINTER;

    if(!Path) {
        *pbstrResult = NULL;
        return S_OK;
    }

    for(end=strlenW(Path)-1; end>=0; end--)
        if(Path[end]!='/' && Path[end]!='\\')
            break;

    for(i=end; i>=0; i--) {
        if(Path[i]=='.' && Path[end+1]!='.')
            end = i-1;
        if(Path[i]=='/' || Path[i]=='\\')
            break;
    }
    i++;

    if((i>end && Path[end+1]!='.') || (i==0 && end==1 && Path[1]==':')) {
        *pbstrResult = NULL;
        return S_OK;
    }

    *pbstrResult = SysAllocStringLen(Path+i, end-i+1);
    if(!*pbstrResult)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI filesys_GetExtensionName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetAbsolutePathName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    static const WCHAR cur_path[] = {'.',0};

    WCHAR buf[MAX_PATH], ch;
    const WCHAR *path;
    DWORD i, beg, len, exp_len;
    WIN32_FIND_DATAW fdata;
    HANDLE fh;

    TRACE("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    if(!pbstrResult)
        return E_POINTER;

    if(!Path)
        path = cur_path;
    else
        path = Path;

    len = GetFullPathNameW(path, MAX_PATH, buf, NULL);
    if(!len)
        return E_FAIL;

    buf[0] = toupperW(buf[0]);
    if(len>3 && buf[len-1] == '\\')
        buf[--len] = 0;

    for(beg=3, i=3; i<=len; i++) {
        if(buf[i]!='\\' && buf[i])
            continue;

        ch = buf[i];
        buf[i] = 0;
        fh = FindFirstFileW(buf, &fdata);
        if(fh == INVALID_HANDLE_VALUE)
            break;

        exp_len = strlenW(fdata.cFileName);
        if(exp_len == i-beg)
            memcpy(buf+beg, fdata.cFileName, exp_len*sizeof(WCHAR));
        FindClose(fh);
        buf[i] = ch;
        beg = i+1;
    }

    *pbstrResult = SysAllocString(buf);
    if(!*pbstrResult)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI filesys_GetTempName(IFileSystem3 *iface, BSTR *pbstrResult)
{
    static const WCHAR fmt[] = {'r','a','d','%','0','5','X','.','t','x','t',0};

    DWORD random;

    TRACE("%p %p\n", iface, pbstrResult);

    if(!pbstrResult)
        return E_POINTER;

    *pbstrResult = SysAllocStringLen(NULL, 12);
    if(!*pbstrResult)
        return E_OUTOFMEMORY;

    if(!RtlGenRandom(&random, sizeof(random)))
        return E_FAIL;
    sprintfW(*pbstrResult, fmt, random & 0xfffff);
    return S_OK;
}

static HRESULT WINAPI filesys_DriveExists(IFileSystem3 *iface, BSTR DriveSpec,
                                            VARIANT_BOOL *pfExists)
{
    FIXME("%p %s %p\n", iface, debugstr_w(DriveSpec), pfExists);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_FileExists(IFileSystem3 *iface, BSTR path, VARIANT_BOOL *ret)
{
    DWORD attrs;
    TRACE("%p %s %p\n", iface, debugstr_w(path), ret);

    if (!ret) return E_POINTER;

    attrs = GetFileAttributesW(path);
    *ret = attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI filesys_FolderExists(IFileSystem3 *iface, BSTR path, VARIANT_BOOL *ret)
{
    DWORD attrs;
    TRACE("%p %s %p\n", iface, debugstr_w(path), ret);

    if (!ret) return E_POINTER;

    attrs = GetFileAttributesW(path);
    *ret = attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI filesys_GetDrive(IFileSystem3 *iface, BSTR DriveSpec,
                                            IDrive **ppdrive)
{
    FIXME("%p %s %p\n", iface, debugstr_w(DriveSpec), ppdrive);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetFile(IFileSystem3 *iface, BSTR FilePath,
                                            IFile **ppfile)
{
    TRACE("%p %s %p\n", iface, debugstr_w(FilePath), ppfile);

    if(!ppfile)
        return E_POINTER;
    if(!FilePath)
        return E_INVALIDARG;

    return create_file(FilePath, ppfile);
}

static HRESULT WINAPI filesys_GetFolder(IFileSystem3 *iface, BSTR FolderPath,
                                            IFolder **ppfolder)
{
    FIXME("%p %s %p\n", iface, debugstr_w(FolderPath), ppfolder);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetSpecialFolder(IFileSystem3 *iface,
                                            SpecialFolderConst SpecialFolder,
                                            IFolder **ppfolder)
{
    FIXME("%p %d %p\n", iface, SpecialFolder, ppfolder);

    return E_NOTIMPL;
}

static inline HRESULT delete_file(const WCHAR *file, DWORD file_len, VARIANT_BOOL force)
{
    WCHAR path[MAX_PATH];
    DWORD len, name_len;
    WIN32_FIND_DATAW ffd;
    HANDLE f;

    f = FindFirstFileW(file, &ffd);
    if(f == INVALID_HANDLE_VALUE)
        return create_error(GetLastError());

    len = get_parent_folder_name(file, file_len);
    if(len+1 >= MAX_PATH) {
        FindClose(f);
        return E_FAIL;
    }
    if(len) {
        memcpy(path, file, len*sizeof(WCHAR));
        path[len++] = '\\';
    }

    do {
        if(ffd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE))
            continue;

        name_len = strlenW(ffd.cFileName);
        if(len+name_len+1 >= MAX_PATH) {
            FindClose(f);
            return E_FAIL;
        }
        memcpy(path+len, ffd.cFileName, (name_len+1)*sizeof(WCHAR));

        TRACE("deleting %s\n", debugstr_w(path));

        if(!DeleteFileW(path)) {
            if(!force || !SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL)
                    || !DeleteFileW(path)) {
                FindClose(f);
                return create_error(GetLastError());
            }
        }
    } while(FindNextFileW(f, &ffd));
    FindClose(f);

    return S_OK;
}

static HRESULT WINAPI filesys_DeleteFile(IFileSystem3 *iface, BSTR FileSpec,
                                            VARIANT_BOOL Force)
{
    TRACE("%p %s %d\n", iface, debugstr_w(FileSpec), Force);

    if(!FileSpec)
        return E_POINTER;

    return delete_file(FileSpec, SysStringLen(FileSpec), Force);
}

static HRESULT delete_folder(const WCHAR *folder, DWORD folder_len, VARIANT_BOOL force)
{
    WCHAR path[MAX_PATH];
    DWORD len, name_len;
    WIN32_FIND_DATAW ffd;
    HANDLE f;
    HRESULT hr;

    f = FindFirstFileW(folder, &ffd);
    if(f == INVALID_HANDLE_VALUE)
        return create_error(GetLastError());

    len = get_parent_folder_name(folder, folder_len);
    if(len+1 >= MAX_PATH) {
        FindClose(f);
        return E_FAIL;
    }
    if(len) {
        memcpy(path, folder, len*sizeof(WCHAR));
        path[len++] = '\\';
    }

    do {
        if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if(ffd.cFileName[0]=='.' && (ffd.cFileName[1]==0 ||
                    (ffd.cFileName[1]=='.' && ffd.cFileName[2]==0)))
            continue;

        name_len = strlenW(ffd.cFileName);
        if(len+name_len+3 >= MAX_PATH) {
            FindClose(f);
            return E_FAIL;
        }
        memcpy(path+len, ffd.cFileName, name_len*sizeof(WCHAR));
        path[len+name_len] = '\\';
        path[len+name_len+1] = '*';
        path[len+name_len+2] = 0;

        hr = delete_file(path, len+name_len+2, force);
        if(FAILED(hr)) {
            FindClose(f);
            return hr;
        }

        hr = delete_folder(path, len+name_len+2, force);
        if(FAILED(hr)) {
            FindClose(f);
            return hr;
        }

        path[len+name_len] = 0;
        TRACE("deleting %s\n", debugstr_w(path));

        if(!RemoveDirectoryW(path)) {
            FindClose(f);
            return create_error(GetLastError());
        }
    } while(FindNextFileW(f, &ffd));
    FindClose(f);

    return S_OK;
}

static HRESULT WINAPI filesys_DeleteFolder(IFileSystem3 *iface, BSTR FolderSpec,
                                            VARIANT_BOOL Force)
{
    TRACE("%p %s %d\n", iface, debugstr_w(FolderSpec), Force);

    if(!FolderSpec)
        return E_POINTER;

    return delete_folder(FolderSpec, SysStringLen(FolderSpec), Force);
}

static HRESULT WINAPI filesys_MoveFile(IFileSystem3 *iface, BSTR Source,
                                            BSTR Destination)
{
    FIXME("%p %s %s\n", iface, debugstr_w(Source), debugstr_w(Destination));

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_MoveFolder(IFileSystem3 *iface,BSTR Source,
                                            BSTR Destination)
{
    FIXME("%p %s %s\n", iface, debugstr_w(Source), debugstr_w(Destination));

    return E_NOTIMPL;
}

static inline HRESULT copy_file(const WCHAR *source, DWORD source_len,
        const WCHAR *destination, DWORD destination_len, VARIANT_BOOL overwrite)
{
    DWORD attrs;
    WCHAR src_path[MAX_PATH], dst_path[MAX_PATH];
    DWORD src_len, dst_len, name_len;
    WIN32_FIND_DATAW ffd;
    HANDLE f;
    HRESULT hr;

    if(!source[0] || !destination[0])
        return E_INVALIDARG;

    attrs = GetFileAttributesW(destination);
    if(attrs==INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        attrs = GetFileAttributesW(source);
        if(attrs == INVALID_FILE_ATTRIBUTES)
            return create_error(GetLastError());
        else if(attrs & FILE_ATTRIBUTE_DIRECTORY)
            return CTL_E_FILENOTFOUND;

        if(!CopyFileW(source, destination, !overwrite))
            return create_error(GetLastError());
        return S_OK;
    }

    f = FindFirstFileW(source, &ffd);
    if(f == INVALID_HANDLE_VALUE)
        return CTL_E_FILENOTFOUND;

    src_len = get_parent_folder_name(source, source_len);
    if(src_len+1 >= MAX_PATH)
        return E_FAIL;
    if(src_len) {
        memcpy(src_path, source, src_len*sizeof(WCHAR));
        src_path[src_len++] = '\\';
    }

    dst_len = destination_len;
    if(dst_len+1 >= MAX_PATH) {
        FindClose(f);
        return E_FAIL;
    }
    memcpy(dst_path, destination, dst_len*sizeof(WCHAR));
    if(dst_path[dst_len-1]!= '\\' && dst_path[dst_len-1]!='/')
        dst_path[dst_len++] = '\\';

    hr = CTL_E_FILENOTFOUND;
    do {
        if(ffd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE))
            continue;

        name_len = strlenW(ffd.cFileName);
        if(src_len+name_len+1>=MAX_PATH || dst_len+name_len+1>=MAX_PATH) {
            FindClose(f);
            return E_FAIL;
        }
        memcpy(src_path+src_len, ffd.cFileName, (name_len+1)*sizeof(WCHAR));
        memcpy(dst_path+dst_len, ffd.cFileName, (name_len+1)*sizeof(WCHAR));

        TRACE("copying %s to %s\n", debugstr_w(src_path), debugstr_w(dst_path));

        if(!CopyFileW(src_path, dst_path, !overwrite)) {
            FindClose(f);
            return create_error(GetLastError());
        }else {
            hr = S_OK;
        }
    } while(FindNextFileW(f, &ffd));
    FindClose(f);

    return hr;
}

static HRESULT WINAPI filesys_CopyFile(IFileSystem3 *iface, BSTR Source,
                                            BSTR Destination, VARIANT_BOOL OverWriteFiles)
{
    TRACE("%p %s %s %d\n", iface, debugstr_w(Source), debugstr_w(Destination), OverWriteFiles);

    if(!Source || !Destination)
        return E_POINTER;

    return copy_file(Source, SysStringLen(Source), Destination,
            SysStringLen(Destination), OverWriteFiles);
}

static HRESULT copy_folder(const WCHAR *source, DWORD source_len, const WCHAR *destination,
        DWORD destination_len, VARIANT_BOOL overwrite)
{
    DWORD tmp, src_len, dst_len, name_len;
    WCHAR src[MAX_PATH], dst[MAX_PATH];
    WIN32_FIND_DATAW ffd;
    HANDLE f;
    HRESULT hr;
    BOOL copied = FALSE;

    if(!source[0] || !destination[0])
        return E_INVALIDARG;

    dst_len = destination_len;
    if(dst_len+1 >= MAX_PATH)
        return E_FAIL;
    memcpy(dst, destination, (dst_len+1)*sizeof(WCHAR));

    if(dst[dst_len-1]!='\\' && dst[dst_len-1]!='/' &&
            (tmp = GetFileAttributesW(source))!=INVALID_FILE_ATTRIBUTES &&
            tmp&FILE_ATTRIBUTE_DIRECTORY) {
        if(!CreateDirectoryW(dst, NULL)) {
            if(overwrite && GetLastError()==ERROR_ALREADY_EXISTS) {
                tmp = GetFileAttributesW(dst);
                if(tmp==INVALID_FILE_ATTRIBUTES || !(tmp&FILE_ATTRIBUTE_DIRECTORY))
                    return CTL_E_FILEALREADYEXISTS;
            }else {
                return create_error(GetLastError());
            }
        }
        copied = TRUE;

        src_len = source_len;
        if(src_len+2 >= MAX_PATH)
            return E_FAIL;
        memcpy(src, source, src_len*sizeof(WCHAR));
        src[src_len++] = '\\';
        src[src_len] = '*';
        src[src_len+1] = 0;

        hr = copy_file(src, src_len+1, dst, dst_len, overwrite);
        if(FAILED(hr) && hr!=CTL_E_FILENOTFOUND)
            return create_error(GetLastError());

        f = FindFirstFileW(src, &ffd);
    }else {
        src_len = get_parent_folder_name(source, source_len);
        if(src_len+2 >= MAX_PATH)
            return E_FAIL;
        memcpy(src, source, src_len*sizeof(WCHAR));
        if(src_len)
            src[src_len++] = '\\';

        f = FindFirstFileW(source, &ffd);
    }
    if(f == INVALID_HANDLE_VALUE)
        return CTL_E_PATHNOTFOUND;

    dst[dst_len++] = '\\';
    dst[dst_len] = 0;

    do {
        if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if(ffd.cFileName[0]=='.' && (ffd.cFileName[1]==0 ||
                    (ffd.cFileName[1]=='.' && ffd.cFileName[2]==0)))
            continue;

        name_len = strlenW(ffd.cFileName);
        if(dst_len+name_len>=MAX_PATH || src_len+name_len+2>=MAX_PATH) {
            FindClose(f);
            return E_FAIL;
        }
        memcpy(dst+dst_len, ffd.cFileName, name_len*sizeof(WCHAR));
        dst[dst_len+name_len] = 0;
        memcpy(src+src_len, ffd.cFileName, name_len*sizeof(WCHAR));
        src[src_len+name_len] = '\\';
        src[src_len+name_len+1] = '*';
        src[src_len+name_len+2] = 0;

        TRACE("copying %s to %s\n", debugstr_w(src), debugstr_w(dst));

        if(!CreateDirectoryW(dst, NULL)) {
            if(overwrite && GetLastError()==ERROR_ALREADY_EXISTS) {
                tmp = GetFileAttributesW(dst);
                if(tmp==INVALID_FILE_ATTRIBUTES || !(tmp&FILE_ATTRIBUTE_DIRECTORY)) {
                    FindClose(f);
                    return CTL_E_FILEALREADYEXISTS;
                }
            }else {
                FindClose(f);
                return create_error(GetLastError());
            }
            return create_error(GetLastError());
        }
        copied = TRUE;

        hr = copy_file(src, src_len+name_len+2, dst, dst_len+name_len, overwrite);
        if(FAILED(hr) && hr!=CTL_E_FILENOTFOUND) {
            FindClose(f);
            return hr;
        }

        hr = copy_folder(src, src_len+name_len+2, dst, dst_len+name_len, overwrite);
        if(FAILED(hr) && hr!=CTL_E_PATHNOTFOUND) {
            FindClose(f);
            return hr;
        }
    } while(FindNextFileW(f, &ffd));
    FindClose(f);

    return copied ? S_OK : CTL_E_PATHNOTFOUND;
}

static HRESULT WINAPI filesys_CopyFolder(IFileSystem3 *iface, BSTR Source,
                                            BSTR Destination, VARIANT_BOOL OverWriteFiles)
{
    TRACE("%p %s %s %d\n", iface, debugstr_w(Source), debugstr_w(Destination), OverWriteFiles);

    if(!Source || !Destination)
        return E_POINTER;

    return copy_folder(Source, SysStringLen(Source), Destination,
            SysStringLen(Destination), OverWriteFiles);
}

static HRESULT WINAPI filesys_CreateFolder(IFileSystem3 *iface, BSTR path,
                                            IFolder **folder)
{
    BOOL ret;

    TRACE("(%p)->(%s %p)\n", iface, debugstr_w(path), folder);

    ret = CreateDirectoryW(path, NULL);
    if (!ret)
    {
        *folder = NULL;
        if (GetLastError() == ERROR_ALREADY_EXISTS) return CTL_E_FILEALREADYEXISTS;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return create_folder(folder);
}

static HRESULT WINAPI filesys_CreateTextFile(IFileSystem3 *iface, BSTR FileName,
                                            VARIANT_BOOL Overwrite, VARIANT_BOOL Unicode,
                                            ITextStream **ppts)
{
    FIXME("%p %s %d %d %p\n", iface, debugstr_w(FileName), Overwrite, Unicode, ppts);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_OpenTextFile(IFileSystem3 *iface, BSTR filename,
                                            IOMode mode, VARIANT_BOOL create,
                                            Tristate format, ITextStream **stream)
{
    FIXME("(%p)->(%s %d %d %d %p)\n", iface, debugstr_w(filename), mode, create, format, stream);
    return create_textstream(mode, stream);
}

static HRESULT WINAPI filesys_GetStandardStream(IFileSystem3 *iface,
                                            StandardStreamTypes StandardStreamType,
                                            VARIANT_BOOL Unicode,
                                            ITextStream **ppts)
{
    FIXME("%p %d %d %p\n", iface, StandardStreamType, Unicode, ppts);

    return E_NOTIMPL;
}

static void get_versionstring(VS_FIXEDFILEINFO *info, WCHAR *ver)
{
    static WCHAR fmtW[] = {'%','d','.','%','d','.','%','d','.','%','d',0};
    DWORDLONG version;
    WORD a, b, c, d;

    version = (((DWORDLONG)info->dwFileVersionMS) << 32) + info->dwFileVersionLS;
    a = (WORD)( version >> 48);
    b = (WORD)((version >> 32) & 0xffff);
    c = (WORD)((version >> 16) & 0xffff);
    d = (WORD)( version & 0xffff);

    sprintfW(ver, fmtW, a, b, c, d);
}

static HRESULT WINAPI filesys_GetFileVersion(IFileSystem3 *iface, BSTR name, BSTR *version)
{
    static const WCHAR rootW[] = {'\\',0};
    VS_FIXEDFILEINFO *info;
    WCHAR ver[30];
    void *ptr;
    DWORD len;
    BOOL ret;

    TRACE("%p %s %p\n", iface, debugstr_w(name), version);

    len = GetFileVersionInfoSizeW(name, NULL);
    if (!len)
        return HRESULT_FROM_WIN32(GetLastError());

    ptr = heap_alloc(len);
    if (!GetFileVersionInfoW(name, 0, len, ptr))
    {
        heap_free(ptr);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ret = VerQueryValueW(ptr, rootW, (void**)&info, &len);
    heap_free(ptr);
    if (!ret)
        return HRESULT_FROM_WIN32(GetLastError());

    get_versionstring(info, ver);
    *version = SysAllocString(ver);
    TRACE("version=%s\n", debugstr_w(ver));

    return S_OK;
}

static const struct IFileSystem3Vtbl filesys_vtbl =
{
    filesys_QueryInterface,
    filesys_AddRef,
    filesys_Release,
    filesys_GetTypeInfoCount,
    filesys_GetTypeInfo,
    filesys_GetIDsOfNames,
    filesys_Invoke,
    filesys_get_Drives,
    filesys_BuildPath,
    filesys_GetDriveName,
    filesys_GetParentFolderName,
    filesys_GetFileName,
    filesys_GetBaseName,
    filesys_GetExtensionName,
    filesys_GetAbsolutePathName,
    filesys_GetTempName,
    filesys_DriveExists,
    filesys_FileExists,
    filesys_FolderExists,
    filesys_GetDrive,
    filesys_GetFile,
    filesys_GetFolder,
    filesys_GetSpecialFolder,
    filesys_DeleteFile,
    filesys_DeleteFolder,
    filesys_MoveFile,
    filesys_MoveFolder,
    filesys_CopyFile,
    filesys_CopyFolder,
    filesys_CreateFolder,
    filesys_CreateTextFile,
    filesys_OpenTextFile,
    filesys_GetStandardStream,
    filesys_GetFileVersion
};

static IFileSystem3 filesystem = { &filesys_vtbl };

HRESULT WINAPI FileSystem_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return IFileSystem3_QueryInterface(&filesystem, riid, ppv);
}
