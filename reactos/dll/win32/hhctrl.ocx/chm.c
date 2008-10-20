/*
 * CHM Utility API
 *
 * Copyright 2005 James Hawkins
 * Copyright 2007 Jacek Caban
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

#include "hhctrl.h"

#include "winreg.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

#define BLOCK_BITS 12
#define BLOCK_SIZE (1 << BLOCK_BITS)
#define BLOCK_MASK (BLOCK_SIZE-1)

/* Reads a string from the #STRINGS section in the CHM file */
static LPCSTR GetChmString(CHMInfo *chm, DWORD offset)
{
    if(!chm->strings_stream)
        return NULL;

    if(chm->strings_size <= (offset >> BLOCK_BITS)) {
        if(chm->strings)
            chm->strings = heap_realloc_zero(chm->strings,
                    chm->strings_size = ((offset >> BLOCK_BITS)+1)*sizeof(char*));
        else
            chm->strings = heap_alloc_zero(
                    chm->strings_size = ((offset >> BLOCK_BITS)+1)*sizeof(char*));

    }

    if(!chm->strings[offset >> BLOCK_BITS]) {
        LARGE_INTEGER pos;
        DWORD read;
        HRESULT hres;

        pos.QuadPart = offset & ~BLOCK_MASK;
        hres = IStream_Seek(chm->strings_stream, pos, STREAM_SEEK_SET, NULL);
        if(FAILED(hres)) {
            WARN("Seek failed: %08x\n", hres);
            return NULL;
        }

        chm->strings[offset >> BLOCK_BITS] = heap_alloc(BLOCK_SIZE);

        hres = IStream_Read(chm->strings_stream, chm->strings[offset >> BLOCK_BITS],
                            BLOCK_SIZE, &read);
        if(FAILED(hres)) {
            WARN("Read failed: %08x\n", hres);
            heap_free(chm->strings[offset >> BLOCK_BITS]);
            chm->strings[offset >> BLOCK_BITS] = NULL;
            return NULL;
        }
    }

    return chm->strings[offset >> BLOCK_BITS] + (offset & BLOCK_MASK);
}

static BOOL ReadChmSystem(CHMInfo *chm)
{
    IStream *stream;
    DWORD ver=0xdeadbeef, read, buf_size;
    char *buf;
    HRESULT hres;

    struct {
        WORD code;
        WORD len;
    } entry;

    static const WCHAR wszSYSTEM[] = {'#','S','Y','S','T','E','M',0};

    hres = IStorage_OpenStream(chm->pStorage, wszSYSTEM, NULL, STGM_READ, 0, &stream);
    if(FAILED(hres)) {
        WARN("Could not open #SYSTEM stream: %08x\n", hres);
        return FALSE;
    }

    IStream_Read(stream, &ver, sizeof(ver), &read);
    TRACE("version is %x\n", ver);

    buf = heap_alloc(8*sizeof(DWORD));
    buf_size = 8*sizeof(DWORD);

    while(1) {
        hres = IStream_Read(stream, &entry, sizeof(entry), &read);
        if(hres != S_OK)
            break;

        if(entry.len > buf_size)
            buf = heap_realloc(buf, buf_size=entry.len);

        hres = IStream_Read(stream, buf, entry.len, &read);
        if(hres != S_OK)
            break;

        switch(entry.code) {
        case 0x0:
            TRACE("TOC is %s\n", debugstr_an(buf, entry.len));
            heap_free(chm->defToc);
            chm->defToc = strdupnAtoW(buf, entry.len);
            break;
        case 0x2:
            TRACE("Default topic is %s\n", debugstr_an(buf, entry.len));
            heap_free(chm->defTopic);
            chm->defTopic = strdupnAtoW(buf, entry.len);
            break;
        case 0x3:
            TRACE("Title is %s\n", debugstr_an(buf, entry.len));
            heap_free(chm->defTitle);
            chm->defTitle = strdupnAtoW(buf, entry.len);
            break;
        case 0x5:
            TRACE("Default window is %s\n", debugstr_an(buf, entry.len));
            break;
        case 0x6:
            TRACE("Compiled file is %s\n", debugstr_an(buf, entry.len));
            break;
        case 0x9:
            TRACE("Version is %s\n", debugstr_an(buf, entry.len));
            break;
        case 0xa:
            TRACE("Time is %08x\n", *(DWORD*)buf);
            break;
        case 0xc:
            TRACE("Number of info types: %d\n", *(DWORD*)buf);
            break;
        case 0xf:
            TRACE("Check sum: %x\n", *(DWORD*)buf);
            break;
        default:
            TRACE("unhandled code %x, size %x\n", entry.code, entry.len);
        }
    }

    heap_free(buf);
    IStream_Release(stream);

    return SUCCEEDED(hres);
}

