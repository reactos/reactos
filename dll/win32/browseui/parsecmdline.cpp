/*
* ReactOS browseui
*
* Copyright 2014 David Quintana <gigaherz@gmail.com>
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
*/

#include "precomp.h"
#include <strsafe.h>

extern "C"
BOOL WINAPI GUIDFromStringW(
    _In_   PCWSTR psz,
    _Out_  LPGUID pguid
    );

static BOOL _CopyAndUnquoteText(LPCWSTR strFieldSource, LPWSTR strField, size_t cchField)
{
    WCHAR cChar;
    PWSTR tmpField = strField;
    size_t lenField = 1;
    BOOL inQuote = FALSE;

    // Remove leading whitespace
    cChar = *strFieldSource;
    while (cChar == L' ' || cChar == L'\t' || cChar == L'\n' || cChar == L'\r')
    {
        strFieldSource = CharNextW(strFieldSource);
        cChar = *strFieldSource;
    }

    while (cChar && cChar != L'=' && cChar != L',')
    {
        if (cChar == L'"')
        {
            // [1] is always valid read because of null-termination
            if (inQuote && strFieldSource[1] == L'"')
            {
                if (lenField < cchField)
                {
                    // Append
                    *(tmpField++) = L'"';
                    ++lenField;
                }

                // Skip second quote
                strFieldSource++;
            }
            else
            {
                inQuote = !inQuote;
            }
        }
        else
        {
            if (inQuote || (cChar != L'=' && cChar != L','))
            {
                if (lenField < cchField)
                {
                    // Append
                    *(tmpField++) = cChar;
                    ++lenField;
                }
            }
        }

        strFieldSource = CharNextW(strFieldSource);
        cChar = *strFieldSource;
    }

    // Remove trailing whitespace
    while (tmpField > strField)
    {
        tmpField = CharPrevW(strField, tmpField);
        cChar = *tmpField;
        if (cChar != L' ' && cChar != L'\t' && cChar != L'\n' && cChar != L'\r')
        {
            tmpField = CharNextW(tmpField);
            break;
        }
    }

    // Terminate output string
    *tmpField = 0;

    return TRUE;
}

static BOOL _FindNextArg(PCWSTR * pstrFieldSource)
{
    PCWSTR strFieldSource = *pstrFieldSource;
    WCHAR cChar = *strFieldSource;
    BOOL inQuote = FALSE;

    while (cChar)
    {
        if (!inQuote && (cChar == L'=' || cChar == L','))
            break;

        if (cChar == L'"')
            inQuote = !inQuote;

        strFieldSource = CharNextW(strFieldSource);
        cChar = *strFieldSource;
    }

    if (cChar == 0)
    {
        *pstrFieldSource = strFieldSource;
        return FALSE;
    }

    *pstrFieldSource = CharNextW(strFieldSource);
    return TRUE;
}

static PCWSTR _FindFirstField(PCWSTR strFieldSource)
{
    //Find end of first arg, because
    // behaviour is different if the first separator is an '='
    BOOL inQuote = FALSE;
    PCWSTR tmpArgs = strFieldSource;
    WCHAR cChar = *tmpArgs;
    while (cChar)
    {
        if (cChar == L'=')
            break;

        if (cChar == L',')
            break;

        if (cChar == L'\"')
            inQuote = !inQuote;

        tmpArgs = CharNextW(tmpArgs);
        cChar = *tmpArgs;
    }

    // Skip the text before the first equal sign, if not quoted, unless the arg 0 was requested.
    if (*tmpArgs == L'=' && !inQuote)
    {
        strFieldSource = ++tmpArgs;
        TRACE("Skipped content before the first '=', remainder=%S\n", strFieldSource);
    }

    return strFieldSource;
}

static BOOL _ReadNextArg(PCWSTR * pstrFieldSource, PWSTR strField, size_t cchField)
{
    // Copy and unquote text
    _CopyAndUnquoteText(*pstrFieldSource, strField, cchField);

    return _FindNextArg(pstrFieldSource);
}

