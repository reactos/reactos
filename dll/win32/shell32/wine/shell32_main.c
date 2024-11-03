/*
 *                 Shell basics
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
 * Copyright 2017 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#include <wine/config.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <winnls.h>

#include "undocshell.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"

#include <wine/debug.h>
#include <wine/unicode.h>

#include <reactos/version.h>
#include <reactos/buildno.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

const char * const SHELL_Authors[] = { "Copyright 1993-"COPYRIGHT_YEAR" WINE team", "Copyright 1998-"COPYRIGHT_YEAR" ReactOS Team", 0 };

/*************************************************************************
 * CommandLineToArgvW            [SHELL32.@]
 *
 * We must interpret the quotes in the command line to rebuild the argv
 * array correctly:
 * - arguments are separated by spaces or tabs
 * - quotes serve as optional argument delimiters
 *   '"a b"'   -> 'a b'
 * - escaped quotes must be converted back to '"'
 *   '\"'      -> '"'
 * - consecutive backslashes preceding a quote see their number halved with
 *   the remainder escaping the quote:
 *   2n   backslashes + quote -> n backslashes + quote as an argument delimiter
 *   2n+1 backslashes + quote -> n backslashes + literal quote
 * - backslashes that are not followed by a quote are copied literally:
 *   'a\b'     -> 'a\b'
 *   'a\\b'    -> 'a\\b'
 * - in quoted strings, consecutive quotes see their number divided by three
 *   with the remainder modulo 3 deciding whether to close the string or not.
 *   Note that the opening quote must be counted in the consecutive quotes,
 *   that's the (1+) below:
 *   (1+) 3n   quotes -> n quotes
 *   (1+) 3n+1 quotes -> n quotes plus closes the quoted string
 *   (1+) 3n+2 quotes -> n+1 quotes plus closes the quoted string
 * - in unquoted strings, the first quote opens the quoted string and the
 *   remaining consecutive quotes follow the above rule.
 */
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR lpCmdline, int* numargs)
{
    DWORD argc;
    LPWSTR  *argv;
    LPCWSTR s;
    LPWSTR d;
    LPWSTR cmdline;
    int qcount,bcount;

    if(!numargs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (*lpCmdline==0)
    {
        /* Return the path to the executable */
        DWORD len, deslen=MAX_PATH, size;

        size = sizeof(LPWSTR)*2 + deslen*sizeof(WCHAR);
        for (;;)
        {
            if (!(argv = LocalAlloc(LMEM_FIXED, size))) return NULL;
            len = GetModuleFileNameW(0, (LPWSTR)(argv+2), deslen);
            if (!len)
            {
                LocalFree(argv);
                return NULL;
            }
            if (len < deslen) break;
            deslen*=2;
            size = sizeof(LPWSTR)*2 + deslen*sizeof(WCHAR);
            LocalFree( argv );
        }
        argv[0]=(LPWSTR)(argv+2);
        argv[1]=NULL;
        *numargs=1;

        return argv;
    }

    /* --- First count the arguments */
    argc=1;
    s=lpCmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s=='"')
    {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++=='"')
                break;
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*s && !isspace(*s))
            s++;
    }
    /* skip to the first argument, if any */
    while (isblank(*s))
        s++;
    if (*s)
        argc++;

    /* Analyze the remaining arguments */
    qcount=bcount=0;
    while (*s)
    {
        if (isblank(*s) && qcount==0)
        {
            /* skip to the next argument and count it if any */
            while (isblank(*s))
                s++;
            if (*s)
                argc++;
            bcount=0;
        }
        else if (*s=='\\')
        {
            /* '\', count them */
            bcount++;
            s++;
        }
        else if (*s=='"')
        {
            /* '"' */
            if ((bcount & 1)==0)
                qcount++; /* unescaped '"' */
            s++;
            bcount=0;
            /* consecutive quotes, see comment in copying code below */
            while (*s=='"')
            {
                qcount++;
                s++;
            }
            qcount=qcount % 3;
            if (qcount==2)
                qcount=0;
        }
        else
        {
            /* a regular character */
            bcount=0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv=LocalAlloc(LMEM_FIXED, (argc+1)*sizeof(LPWSTR)+(strlenW(lpCmdline)+1)*sizeof(WCHAR));
    if (!argv)
        return NULL;
    cmdline=(LPWSTR)(argv+argc+1);
    strcpyW(cmdline, lpCmdline);

    /* --- Then split and copy the arguments */
    argv[0]=d=cmdline;
    argc=1;
    /* The first argument, the executable path, follows special rules */
    if (*d=='"')
    {
        /* The executable path ends at the next quote, no matter what */
        s=d+1;
        while (*s)
        {
            if (*s=='"')
            {
                s++;
                break;
            }
            *d++=*s++;
        }
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*d && !isspace(*d))
            d++;
        s=d;
        if (*s)
            s++;
    }
    /* close the executable path */
    *d++=0;
    /* skip to the first argument and initialize it if any */
    while (isblank(*s))
        s++;

    if (!*s)
    {
        /* There are no parameters so we are all done */
        argv[argc]=NULL;
        *numargs=argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++]=d;
    qcount=bcount=0;
    while (*s)
    {
        if (isblank(*s) && qcount==0)
        {
            /* close the argument */
            *d++=0;
            bcount=0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (isblank(*s));
            if (*s)
                argv[argc++]=d;
        }
        else if (*s=='\\')
        {
            *d++=*s++;
            bcount++;
        }
        else if (*s=='"')
        {
            if ((bcount & 1)==0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d-=bcount/2;
                qcount++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d=d-bcount/2-1;
                *d++='"';
            }
            s++;
            bcount=0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s=='"')
            {
                if (++qcount==3)
                {
                    *d++='"';
                    qcount=0;
                }
                s++;
            }
            if (qcount==2)
                qcount=0;
        }
        else
        {
            /* a regular character */
            *d++=*s++;
            bcount=0;
        }
    }
    *d='\0';
    argv[argc]=NULL;
    *numargs=argc;

    return argv;
}

