/*
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 *   Nearly complete informations about the binary formats 
 *   of .lnk files available at http://www.wotsit.org
 *
 */

#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#include "wine/debug.h"
#include "wine/port.h"
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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* link file formats */

/* flag1: lnk elements: simple link has 0x0B */
#define SCF_PIDL   1
#define SCF_NORMAL 2
#define SCF_DESCRIPTION 4
#define SCF_RELATIVE 8
#define SCF_WORKDIR 0x10
#define SCF_ARGS 0x20
#define SCF_CUSTOMICON 0x40
#define SCF_UNICODE 0x80

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

#include "poppack.h"

static ICOM_VTABLE(IShellLinkA)		slvt;
static ICOM_VTABLE(IShellLinkW)		slvtw;
static ICOM_VTABLE(IPersistFile)	pfvt;
static ICOM_VTABLE(IPersistStream)	psvt;

/* IShellLink Implementation */

typedef struct
{
	ICOM_VFIELD(IShellLinkA);
	DWORD				ref;

	ICOM_VTABLE(IShellLinkW)*	lpvtblw;
	ICOM_VTABLE(IPersistFile)*	lpvtblPersistFile;
	ICOM_VTABLE(IPersistStream)*	lpvtblPersistStream;

	/* data structures according to the informations in the link */
	LPITEMIDLIST	pPidl;
	WORD		wHotKey;
	SYSTEMTIME	time1;
	SYSTEMTIME	time2;
	SYSTEMTIME	time3;

	DWORD		iShowCmd;
	LPWSTR		sIcoPath;
	INT		iIcoNdx;
	LPWSTR		sPath;
	LPWSTR		sArgs;
	LPWSTR		sWorkDir;
	LPWSTR		sDescription;
	LPWSTR		sPathRel;

	BOOL		bDirty;
} IShellLinkImpl;

#define _IShellLinkW_Offset ((int)(&(((IShellLinkImpl*)0)->lpvtblw)))
#define _ICOM_THIS_From_IShellLinkW(class, name) class* This = (class*)(((char*)name)-_IShellLinkW_Offset)

#define _IPersistFile_Offset ((int)(&(((IShellLinkImpl*)0)->lpvtblPersistFile)))
#define _ICOM_THIS_From_IPersistFile(class, name) class* This = (class*)(((char*)name)-_IPersistFile_Offset)

#define _IPersistStream_Offset ((int)(&(((IShellLinkImpl*)0)->lpvtblPersistStream)))
#define _ICOM_THIS_From_IPersistStream(class, name) class* This = (class*)(((char*)name)-_IPersistStream_Offset)
#define _IPersistStream_From_ICOM_THIS(class, name) class* StreamThis = (class*)(((char*)name)+_IPersistStream_Offset)


/* strdup on the process heap */
inline static LPWSTR HEAP_strdupAtoW( HANDLE heap, DWORD flags, LPCSTR str)
{
    INT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    LPWSTR p = HeapAlloc( heap, flags, len*sizeof (WCHAR) );
    if( !p )
        return p;
    MultiByteToWideChar( CP_ACP, 0, str, -1, p, len );
    return p;
}


/**************************************************************************
 *  IPersistFile_QueryInterface
 */
static HRESULT WINAPI IPersistFile_fnQueryInterface(
	IPersistFile* iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);

	TRACE("(%p)\n",This);

	return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvObj);
}

/******************************************************************************
 * IPersistFile_AddRef
 */
static ULONG WINAPI IPersistFile_fnAddRef(IPersistFile* iface)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_AddRef((IShellLinkA*)This);
}
/******************************************************************************
 * IPersistFile_Release
 */
static ULONG WINAPI IPersistFile_fnRelease(IPersistFile* iface)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_Release((IShellLinkA*)This);
}

static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile* iface, CLSID *pClassID)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
	FIXME("(%p)\n",This);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnIsDirty(IPersistFile* iface)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);

	FIXME("(%p)\n",This);

	if (This->bDirty)
	    return S_OK;

	return S_FALSE;
}
static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName, DWORD dwMode)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
	_IPersistStream_From_ICOM_THIS(IPersistStream, This);
        HRESULT r;
        IStream *stm;

        TRACE("(%p, %s)\n",This, debugstr_w(pszFileName));

        r = CreateStreamOnFile(pszFileName, dwMode, &stm);
        if( SUCCEEDED( r ) )
        {
            r = IPersistStream_Load(StreamThis, stm);
            IStream_Release( stm );
        }

        return r;
}

