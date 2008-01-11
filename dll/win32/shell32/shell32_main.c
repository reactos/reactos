/*
 *                 Shell basics
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "dlgs.h"
#include "shellapi.h"
#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"
#include "shlwapi.h"

#include "undocshell.h"
#include "pidl.h"
#include "shell32_main.h"
#include "version.h"
#include "shresdef.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern const char * const SHELL_Authors[];

#define MORE_DEBUG 1
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
 * - an odd number of '\'s followed by '"' correspond to half that number
 *   of '\' followed by a '"' (extension of the above)
 *   '\\\"'    -> '\"'
 *   '\\\\\"'  -> '\\"'
 * - an even number of '\'s followed by a '"' correspond to half that number
 *   of '\', plus a regular quote serving as an argument delimiter (which
 *   means it does not appear in the result)
 *   'a\\"b c"'   -> 'a\b c'
 *   'a\\\\"b c"' -> 'a\\b c'
 * - '\' that are not followed by a '"' are copied literally
 *   'a\b'     -> 'a\b'
 *   'a\\b'    -> 'a\\b'
 *
 * Note:
 * '\t' == 0x0009
 * ' '  == 0x0020
 * '"'  == 0x0022
 * '\\' == 0x005c
 */
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR lpCmdline, int* numargs)
{
    DWORD argc;
    HGLOBAL hargv;
    LPWSTR  *argv;
    LPCWSTR cs;
    LPWSTR arg,s,d;
    LPWSTR cmdline;
    int in_quotes,bcount;

    if (*lpCmdline==0)
    {
        /* Return the path to the executable */
        DWORD len, size=16;

        hargv=GlobalAlloc(GMEM_FIXED, size);
        argv=GlobalLock(hargv);
        for (;;)
        {
            len = GetModuleFileNameW(0, (LPWSTR)(argv+1), (size-sizeof(LPWSTR))/sizeof(WCHAR));
            if (!len)
            {
                GlobalFree(hargv);
                return NULL;
            }
            if (len < size) break;
            size*=2;
            hargv=GlobalReAlloc(hargv, size, 0);
            argv=GlobalLock(hargv);
        }
        argv[0]=(LPWSTR)(argv+1);
        if (numargs)
            *numargs=2;

        return argv;
    }

    /* to get a writable copy */
    argc=0;
    bcount=0;
    in_quotes=0;
    cs=lpCmdline;
    while (1)
    {
        if (*cs==0 || ((*cs==0x0009 || *cs==0x0020) && !in_quotes))
        {
            /* space */
            argc++;
            /* skip the remaining spaces */
            while (*cs==0x0009 || *cs==0x0020) {
                cs++;
            }
            if (*cs==0)
                break;
            bcount=0;
            continue;
        }
        else if (*cs==0x005c)
        {
            /* '\', count them */
            bcount++;
        }
        else if ((*cs==0x0022) && ((bcount & 1)==0))
        {
            /* unescaped '"' */
            in_quotes=!in_quotes;
            bcount=0;
        }
        else
        {
            /* a regular character */
            bcount=0;
        }
        cs++;
    }
    /* Allocate in a single lump, the string array, and the strings that go with it.
     * This way the caller can make a single GlobalFree call to free both, as per MSDN.
     */
    hargv=GlobalAlloc(0, argc*sizeof(LPWSTR)+(strlenW(lpCmdline)+1)*sizeof(WCHAR));
    argv=GlobalLock(hargv);
    if (!argv)
        return NULL;
    cmdline=(LPWSTR)(argv+argc);
    strcpyW(cmdline, lpCmdline);

    argc=0;
    bcount=0;
    in_quotes=0;
    arg=d=s=cmdline;
    while (*s)
    {
        if ((*s==0x0009 || *s==0x0020) && !in_quotes)
        {
            /* Close the argument and copy it */
            *d=0;
            argv[argc++]=arg;

            /* skip the remaining spaces */
            do {
                s++;
            } while (*s==0x0009 || *s==0x0020);

            /* Start with a new argument */
            arg=d=s;
            bcount=0;
        }
        else if (*s==0x005c)
        {
            /* '\\' */
            *d++=*s++;
            bcount++;
        }
        else if (*s==0x0022)
        {
            /* '"' */
            if ((bcount & 1)==0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d-=bcount/2;
                in_quotes=!in_quotes;
                s++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d=d-bcount/2-1;
                *d++='"';
                s++;
            }
            bcount=0;
        }
        else
        {
            /* a regular character */
            *d++=*s++;
            bcount=0;
        }
    }
    if (*arg)
    {
        *d='\0';
        argv[argc++]=arg;
    }
    if (numargs)
        *numargs=argc;

    return argv;
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
    char szTemp[MAX_PATH];
    HKEY keyCls;
    BOOL ret = FALSE;

    if (_ILGetExtension(pidlLast, szTemp, MAX_PATH) &&
          HCR_MapTypeToValueA(szTemp, szTemp, MAX_PATH, TRUE))
    {
        if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, KEY_QUERY_VALUE, &keyCls))
        {
          if (ERROR_SUCCESS == RegQueryValueExA(keyCls, "IsShortcut", NULL, NULL, NULL, NULL))
            ret = TRUE;

          RegCloseKey(keyCls);
        }
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
    LPITEMIDLIST    pidlLast = NULL, pidl = NULL;
    HRESULT hr = S_OK;
    BOOL IconNotYetLoaded=TRUE;
    UINT uGilFlags = 0;

    TRACE("%s fattr=0x%x sfi=%p(attr=0x%08x) size=0x%x flags=0x%x\n",
          (flags & SHGFI_PIDL)? "pidl" : debugstr_w(path), dwFileAttributes,
          psfi, psfi->dwAttributes, sizeofpsfi, flags);

    if ( (flags & SHGFI_USEFILEATTRIBUTES) &&
         (flags & (SHGFI_ATTRIBUTES|SHGFI_EXETYPE|SHGFI_PIDL)))
        return FALSE;

    /* windows initializes these values regardless of the flags */
    if (psfi != NULL)
    {
        psfi->szDisplayName[0] = '\0';
        psfi->szTypeName[0] = '\0';
        psfi->iIcon = 0;
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
    }

    if (flags & SHGFI_EXETYPE)
    {
        if (flags != SHGFI_EXETYPE)
            return 0;
        return shgfi_get_exe_type(szFullPath);
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
        pidl = ILClone((LPCITEMIDLIST)path);
    }
    else if (!(flags & SHGFI_USEFILEATTRIBUTES))
    {
        hr = SHILCreateFromPathW(szFullPath, &pidl, &dwAttributes);
    }

    if ((flags & SHGFI_PIDL) || !(flags & SHGFI_USEFILEATTRIBUTES))
    {
        /* get the parent shellfolder */
        if (pidl)
        {
            hr = SHBindToParent( pidl, &IID_IShellFolder, (LPVOID*)&psfParent,
                                (LPCITEMIDLIST*)&pidlLast );
            if (SUCCEEDED(hr))
                pidlLast = ILClone(pidlLast);
            ILFree(pidl);
        }
        else
        {
            ERR("pidl is null!\n");
            return FALSE;
        }
    }

    /* get the attributes of the child */
    if (SUCCEEDED(hr) && (flags & SHGFI_ATTRIBUTES))
    {
        if (!(flags & SHGFI_ATTR_SPECIFIED))
        {
            psfi->dwAttributes = 0xffffffff;
        }
        IShellFolder_GetAttributesOf( psfParent, 1, (LPCITEMIDLIST*)&pidlLast,
                                      &(psfi->dwAttributes) );
    }

    /* get the displayname */
    if (SUCCEEDED(hr) && (flags & SHGFI_DISPLAYNAME))
    {
        if (flags & SHGFI_USEFILEATTRIBUTES)
        {
            lstrcpyW (psfi->szDisplayName, PathFindFileNameW(szFullPath));
        }
        else
        {
            STRRET str;
            hr = IShellFolder_GetDisplayNameOf( psfParent, pidlLast,
                                                SHGDN_INFOLDER, &str);
            StrRetToStrNW (psfi->szDisplayName, MAX_PATH, &str, pidlLast);
        }
    }

    /* get the type name */
    if (SUCCEEDED(hr) && (flags & SHGFI_TYPENAME))
    {
        static const WCHAR szFile[] = { 'F','i','l','e',0 };
        static const WCHAR szDashFile[] = { '-','f','i','l','e',0 };

        if (!(flags & SHGFI_USEFILEATTRIBUTES))
        {
            char ftype[80];

            _ILGetFileType(pidlLast, ftype, 80);
            MultiByteToWideChar(CP_ACP, 0, ftype, -1, psfi->szTypeName, 80 );
        }
        else
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                strcatW (psfi->szTypeName, szFile);
            else
            {
                WCHAR sTemp[64];

                lstrcpyW(sTemp,PathFindExtensionW(szFullPath));
                if (!( HCR_MapTypeToValueW(sTemp, sTemp, 64, TRUE) &&
                    HCR_MapTypeToValueW(sTemp, psfi->szTypeName, 80, FALSE )))
                {
                    lstrcpynW (psfi->szTypeName, sTemp, 64);
                    strcatW (psfi->szTypeName, szDashFile);
                }
            }
        }
    }

    /* ### icons ###*/
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

        if (flags & SHGFI_USEFILEATTRIBUTES)
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                lstrcpyW(psfi->szDisplayName, swShell32Name);
                psfi->iIcon = -IDI_SHELL_FOLDER;
            }
            else
            {
                WCHAR* szExt;
                static const WCHAR p1W[] = {'%','1',0};
                WCHAR sTemp [MAX_PATH];

                szExt = (LPWSTR) PathFindExtensionW(szFullPath);
                TRACE("szExt=%s\n", debugstr_w(szExt));
                if ( szExt &&
                     HCR_MapTypeToValueW(szExt, sTemp, MAX_PATH, TRUE) &&
                     HCR_GetDefaultIconW(sTemp, sTemp, MAX_PATH, &psfi->iIcon))
                {
                    if (lstrcmpW(p1W, sTemp))
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
        else
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
        if (flags & SHGFI_USEFILEATTRIBUTES)
        {
            WCHAR sTemp [MAX_PATH];
            WCHAR * szExt;
            int icon_idx=0;

            lstrcpynW(sTemp, szFullPath, MAX_PATH);

            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                psfi->iIcon = SIC_GetIconIndex(swShell32Name, -IDI_SHELL_FOLDER, 0);
            else
            {
                static const WCHAR p1W[] = {'%','1',0};

                psfi->iIcon = 0;
                szExt = (LPWSTR) PathFindExtensionW(sTemp);
                if ( szExt &&
                     HCR_MapTypeToValueW(szExt, sTemp, MAX_PATH, TRUE) &&
                     HCR_GetDefaultIconW(sTemp, sTemp, MAX_PATH, &icon_idx))
                {
                    if (!lstrcmpW(p1W,sTemp))            /* icon is in the file */
                        strcpyW(sTemp, szFullPath);

                    if (flags & SHGFI_SYSICONINDEX)
                    {
                        psfi->iIcon = SIC_GetIconIndex(sTemp,icon_idx,0);
                        if (psfi->iIcon == -1)
                            psfi->iIcon = 0;
                    }
                    else
                    {
                        IconNotYetLoaded=FALSE;
                        if (flags & SHGFI_SMALLICON)
                            PrivateExtractIconsW( sTemp,icon_idx,
                                GetSystemMetrics( SM_CXSMICON ),
                                GetSystemMetrics( SM_CYSMICON ),
                                &psfi->hIcon, 0, 1, 0);
                        else
                            PrivateExtractIconsW( sTemp, icon_idx,
                                GetSystemMetrics( SM_CXICON),
                                GetSystemMetrics( SM_CYICON),
                                &psfi->hIcon, 0, 1, 0);
                        psfi->iIcon = icon_idx;
                    }
                }
            }
        }
        else
        {
            if (!(PidlToSicIndex(psfParent, pidlLast, !(flags & SHGFI_SMALLICON),
                uGilFlags, &(psfi->iIcon))))
            {
                ret = FALSE;
            }
        }
        if (ret)
        {
            if (flags & SHGFI_SMALLICON)
                ret = (DWORD_PTR) ShellSmallIconList;
            else
                ret = (DWORD_PTR) ShellBigIconList;
        }
    }

    /* icon handle */
    if (SUCCEEDED(hr) && (flags & SHGFI_ICON) && IconNotYetLoaded)
    {
        if (flags & SHGFI_SMALLICON)
            psfi->hIcon = ImageList_GetIcon( ShellSmallIconList, psfi->iIcon, ILD_NORMAL);
        else
            psfi->hIcon = ImageList_GetIcon( ShellBigIconList, psfi->iIcon, ILD_NORMAL);
    }

    if (flags & ~SHGFI_KNOWN_FLAGS)
        FIXME("unknown flags %08x\n", flags & ~SHGFI_KNOWN_FLAGS);

    if (psfParent)
        IShellFolder_Release(psfParent);

    if (hr != S_OK)
        ret = FALSE;

    SHFree(pidlLast);

#ifdef MORE_DEBUG
    TRACE ("icon=%p index=0x%08x attr=0x%08x name=%s type=%s ret=0x%08lx\n",
           psfi->hIcon, psfi->iIcon, psfi->dwAttributes,
           debugstr_w(psfi->szDisplayName), debugstr_w(psfi->szTypeName), ret);
#endif

    return ret;
}

