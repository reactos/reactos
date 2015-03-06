/*
 *
 *      Copyright 1997  Marcus Meissner
 *      Copyright 1998  Juergen Schmied
 *      Copyright 2005  Mike McCormack
 *      Copyright 2009  Andrew Hill
 *      Copyright 2013  Dominik Hornung
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
 * NOTES
 *   Nearly complete information about the binary formats
 *   of .lnk files available at http://www.wotsit.org
 *
 *  You can use winedump to examine the contents of a link file:
 *   winedump lnk sc.lnk
 *
 *  MSI advertised shortcuts are totally undocumented.  They provide an
 *   icon for a program that is not yet installed, and invoke MSI to
 *   install the program when the shortcut is clicked on.  They are
 *   created by passing a special string to SetPath, and the information
 *   in that string is parsed an stored.
 */

#include "precomp.h"

#include <appmgmt.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHLINK_LOCAL  0
#define SHLINK_REMOTE 1
#define MAX_PROPERTY_SHEET_PAGE 32

/* link file formats */

#include "pshpack1.h"

struct LOCATION_INFO
{
    DWORD        dwTotalSize;
    DWORD        dwHeaderSize;
    DWORD        dwFlags;
    DWORD        dwVolTableOfs;
    DWORD        dwLocalPathOfs;
    DWORD        dwNetworkVolTableOfs;
    DWORD        dwFinalPathOfs;
};

struct LOCAL_VOLUME_INFO
{
    DWORD        dwSize;
    DWORD        dwType;
    DWORD        dwVolSerial;
    DWORD        dwVolLabelOfs;
};

struct volume_info
{
    DWORD        type;
    DWORD        serial;
    WCHAR        label[12];  /* assume 8.3 */
};

#include "poppack.h"

/**************************************************************************
* SH_GetTargetTypeByPath
*
* Function to get target type by passing full path to it
*/
LPWSTR SH_GetTargetTypeByPath(LPCWSTR lpcwFullPath)
{
    LPCWSTR pwszExt;
    static WCHAR wszBuf[MAX_PATH];

    /* Get file information */
    SHFILEINFO fi;
    if (!SHGetFileInfoW(lpcwFullPath, 0, &fi, sizeof(fi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES ))
    {
        ERR("SHGetFileInfoW failed for %ls (%lu)\n", lpcwFullPath, GetLastError());
        fi.szTypeName[0] = L'\0';
        fi.hIcon = NULL;
    }

    pwszExt = PathFindExtensionW(lpcwFullPath);
    if (pwszExt[0])
    {
        if (!fi.szTypeName[0])
        {
            /* The file type is unknown, so default to string "FileExtension File" */
            size_t cchRemaining = 0;
            LPWSTR pwszEnd = NULL;

            StringCchPrintfExW(wszBuf, _countof(wszBuf), &pwszEnd, &cchRemaining, 0, L"%s ", pwszExt + 1);
        }
        else
            StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s (%s)", fi.szTypeName, pwszExt); /* Update file type */
    }

    return wszBuf;
}

/* IShellLink Implementation */

static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath);

/* strdup on the process heap */
static LPWSTR __inline HEAP_strdupAtoW(HANDLE heap, DWORD flags, LPCSTR str)
{
    INT len;
    LPWSTR p;

    assert(str);

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    p = (LPWSTR)HeapAlloc(heap, flags, len * sizeof(WCHAR));
    if (!p)
        return p;
    MultiByteToWideChar(CP_ACP, 0, str, -1, p, len);
    return p;
}

static LPWSTR __inline strdupW(LPCWSTR src)
{
    LPWSTR dest;
    if (!src) return NULL;
    dest = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(src) + 1) * sizeof(WCHAR));
    if (dest)
        wcscpy(dest, src);
    return dest;
}

CShellLink::CShellLink()
{
    pPidl = NULL;
    wHotKey = 0;
    memset(&time1, 0, sizeof(time1));
    memset(&time2, 0, sizeof(time2));
    memset(&time3, 0, sizeof(time3));
    iShowCmd = SW_SHOWNORMAL;
    sIcoPath = NULL;
    iIcoNdx = 0;
    sPath = NULL;
    sArgs = NULL;
    sWorkDir = NULL;
    sDescription = NULL;
    sPathRel = NULL;
    sProduct = NULL;
    sComponent = NULL;
    memset(&volume, 0, sizeof(volume));
    sLinkPath = NULL;
    bRunAs = FALSE;
    bDirty = FALSE;
    iIdOpen = -1;
}

CShellLink::~CShellLink()
{
    TRACE("-- destroying IShellLink(%p)\n", this);

    HeapFree(GetProcessHeap(), 0, sIcoPath);
    HeapFree(GetProcessHeap(), 0, sArgs);
    HeapFree(GetProcessHeap(), 0, sWorkDir);
    HeapFree(GetProcessHeap(), 0, sDescription);
    HeapFree(GetProcessHeap(), 0, sPath);
    HeapFree(GetProcessHeap(), 0, sLinkPath);

    if (pPidl)
        ILFree(pPidl);
}

HRESULT WINAPI CShellLink::GetClassID(CLSID *pclsid)
{
    TRACE("%p %p\n", this, pclsid);

    if (pclsid == NULL)
        return E_POINTER;
    *pclsid = CLSID_ShellLink;
    return S_OK;
}

HRESULT WINAPI CShellLink::IsDirty()
{
    TRACE("(%p)\n", this);

    if (bDirty)
        return S_OK;

    return S_FALSE;
}

HRESULT WINAPI CShellLink::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    TRACE("(%p, %s, %x)\n", this, debugstr_w(pszFileName), dwMode);

    if (dwMode == 0)
        dwMode = STGM_READ | STGM_SHARE_DENY_WRITE;

    CComPtr<IStream> stm;
    HRESULT hr = SHCreateStreamOnFileW(pszFileName, dwMode, &stm);
    if (SUCCEEDED(hr))
    {
        HeapFree(GetProcessHeap(), 0, sLinkPath);
        sLinkPath = strdupW(pszFileName);
        hr = Load(stm);
        ShellLink_UpdatePath(sPathRel, pszFileName, sWorkDir, &sPath);
        bDirty = FALSE;
    }
    TRACE("-- returning hr %08x\n", hr);
    return hr;
}

HRESULT WINAPI CShellLink::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    TRACE("(%p)->(%s)\n", this, debugstr_w(pszFileName));

    if (!pszFileName)
        return E_FAIL;

    CComPtr<IStream> stm;
    HRESULT hr = SHCreateStreamOnFileW(pszFileName, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, &stm);
    if (SUCCEEDED(hr))
    {
        hr = Save(stm, FALSE);

        if (SUCCEEDED(hr))
        {
            if (sLinkPath)
                HeapFree(GetProcessHeap(), 0, sLinkPath);

            sLinkPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(pszFileName) + 1) * sizeof(WCHAR));
            if (sLinkPath)
                wcscpy(sLinkPath, pszFileName);

            bDirty = FALSE;
        }
        else
        {
            DeleteFileW(pszFileName);
            WARN("Failed to create shortcut %s\n", debugstr_w(pszFileName));
        }
    }

    return hr;
}

HRESULT WINAPI CShellLink::SaveCompleted(LPCOLESTR pszFileName)
{
    FIXME("(%p)->(%s)\n", this, debugstr_w(pszFileName));
    return S_OK;
}

HRESULT WINAPI CShellLink::GetCurFile(LPOLESTR *ppszFileName)
{
    *ppszFileName = NULL;

    if (!sLinkPath)
    {
        /* IPersistFile::GetCurFile called before IPersistFile::Save */
        return S_FALSE;
    }

    *ppszFileName = (LPOLESTR)CoTaskMemAlloc((wcslen(sLinkPath) + 1) * sizeof(WCHAR));
    if (!*ppszFileName)
    {
        /* out of memory */
        return E_OUTOFMEMORY;
    }

    /* copy last saved filename */
    wcscpy(*ppszFileName, sLinkPath);

    return S_OK;
}

/************************************************************************
 * IPersistStream_IsDirty (IPersistStream)
 */

