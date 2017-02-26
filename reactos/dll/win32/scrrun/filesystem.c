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

#include "scrrun_private.h"

#include <winver.h>
#include <olectl.h>
#include <ntsecapi.h>
#include <wine/unicode.h>

static const WCHAR bsW[] = {'\\',0};
static const WCHAR utf16bom = 0xfeff;

struct filesystem {
    struct provideclassinfo classinfo;
    IFileSystem3 IFileSystem3_iface;
};

struct foldercollection {
    struct provideclassinfo classinfo;
    IFolderCollection IFolderCollection_iface;
    LONG ref;
    BSTR path;
};

struct filecollection {
    struct provideclassinfo classinfo;
    IFileCollection IFileCollection_iface;
    LONG ref;
    BSTR path;
};

struct drivecollection {
    struct provideclassinfo classinfo;
    IDriveCollection IDriveCollection_iface;
    LONG ref;
    DWORD drives;
    LONG count;
};

struct enumdata {
    union
    {
        struct
        {
            struct foldercollection *coll;
            HANDLE find;
        } foldercoll;
        struct
        {
            struct filecollection *coll;
            HANDLE find;
        } filecoll;
        struct
        {
            struct drivecollection *coll;
            INT cur;
        } drivecoll;
    } u;
};

struct enumvariant {
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;

    struct enumdata data;
};

struct drive {
    struct provideclassinfo classinfo;
    IDrive IDrive_iface;
    LONG ref;
    BSTR root;
};

struct folder {
    struct provideclassinfo classinfo;
    IFolder IFolder_iface;
    LONG ref;
    BSTR path;
};

struct file {
    struct provideclassinfo classinfo;
    IFile IFile_iface;
    LONG ref;

    WCHAR *path;
};

struct textstream {
    struct provideclassinfo classinfo;
    ITextStream ITextStream_iface;
    LONG ref;

    IOMode mode;
    BOOL unicode;
    BOOL first_read;
    LARGE_INTEGER size;
    HANDLE file;
};

enum iotype {
    IORead,
    IOWrite
};

static inline struct filesystem *impl_from_IFileSystem3(IFileSystem3 *iface)
{
    return CONTAINING_RECORD(iface, struct filesystem, IFileSystem3_iface);
}

static inline struct drive *impl_from_IDrive(IDrive *iface)
{
    return CONTAINING_RECORD(iface, struct drive, IDrive_iface);
}

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

static inline struct foldercollection *impl_from_IFolderCollection(IFolderCollection *iface)
{
    return CONTAINING_RECORD(iface, struct foldercollection, IFolderCollection_iface);
}

static inline struct filecollection *impl_from_IFileCollection(IFileCollection *iface)
{
    return CONTAINING_RECORD(iface, struct filecollection, IFileCollection_iface);
}

static inline struct drivecollection *impl_from_IDriveCollection(IDriveCollection *iface)
{
    return CONTAINING_RECORD(iface, struct drivecollection, IDriveCollection_iface);
}

static inline struct enumvariant *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, struct enumvariant, IEnumVARIANT_iface);
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

static HRESULT create_folder(const WCHAR*, IFolder**);
static HRESULT create_file(BSTR, IFile**);
static HRESULT create_foldercoll_enum(struct foldercollection*, IUnknown**);
static HRESULT create_filecoll_enum(struct filecollection*, IUnknown**);