/*************************************************************************
 * SHGetFileInfoA            [SHELL32.@]
 */
DWORD_PTR WINAPI SHGetFileInfoA(LPCSTR path,DWORD dwFileAttributes,
                                SHFILEINFOA *psfi, UINT sizeofpsfi,
                                UINT flags )
{
    INT len;
    LPWSTR temppath = NULL;
    LPCWSTR pathW;
    DWORD ret;
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

    if (psfi && (flags & SHGFI_ATTR_SPECIFIED))
        temppsfi.dwAttributes=psfi->dwAttributes;

    if (psfi == NULL)
        ret = SHGetFileInfoW(pathW, dwFileAttributes, NULL, sizeof(temppsfi), flags);
    else
        ret = SHGetFileInfoW(pathW, dwFileAttributes, &temppsfi, sizeof(temppsfi), flags);

    if (psfi)
    {
        if(flags & SHGFI_ICON)
            psfi->hIcon=temppsfi.hIcon;
        if(flags & (SHGFI_SYSICONINDEX|SHGFI_ICON|SHGFI_ICONLOCATION))
            psfi->iIcon=temppsfi.iIcon;
        if(flags & SHGFI_ATTRIBUTES)
            psfi->dwAttributes=temppsfi.dwAttributes;
        if(flags & (SHGFI_DISPLAYNAME|SHGFI_ICONLOCATION))
        {
            WideCharToMultiByte(CP_ACP, 0, temppsfi.szDisplayName, -1,
                  psfi->szDisplayName, sizeof(psfi->szDisplayName), NULL, NULL);
        }
        if(flags & SHGFI_TYPENAME)
        {
            WideCharToMultiByte(CP_ACP, 0, temppsfi.szTypeName, -1,
                  psfi->szTypeName, sizeof(psfi->szTypeName), NULL, NULL);
        }
    }

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

    if (nIconIndex == 0xFFFFFFFF)
    {
        ret = PrivateExtractIconsW(lpszFile, 0, cx, cy, NULL, NULL, 0, LR_DEFAULTCOLOR);
        if (ret != 0xFFFFFFFF && ret)
            return (HICON)(UINT_PTR)ret;
        return NULL;
    }
    else
        ret = PrivateExtractIconsW(lpszFile, nIconIndex, cx, cy, &hIcon, NULL, 1, LR_DEFAULTCOLOR);

    if (ret == 0xFFFFFFFF)
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
    INT iconindex=IDI_SHELL_PRINTER;

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
    LPCWSTR  szOtherStuff;
    HICON hIcon;
} ABOUT_INFO;

