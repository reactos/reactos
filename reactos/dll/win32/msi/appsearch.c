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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>

#define COBJMACROS

#include "stdio.h"
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

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tagMSISIGNATURE
{
    LPCWSTR  Name;     /* NOT owned by this structure */
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
    sig->Name = name;
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
        sig->File = msi_dup_record_field(row,2);
        minVersion = msi_dup_record_field(row,3);
        if (minVersion)
        {
            ACTION_VerStrToInteger(minVersion, &sig->MinVersionMS,
             &sig->MinVersionLS);
            msi_free( minVersion);
        }
        maxVersion = msi_dup_record_field(row,4);
        if (maxVersion)
        {
            ACTION_VerStrToInteger(maxVersion, &sig->MaxVersionMS,
             &sig->MaxVersionLS);
            msi_free( maxVersion);
        }
        sig->MinSize = MSI_RecordGetInteger(row,5);
        if (sig->MinSize == MSI_NULL_INTEGER)
            sig->MinSize = 0;
        sig->MaxSize = MSI_RecordGetInteger(row,6);
        if (sig->MaxSize == MSI_NULL_INTEGER)
            sig->MaxSize = 0;
        sig->Languages = msi_dup_record_field(row,9);
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
        TRACE("MinSize is %d, MaxSize is %d;\n", sig->MinSize, sig->MaxSize);
        TRACE("Languages is %s\n", debugstr_w(sig->Languages));

end:
        if (row)
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

/* Frees any memory allocated in sig */
static void ACTION_FreeSignature(MSISIGNATURE *sig)
{
    msi_free(sig->File);
    msi_free(sig->Languages);
}

static UINT ACTION_AppSearchComponents(MSIPACKAGE *package, LPWSTR *appValue,
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

    TRACE("(package %p, appValue %p, sig %p)\n", package, appValue, sig);
    *appValue = NULL;
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
        if (row)
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

static void ACTION_ConvertRegValue(DWORD regType, const BYTE *value, DWORD sz,
 LPWSTR *appValue)
{
    static const WCHAR dwordFmt[] = { '#','%','d','\0' };
    static const WCHAR expandSzFmt[] = { '#','%','%','%','s','\0' };
    static const WCHAR binFmt[] = { '#','x','%','x','\0' };
    DWORD i;

    switch (regType)
    {
        case REG_SZ:
            if (*(LPCWSTR)value == '#')
            {
                /* escape leading pound with another */
                *appValue = msi_alloc(sz + sizeof(WCHAR));
                (*appValue)[0] = '#';
                strcpyW(*appValue + 1, (LPCWSTR)value);
            }
            else
            {
                *appValue = msi_alloc(sz);
                strcpyW(*appValue, (LPCWSTR)value);
            }
            break;
        case REG_DWORD:
            /* 7 chars for digits, 1 for NULL, 1 for #, and 1 for sign
             * char if needed
             */
            *appValue = msi_alloc(10 * sizeof(WCHAR));
            sprintfW(*appValue, dwordFmt, *(const DWORD *)value);
            break;
        case REG_EXPAND_SZ:
            /* space for extra #% characters in front */
            *appValue = msi_alloc(sz + 2 * sizeof(WCHAR));
            sprintfW(*appValue, expandSzFmt, (LPCWSTR)value);
            break;
        case REG_BINARY:
            /* 3 == length of "#x<nibble>" */
            *appValue = msi_alloc((sz * 3 + 1) * sizeof(WCHAR));
            for (i = 0; i < sz; i++)
                sprintfW(*appValue + i * 3, binFmt, value[i]);
            break;
        default:
            WARN("unimplemented for values of type %d\n", regType);
            *appValue = NULL;
    }
}

static UINT ACTION_SearchDirectory(MSIPACKAGE *package, MSISIGNATURE *sig,
 LPCWSTR path, int depth, LPWSTR *appValue);

static UINT ACTION_AppSearchReg(MSIPACKAGE *package, LPWSTR *appValue,
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

    TRACE("(package %p, appValue %p, sig %p)\n", package, appValue, sig);
    *appValue = NULL;
    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, sig->Name);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        LPWSTR keyPath = NULL, valueName = NULL;
        int root, type;
        HKEY rootKey, key = NULL;
        DWORD sz = 0, regType;
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
        keyPath = msi_dup_record_field(row,3);
        /* FIXME: keyPath needs to be expanded for properties */
        valueName = msi_dup_record_field(row,4);
        /* FIXME: valueName probably does too */
        type = MSI_RecordGetInteger(row,5);

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

        rc = RegOpenKeyW(rootKey, keyPath, &key);
        if (rc)
        {
            TRACE("RegOpenKeyW returned %d\n", rc);
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
        value = msi_alloc( sz);
        rc = RegQueryValueExW(key, valueName, NULL, &regType, value, &sz);
        if (rc)
        {
            TRACE("RegQueryValueExW returned %d\n", rc);
            rc = ERROR_SUCCESS;
            goto end;
        }

        /* bail out if the registry key is empty */
        if (sz == 0)
        {
            rc = ERROR_SUCCESS;
            goto end;
        }

        switch (type & 0x0f)
        {
        case msidbLocatorTypeDirectory:
            rc = ACTION_SearchDirectory(package, sig, (LPCWSTR)value, 0,
             appValue);
            break;
        case msidbLocatorTypeFileName:
            *appValue = strdupW((LPCWSTR)value);
            break;
        case msidbLocatorTypeRawValue:
            ACTION_ConvertRegValue(regType, value, sz, appValue);
            break;
        default:
            FIXME("AppSearch unimplemented for type %d (key path %s, value %s)\n",
             type, debugstr_w(keyPath), debugstr_w(valueName));
        }
end:
        msi_free( value);
        RegCloseKey(key);

        msi_free( keyPath);
        msi_free( valueName);

        if (row)
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

static UINT ACTION_AppSearchIni(MSIPACKAGE *package, LPWSTR *appValue,
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

    TRACE("(package %p, appValue %p, sig %p)\n", package, appValue, sig);
    *appValue = NULL;
    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, sig->Name);
    if (rc == ERROR_SUCCESS)
    {
        MSIRECORD *row = 0;
        LPWSTR fileName, section, key;
        int field, type;
        WCHAR buf[MAX_PATH];

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

        fileName = msi_dup_record_field(row, 2);
        section = msi_dup_record_field(row, 3);
        key = msi_dup_record_field(row, 4);
        if ((field = MSI_RecordGetInteger(row, 5)) == MSI_NULL_INTEGER)
            field = 0;
        if ((type = MSI_RecordGetInteger(row, 6)) == MSI_NULL_INTEGER)
            type = 0;

        GetPrivateProfileStringW(section, key, NULL, buf,
         sizeof(buf) / sizeof(WCHAR), fileName);
        if (buf[0])
        {
            switch (type & 0x0f)
            {
            case msidbLocatorTypeDirectory:
                FIXME("unimplemented for type Directory (dir: %s)\n",
                 debugstr_w(buf));
                break;
            case msidbLocatorTypeFileName:
                FIXME("unimplemented for type File (file: %s)\n",
                 debugstr_w(buf));
                break;
            case msidbLocatorTypeRawValue:
                *appValue = strdupW(buf);
                break;
            }
        }

        msi_free(fileName);
        msi_free(section);
        msi_free(key);

end:
        if (row)
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
    {
        if (dst) *dst = '\0';
        return;
    }

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
            LPVOID buf = msi_alloc( size);

            if (buf)
            {
                static WCHAR rootW[] = { '\\',0 };
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
                msi_free( buf);
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
static UINT ACTION_RecurseSearchDirectory(MSIPACKAGE *package, LPWSTR *appValue,
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

    *appValue = NULL;
    /* We need the buffer in both paths below, so go ahead and allocate it
     * here.  Add two because we might need to add a backslash if the dir name
     * isn't backslash-terminated.
     */
    buf = msi_alloc( (dirLen + max(fileLen, lstrlenW(starDotStarW)) + 2) * sizeof(WCHAR));
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
                TRACE("found file, returning %s\n", debugstr_w(buf));
                *appValue = buf;
            }
            FindClose(hFind);
        }
        if (rc == ERROR_SUCCESS && !*appValue && depth > 0)
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
                    rc = ACTION_RecurseSearchDirectory(package, appValue, sig,
                     findData.cFileName, depth - 1);
                while (rc == ERROR_SUCCESS && !*appValue &&
                 FindNextFileW(hFind, &findData) != 0)
                {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        rc = ACTION_RecurseSearchDirectory(package, appValue,
                         sig, findData.cFileName, depth - 1);
                }
                FindClose(hFind);
            }
        }
        if (!*appValue)
            msi_free(buf);
    }
    else
        rc = ERROR_OUTOFMEMORY;

    return rc;
}