static inline BOOL is_dir_data(const WIN32_FIND_DATAW *data)
{
    static const WCHAR dotdotW[] = {'.','.',0};
    static const WCHAR dotW[] = {'.',0};

    return (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmpW(data->cFileName, dotdotW) &&
            strcmpW(data->cFileName, dotW);
}

static inline BOOL is_file_data(const WIN32_FIND_DATAW *data)
{
    return !(data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

static BSTR get_full_path(BSTR path, const WIN32_FIND_DATAW *data)
{
    int len = SysStringLen(path);
    WCHAR buffW[MAX_PATH];

    strcpyW(buffW, path);
    if (path[len-1] != '\\')
        strcatW(buffW, bsW);
    strcatW(buffW, data->cFileName);

    return SysAllocString(buffW);
}

static BOOL textstream_check_iomode(struct textstream *This, enum iotype type)
{
    if (type == IORead)
        return This->mode == ForWriting || This->mode == ForAppending;
    else
        return This->mode == ForReading;
}

static HRESULT WINAPI textstream_QueryInterface(ITextStream *iface, REFIID riid, void **obj)
{
    struct textstream *This = impl_from_ITextStream(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_ITextStream) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &This->ITextStream_iface;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
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
    {
        CloseHandle(This->file);
        heap_free(This);
    }

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
    LARGE_INTEGER pos, dist;

    TRACE("(%p)->(%p)\n", This, eos);

    if (!eos)
        return E_POINTER;

    if (textstream_check_iomode(This, IORead)) {
        *eos = VARIANT_TRUE;
        return CTL_E_BADFILEMODE;
    }

    dist.QuadPart = 0;
    if (!SetFilePointerEx(This->file, dist, &pos, FILE_CURRENT))
        return E_FAIL;

    *eos = This->size.QuadPart == pos.QuadPart ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI textstream_get_AtEndOfLine(ITextStream *iface, VARIANT_BOOL *eol)
{
    struct textstream *This = impl_from_ITextStream(iface);
    FIXME("(%p)->(%p): stub\n", This, eol);
    return E_NOTIMPL;
}

/*
   Reads 'toread' bytes from a file, converts if needed
   BOM is skipped if 'bof' is set.
 */
static HRESULT textstream_read(struct textstream *stream, LONG toread, BOOL bof, BSTR *text)
{
    HRESULT hr = S_OK;
    DWORD read;
    char *buff;
    BOOL ret;

    if (toread == 0) {
        *text = SysAllocStringLen(NULL, 0);
        return *text ? S_FALSE : E_OUTOFMEMORY;
    }

    if (toread < sizeof(WCHAR))
        return CTL_E_ENDOFFILE;

    buff = heap_alloc(toread);
    if (!buff)
        return E_OUTOFMEMORY;

    ret = ReadFile(stream->file, buff, toread, &read, NULL);
    if (!ret || toread != read) {
        WARN("failed to read from file %d, %d, error %d\n", read, toread, GetLastError());
        heap_free(buff);
        return E_FAIL;
    }

    if (stream->unicode) {
        int i = 0;

        /* skip BOM */
        if (bof && *(WCHAR*)buff == utf16bom) {
            read -= sizeof(WCHAR);
            i += sizeof(WCHAR);
        }

        *text = SysAllocStringLen(read ? (WCHAR*)&buff[i] : NULL, read/sizeof(WCHAR));
        if (!*text) hr = E_OUTOFMEMORY;
    }
    else {
        INT len = MultiByteToWideChar(CP_ACP, 0, buff, read, NULL, 0);
        *text = SysAllocStringLen(NULL, len);
        if (*text)
            MultiByteToWideChar(CP_ACP, 0, buff, read, *text, len);
        else
            hr = E_OUTOFMEMORY;
    }
    heap_free(buff);

    return hr;
}

static HRESULT WINAPI textstream_Read(ITextStream *iface, LONG len, BSTR *text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    LARGE_INTEGER start, end, dist;
    DWORD toread;
    HRESULT hr;

    TRACE("(%p)->(%d %p)\n", This, len, text);

    if (!text)
        return E_POINTER;

    *text = NULL;
    if (len <= 0)
        return len == 0 ? S_OK : E_INVALIDARG;

    if (textstream_check_iomode(This, IORead))
        return CTL_E_BADFILEMODE;

    if (!This->first_read) {
        VARIANT_BOOL eos;

        /* check for EOF */
        hr = ITextStream_get_AtEndOfStream(iface, &eos);
        if (FAILED(hr))
            return hr;

        if (eos == VARIANT_TRUE)
            return CTL_E_ENDOFFILE;
    }

    /* read everything from current position */
    dist.QuadPart = 0;
    SetFilePointerEx(This->file, dist, &start, FILE_CURRENT);
    SetFilePointerEx(This->file, dist, &end, FILE_END);
    toread = end.QuadPart - start.QuadPart;
    /* rewind back */
    dist.QuadPart = start.QuadPart;
    SetFilePointerEx(This->file, dist, NULL, FILE_BEGIN);

    This->first_read = FALSE;
    if (This->unicode) len *= sizeof(WCHAR);

    hr = textstream_read(This, min(toread, len), start.QuadPart == 0, text);
    if (FAILED(hr))
        return hr;
    else
        return toread <= len ? S_FALSE : S_OK;
}

static HRESULT WINAPI textstream_ReadLine(ITextStream *iface, BSTR *text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    VARIANT_BOOL eos;
    HRESULT hr;

    FIXME("(%p)->(%p): stub\n", This, text);

    if (!text)
        return E_POINTER;

    *text = NULL;
    if (textstream_check_iomode(This, IORead))
        return CTL_E_BADFILEMODE;

    /* check for EOF */
    hr = ITextStream_get_AtEndOfStream(iface, &eos);
    if (FAILED(hr))
        return hr;

    if (eos == VARIANT_TRUE)
        return CTL_E_ENDOFFILE;

    return E_NOTIMPL;
}

static HRESULT WINAPI textstream_ReadAll(ITextStream *iface, BSTR *text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    LARGE_INTEGER start, end, dist;
    DWORD toread;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, text);

    if (!text)
        return E_POINTER;

    *text = NULL;
    if (textstream_check_iomode(This, IORead))
        return CTL_E_BADFILEMODE;

    if (!This->first_read) {
        VARIANT_BOOL eos;

        /* check for EOF */
        hr = ITextStream_get_AtEndOfStream(iface, &eos);
        if (FAILED(hr))
            return hr;

        if (eos == VARIANT_TRUE)
            return CTL_E_ENDOFFILE;
    }

    /* read everything from current position */
    dist.QuadPart = 0;
    SetFilePointerEx(This->file, dist, &start, FILE_CURRENT);
    SetFilePointerEx(This->file, dist, &end, FILE_END);
    toread = end.QuadPart - start.QuadPart;
    /* rewind back */
    dist.QuadPart = start.QuadPart;
    SetFilePointerEx(This->file, dist, NULL, FILE_BEGIN);

    This->first_read = FALSE;

    hr = textstream_read(This, toread, start.QuadPart == 0, text);
    return FAILED(hr) ? hr : S_FALSE;
}

static HRESULT textstream_writestr(struct textstream *stream, BSTR text)
{
    DWORD written = 0;
    BOOL ret;

    if (stream->unicode) {
        ret = WriteFile(stream->file, text, SysStringByteLen(text), &written, NULL);
        return (ret && written == SysStringByteLen(text)) ? S_OK : create_error(GetLastError());
    } else {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, text, SysStringLen(text), NULL, 0, NULL, NULL);
        char *buffA;
        HRESULT hr;

        buffA = heap_alloc(len);
        if (!buffA)
            return E_OUTOFMEMORY;

        WideCharToMultiByte(CP_ACP, 0, text, SysStringLen(text), buffA, len, NULL, NULL);
        ret = WriteFile(stream->file, buffA, len, &written, NULL);
        hr = (ret && written == len) ? S_OK : create_error(GetLastError());
        heap_free(buffA);
        return hr;
    }
}

static HRESULT WINAPI textstream_Write(ITextStream *iface, BSTR text)
{
    struct textstream *This = impl_from_ITextStream(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(text));

    if (textstream_check_iomode(This, IOWrite))
        return CTL_E_BADFILEMODE;

    return textstream_writestr(This, text);
}

static HRESULT textstream_writecrlf(struct textstream *stream)
{
    static const WCHAR crlfW[] = {'\r','\n'};
    static const char crlfA[] = {'\r','\n'};
    DWORD written = 0, len;
    const void *ptr;
    BOOL ret;

    if (stream->unicode) {
        ptr = crlfW;
        len = sizeof(crlfW);
    }
    else {
        ptr = crlfA;
        len = sizeof(crlfA);
    }

    ret = WriteFile(stream->file, ptr, len, &written, NULL);
    return (ret && written == len) ? S_OK : create_error(GetLastError());
}

static HRESULT WINAPI textstream_WriteLine(ITextStream *iface, BSTR text)
{
    struct textstream *This = impl_from_ITextStream(iface);
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(text));

    if (textstream_check_iomode(This, IOWrite))
        return CTL_E_BADFILEMODE;

    hr = textstream_writestr(This, text);
    if (SUCCEEDED(hr))
        hr = textstream_writecrlf(This);
    return hr;
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
    HRESULT hr = S_OK;

    TRACE("(%p)\n", This);

    if(!CloseHandle(This->file))
        hr = S_FALSE;

    This->file = NULL;

    return hr;
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

static HRESULT create_textstream(const WCHAR *filename, DWORD disposition, IOMode mode, BOOL unicode, ITextStream **ret)
{
    struct textstream *stream;
    DWORD access = 0;

    /* map access mode */
    switch (mode)
    {
    case ForReading:
        access = GENERIC_READ;
        break;
    case ForWriting:
        access = GENERIC_WRITE;
        break;
    case ForAppending:
        access = FILE_APPEND_DATA;
        break;
    default:
        return E_INVALIDARG;
    }

    stream = heap_alloc(sizeof(struct textstream));
    if (!stream) return E_OUTOFMEMORY;

    stream->ITextStream_iface.lpVtbl = &textstreamvtbl;
    stream->ref = 1;
    stream->mode = mode;
    stream->unicode = unicode;
    stream->first_read = TRUE;

    stream->file = CreateFileW(filename, access, 0, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (stream->file == INVALID_HANDLE_VALUE)
    {
        HRESULT hr = create_error(GetLastError());
        heap_free(stream);
        return hr;
    }

    if (mode == ForReading)
        GetFileSizeEx(stream->file, &stream->size);
    else
        stream->size.QuadPart = 0;

    /* Write Unicode BOM */
    if (unicode && mode == ForWriting && (disposition == CREATE_ALWAYS || disposition == CREATE_NEW)) {
        DWORD written = 0;
        BOOL ret = WriteFile(stream->file, &utf16bom, sizeof(utf16bom), &written, NULL);
        if (!ret || written != sizeof(utf16bom)) {
            ITextStream_Release(&stream->ITextStream_iface);
            return create_error(GetLastError());
        }
    }

    init_classinfo(&CLSID_TextStream, (IUnknown *)&stream->ITextStream_iface, &stream->classinfo);
    *ret = &stream->ITextStream_iface;
    return S_OK;
}

static HRESULT WINAPI drive_QueryInterface(IDrive *iface, REFIID riid, void **obj)
{
    struct drive *This = impl_from_IDrive(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IDrive ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown))
    {
        *obj = &This->IDrive_iface;
    }
    else if (IsEqualIID( riid, &IID_IProvideClassInfo ))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI drive_AddRef(IDrive *iface)
{
    struct drive *This = impl_from_IDrive(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI drive_Release(IDrive *iface)
{
    struct drive *This = impl_from_IDrive(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        SysFreeString(This->root);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI drive_GetTypeInfoCount(IDrive *iface, UINT *pctinfo)
{
    struct drive *This = impl_from_IDrive(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI drive_GetTypeInfo(IDrive *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    struct drive *This = impl_from_IDrive(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IDrive_tid, ppTInfo);
}

static HRESULT WINAPI drive_GetIDsOfNames(IDrive *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    struct drive *This = impl_from_IDrive(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IDrive_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI drive_Invoke(IDrive *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct drive *This = impl_from_IDrive(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IDrive_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI drive_get_Path(IDrive *iface, BSTR *path)
{
    struct drive *This = impl_from_IDrive(iface);
    FIXME("(%p)->(%p): stub\n", This, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI drive_get_DriveLetter(IDrive *iface, BSTR *letter)
{
    struct drive *This = impl_from_IDrive(iface);

    TRACE("(%p)->(%p)\n", This, letter);

    if (!letter)
        return E_POINTER;

    *letter = SysAllocStringLen(This->root, 1);
    if (!*letter)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI drive_get_ShareName(IDrive *iface, BSTR *share_name)
{
    struct drive *This = impl_from_IDrive(iface);
    FIXME("(%p)->(%p): stub\n", This, share_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI drive_get_DriveType(IDrive *iface, DriveTypeConst *type)
{
    struct drive *This = impl_from_IDrive(iface);

    TRACE("(%p)->(%p)\n", This, type);

    switch (GetDriveTypeW(This->root))
    {
    case DRIVE_REMOVABLE:
        *type = Removable;
        break;
    case DRIVE_FIXED:
        *type = Fixed;
        break;
    case DRIVE_REMOTE:
        *type = Remote;
        break;
    case DRIVE_CDROM:
        *type = CDRom;
        break;
    case DRIVE_RAMDISK:
        *type = RamDisk;
        break;
    default:
        *type = UnknownType;
        break;
    }

    return S_OK;
}

static HRESULT WINAPI drive_get_RootFolder(IDrive *iface, IFolder **folder)
{
    struct drive *This = impl_from_IDrive(iface);
    FIXME("(%p)->(%p): stub\n", This, folder);
    return E_NOTIMPL;
}

static HRESULT variant_from_largeint(const ULARGE_INTEGER *src, VARIANT *v)
{
    HRESULT hr = S_OK;

    if (src->u.HighPart || src->u.LowPart > INT_MAX)
    {
        V_VT(v) = VT_R8;
        hr = VarR8FromUI8(src->QuadPart, &V_R8(v));
    }
    else
    {
        V_VT(v) = VT_I4;
        V_I4(v) = src->u.LowPart;
    }

    return hr;
}

static HRESULT WINAPI drive_get_AvailableSpace(IDrive *iface, VARIANT *v)
{
    struct drive *This = impl_from_IDrive(iface);
    ULARGE_INTEGER avail;

    TRACE("(%p)->(%p)\n", This, v);

    if (!v)
        return E_POINTER;

    if (!GetDiskFreeSpaceExW(This->root, &avail, NULL, NULL))
        return E_FAIL;

    return variant_from_largeint(&avail, v);
}

static HRESULT WINAPI drive_get_FreeSpace(IDrive *iface, VARIANT *v)
{
    struct drive *This = impl_from_IDrive(iface);
    ULARGE_INTEGER freespace;

    TRACE("(%p)->(%p)\n", This, v);

    if (!v)
        return E_POINTER;

    if (!GetDiskFreeSpaceExW(This->root, &freespace, NULL, NULL))
        return E_FAIL;

    return variant_from_largeint(&freespace, v);
}

static HRESULT WINAPI drive_get_TotalSize(IDrive *iface, VARIANT *v)
{
    struct drive *This = impl_from_IDrive(iface);
    ULARGE_INTEGER total;

    TRACE("(%p)->(%p)\n", This, v);

    if (!v)
        return E_POINTER;

    if (!GetDiskFreeSpaceExW(This->root, NULL, &total, NULL))
        return E_FAIL;

    return variant_from_largeint(&total, v);
}

static HRESULT WINAPI drive_get_VolumeName(IDrive *iface, BSTR *name)
{
    struct drive *This = impl_from_IDrive(iface);
    WCHAR nameW[MAX_PATH+1];
    BOOL ret;

    TRACE("(%p)->(%p)\n", This, name);

    if (!name)
        return E_POINTER;

    *name = NULL;
    ret = GetVolumeInformationW(This->root, nameW, sizeof(nameW)/sizeof(WCHAR), NULL, NULL, NULL, NULL, 0);
    if (ret)
        *name = SysAllocString(nameW);
    return ret ? S_OK : E_FAIL;
}

static HRESULT WINAPI drive_put_VolumeName(IDrive *iface, BSTR name)
{
    struct drive *This = impl_from_IDrive(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI drive_get_FileSystem(IDrive *iface, BSTR *fs)
{
    struct drive *This = impl_from_IDrive(iface);
    WCHAR nameW[MAX_PATH+1];
    BOOL ret;

    TRACE("(%p)->(%p)\n", This, fs);

    if (!fs)
        return E_POINTER;

    *fs = NULL;
    ret = GetVolumeInformationW(This->root, NULL, 0, NULL, NULL, NULL, nameW, sizeof(nameW)/sizeof(WCHAR));
    if (ret)
        *fs = SysAllocString(nameW);
    return ret ? S_OK : E_FAIL;
}

static HRESULT WINAPI drive_get_SerialNumber(IDrive *iface, LONG *serial)
{
    struct drive *This = impl_from_IDrive(iface);
    BOOL ret;

    TRACE("(%p)->(%p)\n", This, serial);

    if (!serial)
        return E_POINTER;

    ret = GetVolumeInformationW(This->root, NULL, 0, (DWORD*)serial, NULL, NULL, NULL, 0);
    return ret ? S_OK : E_FAIL;
}

static HRESULT WINAPI drive_get_IsReady(IDrive *iface, VARIANT_BOOL *ready)
{
    struct drive *This = impl_from_IDrive(iface);
    ULARGE_INTEGER freespace;
    BOOL ret;

    TRACE("(%p)->(%p)\n", This, ready);

    if (!ready)
        return E_POINTER;

    ret = GetDiskFreeSpaceExW(This->root, &freespace, NULL, NULL);
    *ready = ret ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static const IDriveVtbl drivevtbl = {
    drive_QueryInterface,
    drive_AddRef,
    drive_Release,
    drive_GetTypeInfoCount,
    drive_GetTypeInfo,
    drive_GetIDsOfNames,
    drive_Invoke,
    drive_get_Path,
    drive_get_DriveLetter,
    drive_get_ShareName,
    drive_get_DriveType,
    drive_get_RootFolder,
    drive_get_AvailableSpace,
    drive_get_FreeSpace,
    drive_get_TotalSize,
    drive_get_VolumeName,
    drive_put_VolumeName,
    drive_get_FileSystem,
    drive_get_SerialNumber,
    drive_get_IsReady
};

static HRESULT create_drive(WCHAR letter, IDrive **drive)
{
    struct drive *This;

    *drive = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IDrive_iface.lpVtbl = &drivevtbl;
    This->ref = 1;
    This->root = SysAllocStringLen(NULL, 3);
    if (!This->root)
    {
        heap_free(This);
        return E_OUTOFMEMORY;
    }
    This->root[0] = letter;
    This->root[1] = ':';
    This->root[2] = '\\';
    This->root[3] = 0;

    init_classinfo(&CLSID_Drive, (IUnknown *)&This->IDrive_iface, &This->classinfo);
    *drive = &This->IDrive_iface;
    return S_OK;
}

static HRESULT WINAPI enumvariant_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **obj)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IEnumVARIANT ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = iface;
        IEnumVARIANT_AddRef(iface);
    }
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI enumvariant_AddRef(IEnumVARIANT *iface)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI foldercoll_enumvariant_Release(IEnumVARIANT *iface)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IFolderCollection_Release(&This->data.u.foldercoll.coll->IFolderCollection_iface);
        FindClose(This->data.u.foldercoll.find);
        heap_free(This);
    }

    return ref;
}

static HANDLE start_enumeration(const WCHAR *path, WIN32_FIND_DATAW *data, BOOL file)
{
    static const WCHAR allW[] = {'*',0};
    WCHAR pathW[MAX_PATH];
    int len;
    HANDLE handle;

    strcpyW(pathW, path);
    len = strlenW(pathW);
    if (len && pathW[len-1] != '\\')
        strcatW(pathW, bsW);
    strcatW(pathW, allW);
    handle = FindFirstFileW(pathW, data);
    if (handle == INVALID_HANDLE_VALUE) return 0;

    /* find first dir/file */
    while (1)
    {
        if (file ? is_file_data(data) : is_dir_data(data))
            break;

        if (!FindNextFileW(handle, data))
        {
            FindClose(handle);
            return 0;
        }
    }
    return handle;
}

static HRESULT WINAPI foldercoll_enumvariant_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    HANDLE handle = This->data.u.foldercoll.find;
    WIN32_FIND_DATAW data;
    ULONG count = 0;

    TRACE("(%p)->(%d %p %p)\n", This, celt, var, fetched);

    if (fetched)
        *fetched = 0;

    if (!celt) return S_OK;

    if (!handle)
    {
        handle = start_enumeration(This->data.u.foldercoll.coll->path, &data, FALSE);
        if (!handle) return S_FALSE;

        This->data.u.foldercoll.find = handle;
    }
    else
    {
        if (!FindNextFileW(handle, &data))
            return S_FALSE;
    }

    do
    {
        if (is_dir_data(&data))
        {
            IFolder *folder;
            HRESULT hr;
            BSTR str;

            str = get_full_path(This->data.u.foldercoll.coll->path, &data);
            hr = create_folder(str, &folder);
            SysFreeString(str);
            if (FAILED(hr)) return hr;

            V_VT(&var[count]) = VT_DISPATCH;
            V_DISPATCH(&var[count]) = (IDispatch*)folder;
            count++;

            if (count >= celt) break;
        }
    } while (FindNextFileW(handle, &data));

    if (fetched)
        *fetched = count;

    return (count < celt) ? S_FALSE : S_OK;
}

static HRESULT WINAPI foldercoll_enumvariant_Skip(IEnumVARIANT *iface, ULONG celt)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    HANDLE handle = This->data.u.foldercoll.find;
    WIN32_FIND_DATAW data;

    TRACE("(%p)->(%d)\n", This, celt);

    if (!celt) return S_OK;

    if (!handle)
    {
        handle = start_enumeration(This->data.u.foldercoll.coll->path, &data, FALSE);
        if (!handle) return S_FALSE;

        This->data.u.foldercoll.find = handle;
    }
    else
    {
        if (!FindNextFileW(handle, &data))
            return S_FALSE;
    }

    do
    {
        if (is_dir_data(&data))
            --celt;

        if (!celt) break;
    } while (FindNextFileW(handle, &data));

    return celt ? S_FALSE : S_OK;
}

static HRESULT WINAPI foldercoll_enumvariant_Reset(IEnumVARIANT *iface)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)\n", This);

    FindClose(This->data.u.foldercoll.find);
    This->data.u.foldercoll.find = NULL;

    return S_OK;
}

static HRESULT WINAPI foldercoll_enumvariant_Clone(IEnumVARIANT *iface, IEnumVARIANT **pclone)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    TRACE("(%p)->(%p)\n", This, pclone);
    return create_foldercoll_enum(This->data.u.foldercoll.coll, (IUnknown**)pclone);
}

static const IEnumVARIANTVtbl foldercollenumvariantvtbl = {
    enumvariant_QueryInterface,
    enumvariant_AddRef,
    foldercoll_enumvariant_Release,
    foldercoll_enumvariant_Next,
    foldercoll_enumvariant_Skip,
    foldercoll_enumvariant_Reset,
    foldercoll_enumvariant_Clone
};

static HRESULT create_foldercoll_enum(struct foldercollection *collection, IUnknown **newenum)
{
    struct enumvariant *This;

    *newenum = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IEnumVARIANT_iface.lpVtbl = &foldercollenumvariantvtbl;
    This->ref = 1;
    This->data.u.foldercoll.find = NULL;
    This->data.u.foldercoll.coll = collection;
    IFolderCollection_AddRef(&collection->IFolderCollection_iface);

    *newenum = (IUnknown*)&This->IEnumVARIANT_iface;

    return S_OK;
}

static ULONG WINAPI filecoll_enumvariant_Release(IEnumVARIANT *iface)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IFileCollection_Release(&This->data.u.filecoll.coll->IFileCollection_iface);
        FindClose(This->data.u.filecoll.find);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI filecoll_enumvariant_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    HANDLE handle = This->data.u.filecoll.find;
    WIN32_FIND_DATAW data;
    ULONG count = 0;

    TRACE("(%p)->(%d %p %p)\n", This, celt, var, fetched);

    if (fetched)
        *fetched = 0;

    if (!celt) return S_OK;

    if (!handle)
    {
        handle = start_enumeration(This->data.u.filecoll.coll->path, &data, TRUE);
        if (!handle) return S_FALSE;
        This->data.u.filecoll.find = handle;
    }
    else if (!FindNextFileW(handle, &data))
        return S_FALSE;

    do
    {
        if (is_file_data(&data))
        {
            IFile *file;
            HRESULT hr;
            BSTR str;

            str = get_full_path(This->data.u.filecoll.coll->path, &data);
            hr = create_file(str, &file);
            SysFreeString(str);
            if (FAILED(hr)) return hr;

            V_VT(&var[count]) = VT_DISPATCH;
            V_DISPATCH(&var[count]) = (IDispatch*)file;
            if (++count >= celt) break;
        }
    } while (FindNextFileW(handle, &data));

    if (fetched)
        *fetched = count;

    return (count < celt) ? S_FALSE : S_OK;
}

static HRESULT WINAPI filecoll_enumvariant_Skip(IEnumVARIANT *iface, ULONG celt)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    HANDLE handle = This->data.u.filecoll.find;
    WIN32_FIND_DATAW data;

    TRACE("(%p)->(%d)\n", This, celt);

    if (!celt) return S_OK;

    if (!handle)
    {
        handle = start_enumeration(This->data.u.filecoll.coll->path, &data, TRUE);
        if (!handle) return S_FALSE;
        This->data.u.filecoll.find = handle;
    }
    else if (!FindNextFileW(handle, &data))
        return S_FALSE;

    do
    {
        if (is_file_data(&data))
            --celt;
    } while (celt && FindNextFileW(handle, &data));

    return celt ? S_FALSE : S_OK;
}

static HRESULT WINAPI filecoll_enumvariant_Reset(IEnumVARIANT *iface)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)\n", This);

    FindClose(This->data.u.filecoll.find);
    This->data.u.filecoll.find = NULL;

    return S_OK;
}

static HRESULT WINAPI filecoll_enumvariant_Clone(IEnumVARIANT *iface, IEnumVARIANT **pclone)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    TRACE("(%p)->(%p)\n", This, pclone);
    return create_filecoll_enum(This->data.u.filecoll.coll, (IUnknown**)pclone);
}

static const IEnumVARIANTVtbl filecollenumvariantvtbl = {
    enumvariant_QueryInterface,
    enumvariant_AddRef,
    filecoll_enumvariant_Release,
    filecoll_enumvariant_Next,
    filecoll_enumvariant_Skip,
    filecoll_enumvariant_Reset,
    filecoll_enumvariant_Clone
};

static HRESULT create_filecoll_enum(struct filecollection *collection, IUnknown **newenum)
{
    struct enumvariant *This;

    *newenum = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IEnumVARIANT_iface.lpVtbl = &filecollenumvariantvtbl;
    This->ref = 1;
    This->data.u.filecoll.find = NULL;
    This->data.u.filecoll.coll = collection;
    IFileCollection_AddRef(&collection->IFileCollection_iface);

    *newenum = (IUnknown*)&This->IEnumVARIANT_iface;

    return S_OK;
}

static ULONG WINAPI drivecoll_enumvariant_Release(IEnumVARIANT *iface)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IDriveCollection_Release(&This->data.u.drivecoll.coll->IDriveCollection_iface);
        heap_free(This);
    }

    return ref;
}

static HRESULT find_next_drive(struct enumvariant *penum)
{
    int i = penum->data.u.drivecoll.cur == -1 ? 0 : penum->data.u.drivecoll.cur + 1;

    for (; i < 32; i++)
        if (penum->data.u.drivecoll.coll->drives & (1 << i))
        {
            penum->data.u.drivecoll.cur = i;
            return S_OK;
        }

    return S_FALSE;
}

static HRESULT WINAPI drivecoll_enumvariant_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    ULONG count = 0;

    TRACE("(%p)->(%d %p %p)\n", This, celt, var, fetched);

    if (fetched)
        *fetched = 0;

    if (!celt) return S_OK;

    while (find_next_drive(This) == S_OK)
    {
        IDrive *drive;
        HRESULT hr;

        hr = create_drive('A' + This->data.u.drivecoll.cur, &drive);
        if (FAILED(hr)) return hr;

        V_VT(&var[count]) = VT_DISPATCH;
        V_DISPATCH(&var[count]) = (IDispatch*)drive;

        if (++count >= celt) break;
    }

    if (fetched)
        *fetched = count;

    return (count < celt) ? S_FALSE : S_OK;
}

static HRESULT WINAPI drivecoll_enumvariant_Skip(IEnumVARIANT *iface, ULONG celt)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%d)\n", This, celt);

    if (!celt) return S_OK;

    while (celt && find_next_drive(This) == S_OK)
        celt--;

    return celt ? S_FALSE : S_OK;
}

static HRESULT WINAPI drivecoll_enumvariant_Reset(IEnumVARIANT *iface)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)\n", This);

    This->data.u.drivecoll.cur = -1;
    return S_OK;
}