#define DROP_FIELD_TOP    (-15)
#define DROP_FIELD_HEIGHT  15

/*************************************************************************
 * SHAppBarMessage            [SHELL32.@]
 */
UINT WINAPI SHAppBarMessage(DWORD msg, PAPPBARDATA data)
{
    int width=data->rc.right - data->rc.left;
    int height=data->rc.bottom - data->rc.top;
    RECT rec=data->rc;

    switch (msg)
    {
    case ABM_GETSTATE:
        return ABS_ALWAYSONTOP | ABS_AUTOHIDE;
    case ABM_GETTASKBARPOS:
        GetWindowRect(data->hWnd, &rec);
        data->rc=rec;
        return TRUE;
    case ABM_ACTIVATE:
        SetActiveWindow(data->hWnd);
        return TRUE;
    case ABM_GETAUTOHIDEBAR:
        data->hWnd=GetActiveWindow();
        return TRUE;
    case ABM_NEW:
        /* cbSize, hWnd, and uCallbackMessage are used. All other ignored */
        SetWindowPos(data->hWnd,HWND_TOP,0,0,0,0,SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
        return TRUE;
    case ABM_QUERYPOS:
        GetWindowRect(data->hWnd, &(data->rc));
        return TRUE;
    case ABM_REMOVE:
        FIXME("ABM_REMOVE broken\n");
        /* FIXME: this is wrong; should it be DestroyWindow instead? */
        /*CloseHandle(data->hWnd);*/
        return TRUE;
    case ABM_SETAUTOHIDEBAR:
        SetWindowPos(data->hWnd,HWND_TOP,rec.left+1000,rec.top,
                         width,height,SWP_SHOWWINDOW);
        return TRUE;
    case ABM_SETPOS:
        data->uEdge=(ABE_RIGHT | ABE_LEFT);
        SetWindowPos(data->hWnd,HWND_TOP,data->rc.left,data->rc.top,
                     width,height,SWP_SHOWWINDOW);
        return TRUE;
    case ABM_WINDOWPOSCHANGED:
        return TRUE;
    }
    return FALSE;
}

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
        return NOERROR;
    }
    return DISP_E_MEMBERNOTFOUND;
}