LPWSTR FindContextAlias(CHMInfo *chm, DWORD index)
{
    IStream *ivb_stream;
    DWORD size, read, i;
    DWORD *buf;
    LPCSTR ret = NULL;
    HRESULT hres;

    static const WCHAR wszIVB[] = {'#','I','V','B',0};

    hres = IStorage_OpenStream(chm->pStorage, wszIVB, NULL, STGM_READ, 0, &ivb_stream);
    if(FAILED(hres)) {
        WARN("Could not open #IVB stream: %08x\n", hres);
        return NULL;
    }

    hres = IStream_Read(ivb_stream, &size, sizeof(size), &read);
    if(FAILED(hres)) {
        WARN("Read failed: %08x\n", hres);
        IStream_Release(ivb_stream);
        return NULL;
    }

    buf = heap_alloc(size);
    hres = IStream_Read(ivb_stream, buf, size, &read);
    IStream_Release(ivb_stream);
    if(FAILED(hres)) {
        WARN("Read failed: %08x\n", hres);
        heap_free(buf);
        return NULL;
    }

    size /= 2*sizeof(DWORD);

    for(i=0; i<size; i++) {
        if(buf[2*i] == index) {
            ret = GetChmString(chm, buf[2*i+1]);
            break;
        }
    }

    heap_free(buf);

    TRACE("returning %s\n", debugstr_a(ret));
    return strdupAtoW(ret);
}

/* Loads the HH_WINTYPE data from the CHM file
 *
 * FIXME: There may be more than one window type in the file, so
 *        add the ability to choose a certain window type
 */
BOOL LoadWinTypeFromCHM(HHInfo *info)
{
    LARGE_INTEGER liOffset;
    IStorage *pStorage = info->pCHMInfo->pStorage;
    IStream *pStream;
    HRESULT hr;
    DWORD cbRead;

    static const WCHAR windowsW[] = {'#','W','I','N','D','O','W','S',0};

    hr = IStorage_OpenStream(pStorage, windowsW, NULL, STGM_READ, 0, &pStream);
    if (FAILED(hr))
    {
        /* no defined window types so use (hopefully) sane defaults */
        static const WCHAR defaultwinW[] = {'d','e','f','a','u','l','t','w','i','n','\0'};
        static const WCHAR null[] = {0};
        memset((void*)&(info->WinType), 0, sizeof(info->WinType));
        info->WinType.cbStruct=sizeof(info->WinType);
        info->WinType.fUniCodeStrings=TRUE;
        info->WinType.pszType=strdupW(defaultwinW);
        info->WinType.pszToc = strdupW(info->pCHMInfo->defToc ? info->pCHMInfo->defToc : null);
        info->WinType.pszIndex = strdupW(null);
        info->WinType.fsValidMembers=0;
        info->WinType.fsWinProperties=HHWIN_PROP_TRI_PANE;
        info->WinType.pszCaption=strdupW(info->pCHMInfo->defTitle);
        info->WinType.dwStyles=WS_POPUP;
        info->WinType.dwExStyles=0;
        info->WinType.nShowState=SW_SHOW;
        info->WinType.pszFile=strdupW(info->pCHMInfo->defTopic);
        info->WinType.curNavType=HHWIN_NAVTYPE_TOC;
        return TRUE;
    }

    /* jump past the #WINDOWS header */
    liOffset.QuadPart = sizeof(DWORD) * 2;

    hr = IStream_Seek(pStream, liOffset, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto done;

    /* read the HH_WINTYPE struct data */
    hr = IStream_Read(pStream, &info->WinType, sizeof(info->WinType), &cbRead);
    if (FAILED(hr)) goto done;

    /* convert the #STRINGS offsets to actual strings */
    info->WinType.pszType     = info->pszType     = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszType));
    info->WinType.pszCaption  = info->pszCaption  = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszCaption));
    info->WinType.pszToc      = info->pszToc      = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszToc));
    info->WinType.pszIndex    = info->pszIndex    = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszIndex));
    info->WinType.pszFile     = info->pszFile     = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszFile));
    info->WinType.pszHome     = info->pszHome     = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszHome));
    info->WinType.pszJump1    = info->pszJump1    = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszJump1));
    info->WinType.pszJump2    = info->pszJump2    = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszJump2));
    info->WinType.pszUrlJump1 = info->pszUrlJump1 = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszUrlJump1));
    info->WinType.pszUrlJump2 = info->pszUrlJump2 = strdupAtoW(GetChmString(info->pCHMInfo, (DWORD_PTR)info->WinType.pszUrlJump2));

    /* FIXME: pszCustomTabs is a list of multiple zero-terminated strings so ReadString won't
     * work in this case
     */
#if 0
    info->WinType.pszCustomTabs = info->pszCustomTabs = CHM_ReadString(pChmInfo, (DWORD_PTR)info->WinType.pszCustomTabs);
#endif

done:
    IStream_Release(pStream);

    return SUCCEEDED(hr);
}