static HRESULT Stream_LoadString(IStream* stm, BOOL unicode, LPWSTR *pstr)
{
    TRACE("%p\n", stm);

    USHORT len;
    DWORD count = 0;
    HRESULT hr = stm->Read(&len, sizeof(len), &count);
    if (FAILED(hr) || count != sizeof(len))
        return E_FAIL;

    if (unicode)
        len *= sizeof (WCHAR);

    TRACE("reading %d\n", len);
    LPSTR temp = (LPSTR)HeapAlloc(GetProcessHeap(), 0, len + sizeof(WCHAR));
    if (!temp)
        return E_OUTOFMEMORY;
    count = 0;
    hr = stm->Read(temp, len, &count);
    if(FAILED(hr) || count != len)
    {
        HeapFree(GetProcessHeap(), 0, temp);
        return E_FAIL;
    }

    TRACE("read %s\n", debugstr_an(temp, len));

    /* convert to unicode if necessary */
    LPWSTR str;
    if (!unicode)
    {
        count = MultiByteToWideChar(CP_ACP, 0, temp, len, NULL, 0);
        str = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (count + 1) * sizeof (WCHAR));
        if (!str)
        {
            HeapFree(GetProcessHeap(), 0, temp);
            return E_OUTOFMEMORY;
        }
        MultiByteToWideChar(CP_ACP, 0, temp, len, str, count);
        HeapFree(GetProcessHeap(), 0, temp);
    }
    else
    {
        count /= 2;
        str = (LPWSTR)temp;
    }
    str[count] = 0;

    *pstr = str;

    return S_OK;
}

static HRESULT Stream_ReadChunk(IStream* stm, LPVOID *data)
{
    struct sized_chunk
    {
        DWORD size;
        unsigned char data[1];
    } *chunk;

    TRACE("%p\n", stm);

    DWORD size;
    ULONG count;
    HRESULT hr = stm->Read(&size, sizeof(size), &count);
    if (FAILED(hr) || count != sizeof(size))
        return E_FAIL;

    chunk = static_cast<sized_chunk *>(HeapAlloc(GetProcessHeap(), 0, size));
    if (!chunk)
        return E_OUTOFMEMORY;

    chunk->size = size;
    hr = stm->Read(chunk->data, size - sizeof(size), &count);
    if (FAILED(hr) || count != (size - sizeof(size)))
    {
        HeapFree(GetProcessHeap(), 0, chunk);
        return E_FAIL;
    }

    TRACE("Read %d bytes\n", chunk->size);

    *data = chunk;

    return S_OK;
}

static BOOL Stream_LoadVolume(LOCAL_VOLUME_INFO *vol, CShellLink::volume_info *volume)
{
    volume->serial = vol->dwVolSerial;
    volume->type = vol->dwType;

    if (!vol->dwVolLabelOfs)
        return FALSE;
    if (vol->dwSize <= vol->dwVolLabelOfs)
        return FALSE;
    INT len = vol->dwSize - vol->dwVolLabelOfs;

    LPSTR label = (LPSTR)vol;
    label += vol->dwVolLabelOfs;
    MultiByteToWideChar(CP_ACP, 0, label, len, volume->label, _countof(volume->label));

    return TRUE;
}

static LPWSTR Stream_LoadPath(LPCSTR p, DWORD maxlen)
{
    UINT len = 0;

    while (p[len] && len < maxlen)
        len++;

    UINT wlen = MultiByteToWideChar(CP_ACP, 0, p, len, NULL, 0);
    LPWSTR path = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wlen + 1) * sizeof(WCHAR));
    if (!path)
        return NULL;
    MultiByteToWideChar(CP_ACP, 0, p, len, path, wlen);
    path[wlen] = 0;

    return path;
}

static HRESULT Stream_LoadLocation(IStream *stm,
                                    CShellLink::volume_info *volume, LPWSTR *path)
{
    char *p = NULL;
    HRESULT hr = Stream_ReadChunk(stm, (LPVOID*) &p);
    if (FAILED(hr))
        return hr;

    LOCATION_INFO *loc = reinterpret_cast<LOCATION_INFO *>(p);
    if (loc->dwTotalSize < sizeof(LOCATION_INFO))
    {
        HeapFree(GetProcessHeap(), 0, p);
        return E_FAIL;
    }

    /* if there's valid local volume information, load it */
    if (loc->dwVolTableOfs &&
        ((loc->dwVolTableOfs + sizeof(LOCAL_VOLUME_INFO)) <= loc->dwTotalSize))
    {
        LOCAL_VOLUME_INFO *volume_info;

        volume_info = (LOCAL_VOLUME_INFO*) &p[loc->dwVolTableOfs];
        Stream_LoadVolume(volume_info, volume);
    }

    /* if there's a local path, load it */
    DWORD n = loc->dwLocalPathOfs;
    if (n && n < loc->dwTotalSize)
        *path = Stream_LoadPath(&p[n], loc->dwTotalSize - n);

    TRACE("type %d serial %08x name %s path %s\n", volume->type,
          volume->serial, debugstr_w(volume->label), debugstr_w(*path));

    HeapFree(GetProcessHeap(), 0, p);
    return S_OK;
}

/*
 *  The format of the advertised shortcut info seems to be:
 *
 *  Offset     Description
 *  ------     -----------
 *
 *    0          Length of the block (4 bytes, usually 0x314)
 *    4          tag (dword)
 *    8          string data in ASCII
 *    8+0x104    string data in UNICODE
 *
 * In the original Win32 implementation the buffers are not initialized
 *  to zero, so data trailing the string is random garbage.
 */
static HRESULT Stream_LoadAdvertiseInfo(IStream* stm, LPWSTR *str)
{
    TRACE("%p\n", stm);

    ULONG count;
    EXP_DARWIN_LINK buffer;
    HRESULT hr = stm->Read(&buffer.dbh.cbSize, sizeof (DWORD), &count);
    if (FAILED(hr))
        return hr;

    /* make sure that we read the size of the structure even on error */
    DWORD size = sizeof buffer - sizeof (DWORD);
    if (buffer.dbh.cbSize != sizeof buffer)
    {
        ERR("Ooops.  This structure is not as expected...\n");
        return E_FAIL;
    }

    hr = stm->Read(&buffer.dbh.dwSignature, size, &count);
    if (FAILED(hr))
        return hr;

    if (count != size)
        return E_FAIL;

    TRACE("magic %08x  string = %s\n", buffer.dbh.dwSignature, debugstr_w(buffer.szwDarwinID));

    if ((buffer.dbh.dwSignature & 0xffff0000) != 0xa0000000)
    {
        ERR("Unknown magic number %08x in advertised shortcut\n", buffer.dbh.dwSignature);
        return E_FAIL;
    }

    *str = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                             (wcslen(buffer.szwDarwinID) + 1) * sizeof(WCHAR));
    wcscpy(*str, buffer.szwDarwinID);

    return S_OK;
}

/************************************************************************
 * IPersistStream_Load (IPersistStream)
 */