static LPITEMIDLIST _ILReadFromSharedMemory(PCWSTR strField)
{
    LPITEMIDLIST ret = NULL;

    // Ensure it really is an IDLIST-formatted parameter
    // Format for IDLIST params: ":pid:shared"
    if (*strField != L':')
        return NULL;

    HANDLE hData = IntToPtr(StrToIntW(strField + 1));
    PWSTR strSecond = StrChrW(strField + 1, L':');

    if (strSecond)
    {
        int pid = StrToIntW(strSecond + 1);
        void* pvShared = SHLockShared(hData, pid);
        if (pvShared)
        {
            ret = ILClone((LPCITEMIDLIST) pvShared);
            SHUnlockShared(pvShared);
            SHFreeShared(hData, pid);
        }
    }
    return ret;
}

static HRESULT _ParsePathToPidl(PWSTR strPath, LPITEMIDLIST * pidl)
{
    CComPtr<IShellFolder> psfDesktop;

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    return psfDesktop->ParseDisplayName(NULL, NULL, strPath, NULL, pidl, NULL);
}

static LPITEMIDLIST _GetDocumentsPidl()
{
    CComPtr<IShellFolder> ppshf;
    LPITEMIDLIST pidl;
    WCHAR guid [] = L"::{450d8fba-ad25-11d0-98a8-0800361b1103}";

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_MYDOCUMENTS, &pidl)))
        return pidl;

    if (FAILED(SHGetDesktopFolder(&ppshf)))
        return NULL;

    if (FAILED(ppshf->ParseDisplayName(NULL, NULL, guid, NULL, &pidl, NULL)))
        return NULL;

    return pidl;
}