static BOOL StartLinkProcessor( LPCOLESTR szLink )
{
    const WCHAR szFormat[] = {'w','i','n','e','m','e','n','u','b','u','i','l','d','e','r','.','e','x','e',
                              ' ','-','r',' ','"','%','s','"',0 };
    LONG len;
    LPWSTR buffer;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    len = sizeof(szFormat) + lstrlenW( szLink ) * sizeof(WCHAR);
    buffer = HeapAlloc( GetProcessHeap(), 0, len );
    if( !buffer )
        return FALSE;

    wsprintfW( buffer, szFormat, szLink );

    TRACE("starting %s\n",debugstr_w(buffer));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW( NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return FALSE;

    /* wait for a while to throttle the creation of linker processes */
    if( WAIT_OBJECT_0 != WaitForSingleObject( pi.hProcess, 10000 ) )
        WARN("Timed out waiting for shell linker\n");

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return TRUE;
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile* iface, LPCOLESTR pszFileName, BOOL fRemember)
{
    _ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
    _IPersistStream_From_ICOM_THIS(IPersistStream, This);
    HRESULT r;
    IStream *stm;

    TRACE("(%p)->(%s)\n",This,debugstr_w(pszFileName));

    if (!pszFileName || !This->sPath)
        return ERROR_UNKNOWN;

    r = CreateStreamOnFile(pszFileName, STGM_READWRITE | STGM_CREATE, &stm);
    if( SUCCEEDED( r ) )
    {
        r = IPersistStream_Save(StreamThis, stm, FALSE);
        IStream_Release( stm );

        if( SUCCEEDED( r ) )
            StartLinkProcessor( pszFileName );
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
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
	FIXME("(%p)->(%s)\n",This,debugstr_w(pszFileName));
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnGetCurFile(IPersistFile* iface, LPOLESTR *ppszFileName)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
	FIXME("(%p)\n",This);
	return NOERROR;
}

static ICOM_VTABLE(IPersistFile) pfvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	VOID**     ppvoid)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n",This);

	return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvoid);
}

/************************************************************************
 * IPersistStream_Release
 */
static ULONG WINAPI IPersistStream_fnRelease(
	IPersistStream* iface)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n",This);

	return IShellLinkA_Release((IShellLinkA*)This);
}

/************************************************************************
 * IPersistStream_AddRef
 */
static ULONG WINAPI IPersistStream_fnAddRef(
	IPersistStream* iface)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n",This);

	return IShellLinkA_AddRef((IShellLinkA*)This);
}

/************************************************************************
 * IPersistStream_GetClassID
 *
 */
static HRESULT WINAPI IPersistStream_fnGetClassID(
	IPersistStream* iface,
	CLSID* pClassID)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n", This);

	if (pClassID==0)
	  return E_POINTER;

/*	memcpy(pClassID, &CLSID_???, sizeof(CLSID_???)); */

	return S_OK;
}

/************************************************************************
 * IPersistStream_IsDirty (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnIsDirty(
	IPersistStream*  iface)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

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
        if( str )
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

static HRESULT Stream_LoadLocation( IStream* stm )
{
    DWORD size;
    ULONG count;
    HRESULT r;
    LOCATION_INFO *loc;

    TRACE("%p\n",stm);

    r = IStream_Read( stm, &size, sizeof(size), &count );
    if( FAILED( r ) )
        return r;
    if( count != sizeof(loc->dwTotalSize) )
        return E_FAIL;

    loc = HeapAlloc( GetProcessHeap(), 0, size );
    if( ! loc )
        return E_OUTOFMEMORY;

    r = IStream_Read( stm, &loc->dwHeaderSize, size-sizeof(size), &count );
    if( FAILED( r ) )
        goto end;
    if( count != (size - sizeof(size)) )
    {
        r = E_FAIL;
        goto end;
    }
    loc->dwTotalSize = size;

    TRACE("Read %ld bytes\n",count);

    /* FIXME: do something useful with it */
    HeapFree( GetProcessHeap(), 0, loc );

    return S_OK;