HRESULT WINAPI CShellLink::Load(IStream *stm)
{
    TRACE("%p %p\n", this, stm);

    if (!stm)
        return STG_E_INVALIDPOINTER;

    SHELL_LINK_HEADER ShlLnkHeader;
    ULONG dwBytesRead = 0;
    HRESULT hr = stm->Read(&ShlLnkHeader, sizeof(ShlLnkHeader), &dwBytesRead);
    if (FAILED(hr))
        return hr;

    if (dwBytesRead != sizeof(ShlLnkHeader))
        return E_FAIL;
    if (ShlLnkHeader.dwSize != sizeof(ShlLnkHeader))
        return E_FAIL;
    if (!IsEqualIID(ShlLnkHeader.clsid, CLSID_ShellLink))
        return E_FAIL;

    /* free all the old stuff */
    ILFree(pPidl);
    pPidl = NULL;
    memset(&volume, 0, sizeof volume);
    HeapFree(GetProcessHeap(), 0, sPath);
    sPath = NULL;
    HeapFree(GetProcessHeap(), 0, sDescription);
    sDescription = NULL;
    HeapFree(GetProcessHeap(), 0, sPathRel);
    sPathRel = NULL;
    HeapFree(GetProcessHeap(), 0, sWorkDir);
    sWorkDir = NULL;
    HeapFree(GetProcessHeap(), 0, sArgs);
    sArgs = NULL;
    HeapFree(GetProcessHeap(), 0, sIcoPath);
    sIcoPath = NULL;
    HeapFree(GetProcessHeap(), 0, sProduct);
    sProduct = NULL;
    HeapFree(GetProcessHeap(), 0, sComponent);
    sComponent = NULL;

    BOOL unicode = FALSE;
    iShowCmd = ShlLnkHeader.nShowCommand;
    wHotKey = ShlLnkHeader.wHotKey;
    iIcoNdx = ShlLnkHeader.nIconIndex;
    FileTimeToSystemTime (&ShlLnkHeader.ftCreationTime, &time1);
    FileTimeToSystemTime (&ShlLnkHeader.ftLastAccessTime, &time2);
    FileTimeToSystemTime (&ShlLnkHeader.ftLastWriteTime, &time3);
    if (TRACE_ON(shell))
    {
        WCHAR sTemp[MAX_PATH];
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time1,
                       NULL, sTemp, sizeof(sTemp) / sizeof(*sTemp));
        TRACE("-- time1: %s\n", debugstr_w(sTemp));
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time2,
                       NULL, sTemp, sizeof(sTemp) / sizeof(*sTemp));
        TRACE("-- time2: %s\n", debugstr_w(sTemp));
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time3,
                       NULL, sTemp, sizeof(sTemp) / sizeof(*sTemp));
        TRACE("-- time3: %s\n", debugstr_w(sTemp));
    }

    /* load all the new stuff */
    if (ShlLnkHeader.dwFlags & SLDF_HAS_ID_LIST)
    {
        hr = ILLoadFromStream(stm, &pPidl);
        if (FAILED(hr))
            return hr;
    }
    pdump(pPidl);

    /* load the location information */
    if (ShlLnkHeader.dwFlags & SLDF_HAS_LINK_INFO)
        hr = Stream_LoadLocation(stm, &volume, &sPath);
    if (FAILED(hr))
        goto end;

    if (ShlLnkHeader.dwFlags & SLDF_UNICODE)
        unicode = TRUE;

    if (ShlLnkHeader.dwFlags & SLDF_HAS_NAME)
    {
        hr = Stream_LoadString(stm, unicode, &sDescription);
        TRACE("Description  -> %s\n", debugstr_w(sDescription));
    }
    if (FAILED(hr))
        goto end;

    if (ShlLnkHeader.dwFlags & SLDF_HAS_RELPATH)
    {
        hr = Stream_LoadString(stm, unicode, &sPathRel);
        TRACE("Relative Path-> %s\n", debugstr_w(sPathRel));
    }
    if (FAILED(hr))
        goto end;

    if (ShlLnkHeader.dwFlags & SLDF_HAS_WORKINGDIR)
    {
        hr = Stream_LoadString(stm, unicode, &sWorkDir);
        PathRemoveBackslash(sWorkDir);
        TRACE("Working Dir  -> %s\n", debugstr_w(sWorkDir));
    }
    if (FAILED(hr))
        goto end;

    if (ShlLnkHeader.dwFlags & SLDF_HAS_ARGS)
    {
        hr = Stream_LoadString(stm, unicode, &sArgs);
        TRACE("Arguments    -> %s\n", debugstr_w(sArgs));
    }
    if (FAILED(hr))
        goto end;

    if (ShlLnkHeader.dwFlags & SLDF_HAS_ICONLOCATION)
    {
        hr = Stream_LoadString(stm, unicode, &sIcoPath);
        TRACE("Icon file    -> %s\n", debugstr_w(sIcoPath));
    }
    if (FAILED(hr))
        goto end;

#if (NTDDI_VERSION < NTDDI_LONGHORN)
    if (ShlLnkHeader.dwFlags & SLDF_HAS_LOGO3ID)
    {
        hr = Stream_LoadAdvertiseInfo(stm, &sProduct);
        TRACE("Product      -> %s\n", debugstr_w(sProduct));
    }
    if (FAILED(hr))
        goto end;
#endif

    if (ShlLnkHeader.dwFlags & SLDF_HAS_DARWINID)
    {
        hr = Stream_LoadAdvertiseInfo(stm, &sComponent);
        TRACE("Component    -> %s\n", debugstr_w(sComponent));
    }
    if (ShlLnkHeader.dwFlags & SLDF_RUNAS_USER)
    {
        bRunAs = TRUE;
    }
    else
    {
        bRunAs = FALSE;
    }

    if (FAILED(hr))
        goto end;

    DWORD dwZero;
    hr = stm->Read(&dwZero, sizeof(dwZero), &dwBytesRead);
    if (FAILED(hr) || dwZero || dwBytesRead != sizeof(dwZero))
        ERR("Last word was not zero\n");

    TRACE("OK\n");

    pdump (pPidl);

    return S_OK;

end:
    return hr;
}

/************************************************************************
 * Stream_WriteString
 *
 * Helper function for IPersistStream_Save. Writes a unicode string
 *  with terminating nul byte to a stream, preceded by the its length.
 */
static HRESULT Stream_WriteString(IStream* stm, LPCWSTR str)
{
    USHORT len = wcslen(str) + 1;
    DWORD count;

    HRESULT hr = stm->Write(&len, sizeof(len), &count);
    if (FAILED(hr))
        return hr;

    len *= sizeof(WCHAR);

    hr = stm->Write(str, len, &count);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

/************************************************************************
 * Stream_WriteLocationInfo
 *
 * Writes the location info to a stream
 *
 * FIXME: One day we might want to write the network volume information
 *        and the final path.
 *        Figure out how Windows deals with unicode paths here.
 */
static HRESULT Stream_WriteLocationInfo(IStream* stm, LPCWSTR path,
        CShellLink::volume_info *volume)
{
    LOCAL_VOLUME_INFO *vol;
    LOCATION_INFO *loc;

    TRACE("%p %s %p\n", stm, debugstr_w(path), volume);

    /* figure out the size of everything */
    DWORD label_size = WideCharToMultiByte(CP_ACP, 0, volume->label, -1,
                                      NULL, 0, NULL, NULL);
    DWORD path_size = WideCharToMultiByte(CP_ACP, 0, path, -1,
                                     NULL, 0, NULL, NULL);
    DWORD volume_info_size = sizeof(*vol) + label_size;
    DWORD final_path_size = 1;
    DWORD total_size = sizeof(*loc) + volume_info_size + path_size + final_path_size;

    /* create pointers to everything */
    loc = static_cast<LOCATION_INFO *>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, total_size));
    vol = (LOCAL_VOLUME_INFO*) &loc[1];
    LPSTR szLabel = (LPSTR) &vol[1];
    LPSTR szPath = &szLabel[label_size];
    LPSTR szFinalPath = &szPath[path_size];

    /* fill in the location information header */
    loc->dwTotalSize = total_size;
    loc->dwHeaderSize = sizeof (*loc);
    loc->dwFlags = 1;
    loc->dwVolTableOfs = sizeof (*loc);
    loc->dwLocalPathOfs = sizeof (*loc) + volume_info_size;
    loc->dwNetworkVolTableOfs = 0;
    loc->dwFinalPathOfs = sizeof (*loc) + volume_info_size + path_size;

    /* fill in the volume information */
    vol->dwSize = volume_info_size;
    vol->dwType = volume->type;
    vol->dwVolSerial = volume->serial;
    vol->dwVolLabelOfs = sizeof (*vol);

    /* copy in the strings */
    WideCharToMultiByte(CP_ACP, 0, volume->label, -1,
                         szLabel, label_size, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, path, -1,
                         szPath, path_size, NULL, NULL);
    szFinalPath[0] = 0;

    ULONG count = 0;
    HRESULT hr = stm->Write(loc, total_size, &count);
    HeapFree(GetProcessHeap(), 0, loc);

    return hr;
}

static EXP_DARWIN_LINK* shelllink_build_darwinid(LPCWSTR string, DWORD magic)
{
    EXP_DARWIN_LINK *buffer = (EXP_DARWIN_LINK *)LocalAlloc(LMEM_ZEROINIT, sizeof * buffer);
    buffer->dbh.cbSize = sizeof * buffer;
    buffer->dbh.dwSignature = magic;
    lstrcpynW(buffer->szwDarwinID, string, MAX_PATH);
    WideCharToMultiByte(CP_ACP, 0, string, -1, buffer->szDarwinID, MAX_PATH, NULL, NULL);

    return buffer;
}

static HRESULT Stream_WriteAdvertiseInfo(IStream* stm, LPCWSTR string, DWORD magic)
{
    TRACE("%p\n", stm);

    EXP_DARWIN_LINK *buffer = shelllink_build_darwinid(string, magic);

    ULONG count;
    return stm->Write(buffer, buffer->dbh.cbSize, &count);
}

/************************************************************************
 * IPersistStream_Save (IPersistStream)
 *
 * FIXME: makes assumptions about byte order
 */
