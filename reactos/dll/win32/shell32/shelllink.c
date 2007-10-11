/*
 *
 *      Copyright 1997  Marcus Meissner
 *      Copyright 1998  Juergen Schmied
 *      Copyright 2005  Mike McCormack
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

#define COBJMACROS
#define NONAMELESSUNION

#include "wine/debug.h"
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"

#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"
#include "undocshell.h"

#include "pidl.h"
#include "shell32_main.h"
#include "shlguid.h"
#include "shlwapi.h"
#include "msi.h"
#include "appmgmt.h"
#include "prsht.h"
#include "initguid.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

DEFINE_GUID( SHELL32_AdvtShortcutProduct,
       0x9db1186f,0x40df,0x11d1,0xaa,0x8c,0x00,0xc0,0x4f,0xb6,0x78,0x63);
DEFINE_GUID( SHELL32_AdvtShortcutComponent,
       0x9db1186e,0x40df,0x11d1,0xaa,0x8c,0x00,0xc0,0x4f,0xb6,0x78,0x63);

/* link file formats */

#include "pshpack1.h"

typedef struct _LINK_HEADER
{
	DWORD    dwSize;	/* 0x00 size of the header - 0x4c */
	GUID     MagicGuid;	/* 0x04 is CLSID_ShellLink */
	DWORD    dwFlags;	/* 0x14 describes elements following */
	DWORD    dwFileAttr;	/* 0x18 attributes of the target file */
	FILETIME Time1;		/* 0x1c */
	FILETIME Time2;		/* 0x24 */
	FILETIME Time3;		/* 0x2c */
	DWORD    dwFileLength;	/* 0x34 File length */
	DWORD    nIcon;		/* 0x38 icon number */
	DWORD	fStartup;	/* 0x3c startup type */
	DWORD	wHotKey;	/* 0x40 hotkey */
	DWORD	Unknown5;	/* 0x44 */
	DWORD	Unknown6;	/* 0x48 */
} LINK_HEADER, * PLINK_HEADER;

#define SHLINK_LOCAL  0
#define SHLINK_REMOTE 1
#define MAX_PROPERTY_SHEET_PAGE 32

typedef struct _LOCATION_INFO
{
    DWORD  dwTotalSize;
    DWORD  dwHeaderSize;
    DWORD  dwFlags;
    DWORD  dwVolTableOfs;
    DWORD  dwLocalPathOfs;
    DWORD  dwNetworkVolTableOfs;
    DWORD  dwFinalPathOfs;
} LOCATION_INFO;

typedef struct _LOCAL_VOLUME_INFO
{
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVolSerial;
    DWORD dwVolLabelOfs;
} LOCAL_VOLUME_INFO;

typedef struct volume_info_t
{
    DWORD type;
    DWORD serial;
    WCHAR label[12];  /* assume 8.3 */
} volume_info;

#include "poppack.h"

static const IShellLinkAVtbl slvt;
static const IShellLinkWVtbl slvtw;
static const IPersistFileVtbl pfvt;
static const IPersistStreamVtbl psvt;
static const IShellLinkDataListVtbl dlvt;
static const IShellExtInitVtbl eivt;
static const IContextMenuVtbl cmvt;
static const IObjectWithSiteVtbl owsvt;

/* IShellLink Implementation */

typedef struct
{
	const IShellLinkAVtbl *lpVtbl;
	const IShellLinkWVtbl *lpvtblw;
	const IPersistFileVtbl *lpvtblPersistFile;
	const IPersistStreamVtbl *lpvtblPersistStream;
	const IShellLinkDataListVtbl *lpvtblShellLinkDataList;
	const IShellExtInitVtbl *lpvtblShellExtInit;
	const IContextMenuVtbl *lpvtblContextMenu;
	const IObjectWithSiteVtbl *lpvtblObjectWithSite;

	LONG            ref;

	/* data structures according to the information in the link */
	LPITEMIDLIST	pPidl;
	WORD		wHotKey;
	SYSTEMTIME	time1;
	SYSTEMTIME	time2;
	SYSTEMTIME	time3;

	DWORD         iShowCmd;
	LPWSTR        sIcoPath;
	INT           iIcoNdx;
	LPWSTR        sPath;
	LPWSTR        sArgs;
	LPWSTR        sWorkDir;
	LPWSTR        sDescription;
	LPWSTR        sPathRel;
 	LPWSTR        sProduct;
 	LPWSTR        sComponent;
	volume_info   volume;
    LPWSTR        sLinkPath;
    BOOL          bRunAs;
	BOOL          bDirty;
        INT           iIdOpen;  /* id of the "Open" entry in the context menu */
        INT           iIdProperties; /* id of the "Properties" entry in the context menu */
	IUnknown      *site;
} IShellLinkImpl;

static inline IShellLinkImpl *impl_from_IShellLinkW( IShellLinkW *iface )
{
    return (IShellLinkImpl *)((char*)iface - FIELD_OFFSET(IShellLinkImpl, lpvtblw));
}

static inline IShellLinkImpl *impl_from_IPersistFile( IPersistFile *iface )
{
    return (IShellLinkImpl *)((char*)iface - FIELD_OFFSET(IShellLinkImpl, lpvtblPersistFile));
}

static inline IShellLinkImpl *impl_from_IPersistStream( IPersistStream *iface )
{
    return (IShellLinkImpl *)((char*)iface - FIELD_OFFSET(IShellLinkImpl, lpvtblPersistStream));
}

static inline IShellLinkImpl *impl_from_IShellLinkDataList( IShellLinkDataList *iface )
{
    return (IShellLinkImpl *)((char*)iface - FIELD_OFFSET(IShellLinkImpl, lpvtblShellLinkDataList));
}

static inline IShellLinkImpl *impl_from_IShellExtInit( IShellExtInit *iface )
{
    return (IShellLinkImpl *)((char*)iface - FIELD_OFFSET(IShellLinkImpl, lpvtblShellExtInit));
}

static inline IShellLinkImpl *impl_from_IContextMenu( IContextMenu *iface )
{
    return (IShellLinkImpl *)((char*)iface - FIELD_OFFSET(IShellLinkImpl, lpvtblContextMenu));
}

static inline IShellLinkImpl *impl_from_IObjectWithSite( IObjectWithSite *iface )
{
    return (IShellLinkImpl *)((char*)iface - FIELD_OFFSET(IShellLinkImpl, lpvtblObjectWithSite));
}

static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath);

/* strdup on the process heap */
static inline LPWSTR HEAP_strdupAtoW( HANDLE heap, DWORD flags, LPCSTR str)
{
    INT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    LPWSTR p = HeapAlloc( heap, flags, len*sizeof (WCHAR) );
    if( !p )
        return p;
    MultiByteToWideChar( CP_ACP, 0, str, -1, p, len );
    return p;
}

static inline LPWSTR strdupW( LPCWSTR src )
{
    LPWSTR dest;
    if (!src) return NULL;
    dest = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(src)+1)*sizeof(WCHAR) );
    if (dest)
        lstrcpyW(dest, src);
    return dest;
}

/**************************************************************************
 *  ShellLink::QueryInterface implementation
 */
static HRESULT ShellLink_QueryInterface( IShellLinkImpl *This, REFIID riid,  LPVOID *ppvObj)
{
    TRACE("(%p)->(\n\tIID:\t%s)\n",This,debugstr_guid(riid));

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IShellLinkA))
    {
        *ppvObj = This;
    }
    else if(IsEqualIID(riid, &IID_IShellLinkW))
    {
        *ppvObj = &(This->lpvtblw);
    }
    else if(IsEqualIID(riid, &IID_IPersistFile))
    {
        *ppvObj = &(This->lpvtblPersistFile);
    }
    else if(IsEqualIID(riid, &IID_IPersistStream))
    {
        *ppvObj = &(This->lpvtblPersistStream);
    }
    else if(IsEqualIID(riid, &IID_IShellLinkDataList))
    {
        *ppvObj = &(This->lpvtblShellLinkDataList);
    }
    else if(IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppvObj = &(This->lpvtblShellExtInit);
    }
    else if(IsEqualIID(riid, &IID_IContextMenu))
    {
        *ppvObj = &(This->lpvtblContextMenu);
    }
    else if(IsEqualIID(riid, &IID_IObjectWithSite))
    {
        *ppvObj = &(This->lpvtblObjectWithSite);
    }

    if(*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)(*ppvObj));
        TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
        return S_OK;
    }
    ERR("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/**************************************************************************
 *  ShellLink::AddRef implementation
 */
static ULONG ShellLink_AddRef( IShellLinkImpl *This )
{
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

/**************************************************************************
 *  ShellLink::Release implementation
 */
static ULONG ShellLink_Release( IShellLinkImpl *This )
{
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount + 1);

    if (refCount)
        return refCount;

    TRACE("-- destroying IShellLink(%p)\n",This);

    HeapFree(GetProcessHeap(), 0, This->sIcoPath);
    HeapFree(GetProcessHeap(), 0, This->sArgs);
    HeapFree(GetProcessHeap(), 0, This->sWorkDir);
    HeapFree(GetProcessHeap(), 0, This->sDescription);
    HeapFree(GetProcessHeap(),0,This->sPath);
    HeapFree(GetProcessHeap(),0,This->sLinkPath);

    if (This->site)
        IUnknown_Release( This->site );

    if (This->pPidl)
        ILFree(This->pPidl);

    LocalFree((HANDLE)This);

    return 0;
}

static HRESULT ShellLink_GetClassID( IShellLinkImpl *This, CLSID *pclsid )
{
    TRACE("%p %p\n", This, pclsid);

    memcpy( pclsid, &CLSID_ShellLink, sizeof (CLSID) );
    return S_OK;
}

/**************************************************************************
 *  IPersistFile_QueryInterface
 */
static HRESULT WINAPI IPersistFile_fnQueryInterface(
	IPersistFile* iface,
	REFIID riid,
	LPVOID *ppvObj)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    return ShellLink_QueryInterface( This, riid, ppvObj );
}

/******************************************************************************
 * IPersistFile_AddRef
 */
static ULONG WINAPI IPersistFile_fnAddRef(IPersistFile* iface)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    return ShellLink_AddRef( This );
}

/******************************************************************************
 * IPersistFile_Release
 */
static ULONG WINAPI IPersistFile_fnRelease(IPersistFile* iface)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    return IShellLinkA_Release((IShellLinkA*)This);
}

static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile* iface, CLSID *pClassID)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    return ShellLink_GetClassID( This, pClassID );
}