end:
    HeapFree( GetProcessHeap(), 0, loc );
    return r;
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
    WCHAR    sTemp[MAX_PATH];
    HRESULT  r;

    _ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

    TRACE("(%p)(%p)\n", This, stm);

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

    if( hdr.dwFlags & SCF_PIDL )
    {
        r = ILLoadFromStream( stm, &This->pPidl );
        if( FAILED( r ) )
            return r;
    }
    This->wHotKey = (WORD)hdr.wHotKey;
    This->iIcoNdx = hdr.nIcon;
    FileTimeToSystemTime (&hdr.Time1, &This->time1);
    FileTimeToSystemTime (&hdr.Time2, &This->time2);
    FileTimeToSystemTime (&hdr.Time3, &This->time3);
#if 1
    GetDateFormatW(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&This->time1, NULL, sTemp, 256);
    TRACE("-- time1: %s\n", debugstr_w(sTemp) );
    GetDateFormatW(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&This->time2, NULL, sTemp, 256);
    TRACE("-- time1: %s\n", debugstr_w(sTemp) );
    GetDateFormatW(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&This->time3, NULL, sTemp, 256);
    TRACE("-- time1: %s\n", debugstr_w(sTemp) );
    pdump (This->pPidl);
#endif
    if( hdr.dwFlags & SCF_NORMAL )
        r = Stream_LoadLocation( stm );
    if( FAILED( r ) )
        goto end;
    unicode = hdr.dwFlags & SCF_UNICODE;
    if( hdr.dwFlags & SCF_DESCRIPTION )
    {
        r = Stream_LoadString( stm, unicode, &This->sDescription );
        TRACE("Description  -> %s\n",debugstr_w(This->sDescription));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SCF_RELATIVE )
    {
        r = Stream_LoadString( stm, unicode, &This->sPathRel );
        TRACE("Relative Path-> %s\n",debugstr_w(This->sPathRel));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SCF_WORKDIR )
    {
        r = Stream_LoadString( stm, unicode, &This->sWorkDir );
        TRACE("Working Dir  -> %s\n",debugstr_w(This->sWorkDir));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SCF_ARGS )
    {
        r = Stream_LoadString( stm, unicode, &This->sArgs );
        TRACE("Working Dir  -> %s\n",debugstr_w(This->sArgs));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SCF_CUSTOMICON )
    {
        r = Stream_LoadString( stm, unicode, &This->sIcoPath );
        TRACE("Icon file    -> %s\n",debugstr_w(This->sIcoPath));
    }
    if( FAILED( r ) )
        goto end;

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

static HRESULT Stream_WriteLocationInfo( IStream* stm, LPCWSTR filename )
{
    LOCATION_INFO loc;
    ULONG count;

    FIXME("writing empty location info\n");

    memset( &loc, 0, sizeof(loc) );
    loc.dwTotalSize = sizeof(loc) - sizeof(loc.dwTotalSize);

    /* FIXME: fill this in */

    return IStream_Write( stm, &loc, loc.dwTotalSize, &count );
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
    static const WCHAR wOpen[] = {'o','p','e','n',0};

    LINK_HEADER header;
    WCHAR   exePath[MAX_PATH];
    ULONG   count;
    HRESULT r;

    _ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

    TRACE("(%p) %p %x\n", This, stm, fClearDirty);

    *exePath = '\0';

    if (This->sPath)
	SHELL_FindExecutable(NULL, This->sPath, wOpen, exePath, NULL, NULL, NULL, NULL);

    /* if there's no PIDL, generate one */
    if( ! This->pPidl )
    {
        if( !*exePath )
            return E_FAIL;

        This->pPidl = ILCreateFromPathW(exePath);
    }

    memset(&header, 0, sizeof(header));
    header.dwSize = sizeof(header);
    memcpy(&header.MagicGuid, &CLSID_ShellLink, sizeof(header.MagicGuid) );

    header.wHotKey = This->wHotKey;
    header.nIcon = This->iIcoNdx;
    header.dwFlags = SCF_UNICODE;   /* strings are in unicode */
    header.dwFlags |= SCF_NORMAL;   /* how do we determine this ? */
    if( This->pPidl )
        header.dwFlags |= SCF_PIDL;
    if( This->sDescription )
        header.dwFlags |= SCF_DESCRIPTION;
    if( This->sWorkDir )
        header.dwFlags |= SCF_WORKDIR;
    if( This->sArgs )
        header.dwFlags |= SCF_ARGS;
    if( This->sIcoPath )
        header.dwFlags |= SCF_CUSTOMICON;

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

    TRACE("Writing pidl \n");

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

    Stream_WriteLocationInfo( stm, exePath );

    TRACE("Description = %s\n", debugstr_w(This->sDescription));
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

    return S_OK;
}