static VOID SetRegTextData(HWND hWnd, HKEY hKey, LPWSTR Value, UINT uID)
{
    DWORD dwBufferSize;
    DWORD dwType;
    LPWSTR lpBuffer;

    if( RegQueryValueExW(hKey, Value, NULL, &dwType, NULL, &dwBufferSize) == ERROR_SUCCESS )
    {
        if(dwType == REG_SZ)
        {
            lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwBufferSize);

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
            SendDlgItemMessageW( hWnd, IDC_SHELL_ABOUT_AUTHORS_LISTBOX, WM_SETREDRAW, FALSE, 0 );

            while (*pstr)
            {
                WCHAR name[64];

                /* authors list is in utf-8 format */
                MultiByteToWideChar( CP_UTF8, 0, *pstr, -1, name, sizeof(name)/sizeof(WCHAR) );
                SendDlgItemMessageW( hWnd, IDC_SHELL_ABOUT_AUTHORS_LISTBOX, LB_ADDSTRING, (WPARAM)-1, (LPARAM)name );
                pstr++;
            }

            SendDlgItemMessageW( hWnd, IDC_SHELL_ABOUT_AUTHORS_LISTBOX, WM_SETREDRAW, TRUE, 0 );

            return TRUE;
        }
    }

    return FALSE;
}
/*************************************************************************
 * AboutDlgProc            (internal)
 */