static HRESULT WINAPI IPersistFile_fnIsDirty(IPersistFile* iface)
{
	IShellLinkImpl *This = impl_from_IPersistFile(iface);

	TRACE("(%p)\n",This);

	if (This->bDirty)
	    return S_OK;

	return S_FALSE;
}

static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName, DWORD dwMode)
{
	IShellLinkImpl *This = impl_from_IPersistFile(iface);
	IPersistStream *StreamThis = (IPersistStream *)&This->lpvtblPersistStream;
        HRESULT r;
        IStream *stm;

        TRACE("(%p, %s, %x)\n",This, debugstr_w(pszFileName), dwMode);

        if( dwMode == 0 )
 		dwMode = STGM_READ | STGM_SHARE_DENY_WRITE;
        r = SHCreateStreamOnFileW(pszFileName, dwMode, &stm);
        if( SUCCEEDED( r ) )
        {
            HeapFree(GetProcessHeap(), 0, This->sLinkPath);
            This->sLinkPath = _wcsdup(pszFileName);
            r = IPersistStream_Load(StreamThis, stm);
            ShellLink_UpdatePath(This->sPathRel, pszFileName, This->sWorkDir, &This->sPath);
            IStream_Release( stm );
            This->bDirty = FALSE;
        }
        TRACE("-- returning hr %08x\n", r);
        return r;
}

static BOOL StartLinkProcessor( LPCOLESTR szLink )
{
    static const WCHAR szFormat[] = {
        'w','i','n','e','m','e','n','u','b','u','i','l','d','e','r','.','e','x','e',
        ' ','-','w',' ','"','%','s','"',0 };
    LONG len;
    LPWSTR buffer;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    BOOL ret;

    len = sizeof(szFormat) + lstrlenW( szLink ) * sizeof(WCHAR);
    buffer = HeapAlloc( GetProcessHeap(), 0, len );
    if( !buffer )
        return FALSE;

    wsprintfW( buffer, szFormat, szLink );

    TRACE("starting %s\n",debugstr_w(buffer));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    ret = CreateProcessW( NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );

    HeapFree( GetProcessHeap(), 0, buffer );

    if (ret)
    {
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    return ret;
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile* iface, LPCOLESTR pszFileName, BOOL fRemember)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    IPersistStream *StreamThis = (IPersistStream *)&This->lpvtblPersistStream;
    HRESULT r;
    IStream *stm;

    TRACE("(%p)->(%s)\n",This,debugstr_w(pszFileName));

    if (!pszFileName)
        return E_FAIL;

    r = SHCreateStreamOnFileW( pszFileName, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, &stm );
    if( SUCCEEDED( r ) )
    {
        r = IPersistStream_Save(StreamThis, stm, FALSE);
        IStream_Release( stm );

        if( SUCCEEDED( r ) )
	{
            StartLinkProcessor( pszFileName );

            This->bDirty = FALSE;
        }
	else
        {
            DeleteFileW( pszFileName );
            WARN("Failed to create shortcut %s\n", debugstr_w(pszFileName) );
        }
    }

    return r;
}

static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile* iface, LPCOLESTR pszFileName)
{
	IShellLinkImpl *This = impl_from_IPersistFile(iface);
	FIXME("(%p)->(%s)\n",This,debugstr_w(pszFileName));
	return NOERROR;
}

static HRESULT WINAPI IPersistFile_fnGetCurFile(IPersistFile* iface, LPOLESTR *ppszFileName)
{
	IShellLinkImpl *This = impl_from_IPersistFile(iface);
	FIXME("(%p)\n",This);
	return NOERROR;
}

static const IPersistFileVtbl pfvt =
{
	IPersistFile_fnQueryInterface,
	IPersistFile_fnAddRef,
	IPersistFile_fnRelease,
	IPersistFile_fnGetClassID,
	IPersistFile_fnIsDirty,
	IPersistFile_fnLoad,
	IPersistFile_fnSave,
	IPersistFile_fnSaveCompleted,
	IPersistFile_fnGetCurFile
};

/************************************************************************
 * IPersistStream_QueryInterface
 */
static HRESULT WINAPI IPersistStream_fnQueryInterface(
	IPersistStream* iface,
	REFIID     riid,
	VOID**     ppvObj)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return ShellLink_QueryInterface( This, riid, ppvObj );
}

/************************************************************************
 * IPersistStream_Release
 */
static ULONG WINAPI IPersistStream_fnRelease(
	IPersistStream* iface)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return IShellLinkA_Release((IShellLinkA*)This);
}

/************************************************************************
 * IPersistStream_AddRef
 */
static ULONG WINAPI IPersistStream_fnAddRef(
	IPersistStream* iface)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return ShellLink_AddRef( This );
}

/************************************************************************
 * IPersistStream_GetClassID
 *
 */
static HRESULT WINAPI IPersistStream_fnGetClassID(
	IPersistStream* iface,
	CLSID* pClassID)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return ShellLink_GetClassID( This, pClassID );
}

/************************************************************************
 * IPersistStream_IsDirty (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnIsDirty(
	IPersistStream*  iface)
{
	IShellLinkImpl *This = impl_from_IPersistStream(iface);

	TRACE("(%p)\n", This);

	return S_OK;
}


static HRESULT Stream_LoadString( IStream* stm, BOOL unicode, LPWSTR *pstr )
{
    DWORD count;
    USHORT len;
    LPVOID temp;
    LPWSTR str;
    HRESULT r;

    TRACE("%p\n", stm);

    count = 0;
    r = IStream_Read(stm, &len, sizeof(len), &count);
    if ( FAILED (r) || ( count != sizeof(len) ) )
        return E_FAIL;

    if( unicode )
        len *= sizeof (WCHAR);

    TRACE("reading %d\n", len);
    temp = HeapAlloc(GetProcessHeap(), 0, len+sizeof(WCHAR));
    if( !temp )
        return E_OUTOFMEMORY;
    count = 0;
    r = IStream_Read(stm, temp, len, &count);
    if( FAILED (r) || ( count != len ) )
    {
        HeapFree( GetProcessHeap(), 0, temp );
        return E_FAIL;
    }

    TRACE("read %s\n", debugstr_an(temp,len));

    /* convert to unicode if necessary */
    if( !unicode )
    {
        count = MultiByteToWideChar( CP_ACP, 0, (LPSTR) temp, len, NULL, 0 );
        str = HeapAlloc( GetProcessHeap(), 0, (count+1)*sizeof (WCHAR) );
        if( !str )
        {
            HeapFree( GetProcessHeap(), 0, temp );
            return E_OUTOFMEMORY;
        }
        MultiByteToWideChar( CP_ACP, 0, (LPSTR) temp, len, str, count );
        HeapFree( GetProcessHeap(), 0, temp );
    }
    else
    {
        count /= 2;
        str = (LPWSTR) temp;
    }
    str[count] = 0;

    *pstr = str;

    return S_OK;
}

static HRESULT Stream_ReadChunk( IStream* stm, LPVOID *data )
{
    DWORD size;
    ULONG count;
    HRESULT r;
    struct sized_chunk {
        DWORD size;
        unsigned char data[1];
    } *chunk;

    TRACE("%p\n",stm);

    r = IStream_Read( stm, &size, sizeof(size), &count );
    if( FAILED( r )  || count != sizeof(size) )
        return E_FAIL;

    chunk = HeapAlloc( GetProcessHeap(), 0, size );
    if( !chunk )
        return E_OUTOFMEMORY;

    chunk->size = size;
    r = IStream_Read( stm, chunk->data, size - sizeof(size), &count );
    if( FAILED( r ) || count != (size - sizeof(size)) )
    {
        HeapFree( GetProcessHeap(), 0, chunk );
        return E_FAIL;
    }

    TRACE("Read %d bytes\n",chunk->size);

    *data = (LPVOID) chunk;

    return S_OK;
}

static BOOL Stream_LoadVolume( LOCAL_VOLUME_INFO *vol, volume_info *volume )
{
    const int label_sz = sizeof volume->label/sizeof volume->label[0];
    LPSTR label;
    int len;

    volume->serial = vol->dwVolSerial;
    volume->type = vol->dwType;

    if( !vol->dwVolLabelOfs )
        return FALSE;
    if( vol->dwSize <= vol->dwVolLabelOfs )
        return FALSE;
    len = vol->dwSize - vol->dwVolLabelOfs;

    label = (LPSTR) vol;
    label += vol->dwVolLabelOfs;
    MultiByteToWideChar( CP_ACP, 0, label, len, volume->label, label_sz-1);

    return TRUE;
}