/************************************************************************
 * IPersistStream_GetSizeMax (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnGetSizeMax(
	IPersistStream*  iface,
	ULARGE_INTEGER*  pcbSize)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n", This);

	return E_NOTIMPL;
}

static ICOM_VTABLE(IPersistStream) psvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
HRESULT WINAPI IShellLink_Constructor (
	IUnknown * pUnkOuter,
	REFIID riid,
	LPVOID * ppv)
{
	IShellLinkImpl * sl;

	TRACE("unkOut=%p riid=%s\n",pUnkOuter, debugstr_guid(riid));

	*ppv = NULL;

	if(pUnkOuter) return CLASS_E_NOAGGREGATION;
	sl = (IShellLinkImpl *) LocalAlloc(GMEM_ZEROINIT,sizeof(IShellLinkImpl));
	if (!sl) return E_OUTOFMEMORY;

	sl->ref = 1;
	sl->lpVtbl = &slvt;
	sl->lpvtblw = &slvtw;
	sl->lpvtblPersistFile = &pfvt;
	sl->lpvtblPersistStream = &psvt;
	sl->iShowCmd = SW_SHOWNORMAL;
	sl->bDirty = FALSE;

	TRACE("(%p)->()\n",sl);

	if (IsEqualIID(riid, &IID_IUnknown) ||
	    IsEqualIID(riid, &IID_IShellLinkA))
	    *ppv = sl;
	else if (IsEqualIID(riid, &IID_IShellLinkW))
	    *ppv = &(sl->lpvtblw);
	else {
	    LocalFree((HLOCAL)sl);
	    ERR("E_NOINTERFACE\n");
	    return E_NOINTERFACE;
	}

	return S_OK;
}

static BOOL SHELL_ExistsFileW(LPCWSTR path)
{
    HANDLE hfile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hfile != INVALID_HANDLE_VALUE) {
	CloseHandle(hfile);
	return TRUE;
    } else
        return FALSE;
}

/**************************************************************************
 *  SHELL_ShellLink_UpdatePath
 *	update absolute path in sPath using relative path in sPathRel
 */