static HRESULT SHELL_GetDetailsOfToBuffer(IShellFolder *psf, PCUITEMID_CHILD pidl,
                                          UINT col, LPWSTR Buf, UINT cchBuf)
{
    IShellFolder2 *psf2;
    IShellDetails *psd;
    SHELLDETAILS details;
    HRESULT hr = IShellFolder_QueryInterface(psf, &IID_IShellFolder2, (void**)&psf2);
    if (SUCCEEDED(hr))
    {
        hr = IShellFolder2_GetDetailsOf(psf2, pidl, col, &details);
        IShellFolder2_Release(psf2);
    }
    else if (SUCCEEDED(hr = IShellFolder_QueryInterface(psf, &IID_IShellDetails, (void**)&psd)))
    {
        hr = IShellDetails_GetDetailsOf(psd, pidl, col, &details);
        IShellDetails_Release(psd);
    }
    if (SUCCEEDED(hr))
        hr = StrRetToStrNW(Buf, cchBuf, &details.str, pidl) ? S_OK : E_FAIL;
    return hr;
}

static DWORD shgfi_get_exe_type(LPCWSTR szFullPath)
{
    BOOL status = FALSE;
    HANDLE hfile;
    DWORD BinaryType;
    IMAGE_DOS_HEADER mz_header;
    IMAGE_NT_HEADERS nt;
    DWORD len;
    char magic[4];

    status = GetBinaryTypeW (szFullPath, &BinaryType);
    if (!status)
        return 0;
    if (BinaryType == SCS_DOS_BINARY || BinaryType == SCS_PIF_BINARY)
        return 0x4d5a;

    hfile = CreateFileW( szFullPath, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, 0 );
    if ( hfile == INVALID_HANDLE_VALUE )
        return 0;

    /*
     * The next section is adapted from MODULE_GetBinaryType, as we need
     * to examine the image header to get OS and version information. We
     * know from calling GetBinaryTypeA that the image is valid and either
     * an NE or PE, so much error handling can be omitted.
     * Seek to the start of the file and read the header information.
     */

    SetFilePointer( hfile, 0, NULL, SEEK_SET );
    ReadFile( hfile, &mz_header, sizeof(mz_header), &len, NULL );

    SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET );
    ReadFile( hfile, magic, sizeof(magic), &len, NULL );
    if ( *(DWORD*)magic == IMAGE_NT_SIGNATURE )
    {
        SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET );
        ReadFile( hfile, &nt, sizeof(nt), &len, NULL );
        CloseHandle( hfile );
        /* DLL files are not executable and should return 0 */
        if (nt.FileHeader.Characteristics & IMAGE_FILE_DLL)
            return 0;
        if (nt.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
        {
             return IMAGE_NT_SIGNATURE | 
                   (nt.OptionalHeader.MajorSubsystemVersion << 24) |
                   (nt.OptionalHeader.MinorSubsystemVersion << 16);
        }
        return IMAGE_NT_SIGNATURE;
    }
    else if ( *(WORD*)magic == IMAGE_OS2_SIGNATURE )
    {
        IMAGE_OS2_HEADER ne;
        SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET );
        ReadFile( hfile, &ne, sizeof(ne), &len, NULL );
        CloseHandle( hfile );
        if (ne.ne_exetyp == 2)
            return IMAGE_OS2_SIGNATURE | (ne.ne_expver << 16);
        return 0;
    }
    CloseHandle( hfile );
    return 0;
}

/*************************************************************************
 * SHELL_IsShortcut		[internal]
 *
 * Decide if an item id list points to a shell shortcut
 */