static LPWSTR Stream_LoadPath( LPCSTR p, DWORD maxlen )
{
    int len = 0, wlen;
    LPWSTR path;

    while( p[len] && (len < maxlen) )
        len++;

    wlen = MultiByteToWideChar(CP_ACP, 0, p, len, NULL, 0);
    path = HeapAlloc(GetProcessHeap(), 0, (wlen+1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, p, len, path, wlen);
    path[wlen] = 0;

    return path;
}

static HRESULT Stream_LoadLocation( IStream *stm,
                volume_info *volume, LPWSTR *path )
{
    char *p = NULL;
    LOCATION_INFO *loc;
    HRESULT r;
    int n;

    r = Stream_ReadChunk( stm, (LPVOID*) &p );
    if( FAILED(r) )
        return r;

    loc = (LOCATION_INFO*) p;
    if (loc->dwTotalSize < sizeof(LOCATION_INFO))
    {
        HeapFree( GetProcessHeap(), 0, p );
        return E_FAIL;
    }

    /* if there's valid local volume information, load it */
    if( loc->dwVolTableOfs && 
       ((loc->dwVolTableOfs + sizeof(LOCAL_VOLUME_INFO)) <= loc->dwTotalSize) )
    {
        LOCAL_VOLUME_INFO *volume_info;

        volume_info = (LOCAL_VOLUME_INFO*) &p[loc->dwVolTableOfs];
        Stream_LoadVolume( volume_info, volume );
    }

    /* if there's a local path, load it */
    n = loc->dwLocalPathOfs;
    if( n && (n < loc->dwTotalSize) )
        *path = Stream_LoadPath( &p[n], loc->dwTotalSize - n );

    TRACE("type %d serial %08x name %s path %s\n", volume->type,
          volume->serial, debugstr_w(volume->label), debugstr_w(*path));

    HeapFree( GetProcessHeap(), 0, p );
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
static HRESULT Stream_LoadAdvertiseInfo( IStream* stm, LPWSTR *str )
{
    DWORD size;
    ULONG count;
    HRESULT r;
    EXP_DARWIN_LINK buffer;
    
    TRACE("%p\n",stm);

    r = IStream_Read( stm, &buffer.dbh.cbSize, sizeof (DWORD), &count );
    if( FAILED( r ) )
        return r;

    /* make sure that we read the size of the structure even on error */
    size = sizeof buffer - sizeof (DWORD);
    if( buffer.dbh.cbSize != sizeof buffer )
    {
        ERR("Ooops.  This structure is not as expected...\n");
        return E_FAIL;
    }

    r = IStream_Read( stm, &buffer.dbh.dwSignature, size, &count );
    if( FAILED( r ) )
        return r;

    if( count != size )
        return E_FAIL;

    TRACE("magic %08x  string = %s\n", buffer.dbh.dwSignature, debugstr_w(buffer.szwDarwinID));

    if( (buffer.dbh.dwSignature&0xffff0000) != 0xa0000000 )
    {
        ERR("Unknown magic number %08x in advertised shortcut\n", buffer.dbh.dwSignature);
        return E_FAIL;
    }

    *str = HeapAlloc( GetProcessHeap(), 0, 
                     (lstrlenW(buffer.szwDarwinID)+1) * sizeof(WCHAR) );
    lstrcpyW( *str, buffer.szwDarwinID );

    return S_OK;
}

/************************************************************************
 * IPersistStream_Load (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnLoad(
    IPersistStream*  iface,
    IStream*         stm)
{
    LINK_HEADER hdr;
    ULONG    dwBytesRead;
    BOOL     unicode;
    HRESULT  r;
    DWORD    zero;

    IShellLinkImpl *This = impl_from_IPersistStream(iface);

    TRACE("%p %p\n", This, stm);

    if( !stm )
        return STG_E_INVALIDPOINTER;

    dwBytesRead = 0;
    r = IStream_Read(stm, &hdr, sizeof(hdr), &dwBytesRead);
    if( FAILED( r ) )
        return r;

    if( dwBytesRead != sizeof(hdr))
        return E_FAIL;
    if( hdr.dwSize != sizeof(hdr))
        return E_FAIL;
    if( !IsEqualIID(&hdr.MagicGuid, &CLSID_ShellLink) )
        return E_FAIL;

    /* free all the old stuff */
    ILFree(This->pPidl);
    This->pPidl = NULL;
    memset( &This->volume, 0, sizeof This->volume );
    HeapFree(GetProcessHeap(), 0, This->sPath);
    This->sPath = NULL;
    HeapFree(GetProcessHeap(), 0, This->sDescription);
    This->sDescription = NULL;
    HeapFree(GetProcessHeap(), 0, This->sPathRel);
    This->sPathRel = NULL;
    HeapFree(GetProcessHeap(), 0, This->sWorkDir);
    This->sWorkDir = NULL;
    HeapFree(GetProcessHeap(), 0, This->sArgs);
    This->sArgs = NULL;
    HeapFree(GetProcessHeap(), 0, This->sIcoPath);
    This->sIcoPath = NULL;
    HeapFree(GetProcessHeap(), 0, This->sProduct);
    This->sProduct = NULL;
    HeapFree(GetProcessHeap(), 0, This->sComponent);
    This->sComponent = NULL;
        
    This->wHotKey = (WORD)hdr.wHotKey;
    This->iIcoNdx = hdr.nIcon;
    FileTimeToSystemTime (&hdr.Time1, &This->time1);
    FileTimeToSystemTime (&hdr.Time2, &This->time2);
    FileTimeToSystemTime (&hdr.Time3, &This->time3);
    if (TRACE_ON(shell))
    {
        WCHAR sTemp[MAX_PATH];
        GetDateFormatW(LOCALE_USER_DEFAULT,DATE_SHORTDATE, &This->time1,
                       NULL, sTemp, sizeof(sTemp)/sizeof(*sTemp));
        TRACE("-- time1: %s\n", debugstr_w(sTemp) );
        GetDateFormatW(LOCALE_USER_DEFAULT,DATE_SHORTDATE, &This->time2,
                       NULL, sTemp, sizeof(sTemp)/sizeof(*sTemp));
        TRACE("-- time2: %s\n", debugstr_w(sTemp) );
        GetDateFormatW(LOCALE_USER_DEFAULT,DATE_SHORTDATE, &This->time3,
                       NULL, sTemp, sizeof(sTemp)/sizeof(*sTemp));
        TRACE("-- time3: %s\n", debugstr_w(sTemp) );
    }

    /* load all the new stuff */
    if( hdr.dwFlags & SLDF_HAS_ID_LIST )
    {
        r = ILLoadFromStream( stm, &This->pPidl );
        if( FAILED( r ) )
            return r;
    }
    pdump(This->pPidl);

    /* load the location information */
    if( hdr.dwFlags & SLDF_HAS_LINK_INFO )
        r = Stream_LoadLocation( stm, &This->volume, &This->sPath );
    if( FAILED( r ) )
        goto end;

    unicode = hdr.dwFlags & SLDF_UNICODE;
    if( hdr.dwFlags & SLDF_HAS_NAME )
    {
        r = Stream_LoadString( stm, unicode, &This->sDescription );
        TRACE("Description  -> %s\n",debugstr_w(This->sDescription));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_RELPATH )
    {
        r = Stream_LoadString( stm, unicode, &This->sPathRel );
        TRACE("Relative Path-> %s\n",debugstr_w(This->sPathRel));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_WORKINGDIR )
    {
        r = Stream_LoadString( stm, unicode, &This->sWorkDir );
        TRACE("Working Dir  -> %s\n",debugstr_w(This->sWorkDir));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_ARGS )
    {
        r = Stream_LoadString( stm, unicode, &This->sArgs );
        TRACE("Working Dir  -> %s\n",debugstr_w(This->sArgs));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_ICONLOCATION )
    {
        r = Stream_LoadString( stm, unicode, &This->sIcoPath );
        TRACE("Icon file    -> %s\n",debugstr_w(This->sIcoPath));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_LOGO3ID )
    {
        r = Stream_LoadAdvertiseInfo( stm, &This->sProduct );
        TRACE("Product      -> %s\n",debugstr_w(This->sProduct));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_DARWINID )
    {
        r = Stream_LoadAdvertiseInfo( stm, &This->sComponent );
        TRACE("Component    -> %s\n",debugstr_w(This->sComponent));
    }
    if( hdr.dwFlags & SLDF_RUNAS_USER )
    {
       This->bRunAs = TRUE;
    }
    else
    {
       This->bRunAs = FALSE;
    }

    if( FAILED( r ) )
        goto end;

    r = IStream_Read(stm, &zero, sizeof zero, &dwBytesRead);
    if( FAILED( r ) || zero || dwBytesRead != sizeof zero )
        ERR("Last word was not zero\n");

    TRACE("OK\n");

    pdump (This->pPidl);

    return S_OK;
end:
    return r;
}

/************************************************************************
 * Stream_WriteString
 *
 * Helper function for IPersistStream_Save. Writes a unicode string 
 *  with terminating nul byte to a stream, preceded by the its length.
 */
static HRESULT Stream_WriteString( IStream* stm, LPCWSTR str )
{
    USHORT len = lstrlenW( str ) + 1;
    DWORD count;
    HRESULT r;

    r = IStream_Write( stm, &len, sizeof(len), &count );
    if( FAILED( r ) )
        return r;

    len *= sizeof(WCHAR);

    r = IStream_Write( stm, str, len, &count );
    if( FAILED( r ) )
        return r;

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
static HRESULT Stream_WriteLocationInfo( IStream* stm, LPCWSTR path,
                                         volume_info *volume )
{
    DWORD total_size, path_size, volume_info_size, label_size, final_path_size;
    LOCAL_VOLUME_INFO *vol;
    LOCATION_INFO *loc;
    LPSTR szLabel, szPath, szFinalPath;
    ULONG count = 0;

    TRACE("%p %s %p\n", stm, debugstr_w(path), volume);

    /* figure out the size of everything */
    label_size = WideCharToMultiByte( CP_ACP, 0, volume->label, -1,
                                      NULL, 0, NULL, NULL );
    path_size = WideCharToMultiByte( CP_ACP, 0, path, -1,
                                     NULL, 0, NULL, NULL );
    volume_info_size = sizeof *vol + label_size;
    final_path_size = 1;
    total_size = sizeof *loc + volume_info_size + path_size + final_path_size;

    /* create pointers to everything */
    loc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, total_size);
    vol = (LOCAL_VOLUME_INFO*) &loc[1];
    szLabel = (LPSTR) &vol[1];
    szPath = &szLabel[label_size];
    szFinalPath = &szPath[path_size];

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
    WideCharToMultiByte( CP_ACP, 0, volume->label, -1,
                         szLabel, label_size, NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, path, -1,
                         szPath, path_size, NULL, NULL );
    szFinalPath[0] = 0;

    return IStream_Write( stm, loc, total_size, &count );
}

static EXP_DARWIN_LINK* shelllink_build_darwinid( LPCWSTR string, DWORD magic )
{
    EXP_DARWIN_LINK *buffer;
    
    buffer = LocalAlloc( LMEM_ZEROINIT, sizeof *buffer );
    buffer->dbh.cbSize = sizeof *buffer;
    buffer->dbh.dwSignature = magic;
    lstrcpynW( buffer->szwDarwinID, string, MAX_PATH );
    WideCharToMultiByte(CP_ACP, 0, string, -1, buffer->szDarwinID, MAX_PATH, NULL, NULL );

    return buffer;
}

static HRESULT Stream_WriteAdvertiseInfo( IStream* stm, LPCWSTR string, DWORD magic )
{
    EXP_DARWIN_LINK *buffer;
    ULONG count;
    
    TRACE("%p\n",stm);

    buffer = shelllink_build_darwinid( string, magic );

    return IStream_Write( stm, buffer, buffer->dbh.cbSize, &count );
}

/************************************************************************
 * IPersistStream_Save (IPersistStream)
 *
 * FIXME: makes assumptions about byte order
 */
static HRESULT WINAPI IPersistStream_fnSave(
	IPersistStream*  iface,
	IStream*         stm,
	BOOL             fClearDirty)
{
    LINK_HEADER header;
    ULONG   count;
    DWORD   zero;
    HRESULT r;

    IShellLinkImpl *This = impl_from_IPersistStream(iface);

    TRACE("%p %p %x\n", This, stm, fClearDirty);

    memset(&header, 0, sizeof(header));
    header.dwSize = sizeof(header);
    header.fStartup = This->iShowCmd;
    memcpy(&header.MagicGuid, &CLSID_ShellLink, sizeof(header.MagicGuid) );

    header.wHotKey = This->wHotKey;
    header.nIcon = This->iIcoNdx;
    header.dwFlags = SLDF_UNICODE;   /* strings are in unicode */
    if( This->pPidl )
        header.dwFlags |= SLDF_HAS_ID_LIST;
    if( This->sPath )
        header.dwFlags |= SLDF_HAS_LINK_INFO;
    if( This->sDescription )
        header.dwFlags |= SLDF_HAS_NAME;
    if( This->sWorkDir )
        header.dwFlags |= SLDF_HAS_WORKINGDIR;
    if( This->sArgs )
        header.dwFlags |= SLDF_HAS_ARGS;
    if( This->sIcoPath )
        header.dwFlags |= SLDF_HAS_ICONLOCATION;
    if( This->sProduct )
        header.dwFlags |= SLDF_HAS_LOGO3ID;
    if( This->sComponent )
        header.dwFlags |= SLDF_HAS_DARWINID;
    if( This->bRunAs )
        header.dwFlags |= SLDF_RUNAS_USER;

    SystemTimeToFileTime ( &This->time1, &header.Time1 );
    SystemTimeToFileTime ( &This->time2, &header.Time2 );
    SystemTimeToFileTime ( &This->time3, &header.Time3 );

    /* write the Shortcut header */
    r = IStream_Write( stm, &header, sizeof(header), &count );
    if( FAILED( r ) )
    {
        ERR("Write failed at %d\n",__LINE__);
        return r;
    }

    TRACE("Writing pidl\n");

    /* write the PIDL to the shortcut */
    if( This->pPidl )
    {
        r = ILSaveToStream( stm, This->pPidl );
        if( FAILED( r ) )
        {
            ERR("Failed to write PIDL at %d\n",__LINE__);
            return r;
        }
    }

    if( This->sPath )
        Stream_WriteLocationInfo( stm, This->sPath, &This->volume );

    if( This->sDescription )
        r = Stream_WriteString( stm, This->sDescription );

    if( This->sPathRel )
        r = Stream_WriteString( stm, This->sPathRel );

    if( This->sWorkDir )
        r = Stream_WriteString( stm, This->sWorkDir );

    if( This->sArgs )
        r = Stream_WriteString( stm, This->sArgs );

    if( This->sIcoPath )
        r = Stream_WriteString( stm, This->sIcoPath );

    if( This->sProduct )
        r = Stream_WriteAdvertiseInfo( stm, This->sProduct, EXP_SZ_ICON_SIG );

    if( This->sComponent )
        r = Stream_WriteAdvertiseInfo( stm, This->sComponent, EXP_DARWIN_ID_SIG );

    /* the last field is a single zero dword */
    zero = 0;
    r = IStream_Write( stm, &zero, sizeof zero, &count );

    return S_OK;
}

/************************************************************************
 * IPersistStream_GetSizeMax (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnGetSizeMax(
	IPersistStream*  iface,
	ULARGE_INTEGER*  pcbSize)
{
	IShellLinkImpl *This = impl_from_IPersistStream(iface);

	TRACE("(%p)\n", This);

	return E_NOTIMPL;
}

static const IPersistStreamVtbl psvt =
{
	IPersistStream_fnQueryInterface,
	IPersistStream_fnAddRef,
	IPersistStream_fnRelease,
	IPersistStream_fnGetClassID,
	IPersistStream_fnIsDirty,
	IPersistStream_fnLoad,
	IPersistStream_fnSave,
	IPersistStream_fnGetSizeMax
};

/**************************************************************************
 *	  IShellLink_Constructor
 */
HRESULT WINAPI IShellLink_Constructor( IUnknown *pUnkOuter,
               REFIID riid, LPVOID *ppv )
{
	IShellLinkImpl * sl;
	HRESULT r;

	TRACE("unkOut=%p riid=%s\n",pUnkOuter, debugstr_guid(riid));

	*ppv = NULL;

	if (pUnkOuter)
            return CLASS_E_NOAGGREGATION;
	sl = LocalAlloc(LMEM_ZEROINIT,sizeof(IShellLinkImpl));
	if (!sl)
            return E_OUTOFMEMORY;

	sl->ref = 1;
	sl->lpVtbl = &slvt;
	sl->lpvtblw = &slvtw;
	sl->lpvtblPersistFile = &pfvt;
	sl->lpvtblPersistStream = &psvt;
	sl->lpvtblShellLinkDataList = &dlvt;
	sl->lpvtblShellExtInit = &eivt;
	sl->lpvtblContextMenu = &cmvt;
	sl->lpvtblObjectWithSite = &owsvt;
	sl->iShowCmd = SW_SHOWNORMAL;
	sl->bDirty = FALSE;
	sl->iIdOpen = -1;
	sl->site = NULL;
    sl->bRunAs = FALSE;

	TRACE("(%p)->()\n",sl);

        r = ShellLink_QueryInterface( sl, riid, ppv );
        ShellLink_Release( sl );
        return r;
}


static BOOL SHELL_ExistsFileW(LPCWSTR path)
{
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(path))
        return FALSE;
    return TRUE;
}