static HRESULT SHELL_ShellLink_UpdatePath(LPWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath)
{
    if (!path || !psPath)
	return E_INVALIDARG;

    if (!*psPath && sPathRel) {
	WCHAR buffer[2*MAX_PATH], abs_path[2*MAX_PATH];

	/* first try if [directory of link file] + [relative path] finds an existing file */
	LPCWSTR src = path;
	LPWSTR last_slash = NULL;
	LPWSTR dest = buffer;
	LPWSTR final;

	/* copy path without file name to buffer */
	while(*src) {
	    if (*src=='/' || *src=='\\')
		last_slash = dest;

	    *dest++ = *src++;
	}

	lstrcpyW(last_slash? last_slash+1: buffer, sPathRel);

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
HRESULT WINAPI IShellLink_ConstructFromFile (
	IUnknown* pUnkOuter,
	REFIID riid,
	LPCITEMIDLIST pidl,
	LPVOID* ppv
)
{
    IShellLinkW* psl;

    HRESULT hr = IShellLink_Constructor(NULL, riid, (LPVOID*)&psl);

    if (SUCCEEDED(hr)) {
	IPersistFile* ppf;

	*ppv = NULL;

	hr = IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

	if (SUCCEEDED(hr)) {
	    WCHAR path[MAX_PATH];

	    if (SHGetPathFromIDListW(pidl, path)) {
		hr = IPersistFile_Load(ppf, path, 0);

		if (SUCCEEDED(hr)) {
		    *ppv = (IUnknown*) psl;

		    /*
			The following code is here, not in IPersistStream_fnLoad() because
			to be able to convert the relative path into the absolute path,
			we need to know the path of the shell link file.
		    */
		    if (IsEqualIID(riid, &IID_IShellLinkW)) {
			_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, psl);

			hr = SHELL_ShellLink_UpdatePath(This->sPathRel, path, This->sWorkDir, &This->sPath);
		    } else {
			ICOM_THIS(IShellLinkImpl, psl);

			hr = SHELL_ShellLink_UpdatePath(This->sPathRel, path, This->sWorkDir, &This->sPath);
		    }
		}
	    }

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
static HRESULT WINAPI IShellLinkA_fnQueryInterface( IShellLinkA * iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IShellLinkImpl, iface);

	TRACE("(%p)->(\n\tIID:\t%s)\n",This,debugstr_guid(riid));

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown) ||
	   IsEqualIID(riid, &IID_IShellLinkA))
	{
	  *ppvObj = This;
	}
	else if(IsEqualIID(riid, &IID_IShellLinkW))
	{
	  *ppvObj = (IShellLinkW *)&(This->lpvtblw);
	}
	else if(IsEqualIID(riid, &IID_IPersistFile))
	{
	  *ppvObj = (IPersistFile *)&(This->lpvtblPersistFile);
	}
	else if(IsEqualIID(riid, &IID_IPersistStream))
	{
	  *ppvObj = (IPersistStream *)&(This->lpvtblPersistStream);
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)(*ppvObj));
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/******************************************************************************
 * IShellLinkA_AddRef
 */
static ULONG WINAPI IShellLinkA_fnAddRef(IShellLinkA * iface)
{
	ICOM_THIS(IShellLinkImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return ++(This->ref);
}

/******************************************************************************
 *	IShellLinkA_Release
 */
static ULONG WINAPI IShellLinkA_fnRelease(IShellLinkA * iface)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(count=%lu)\n",This,This->ref);

    if (--(This->ref))
        return This->ref;

    TRACE("-- destroying IShellLink(%p)\n",This);

    if (This->sIcoPath)
        HeapFree(GetProcessHeap(), 0, This->sIcoPath);

    if (This->sArgs)
        HeapFree(GetProcessHeap(), 0, This->sArgs);

    if (This->sWorkDir)
        HeapFree(GetProcessHeap(), 0, This->sWorkDir);

    if (This->sDescription)
        HeapFree(GetProcessHeap(), 0, This->sDescription);

    if (This->sPath)
        HeapFree(GetProcessHeap(), 0, This->sPath);

    if (This->pPidl)
        ILFree(This->pPidl);

    LocalFree((HANDLE)This);

    return 0;
}

static HRESULT WINAPI IShellLinkA_fnGetPath(IShellLinkA * iface, LPSTR pszFile,
                  INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%lu)(%s)\n",
          This, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(This->sPath));

    if( cchMaxPath )
        pszFile[0] = 0;
    if (This->sPath)
        WideCharToMultiByte( CP_ACP, 0, This->sPath, -1,
                             pszFile, cchMaxPath, NULL, NULL);

	if (pfd) {

		FIXME("(%p): WIN32_FIND_DATA is not yet filled.\n", This);

	}

    return NOERROR;
}

static HRESULT WINAPI IShellLinkA_fnGetIDList(IShellLinkA * iface, LPITEMIDLIST * ppidl)
{
	ICOM_THIS(IShellLinkImpl, iface);

	TRACE("(%p)->(ppidl=%p)\n",This, ppidl);

	*ppidl = ILClone(This->pPidl);

	return NOERROR;
}