static HRESULT WINAPI drivecoll_enumvariant_Clone(IEnumVARIANT *iface, IEnumVARIANT **pclone)
{
    struct enumvariant *This = impl_from_IEnumVARIANT(iface);
    FIXME("(%p)->(%p): stub\n", This, pclone);
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl drivecollenumvariantvtbl = {
    enumvariant_QueryInterface,
    enumvariant_AddRef,
    drivecoll_enumvariant_Release,
    drivecoll_enumvariant_Next,
    drivecoll_enumvariant_Skip,
    drivecoll_enumvariant_Reset,
    drivecoll_enumvariant_Clone
};

static HRESULT create_drivecoll_enum(struct drivecollection *collection, IUnknown **newenum)
{
    struct enumvariant *This;

    *newenum = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IEnumVARIANT_iface.lpVtbl = &drivecollenumvariantvtbl;
    This->ref = 1;
    This->data.u.drivecoll.coll = collection;
    This->data.u.drivecoll.cur = -1;
    IDriveCollection_AddRef(&collection->IDriveCollection_iface);

    *newenum = (IUnknown*)&This->IEnumVARIANT_iface;

    return S_OK;
}

static HRESULT WINAPI foldercoll_QueryInterface(IFolderCollection *iface, REFIID riid, void **obj)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IFolderCollection ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = &This->IFolderCollection_iface;
    }
    else if (IsEqualIID( riid, &IID_IProvideClassInfo ))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI foldercoll_AddRef(IFolderCollection *iface)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI foldercoll_Release(IFolderCollection *iface)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        SysFreeString(This->path);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI foldercoll_GetTypeInfoCount(IFolderCollection *iface, UINT *pctinfo)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI foldercoll_GetTypeInfo(IFolderCollection *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IFolderCollection_tid, ppTInfo);
}