BOOL SHELL_IsShortcut(LPCITEMIDLIST pidlLast)
{
    WCHAR szTemp[MAX_PATH];
    HKEY keyCls;
    BOOL ret = FALSE;

    if (_ILGetExtension(pidlLast, szTemp, _countof(szTemp)) &&
        SUCCEEDED(HCR_GetProgIdKeyOfExtension(szTemp, &keyCls, FALSE)))
    {
        ret = RegQueryValueExW(keyCls, L"IsShortcut", NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
        RegCloseKey(keyCls);
    }
    return ret;
}

#define SHGFI_KNOWN_FLAGS \
    (SHGFI_SMALLICON | SHGFI_OPENICON | SHGFI_SHELLICONSIZE | SHGFI_PIDL | \
     SHGFI_USEFILEATTRIBUTES | SHGFI_ADDOVERLAYS | SHGFI_OVERLAYINDEX | \
     SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_TYPENAME | SHGFI_ATTRIBUTES | \
     SHGFI_ICONLOCATION | SHGFI_EXETYPE | SHGFI_SYSICONINDEX | \
     SHGFI_LINKOVERLAY | SHGFI_SELECTED | SHGFI_ATTR_SPECIFIED)

/*************************************************************************
 * SHGetFileInfoW            [SHELL32.@]
 *
 */
DWORD_PTR WINAPI SHGetFileInfoW(LPCWSTR path,DWORD dwFileAttributes,
                                SHFILEINFOW *psfi, UINT sizeofpsfi, UINT flags )
{
    WCHAR szLocation[MAX_PATH], szFullPath[MAX_PATH];
    int iIndex;
    DWORD_PTR ret = TRUE;
    DWORD dwAttributes = 0;
    IShellFolder * psfParent = NULL;
    IExtractIconW * pei = NULL;
    LPITEMIDLIST pidlLast = NULL, pidl = NULL, pidlFree = NULL;
    HRESULT hr = S_OK;
    BOOL IconNotYetLoaded=TRUE;
    UINT uGilFlags = 0;
    HIMAGELIST big_icons, small_icons;

    TRACE("%s fattr=0x%x sfi=%p(attr=0x%08x) size=0x%x flags=0x%x\n",
          (flags & SHGFI_PIDL)? "pidl" : debugstr_w(path), dwFileAttributes,
          psfi, psfi ? psfi->dwAttributes : 0, sizeofpsfi, flags);

    if (!path)
        return FALSE;

    /* windows initializes these values regardless of the flags */
    if (psfi != NULL)
    {
        psfi->szDisplayName[0] = '\0';
        psfi->szTypeName[0] = '\0';
        psfi->hIcon = NULL;
    }

    if (!(flags & SHGFI_PIDL))
    {
        /* SHGetFileInfo should work with absolute and relative paths */
        if (PathIsRelativeW(path))
        {
            GetCurrentDirectoryW(MAX_PATH, szLocation);
            PathCombineW(szFullPath, szLocation, path);
        }
        else
        {
            lstrcpynW(szFullPath, path, MAX_PATH);
        }

        if ((flags & SHGFI_TYPENAME) && !PathIsRootW(szFullPath))
        {
            HRESULT hr2;
            if (!(flags & SHGFI_USEFILEATTRIBUTES))
            {
                dwFileAttributes = GetFileAttributesW(szFullPath);
                if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
                    dwFileAttributes = 0;
            }
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                hr2 = SHELL32_AssocGetFSDirectoryDescription(psfi->szTypeName, _countof(psfi->szTypeName));
            else
                hr2 = SHELL32_AssocGetFileDescription(path, psfi->szTypeName, _countof(psfi->szTypeName));
            if (SUCCEEDED(hr2))
            {
                flags &= ~SHGFI_TYPENAME;
                if (!(flags & ~SHGFI_USEFILEATTRIBUTES))
                    return ret; /* Bail out early if this was our only operation */
            }
        }
    }
    else
    {
        SHGetPathFromIDListW((LPITEMIDLIST)path, szFullPath);
    }

    if (flags & SHGFI_EXETYPE)
    {
        if (!(flags & SHGFI_SYSICONINDEX))
        {
            if (flags & SHGFI_USEFILEATTRIBUTES)
            {
                return TRUE;
            }
            else if (GetFileAttributesW(szFullPath) != INVALID_FILE_ATTRIBUTES)
            {
                return shgfi_get_exe_type(szFullPath);
            }
        }
    }

    /*
     * psfi is NULL normally to query EXE type. If it is NULL, none of the
     * below makes sense anyway. Windows allows this and just returns FALSE
     */
    if (psfi == NULL)
        return FALSE;

    /*
     * translate the path into a pidl only when SHGFI_USEFILEATTRIBUTES
     * is not specified.
     * The pidl functions fail on not existing file names
     */

    if (flags & SHGFI_PIDL)
    {
        pidl = (LPITEMIDLIST)path;
        hr = pidl ? S_OK : E_FAIL;
    }
    else
    {
        if (flags & SHGFI_USEFILEATTRIBUTES)
        {
            pidl = SHELL32_CreateSimpleIDListFromPath(szFullPath, dwFileAttributes);
            hr = pidl ? S_OK : E_FAIL;
        }
        else
        {
            hr = SHILCreateFromPathW(szFullPath, &pidl, &dwAttributes);
        }
        pidlFree = pidl;
    }

    if (SUCCEEDED(hr))
    {
        hr = SHBindToParent(pidl, &IID_IShellFolder, (void**)&psfParent, (LPCITEMIDLIST*)&pidlLast);
    }

    /* get the attributes of the child */
    if (SUCCEEDED(hr) && (flags & SHGFI_ATTRIBUTES))
    {
        if (!(flags & SHGFI_ATTR_SPECIFIED))
        {
            psfi->dwAttributes = 0xffffffff;
        }
        hr = IShellFolder_GetAttributesOf(psfParent, 1, (LPCITEMIDLIST*)&pidlLast, &psfi->dwAttributes);
    }

    if (flags & SHGFI_USEFILEATTRIBUTES)
    {
        if (flags & SHGFI_ICON)
        {
            psfi->dwAttributes = 0;
        }
    }

    /* get the displayname */
    if (SUCCEEDED(hr) && (flags & SHGFI_DISPLAYNAME))
    {
        STRRET str;
        psfi->szDisplayName[0] = UNICODE_NULL;
        hr = IShellFolder_GetDisplayNameOf(psfParent, pidlLast, SHGDN_INFOLDER, &str);
        if (SUCCEEDED(hr))
            StrRetToStrNW(psfi->szDisplayName, _countof(psfi->szDisplayName), &str, pidlLast);
    }

    /* get the type name */
    if (SUCCEEDED(hr) && (flags & SHGFI_TYPENAME))
    {
        /* FIXME: Use IShellFolder2::GetDetailsEx */
        UINT col = _ILIsDrive(pidlLast) ? 1 : 2; /* SHFSF_COL_TYPE */
        psfi->szTypeName[0] = UNICODE_NULL;
        hr = SHELL_GetDetailsOfToBuffer(psfParent, pidlLast, col, psfi->szTypeName, _countof(psfi->szTypeName));
    }

    /* ### icons ###*/

    Shell_GetImageLists( &big_icons, &small_icons );

    if (flags & SHGFI_OPENICON)
        uGilFlags |= GIL_OPENICON;

    if (flags & SHGFI_LINKOVERLAY)
        uGilFlags |= GIL_FORSHORTCUT;
    else if ((flags&SHGFI_ADDOVERLAYS) ||
             (flags&(SHGFI_ICON|SHGFI_SMALLICON))==SHGFI_ICON)
    {
        if (SHELL_IsShortcut(pidlLast))
            uGilFlags |= GIL_FORSHORTCUT;
    }

    if (flags & SHGFI_OVERLAYINDEX)
        FIXME("SHGFI_OVERLAYINDEX unhandled\n");

    if (flags & SHGFI_SELECTED)
        FIXME("set icon to selected, stub\n");

    if (flags & SHGFI_SHELLICONSIZE)
        FIXME("set icon to shell size, stub\n");

    /* get the iconlocation */
    if (SUCCEEDED(hr) && (flags & SHGFI_ICONLOCATION ))
    {
        UINT uDummy,uFlags;

        if (flags & SHGFI_USEFILEATTRIBUTES && !(flags & SHGFI_PIDL))
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                lstrcpyW(psfi->szDisplayName, swShell32Name);
                psfi->iIcon = -IDI_SHELL_FOLDER;
            }
            else
            {
                WCHAR* szExt;
                WCHAR sTemp [MAX_PATH];

                szExt = PathFindExtensionW(szFullPath);
                TRACE("szExt=%s\n", debugstr_w(szExt));
                if ( szExt &&
                     HCR_MapTypeToValueW(szExt, sTemp, MAX_PATH, TRUE) &&
                     HCR_GetIconW(sTemp, sTemp, NULL, MAX_PATH, &psfi->iIcon))
                {
                    if (lstrcmpW(L"%1", sTemp))
                        strcpyW(psfi->szDisplayName, sTemp);
                    else
                    {
                        /* the icon is in the file */
                        strcpyW(psfi->szDisplayName, szFullPath);
                    }
                }
                else
                    ret = FALSE;
            }
        }
        else if (psfParent)
        {
            hr = IShellFolder_GetUIObjectOf(psfParent, 0, 1,
                (LPCITEMIDLIST*)&pidlLast, &IID_IExtractIconW,
                &uDummy, (LPVOID*)&pei);
            if (SUCCEEDED(hr))
            {
                hr = IExtractIconW_GetIconLocation(pei, uGilFlags,
                    szLocation, MAX_PATH, &iIndex, &uFlags);

                if (uFlags & GIL_NOTFILENAME)
                    ret = FALSE;
                else
                {
                    lstrcpyW (psfi->szDisplayName, szLocation);
                    psfi->iIcon = iIndex;
                }
                IExtractIconW_Release(pei);
            }
        }
    }

    /* get icon index (or load icon)*/
    if (SUCCEEDED(hr) && (flags & (SHGFI_ICON | SHGFI_SYSICONINDEX)))
    {
        if (flags & SHGFI_USEFILEATTRIBUTES && !(flags & SHGFI_PIDL))
        {
            WCHAR sTemp [MAX_PATH];
            WCHAR * szExt;
            int icon_idx=0;

            lstrcpynW(sTemp, szFullPath, MAX_PATH);

            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                psfi->iIcon = SIC_GetIconIndex(swShell32Name, -IDI_SHELL_FOLDER, 0);
            else
            {
                psfi->iIcon = 0;
                szExt = PathFindExtensionW(sTemp);
                if ( szExt &&
                     HCR_MapTypeToValueW(szExt, sTemp, MAX_PATH, TRUE) &&
                     HCR_GetIconW(sTemp, sTemp, NULL, MAX_PATH, &icon_idx))
                {
                    if (!lstrcmpW(L"%1",sTemp))            /* icon is in the file */
                        strcpyW(sTemp, szFullPath);

                    if (flags & SHGFI_SYSICONINDEX) 
                    {
                        psfi->iIcon = SIC_GetIconIndex(sTemp,icon_idx,0);
                        if (psfi->iIcon == -1)
                            psfi->iIcon = 0;
                    }
                    else 
                    {
                        UINT ret;
                        if (flags & SHGFI_SMALLICON)
                            ret = PrivateExtractIconsW( sTemp,icon_idx,
                                GetSystemMetrics( SM_CXSMICON ),
                                GetSystemMetrics( SM_CYSMICON ),
                                &psfi->hIcon, 0, 1, 0);
                        else
                            ret = PrivateExtractIconsW( sTemp, icon_idx,
                                GetSystemMetrics( SM_CXICON),
                                GetSystemMetrics( SM_CYICON),
                                &psfi->hIcon, 0, 1, 0);
                        if (ret != 0 && ret != (UINT)-1)
                        {
                            IconNotYetLoaded=FALSE;
                            psfi->iIcon = icon_idx;
                        }
                    }
                }
            }
        }
        else if (psfParent)
        {
            if (!(PidlToSicIndex(psfParent, pidlLast, !(flags & SHGFI_SMALLICON),
                uGilFlags, &(psfi->iIcon))))
            {
                ret = FALSE;
            }
        }
        if (ret && (flags & SHGFI_SYSICONINDEX))
        {
            if (flags & SHGFI_SMALLICON)
                ret = (DWORD_PTR)small_icons;
            else
                ret = (DWORD_PTR)big_icons;
        }
    }

    /* icon handle */
    if (SUCCEEDED(hr) && (flags & SHGFI_ICON) && IconNotYetLoaded)
    {
        if (flags & SHGFI_SMALLICON)
            psfi->hIcon = ImageList_GetIcon( small_icons, psfi->iIcon, ILD_NORMAL);
        else
            psfi->hIcon = ImageList_GetIcon( big_icons, psfi->iIcon, ILD_NORMAL);
    }

    if (flags & ~SHGFI_KNOWN_FLAGS)
        FIXME("unknown flags %08x\n", flags & ~SHGFI_KNOWN_FLAGS);

    if (psfParent)
        IShellFolder_Release(psfParent);
    SHFree(pidlFree);

    if (hr != S_OK)
        ret = FALSE;

    TRACE ("icon=%p index=0x%08x attr=0x%08x name=%s type=%s ret=0x%08lx\n",
           psfi->hIcon, psfi->iIcon, psfi->dwAttributes,
           debugstr_w(psfi->szDisplayName), debugstr_w(psfi->szTypeName), ret);

    return ret;
}