HRESULT WINAPI CShellLink::Save(IStream *stm, BOOL fClearDirty)
{
    TRACE("%p %p %x\n", this, stm, fClearDirty);

    SHELL_LINK_HEADER ShlLnkHeader;
    memset(&ShlLnkHeader, 0, sizeof(ShlLnkHeader));
    ShlLnkHeader.dwSize = sizeof(ShlLnkHeader);
    ShlLnkHeader.nShowCommand = iShowCmd;
    ShlLnkHeader.clsid = CLSID_ShellLink;

    ShlLnkHeader.wHotKey = wHotKey;
    ShlLnkHeader.nIconIndex = iIcoNdx;
    ShlLnkHeader.dwFlags = SLDF_UNICODE;   /* strings are in unicode */
    if (pPidl)
        ShlLnkHeader.dwFlags |= SLDF_HAS_ID_LIST;
    if (sPath)
        ShlLnkHeader.dwFlags |= SLDF_HAS_LINK_INFO;
    if (sDescription)
        ShlLnkHeader.dwFlags |= SLDF_HAS_NAME;
    if (sWorkDir)
        ShlLnkHeader.dwFlags |= SLDF_HAS_WORKINGDIR;
    if (sArgs)
        ShlLnkHeader.dwFlags |= SLDF_HAS_ARGS;
    if (sIcoPath)
        ShlLnkHeader.dwFlags |= SLDF_HAS_ICONLOCATION;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    if (sProduct)
        ShlLnkHeader.dwFlags |= SLDF_HAS_LOGO3ID;
#endif
    if (sComponent)
        ShlLnkHeader.dwFlags |= SLDF_HAS_DARWINID;
    if (bRunAs)
        ShlLnkHeader.dwFlags |= SLDF_RUNAS_USER;

    SystemTimeToFileTime (&time1, &ShlLnkHeader.ftCreationTime);
    SystemTimeToFileTime (&time2, &ShlLnkHeader.ftLastAccessTime);
    SystemTimeToFileTime (&time3, &ShlLnkHeader.ftLastWriteTime);

    /* write the Shortcut header */
    ULONG count;
    HRESULT hr = stm->Write(&ShlLnkHeader, sizeof(ShlLnkHeader), &count);
    if (FAILED(hr))
    {
        ERR("Write failed\n");
        return hr;
    }

    TRACE("Writing pidl\n");

    /* write the PIDL to the shortcut */
    if (pPidl)
    {
        hr = ILSaveToStream(stm, pPidl);
        if (FAILED(hr))
        {
            ERR("Failed to write PIDL\n");
            return hr;
        }
    }

    if (sPath)
        Stream_WriteLocationInfo(stm, sPath, &volume);

    if (sDescription)
        hr = Stream_WriteString(stm, sDescription);

    if (sPathRel)
        hr = Stream_WriteString(stm, sPathRel);

    if (sWorkDir)
        hr = Stream_WriteString(stm, sWorkDir);

    if (sArgs)
        hr = Stream_WriteString(stm, sArgs);

    if (sIcoPath)
        hr = Stream_WriteString(stm, sIcoPath);

    if (sProduct)
        hr = Stream_WriteAdvertiseInfo(stm, sProduct, EXP_SZ_ICON_SIG);

    if (sComponent)
        hr = Stream_WriteAdvertiseInfo(stm, sComponent, EXP_DARWIN_ID_SIG);

    /* the last field is a single zero dword */
    DWORD zero = 0;
    hr = stm->Write(&zero, sizeof zero, &count);

    return S_OK;
}

/************************************************************************
 * IPersistStream_GetSizeMax (IPersistStream)
 */
HRESULT WINAPI CShellLink::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    TRACE("(%p)\n", this);

    return E_NOTIMPL;
}

static BOOL SHELL_ExistsFileW(LPCWSTR path)
{
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(path))
        return FALSE;

    return TRUE;
}

/**************************************************************************
 *  ShellLink_UpdatePath
 *    update absolute path in sPath using relative path in sPathRel
 */
static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath)
{
    if (!path || !psPath)
        return E_INVALIDARG;

    if (!*psPath && sPathRel)
    {
        WCHAR buffer[2*MAX_PATH], abs_path[2*MAX_PATH];
        LPWSTR final = NULL;

        /* first try if [directory of link file] + [relative path] finds an existing file */

        GetFullPathNameW(path, MAX_PATH * 2, buffer, &final);
        if (!final)
            final = buffer;
        wcscpy(final, sPathRel);

        *abs_path = '\0';

        if (SHELL_ExistsFileW(buffer))
        {
            if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
                wcscpy(abs_path, buffer);
        }
        else
        {
            /* try if [working directory] + [relative path] finds an existing file */
            if (sWorkDir)
            {
                wcscpy(buffer, sWorkDir);
                wcscpy(PathAddBackslashW(buffer), sPathRel);

                if (SHELL_ExistsFileW(buffer))
                    if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
                        wcscpy(abs_path, buffer);
            }
        }

        /* FIXME: This is even not enough - not all shell links can be resolved using this algorithm. */
        if (!*abs_path)
            wcscpy(abs_path, sPathRel);

        *psPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(abs_path) + 1) * sizeof(WCHAR));
        if (!*psPath)
            return E_OUTOFMEMORY;

        wcscpy(*psPath, abs_path);
    }

    return S_OK;
}

HRESULT WINAPI CShellLink::GetPath(LPSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%u)(%s)\n",
          this, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(sPath));

    if (sComponent || sProduct)
        return S_FALSE;

    if (cchMaxPath)
        pszFile[0] = 0;

    if (sPath)
        WideCharToMultiByte(CP_ACP, 0, sPath, -1,
                             pszFile, cchMaxPath, NULL, NULL);

    if (pfd) FIXME("(%p): WIN32_FIND_DATA is not yet filled.\n", this);

    return S_OK;
}

HRESULT WINAPI CShellLink::GetIDList(LPITEMIDLIST * ppidl)
{
    TRACE("(%p)->(ppidl=%p)\n", this, ppidl);

    if (!pPidl)
    {
        *ppidl = NULL;
        return S_FALSE;
    }

    *ppidl = ILClone(pPidl);
    return S_OK;
}

HRESULT WINAPI CShellLink::SetIDList(LPCITEMIDLIST pidl)
{
    TRACE("(%p)->(pidl=%p)\n", this, pidl);

    if (pPidl)
        ILFree(pPidl);

    pPidl = ILClone(pidl);
    if (!pPidl)
        return E_FAIL;

    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetDescription(LPSTR pszName, INT cchMaxName)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszName, cchMaxName);

    if (cchMaxName)
        pszName[0] = 0;

    if (sDescription)
        WideCharToMultiByte(CP_ACP, 0, sDescription, -1,
                             pszName, cchMaxName, NULL, NULL);

    return S_OK;
}

HRESULT WINAPI CShellLink::SetDescription(LPCSTR pszName)
{
    TRACE("(%p)->(pName=%s)\n", this, pszName);

    HeapFree(GetProcessHeap(), 0, sDescription);
    sDescription = NULL;

    if (pszName)
    {
        sDescription = HEAP_strdupAtoW(GetProcessHeap(), 0, pszName);
        if (!sDescription)
            return E_OUTOFMEMORY;
    }
    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetWorkingDirectory(LPSTR pszDir, INT cchMaxPath)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszDir, cchMaxPath);

    if (cchMaxPath)
        pszDir[0] = 0;

    if (sWorkDir)
        WideCharToMultiByte(CP_ACP, 0, sWorkDir, -1,
                             pszDir, cchMaxPath, NULL, NULL);

    return S_OK;
}

HRESULT WINAPI CShellLink::SetWorkingDirectory(LPCSTR pszDir)
{
    TRACE("(%p)->(dir=%s)\n", this, pszDir);

    HeapFree(GetProcessHeap(), 0, sWorkDir);
    sWorkDir = NULL;

    if (pszDir)
    {
        sWorkDir = HEAP_strdupAtoW(GetProcessHeap(), 0, pszDir);
        if (!sWorkDir)
            return E_OUTOFMEMORY;
    }
    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetArguments(LPSTR pszArgs, INT cchMaxPath)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszArgs, cchMaxPath);

    if (cchMaxPath)
        pszArgs[0] = 0;
    if (sArgs)
        WideCharToMultiByte(CP_ACP, 0, sArgs, -1,
                             pszArgs, cchMaxPath, NULL, NULL);

    return S_OK;
}