/*************************************************************************
* SHExplorerParseCmdLine		[BROWSEUI.107]
*/
// Returns FALSE, TRUE or an address.
extern "C"
UINT_PTR
WINAPI
SHExplorerParseCmdLine(_Out_ PEXPLORER_CMDLINE_PARSE_RESULTS pInfo)
{
    WCHAR   strField[MAX_PATH];
    WCHAR   strDir[MAX_PATH];

    PCWSTR strCmdLine = GetCommandLineW();
    PCWSTR strFieldArray = PathGetArgsW(strCmdLine);

    if (!*strFieldArray)
    {
        pInfo->dwFlags = 9;
        pInfo->pidlPath = _GetDocumentsPidl();
        if (!pInfo->pidlPath)
        {
            GetWindowsDirectoryW(strDir, MAX_PATH);
            PathStripToRootW(strDir);
            pInfo->pidlPath = ILCreateFromPathW(strDir);
        }
        return (UINT_PTR)pInfo->pidlPath;
    }

    PCWSTR strNextArg = _FindFirstField(strFieldArray);

    BOOL hasNext = TRUE;

    hasNext = _ReadNextArg(&strNextArg, strField, _countof(strField));
    while (TRUE)
    {
        // Basic flags-only params first
        if (!StrCmpIW(strField, L"/N"))
        {
            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_N | SH_EXPLORER_CMDLINE_FLAG_ONE;
            TRACE("CmdLine Parser: Parsed %S flag. dwFlags=%08lx\n", strField, pInfo->dwFlags);
        }
        else if (!StrCmpIW(strField, L"/S"))
        {
            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_S;
            TRACE("CmdLine Parser: Parsed %S flag. dwFlags=%08lx\n", strField, pInfo->dwFlags);
        }
        else if (!StrCmpIW(strField, L"/E"))
        {
            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_E;
            TRACE("CmdLine Parser: Parsed %S flag. dwFlags=%08lx\n", strField, pInfo->dwFlags);
        }
        else if (!StrCmpIW(strField, L"/SELECT"))
        {
            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_SELECT;
            TRACE("CmdLine Parser: Parsed %S flag. dwFlags=%08lx\n", strField, pInfo->dwFlags);
        }
        else if (!StrCmpIW(strField, L"/NOUI"))
        {
            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_NOUI;
            TRACE("CmdLine Parser: Parsed %S flag. dwFlags=%08lx\n", strField, pInfo->dwFlags);
        }
        else if (!StrCmpIW(strField, L"-embedding"))
        {
            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_EMBED;
            TRACE("CmdLine Parser: Parsed %S flag. dwFlags=%08lx\n", strField, pInfo->dwFlags);
        }
        else if (!StrCmpIW(strField, L"/SEPARATE"))
        {
            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_SEPARATE;
            TRACE("CmdLine Parser: Parsed %S flag. dwFlags=%08lx\n", strField, pInfo->dwFlags);
        }
        else if (!StrCmpIW(strField, L"/INPROC"))
        {
            // No idea what Inproc is supposed to do, but it gets a GUID, and parses it.

            TRACE("CmdLine Parser: Found %S flag\n", strField);

            if (!hasNext)
                return FALSE;

            hasNext = _ReadNextArg(&strNextArg, strField, _countof(strField));

            if (!GUIDFromStringW(strField, &(pInfo->guidInproc)))
                return FALSE;

            pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_INPROC;

            TRACE("CmdLine Parser: Parsed /INPROC flag. dwFlags=%08lx, guidInproc=%S\n", pInfo->dwFlags, strField);
        }
        else if (!StrCmpIW(strField, L"/ROOT"))
        {
            LPITEMIDLIST pidlRoot = NULL;

            // The window should be rooted

            TRACE("CmdLine Parser: Found %S flag\n", strField);

            if (!pInfo->pidlPath)
                return FALSE;

            if (!hasNext)
                return FALSE;

            hasNext = _ReadNextArg(&strNextArg, strField, _countof(strField));

            // Root may be a pidl
            if (!StrCmpIW(strField, L"/IDLIST"))
            {
                if (hasNext)
                {
                    hasNext = _ReadNextArg(&strNextArg, strField, _countof(strField));
                }
                pidlRoot = _ILReadFromSharedMemory(strField);
            }
            else
            {
                // Or just a path string
                _ParsePathToPidl(strField, &pidlRoot);
            }

            pInfo->pidlRoot = pidlRoot;

            // The root defaults to the desktop
            if (!pidlRoot)
            {
                if (FAILED(SHGetSpecialFolderLocation(0, CSIDL_DESKTOP, &(pInfo->pidlRoot))))
                    pInfo->pidlRoot = NULL;
            }

            // TODO: Create rooted PIDL from pInfo->pidlPath and pInfo->pidlRoot

            TRACE("CmdLine Parser: Parsed /ROOT flag. dwFlags=%08lx, pidlRoot=%p\n", pInfo->dwFlags, pInfo->pidlRoot);
        }
        else
        {
            // Anything else is part of the target path to browse to
            TRACE("CmdLine Parser: Found target path %S\n", strField);

            // Which can be a shared-memory itemidlist
            if (!StrCmpIW(strField, L"/IDLIST"))
            {
                LPITEMIDLIST pidlArg;

                if (!hasNext)
                    return FALSE;

                hasNext = _ReadNextArg(&strNextArg, strField, _countof(strField));
                pidlArg = _ILReadFromSharedMemory(strField);
                if (!pidlArg)
                    return FALSE;

                if (pInfo->pidlPath)
                    ILFree(pInfo->pidlPath);
                pInfo->pidlPath = pidlArg;

                TRACE("CmdLine Parser: Parsed target path. dwFlags=%08lx, pidlPath=%p\n", pInfo->dwFlags, pInfo->pidlPath);
            }
            else
            {
                // Or just a plain old string.

                if (PathIsDirectoryW(strField))
                    PathAddBackslash(strField);

                WCHAR szPath[MAX_PATH];
                DWORD result = GetFullPathNameW(strField, _countof(szPath), szPath, NULL);

                if (result != 0 && result <= _countof(szPath) && PathFileExistsW(szPath))
                    StringCchCopyW(strField, _countof(strField), szPath);

                LPITEMIDLIST pidlPath = ILCreateFromPathW(strField);

                pInfo->pidlPath = pidlPath;

                if (pidlPath)
                {
                    pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_IDLIST;
                    TRACE("CmdLine Parser: Parsed target path. dwFlags=%08lx, pidlPath=%p\n", pInfo->dwFlags, pInfo->pidlPath);
                }
                else
                {
                    // The path could not be parsed into an ID List,
                    // so pass it on as a plain string.

                    PWSTR field = StrDupW(strField);
                    pInfo->strPath = field;
                    if (field)
                    {
                        pInfo->dwFlags |= SH_EXPLORER_CMDLINE_FLAG_STRING;
                        TRACE("CmdLine Parser: Parsed target path. dwFlags=%08lx, strPath=%S\n", pInfo->dwFlags, field);
                    }
                }

            }
        }

        if (!hasNext)
            break;
        hasNext = _ReadNextArg(&strNextArg, strField, _countof(strField));
    }

    return TRUE;
}