/*************************************************************************
 * SHGetFileInfoA            [SHELL32.@]
 *
 * Note:
 *    MSVBVM60.__vbaNew2 expects this function to return a value in range
 *    1 .. 0x7fff when the function succeeds and flags does not contain
 *    SHGFI_EXETYPE or SHGFI_SYSICONINDEX (see bug 7701)
 */
DWORD_PTR WINAPI SHGetFileInfoA(LPCSTR path,DWORD dwFileAttributes,
                                SHFILEINFOA *psfi, UINT sizeofpsfi,
                                UINT flags )
{
    INT len;
    LPWSTR temppath = NULL;
    LPCWSTR pathW;
    DWORD_PTR ret;
    SHFILEINFOW temppsfi;

    if (flags & SHGFI_PIDL)
    {
        /* path contains a pidl */
        pathW = (LPCWSTR)path;
    }
    else
    {
        len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        temppath = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, path, -1, temppath, len);
        pathW = temppath;
    }

    if (psfi)
    {
        temppsfi.hIcon = psfi->hIcon;
        temppsfi.iIcon = psfi->iIcon;
        temppsfi.dwAttributes = psfi->dwAttributes;

        ret = SHGetFileInfoW(pathW, dwFileAttributes, &temppsfi, sizeof(temppsfi), flags);
        psfi->hIcon = temppsfi.hIcon;
        psfi->iIcon = temppsfi.iIcon;
        psfi->dwAttributes = temppsfi.dwAttributes;

        WideCharToMultiByte(CP_ACP, 0, temppsfi.szDisplayName, -1,
              psfi->szDisplayName, sizeof(psfi->szDisplayName), NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, temppsfi.szTypeName, -1,
              psfi->szTypeName, sizeof(psfi->szTypeName), NULL, NULL);
    }
    else
        ret = SHGetFileInfoW(pathW, dwFileAttributes, NULL, 0, flags);

    HeapFree(GetProcessHeap(), 0, temppath);

    return ret;
}

