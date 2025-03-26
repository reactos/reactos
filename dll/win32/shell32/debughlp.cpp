/*
 * Helper functions for debugging
 *
 * Copyright 1998, 2002 Juergen Schmied
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(pidl);

static inline BYTE _dbg_ILGetType(LPCITEMIDLIST pidl)
{
    return pidl && pidl->mkid.cb >= 3 ? pidl->mkid.abID[0] : 0;
}

static inline BYTE _dbg_ILGetFSType(LPCITEMIDLIST pidl)
{
    const BYTE type = _dbg_ILGetType(pidl);
    return (type & PT_FOLDERTYPEMASK) == PT_FS ? type : 0;
}

static
LPITEMIDLIST _dbg_ILGetNext(LPCITEMIDLIST pidl)
{
    WORD len;

    if(pidl)
    {
      len =  pidl->mkid.cb;
      if (len)
      {
        return (LPITEMIDLIST) (((LPBYTE)pidl)+len);
      }
    }
    return NULL;
}

static
BOOL _dbg_ILIsDesktop(LPCITEMIDLIST pidl)
{
    return ( !pidl || (pidl && pidl->mkid.cb == 0x00) );
}

static
LPPIDLDATA _dbg_ILGetDataPointer(LPCITEMIDLIST pidl)
{
    if(pidl && pidl->mkid.cb != 0x00)
      return (LPPIDLDATA) &(pidl->mkid.abID);
    return NULL;
}

static
LPSTR _dbg_ILGetTextPointer(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata =_dbg_ILGetDataPointer(pidl);

    if (pdata)
    {
      switch (pdata->type)
      {
        case PT_GUID:
        case PT_SHELLEXT:
        case PT_YAGUID:
          return NULL;

        case PT_DRIVE:
        case PT_DRIVE1:
        case PT_DRIVE2:
        case PT_DRIVE3:
          return (LPSTR)&(pdata->u.drive.szDriveName);

        case PT_FOLDER:
        case PT_FOLDER1:
        case PT_VALUE:
        case PT_IESPECIAL1:
        case PT_IESPECIAL2:
          return (LPSTR)&(pdata->u.file.szNames);

        case PT_WORKGRP:
        case PT_COMP:
        case PT_NETWORK:
        case PT_NETPROVIDER:
        case PT_SHARE:
          return (LPSTR)&(pdata->u.network.szNames);
      }
    }
    return NULL;
}

static
LPWSTR _dbg_ILGetTextPointerW(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata =_dbg_ILGetDataPointer(pidl);

    if (pdata)
    {
      if (_dbg_ILGetFSType(pidl) & PT_FS_UNICODE_FLAG)
        return (LPWSTR)&(pdata->u.file.szNames);

      switch (pdata->type)
      {
        case PT_GUID:
        case PT_SHELLEXT:
        case PT_YAGUID:
          return NULL;

        case PT_DRIVE:
        case PT_DRIVE1:
        case PT_DRIVE2:
        case PT_DRIVE3:
          /* return (LPSTR)&(pdata->u.drive.szDriveName);*/
          return NULL;

        case PT_FOLDER:
        case PT_FOLDER1:
        case PT_VALUE:
        case PT_IESPECIAL1:
        case PT_IESPECIAL2:
          /* return (LPSTR)&(pdata->u.file.szNames); */
          return NULL;

        case PT_WORKGRP:
        case PT_COMP:
        case PT_NETWORK:
        case PT_NETPROVIDER:
        case PT_SHARE:
          /* return (LPSTR)&(pdata->u.network.szNames); */
          return NULL;
      }
    }
    return NULL;
}


static
LPSTR _dbg_ILGetSTextPointer(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata =_dbg_ILGetDataPointer(pidl);

    if (pdata)
    {
      switch (pdata->type)
      {
        case PT_FOLDER:
        case PT_VALUE:
        case PT_IESPECIAL1:
        case PT_IESPECIAL2:
          return (LPSTR)(pdata->u.file.szNames + strlen (pdata->u.file.szNames) + 1);

        case PT_WORKGRP:
          return (LPSTR)(pdata->u.network.szNames + strlen (pdata->u.network.szNames) + 1);
      }
    }
    return NULL;
}