static HRESULT WINAPI foldercoll_GetIDsOfNames(IFolderCollection *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IFolderCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI foldercoll_Invoke(IFolderCollection *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IFolderCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI foldercoll_Add(IFolderCollection *iface, BSTR name, IFolder **folder)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_w(name), folder);
    return E_NOTIMPL;
}

static HRESULT WINAPI foldercoll_get_Item(IFolderCollection *iface, VARIANT key, IFolder **folder)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    FIXME("(%p)->(%p): stub\n", This, folder);
    return E_NOTIMPL;
}

static HRESULT WINAPI foldercoll_get__NewEnum(IFolderCollection *iface, IUnknown **newenum)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);

    TRACE("(%p)->(%p)\n", This, newenum);

    if(!newenum)
        return E_POINTER;

    return create_foldercoll_enum(This, newenum);
}

static HRESULT WINAPI foldercoll_get_Count(IFolderCollection *iface, LONG *count)
{
    struct foldercollection *This = impl_from_IFolderCollection(iface);
    static const WCHAR allW[] = {'\\','*',0};
    WIN32_FIND_DATAW data;
    WCHAR pathW[MAX_PATH];
    HANDLE handle;

    TRACE("(%p)->(%p)\n", This, count);

    if(!count)
        return E_POINTER;

    *count = 0;

    strcpyW(pathW, This->path);
    strcatW(pathW, allW);
    handle = FindFirstFileW(pathW, &data);
    if (handle == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    do
    {
        if (is_dir_data(&data))
            *count += 1;
    } while (FindNextFileW(handle, &data));
    FindClose(handle);

    return S_OK;
}

static const IFolderCollectionVtbl foldercollvtbl = {
    foldercoll_QueryInterface,
    foldercoll_AddRef,
    foldercoll_Release,
    foldercoll_GetTypeInfoCount,
    foldercoll_GetTypeInfo,
    foldercoll_GetIDsOfNames,
    foldercoll_Invoke,
    foldercoll_Add,
    foldercoll_get_Item,
    foldercoll_get__NewEnum,
    foldercoll_get_Count
};

static HRESULT create_foldercoll(BSTR path, IFolderCollection **folders)
{
    struct foldercollection *This;

    *folders = NULL;

    This = heap_alloc(sizeof(struct foldercollection));
    if (!This) return E_OUTOFMEMORY;

    This->IFolderCollection_iface.lpVtbl = &foldercollvtbl;
    This->ref = 1;
    This->path = SysAllocString(path);
    if (!This->path)
    {
        heap_free(This);
        return E_OUTOFMEMORY;
    }

    init_classinfo(&CLSID_Folders, (IUnknown *)&This->IFolderCollection_iface, &This->classinfo);
    *folders = &This->IFolderCollection_iface;

    return S_OK;
}

static HRESULT WINAPI filecoll_QueryInterface(IFileCollection *iface, REFIID riid, void **obj)
{
    struct filecollection *This = impl_from_IFileCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IFileCollection ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = &This->IFileCollection_iface;
    }
    else if (IsEqualIID( riid, &IID_IProvideClassInfo ))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI filecoll_AddRef(IFileCollection *iface)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI filecoll_Release(IFileCollection *iface)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        SysFreeString(This->path);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI filecoll_GetTypeInfoCount(IFileCollection *iface, UINT *pctinfo)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI filecoll_GetTypeInfo(IFileCollection *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IFileCollection_tid, ppTInfo);
}