INT_PTR CALLBACK AboutDlgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static DWORD   cxLogoBmp;
    static DWORD   cyLogoBmp;
    static HBITMAP hLogoBmp;
    static HWND    hWndAuthors;

    switch(msg)
    {
        case WM_INITDIALOG:
        {
            ABOUT_INFO *info = (ABOUT_INFO *)lParam;

            if (info)
            {
                const WCHAR szRegKey[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
                HKEY hRegKey;
                MEMORYSTATUSEX MemStat;
                WCHAR szAppTitle[512];
                WCHAR szAppTitleTemplate[512];
                WCHAR szAuthorsText[20];

                // Preload the ROS bitmap
                hLogoBmp = LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDB_SHELL_ABOUT_LOGO_24BPP), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

                if(hLogoBmp)
                {
                    BITMAP bmpLogo;

                    GetObject( hLogoBmp, sizeof(BITMAP), &bmpLogo );

                    cxLogoBmp = bmpLogo.bmWidth;
                    cyLogoBmp = bmpLogo.bmHeight;
                }

                // Set App-specific stuff (icon, app name, szOtherStuff string)
                SendDlgItemMessageW(hWnd, IDC_SHELL_ABOUT_ICON, STM_SETICON, (WPARAM)info->hIcon, 0);

                GetWindowTextW( hWnd, szAppTitleTemplate, sizeof(szAppTitleTemplate) / sizeof(WCHAR) );
                wsprintfW( szAppTitle, szAppTitleTemplate, info->szApp );
                SetWindowTextW( hWnd, szAppTitle );

                SetDlgItemTextW( hWnd, IDC_SHELL_ABOUT_APPNAME, info->szApp );
                SetDlgItemTextW( hWnd, IDC_SHELL_ABOUT_OTHERSTUFF, info->szOtherStuff );

                // Set the registered user and organization name
                if(RegOpenKeyExW( HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_QUERY_VALUE, &hRegKey ) == ERROR_SUCCESS)
                {
                    SetRegTextData( hWnd, hRegKey, L"RegisteredOwner", IDC_SHELL_ABOUT_REG_USERNAME );
                    SetRegTextData( hWnd, hRegKey, L"RegisteredOrganization", IDC_SHELL_ABOUT_REG_ORGNAME );

                    RegCloseKey( hRegKey );
                }

                // Set the value for the installed physical memory
                MemStat.dwLength = sizeof(MemStat);
                if( GlobalMemoryStatusEx(&MemStat) )
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
                                wcscpy( szUnits, L"PB" );
                            }
                            else
                            {
                                dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
                                wcscpy( szUnits, L"TB" );
                            }
                        }
                        else
                        {
                            dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
                            wcscpy( szUnits, L"GB" );
                        }

                        // We need the decimal point of the current locale to display the RAM size correctly
                        if( GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSeparator, sizeof(szDecimalSeparator) / sizeof(WCHAR)) > 0)
                        {
                            UCHAR uDecimals;
                            UINT uIntegral;

                            uIntegral = (UINT)dTotalPhys;
                            uDecimals = (UCHAR)((UINT)(dTotalPhys * 100) - uIntegral * 100);

                            // Display the RAM size with 2 decimals
                            wsprintfW(szBuf, L"%u%s%02u %s", uIntegral, szDecimalSeparator, uDecimals, szUnits);
                        }
                    }
                    else
                    {
                        // We're dealing with MBs, don't show any decimals
                        wsprintfW( szBuf, L"%u MB", (UINT)MemStat.ullTotalPhys / 1024 / 1024 );
                    }

                    SetDlgItemTextW( hWnd, IDC_SHELL_ABOUT_PHYSMEM, szBuf);
                }

                // Add the Authors dialog
                hWndAuthors = CreateDialogW( shell32_hInstance, MAKEINTRESOURCEW(IDD_SHELL_ABOUT_AUTHORS), hWnd, AboutAuthorsDlgProc );
                LoadStringW( shell32_hInstance, IDS_SHELL_ABOUT_AUTHORS, szAuthorsText, sizeof(szAuthorsText) / sizeof(WCHAR) );
                SetDlgItemTextW( hWnd, IDC_SHELL_ABOUT_AUTHORS, szAuthorsText );
            }

            return TRUE;
        }

        case WM_PAINT:
        {
            if(hLogoBmp)
            {
                PAINTSTRUCT ps;
                HDC hdc;
                HDC hdcMem;
                
                hdc = BeginPaint(hWnd, &ps);
                hdcMem = CreateCompatibleDC(hdc);

                if(hdcMem)
                {
                    SelectObject(hdcMem, hLogoBmp);
                    BitBlt(hdc, 0, 0, cxLogoBmp, cyLogoBmp, hdcMem, 0, 0, SRCCOPY);

                    DeleteDC(hdcMem);
                }

                EndPaint(hWnd, &ps);
            }

            break;
        }

        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hWnd, TRUE);
                    return TRUE;

                case IDC_SHELL_ABOUT_AUTHORS:
                {
                    static BOOL bShowingAuthors = FALSE;
                    WCHAR szAuthorsText[20];

                    if(bShowingAuthors)
                    {
                        LoadStringW( shell32_hInstance, IDS_SHELL_ABOUT_AUTHORS, szAuthorsText, sizeof(szAuthorsText) / sizeof(WCHAR) );
                        ShowWindow( hWndAuthors, SW_HIDE );
                    }
                    else
                    {
                        LoadStringW( shell32_hInstance, IDS_SHELL_ABOUT_BACK, szAuthorsText, sizeof(szAuthorsText) / sizeof(WCHAR) );
                        ShowWindow( hWndAuthors, SW_SHOW );
                    }

                    SetDlgItemTextW( hWnd, IDC_SHELL_ABOUT_AUTHORS, szAuthorsText );
                    bShowingAuthors = !bShowingAuthors;
                    return TRUE;
                }
            }
            break;

        case WM_CLOSE:
            EndDialog(hWnd, TRUE);
            break;
    }

    return FALSE;
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
    LPVOID DlgTemplate;
    BOOL bRet;

    TRACE("\n");

    // DialogBoxIndirectParamW will be called with the hInstance of the calling application, so we have to preload the dialog template
    if(!(hRes = FindResourceW(shell32_hInstance, MAKEINTRESOURCEW(IDD_SHELL_ABOUT), (LPWSTR)RT_DIALOG)))
        return FALSE;
    if(!(DlgTemplate = (LPVOID)LoadResource(shell32_hInstance, hRes)))
        return FALSE;

    info.szApp        = szApp;
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