static LPCWSTR skip_schema(LPCWSTR url)
{
    static const WCHAR its_schema[] = {'i','t','s',':'};
    static const WCHAR msits_schema[] = {'m','s','-','i','t','s',':'};
    static const WCHAR mk_schema[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':'};

    if(!strncmpiW(its_schema, url, sizeof(its_schema)/sizeof(WCHAR)))
        return url+sizeof(its_schema)/sizeof(WCHAR);
    if(!strncmpiW(msits_schema, url, sizeof(msits_schema)/sizeof(WCHAR)))
        return url+sizeof(msits_schema)/sizeof(WCHAR);
    if(!strncmpiW(mk_schema, url, sizeof(mk_schema)/sizeof(WCHAR)))
        return url+sizeof(mk_schema)/sizeof(WCHAR);

    return url;
}

void SetChmPath(ChmPath *file, LPCWSTR base_file, LPCWSTR path)
{
    LPCWSTR ptr;
    static const WCHAR separatorW[] = {':',':',0};

    path = skip_schema(path);

    ptr = strstrW(path, separatorW);
    if(ptr) {
        WCHAR chm_file[MAX_PATH];
        WCHAR rel_path[MAX_PATH];
        WCHAR base_path[MAX_PATH];
        LPWSTR p;

        strcpyW(base_path, base_file);
        p = strrchrW(base_path, '\\');
        if(p)
            *p = 0;

        memcpy(rel_path, path, (ptr-path)*sizeof(WCHAR));
        rel_path[ptr-path] = 0;

        PathCombineW(chm_file, base_path, rel_path);

        file->chm_file = strdupW(chm_file);
        ptr += 2;
    }else {
        file->chm_file = strdupW(base_file);
        ptr = path;
    }

    file->chm_index = strdupW(ptr);

    TRACE("ChmFile = {%s %s}\n", debugstr_w(file->chm_file), debugstr_w(file->chm_index));
}

IStream *GetChmStream(CHMInfo *info, LPCWSTR parent_chm, ChmPath *chm_file)
{
    IStorage *storage;
    IStream *stream = NULL;
    HRESULT hres;

    TRACE("%s (%s :: %s)\n", debugstr_w(parent_chm), debugstr_w(chm_file->chm_file),
          debugstr_w(chm_file->chm_index));

    if(parent_chm || chm_file->chm_file) {
        hres = IITStorage_StgOpenStorage(info->pITStorage,
                chm_file->chm_file ? chm_file->chm_file : parent_chm, NULL,
                STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &storage);
        if(FAILED(hres)) {
            WARN("Could not open storage: %08x\n", hres);
            return NULL;
        }
    }else {
        storage = info->pStorage;
        IStorage_AddRef(info->pStorage);
    }

    hres = IStorage_OpenStream(storage, chm_file->chm_index, NULL, STGM_READ, 0, &stream);
    IStorage_Release(storage);
    if(FAILED(hres))
        WARN("Could not open stream: %08x\n", hres);

    return stream;
}

/* Opens the CHM file for reading */
CHMInfo *OpenCHM(LPCWSTR szFile)
{
    WCHAR file[MAX_PATH] = {0};
    HRESULT hres;

    static const WCHAR wszSTRINGS[] = {'#','S','T','R','I','N','G','S',0};

    CHMInfo *ret = heap_alloc_zero(sizeof(CHMInfo));

    GetFullPathNameW(szFile, sizeof(file)/sizeof(file[0]), file, NULL);
    ret->szFile = strdupW(file);

    hres = CoCreateInstance(&CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER,
            &IID_IITStorage, (void **) &ret->pITStorage) ;
    if(FAILED(hres)) {
        WARN("Could not create ITStorage: %08x\n", hres);
        return CloseCHM(ret);
    }

    hres = IITStorage_StgOpenStorage(ret->pITStorage, szFile, NULL,
            STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &ret->pStorage);
    if(FAILED(hres)) {
        WARN("Could not open storage: %08x\n", hres);
        return CloseCHM(ret);
    }

    hres = IStorage_OpenStream(ret->pStorage, wszSTRINGS, NULL, STGM_READ, 0,
            &ret->strings_stream);
    if(FAILED(hres)) {
        WARN("Could not open #STRINGS stream: %08x\n", hres);
        /* It's not critical, so we pass */
    }

    if(!ReadChmSystem(ret)) {
        WARN("Could not read #SYSTEM\n");
        return CloseCHM(ret);
    }

    return ret;
}

CHMInfo *CloseCHM(CHMInfo *chm)
{
    if(chm->pITStorage)
        IITStorage_Release(chm->pITStorage);

    if(chm->pStorage)
        IStorage_Release(chm->pStorage);

    if(chm->strings_stream)
        IStream_Release(chm->strings_stream);

    if(chm->strings_size) {
        DWORD i;

        for(i=0; i<chm->strings_size; i++)
            heap_free(chm->strings[i]);
    }

    heap_free(chm->strings);
    heap_free(chm->defTitle);
    heap_free(chm->defTopic);
    heap_free(chm->defToc);
    heap_free(chm);

    return NULL;
}