/**************************************************************************
 *  ShellLink_UpdatePath
 *	update absolute path in sPath using relative path in sPathRel
 */
static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath)
{
    if (!path || !psPath)
	return E_INVALIDARG;

    if (!*psPath && sPathRel) {
	WCHAR buffer[2*MAX_PATH], abs_path[2*MAX_PATH];
	LPWSTR final = NULL;

	/* first try if [directory of link file] + [relative path] finds an existing file */

        GetFullPathNameW( path, MAX_PATH*2, buffer, &final );
        if( !final )
            final = buffer;
	lstrcpyW(final, sPathRel);

	*abs_path = '\0';

	if (SHELL_ExistsFileW(buffer)) {
	    if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
		lstrcpyW(abs_path, buffer);
	} else {
	    /* try if [working directory] + [relative path] finds an existing file */
	    if (sWorkDir) {
		lstrcpyW(buffer, sWorkDir);
		lstrcpyW(PathAddBackslashW(buffer), sPathRel);

		if (SHELL_ExistsFileW(buffer))
		    if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
			lstrcpyW(abs_path, buffer);
	    }
	}

	/* FIXME: This is even not enough - not all shell links can be resolved using this algorithm. */
	if (!*abs_path)
	    lstrcpyW(abs_path, sPathRel);

	*psPath = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(abs_path)+1)*sizeof(WCHAR));
	if (!*psPath)
	    return E_OUTOFMEMORY;

	lstrcpyW(*psPath, abs_path);
    }

    return S_OK;
}

/**************************************************************************
 *	  IShellLink_ConstructFromFile
 */
HRESULT WINAPI IShellLink_ConstructFromFile( IUnknown* pUnkOuter, REFIID riid,
               LPCITEMIDLIST pidl, LPVOID* ppv)
{
    IShellLinkW* psl;

    HRESULT hr = IShellLink_Constructor(NULL, riid, (LPVOID*)&psl);

    if (SUCCEEDED(hr)) {
	IPersistFile* ppf;

	*ppv = NULL;

	hr = IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

	if (SUCCEEDED(hr)) {
	    WCHAR path[MAX_PATH];

	    if (SHGetPathFromIDListW(pidl, path)) 
		hr = IPersistFile_Load(ppf, path, 0);
            else
                hr = E_FAIL;

	    if (SUCCEEDED(hr))
		*ppv = (IUnknown*) psl;

	    IPersistFile_Release(ppf);
	}

	if (!*ppv)
	    IShellLinkW_Release(psl);
    }

    return hr;
}

/**************************************************************************
 *  IShellLinkA_QueryInterface
 */
static HRESULT WINAPI IShellLinkA_fnQueryInterface( IShellLinkA * iface, REFIID riid,  LPVOID *ppvObj)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;
    return ShellLink_QueryInterface( This, riid, ppvObj );
}

/******************************************************************************
 * IShellLinkA_AddRef
 */
static ULONG WINAPI IShellLinkA_fnAddRef(IShellLinkA * iface)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;
    return ShellLink_AddRef( This );
}

/******************************************************************************
 *	IShellLinkA_Release
 */
static ULONG WINAPI IShellLinkA_fnRelease(IShellLinkA * iface)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;
    return ShellLink_Release( This );
}

static HRESULT WINAPI IShellLinkA_fnGetPath(IShellLinkA * iface, LPSTR pszFile,
                  INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%u)(%s)\n",
          This, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(This->sPath));

    if (This->sComponent || This->sProduct)
        return S_FALSE;

    if (cchMaxPath)
        pszFile[0] = 0;
    if (This->sPath)
        WideCharToMultiByte( CP_ACP, 0, This->sPath, -1,
                             pszFile, cchMaxPath, NULL, NULL);

    if (pfd) FIXME("(%p): WIN32_FIND_DATA is not yet filled.\n", This);

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetIDList(IShellLinkA * iface, LPITEMIDLIST * ppidl)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(ppidl=%p)\n",This, ppidl);

    return IShellLinkW_GetIDList((IShellLinkW*)&(This->lpvtblw), ppidl);
}