HRESULT WINAPI CShellLink::SetArguments(LPCSTR pszArgs)
{
    TRACE("(%p)->(args=%s)\n", this, pszArgs);

    HeapFree(GetProcessHeap(), 0, sArgs);
    sArgs = NULL;

    if (pszArgs)
    {
        sArgs = HEAP_strdupAtoW(GetProcessHeap(), 0, pszArgs);
        if (!sArgs)
            return E_OUTOFMEMORY;
    }

    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetHotkey(WORD *pwHotkey)
{
    TRACE("(%p)->(%p)(0x%08x)\n", this, pwHotkey, wHotKey);

    *pwHotkey = wHotKey;

    return S_OK;
}

HRESULT WINAPI CShellLink::SetHotkey(WORD wHotkey)
{
    TRACE("(%p)->(hotkey=%x)\n", this, wHotkey);

    wHotKey = wHotkey;
    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetShowCmd(INT *piShowCmd)
{
    TRACE("(%p)->(%p)\n", this, piShowCmd);
    *piShowCmd = iShowCmd;
    return S_OK;
}

HRESULT WINAPI CShellLink::SetShowCmd(INT iShowCmd)
{
    TRACE("(%p) %d\n", this, iShowCmd);

    this->iShowCmd = iShowCmd;
    bDirty = TRUE;

    return S_OK;
}

static HRESULT SHELL_PidlGeticonLocationA(IShellFolder* psf, LPCITEMIDLIST pidl,
        LPSTR pszIconPath, int cchIconPath, int* piIcon)
{
    LPCITEMIDLIST pidlLast;

    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);

    if (SUCCEEDED(hr))
    {
        CComPtr<IExtractIconA> pei;

        hr = psf->GetUIObjectOf(0, 1, &pidlLast, IID_NULL_PPV_ARG(IExtractIconA, &pei));

        if (SUCCEEDED(hr))
            hr = pei->GetIconLocation(0, pszIconPath, MAX_PATH, piIcon, NULL);

        psf->Release();
    }

    return hr;
}

HRESULT WINAPI CShellLink::GetIconLocation(LPSTR pszIconPath, INT cchIconPath, INT *piIcon)
{
    TRACE("(%p)->(%p len=%u iicon=%p)\n", this, pszIconPath, cchIconPath, piIcon);

    pszIconPath[0] = 0;
    *piIcon = iIcoNdx;

    if (sIcoPath)
    {
        WideCharToMultiByte(CP_ACP, 0, sIcoPath, -1, pszIconPath, cchIconPath, NULL, NULL);
        return S_OK;
    }

    if (pPidl || sPath)
    {
        CComPtr<IShellFolder> pdsk;

        HRESULT hr = SHGetDesktopFolder(&pdsk);

        if (SUCCEEDED(hr))
        {
            /* first look for an icon using the PIDL (if present) */
            if (pPidl)
                hr = SHELL_PidlGeticonLocationA(pdsk, pPidl, pszIconPath, cchIconPath, piIcon);
            else
                hr = E_FAIL;

            /* if we couldn't find an icon yet, look for it using the file system path */
            if (FAILED(hr) && sPath)
            {
                LPITEMIDLIST pidl;

                hr = pdsk->ParseDisplayName(0, NULL, sPath, NULL, &pidl, NULL);

                if (SUCCEEDED(hr))
                {
                    hr = SHELL_PidlGeticonLocationA(pdsk, pidl, pszIconPath, cchIconPath, piIcon);

                    SHFree(pidl);
                }
            }
        }

        return hr;
    }
    return S_OK;
}

HRESULT WINAPI CShellLink::SetIconLocation(LPCSTR pszIconPath, INT iIcon)
{
    TRACE("(%p)->(path=%s iicon=%u)\n", this, pszIconPath, iIcon);

    HeapFree(GetProcessHeap(), 0, sIcoPath);
    sIcoPath = NULL;

    if (pszIconPath)
    {
        sIcoPath = HEAP_strdupAtoW(GetProcessHeap(), 0, pszIconPath);
        if (!sIcoPath)
            return E_OUTOFMEMORY;
    }

    iIcoNdx = iIcon;
    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::SetRelativePath(LPCSTR pszPathRel, DWORD dwReserved)
{
    TRACE("(%p)->(path=%s %x)\n", this, pszPathRel, dwReserved);

    HeapFree(GetProcessHeap(), 0, sPathRel);
    sPathRel = NULL;

    if (pszPathRel)
    {
        sPathRel = HEAP_strdupAtoW(GetProcessHeap(), 0, pszPathRel);
        bDirty = TRUE;
    }

    return ShellLink_UpdatePath(sPathRel, sPath, sWorkDir, &sPath);
}

HRESULT WINAPI CShellLink::Resolve(HWND hwnd, DWORD fFlags)
{
    HRESULT hr = S_OK;
    BOOL bSuccess;

    TRACE("(%p)->(hwnd=%p flags=%x)\n", this, hwnd, fFlags);

    /*FIXME: use IResolveShellLink interface */

    if (!sPath && pPidl)
    {
        WCHAR buffer[MAX_PATH];

        bSuccess = SHGetPathFromIDListW(pPidl, buffer);

        if (bSuccess && *buffer)
        {
            sPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(buffer) + 1) * sizeof(WCHAR));

            if (!sPath)
                return E_OUTOFMEMORY;

            wcscpy(sPath, buffer);

            bDirty = TRUE;
        }
        else
            hr = S_OK;    /* don't report an error occurred while just caching information */
    }

    if (!sIcoPath && sPath)
    {
        sIcoPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(sPath) + 1) * sizeof(WCHAR));

        if (!sIcoPath)
            return E_OUTOFMEMORY;

        wcscpy(sIcoPath, sPath);
        iIcoNdx = 0;

        bDirty = TRUE;
    }

    return hr;
}

HRESULT WINAPI CShellLink::SetPath(LPCSTR pszFile)
{
    TRACE("(%p)->(path=%s)\n", this, pszFile);
    if (pszFile == NULL)
        return E_INVALIDARG;

    LPWSTR str = HEAP_strdupAtoW(GetProcessHeap(), 0, pszFile);
    if (!str)
        return E_OUTOFMEMORY;

    HRESULT hr = SetPath(str);
    HeapFree(GetProcessHeap(), 0, str);

    return hr;
}

HRESULT WINAPI CShellLink::GetPath(LPWSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags)
{
    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%u)(%s)\n",
          this, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(sPath));

    if (sComponent || sProduct)
        return S_FALSE;

    if (cchMaxPath)
        pszFile[0] = 0;

    if (sPath)
        lstrcpynW(pszFile, sPath, cchMaxPath);

    if (pfd) FIXME("(%p): WIN32_FIND_DATA is not yet filled.\n", this);

    return S_OK;
}

HRESULT WINAPI CShellLink::GetDescription(LPWSTR pszName, INT cchMaxName)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszName, cchMaxName);

    pszName[0] = 0;
    if (sDescription)
        lstrcpynW(pszName, sDescription, cchMaxName);

    return S_OK;
}

HRESULT WINAPI CShellLink::SetDescription(LPCWSTR pszName)
{
    TRACE("(%p)->(desc=%s)\n", this, debugstr_w(pszName));

    HeapFree(GetProcessHeap(), 0, sDescription);
    sDescription = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                     (wcslen(pszName) + 1) * sizeof(WCHAR));
    if (!sDescription)
        return E_OUTOFMEMORY;

    wcscpy(sDescription, pszName);
    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetWorkingDirectory(LPWSTR pszDir, INT cchMaxPath)
{
    TRACE("(%p)->(%p len %u)\n", this, pszDir, cchMaxPath);

    if (cchMaxPath)
        pszDir[0] = 0;
    if (sWorkDir)
        lstrcpynW(pszDir, sWorkDir, cchMaxPath);

    return S_OK;
}

HRESULT WINAPI CShellLink::SetWorkingDirectory(LPCWSTR pszDir)
{
    TRACE("(%p)->(dir=%s)\n", this, debugstr_w(pszDir));

    HeapFree(GetProcessHeap(), 0, sWorkDir);
    sWorkDir = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                 (wcslen(pszDir) + 1) * sizeof (WCHAR));
    if (!sWorkDir)
        return E_OUTOFMEMORY;
    wcscpy(sWorkDir, pszDir);
    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetArguments(LPWSTR pszArgs, INT cchMaxPath)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszArgs, cchMaxPath);

    if (cchMaxPath)
        pszArgs[0] = 0;
    if (sArgs)
        lstrcpynW(pszArgs, sArgs, cchMaxPath);

    return S_OK;
}

HRESULT WINAPI CShellLink::SetArguments(LPCWSTR pszArgs)
{
    TRACE("(%p)->(args=%s)\n", this, debugstr_w(pszArgs));

    HeapFree(GetProcessHeap(), 0, sArgs);
    sArgs = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                              (wcslen(pszArgs) + 1) * sizeof (WCHAR));
    if (!sArgs)
        return E_OUTOFMEMORY;

    wcscpy(sArgs, pszArgs);
    bDirty = TRUE;

    return S_OK;
}