/*************************************************************************
 * DuplicateIcon            [SHELL32.@]
 */
HICON WINAPI DuplicateIcon( HINSTANCE hInstance, HICON hIcon)
{
    ICONINFO IconInfo;
    HICON hDupIcon = 0;

    TRACE("%p %p\n", hInstance, hIcon);

    if (GetIconInfo(hIcon, &IconInfo))
    {
        hDupIcon = CreateIconIndirect(&IconInfo);

        /* clean up hbmMask and hbmColor */
        DeleteObject(IconInfo.hbmMask);
        DeleteObject(IconInfo.hbmColor);
    }

    return hDupIcon;
}

/*************************************************************************
 * ExtractIconA                [SHELL32.@]
 */
HICON WINAPI ExtractIconA(HINSTANCE hInstance, LPCSTR lpszFile, UINT nIconIndex)
{   
    HICON ret;
    INT len = MultiByteToWideChar(CP_ACP, 0, lpszFile, -1, NULL, 0);
    LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    TRACE("%p %s %d\n", hInstance, lpszFile, nIconIndex);

    MultiByteToWideChar(CP_ACP, 0, lpszFile, -1, lpwstrFile, len);
    ret = ExtractIconW(hInstance, lpwstrFile, nIconIndex);
    HeapFree(GetProcessHeap(), 0, lpwstrFile);

    return ret;
}

/*************************************************************************
 * ExtractIconW                [SHELL32.@]
 */
HICON WINAPI ExtractIconW(HINSTANCE hInstance, LPCWSTR lpszFile, UINT nIconIndex)
{
    HICON  hIcon = NULL;
    UINT ret;
    UINT cx = GetSystemMetrics(SM_CXICON), cy = GetSystemMetrics(SM_CYICON);

    TRACE("%p %s %d\n", hInstance, debugstr_w(lpszFile), nIconIndex);

    if (nIconIndex == (UINT)-1)
    {
        ret = PrivateExtractIconsW(lpszFile, 0, cx, cy, NULL, NULL, 0, LR_DEFAULTCOLOR);
        if (ret != (UINT)-1 && ret)
            return (HICON)(UINT_PTR)ret;
        return NULL;
    }
    else
        ret = PrivateExtractIconsW(lpszFile, nIconIndex, cx, cy, &hIcon, NULL, 1, LR_DEFAULTCOLOR);

    if (ret == (UINT)-1)
        return (HICON)1;
    else if (ret > 0 && hIcon)
        return hIcon;

    return NULL;
}

