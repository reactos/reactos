/*
 * Implementation of the AppSearch action of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Juan Lang
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
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "msi.h"
#include "msiquery.h"
#include "msidefs.h"
#include "winver.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "msipriv.h"
#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tagMSISIGNATURE
{
    LPWSTR   Name;     /* NOT owned by this structure */
    LPWSTR   Property; /* NOT owned by this structure */
    LPWSTR   File;
    DWORD    MinVersionMS;
    DWORD    MinVersionLS;
    DWORD    MaxVersionMS;
    DWORD    MaxVersionLS;
    DWORD    MinSize;
    DWORD    MaxSize;
    FILETIME MinTime;
    FILETIME MaxTime;
    LPWSTR   Languages;
}MSISIGNATURE;

static void ACTION_VerStrToInteger(LPCWSTR verStr, PDWORD ms, PDWORD ls)
{
    const WCHAR *ptr;
    int x1 = 0, x2 = 0, x3 = 0, x4 = 0;

    x1 = atoiW(verStr);
    ptr = strchrW(verStr, '.');
    if (ptr)
    {
        x2 = atoiW(ptr + 1);
        ptr = strchrW(ptr + 1, '.');
    }
    if (ptr)
    {
        x3 = atoiW(ptr + 1);
        ptr = strchrW(ptr + 1, '.');
    }
    if (ptr)
        x4 = atoiW(ptr + 1);
    /* FIXME: byte-order dependent? */
    *ms = x1 << 16 | x2;
    *ls = x3 << 16 | x4;
}

/* Fills in sig with the the values from the Signature table, where name is the
 * signature to find.  Upon return, sig->File will be NULL if the record is not
 * found, and not NULL if it is found.
 * Warning: clears all fields in sig!
 * Returns ERROR_SUCCESS upon success (where not finding the record counts as
 * success), something else on error.
 */
static UINT ACTION_AppSearchGetSignature(MSIPACKAGE *package, MSISIGNATURE *sig,
 LPCWSTR name)
{
    MSIQUERY *view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =  {
   's','e','l','e','c','t',' ','*',' ',
   'f','r','o','m',' ',
   'S','i','g','n','a','t','u','r','e',' ',
   'w','h','e','r','e',' ','S','i','g','n','a','t','u','r','e',' ','=',' ',
   '\'','%','s','\'',0};

    TRACE("(package %p, sig %p)\n", package, sig);
    memset(sig, 0, sizeof(*sig));
    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, name);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        DWORD time;
        WCHAR *minVersion, *maxVersion;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewExecute returned %d\n", rc);
            goto end;
        }
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewFetch returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }

        /* get properties */
        sig->File = load_dynamic_stringW(row,2);
        minVersion = load_dynamic_stringW(row,3);
        if (minVersion)
        {
            ACTION_VerStrToInteger(minVersion, &sig->MinVersionMS,
             &sig->MinVersionLS);
            HeapFree(GetProcessHeap(), 0, minVersion);
        }
        maxVersion = load_dynamic_stringW(row,4);
        if (maxVersion)
        {
            ACTION_VerStrToInteger(maxVersion, &sig->MaxVersionMS,
             &sig->MaxVersionLS);
            HeapFree(GetProcessHeap(), 0, maxVersion);
        }
        sig->MinSize = MSI_RecordGetInteger(row,5);
        if (sig->MinSize == MSI_NULL_INTEGER)
            sig->MinSize = 0;
        sig->MaxSize = MSI_RecordGetInteger(row,6);
        if (sig->MaxSize == MSI_NULL_INTEGER)
            sig->MaxSize = 0;
        sig->Languages = load_dynamic_stringW(row,9);
        time = MSI_RecordGetInteger(row,7);
        if (time != MSI_NULL_INTEGER)
            DosDateTimeToFileTime(HIWORD(time), LOWORD(time), &sig->MinTime);
        time = MSI_RecordGetInteger(row,8);
        if (time != MSI_NULL_INTEGER)
            DosDateTimeToFileTime(HIWORD(time), LOWORD(time), &sig->MaxTime);
        TRACE("Found file name %s for Signature_ %s;\n",
         debugstr_w(sig->File), debugstr_w(name));
        TRACE("MinVersion is %d.%d.%d.%d\n", HIWORD(sig->MinVersionMS),
         LOWORD(sig->MinVersionMS), HIWORD(sig->MinVersionLS),
         LOWORD(sig->MinVersionLS));
        TRACE("MaxVersion is %d.%d.%d.%d\n", HIWORD(sig->MaxVersionMS),
         LOWORD(sig->MaxVersionMS), HIWORD(sig->MaxVersionLS),
         LOWORD(sig->MaxVersionLS));
        TRACE("MinSize is %ld, MaxSize is %ld;\n", sig->MinSize, sig->MaxSize);
        TRACE("Languages is %s\n", debugstr_w(sig->Languages));