static HRESULT SHELL_PidlGeticonLocationW(IShellFolder* psf, LPCITEMIDLIST pidl,
        LPWSTR pszIconPath, int cchIconPath, int* piIcon)
{
    LPCITEMIDLIST pidlLast;
    UINT wFlags;

    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);

    if (SUCCEEDED(hr))
    {
        CComPtr<IExtractIconW> pei;

        hr = psf->GetUIObjectOf(0, 1, &pidlLast, IID_NULL_PPV_ARG(IExtractIconW, &pei));

        if (SUCCEEDED(hr))
            hr = pei->GetIconLocation(0, pszIconPath, MAX_PATH, piIcon, &wFlags);

        psf->Release();
    }

    return hr;
}

HRESULT WINAPI CShellLink::GetIconLocation(LPWSTR pszIconPath, INT cchIconPath, INT *piIcon)
{
    TRACE("(%p)->(%p len=%u iicon=%p)\n", this, pszIconPath, cchIconPath, piIcon);

    pszIconPath[0] = 0;
    *piIcon = iIcoNdx;

    if (sIcoPath)
    {
        lstrcpynW(pszIconPath, sIcoPath, cchIconPath);
        return S_OK;
    }

    if (pPidl || sPath)
    {
        CComPtr<IShellFolder>        pdsk;

        HRESULT hr = SHGetDesktopFolder(&pdsk);

        if (SUCCEEDED(hr))
        {
            /* first look for an icon using the PIDL (if present) */
            if (pPidl)
                hr = SHELL_PidlGeticonLocationW(pdsk, pPidl, pszIconPath, cchIconPath, piIcon);
            else
                hr = E_FAIL;

            /* if we couldn't find an icon yet, look for it using the file system path */
            if (FAILED(hr) && sPath)
            {
                LPITEMIDLIST pidl;

                hr = pdsk->ParseDisplayName(0, NULL, sPath, NULL, &pidl, NULL);

                if (SUCCEEDED(hr))
                {
                    hr = SHELL_PidlGeticonLocationW(pdsk, pidl, pszIconPath, cchIconPath, piIcon);

                    SHFree(pidl);
                }
            }
        }
        return hr;
    }
    return S_OK;
}

HRESULT WINAPI CShellLink::SetIconLocation(LPCWSTR pszIconPath, INT iIcon)
{
    TRACE("(%p)->(path=%s iicon=%u)\n", this, debugstr_w(pszIconPath), iIcon);

    HeapFree(GetProcessHeap(), 0, sIcoPath);
    sIcoPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                  (wcslen(pszIconPath) + 1) * sizeof (WCHAR));
    if (!sIcoPath)
        return E_OUTOFMEMORY;
    wcscpy(sIcoPath, pszIconPath);

    iIcoNdx = iIcon;
    bDirty = TRUE;

    return S_OK;
}

HRESULT WINAPI CShellLink::SetRelativePath(LPCWSTR pszPathRel, DWORD dwReserved)
{
    TRACE("(%p)->(path=%s %x)\n", this, debugstr_w(pszPathRel), dwReserved);

    HeapFree(GetProcessHeap(), 0, sPathRel);
    sPathRel = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                  (wcslen(pszPathRel) + 1) * sizeof (WCHAR));
    if (!sPathRel)
        return E_OUTOFMEMORY;
    wcscpy(sPathRel, pszPathRel);
    bDirty = TRUE;

    return ShellLink_UpdatePath(sPathRel, sPath, sWorkDir, &sPath);
}

LPWSTR CShellLink::ShellLink_GetAdvertisedArg(LPCWSTR str)
{
    if (!str)
        return NULL;

    LPCWSTR p = wcschr(str, L':');
    if (!p)
        return NULL;
    DWORD len = p - str;
    LPWSTR ret = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (len + 1));
    if (!ret)
        return ret;
    memcpy(ret, str, sizeof(WCHAR)*len);
    ret[len] = 0;
    return ret;
}

HRESULT CShellLink::ShellLink_SetAdvertiseInfo(LPCWSTR str)
{
    LPCWSTR szComponent = NULL, szProduct = NULL;

    while (str[0])
    {
        /* each segment must start with two colons */
        if (str[0] != ':' || str[1] != ':')
            return E_FAIL;

        /* the last segment is just two colons */
        if (!str[2])
            break;
        str += 2;

        /* there must be a colon straight after a guid */
        LPCWSTR p = wcschr(str, L':');
        if (!p)
            return E_FAIL;
        INT len = p - str;
        if (len != 38)
            return E_FAIL;

        /* get the guid, and check it's validly formatted */
        WCHAR szGuid[39];
        memcpy(szGuid, str, sizeof(WCHAR)*len);
        szGuid[len] = 0;

        GUID guid;
        HRESULT hr = CLSIDFromString(szGuid, &guid);
        if (hr != S_OK)
            return hr;
        str = p + 1;

        /* match it up to a guid that we care about */
        if (IsEqualGUID(guid, SHELL32_AdvtShortcutComponent) && !szComponent)
            szComponent = str;
        else if (IsEqualGUID(guid, SHELL32_AdvtShortcutProduct) && !szProduct)
            szProduct = str;
        else
            return E_FAIL;

        /* skip to the next field */
        str = wcschr(str, L':');
        if (!str)
            return E_FAIL;
    }

    /* we have to have a component for an advertised shortcut */
    if (!szComponent)
        return E_FAIL;

    sComponent = ShellLink_GetAdvertisedArg(szComponent);
    sProduct = ShellLink_GetAdvertisedArg(szProduct);

    TRACE("Component = %s\n", debugstr_w(sComponent));
    TRACE("Product = %s\n", debugstr_w(sProduct));

    return S_OK;
}

static BOOL ShellLink_GetVolumeInfo(LPCWSTR path, CShellLink::volume_info *volume)
{
    WCHAR drive[4] = { path[0], ':', '\\', 0 };

    volume->type = GetDriveTypeW(drive);
    BOOL bRet = GetVolumeInformationW(drive, volume->label, _countof(volume->label), &volume->serial, NULL, NULL, NULL, 0);
    TRACE("ret = %d type %d serial %08x name %s\n", bRet,
          volume->type, volume->serial, debugstr_w(volume->label));
    return bRet;
}

HRESULT WINAPI CShellLink::SetPath(LPCWSTR pszFile)
{
    LPWSTR unquoted = NULL;
    HRESULT hr = S_OK;

    TRACE("(%p)->(path=%s)\n", this, debugstr_w(pszFile));

    if (!pszFile)
        return E_INVALIDARG;

    /* quotes at the ends of the string are stripped */
    UINT len = wcslen(pszFile);
    if (pszFile[0] == '"' && pszFile[len-1] == '"')
    {
        unquoted = strdupW(pszFile);
        PathUnquoteSpacesW(unquoted);
        pszFile = unquoted;
    }

    /* any other quote marks are invalid */
    if (wcschr(pszFile, '"'))
    {
        HeapFree(GetProcessHeap(), 0, unquoted);
        return S_FALSE;
    }

    HeapFree(GetProcessHeap(), 0, sPath);
    sPath = NULL;

    HeapFree(GetProcessHeap(), 0, sComponent);
    sComponent = NULL;

    if (pPidl)
        ILFree(pPidl);
    pPidl = NULL;

    if (S_OK != ShellLink_SetAdvertiseInfo(pszFile))
    {
        WCHAR buffer[MAX_PATH];
        LPWSTR fname;

        if (*pszFile == '\0')
            *buffer = '\0';
        else if (!GetFullPathNameW(pszFile, MAX_PATH, buffer, &fname))
            return E_FAIL;
        else if(!PathFileExistsW(buffer) &&
                !SearchPathW(NULL, pszFile, NULL, MAX_PATH, buffer, NULL))
            hr = S_FALSE;

        pPidl = SHSimpleIDListFromPathW(pszFile);
        ShellLink_GetVolumeInfo(buffer, &volume);

        sPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                  (wcslen(buffer) + 1) * sizeof (WCHAR));
        if (!sPath)
            return E_OUTOFMEMORY;

        wcscpy(sPath, buffer);
    }

    bDirty = TRUE;
    HeapFree(GetProcessHeap(), 0, unquoted);

    return hr;
}