/*************************************************************************
 * Printer_LoadIconsW        [SHELL32.205]
 */
VOID WINAPI Printer_LoadIconsW(LPCWSTR wsPrinterName, HICON * pLargeIcon, HICON * pSmallIcon)
{
    INT iconindex=IDI_SHELL_PRINTERS_FOLDER;

    TRACE("(%s, %p, %p)\n", debugstr_w(wsPrinterName), pLargeIcon, pSmallIcon);

    /* We should check if wsPrinterName is
       1. the Default Printer or not
       2. connected or not
       3. a Local Printer or a Network-Printer
       and use different Icons
    */
    if((wsPrinterName != NULL) && (wsPrinterName[0] != 0))
    {
        FIXME("(select Icon by PrinterName %s not implemented)\n", debugstr_w(wsPrinterName));
    }

    if(pLargeIcon != NULL)
        *pLargeIcon = LoadImageW(shell32_hInstance,
                                 (LPCWSTR) MAKEINTRESOURCE(iconindex), IMAGE_ICON,
                                 0, 0, LR_DEFAULTCOLOR|LR_DEFAULTSIZE);

    if(pSmallIcon != NULL)
        *pSmallIcon = LoadImageW(shell32_hInstance,
                                 (LPCWSTR) MAKEINTRESOURCE(iconindex), IMAGE_ICON,
                                 16, 16, LR_DEFAULTCOLOR);
}

/*************************************************************************
 * Printers_RegisterWindowW        [SHELL32.213]
 * used by "printui.dll":
 * find the Window of the given Type for the specific Printer and 
 * return the already existent hwnd or open a new window
 */
BOOL WINAPI Printers_RegisterWindowW(LPCWSTR wsPrinter, DWORD dwType,
            HANDLE * phClassPidl, HWND * phwnd)
{
    FIXME("(%s, %x, %p (%p), %p (%p)) stub!\n", debugstr_w(wsPrinter), dwType,
                phClassPidl, (phClassPidl != NULL) ? *(phClassPidl) : NULL,
                phwnd, (phwnd != NULL) ? *(phwnd) : NULL);

    return FALSE;
} 

/*************************************************************************
 * Printers_UnregisterWindow      [SHELL32.214]
 */
VOID WINAPI Printers_UnregisterWindow(HANDLE hClassPidl, HWND hwnd)
{
    FIXME("(%p, %p) stub!\n", hClassPidl, hwnd);
}

/*************************************************************************/

typedef struct
{
    LPCWSTR  szApp;
#ifdef __REACTOS__
    LPCWSTR  szOSVersion;
#endif
    LPCWSTR  szOtherStuff;
    HICON hIcon;
} ABOUT_INFO;

/*************************************************************************
 * SHHelpShortcuts_RunDLLA        [SHELL32.@]
 *
 */
DWORD WINAPI SHHelpShortcuts_RunDLLA(DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{
    FIXME("(%x, %x, %x, %x) stub!\n", dwArg1, dwArg2, dwArg3, dwArg4);
    return 0;
}

/*************************************************************************
 * SHHelpShortcuts_RunDLLA        [SHELL32.@]
 *
 */
DWORD WINAPI SHHelpShortcuts_RunDLLW(DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{
    FIXME("(%x, %x, %x, %x) stub!\n", dwArg1, dwArg2, dwArg3, dwArg4);
    return 0;
}

/*************************************************************************
 * SHLoadInProc                [SHELL32.@]
 * Create an instance of specified object class from within
 * the shell process and release it immediately
 */
HRESULT WINAPI SHLoadInProc (REFCLSID rclsid)
{
    void *ptr = NULL;

    TRACE("%s\n", debugstr_guid(rclsid));

    CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown,&ptr);
    if(ptr)
    {
        IUnknown * pUnk = ptr;
        IUnknown_Release(pUnk);
        return S_OK;
    }
    return DISP_E_MEMBERNOTFOUND;
}

static VOID SetRegTextData(HWND hWnd, HKEY hKey, LPCWSTR Value, UINT uID)
{
    DWORD dwBufferSize;
    DWORD dwType;
    LPWSTR lpBuffer;

    if( RegQueryValueExW(hKey, Value, NULL, &dwType, NULL, &dwBufferSize) == ERROR_SUCCESS )
    {
        if(dwType == REG_SZ)
        {
            lpBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBufferSize);

            if(lpBuffer)
            {
                if( RegQueryValueExW(hKey, Value, NULL, &dwType, (LPBYTE)lpBuffer, &dwBufferSize) == ERROR_SUCCESS )
                {
                    SetDlgItemTextW(hWnd, uID, lpBuffer);
                }

                HeapFree(GetProcessHeap(), 0, lpBuffer);
            }
        }
    }
}

INT_PTR CALLBACK AboutAuthorsDlgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            const char* const *pstr = SHELL_Authors;

            // Add the authors to the list
            SendDlgItemMessageW( hWnd, IDC_ABOUT_AUTHORS_LISTBOX, WM_SETREDRAW, FALSE, 0 );

            while (*pstr)
            {
                WCHAR name[64];

                /* authors list is in utf-8 format */
                MultiByteToWideChar( CP_UTF8, 0, *pstr, -1, name, sizeof(name)/sizeof(WCHAR) );
                SendDlgItemMessageW( hWnd, IDC_ABOUT_AUTHORS_LISTBOX, LB_ADDSTRING, (WPARAM)-1, (LPARAM)name );
                pstr++;
            }

            SendDlgItemMessageW( hWnd, IDC_ABOUT_AUTHORS_LISTBOX, WM_SETREDRAW, TRUE, 0 );

            return TRUE;
        }
    }

    return FALSE;
}
/*************************************************************************
 * AboutDlgProc            (internal)
 */