static HRESULT WINAPI IShellLinkA_fnSetIDList(IShellLinkA * iface, LPCITEMIDLIST pidl)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(pidl=%p)\n",This, pidl);

    if (This->pPidl)
	ILFree(This->pPidl);
    This->pPidl = ILClone (pidl);
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetDescription(IShellLinkA * iface, LPSTR pszName,INT cchMaxName)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(%p len=%u)\n",This, pszName, cchMaxName);

    if( cchMaxName )
        pszName[0] = 0;
    if( This->sDescription )
        WideCharToMultiByte( CP_ACP, 0, This->sDescription, -1,
            pszName, cchMaxName, NULL, NULL);

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetDescription(IShellLinkA * iface, LPCSTR pszName)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(pName=%s)\n", This, pszName);

    HeapFree(GetProcessHeap(), 0, This->sDescription);
    This->sDescription = HEAP_strdupAtoW( GetProcessHeap(), 0, pszName);
    if ( !This->sDescription )
        return E_OUTOFMEMORY;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetWorkingDirectory(IShellLinkA * iface, LPSTR pszDir,INT cchMaxPath)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(%p len=%u)\n", This, pszDir, cchMaxPath);

    if( cchMaxPath )
        pszDir[0] = 0;
    if( This->sWorkDir )
        WideCharToMultiByte( CP_ACP, 0, This->sWorkDir, -1,
                             pszDir, cchMaxPath, NULL, NULL);

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetWorkingDirectory(IShellLinkA * iface, LPCSTR pszDir)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(dir=%s)\n",This, pszDir);

    HeapFree(GetProcessHeap(), 0, This->sWorkDir);
    This->sWorkDir = HEAP_strdupAtoW( GetProcessHeap(), 0, pszDir);
    if ( !This->sWorkDir )
        return E_OUTOFMEMORY;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetArguments(IShellLinkA * iface, LPSTR pszArgs,INT cchMaxPath)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(%p len=%u)\n", This, pszArgs, cchMaxPath);

    if( cchMaxPath )
        pszArgs[0] = 0;
    if( This->sArgs )
        WideCharToMultiByte( CP_ACP, 0, This->sArgs, -1,
                             pszArgs, cchMaxPath, NULL, NULL);

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetArguments(IShellLinkA * iface, LPCSTR pszArgs)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(args=%s)\n",This, pszArgs);

    HeapFree(GetProcessHeap(), 0, This->sArgs);
    This->sArgs = HEAP_strdupAtoW( GetProcessHeap(), 0, pszArgs);
    if( !This->sArgs )
        return E_OUTOFMEMORY;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetHotkey(IShellLinkA * iface, WORD *pwHotkey)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(%p)(0x%08x)\n",This, pwHotkey, This->wHotKey);

    *pwHotkey = This->wHotKey;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetHotkey(IShellLinkA * iface, WORD wHotkey)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(hotkey=%x)\n",This, wHotkey);

    This->wHotKey = wHotkey;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetShowCmd(IShellLinkA * iface, INT *piShowCmd)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(%p)\n",This, piShowCmd);
    *piShowCmd = This->iShowCmd;
    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetShowCmd(IShellLinkA * iface, INT iShowCmd)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p) %d\n",This, iShowCmd);

    This->iShowCmd = iShowCmd;
    This->bDirty = TRUE;

    return NOERROR;
}

static HRESULT SHELL_PidlGeticonLocationA(IShellFolder* psf, LPCITEMIDLIST pidl,
                                          LPSTR pszIconPath, int cchIconPath, int* piIcon)
{
    LPCITEMIDLIST pidlLast;

    HRESULT hr = SHBindToParent(pidl, &IID_IShellFolder, (LPVOID*)&psf, &pidlLast);

    if (SUCCEEDED(hr)) {
	IExtractIconA* pei;

	hr = IShellFolder_GetUIObjectOf(psf, 0, 1, (LPCITEMIDLIST*)&pidlLast, &IID_IExtractIconA, NULL, (LPVOID*)&pei);

	if (SUCCEEDED(hr)) {
	    hr = IExtractIconA_GetIconLocation(pei, 0, pszIconPath, MAX_PATH, piIcon, NULL);

	    IExtractIconA_Release(pei);
	}

	IShellFolder_Release(psf);
    }

    return hr;
}

static HRESULT WINAPI IShellLinkA_fnGetIconLocation(IShellLinkA * iface, LPSTR pszIconPath,INT cchIconPath,INT *piIcon)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(%p len=%u iicon=%p)\n", This, pszIconPath, cchIconPath, piIcon);

    pszIconPath[0] = 0;
    *piIcon = This->iIcoNdx;

    if (This->sIcoPath)
    {
        WideCharToMultiByte(CP_ACP, 0, This->sIcoPath, -1, pszIconPath, cchIconPath, NULL, NULL);
	return S_OK;
    }

    if (This->pPidl || This->sPath)
    {
	IShellFolder* pdsk;

	HRESULT hr = SHGetDesktopFolder(&pdsk);

	if (SUCCEEDED(hr))
        {
	    /* first look for an icon using the PIDL (if present) */
	    if (This->pPidl)
		hr = SHELL_PidlGeticonLocationA(pdsk, This->pPidl, pszIconPath, cchIconPath, piIcon);
	    else
		hr = E_FAIL;

	    /* if we couldn't find an icon yet, look for it using the file system path */
	    if (FAILED(hr) && This->sPath)
            {
		LPITEMIDLIST pidl;

		hr = IShellFolder_ParseDisplayName(pdsk, 0, NULL, This->sPath, NULL, &pidl, NULL);

		if (SUCCEEDED(hr)) {
		    hr = SHELL_PidlGeticonLocationA(pdsk, pidl, pszIconPath, cchIconPath, piIcon);

		    SHFree(pidl);
		}
	    }

	    IShellFolder_Release(pdsk);
	}

	return hr;
    }
    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetIconLocation(IShellLinkA * iface, LPCSTR pszIconPath,INT iIcon)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(path=%s iicon=%u)\n",This, pszIconPath, iIcon);

    HeapFree(GetProcessHeap(), 0, This->sIcoPath);
    This->sIcoPath = HEAP_strdupAtoW(GetProcessHeap(), 0, pszIconPath);
    if ( !This->sIcoPath )
        return E_OUTOFMEMORY;

    This->iIcoNdx = iIcon;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetRelativePath(IShellLinkA * iface, LPCSTR pszPathRel, DWORD dwReserved)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(path=%s %x)\n",This, pszPathRel, dwReserved);

    HeapFree(GetProcessHeap(), 0, This->sPathRel);
    This->sPathRel = HEAP_strdupAtoW(GetProcessHeap(), 0, pszPathRel);
    This->bDirty = TRUE;

    return ShellLink_UpdatePath(This->sPathRel, This->sPath, This->sWorkDir, &This->sPath);
}

static HRESULT WINAPI IShellLinkA_fnResolve(IShellLinkA * iface, HWND hwnd, DWORD fFlags)
{
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(hwnd=%p flags=%x)\n",This, hwnd, fFlags);

    return IShellLinkW_Resolve( (IShellLinkW*)&(This->lpvtblw), hwnd, fFlags );
}

static HRESULT WINAPI IShellLinkA_fnSetPath(IShellLinkA * iface, LPCSTR pszFile)
{
    HRESULT r;
    LPWSTR str;
    IShellLinkImpl *This = (IShellLinkImpl *)iface;

    TRACE("(%p)->(path=%s)\n",This, pszFile);

    str = HEAP_strdupAtoW(GetProcessHeap(), 0, pszFile);
    if( !str ) 
        return E_OUTOFMEMORY;

    r = IShellLinkW_SetPath((IShellLinkW*)&(This->lpvtblw), str);
    HeapFree( GetProcessHeap(), 0, str );

    return r;
}

/**************************************************************************
* IShellLink Implementation
*/

static const IShellLinkAVtbl slvt =
{
    IShellLinkA_fnQueryInterface,
    IShellLinkA_fnAddRef,
    IShellLinkA_fnRelease,
    IShellLinkA_fnGetPath,
    IShellLinkA_fnGetIDList,
    IShellLinkA_fnSetIDList,
    IShellLinkA_fnGetDescription,
    IShellLinkA_fnSetDescription,
    IShellLinkA_fnGetWorkingDirectory,
    IShellLinkA_fnSetWorkingDirectory,
    IShellLinkA_fnGetArguments,
    IShellLinkA_fnSetArguments,
    IShellLinkA_fnGetHotkey,
    IShellLinkA_fnSetHotkey,
    IShellLinkA_fnGetShowCmd,
    IShellLinkA_fnSetShowCmd,
    IShellLinkA_fnGetIconLocation,
    IShellLinkA_fnSetIconLocation,
    IShellLinkA_fnSetRelativePath,
    IShellLinkA_fnResolve,
    IShellLinkA_fnSetPath
};


/**************************************************************************
 *  IShellLinkW_fnQueryInterface
 */
static HRESULT WINAPI IShellLinkW_fnQueryInterface(
  IShellLinkW * iface, REFIID riid, LPVOID *ppvObj)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    return ShellLink_QueryInterface( This, riid, ppvObj );
}

/******************************************************************************
 * IShellLinkW_fnAddRef
 */
static ULONG WINAPI IShellLinkW_fnAddRef(IShellLinkW * iface)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    return ShellLink_AddRef( This );
}

/******************************************************************************
 * IShellLinkW_fnRelease
 */
static ULONG WINAPI IShellLinkW_fnRelease(IShellLinkW * iface)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    return ShellLink_Release( This );
}