static HRESULT WINAPI IShellLinkA_fnSetIDList(IShellLinkA * iface, LPCITEMIDLIST pidl)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(pidl=%p)\n",This, pidl);

    if (This->pPidl)
        ILFree(This->pPidl);

    This->pPidl = ILClone (pidl);
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetDescription(IShellLinkA * iface, LPSTR pszName,INT cchMaxName)
{
    ICOM_THIS(IShellLinkImpl, iface);

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
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(pName=%s)\n", This, pszName);

    if (This->sDescription)
        HeapFree(GetProcessHeap(), 0, This->sDescription);
    This->sDescription = HEAP_strdupAtoW( GetProcessHeap(), 0, pszName);
    if ( !This->sDescription )
        return E_OUTOFMEMORY;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetWorkingDirectory(IShellLinkA * iface, LPSTR pszDir,INT cchMaxPath)
{
    ICOM_THIS(IShellLinkImpl, iface);

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
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(dir=%s)\n",This, pszDir);

    if (This->sWorkDir)
        HeapFree(GetProcessHeap(), 0, This->sWorkDir);
    This->sWorkDir = HEAP_strdupAtoW( GetProcessHeap(), 0, pszDir);
    if ( !This->sWorkDir )
        return E_OUTOFMEMORY;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetArguments(IShellLinkA * iface, LPSTR pszArgs,INT cchMaxPath)
{
    ICOM_THIS(IShellLinkImpl, iface);

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
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(args=%s)\n",This, pszArgs);

    if (This->sArgs)
        HeapFree(GetProcessHeap(), 0, This->sArgs);
    This->sArgs = HEAP_strdupAtoW( GetProcessHeap(), 0, pszArgs);
    if( !This->sArgs )
        return E_OUTOFMEMORY;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetHotkey(IShellLinkA * iface, WORD *pwHotkey)
{
	ICOM_THIS(IShellLinkImpl, iface);

	TRACE("(%p)->(%p)(0x%08x)\n",This, pwHotkey, This->wHotKey);

	*pwHotkey = This->wHotKey;

	return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetHotkey(IShellLinkA * iface, WORD wHotkey)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(hotkey=%x)\n",This, wHotkey);

    This->wHotKey = wHotkey;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnGetShowCmd(IShellLinkA * iface, INT *piShowCmd)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(%p)\n",This, piShowCmd);
    *piShowCmd = This->iShowCmd;
    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetShowCmd(IShellLinkA * iface, INT iShowCmd)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p) %d\n",This, iShowCmd);

    This->iShowCmd = iShowCmd;
    This->bDirty = TRUE;

    return NOERROR;
}

static HRESULT SHELL_PidlGeticonLocationA(IShellFolder* psf, LPITEMIDLIST pidl, LPSTR pszIconPath, int cchIconPath, int* piIcon)
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

static HRESULT WINAPI IShellLinkA_fnGetIconLocation(IShellLinkA * iface, LPSTR pszIconPath, INT cchIconPath, INT *piIcon)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(%p len=%u iicon=%p)\n", This, pszIconPath, cchIconPath, piIcon);

    if (cchIconPath)
        pszIconPath[0] = 0;

    if (This->sIcoPath) {
        WideCharToMultiByte(CP_ACP, 0, This->sIcoPath, -1, pszIconPath, cchIconPath, NULL, NULL);
	*piIcon = This->iIcoNdx;
	return S_OK;
    }

    if (This->pPidl || This->sPath) {
	IShellFolder* pdsk;

	HRESULT hr = SHGetDesktopFolder(&pdsk);

	if (SUCCEEDED(hr)) {
	    /* first look for an icon using the PIDL (if present) */
	    if (This->pPidl)
		hr = SHELL_PidlGeticonLocationA(pdsk, This->pPidl, pszIconPath, cchIconPath, piIcon);
	    else
		hr = E_FAIL;

	    /* if we couldn't find an icon yet, look for it using the file system path */
	    if (FAILED(hr) && This->sPath) {
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
    } else
        return E_FAIL;
}

static HRESULT WINAPI IShellLinkA_fnSetIconLocation(IShellLinkA * iface, LPCSTR pszIconPath, INT iIcon)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(path=%s iicon=%u)\n",This, pszIconPath, iIcon);

    if (This->sIcoPath)
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
    ICOM_THIS(IShellLinkImpl, iface);

    FIXME("(%p)->(path=%s %lx)\n",This, pszPathRel, dwReserved);

    if (This->sPathRel)
        HeapFree(GetProcessHeap(), 0, This->sPathRel);

    This->sPathRel = HEAP_strdupAtoW(GetProcessHeap(), 0, pszPathRel);
    This->bDirty = TRUE;

    return SHELL_ShellLink_UpdatePath(This->sPathRel, This->sPath, This->sWorkDir, &This->sPath);
}

static HRESULT WINAPI IShellLinkA_fnResolve(IShellLinkA * iface, HWND hwnd, DWORD fFlags)
{
    HRESULT hr = S_OK;

    ICOM_THIS(IShellLinkImpl, iface);

    FIXME("(%p)->(hwnd=%p flags=%lx)\n",This, hwnd, fFlags);

    /*FIXME: use IResolveShellLink interface */

    if (!This->sPath && This->pPidl) {
	WCHAR buffer[MAX_PATH];

	hr = SHELL_GetPathFromIDListW(This->pPidl, buffer, MAX_PATH);

	if (SUCCEEDED(hr) && *buffer) {
	    This->sPath = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (lstrlenW(buffer)+1)*sizeof(WCHAR));
	    if (!This->sPath)
		return E_OUTOFMEMORY;

	    lstrcpyW(This->sPath, buffer);

	    This->bDirty = TRUE;
	} else
	    hr = S_OK;    /* don't report any error occured while just caching information */
    }

    if (!This->sIcoPath && This->sPath) {
	This->sIcoPath = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (lstrlenW(This->sPath)+1)*sizeof(WCHAR));
	if (!This->sIcoPath)
	    return E_OUTOFMEMORY;

	lstrcpyW(This->sIcoPath, This->sPath);
	This->iIcoNdx = 0;

	This->bDirty = TRUE;
    }

    return hr;
}