HRESULT WINAPI CShellLink::AddDataBlock(void* pDataBlock)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CShellLink::CopyDataBlock(DWORD dwSig, void** ppDataBlock)
{
    LPVOID block = NULL;
    HRESULT hr = E_FAIL;

    TRACE("%p %08x %p\n", this, dwSig, ppDataBlock);

    switch (dwSig)
    {
        case EXP_DARWIN_ID_SIG:
            if (!sComponent)
                break;
            block = shelllink_build_darwinid(sComponent, dwSig);
            hr = S_OK;
            break;
        case EXP_SZ_LINK_SIG:
        case NT_CONSOLE_PROPS_SIG:
        case NT_FE_CONSOLE_PROPS_SIG:
        case EXP_SPECIAL_FOLDER_SIG:
        case EXP_SZ_ICON_SIG:
            FIXME("valid but unhandled datablock %08x\n", dwSig);
            break;
        default:
            ERR("unknown datablock %08x\n", dwSig);
    }
    *ppDataBlock = block;
    return hr;
}

HRESULT WINAPI CShellLink::RemoveDataBlock(DWORD dwSig)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CShellLink::GetFlags(DWORD *pdwFlags)
{
    DWORD flags = 0;

    FIXME("%p %p\n", this, pdwFlags);

    /* FIXME: add more */
    if (sArgs)
        flags |= SLDF_HAS_ARGS;
    if (sComponent)
        flags |= SLDF_HAS_DARWINID;
    if (sIcoPath)
        flags |= SLDF_HAS_ICONLOCATION;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    if (sProduct)
        flags |= SLDF_HAS_LOGO3ID;
#endif
    if (pPidl)
        flags |= SLDF_HAS_ID_LIST;

    *pdwFlags = flags;

    return S_OK;
}

HRESULT WINAPI CShellLink::SetFlags(DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/**************************************************************************
 * CShellLink implementation of IShellExtInit::Initialize()
 *
 * Loads the shelllink from the dataobject the shell is pointing to.
 */
HRESULT WINAPI CShellLink::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    TRACE("%p %p %p %p\n", this, pidlFolder, pdtobj, hkeyProgID);

    if (!pdtobj)
        return E_FAIL;

    FORMATETC format;
    format.cfFormat = CF_HDROP;
    format.ptd = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex = -1;
    format.tymed = TYMED_HGLOBAL;

    STGMEDIUM stgm;
    HRESULT hr = pdtobj->GetData(&format, &stgm);
    if (FAILED(hr))
        return hr;

    UINT count = DragQueryFileW((HDROP)stgm.hGlobal, -1, NULL, 0);
    if (count == 1)
    {
        count = DragQueryFileW((HDROP)stgm.hGlobal, 0, NULL, 0);
        count++;
        LPWSTR path = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
        if (path)
        {
            count = DragQueryFileW((HDROP)stgm.hGlobal, 0, path, count);
            hr = Load(path, 0);
            HeapFree(GetProcessHeap(), 0, path);
        }
    }
    ReleaseStgMedium(&stgm);

    return S_OK;
}

HRESULT WINAPI CShellLink::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int id = 1;

    TRACE("%p %p %u %u %u %u\n", this,
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (!hMenu)
        return E_INVALIDARG;

    WCHAR wszOpen[20];
    if (!LoadStringW(shell32_hInstance, IDS_OPEN_VERB, wszOpen, _countof(wszOpen)))
        wszOpen[0] = L'\0';

    MENUITEMINFOW mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof (mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.dwTypeData = wszOpen;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmdFirst + id++;
    mii.fState = MFS_DEFAULT | MFS_ENABLED;
    mii.fType = MFT_STRING;
    if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
        return E_FAIL;
    iIdOpen = 1;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, id);
}

static LPWSTR
shelllink_get_msi_component_path(LPWSTR component)
{
    DWORD Result, sz = 0;

    Result = CommandLineFromMsiDescriptor(component, NULL, &sz);
    if (Result != ERROR_SUCCESS)
        return NULL;

    sz++;
    LPWSTR path = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR));
    Result = CommandLineFromMsiDescriptor(component, path, &sz);
    if (Result != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, path);
        path = NULL;
    }

    TRACE("returning %s\n", debugstr_w(path));

    return path;
}

HRESULT WINAPI CShellLink::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HWND hwnd = NULL; /* FIXME: get using interface set from IObjectWithSite */
    LPWSTR args = NULL;
    LPWSTR path = NULL;

    TRACE("%p %p\n", this, lpici);

    if (lpici->cbSize < sizeof (CMINVOKECOMMANDINFO))
        return E_INVALIDARG;

    HRESULT hr = Resolve(hwnd, 0);
    if (FAILED(hr))
    {
        TRACE("failed to resolve component with error 0x%08x", hr);
        return hr;
    }
    if (sComponent)
    {
        path = shelllink_get_msi_component_path(sComponent);
        if (!path)
            return E_FAIL;
    }
    else
        path = strdupW(sPath);

    if ( lpici->cbSize == sizeof(CMINVOKECOMMANDINFOEX) &&
        (lpici->fMask & CMIC_MASK_UNICODE) )
    {
        LPCMINVOKECOMMANDINFOEX iciex = (LPCMINVOKECOMMANDINFOEX)lpici;
        DWORD len = 2;

        if (sArgs)
            len += wcslen(sArgs);
        if (iciex->lpParametersW)
            len += wcslen(iciex->lpParametersW);

        args = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        args[0] = 0;
        if (sArgs)
            wcscat(args, sArgs);
        if (iciex->lpParametersW)
        {
            wcscat(args, L" ");
            wcscat(args, iciex->lpParametersW);
        }
    }
    else if (sArgs != NULL)
    {
        args = strdupW(sArgs);
    }

    SHELLEXECUTEINFOW sei;
    memset(&sei, 0, sizeof sei);
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_HASLINKNAME | SEE_MASK_UNICODE |
               (lpici->fMask & (SEE_MASK_NOASYNC | SEE_MASK_ASYNCOK | SEE_MASK_FLAG_NO_UI));
    sei.lpFile = path;
    sei.lpClass = sLinkPath;
    sei.nShow = iShowCmd;
    sei.lpDirectory = sWorkDir;
    sei.lpParameters = args;
    sei.lpVerb = L"open";

    // HACK for ShellExecuteExW
    if (wcsstr(sPath, L".cpl"))
        sei.lpVerb = L"cplopen";

    if (ShellExecuteExW(&sei))
        hr = S_OK;
    else
        hr = E_FAIL;

    HeapFree(GetProcessHeap(), 0, args);
    HeapFree(GetProcessHeap(), 0, path);

    return hr;
}

HRESULT WINAPI CShellLink::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this, idCmd, uType, pwReserved, pszName, cchMax);

    return E_NOTIMPL;
}