end:
        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    else
    {
        TRACE("MSI_OpenQuery returned %d\n", rc);
        rc = ERROR_SUCCESS;
    }

    TRACE("returning %d\n", rc);
    return rc;
}

static UINT ACTION_AppSearchComponents(MSIPACKAGE *package, BOOL *appFound,
 MSISIGNATURE *sig)
{
    MSIQUERY *view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =  {
   's','e','l','e','c','t',' ','*',' ',
   'f','r','o','m',' ',
   'C','o','m','p','L','o','c','a','t','o','r',' ',
   'w','h','e','r','e',' ','S','i','g','n','a','t','u','r','e','_',' ','=',' ',
   '\'','%','s','\'',0};

    TRACE("(package %p, appFound %p, sig %p)\n", package, appFound, sig);
    *appFound = FALSE;
    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, sig->Name);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        WCHAR guid[50];
        DWORD sz;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewExecute returned %d\n", rc);
            goto end;
        }
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewFetch returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }

        /* get GUID */
        guid[0] = 0;
        sz=sizeof(guid)/sizeof(guid[0]);
        rc = MSI_RecordGetStringW(row,2,guid,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Error is %x\n",rc);
            goto end;
        }
        FIXME("AppSearch unimplemented for CompLocator table (GUID %s)\n",
         debugstr_w(guid));

end:
        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    else
    {
        TRACE("MSI_OpenQuery returned %d\n", rc);
        rc = ERROR_SUCCESS;
    }

    TRACE("returning %d\n", rc);
    return rc;
}