static HRESULT WINAPI IShellLinkW_fnGetPath(IShellLinkW * iface, LPWSTR pszFile,INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%u)(%s)\n",
          This, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(This->sPath));

    if (This->sComponent || This->sProduct)
        return S_FALSE;

    if (cchMaxPath)
        pszFile[0] = 0;
    if (This->sPath)
        lstrcpynW( pszFile, This->sPath, cchMaxPath );

    if (pfd) FIXME("(%p): WIN32_FIND_DATA is not yet filled.\n", This);

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetIDList(IShellLinkW * iface, LPITEMIDLIST * ppidl)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(ppidl=%p)\n",This, ppidl);

    if (!This->pPidl)
    {
	*ppidl = NULL;
        return S_FALSE;
    }
    *ppidl = ILClone(This->pPidl);
    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetIDList(IShellLinkW * iface, LPCITEMIDLIST pidl)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(pidl=%p)\n",This, pidl);

    if( This->pPidl )
        ILFree( This->pPidl );
    This->pPidl = ILClone( pidl );
    if( !This->pPidl )
        return E_FAIL;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetDescription(IShellLinkW * iface, LPWSTR pszName,INT cchMaxName)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len=%u)\n",This, pszName, cchMaxName);

    pszName[0] = 0;
    if( This->sDescription )
        lstrcpynW( pszName, This->sDescription, cchMaxName );

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetDescription(IShellLinkW * iface, LPCWSTR pszName)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(desc=%s)\n",This, debugstr_w(pszName));

    HeapFree(GetProcessHeap(), 0, This->sDescription);
    This->sDescription = HeapAlloc( GetProcessHeap(), 0,
                                    (lstrlenW( pszName )+1)*sizeof(WCHAR) );
    if ( !This->sDescription )
        return E_OUTOFMEMORY;

    lstrcpyW( This->sDescription, pszName );
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetWorkingDirectory(IShellLinkW * iface, LPWSTR pszDir,INT cchMaxPath)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len %u)\n", This, pszDir, cchMaxPath);

    if( cchMaxPath )
        pszDir[0] = 0;
    if( This->sWorkDir )
        lstrcpynW( pszDir, This->sWorkDir, cchMaxPath );

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetWorkingDirectory(IShellLinkW * iface, LPCWSTR pszDir)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(dir=%s)\n",This, debugstr_w(pszDir));

    HeapFree(GetProcessHeap(), 0, This->sWorkDir);
    This->sWorkDir = HeapAlloc( GetProcessHeap(), 0,
                                (lstrlenW( pszDir )+1)*sizeof (WCHAR) );
    if ( !This->sWorkDir )
        return E_OUTOFMEMORY;
    lstrcpyW( This->sWorkDir, pszDir );
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetArguments(IShellLinkW * iface, LPWSTR pszArgs,INT cchMaxPath)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len=%u)\n", This, pszArgs, cchMaxPath);

    if( cchMaxPath )
        pszArgs[0] = 0;
    if( This->sArgs )
        lstrcpynW( pszArgs, This->sArgs, cchMaxPath );

    return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetArguments(IShellLinkW * iface, LPCWSTR pszArgs)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(args=%s)\n",This, debugstr_w(pszArgs));

    HeapFree(GetProcessHeap(), 0, This->sArgs);
    This->sArgs = HeapAlloc( GetProcessHeap(), 0,
                             (lstrlenW( pszArgs )+1)*sizeof (WCHAR) );
    if ( !This->sArgs )
        return E_OUTOFMEMORY;
    lstrcpyW( This->sArgs, pszArgs );
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetHotkey(IShellLinkW * iface, WORD *pwHotkey)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p)\n",This, pwHotkey);

    *pwHotkey=This->wHotKey;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetHotkey(IShellLinkW * iface, WORD wHotkey)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(hotkey=%x)\n",This, wHotkey);

    This->wHotKey = wHotkey;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetShowCmd(IShellLinkW * iface, INT *piShowCmd)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p)\n",This, piShowCmd);

    *piShowCmd = This->iShowCmd;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetShowCmd(IShellLinkW * iface, INT iShowCmd)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    This->iShowCmd = iShowCmd;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT SHELL_PidlGeticonLocationW(IShellFolder* psf, LPCITEMIDLIST pidl,
                                          LPWSTR pszIconPath, int cchIconPath, int* piIcon)
{
    LPCITEMIDLIST pidlLast;

    HRESULT hr = SHBindToParent(pidl, &IID_IShellFolder, (LPVOID*)&psf, &pidlLast);

    if (SUCCEEDED(hr)) {
	IExtractIconW* pei;

	hr = IShellFolder_GetUIObjectOf(psf, 0, 1, (LPCITEMIDLIST*)&pidlLast, &IID_IExtractIconW, NULL, (LPVOID*)&pei);

	if (SUCCEEDED(hr)) {
	    hr = IExtractIconW_GetIconLocation(pei, 0, pszIconPath, MAX_PATH, piIcon, NULL);

	    IExtractIconW_Release(pei);
	}

	IShellFolder_Release(psf);
    }

    return hr;
}

static HRESULT WINAPI IShellLinkW_fnGetIconLocation(IShellLinkW * iface, LPWSTR pszIconPath,INT cchIconPath,INT *piIcon)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len=%u iicon=%p)\n", This, pszIconPath, cchIconPath, piIcon);

    pszIconPath[0] = 0;
    *piIcon = This->iIcoNdx;

    if (This->sIcoPath)
    {
	lstrcpynW(pszIconPath, This->sIcoPath, cchIconPath);
	return S_OK;
    }

    if (This->pPidl || This->sPath)
    {
	IShellFolder* pdsk;

	HRESULT hr = SHGetDesktopFolder(&pdsk);

	if (SUCCEEDED(hr))
        {
	    /* first look for an icon using the PIDL (if present) */
	    if (This->pPidl)
		hr = SHELL_PidlGeticonLocationW(pdsk, This->pPidl, pszIconPath, cchIconPath, piIcon);
	    else
		hr = E_FAIL;

	    /* if we couldn't find an icon yet, look for it using the file system path */
	    if (FAILED(hr) && This->sPath)
            {
		LPITEMIDLIST pidl;

		hr = IShellFolder_ParseDisplayName(pdsk, 0, NULL, This->sPath, NULL, &pidl, NULL);

		if (SUCCEEDED(hr))
                {
		    hr = SHELL_PidlGeticonLocationW(pdsk, pidl, pszIconPath, cchIconPath, piIcon);

		    SHFree(pidl);
		}
	    }

	    IShellFolder_Release(pdsk);
	}
	return hr;
    }
    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetIconLocation(IShellLinkW * iface, LPCWSTR pszIconPath,INT iIcon)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(path=%s iicon=%u)\n",This, debugstr_w(pszIconPath), iIcon);

    HeapFree(GetProcessHeap(), 0, This->sIcoPath);
    This->sIcoPath = HeapAlloc( GetProcessHeap(), 0,
                                (lstrlenW( pszIconPath )+1)*sizeof (WCHAR) );
    if ( !This->sIcoPath )
        return E_OUTOFMEMORY;
    lstrcpyW( This->sIcoPath, pszIconPath );

    This->iIcoNdx = iIcon;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetRelativePath(IShellLinkW * iface, LPCWSTR pszPathRel, DWORD dwReserved)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(path=%s %x)\n",This, debugstr_w(pszPathRel), dwReserved);

    HeapFree(GetProcessHeap(), 0, This->sPathRel);
    This->sPathRel = HeapAlloc( GetProcessHeap(), 0,
                                (lstrlenW( pszPathRel )+1) * sizeof (WCHAR) );
    if ( !This->sPathRel )
        return E_OUTOFMEMORY;
    lstrcpyW( This->sPathRel, pszPathRel );
    This->bDirty = TRUE;

    return ShellLink_UpdatePath(This->sPathRel, This->sPath, This->sWorkDir, &This->sPath);
}

static HRESULT WINAPI IShellLinkW_fnResolve(IShellLinkW * iface, HWND hwnd, DWORD fFlags)
{
    HRESULT hr = S_OK;
    BOOL bSuccess;

    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(hwnd=%p flags=%x)\n",This, hwnd, fFlags);

    /*FIXME: use IResolveShellLink interface */

    if (!This->sPath && This->pPidl) {
	WCHAR buffer[MAX_PATH];

	bSuccess = SHGetPathFromIDListW(This->pPidl, buffer);

	if (bSuccess && *buffer) {
	    This->sPath = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(buffer)+1)*sizeof(WCHAR));
	    if (!This->sPath)
		return E_OUTOFMEMORY;

	    lstrcpyW(This->sPath, buffer);

	    This->bDirty = TRUE;
	} else
	    hr = S_OK;    /* don't report an error occurred while just caching information */
    }

    if (!This->sIcoPath && This->sPath) {
	This->sIcoPath = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(This->sPath)+1)*sizeof(WCHAR));
	if (!This->sIcoPath)
	    return E_OUTOFMEMORY;

	lstrcpyW(This->sIcoPath, This->sPath);
	This->iIcoNdx = 0;

	This->bDirty = TRUE;
    }

    return hr;
}

static LPWSTR ShellLink_GetAdvertisedArg(LPCWSTR str)
{
    LPWSTR ret;
    LPCWSTR p;
    DWORD len;

    if( !str )
        return NULL;

    p = strchrW( str, ':' );
    if( !p )
        return NULL;
    len = p - str;
    ret = HeapAlloc( GetProcessHeap(), 0, sizeof(WCHAR)*(len+1));
    if( !ret )
        return ret;
    memcpy( ret, str, sizeof(WCHAR)*len );
    ret[len] = 0;
    return ret;
}

static HRESULT ShellLink_SetAdvertiseInfo(IShellLinkImpl *This, LPCWSTR str)
{
    LPCWSTR szComponent = NULL, szProduct = NULL, p;
    WCHAR szGuid[39];
    HRESULT r;
    GUID guid;
    int len;

    while( str[0] )
    {
        /* each segment must start with two colons */
        if( str[0] != ':' || str[1] != ':' )
            return E_FAIL;

        /* the last segment is just two colons */
        if( !str[2] )
            break;
        str += 2;

        /* there must be a colon straight after a guid */
        p = strchrW( str, ':' );
        if( !p )
            return E_FAIL;
        len = p - str;
        if( len != 38 )
            return E_FAIL;

        /* get the guid, and check it's validly formatted */
        memcpy( szGuid, str, sizeof(WCHAR)*len );
        szGuid[len] = 0;
        r = CLSIDFromString( szGuid, &guid );
        if( r != S_OK )
            return r;
        str = p + 1;

        /* match it up to a guid that we care about */
        if( IsEqualGUID( &guid, &SHELL32_AdvtShortcutComponent ) && !szComponent )
            szComponent = str;
        else if( IsEqualGUID( &guid, &SHELL32_AdvtShortcutProduct ) && !szProduct )
            szProduct = str;
        else
            return E_FAIL;

        /* skip to the next field */
        str = strchrW( str, ':' );
        if( !str )
            return E_FAIL;
    }

    /* we have to have a component for an advertised shortcut */
    if( !szComponent )
        return E_FAIL;

    This->sComponent = ShellLink_GetAdvertisedArg( szComponent );
    This->sProduct = ShellLink_GetAdvertisedArg( szProduct );

    TRACE("Component = %s\n", debugstr_w(This->sComponent));
    TRACE("Product = %s\n", debugstr_w(This->sProduct));

    return S_OK;
}

static BOOL ShellLink_GetVolumeInfo(LPCWSTR path, volume_info *volume)
{
    const int label_sz = sizeof volume->label/sizeof volume->label[0];
    WCHAR drive[4] = { path[0], ':', '\\', 0 };
    BOOL r;

    volume->type = GetDriveTypeW(drive);
    r = GetVolumeInformationW(drive, volume->label, label_sz,
                              &volume->serial, NULL, NULL, NULL, 0);
    TRACE("r = %d type %d serial %08x name %s\n", r,
          volume->type, volume->serial, debugstr_w(volume->label));
    return r;
}