INT_PTR CALLBACK ExtendedShortcutProc(HWND hwndDlg, UINT uMsg,
                                      WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            if (lParam)
            {
                HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
                SendMessage(hDlgCtrl, BM_SETCHECK, BST_CHECKED, 0);
            }
            return TRUE;
        case WM_COMMAND:
        {
            HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
            if (LOWORD(wParam) == IDOK)
            {
                if (SendMessage(hDlgCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    EndDialog(hwndDlg, 1);
                else
                    EndDialog(hwndDlg, 0);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, -1);
            }
            else if (LOWORD(wParam) == 14000)
            {
                if (SendMessage(hDlgCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    SendMessage(hDlgCtrl, BM_SETCHECK, BST_UNCHECKED, 0);
                else
                    SendMessage(hDlgCtrl, BM_SETCHECK, BST_CHECKED, 0);
            }
        }
    }
    return FALSE;
}

/**************************************************************************
 * SH_ShellLinkDlgProc
 *
 * dialog proc of the shortcut property dialog
 */

INT_PTR CALLBACK CShellLink::SH_ShellLinkDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CShellLink *pThis = reinterpret_cast<CShellLink *>(GetWindowLongPtr(hwndDlg, DWLP_USER));

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW ppsp = (LPPROPSHEETPAGEW)lParam;
            if (ppsp == NULL)
                break;

            TRACE("ShellLink_DlgProc (WM_INITDIALOG hwnd %p lParam %p ppsplParam %x)\n", hwndDlg, lParam, ppsp->lParam);

            pThis = reinterpret_cast<CShellLink *>(ppsp->lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pThis);

            TRACE("sArgs: %S sComponent: %S sDescription: %S sIcoPath: %S sPath: %S sPathRel: %S sProduct: %S sWorkDir: %S\n", pThis->sArgs, pThis->sComponent, pThis->sDescription,
                  pThis->sIcoPath, pThis->sPath, pThis->sPathRel, pThis->sProduct, pThis->sWorkDir);

            /* Get file information */
            SHFILEINFO fi;
            if (!SHGetFileInfoW(pThis->sLinkPath, 0, &fi, sizeof(fi), SHGFI_TYPENAME|SHGFI_ICON))
            {
                ERR("SHGetFileInfoW failed for %ls (%lu)\n", pThis->sLinkPath, GetLastError());
                fi.szTypeName[0] = L'\0';
                fi.hIcon = NULL;
            }

            if (fi.hIcon) // TODO: destroy icon
                SendDlgItemMessageW(hwndDlg, 14000, STM_SETICON, (WPARAM)fi.hIcon, 0);
            else
                ERR("ExtractIconW failed %ls %u\n", pThis->sIcoPath, pThis->iIcoNdx);

            /* target type */
            if (pThis->sPath)
                SetDlgItemTextW(hwndDlg, 14005, SH_GetTargetTypeByPath(pThis->sPath));

            /* target location */
            if (pThis->sWorkDir)
                SetDlgItemTextW(hwndDlg, 14007, PathFindFileName(pThis->sWorkDir));

            /* target path */
            if (pThis->sPath)
            {
                WCHAR newpath[2*MAX_PATH] = L"\0";
                if (wcschr(pThis->sPath, ' '))
                    StringCchPrintfExW(newpath, 2*MAX_PATH, NULL, NULL, 0, L"\"%ls\"", pThis->sPath);
                else 
                    StringCchCopyExW(newpath, 2*MAX_PATH, pThis->sPath, NULL, NULL, 0);
                
                if (pThis->sArgs && pThis->sArgs[0])
                {
                    StringCchCatW(newpath, 2*MAX_PATH, L" ");
                    StringCchCatW(newpath, 2*MAX_PATH, pThis->sArgs);
                }
                SetDlgItemTextW(hwndDlg, 14009, newpath);
            }
            /* working dir */
            if (pThis->sWorkDir)
                SetDlgItemTextW(hwndDlg, 14011, pThis->sWorkDir);

            /* description */
            if (pThis->sDescription)
                SetDlgItemTextW(hwndDlg, 14019, pThis->sDescription);

            return TRUE;
        }
        case WM_NOTIFY:
        {
            LPPSHNOTIFY lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                WCHAR wszBuf[MAX_PATH];
                /* set working directory */
                GetDlgItemTextW(hwndDlg, 14011, wszBuf, MAX_PATH);
                pThis->SetWorkingDirectory(wszBuf);
                /* set link destination */
                GetDlgItemTextW(hwndDlg, 14009, wszBuf, MAX_PATH);
                LPWSTR lpszArgs = NULL;
                LPWSTR unquoted = strdupW(wszBuf);
                StrTrimW(unquoted, L" ");
                if (!PathFileExistsW(unquoted)) 
                {
                    lpszArgs = PathGetArgsW(unquoted);
                    PathRemoveArgsW(unquoted);
                    StrTrimW(lpszArgs, L" ");
                }
                if (unquoted[0] == '"' && unquoted[wcslen(unquoted)-1] == '"')
                    PathUnquoteSpacesW(unquoted);


                WCHAR *pwszExt = PathFindExtensionW(unquoted);
                if (!wcsicmp(pwszExt, L".lnk"))
                {
                    // FIXME load localized error msg
                    MessageBoxW(hwndDlg, L"You cannot create a link to a shortcut", L"Error", MB_ICONERROR);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }

                if (!PathFileExistsW(unquoted))
                {
                    //FIXME load localized error msg
                    MessageBoxW(hwndDlg, L"The specified file name in the target box is invalid", L"Error", MB_ICONERROR);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }
                
                pThis->SetPath(unquoted);
                if (lpszArgs)
                    pThis->SetArguments(lpszArgs);
                else 
                    pThis->SetArguments(L"\0");

                HeapFree(GetProcessHeap(), 0, unquoted);

                TRACE("This %p sLinkPath %S\n", pThis, pThis->sLinkPath);
                pThis->Save(pThis->sLinkPath, TRUE);
                SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);                return TRUE;
            }
            break;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case 14020:
                    ///
                    /// FIXME
                    /// open target directory
                    ///
                    return TRUE;
                case 14021:
                {
                    WCHAR wszPath[MAX_PATH] = L"";

                    if (pThis->sIcoPath)
                        wcscpy(wszPath, pThis->sIcoPath);
                    INT IconIndex = pThis->iIcoNdx;
                    if (PickIconDlg(hwndDlg, wszPath, MAX_PATH, &IconIndex))
                    {
                        pThis->SetIconLocation(wszPath, IconIndex);
                        ///
                        /// FIXME redraw icon
                    }
                    return TRUE;
                }

                case 14022:
                {
                    INT_PTR result = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_SHORTCUT_EXTENDED_PROPERTIES), hwndDlg, ExtendedShortcutProc, (LPARAM)pThis->bRunAs);
                    if (result == 1 || result == 0)
                    {
                        if (pThis->bRunAs != result)
                        {
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        }

                        pThis->bRunAs = result;
                    }
                    return TRUE;
                }
            }
            if(HIWORD(wParam) == EN_CHANGE)
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            break;
        default:
            break;
    }
    return FALSE;
}

/**************************************************************************
 * ShellLink_IShellPropSheetExt interface
 */

HRESULT WINAPI CShellLink::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hPage = SH_CreatePropertySheetPage(IDD_SHORTCUT_PROPERTIES, SH_ShellLinkDlgProc, (LPARAM)this, NULL);
    if (hPage == NULL)
    {
        ERR("failed to create property sheet page\n");
        return E_FAIL;
    }

    if (!pfnAddPage(hPage, lParam))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI CShellLink::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam)
{
    TRACE("(%p) (uPageID %u, pfnReplacePage %p lParam %p\n", this, uPageID, pfnReplacePage, lParam);
    return E_NOTIMPL;
}

HRESULT WINAPI CShellLink::SetSite(IUnknown *punk)
{
    TRACE("%p %p\n", this, punk);

    site = punk;

    return S_OK;
}

HRESULT WINAPI CShellLink::GetSite(REFIID iid, void ** ppvSite)
{
    TRACE("%p %s %p\n", this, debugstr_guid(&iid), ppvSite);

    if (site == NULL)
        return E_FAIL;

    return site->QueryInterface(iid, ppvSite);
}

HRESULT WINAPI CShellLink::DragEnter(IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);
    LPCITEMIDLIST pidlLast;
    CComPtr<IShellFolder> psf;

    HRESULT hr = SHBindToParent(pPidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);

    if (SUCCEEDED(hr))
    {
        hr = psf->GetUIObjectOf(0, 1, &pidlLast, IID_NULL_PPV_ARG(IDropTarget, &mDropTarget));

        if (SUCCEEDED(hr))
            hr = mDropTarget->DragEnter(pDataObject, dwKeyState, pt, pdwEffect);
        else 
            *pdwEffect = DROPEFFECT_NONE;
    }
    else
        *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}



HRESULT WINAPI CShellLink::DragOver(DWORD dwKeyState, POINTL pt,
    DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);
    HRESULT hr = S_OK;
    if (mDropTarget)
        hr = mDropTarget->DragOver(dwKeyState, pt, pdwEffect);
    return hr;
}

HRESULT WINAPI CShellLink::DragLeave()
{
    TRACE("(%p)\n", this);
    HRESULT hr = S_OK;
    if (mDropTarget)
    {
        hr = mDropTarget->DragLeave();
        mDropTarget->Release();
    }

    return hr;
}

HRESULT WINAPI CShellLink::Drop(IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);
    HRESULT hr = S_OK;
    if (mDropTarget)
        hr = mDropTarget->Drop(pDataObject, dwKeyState, pt, pdwEffect);

    return hr;
}

/**************************************************************************
 *      IShellLink_ConstructFromFile
 */
HRESULT WINAPI IShellLink_ConstructFromFile(IUnknown *pUnkOuter, REFIID riid, LPCITEMIDLIST pidl, LPVOID *ppv)
{
    CComPtr<IUnknown> psl;

    HRESULT hr = CShellLink::_CreatorClass::CreateInstance(NULL, riid, (void**)&psl);

    if (SUCCEEDED(hr))
    {
        CComPtr<IPersistFile> ppf;

        *ppv = NULL;

        hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));

        if (SUCCEEDED(hr))
        {
            WCHAR path[MAX_PATH];

            if (SHGetPathFromIDListW(pidl, path))
                hr = ppf->Load(path, 0);
            else
                hr = E_FAIL;

            if (SUCCEEDED(hr))
                *ppv = psl.Detach();
        }
    }

    return hr;
}