static HRESULT WINAPI filecoll_GetIDsOfNames(IFileCollection *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IFileCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI filecoll_Invoke(IFileCollection *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IFileCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI filecoll_get_Item(IFileCollection *iface, VARIANT Key, IFile **file)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    FIXME("(%p)->(%p)\n", This, file);
    return E_NOTIMPL;
}

static HRESULT WINAPI filecoll_get__NewEnum(IFileCollection *iface, IUnknown **ppenum)
{
    struct filecollection *This = impl_from_IFileCollection(iface);

    TRACE("(%p)->(%p)\n", This, ppenum);

    if(!ppenum)
        return E_POINTER;

    return create_filecoll_enum(This, ppenum);
}

static HRESULT WINAPI filecoll_get_Count(IFileCollection *iface, LONG *count)
{
    struct filecollection *This = impl_from_IFileCollection(iface);
    static const WCHAR allW[] = {'\\','*',0};
    WIN32_FIND_DATAW data;
    WCHAR pathW[MAX_PATH];
    HANDLE handle;

    TRACE("(%p)->(%p)\n", This, count);

    if(!count)
        return E_POINTER;

    *count = 0;

    strcpyW(pathW, This->path);
    strcatW(pathW, allW);
    handle = FindFirstFileW(pathW, &data);
    if (handle == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    do
    {
        if (is_file_data(&data))
            *count += 1;
    } while (FindNextFileW(handle, &data));
    FindClose(handle);

    return S_OK;
}

static const IFileCollectionVtbl filecollectionvtbl = {
    filecoll_QueryInterface,
    filecoll_AddRef,
    filecoll_Release,
    filecoll_GetTypeInfoCount,
    filecoll_GetTypeInfo,
    filecoll_GetIDsOfNames,
    filecoll_Invoke,
    filecoll_get_Item,
    filecoll_get__NewEnum,
    filecoll_get_Count
};

static HRESULT create_filecoll(BSTR path, IFileCollection **files)
{
    struct filecollection *This;

    *files = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IFileCollection_iface.lpVtbl = &filecollectionvtbl;
    This->ref = 1;
    This->path = SysAllocString(path);
    if (!This->path)
    {
        heap_free(This);
        return E_OUTOFMEMORY;
    }

    init_classinfo(&CLSID_Files, (IUnknown *)&This->IFileCollection_iface, &This->classinfo);
    *files = &This->IFileCollection_iface;
    return S_OK;
}

static HRESULT WINAPI drivecoll_QueryInterface(IDriveCollection *iface, REFIID riid, void **obj)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IDriveCollection ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = &This->IDriveCollection_iface;
    }
    else if (IsEqualIID( riid, &IID_IProvideClassInfo ))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI drivecoll_AddRef(IDriveCollection *iface)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI drivecoll_Release(IDriveCollection *iface)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI drivecoll_GetTypeInfoCount(IDriveCollection *iface, UINT *pctinfo)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI drivecoll_GetTypeInfo(IDriveCollection *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IDriveCollection_tid, ppTInfo);
}