static HRESULT WINAPI IShellLinkW_fnSetPath(IShellLinkW * iface, LPCWSTR pszFile)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    WCHAR buffer[MAX_PATH];
    LPWSTR fname, unquoted = NULL;
    HRESULT hr = S_OK;
    UINT len;

    TRACE("(%p)->(path=%s)\n",This, debugstr_w(pszFile));

    /* quotes at the ends of the string are stripped */
    len = lstrlenW(pszFile);
    if (pszFile[0] == '"' && pszFile[len-1] == '"')
    {
        unquoted = strdupW(pszFile);
        PathUnquoteSpacesW(unquoted);
        pszFile = unquoted;
    }

    /* any other quote marks are invalid */
    if (strchrW(pszFile, '"'))
        return S_FALSE;

    HeapFree(GetProcessHeap(), 0, This->sPath);
    This->sPath = NULL;

    HeapFree(GetProcessHeap(), 0, This->sComponent);
    This->sComponent = NULL;

    if (This->pPidl)
        ILFree(This->pPidl);
    This->pPidl = NULL;

    if (S_OK != ShellLink_SetAdvertiseInfo( This, pszFile ))
    {
        if (*pszFile == '\0')
            *buffer = '\0';
        else if (!GetFullPathNameW(pszFile, MAX_PATH, buffer, &fname))
	    return E_FAIL;
        else if(!PathFileExistsW(buffer) &&
		!SearchPathW(NULL, pszFile, NULL, MAX_PATH, buffer, NULL))
	  hr = S_FALSE;

        This->pPidl = SHSimpleIDListFromPathW(pszFile);
        ShellLink_GetVolumeInfo(buffer, &This->volume);

        This->sPath = HeapAlloc( GetProcessHeap(), 0,
                             (lstrlenW( buffer )+1) * sizeof (WCHAR) );
        if (!This->sPath)
            return E_OUTOFMEMORY;

        lstrcpyW(This->sPath, buffer);
    }
    This->bDirty = TRUE;
    HeapFree(GetProcessHeap(), 0, unquoted);

    return hr;
}

/**************************************************************************
* IShellLinkW Implementation
*/

static const IShellLinkWVtbl slvtw =
{
    IShellLinkW_fnQueryInterface,
    IShellLinkW_fnAddRef,
    IShellLinkW_fnRelease,
    IShellLinkW_fnGetPath,
    IShellLinkW_fnGetIDList,
    IShellLinkW_fnSetIDList,
    IShellLinkW_fnGetDescription,
    IShellLinkW_fnSetDescription,
    IShellLinkW_fnGetWorkingDirectory,
    IShellLinkW_fnSetWorkingDirectory,
    IShellLinkW_fnGetArguments,
    IShellLinkW_fnSetArguments,
    IShellLinkW_fnGetHotkey,
    IShellLinkW_fnSetHotkey,
    IShellLinkW_fnGetShowCmd,
    IShellLinkW_fnSetShowCmd,
    IShellLinkW_fnGetIconLocation,
    IShellLinkW_fnSetIconLocation,
    IShellLinkW_fnSetRelativePath,
    IShellLinkW_fnResolve,
    IShellLinkW_fnSetPath
};

static HRESULT WINAPI
ShellLink_DataList_QueryInterface( IShellLinkDataList* iface, REFIID riid, void** ppvObject)
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvObject);
}

static ULONG WINAPI
ShellLink_DataList_AddRef( IShellLinkDataList* iface )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    return IShellLinkA_AddRef((IShellLinkA*)This);
}

static ULONG WINAPI
ShellLink_DataList_Release( IShellLinkDataList* iface )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    return ShellLink_Release( This );
}

static HRESULT WINAPI
ShellLink_AddDataBlock( IShellLinkDataList* iface, void* pDataBlock )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI
ShellLink_CopyDataBlock( IShellLinkDataList* iface, DWORD dwSig, void** ppDataBlock )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    LPVOID block = NULL;
    HRESULT r = E_FAIL;

    TRACE("%p %08x %p\n", iface, dwSig, ppDataBlock );

    switch (dwSig)
    {
    case EXP_DARWIN_ID_SIG:
        if (!This->sComponent)
            break;
        block = shelllink_build_darwinid( This->sComponent, dwSig );
        r = S_OK;
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
    return r;
}

static HRESULT WINAPI
ShellLink_RemoveDataBlock( IShellLinkDataList* iface, DWORD dwSig )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI
ShellLink_GetFlags( IShellLinkDataList* iface, DWORD* pdwFlags )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    DWORD flags = 0;

    FIXME("%p %p\n", This, pdwFlags );

    /* FIXME: add more */
    if (This->sArgs)
        flags |= SLDF_HAS_ARGS;
    if (This->sComponent)
        flags |= SLDF_HAS_DARWINID;
    if (This->sIcoPath)
        flags |= SLDF_HAS_ICONLOCATION;
    if (This->sProduct)
        flags |= SLDF_HAS_LOGO3ID;
    if (This->pPidl)
        flags |= SLDF_HAS_ID_LIST;

    *pdwFlags = flags;

    return S_OK;
}

static HRESULT WINAPI
ShellLink_SetFlags( IShellLinkDataList* iface, DWORD dwFlags )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const IShellLinkDataListVtbl dlvt =
{
    ShellLink_DataList_QueryInterface,
    ShellLink_DataList_AddRef,
    ShellLink_DataList_Release,
    ShellLink_AddDataBlock,
    ShellLink_CopyDataBlock,
    ShellLink_RemoveDataBlock,
    ShellLink_GetFlags,
    ShellLink_SetFlags
};

static HRESULT WINAPI
ShellLink_ExtInit_QueryInterface( IShellExtInit* iface, REFIID riid, void** ppvObject )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvObject);
}

static ULONG WINAPI
ShellLink_ExtInit_AddRef( IShellExtInit* iface )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    return IShellLinkA_AddRef((IShellLinkA*)This);
}

static ULONG WINAPI
ShellLink_ExtInit_Release( IShellExtInit* iface )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    return ShellLink_Release( This );
}

/**************************************************************************
 * ShellLink implementation of IShellExtInit::Initialize()
 *
 * Loads the shelllink from the dataobject the shell is pointing to.
 */
static HRESULT WINAPI
ShellLink_ExtInit_Initialize( IShellExtInit* iface, LPCITEMIDLIST pidlFolder,
                              IDataObject *pdtobj, HKEY hkeyProgID )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    FORMATETC format;
    STGMEDIUM stgm;
    UINT count;
    HRESULT r = E_FAIL;

    TRACE("%p %p %p %p\n", This, pidlFolder, pdtobj, hkeyProgID );

    if( !pdtobj )
        return r;

    format.cfFormat = CF_HDROP;
    format.ptd = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex = -1;
    format.tymed = TYMED_HGLOBAL;

    if( FAILED( IDataObject_GetData( pdtobj, &format, &stgm ) ) )
        return r;

    count = DragQueryFileW( stgm.u.hGlobal, -1, NULL, 0 );
    if( count == 1 )
    {
        LPWSTR path;

        count = DragQueryFileW( stgm.u.hGlobal, 0, NULL, 0 );
        count++;
        path = HeapAlloc( GetProcessHeap(), 0, count*sizeof(WCHAR) );
        if( path )
        {
            IPersistFile *pf = (IPersistFile*) &This->lpvtblPersistFile;

            count = DragQueryFileW( stgm.u.hGlobal, 0, path, count );
            r = IPersistFile_Load( pf, path, 0 );
            HeapFree( GetProcessHeap(), 0, path );
        }
    }
    ReleaseStgMedium( &stgm );

    return r;
}

static const IShellExtInitVtbl eivt =
{
    ShellLink_ExtInit_QueryInterface,
    ShellLink_ExtInit_AddRef,
    ShellLink_ExtInit_Release,
    ShellLink_ExtInit_Initialize
};

static HRESULT WINAPI
ShellLink_ContextMenu_QueryInterface( IContextMenu* iface, REFIID riid, void** ppvObject )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvObject);
}

static ULONG WINAPI
ShellLink_ContextMenu_AddRef( IContextMenu* iface )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    return IShellLinkA_AddRef((IShellLinkA*)This);
}

static ULONG WINAPI
ShellLink_ContextMenu_Release( IContextMenu* iface )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    return ShellLink_Release( This );
}