static
LPWSTR _dbg_ILGetSTextPointerW(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata =_dbg_ILGetDataPointer(pidl);

    if (pdata)
    {
      switch (pdata->type)
      {
        case PT_FOLDER:
        case PT_VALUE:
        case PT_IESPECIAL1:
        case PT_IESPECIAL2:
          /*return (LPSTR)(pdata->u.file.szNames + strlen (pdata->u.file.szNames) + 1); */
          return NULL;

        case PT_WORKGRP:
          /* return (LPSTR)(pdata->u.network.szNames + strlen (pdata->u.network.szNames) + 1); */
          return NULL;

        case PT_VALUEW:
          return (LPWSTR)(pdata->u.file.szNames + wcslen ((LPWSTR)pdata->u.file.szNames) + 1);
      }
    }
    return NULL;
}


static
IID* _dbg_ILGetGUIDPointer(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata =_ILGetDataPointer(pidl);

    if (pdata)
    {
      switch (pdata->type)
      {
        case PT_SHELLEXT:
        case PT_GUID:
            case PT_YAGUID:
          return &(pdata->u.guid.guid);
      }
    }
    return NULL;
}

static
void _dbg_ILSimpleGetText (LPCITEMIDLIST pidl, LPSTR szOut, UINT uOutSize)
{
    LPSTR        szSrc;
    LPWSTR        szSrcW;
    GUID const *     riid;

    if (!pidl) return;

    if (szOut)
      *szOut = 0;

    if (_dbg_ILIsDesktop(pidl))
    {
     /* desktop */
      if (szOut) lstrcpynA(szOut, "Desktop", uOutSize);
    }
    else if (( szSrc = _dbg_ILGetTextPointer(pidl) ))
    {
      /* filesystem */
      if (szOut) lstrcpynA(szOut, szSrc, uOutSize);
    }
    else if (( szSrcW = _dbg_ILGetTextPointerW(pidl) ))
    {
      CHAR tmp[MAX_PATH];
      /* unicode filesystem */
      WideCharToMultiByte(CP_ACP,0,szSrcW, -1, tmp, MAX_PATH, NULL, NULL);
      if (szOut) lstrcpynA(szOut, tmp, uOutSize);
    }
    else if (( riid = _dbg_ILGetGUIDPointer(pidl) ))
    {
      if (szOut)
            sprintf( szOut, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 riid->Data1, riid->Data2, riid->Data3,
                 riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                 riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7] );
    }
}




static void pdump_impl (LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidltemp = pidl;


    if (! pidltemp)
    {
      MESSAGE ("-------- pidl=NULL (Desktop)\n");
    }
    else
    {
      MESSAGE ("-------- pidl=%p\n", pidl);
      if (pidltemp->mkid.cb)
      {
        do
        {
          if (_ILIsUnicode(pidltemp))
          {
              DWORD dwAttrib = 0;
              LPPIDLDATA pData   = _dbg_ILGetDataPointer(pidltemp);
              DWORD type = pData ? pData->type : 0;
              LPWSTR szLongName   = _dbg_ILGetTextPointerW(pidltemp);
              LPWSTR szShortName  = _dbg_ILGetSTextPointerW(pidltemp);
              char szName[MAX_PATH];

              _dbg_ILSimpleGetText(pidltemp, szName, MAX_PATH);
              if (_dbg_ILGetFSType(pidltemp))
                dwAttrib = pData->u.file.uFileAttribs;

              MESSAGE ("[%p] size=%04u type=%x attr=0x%08x name=%s (%s,%s)\n",
                       pidltemp, pidltemp->mkid.cb, type, dwAttrib,
                           debugstr_a(szName), debugstr_w(szLongName), debugstr_w(szShortName));
          }
          else
          {
              DWORD dwAttrib = 0;
              LPPIDLDATA pData   = _dbg_ILGetDataPointer(pidltemp);
              DWORD type = pData ? pData->type : 0;
              LPSTR szLongName   = _dbg_ILGetTextPointer(pidltemp);
              LPSTR szShortName  = _dbg_ILGetSTextPointer(pidltemp);
              char szName[MAX_PATH];

              _dbg_ILSimpleGetText(pidltemp, szName, MAX_PATH);
              if (_dbg_ILGetFSType(pidltemp))
                dwAttrib = pData->u.file.uFileAttribs;

              MESSAGE ("[%p] size=%04u type=%x attr=0x%08x name=%s (%s,%s)\n",
                       pidltemp, pidltemp->mkid.cb, type, dwAttrib,
                           debugstr_a(szName), debugstr_a(szLongName), debugstr_a(szShortName));
          }

          pidltemp = _dbg_ILGetNext(pidltemp);

        } while (pidltemp && pidltemp->mkid.cb);
      }
      else
      {
        MESSAGE ("empty pidl (Desktop)\n");
      }
      pcheck(pidl);
    }
}

void pdump(LPCITEMIDLIST pidl)
{
    if (!TRACE_ON(pidl)) return;

    return pdump_impl(pidl);
}