/***********************************************************************
 * DllGetVersion [SHELL32.@]
 *
 * Retrieves version information of the 'SHELL32.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a shell32.dll from IE4.01 SP1.
 */

HRESULT WINAPI DllGetVersion (DLLVERSIONINFO *pdvi)
{
    /* FIXME: shouldn't these values come from the version resource? */
    if (pdvi->cbSize == sizeof(DLLVERSIONINFO) ||
        pdvi->cbSize == sizeof(DLLVERSIONINFO2))
    {
        pdvi->dwMajorVersion = WINE_FILEVERSION_MAJOR;
        pdvi->dwMinorVersion = WINE_FILEVERSION_MINOR;
        pdvi->dwBuildNumber = WINE_FILEVERSION_BUILD;
        pdvi->dwPlatformID = WINE_FILEVERSION_PLATFORMID;
        if (pdvi->cbSize == sizeof(DLLVERSIONINFO2))
        {
            DLLVERSIONINFO2 *pdvi2 = (DLLVERSIONINFO2 *)pdvi;

            pdvi2->dwFlags = 0;
            pdvi2->ullVersion = MAKEDLLVERULL(WINE_FILEVERSION_MAJOR,
                                              WINE_FILEVERSION_MINOR,
                                              WINE_FILEVERSION_BUILD,
                                              WINE_FILEVERSION_PLATFORMID);
        }
        TRACE("%u.%u.%u.%u\n",
              pdvi->dwMajorVersion, pdvi->dwMinorVersion,
              pdvi->dwBuildNumber, pdvi->dwPlatformID);
        return S_OK;
    }
    else
    {
        WARN("wrong DLLVERSIONINFO size from app\n");
        return E_INVALIDARG;
    }
}