static HRESULT WINAPI drivecoll_GetIDsOfNames(IDriveCollection *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IDriveCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI drivecoll_Invoke(IDriveCollection *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IDriveCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI drivecoll_get_Item(IDriveCollection *iface, VARIANT key, IDrive **drive)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);
    FIXME("(%p)->(%p): stub\n", This, drive);
    return E_NOTIMPL;
}

static HRESULT WINAPI drivecoll_get__NewEnum(IDriveCollection *iface, IUnknown **ppenum)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);

    TRACE("(%p)->(%p)\n", This, ppenum);

    if(!ppenum)
        return E_POINTER;

    return create_drivecoll_enum(This, ppenum);
}

static HRESULT WINAPI drivecoll_get_Count(IDriveCollection *iface, LONG *count)
{
    struct drivecollection *This = impl_from_IDriveCollection(iface);

    TRACE("(%p)->(%p)\n", This, count);

    if (!count) return E_POINTER;

    *count = This->count;
    return S_OK;
}

static const IDriveCollectionVtbl drivecollectionvtbl = {
    drivecoll_QueryInterface,
    drivecoll_AddRef,
    drivecoll_Release,
    drivecoll_GetTypeInfoCount,
    drivecoll_GetTypeInfo,
    drivecoll_GetIDsOfNames,
    drivecoll_Invoke,
    drivecoll_get_Item,
    drivecoll_get__NewEnum,
    drivecoll_get_Count
};

static HRESULT create_drivecoll(IDriveCollection **drives)
{
    struct drivecollection *This;
    DWORD mask;

    *drives = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IDriveCollection_iface.lpVtbl = &drivecollectionvtbl;
    This->ref = 1;
    This->drives = mask = GetLogicalDrives();
    /* count set bits */
    for (This->count = 0; mask; This->count++)
        mask &= mask - 1;

    init_classinfo(&CLSID_Drives, (IUnknown *)&This->IDriveCollection_iface, &This->classinfo);
    *drives = &This->IDriveCollection_iface;
    return S_OK;
}