void pdump_always(LPCITEMIDLIST pidl)
{
    pdump_impl(pidl);
}


static void dump_pidl_hex( LPCITEMIDLIST pidl )
{
    const unsigned char *p = (const unsigned char *)pidl;
    const int max_bytes = 0x80;
#define max_line 0x10
    char szHex[max_line*3+1], szAscii[max_line+1];
    int i, n;

    n = pidl->mkid.cb;
    if( n>max_bytes )
        n = max_bytes;
    for( i=0; i<n; i++ )
    {
        sprintf( &szHex[ (i%max_line)*3 ], "%02X ", p[i] );
        szAscii[ (i%max_line) ] = isprint( p[i] ) ? p[i] : '.';

        /* print out at the end of each line and when we're finished */
        if( i!=(n-1) && (i%max_line) != (max_line-1) )
            continue;
        szAscii[ (i%max_line)+1 ] = 0;
        ERR("%-*s   %s\n", max_line*3, szHex, szAscii );
    }
}

BOOL pcheck( LPCITEMIDLIST pidl )
{
    DWORD type;
    LPCITEMIDLIST pidltemp = pidl;

    while( pidltemp && pidltemp->mkid.cb )
    {
        LPPIDLDATA pidlData = _dbg_ILGetDataPointer(pidltemp);

        if (pidlData)
        {
            type = pidlData->type;
            switch( type )
            {
                case PT_CPLAPPLET:
                case PT_GUID:
                case PT_SHELLEXT:
                case PT_DRIVE:
                case PT_DRIVE1:
                case PT_DRIVE2:
                case PT_DRIVE3:
                case PT_FOLDER:
                case PT_VALUE:
                case PT_VALUEW:
                case PT_FOLDER1:
                case PT_WORKGRP:
                case PT_COMP:
                case PT_NETPROVIDER:
                case PT_NETWORK:
                case PT_IESPECIAL1:
                case PT_YAGUID:
                case PT_IESPECIAL2:
                case PT_SHARE:
                    break;
                default:
                    ERR("unknown IDLIST %p [%p] size=%u type=%x\n",
                        pidl, pidltemp, pidltemp->mkid.cb,type );
                    dump_pidl_hex( pidltemp );
                    return FALSE;
            }
            pidltemp = _dbg_ILGetNext(pidltemp);
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}

static const struct {
    REFIID riid;
    const char *name;
} InterfaceDesc[] = {
    {IID_IUnknown,            "IID_IUnknown"},
    {IID_IClassFactory,        "IID_IClassFactory"},
    {IID_IShellView,        "IID_IShellView"},
    {IID_IOleCommandTarget,    "IID_IOleCommandTarget"},
    {IID_IDropTarget,        "IID_IDropTarget"},
    {IID_IDropSource,        "IID_IDropSource"},
    {IID_IViewObject,        "IID_IViewObject"},
    {IID_IContextMenu,        "IID_IContextMenu"},
    {IID_IShellExtInit,        "IID_IShellExtInit"},
    {IID_IShellFolder,        "IID_IShellFolder"},
    {IID_IShellFolder2,        "IID_IShellFolder2"},
    {IID_IPersist,            "IID_IPersist"},
    {IID_IPersistFolder,        "IID_IPersistFolder"},
    {IID_IPersistFolder2,        "IID_IPersistFolder2"},
    {IID_IPersistFolder3,        "IID_IPersistFolder3"},
    {IID_IExtractIconA,        "IID_IExtractIconA"},
    {IID_IExtractIconW,        "IID_IExtractIconW"},
    {IID_IDataObject,        "IID_IDataObject"},
    {IID_IAutoComplete,            "IID_IAutoComplete"},
    {IID_IAutoComplete2,           "IID_IAutoComplete2"},
        {IID_IShellLinkA,              "IID_IShellLinkA"},
        {IID_IShellLinkW,              "IID_IShellLinkW"},
    };

const char * shdebugstr_guid( const struct _GUID *id )
{
    unsigned int i;
    const char* name = NULL;
    char clsidbuf[100];

    if (!id) return "(null)";

        for (i=0; i < sizeof(InterfaceDesc) / sizeof(InterfaceDesc[0]); i++) {
            if (IsEqualIID(InterfaceDesc[i].riid, *id)) name = InterfaceDesc[i].name;
        }
        if (!name) {
        if (HCR_GetClassNameA(*id, clsidbuf, 100))
            name = clsidbuf;
        }

            return wine_dbg_sprintf( "\n\t{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} (%s)",
                 id->Data1, id->Data2, id->Data3,
                 id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                 id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7], name ? name : "unknown" );
}