static UINT ACTION_AppSearchReg(MSIPACKAGE *package, BOOL *appFound,
 MSISIGNATURE *sig)
{
    MSIQUERY *view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =  {
   's','e','l','e','c','t',' ','*',' ',
   'f','r','o','m',' ',
   'R','e','g','L','o','c','a','t','o','r',' ',
   'w','h','e','r','e',' ','S','i','g','n','a','t','u','r','e','_',' ','=',' ',
   '\'','%','s','\'',0};
    static const WCHAR dwordFmt[] = { '#','%','d','\0' };
    static const WCHAR expandSzFmt[] = { '#','%','%','%','s','\0' };
    static const WCHAR binFmt[] = { '#','x','%','x','\0' };

    TRACE("(package %p, appFound %p, sig %p)\n", package, appFound, sig);
    *appFound = FALSE;
    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, sig->Name);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        LPWSTR keyPath = NULL, valueName = NULL, propertyValue = NULL;
        int root, type;
        HKEY rootKey, key = NULL;
        DWORD sz = 0, regType, i;
        LPBYTE value = NULL;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewExecute returned %d\n", rc);
            goto end;
        }
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewFetch returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }

        root = MSI_RecordGetInteger(row,2);
        keyPath = load_dynamic_stringW(row,3);
        /* FIXME: keyPath needs to be expanded for properties */
        valueName = load_dynamic_stringW(row,4);
        /* FIXME: valueName probably does too */
        type = MSI_RecordGetInteger(row,5);

        if ((type & 0x0f) != msidbLocatorTypeRawValue)
        {
            FIXME("AppSearch unimplemented for type %d (key path %s, value %s)\n",
             type, debugstr_w(keyPath), debugstr_w(valueName));
            goto end;
        }

        switch (root)
        {
            case msidbRegistryRootClassesRoot:
                rootKey = HKEY_CLASSES_ROOT;
                break;
            case msidbRegistryRootCurrentUser:
                rootKey = HKEY_CURRENT_USER;
                break;
            case msidbRegistryRootLocalMachine:
                rootKey = HKEY_LOCAL_MACHINE;
                break;
            case msidbRegistryRootUsers:
                rootKey = HKEY_USERS;
                break;
            default:
                WARN("Unknown root key %d\n", root);
                goto end;
        }

        rc = RegCreateKeyW(rootKey, keyPath, &key);
        if (rc)
        {
            TRACE("RegCreateKeyW returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }
        rc = RegQueryValueExW(key, valueName, NULL, NULL, NULL, &sz);
        if (rc)
        {
            TRACE("RegQueryValueExW returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }
        /* FIXME: sanity-check sz before allocating (is there an upper-limit
         * on the value of a property?)
         */
        value = HeapAlloc(GetProcessHeap(), 0, sz);
        rc = RegQueryValueExW(key, valueName, NULL, &regType, value, &sz);
        if (rc)
        {
            TRACE("RegQueryValueExW returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }

        switch (regType)
        {
            case REG_SZ:
                if (*(LPWSTR)value == '#')
                {
                    /* escape leading pound with another */
                    propertyValue = HeapAlloc(GetProcessHeap(), 0,
                     sz + sizeof(WCHAR));
                    propertyValue[0] = '#';
                    strcpyW(propertyValue + 1, (LPWSTR)value);
                }
                else
                {
                    propertyValue = HeapAlloc(GetProcessHeap(), 0, sz);
                    strcpyW(propertyValue, (LPWSTR)value);
                }
                break;
            case REG_DWORD:
                /* 7 chars for digits, 1 for NULL, 1 for #, and 1 for sign
                 * char if needed
                 */
                propertyValue = HeapAlloc(GetProcessHeap(), 0,
                 10 * sizeof(WCHAR));
                sprintfW(propertyValue, dwordFmt, *(DWORD *)value);
                break;
            case REG_EXPAND_SZ:
                /* space for extra #% characters in front */
                propertyValue = HeapAlloc(GetProcessHeap(), 0,
                 sz + 2 * sizeof(WCHAR));
                sprintfW(propertyValue, expandSzFmt, (LPWSTR)value);
                break;
            case REG_BINARY:
                /* 3 == length of "#x<nibble>" */
                propertyValue = HeapAlloc(GetProcessHeap(), 0,
                 (sz * 3 + 1) * sizeof(WCHAR));
                for (i = 0; i < sz; i++)
                    sprintfW(propertyValue + i * 3, binFmt, value[i]);
                break;
            default:
                WARN("unimplemented for values of type %ld\n", regType);
                goto end;
        }

        TRACE("found registry value, setting %s to %s\n",
         debugstr_w(sig->Property), debugstr_w(propertyValue));
        rc = MSI_SetPropertyW(package, sig->Property, propertyValue);
        *appFound = TRUE;

end:
        HeapFree(GetProcessHeap(), 0, propertyValue);
        HeapFree(GetProcessHeap(), 0, value);
        RegCloseKey(key);

        HeapFree(GetProcessHeap(), 0, keyPath);
        HeapFree(GetProcessHeap(), 0, valueName);

        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    else
    {
        TRACE("MSI_OpenQuery returned %d\n", rc);
        rc = ERROR_SUCCESS;
    }

    TRACE("returning %d\n", rc);
    return rc;
}

static UINT ACTION_AppSearchIni(MSIPACKAGE *package, BOOL *appFound,
 MSISIGNATURE *sig)
{
    MSIQUERY *view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =  {
   's','e','l','e','c','t',' ','*',' ',
   'f','r','o','m',' ',
   'I','n','i','L','o','c','a','t','o','r',' ',
   'w','h','e','r','e',' ','S','i','g','n','a','t','u','r','e','_',' ','=',' ',
   '\'','%','s','\'',0};

    TRACE("(package %p, appFound %p, sig %p)\n", package, appFound, sig);
    *appFound = FALSE;
    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, sig->Name);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        LPWSTR fileName;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewExecute returned %d\n", rc);
            goto end;
        }
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewFetch returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }

        /* get file name */
        fileName = load_dynamic_stringW(row,2);
        FIXME("AppSearch unimplemented for IniLocator (ini file name %s)\n",
         debugstr_w(fileName));
        HeapFree(GetProcessHeap(), 0, fileName);

end:
        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    else
    {
        TRACE("MSI_OpenQuery returned %d\n", rc);
        rc = ERROR_SUCCESS;
    }


    TRACE("returning %d\n", rc);
    return rc;
}

/* Expands the value in src into a path without property names and only
 * containing long path names into dst.  Replaces at most len characters of dst,
 * and always NULL-terminates dst if dst is not NULL and len >= 1.
 * May modify src.
 * Assumes src and dst are non-overlapping.
 * FIXME: return code probably needed:
 * - what does AppSearch return if the table values are invalid?
 * - what if dst is too small?
 */
static void ACTION_ExpandAnyPath(MSIPACKAGE *package, WCHAR *src, WCHAR *dst,
 size_t len)
{
    WCHAR *ptr;
    size_t copied = 0;

    if (!src || !dst || !len)
        return;

    /* Ignore the short portion of the path, don't think we can use it anyway */
    if ((ptr = strchrW(src, '|')))
        ptr++;
    else
        ptr = src;
    while (*ptr && copied < len - 1)
    {
        WCHAR *prop = strchrW(ptr, '[');

        if (prop)
        {
            WCHAR *propEnd = strchrW(prop + 1, ']');

            if (!propEnd)
            {
                WARN("Unterminated property name in AnyPath: %s\n",
                 debugstr_w(prop));
                break;
            }
            else
            {
                DWORD propLen;

                *propEnd = 0;
                propLen = len - copied - 1;
                MSI_GetPropertyW(package, prop + 1, dst + copied, &propLen);
                ptr = propEnd + 1;
                copied += propLen;
            }
        }
        else
        {
            size_t toCopy = min(strlenW(ptr) + 1, len - copied - 1);

            memcpy(dst + copied, ptr, toCopy * sizeof(WCHAR));
            ptr += toCopy;
            copied += toCopy;
        }
    }
    *(dst + copied) = '\0';
}

/* Sets *matches to whether the file (whose path is filePath) matches the
 * versions set in sig.
 * Return ERROR_SUCCESS in case of success (whether or not the file matches),
 * something else if an install-halting error occurs.
 */
static UINT ACTION_FileVersionMatches(MSISIGNATURE *sig, LPCWSTR filePath,
 BOOL *matches)
{
    UINT rc = ERROR_SUCCESS;

    *matches = FALSE;
    if (sig->Languages)
    {
        FIXME(": need to check version for languages %s\n",
         debugstr_w(sig->Languages));
    }
    else
    {
        DWORD zero, size = GetFileVersionInfoSizeW(filePath, &zero);

        if (size)
        {
            LPVOID buf = HeapAlloc(GetProcessHeap(), 0, size);

            if (buf)
            {
                static const WCHAR rootW[] = { '\\',0 };
                UINT versionLen;
                LPVOID subBlock = NULL;

                if (GetFileVersionInfoW(filePath, 0, size, buf))
                    VerQueryValueW(buf, rootW, &subBlock, &versionLen);
                if (subBlock)
                {
                    VS_FIXEDFILEINFO *info =
                     (VS_FIXEDFILEINFO *)subBlock;

                    TRACE("Comparing file version %d.%d.%d.%d:\n",
                     HIWORD(info->dwFileVersionMS),
                     LOWORD(info->dwFileVersionMS),
                     HIWORD(info->dwFileVersionLS),
                     LOWORD(info->dwFileVersionLS));
                    if (info->dwFileVersionMS < sig->MinVersionMS
                     || (info->dwFileVersionMS == sig->MinVersionMS &&
                     info->dwFileVersionLS < sig->MinVersionLS))
                    {
                        TRACE("Less than minimum version %d.%d.%d.%d\n",
                         HIWORD(sig->MinVersionMS),
                         LOWORD(sig->MinVersionMS),
                         HIWORD(sig->MinVersionLS),
                         LOWORD(sig->MinVersionLS));
                    }
                    else if (info->dwFileVersionMS < sig->MinVersionMS
                     || (info->dwFileVersionMS == sig->MinVersionMS &&
                     info->dwFileVersionLS < sig->MinVersionLS))
                    {
                        TRACE("Greater than minimum version %d.%d.%d.%d\n",
                         HIWORD(sig->MaxVersionMS),
                         LOWORD(sig->MaxVersionMS),
                         HIWORD(sig->MaxVersionLS),
                         LOWORD(sig->MaxVersionLS));
                    }
                    else
                        *matches = TRUE;
                }
                HeapFree(GetProcessHeap(), 0, buf);
            }
            else
                rc = ERROR_OUTOFMEMORY;
        }
    }
    return rc;
}

/* Sets *matches to whether the file in findData matches that in sig.
 * fullFilePath is assumed to be the full path of the file specified in
 * findData, which may be necessary to compare the version.
 * Return ERROR_SUCCESS in case of success (whether or not the file matches),
 * something else if an install-halting error occurs.
 */
static UINT ACTION_FileMatchesSig(MSISIGNATURE *sig,
 LPWIN32_FIND_DATAW findData, LPCWSTR fullFilePath, BOOL *matches)
{
    UINT rc = ERROR_SUCCESS;

    *matches = TRUE;
    /* assumes the caller has already ensured the filenames match, so check
     * the other fields..
     */
    if (sig->MinTime.dwLowDateTime || sig->MinTime.dwHighDateTime)
    {
        if (findData->ftCreationTime.dwHighDateTime <
         sig->MinTime.dwHighDateTime ||
         (findData->ftCreationTime.dwHighDateTime == sig->MinTime.dwHighDateTime
         && findData->ftCreationTime.dwLowDateTime <
         sig->MinTime.dwLowDateTime))
            *matches = FALSE;
    }
    if (*matches && (sig->MaxTime.dwLowDateTime || sig->MaxTime.dwHighDateTime))
    {
        if (findData->ftCreationTime.dwHighDateTime >
         sig->MaxTime.dwHighDateTime ||
         (findData->ftCreationTime.dwHighDateTime == sig->MaxTime.dwHighDateTime
         && findData->ftCreationTime.dwLowDateTime >
         sig->MaxTime.dwLowDateTime))
            *matches = FALSE;
    }
    if (*matches && sig->MinSize && findData->nFileSizeLow < sig->MinSize)
        *matches = FALSE;
    if (*matches && sig->MaxSize && findData->nFileSizeLow > sig->MaxSize)
        *matches = FALSE;
    if (*matches && (sig->MinVersionMS || sig->MinVersionLS ||
     sig->MaxVersionMS || sig->MaxVersionLS))
        rc = ACTION_FileVersionMatches(sig, fullFilePath, matches);
    return rc;
}

/* Recursively searches the directory dir for files that match the signature
 * sig, up to (depth + 1) levels deep.  That is, if depth is 0, it searches dir
 * (and only dir).  If depth is 1, searches dir and its immediate
 * subdirectories.
 * Assumes sig->File is not NULL.
 * Returns ERROR_SUCCESS on success (which may include non-critical errors),
 * something else on failures which should halt the install.
 */
static UINT ACTION_RecurseSearchDirectory(MSIPACKAGE *package, BOOL *appFound,
 MSISIGNATURE *sig, LPCWSTR dir, int depth)
{
    static const WCHAR starDotStarW[] = { '*','.','*',0 };
    UINT rc = ERROR_SUCCESS;
    size_t dirLen = lstrlenW(dir), fileLen = lstrlenW(sig->File);
    WCHAR *buf;

    TRACE("Searching directory %s for file %s, depth %d\n", debugstr_w(dir),
     debugstr_w(sig->File), depth);

    if (depth < 0)
        return ERROR_INVALID_PARAMETER;

    *appFound = FALSE;
    /* We need the buffer in both paths below, so go ahead and allocate it
     * here.  Add two because we might need to add a backslash if the dir name
     * isn't backslash-terminated.
     */
    buf = HeapAlloc(GetProcessHeap(), 0,
     (dirLen + max(fileLen, lstrlenW(starDotStarW)) + 2) * sizeof(WCHAR));
    if (buf)
    {
        /* a depth of 0 implies we should search dir, so go ahead and search */
        HANDLE hFind;
        WIN32_FIND_DATAW findData;

        memcpy(buf, dir, dirLen * sizeof(WCHAR));
        if (buf[dirLen - 1] != '\\')
            buf[dirLen++ - 1] = '\\';
        memcpy(buf + dirLen, sig->File, (fileLen + 1) * sizeof(WCHAR));
        hFind = FindFirstFileW(buf, &findData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            BOOL matches;

            /* assuming Signature can't contain wildcards for the file name,
             * so don't bother with FindNextFileW here.
             */
            if (!(rc = ACTION_FileMatchesSig(sig, &findData, buf, &matches))
             && matches)
            {
                TRACE("found file, setting %s to %s\n",
                 debugstr_w(sig->Property), debugstr_w(buf));
                rc = MSI_SetPropertyW(package, sig->Property, buf);
                *appFound = TRUE;
            }
            FindClose(hFind);
        }
        if (rc == ERROR_SUCCESS && !*appFound && depth > 0)
        {
            HANDLE hFind;
            WIN32_FIND_DATAW findData;

            memcpy(buf, dir, dirLen * sizeof(WCHAR));
            if (buf[dirLen - 1] != '\\')
                buf[dirLen++ - 1] = '\\';
            lstrcpyW(buf + dirLen, starDotStarW);
            hFind = FindFirstFileW(buf, &findData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    rc = ACTION_RecurseSearchDirectory(package, appFound,
                     sig, findData.cFileName, depth - 1);
                while (rc == ERROR_SUCCESS && !*appFound &&
                 FindNextFileW(hFind, &findData) != 0)
                {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        rc = ACTION_RecurseSearchDirectory(package, appFound,
                         sig, findData.cFileName, depth - 1);
                }
                FindClose(hFind);
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
    else
        rc = ERROR_OUTOFMEMORY;

    return rc;
}

static UINT ACTION_CheckDirectory(MSIPACKAGE *package, MSISIGNATURE *sig,
 LPCWSTR dir)
{
    UINT rc = ERROR_SUCCESS;

    if (GetFileAttributesW(dir) & FILE_ATTRIBUTE_DIRECTORY)
    {
        TRACE("directory exists, setting %s to %s\n",
         debugstr_w(sig->Property), debugstr_w(dir));
        rc = MSI_SetPropertyW(package, sig->Property, dir);
    }
    return rc;
}

static BOOL ACTION_IsFullPath(LPCWSTR path)
{
    WCHAR first = toupperW(path[0]);
    BOOL ret;

    if (first >= 'A' && first <= 'Z' && path[1] == ':')
        ret = TRUE;
    else if (path[0] == '\\' && path[1] == '\\')
        ret = TRUE;
    else
        ret = FALSE;
    return ret;
}

static UINT ACTION_SearchDirectory(MSIPACKAGE *package, MSISIGNATURE *sig,
 LPCWSTR expanded, int depth)
{
    UINT rc;
    BOOL found;

    TRACE("%p, %p, %s, %d\n", package, sig, debugstr_w(expanded), depth);
    if (ACTION_IsFullPath(expanded))
    {
        if (sig->File)
            rc = ACTION_RecurseSearchDirectory(package, &found, sig,
             expanded, depth);
        else
        {
            /* Recursively searching a directory makes no sense when the
             * directory to search is the thing you're trying to find.
             */
            rc = ACTION_CheckDirectory(package, sig, expanded);
        }
    }
    else
    {
        WCHAR pathWithDrive[MAX_PATH] = { 'C',':','\\',0 };
        DWORD drives = GetLogicalDrives();
        int i;

        rc = ERROR_SUCCESS;
        found = FALSE;
        for (i = 0; rc == ERROR_SUCCESS && !found && i < 26; i++)
            if (drives & (1 << drives))
            {
                pathWithDrive[0] = 'A' + i;
                if (GetDriveTypeW(pathWithDrive) == DRIVE_FIXED)
                {
                    lstrcpynW(pathWithDrive + 3, expanded,
                              sizeof(pathWithDrive) / sizeof(pathWithDrive[0]) - 3);
                    if (sig->File)
                        rc = ACTION_RecurseSearchDirectory(package, &found, sig,
                         pathWithDrive, depth);
                    else
                        rc = ACTION_CheckDirectory(package, sig, pathWithDrive);
                }
            }
    }
    TRACE("returning %d\n", rc);
    return rc;
}

static UINT ACTION_AppSearchDr(MSIPACKAGE *package, MSISIGNATURE *sig)
{
    MSIQUERY *view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =  {
   's','e','l','e','c','t',' ','*',' ',
   'f','r','o','m',' ',
   'D','r','L','o','c','a','t','o','r',' ',
   'w','h','e','r','e',' ','S','i','g','n','a','t','u','r','e','_',' ','=',' ',
   '\'','%','s','\'',0};

    TRACE("(package %p, sig %p)\n", package, sig);
    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, sig->Name);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        WCHAR buffer[MAX_PATH], expanded[MAX_PATH];
        DWORD sz;
        int depth;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewExecute returned %d\n", rc);
            goto end;
        }
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("MSI_ViewFetch returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }

        /* check whether parent is set */
        buffer[0] = 0;
        sz=sizeof(buffer)/sizeof(buffer[0]);
        rc = MSI_RecordGetStringW(row,2,buffer,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Error is %x\n",rc);
            goto end;
        }
        else if (buffer[0])
        {
            FIXME(": searching parent (%s) unimplemented\n",
             debugstr_w(buffer));
            goto end;
        }
        /* no parent, now look for path */
        buffer[0] = 0;
        sz=sizeof(buffer)/sizeof(buffer[0]);
        rc = MSI_RecordGetStringW(row,3,buffer,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Error is %x\n",rc);
            goto end;
        }
        if (MSI_RecordIsNull(row,4))
            depth = 0;
        else
            depth = MSI_RecordGetInteger(row,4);
        ACTION_ExpandAnyPath(package, buffer, expanded,
         sizeof(expanded) / sizeof(expanded[0]));
        rc = ACTION_SearchDirectory(package, sig, expanded, depth);

end:
        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    else
    {
        TRACE("MSI_OpenQuery returned %d\n", rc);
        rc = ERROR_SUCCESS;
    }


    TRACE("returning %d\n", rc);
    return rc;
}

/* http://msdn.microsoft.com/library/en-us/msi/setup/appsearch_table.asp
 * is the best reference for the AppSearch table and how it's used.
 */
UINT ACTION_AppSearch(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =  {
   's','e','l','e','c','t',' ','*',' ',
   'f','r','o','m',' ',
   'A','p','p','S','e','a','r','c','h',0};

    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        WCHAR propBuf[0x100], sigBuf[0x100];
        DWORD sz;
        MSISIGNATURE sig;
        BOOL appFound = FALSE;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
            goto end;

        while (!rc)
        {
            rc = MSI_ViewFetch(view,&row);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            /* get property and signature */
            propBuf[0] = 0;
            sz=sizeof(propBuf)/sizeof(propBuf[0]);
            rc = MSI_RecordGetStringW(row,1,propBuf,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Error is %x\n",rc);
                msiobj_release(&row->hdr);
                break;
            }
            sigBuf[0] = 0;
            sz=sizeof(sigBuf)/sizeof(sigBuf[0]);
            rc = MSI_RecordGetStringW(row,2,sigBuf,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Error is %x\n",rc);
                msiobj_release(&row->hdr);
                break;
            }
            TRACE("Searching for Property %s, Signature_ %s\n",
             debugstr_w(propBuf), debugstr_w(sigBuf));
            /* This clears all the fields, so set Name and Property afterward */
            rc = ACTION_AppSearchGetSignature(package, &sig, sigBuf);
            sig.Name = sigBuf;
            sig.Property = propBuf;
            if (rc == ERROR_SUCCESS)
            {
                rc = ACTION_AppSearchComponents(package, &appFound, &sig);
                if (rc == ERROR_SUCCESS && !appFound)
                {
                    rc = ACTION_AppSearchReg(package, &appFound, &sig);
                    if (rc == ERROR_SUCCESS && !appFound)
                    {
                        rc = ACTION_AppSearchIni(package, &appFound, &sig);
                        if (rc == ERROR_SUCCESS && !appFound)
                            rc = ACTION_AppSearchDr(package, &sig);
                    }
                }
            }
            HeapFree(GetProcessHeap(), 0, sig.File);
            HeapFree(GetProcessHeap(), 0, sig.Languages);
            msiobj_release(&row->hdr);
        }

end:
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    else
        rc = ERROR_SUCCESS;

    return rc;
}