static HRESULT WINAPI IShellLinkA_fnSetPath(IShellLinkA * iface, LPCSTR pszFile)
{
    ICOM_THIS(IShellLinkImpl, iface);

    TRACE("(%p)->(path=%s)\n",This, pszFile);

    if (This->sPath)
        HeapFree(GetProcessHeap(), 0, This->sPath);
    This->sPath = HEAP_strdupAtoW(GetProcessHeap(), 0, pszFile);
    if( !This->sPath )
        return E_OUTOFMEMORY;

    This->bDirty = TRUE;

    return S_OK;
}

/**************************************************************************
* IShellLink Implementation
*/

static ICOM_VTABLE(IShellLinkA) slvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

	return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvObj);
}

/******************************************************************************
 * IShellLinkW_fnAddRef
 */
static ULONG WINAPI IShellLinkW_fnAddRef(IShellLinkW * iface)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_AddRef((IShellLinkA*)This);
}
/******************************************************************************
 * IShellLinkW_fnRelease
 */

static ULONG WINAPI IShellLinkW_fnRelease(IShellLinkW * iface)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_Release((IShellLinkA*)This);
}

static HRESULT WINAPI IShellLinkW_fnGetPath(IShellLinkW * iface, LPWSTR pszFile,INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%lu)\n",
		  This, pszFile, cchMaxPath, pfd, fFlags);

    if( cchMaxPath )
        pszFile[0] = 0;
    if( This->sPath )
        lstrcpynW( pszFile, This->sPath, cchMaxPath );

	if (pfd) {

		FIXME("(%p): WIN32_FIND_DATA is not yet filled.\n", This);

	}

    return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetIDList(IShellLinkW * iface, LPITEMIDLIST * ppidl)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(ppidl=%p)\n",This, ppidl);

    if( This->pPidl)
        *ppidl = ILClone( This->pPidl );
    else
        *ppidl = NULL;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetIDList(IShellLinkW * iface, LPCITEMIDLIST pidl)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

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
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(%p len=%u)\n",This, pszName, cchMaxName);

    if( cchMaxName )
        pszName[0] = 0;
    if( This->sDescription )
        lstrcpynW( pszName, This->sDescription, cchMaxName );

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetDescription(IShellLinkW * iface, LPCWSTR pszName)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(desc=%s)\n",This, debugstr_w(pszName));

    if (This->sDescription)
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
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(%p len %u)\n", This, pszDir, cchMaxPath);

    if( cchMaxPath )
        pszDir[0] = 0;
    if( This->sWorkDir )
        lstrcpynW( pszDir, This->sWorkDir, cchMaxPath );

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetWorkingDirectory(IShellLinkW * iface, LPCWSTR pszDir)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(dir=%s)\n",This, debugstr_w(pszDir));

    if (This->sWorkDir)
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
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(%p len=%u)\n", This, pszArgs, cchMaxPath);

    if( cchMaxPath )
        pszArgs[0] = 0;
    if( This->sArgs )
        lstrcpynW( pszArgs, This->sArgs, cchMaxPath );

    return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetArguments(IShellLinkW * iface, LPCWSTR pszArgs)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(args=%s)\n",This, debugstr_w(pszArgs));

    if (This->sArgs)
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
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(%p)\n",This, pwHotkey);

    *pwHotkey=This->wHotKey;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetHotkey(IShellLinkW * iface, WORD wHotkey)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(hotkey=%x)\n",This, wHotkey);

    This->wHotKey = wHotkey;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetShowCmd(IShellLinkW * iface, INT *piShowCmd)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(%p)\n",This, piShowCmd);

    *piShowCmd = This->iShowCmd;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetShowCmd(IShellLinkW * iface, INT iShowCmd)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    This->iShowCmd = iShowCmd;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT SHELL_PidlGeticonLocationW(IShellFolder* psf, LPITEMIDLIST pidl, LPWSTR pszIconPath, int cchIconPath, int* piIcon)
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