static UINT ACTION_CheckDirectory(MSIPACKAGE *package, LPCWSTR dir,
 LPWSTR *appValue)
{
    UINT rc = ERROR_SUCCESS;

    if (GetFileAttributesW(dir) & FILE_ATTRIBUTE_DIRECTORY)
    {
        TRACE("directory exists, returning %s\n", debugstr_w(dir));
        *appValue = strdupW(dir);
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
 LPCWSTR path, int depth, LPWSTR *appValue)
{
    UINT rc;

    TRACE("%p, %p, %s, %d, %p\n", package, sig, debugstr_w(path), depth,
     appValue);
    if (ACTION_IsFullPath(path))
    {
        if (sig->File)
            rc = ACTION_RecurseSearchDirectory(package, appValue, sig,
             path, depth);
        else
        {
            /* Recursively searching a directory makes no sense when the
             * directory to search is the thing you're trying to find.
             */
            rc = ACTION_CheckDirectory(package, path, appValue);
        }
    }
    else
    {
        WCHAR pathWithDrive[MAX_PATH] = { 'C',':','\\',0 };
        DWORD drives = GetLogicalDrives();
        int i;

        rc = ERROR_SUCCESS;
        *appValue = NULL;
        for (i = 0; rc == ERROR_SUCCESS && !*appValue && i < 26; i++)
            if (drives & (1 << drives))
            {
                pathWithDrive[0] = 'A' + i;
                if (GetDriveTypeW(pathWithDrive) == DRIVE_FIXED)
                {
                    lstrcpynW(pathWithDrive + 3, path,
                              sizeof(pathWithDrive) / sizeof(pathWithDrive[0]) - 3);
                    if (sig->File)
                        rc = ACTION_RecurseSearchDirectory(package, appValue,
                         sig, pathWithDrive, depth);
                    else
                        rc = ACTION_CheckDirectory(package, pathWithDrive,
                         appValue);
                }
            }
    }
    TRACE("returning %d\n", rc);
    return rc;
}

static UINT ACTION_AppSearchSigName(MSIPACKAGE *package, LPCWSTR sigName,
 MSISIGNATURE *sig, LPWSTR *appValue);

static UINT ACTION_AppSearchDr(MSIPACKAGE *package, LPWSTR *appValue,
 MSISIGNATURE *sig)
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
        WCHAR expanded[MAX_PATH];
        LPWSTR parentName = NULL, path = NULL, parent = NULL;
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
        parentName = msi_dup_record_field(row,2);
        if (parentName)
        {
            MSISIGNATURE parentSig;

            rc = ACTION_AppSearchSigName(package, parentName, &parentSig,
             &parent);
            ACTION_FreeSignature(&parentSig);
            msi_free(parentName);
        }
        /* now look for path */
        path = msi_dup_record_field(row,3);
        if (MSI_RecordIsNull(row,4))
            depth = 0;
        else
            depth = MSI_RecordGetInteger(row,4);
        ACTION_ExpandAnyPath(package, path, expanded,
         sizeof(expanded) / sizeof(expanded[0]));
        msi_free(path);
        if (parent)
        {
            path = msi_alloc((strlenW(parent) + strlenW(expanded) + 1) * sizeof(WCHAR));
            if (!path)
                goto end;
            strcpyW(path, parent);
            strcatW(path, expanded);
        }
        else
            path = expanded;
        rc = ACTION_SearchDirectory(package, sig, path, depth, appValue);

end:
        if (path != expanded)
            msi_free(path);
        msi_free(parent);
        if (row)
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

static UINT ACTION_AppSearchSigName(MSIPACKAGE *package, LPCWSTR sigName,
 MSISIGNATURE *sig, LPWSTR *appValue)
{
    UINT rc;

    *appValue = NULL;
    rc = ACTION_AppSearchGetSignature(package, sig, sigName);
    if (rc == ERROR_SUCCESS)
    {
        rc = ACTION_AppSearchComponents(package, appValue, sig);
        if (rc == ERROR_SUCCESS && !*appValue)
        {
            rc = ACTION_AppSearchReg(package, appValue, sig);
            if (rc == ERROR_SUCCESS && !*appValue)
            {
                rc = ACTION_AppSearchIni(package, appValue, sig);
                if (rc == ERROR_SUCCESS && !*appValue)
                    rc = ACTION_AppSearchDr(package, appValue, sig);
            }
        }
    }
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
        LPWSTR propName, sigName;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
            goto end;

        while (!rc)
        {
            MSISIGNATURE sig;
            LPWSTR value = NULL;

            rc = MSI_ViewFetch(view,&row);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            /* get property and signature */
            propName = msi_dup_record_field(row,1);
            sigName = msi_dup_record_field(row,2);

            TRACE("Searching for Property %s, Signature_ %s\n",
             debugstr_w(propName), debugstr_w(sigName));

            rc = ACTION_AppSearchSigName(package, sigName, &sig, &value);
            if (value)
            {
                MSI_SetPropertyW(package, propName, value);
                msi_free(value);
            }
            ACTION_FreeSignature(&sig);
            msi_free(propName);
            msi_free(sigName);
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