static HRESULT WINAPI folder_QueryInterface(IFolder *iface, REFIID riid, void **obj)
{
    struct folder *This = impl_from_IFolder(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IFolder ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown))
    {
        *obj = &This->IFolder_iface;
    }
    else if (IsEqualIID( riid, &IID_IProvideClassInfo ))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*obj);
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
    {
        SysFreeString(This->path);
        heap_free(This);
    }

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

    TRACE("(%p)->(%p)\n", This, path);

    if(!path)
        return E_POINTER;

    *path = SysAllocString(This->path);
    return *path ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI folder_get_Name(IFolder *iface, BSTR *name)
{
    struct folder *This = impl_from_IFolder(iface);
    WCHAR *ptr;

    TRACE("(%p)->(%p)\n", This, name);

    if(!name)
        return E_POINTER;

    *name = NULL;

    ptr = strrchrW(This->path, '\\');
    if (ptr)
    {
        *name = SysAllocString(ptr+1);
        TRACE("%s\n", debugstr_w(*name));
        if (!*name) return E_OUTOFMEMORY;
    }
    else
        return E_FAIL;

    return S_OK;
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

    TRACE("(%p)->(%p)\n", This, folders);

    if(!folders)
        return E_POINTER;

    return create_foldercoll(This->path, folders);
}