static HRESULT WINAPI
ShellLink_QueryContextMenu( IContextMenu* iface, HMENU hmenu, UINT indexMenu,
                            UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    static WCHAR szOpen[] = { 'O','p','e','n',0 };
    static WCHAR szProperties[] = { 'P','r','o','p','e','r','t','i','e','s',0 };
    MENUITEMINFOW mii;
    int id = 1;

    TRACE("%p %p %u %u %u %u\n", This,
          hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags );

    if ( !hmenu )
        return E_INVALIDARG;

    memset( &mii, 0, sizeof(mii) );
    mii.cbSize = sizeof (mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.dwTypeData = (LPWSTR)szOpen;
    mii.cch = strlenW( mii.dwTypeData );
    mii.wID = idCmdFirst + id++;
    mii.fState = MFS_DEFAULT | MFS_ENABLED;
    mii.fType = MFT_STRING;
    if (!InsertMenuItemW( hmenu, indexMenu, TRUE, &mii ))
        return E_FAIL;
    This->iIdOpen = 0;

    mii.fState = MFS_ENABLED;
    mii.dwTypeData = (LPWSTR)szProperties;
    mii.cch = strlenW( mii.dwTypeData );
    mii.wID = idCmdFirst + id++;
    if (!InsertMenuItemW( hmenu, idCmdLast, TRUE, &mii ))
    {
        TRACE("ShellLink_QueryContextMenu failed to insert item properties");
        return E_FAIL;
    }
    This->iIdProperties = 1;
    id++;

    return MAKE_HRESULT( SEVERITY_SUCCESS, 0, id );
}

static LPWSTR
shelllink_get_msi_component_path( LPWSTR component )
{
    LPWSTR path;
    DWORD r, sz = 0;

    r = CommandLineFromMsiDescriptor( component, NULL, &sz );
    if (r != ERROR_SUCCESS)
         return NULL;

    sz++;
    path = HeapAlloc( GetProcessHeap(), 0, sz*sizeof(WCHAR) );
    r = CommandLineFromMsiDescriptor( component, path, &sz );
    if (r != ERROR_SUCCESS)
    {
        HeapFree( GetProcessHeap(), 0, path );
        path = NULL;
    }

    TRACE("returning %s\n", debugstr_w( path ) );

    return path;
}

INT_PTR CALLBACK ExtendedShortcutProc(      
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
   HWND hDlgCtrl;

   switch(uMsg)
   {
   case WM_INITDIALOG:
       if (lParam)
       {
            hDlgCtrl = GetDlgItem(hwndDlg, 14000);
            SendMessage(hDlgCtrl, BM_SETCHECK, BST_CHECKED, 0);
       }
       return TRUE;
   case WM_COMMAND:
        hDlgCtrl = GetDlgItem(hwndDlg, 14000);
		if (LOWORD(wParam) == IDOK)
        {
		   if ( SendMessage(hDlgCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED )
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
           if ( SendMessage(hDlgCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
              SendMessage(hDlgCtrl, BM_SETCHECK, BST_UNCHECKED, 0);
           else
              SendMessage(hDlgCtrl, BM_SETCHECK, BST_CHECKED, 0);

        }
   }
   return FALSE;
}

/**************************************************************************
 * SH_ShellLinkDlgProc
 *
 * dialog proc of the shortcut property dialog
 */

INT_PTR 
CALLBACK 
SH_ShellLinkDlgProc(   
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPPROPSHEETPAGEW ppsp;
    LPPSHNOTIFY lppsn;
    IShellLinkImpl *This;
    HWND hDlgCtrl;
    WCHAR szBuffer[MAX_PATH];
    int IconIndex;
    INT_PTR result;

    This = (IShellLinkImpl *)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        ppsp = (LPPROPSHEETPAGEW)lParam;
        if (ppsp == NULL)
            break;
     
        TRACE("ShellLink_DlgProc (WM_INITDIALOG hwnd %p lParam %p ppsplParam %x)\n",hwndDlg, lParam, ppsp->lParam);

        This = (IShellLinkImpl *)ppsp->lParam;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)This);

		TRACE("sArgs: %S sComponent: %S sDescription: %S sIcoPath: %S sPath: %S sPathRel: %S sProduct: %S sWorkDir: %S\n", This->sArgs, This->sComponent ,This->sDescription,
			This->sIcoPath, This->sPath, This->sPathRel, This->sProduct, This->sWorkDir);

        /* target path */
        hDlgCtrl = GetDlgItem( hwndDlg, 14009 );
        if ( hDlgCtrl != NULL )
            SendMessageW( hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)This->sPath );

       /* working dir */
       hDlgCtrl = GetDlgItem( hwndDlg, 14011 );
       if ( hDlgCtrl != NULL )
            SendMessageW( hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)This->sWorkDir );

	   /* description */
       hDlgCtrl = GetDlgItem( hwndDlg, 14019 );
       if ( hDlgCtrl != NULL )
            SendMessageW( hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)This->sDescription );
        return TRUE;
    case WM_NOTIFY:
       lppsn = (LPPSHNOTIFY) lParam; 
       if ( lppsn->hdr.code == PSN_APPLY )
       {
            /* set working directory */
            hDlgCtrl = GetDlgItem( hwndDlg, 14011 );
            SendMessageW( hDlgCtrl, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szBuffer );
            IShellLinkW_fnSetWorkingDirectory((IShellLinkW*)&This->lpvtblw, szBuffer);
            /* set link destination */
            hDlgCtrl = GetDlgItem( hwndDlg, 14009 );
            SendMessageW( hDlgCtrl, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szBuffer);
            if ( !SHELL_ExistsFileW(szBuffer) )
            {
                MessageBoxW( hwndDlg, L"file not existing", szBuffer, MB_OK );
                SetWindowLong( hwndDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
                return TRUE;
            }
            IShellLinkW_fnSetPath((IShellLinkW*)&This->lpvtblw, szBuffer);

            TRACE("This %p sLinkPath %S\n", This, This->sLinkPath);
            IPersistFile_fnSave( (IPersistFile*)&This->lpvtblPersistFile, This->sLinkPath, TRUE );
            SetWindowLong( hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR );
            return TRUE;
       }
       break;
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
               if (PickIconDlg(hwndDlg, szBuffer, MAX_PATH, &IconIndex))
               {
                    IShellLinkW_fnSetIconLocation((IShellLinkW*)&This->lpvtblw, szBuffer, IconIndex);
                    ///
                    /// FIXME redraw icon
               }
               return TRUE;
           case 14022:
               result = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(SHELL_EXTENDED_SHORTCUT_DLG), hwndDlg, ExtendedShortcutProc, (LPARAM)This->bRunAs);
               if (result == 1 || result == 0)
               {
                   if ( This->bRunAs != result )
                   {
                       PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                   }

                   This->bRunAs = result;
               }
               return TRUE;
       }
       switch(HIWORD(wParam))
       {
           case EN_CHANGE:
              PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
              break;
       }
       break;
    default:
        break;
   }
   return FALSE;
}

/**************************************************************************
 * ShellLink_ShortcutDialog [Internal]
 *
 * creates a shortcut property dialog
 */

static HRESULT WINAPI
ShellLink_ShowProperties( IShellLinkImpl *This )
{
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    HPROPSHEETPAGE hpage;
    UINT numpages = 0;

    TRACE("ShellLink_ShortcutDialog entered\n");

    memset(hppages, 0x0, sizeof(HPROPSHEETPAGE) * MAX_PROPERTY_SHEET_PAGE);

    hpage = SH_CreatePropertySheetPage("SHELL_FILE_GENERAL_DLG", SH_FileGeneralDlgProc, (LPARAM)This->sLinkPath);
    if ( hpage == NULL )
        return E_FAIL;
    else
        hppages[numpages++] = hpage;

	hpage = SH_CreatePropertySheetPage("SHELL_GENERAL_SHORTCUT_DLG", SH_ShellLinkDlgProc, (LPARAM)This);
	if ( hpage == NULL )
    {
        ERR("SH_CreatePropertySheetPage failed\n");
        DestroyPropertySheetPage(hppages[0]);
        return E_FAIL;
	}
    hppages[numpages++] = hpage;

    ///FIXME
    /// load extensions

    memset(&pinfo, 0x0, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    pinfo.nPages = numpages;
	pinfo.u3.phpage = hppages;
    pinfo.pszCaption = This->sDescription;
    pinfo.u2.nStartPage = 1;

    if ( PropertySheetW(&pinfo) < 0 )
        return E_FAIL;
	else
        return S_OK;
}

static HRESULT WINAPI
ShellLink_InvokeCommand( IContextMenu* iface, LPCMINVOKECOMMANDINFO lpici )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    static const WCHAR szOpen[] = { 'O','p','e','n',0 };
    SHELLEXECUTEINFOW sei;
    HWND hwnd = NULL; /* FIXME: get using interface set from IObjectWithSite */
    LPWSTR args = NULL;
    LPWSTR path = NULL;
    HRESULT r;

    TRACE("%p %p\n", This, lpici );

    if ( lpici->cbSize < sizeof (CMINVOKECOMMANDINFO) )
        return E_INVALIDARG;

    if ( lpici->lpVerb == MAKEINTRESOURCEA(This->iIdProperties))
    {
        ShellLink_ShowProperties(This);
        return S_OK;
    }

    if ( lpici->lpVerb != MAKEINTRESOURCEA(This->iIdOpen) )
    {
        ERR("Unknown id %d != %d\n", (INT)lpici->lpVerb, This->iIdOpen );
        return E_INVALIDARG;
    }

    r = IShellLinkW_Resolve( (IShellLinkW*)&(This->lpvtblw), hwnd, 0 );
    if ( FAILED( r ) )
        return r;

    if ( This->sComponent )
    {
        path = shelllink_get_msi_component_path( This->sComponent );
        if (!path)
            return E_FAIL;
    }
    else
        path = strdupW( This->sPath );

    if ( lpici->cbSize == sizeof (CMINVOKECOMMANDINFOEX) &&
         ( lpici->fMask & CMIC_MASK_UNICODE ) )
    {
        LPCMINVOKECOMMANDINFOEX iciex = (LPCMINVOKECOMMANDINFOEX) lpici;
        DWORD len = 2;

        if ( This->sArgs )
            len += lstrlenW( This->sArgs );
        if ( iciex->lpParametersW )
            len += lstrlenW( iciex->lpParametersW );

        args = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        args[0] = 0;
        if ( This->sArgs )
            lstrcatW( args, This->sArgs );
        if ( iciex->lpParametersW )
        {
            static const WCHAR space[] = { ' ', 0 };
            lstrcatW( args, space );
            lstrcatW( args, iciex->lpParametersW );
        }
    }

    memset( &sei, 0, sizeof sei );
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_UNICODE | SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = path;
    sei.nShow = This->iShowCmd;
    sei.lpIDList = This->pPidl;
    sei.lpDirectory = This->sWorkDir;
    sei.lpParameters = args;
    sei.lpVerb = szOpen;

    if( ShellExecuteExW( &sei ) )
    {
        if ( sei.hProcess )
        {
            WaitForSingleObject( sei.hProcess, 10000 );
            CloseHandle( sei.hProcess );
        }
        r = S_OK;
    }
    else
        r = E_FAIL;

    HeapFree( GetProcessHeap(), 0, args );
    HeapFree( GetProcessHeap(), 0, path );

    return r;
}

static HRESULT WINAPI
ShellLink_GetCommandString( IContextMenu* iface, UINT_PTR idCmd, UINT uType,
                            UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);

    FIXME("%p %lu %u %p %p %u\n", This,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

static const IContextMenuVtbl cmvt =
{
    ShellLink_ContextMenu_QueryInterface,
    ShellLink_ContextMenu_AddRef,
    ShellLink_ContextMenu_Release,
    ShellLink_QueryContextMenu,
    ShellLink_InvokeCommand,
    ShellLink_GetCommandString
};

static HRESULT WINAPI
ShellLink_ObjectWithSite_QueryInterface( IObjectWithSite* iface, REFIID riid, void** ppvObject )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);
    return ShellLink_QueryInterface( This, riid, ppvObject );
}

static ULONG WINAPI
ShellLink_ObjectWithSite_AddRef( IObjectWithSite* iface )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);
    return ShellLink_AddRef( This );
}

static ULONG WINAPI
ShellLink_ObjectWithSite_Release( IObjectWithSite* iface )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);
    return ShellLink_Release( This );
}

static HRESULT WINAPI
ShellLink_GetSite( IObjectWithSite *iface, REFIID iid, void ** ppvSite )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);

    TRACE("%p %s %p\n", This, debugstr_guid( iid ), ppvSite );

    if ( !This->site )
        return E_FAIL;
    return IUnknown_QueryInterface( This->site, iid, ppvSite );
}

static HRESULT WINAPI
ShellLink_SetSite( IObjectWithSite *iface, IUnknown *punk )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);

    TRACE("%p %p\n", iface, punk);

    if ( punk )
        IUnknown_AddRef( punk );
    This->site = punk;

    return S_OK;
}

static const IObjectWithSiteVtbl owsvt =
{
    ShellLink_ObjectWithSite_QueryInterface,
    ShellLink_ObjectWithSite_AddRef,
    ShellLink_ObjectWithSite_Release,
    ShellLink_SetSite,
    ShellLink_GetSite,
};