static INT_PTR CALLBACK AboutDlgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
#ifdef __REACTOS__

    static DWORD   cxLogoBmp;
    static DWORD   cyLogoBmp, cyLineBmp;
    static HBITMAP hLogoBmp, hLineBmp;
    static HWND    hWndAuthors;

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            ABOUT_INFO *info = (ABOUT_INFO *)lParam;

            if (info)
            {
                HKEY hRegKey;
                MEMORYSTATUSEX MemStat;
                WCHAR szAppTitle[512];
                WCHAR szAppTitleTemplate[512];
                WCHAR szAuthorsText[20];

                // Preload the ROS bitmap
                hLogoBmp = (HBITMAP)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDB_REACTOS), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
                hLineBmp = (HBITMAP)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDB_LINEBAR), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

                if (hLogoBmp && hLineBmp)
                {
                    BITMAP bmpLogo;

                    GetObject(hLogoBmp, sizeof(BITMAP), &bmpLogo);

                    cxLogoBmp = bmpLogo.bmWidth;
                    cyLogoBmp = bmpLogo.bmHeight;

                    GetObject(hLineBmp, sizeof(BITMAP), &bmpLogo);
                    cyLineBmp = bmpLogo.bmHeight;
                }

                // Set App-specific stuff (icon, app name, szOtherStuff string)
                SendDlgItemMessageW(hWnd, IDC_ABOUT_ICON, STM_SETICON, (WPARAM)info->hIcon, 0);

                GetWindowTextW(hWnd, szAppTitleTemplate, ARRAY_SIZE(szAppTitleTemplate));
                swprintf(szAppTitle, szAppTitleTemplate, info->szApp);
                SetWindowTextW(hWnd, szAppTitle);

                SetDlgItemTextW(hWnd, IDC_ABOUT_APPNAME, info->szApp);
                SetDlgItemTextW(hWnd, IDC_ABOUT_VERSION, info->szOSVersion);
                SetDlgItemTextW(hWnd, IDC_ABOUT_OTHERSTUFF, info->szOtherStuff);

                // Set the registered user and organization name
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                  0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS)
                {
                    SetRegTextData(hWnd, hRegKey, L"RegisteredOwner", IDC_ABOUT_REG_USERNAME);
                    SetRegTextData(hWnd, hRegKey, L"RegisteredOrganization", IDC_ABOUT_REG_ORGNAME);

                    if (GetWindowTextLengthW(GetDlgItem(hWnd, IDC_ABOUT_REG_USERNAME)) == 0 &&
                        GetWindowTextLengthW(GetDlgItem(hWnd, IDC_ABOUT_REG_ORGNAME)) == 0)
                    {
                        ShowWindow(GetDlgItem(hWnd, IDC_ABOUT_REG_TO), SW_HIDE);
                    }

                    RegCloseKey(hRegKey);
                }

                // Set the value for the installed physical memory
                MemStat.dwLength = sizeof(MemStat);
                if (GlobalMemoryStatusEx(&MemStat))
                {
                    WCHAR szBuf[12];

                    if (MemStat.ullTotalPhys > 1024 * 1024 * 1024)
                    {
                        double dTotalPhys;
                        WCHAR szDecimalSeparator[4];
                        WCHAR szUnits[3];

                        // We're dealing with GBs or more
                        MemStat.ullTotalPhys /= 1024 * 1024;

                        if (MemStat.ullTotalPhys > 1024 * 1024)
                        {
                            // We're dealing with TBs or more
                            MemStat.ullTotalPhys /= 1024;

                            if (MemStat.ullTotalPhys > 1024 * 1024)
                            {
                                // We're dealing with PBs or more
                                MemStat.ullTotalPhys /= 1024;

                                dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
                                wcscpy(szUnits, L"PB");
                            }
                            else
                            {
                                dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
                                wcscpy(szUnits, L"TB");
                            }
                        }
                        else
                        {
                            dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
                            wcscpy(szUnits, L"GB");
                        }

                        // We need the decimal point of the current locale to display the RAM size correctly
                        if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                            szDecimalSeparator,
                            ARRAY_SIZE(szDecimalSeparator)) > 0)
                        {
                            UCHAR uDecimals;
                            UINT uIntegral;

                            uIntegral = (UINT)dTotalPhys;
                            uDecimals = (UCHAR)((UINT)(dTotalPhys * 100) - uIntegral * 100);

                            // Display the RAM size with 2 decimals
                            swprintf(szBuf, L"%u%s%02u %s", uIntegral, szDecimalSeparator, uDecimals, szUnits);
                        }
                    }
                    else
                    {
                        // We're dealing with MBs, don't show any decimals
                        swprintf(szBuf, L"%u MB", (UINT)MemStat.ullTotalPhys / 1024 / 1024);
                    }

                    SetDlgItemTextW(hWnd, IDC_ABOUT_PHYSMEM, szBuf);
                }

                // Add the Authors dialog
                hWndAuthors = CreateDialogW(shell32_hInstance, MAKEINTRESOURCEW(IDD_ABOUT_AUTHORS), hWnd, AboutAuthorsDlgProc);
                LoadStringW(shell32_hInstance, IDS_SHELL_ABOUT_AUTHORS, szAuthorsText, ARRAY_SIZE(szAuthorsText));
                SetDlgItemTextW(hWnd, IDC_ABOUT_AUTHORS, szAuthorsText);
            }

            return TRUE;
        }

        case WM_PAINT:
        {
            if (hLogoBmp && hLineBmp)
            {
                PAINTSTRUCT ps;
                HDC hdc;
                HDC hdcMem;
                HGDIOBJ hOldObj;

                hdc = BeginPaint(hWnd, &ps);
                hdcMem = CreateCompatibleDC(hdc);

                if (hdcMem)
                {
                    hOldObj = SelectObject(hdcMem, hLogoBmp);
                    BitBlt(hdc, 0, 0, cxLogoBmp, cyLogoBmp, hdcMem, 0, 0, SRCCOPY);

                    SelectObject(hdcMem, hLineBmp);
                    BitBlt(hdc, 0, cyLogoBmp, cxLogoBmp, cyLineBmp, hdcMem, 0, 0, SRCCOPY);

                    SelectObject(hdcMem, hOldObj);
                    DeleteDC(hdcMem);
                }

                EndPaint(hWnd, &ps);
            }
            break;
        }

        case WM_COMMAND:
        {
            switch(wParam)
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hWnd, TRUE);
                    return TRUE;

                case IDC_ABOUT_AUTHORS:
                {
                    static BOOL bShowingAuthors = FALSE;
                    WCHAR szAuthorsText[20];

                    if (bShowingAuthors)
                    {
                        LoadStringW(shell32_hInstance, IDS_SHELL_ABOUT_AUTHORS, szAuthorsText, ARRAY_SIZE(szAuthorsText));
                        ShowWindow(hWndAuthors, SW_HIDE);
                    }
                    else
                    {
                        LoadStringW(shell32_hInstance, IDS_SHELL_ABOUT_BACK, szAuthorsText, ARRAY_SIZE(szAuthorsText));
                        ShowWindow(hWndAuthors, SW_SHOW);
                    }

                    SetDlgItemTextW(hWnd, IDC_ABOUT_AUTHORS, szAuthorsText);
                    bShowingAuthors = !bShowingAuthors;
                    return TRUE;
                }
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hWnd, TRUE);
            break;
    }