static HRESULT WINAPI folder_get_Files(IFolder *iface, IFileCollection **files)
{
    struct folder *This = impl_from_IFolder(iface);

    TRACE("(%p)->(%p)\n", This, files);

    if(!files)
        return E_POINTER;

    return create_filecoll(This->path, files);
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

HRESULT create_folder(const WCHAR *path, IFolder **folder)
{
    struct folder *This;

    *folder = NULL;

    TRACE("%s\n", debugstr_w(path));

    This = heap_alloc(sizeof(struct folder));
    if (!This) return E_OUTOFMEMORY;

    This->IFolder_iface.lpVtbl = &foldervtbl;
    This->ref = 1;
    This->path = SysAllocString(path);
    if (!This->path)
    {
        heap_free(This);
        return E_OUTOFMEMORY;
    }

    init_classinfo(&CLSID_Folder, (IUnknown *)&This->IFolder_iface, &This->classinfo);
    *folder = &This->IFolder_iface;

    return S_OK;
}

static HRESULT WINAPI file_QueryInterface(IFile *iface, REFIID riid, void **obj)
{
    struct file *This = impl_from_IFile(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IFile) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &This->IFile_iface;
    }
    else if (IsEqualIID( riid, &IID_IProvideClassInfo ))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
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
    {
        heap_free(This->path);
        heap_free(This);
    }

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

static HRESULT WINAPI file_get_Path(IFile *iface, BSTR *path)
{
    struct file *This = impl_from_IFile(iface);

    TRACE("(%p)->(%p)\n", This, path);

    if (!path)
        return E_POINTER;

    *path = SysAllocString(This->path);
    if (!*path)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI file_get_Name(IFile *iface, BSTR *name)
{
    struct file *This = impl_from_IFile(iface);
    WCHAR *ptr;

    TRACE("(%p)->(%p)\n", This, name);

    if(!name)
        return E_POINTER;

    *name = NULL;

    ptr = strrchrW(This->path, '\\');
    if (ptr)
    {
        *name = SysAllocString(ptr+1);
        TRACE("%s\n", debugstr_w(*name));
        if (!*name) return E_OUTOFMEMORY;
    }
    else
        return E_FAIL;

    return S_OK;
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

    TRACE("(%p)->(%x)\n", This, pfa);

    return SetFileAttributesW(This->path, pfa) ? S_OK : create_error(GetLastError());
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
    ULARGE_INTEGER size;
    WIN32_FIND_DATAW fd;
    HANDLE f;

    TRACE("(%p)->(%p)\n", This, pvarSize);

    if(!pvarSize)
        return E_POINTER;

    f = FindFirstFileW(This->path, &fd);
    if(f == INVALID_HANDLE_VALUE)
        return create_error(GetLastError());
    FindClose(f);

    size.u.LowPart = fd.nFileSizeLow;
    size.u.HighPart = fd.nFileSizeHigh;

    return variant_from_largeint(&size, pvarSize);
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

static HRESULT WINAPI file_OpenAsTextStream(IFile *iface, IOMode mode, Tristate format, ITextStream **stream)
{
    struct file *This = impl_from_IFile(iface);

    TRACE("(%p)->(%d %d %p)\n", This, mode, format, stream);

    if (format == TristateUseDefault) {
        FIXME("default format not handled, defaulting to unicode\n");
        format = TristateTrue;
    }

    return create_textstream(This->path, OPEN_EXISTING, mode, format == TristateTrue, stream);
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

    attrs = GetFileAttributesW(f->path);
    if(attrs==INVALID_FILE_ATTRIBUTES ||
            (attrs&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE))) {
        heap_free(f->path);
        heap_free(f);
        return create_error(GetLastError());
    }

    init_classinfo(&CLSID_File, (IUnknown *)&f->IFile_iface, &f->classinfo);
    *file = &f->IFile_iface;
    return S_OK;
}

static HRESULT WINAPI filesys_QueryInterface(IFileSystem3 *iface, REFIID riid, void **ppvObject)
{
    struct filesystem *This = impl_from_IFileSystem3(iface);

    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IFileSystem3 ) ||
         IsEqualGUID( riid, &IID_IFileSystem ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->IFileSystem3_iface;
    }
    else if (IsEqualGUID( riid, &IID_IProvideClassInfo ))
    {
        *ppvObject = &This->classinfo.IProvideClassInfo_iface;
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

    IUnknown_AddRef((IUnknown*)*ppvObject);

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
    TRACE("%p %p\n", iface, ppdrives);
    return create_drivecoll(ppdrives);
}

static HRESULT WINAPI filesys_BuildPath(IFileSystem3 *iface, BSTR Path,
                                            BSTR Name, BSTR *Result)
{
    BSTR ret;

    TRACE("%p %s %s %p\n", iface, debugstr_w(Path), debugstr_w(Name), Result);

    if (!Result) return E_POINTER;

    if (Path && Name)
    {
        int path_len = SysStringLen(Path), name_len = SysStringLen(Name);

        /* if both parts have backslashes strip one from Path */
        if (Path[path_len-1] == '\\' && Name[0] == '\\')
        {
            path_len -= 1;

            ret = SysAllocStringLen(NULL, path_len + name_len);
            if (ret)
            {
                strcpyW(ret, Path);
                ret[path_len] = 0;
                strcatW(ret, Name);
            }
        }
        else if (Path[path_len-1] != '\\' && Name[0] != '\\')
        {
            ret = SysAllocStringLen(NULL, path_len + name_len + 1);
            if (ret)
            {
                strcpyW(ret, Path);
                if (Path[path_len-1] != ':')
                    strcatW(ret, bsW);
                strcatW(ret, Name);
            }
        }
        else
        {
            ret = SysAllocStringLen(NULL, path_len + name_len);
            if (ret)
            {
                strcpyW(ret, Path);
                strcatW(ret, Name);
            }
        }
    }
    else if (Path || Name)
        ret = SysAllocString(Path ? Path : Name);
    else
        ret = SysAllocStringLen(NULL, 0);

    if (!ret) return E_OUTOFMEMORY;
    *Result = ret;

    return S_OK;
}

static HRESULT WINAPI filesys_GetDriveName(IFileSystem3 *iface, BSTR path, BSTR *drive)
{
    TRACE("(%p)->(%s %p)\n", iface, debugstr_w(path), drive);

    if (!drive)
        return E_POINTER;

    *drive = NULL;

    if (path && strlenW(path) > 1 && path[1] == ':')
        *drive = SysAllocStringLen(path, 2);

    return S_OK;
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

static HRESULT WINAPI filesys_GetExtensionName(IFileSystem3 *iface, BSTR path,
                                            BSTR *ext)
{
    INT len;

    TRACE("%p %s %p\n", iface, debugstr_w(path), ext);

    *ext = NULL;
    len = SysStringLen(path);
    while (len) {
        if (path[len-1] == '.') {
            *ext = SysAllocString(&path[len]);
            if (!*ext)
                return E_OUTOFMEMORY;
            break;
        }
        len--;
    }

    return S_OK;
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
    UINT len;
    WCHAR driveletter;
    TRACE("%p %s %p\n", iface, debugstr_w(DriveSpec), pfExists);

    if (!pfExists) return E_POINTER;

    *pfExists = VARIANT_FALSE;
    len = SysStringLen(DriveSpec);

    if (len >= 1) {
        driveletter = toupperW(DriveSpec[0]);
        if (driveletter >= 'A' && driveletter <= 'Z'
                && (len < 2 || DriveSpec[1] == ':')
                && (len < 3 || DriveSpec[2] == '\\')) {
            const WCHAR root[] = {driveletter, ':', '\\', 0};
            UINT drivetype = GetDriveTypeW(root);
            *pfExists = drivetype != DRIVE_NO_ROOT_DIR && drivetype != DRIVE_UNKNOWN ? VARIANT_TRUE : VARIANT_FALSE;
        }
    }

    return S_OK;
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
    UINT len;
    HRESULT hr;
    WCHAR driveletter;
    VARIANT_BOOL drive_exists;

    TRACE("%p %s %p\n", iface, debugstr_w(DriveSpec), ppdrive);

    if (!ppdrive)
        return E_POINTER;

    *ppdrive = NULL;

    /* DriveSpec may be one of: 'x', 'x:', 'x:\', '\\computer\share' */
    len = SysStringLen(DriveSpec);
    if (!len)
        return E_INVALIDARG;
    else if (len <= 3) {
        driveletter = toupperW(DriveSpec[0]);
        if (driveletter < 'A' || driveletter > 'Z'
                || (len >= 2 && DriveSpec[1] != ':')
                || (len == 3 && DriveSpec[2] != '\\'))
            return E_INVALIDARG;
        hr = IFileSystem3_DriveExists(iface, DriveSpec, &drive_exists);
        if (FAILED(hr))
            return hr;
        if (drive_exists == VARIANT_FALSE)
            return CTL_E_DEVICEUNAVAILABLE;
        return create_drive(driveletter, ppdrive);
    } else {
        if (DriveSpec[0] != '\\' || DriveSpec[1] != '\\')
            return E_INVALIDARG;
        FIXME("%s not implemented yet\n", debugstr_w(DriveSpec));
        return E_NOTIMPL;
    }
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
                                            IFolder **folder)
{
    DWORD attrs;

    TRACE("%p %s %p\n", iface, debugstr_w(FolderPath), folder);

    if(!folder)
        return E_POINTER;

    *folder = NULL;
    if(!FolderPath)
        return E_INVALIDARG;

    attrs = GetFileAttributesW(FolderPath);
    if((attrs == INVALID_FILE_ATTRIBUTES) || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return CTL_E_PATHNOTFOUND;

    return create_folder(FolderPath, folder);
}

static HRESULT WINAPI filesys_GetSpecialFolder(IFileSystem3 *iface,
                                            SpecialFolderConst SpecialFolder,
                                            IFolder **folder)
{
    WCHAR pathW[MAX_PATH];
    DWORD ret;

    TRACE("%p %d %p\n", iface, SpecialFolder, folder);

    if (!folder)
        return E_POINTER;

    *folder = NULL;

    switch (SpecialFolder)
    {
    case WindowsFolder:
        ret = GetWindowsDirectoryW(pathW, sizeof(pathW)/sizeof(WCHAR));
        break;
    case SystemFolder:
        ret = GetSystemDirectoryW(pathW, sizeof(pathW)/sizeof(WCHAR));
        break;
    case TemporaryFolder:
        ret = GetTempPathW(sizeof(pathW)/sizeof(WCHAR), pathW);
        /* we don't want trailing backslash */
        if (ret && pathW[ret-1] == '\\')
            pathW[ret-1] = 0;
        break;
    default:
        FIXME("unknown special folder type, %d\n", SpecialFolder);
        return E_INVALIDARG;
    }

    if (!ret)
        return HRESULT_FROM_WIN32(GetLastError());

    return create_folder(pathW, folder);
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
    if(src_len+1 >= MAX_PATH) {
        FindClose(f);
        return E_FAIL;
    }
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
            }

            FindClose(f);
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

    return create_folder(path, folder);
}

static HRESULT WINAPI filesys_CreateTextFile(IFileSystem3 *iface, BSTR filename,
                                            VARIANT_BOOL overwrite, VARIANT_BOOL unicode,
                                            ITextStream **stream)
{
    DWORD disposition;

    TRACE("%p %s %d %d %p\n", iface, debugstr_w(filename), overwrite, unicode, stream);

    disposition = overwrite == VARIANT_TRUE ? CREATE_ALWAYS : CREATE_NEW;
    return create_textstream(filename, disposition, ForWriting, !!unicode, stream);
}

static HRESULT WINAPI filesys_OpenTextFile(IFileSystem3 *iface, BSTR filename,
                                            IOMode mode, VARIANT_BOOL create,
                                            Tristate format, ITextStream **stream)
{
    DWORD disposition;

    TRACE("(%p)->(%s %d %d %d %p)\n", iface, debugstr_w(filename), mode, create, format, stream);
    disposition = create == VARIANT_TRUE ? OPEN_ALWAYS : OPEN_EXISTING;

    if (format == TristateUseDefault) {
        FIXME("default format not handled, defaulting to unicode\n");
        format = TristateTrue;
    }

    return create_textstream(filename, disposition, mode, format == TristateTrue, stream);
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
    static const WCHAR fmtW[] = {'%','d','.','%','d','.','%','d','.','%','d',0};
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
    if (!ret)
    {
        heap_free(ptr);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    get_versionstring(info, ver);
    heap_free(ptr);

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

static struct filesystem filesystem;

HRESULT WINAPI FileSystem_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    filesystem.IFileSystem3_iface.lpVtbl = &filesys_vtbl;
    init_classinfo(&CLSID_FileSystemObject, (IUnknown *)&filesystem.IFileSystem3_iface, &filesystem.classinfo);
    return IFileSystem3_QueryInterface(&filesystem.IFileSystem3_iface, riid, ppv);
}