static HRESULT WINAPI IShellLinkW_fnGetIconLocation(IShellLinkW * iface, LPWSTR pszIconPath, INT cchIconPath, INT *piIcon)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(%p len=%u iicon=%p)\n", This, pszIconPath, cchIconPath, piIcon);

    if (cchIconPath)
        pszIconPath[0] = 0;

    if (This->sIcoPath) {
	lstrcpynW(pszIconPath, This->sIcoPath, cchIconPath);
	*piIcon = This->iIcoNdx;
	return S_OK;
    }

    if (This->pPidl || This->sPath) {
	IShellFolder* pdsk;

	HRESULT hr = SHGetDesktopFolder(&pdsk);

	if (SUCCEEDED(hr)) {
	    /* first look for an icon using the PIDL (if present) */
	    if (This->pPidl)
		hr = SHELL_PidlGeticonLocationW(pdsk, This->pPidl, pszIconPath, cchIconPath, piIcon);
	    else
		hr = E_FAIL;

	    /* if we couldn't find an icon yet, look for it using the file system path */
	    if (FAILED(hr) && This->sPath) {
		LPITEMIDLIST pidl;

		hr = IShellFolder_ParseDisplayName(pdsk, 0, NULL, This->sPath, NULL, &pidl, NULL);

		if (SUCCEEDED(hr)) {
		    hr = SHELL_PidlGeticonLocationW(pdsk, pidl, pszIconPath, cchIconPath, piIcon);

		    SHFree(pidl);
		}
	    }

	    IShellFolder_Release(pdsk);
	}

	return hr;
    } else
        return E_FAIL;
}

static HRESULT WINAPI IShellLinkW_fnSetIconLocation(IShellLinkW * iface, LPCWSTR pszIconPath, INT iIcon)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(path=%s iicon=%u)\n",This, debugstr_w(pszIconPath), iIcon);

    if (This->sIcoPath)
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
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(path=%s %lx)\n",This, debugstr_w(pszPathRel), dwReserved);

    if (This->sPathRel)
        HeapFree(GetProcessHeap(), 0, This->sPathRel);
    This->sPathRel = HeapAlloc( GetProcessHeap(), 0,
                                (lstrlenW( pszPathRel )+1) * sizeof (WCHAR) );
    if ( !This->sPathRel )
        return E_OUTOFMEMORY;

    lstrcpyW( This->sPathRel, pszPathRel );
    This->bDirty = TRUE;

    return SHELL_ShellLink_UpdatePath(This->sPathRel, This->sPath, This->sWorkDir, &This->sPath);
}

static HRESULT WINAPI IShellLinkW_fnResolve(IShellLinkW * iface, HWND hwnd, DWORD fFlags)
{
    HRESULT hr = S_OK;

    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    FIXME("(%p)->(hwnd=%p flags=%lx)\n",This, hwnd, fFlags);

    /*FIXME: use IResolveShellLink interface */

    if (!This->sPath && This->pPidl) {
	WCHAR buffer[MAX_PATH];

	hr = SHELL_GetPathFromIDListW(This->pPidl, buffer, MAX_PATH);

	if (SUCCEEDED(hr) && *buffer) {
	    This->sPath = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (lstrlenW(buffer)+1)*sizeof(WCHAR));
	    if (!This->sPath)
		return E_OUTOFMEMORY;

	    lstrcpyW(This->sPath, buffer);

	    This->bDirty = TRUE;
	} else
	    hr = S_OK;    /* don't report any error occured while just caching information */
    }

    if (!This->sIcoPath && This->sPath) {
	This->sIcoPath = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (lstrlenW(This->sPath)+1)*sizeof(WCHAR));
	if (!This->sIcoPath)
	    return E_OUTOFMEMORY;

	lstrcpyW(This->sIcoPath, This->sPath);
	This->iIcoNdx = 0;

	This->bDirty = TRUE;
    }

    return hr;
}

static HRESULT WINAPI IShellLinkW_fnSetPath(IShellLinkW * iface, LPCWSTR pszFile)
{
    _ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);

    TRACE("(%p)->(path=%s)\n",This, debugstr_w(pszFile));

    if (This->sPath)
        HeapFree(GetProcessHeap(), 0, This->sPath);
    This->sPath = HeapAlloc( GetProcessHeap(), 0,
                             (lstrlenW( pszFile )+1) * sizeof (WCHAR) );
    if ( !This->sPath )
        return E_OUTOFMEMORY;

    lstrcpyW( This->sPath, pszFile );
    This->bDirty = TRUE;

    return S_OK;
}

/**************************************************************************
* IShellLinkW Implementation
*/

static ICOM_VTABLE(IShellLinkW) slvtw =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