#endif // __REACTOS__

    return 0;
}


/*************************************************************************
 * ShellAboutA                [SHELL32.288]
 */
BOOL WINAPI ShellAboutA( HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon )
{
    BOOL ret;
    LPWSTR appW = NULL, otherW = NULL;
    int len;

    if (szApp)
    {
        len = MultiByteToWideChar(CP_ACP, 0, szApp, -1, NULL, 0);
        appW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, szApp, -1, appW, len);
    }
    if (szOtherStuff)
    {
        len = MultiByteToWideChar(CP_ACP, 0, szOtherStuff, -1, NULL, 0);
        otherW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, szOtherStuff, -1, otherW, len);
    }

    ret = ShellAboutW(hWnd, appW, otherW, hIcon);

    HeapFree(GetProcessHeap(), 0, otherW);
    HeapFree(GetProcessHeap(), 0, appW);
    return ret;
}


/*************************************************************************
 * ShellAboutW                [SHELL32.289]
 */
BOOL WINAPI ShellAboutW( HWND hWnd, LPCWSTR szApp, LPCWSTR szOtherStuff,
                         HICON hIcon )
{
    ABOUT_INFO info;
    HRSRC hRes;
    DLGTEMPLATE *DlgTemplate;
    BOOL bRet;
#ifdef __REACTOS__
    WCHAR szVersionString[256];
    WCHAR szFormat[256];
#endif

    TRACE("\n");

    // DialogBoxIndirectParamW will be called with the hInstance of the calling application, so we have to preload the dialog template
    hRes = FindResourceW(shell32_hInstance, MAKEINTRESOURCEW(IDD_ABOUT), (LPWSTR)RT_DIALOG);
    if(!hRes)
        return FALSE;

    DlgTemplate = (DLGTEMPLATE *)LoadResource(shell32_hInstance, hRes);
    if(!DlgTemplate)
        return FALSE;

#ifdef __REACTOS__
    /* Output the version OS kernel strings */
    LoadStringW(shell32_hInstance, IDS_ABOUT_VERSION_STRING, szFormat, _countof(szFormat));
    StringCchPrintfW(szVersionString, _countof(szVersionString), szFormat, KERNEL_VERSION_STR, KERNEL_VERSION_BUILD_STR);
#endif

    info.szApp        = szApp;
#ifdef __REACTOS__
    info.szOSVersion  = szVersionString;
#endif
    info.szOtherStuff = szOtherStuff;
    info.hIcon        = hIcon ? hIcon : LoadIconW( 0, (LPWSTR)IDI_WINLOGO );

    bRet = DialogBoxIndirectParamW((HINSTANCE)GetWindowLongPtrW( hWnd, GWLP_HINSTANCE ),
                                   DlgTemplate, hWnd, AboutDlgProc, (LPARAM)&info );
    return bRet;
}

/*************************************************************************
 * FreeIconList (SHELL32.@)
 */
void WINAPI FreeIconList( DWORD dw )
{
    FIXME("%x: stub\n",dw);
}

/*************************************************************************
 * SHLoadNonloadedIconOverlayIdentifiers (SHELL32.@)
 */
HRESULT WINAPI SHLoadNonloadedIconOverlayIdentifiers( VOID )
{
    FIXME("stub\n");
    return S_OK;
}