/*************************************************************************
 * global variables of the shell32.dll
 * all are once per process
 *
 */
HINSTANCE    shell32_hInstance = 0;
HIMAGELIST   ShellSmallIconList = 0;
HIMAGELIST   ShellBigIconList = 0;


/*************************************************************************
 * SHELL32 DllMain
 *
 * NOTES
 *  calling oleinitialize here breaks sone apps.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        shell32_hInstance = hinstDLL;
        DisableThreadLibraryCalls(shell32_hInstance);

        /* get full path to this DLL for IExtractIconW_fnGetIconLocation() */
        GetModuleFileNameW(hinstDLL, swShell32Name, MAX_PATH);
        swShell32Name[MAX_PATH - 1] = '\0';

        InitCommonControlsEx(NULL);

        SIC_Initialize();
        InitChangeNotifications();
        break;

    case DLL_PROCESS_DETACH:
        shell32_hInstance = 0;
        SIC_Destroy();
        FreeChangeNotifications();
        break;
    }
    return TRUE;
}

/*************************************************************************
 * DllInstall         [SHELL32.@]
 *
 * PARAMETERS
 *
 *    BOOL bInstall - TRUE for install, FALSE for uninstall
 *    LPCWSTR pszCmdLine - command line (unused by shell32?)
 */

HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("%s %s: stub\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));
    return S_OK;        /* indicate success */
}

/***********************************************************************
 *              DllCanUnloadNow (SHELL32.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("stub\n");
    return S_FALSE;
}
