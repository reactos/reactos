/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Mislav Blažević
 * Copyright 2015-2017 Mark Jansen
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

#define WIN32_NO_STATUS
#include "windows.h"
#include "ntndk.h"
#include "strsafe.h"
#include "apphelp.h"

#include "wine/unicode.h"

#define MAX_LAYER_LENGTH            256
#define GPLK_USER                   1
#define GPLK_MACHINE                2

static BOOL WINAPI SdbpFileExists(LPCWSTR path)
{
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

static BOOL WINAPI SdbpMatchExe(PDB db, TAGID exe, WCHAR* dir)
{
    static const WCHAR fmt[] = {'%','s','%','s',0};
    WCHAR buffer[256];  /* FIXME: rewrite using a buffer that can grow if needed, f.e. RtlInitBuffer stuff! */
    TAGID matching_file;

    /* TODO: check size/checksum from the main exe as well as from the extra files */
    for (matching_file = SdbFindFirstTag(db, exe, TAG_MATCHING_FILE);
            matching_file != TAGID_NULL; matching_file = SdbFindNextTag(db, exe, matching_file))
    {
        TAGID tagName = SdbFindFirstTag(db, matching_file, TAG_NAME);
        LPWSTR name = SdbGetStringTagPtr(db, tagName);

        if (!wcscmp(name, L"*"))
        {
            // if attributes dont match main file, return FALSE!
            continue;
        }

        snprintfW(buffer, _countof(buffer), fmt, dir, name);
        if (!SdbpFileExists(buffer))
            return FALSE;
    }

    return TRUE;
}

static void SdbpAddDatabaseGuid(PDB db, PSDBQUERYRESULT result)
{
    size_t n;

    for (n = 0; n < _countof(result->rgGuidDB); ++n)
    {
        if (!memcmp(&result->rgGuidDB[n], &db->database_id, sizeof(db->database_id)))
            return;

        if (result->dwCustomSDBMap & (1<<n))
            continue;

        memcpy(&result->rgGuidDB[n], &db->database_id, sizeof(result->rgGuidDB[n]));
        result->dwCustomSDBMap |= (1<<n);
        return;
    }
}

static BOOL SdbpAddSingleLayerMatch(TAGREF layer, PSDBQUERYRESULT result)
{
    size_t n;

    for (n = 0; n < result->dwLayerCount; ++n)
    {
        if (result->atrLayers[n] == layer)
            return FALSE;
    }

    if (n >= _countof(result->atrLayers))
        return FALSE;

    result->atrLayers[n] = layer;
    result->dwLayerCount++;

    return TRUE;
}


static BOOL SdbpAddNamedLayerMatch(HSDB hsdb, PCWSTR layerName, PSDBQUERYRESULT result)
{
    TAGID database, layer;
    TAGREF tr;
    PDB db = hsdb->db;

    database = SdbFindFirstTag(db, TAGID_ROOT, TAG_DATABASE);
    if (database == TAGID_NULL)
        return FALSE;

    layer = SdbFindFirstNamedTag(db, database, TAG_LAYER, TAG_NAME, layerName);
    if (layer == TAGID_NULL)
        return FALSE;

    if (!SdbTagIDToTagRef(hsdb, db, layer, &tr))
        return FALSE;

    if (!SdbpAddSingleLayerMatch(tr, result))
        return FALSE;

    SdbpAddDatabaseGuid(db, result);
    return TRUE;
}

static void SdbpAddExeLayers(HSDB hsdb, PDB db, TAGID tagExe, PSDBQUERYRESULT result)
{
    TAGID layer = SdbFindFirstTag(db, tagExe, TAG_LAYER);

    while (layer != TAGID_NULL)
    {
        TAGREF tr;
        TAGID layerIdTag = SdbFindFirstTag(db, layer, TAG_LAYER_TAGID);
        DWORD tagId = SdbReadDWORDTag(db, layerIdTag, TAGID_NULL);

        if (layerIdTag != TAGID_NULL &&
            tagId != TAGID_NULL &&
            SdbTagIDToTagRef(hsdb, db, tagId, &tr))
        {
            SdbpAddSingleLayerMatch(tr, result);
        }
        else
        {
            /* Try a name lookup */
            TAGID layerTag = SdbFindFirstTag(db, layer, TAG_NAME);
            if (layerTag != TAGID_NULL)
            {
                LPCWSTR layerName = SdbGetStringTagPtr(db, layerTag);
                if (layerName)
                {
                    SdbpAddNamedLayerMatch(hsdb, layerName, result);
                }
            }
        }

        layer = SdbFindNextTag(db, tagExe, layer);
    }
}

static void SdbpAddExeMatch(HSDB hsdb, PDB db, TAGID tagExe, PSDBQUERYRESULT result)
{
    size_t n;
    TAGREF tr;

    if (!SdbTagIDToTagRef(hsdb, db, tagExe, &tr))
        return;

    for (n = 0; n < result->dwExeCount; ++n)
    {
        if (result->atrExes[n] == tr)
            return;
    }

    if (n >= _countof(result->atrExes))
        return;

    result->atrExes[n] = tr;
    result->dwExeCount++;

    SdbpAddExeLayers(hsdb, db, tagExe, result);

    SdbpAddDatabaseGuid(db, result);
}

static ULONG SdbpAddLayerMatches(HSDB hsdb, PWSTR pwszLayers, DWORD pdwBytes, PSDBQUERYRESULT result)
{
    PWSTR start = pwszLayers, p;
    ULONG Added = 0;

    const PWSTR end = pwszLayers + (pdwBytes / sizeof(WCHAR));
    while (start < end && (*start == L'!' || *start == L'#' || *start == L' ' || *start == L'\t'))
        start++;

    if (start == end)
        return 0;

    do
    {
        while (*start == L' ' || *start == L'\t')
            ++start;

        if (*start == UNICODE_NULL)
            break;
        p = wcspbrk(start, L" \t");

        if (p)
            *p = UNICODE_NULL;

        if (SdbpAddNamedLayerMatch(hsdb, start, result))
            Added++;

        start = p + 1;
    } while (start < end && p);

    return Added;
}

static BOOL SdbpPropagateEnvLayers(HSDB hsdb, LPWSTR Environment, PSDBQUERYRESULT Result)
{
    static const UNICODE_STRING EnvKey = RTL_CONSTANT_STRING(L"__COMPAT_LAYER");
    UNICODE_STRING EnvValue;
    NTSTATUS Status;
    WCHAR Buffer[MAX_LAYER_LENGTH];

    RtlInitEmptyUnicodeString(&EnvValue, Buffer, sizeof(Buffer));

    Status = RtlQueryEnvironmentVariable_U(Environment, &EnvKey, &EnvValue);

    if (!NT_SUCCESS(Status))
        return FALSE;

    return SdbpAddLayerMatches(hsdb, Buffer, EnvValue.Length, Result) > 0;
}



/**
 * Opens specified shim database file Handle returned by this function may only be used by
 * functions which take HSDB param thus differing it from SdbOpenDatabase.
 *
 * @param [in]  flags   Specifies type of path or predefined database.
 * @param [in]  path    Path to the shim database file.
 *
 * @return  Success: Handle to the opened shim database, NULL otherwise.
 */
HSDB WINAPI SdbInitDatabase(DWORD flags, LPCWSTR path)
{
    static const WCHAR shim[] = {'\\','s','y','s','m','a','i','n','.','s','d','b',0};
    static const WCHAR msi[] = {'\\','m','s','i','m','a','i','n','.','s','d','b',0};
    static const WCHAR drivers[] = {'\\','d','r','v','m','a','i','n','.','s','d','b',0};
    LPCWSTR name;
    WCHAR buffer[128];
    HSDB hsdb;

    hsdb = SdbAlloc(sizeof(SDB));
    if (!hsdb)
        return NULL;
    hsdb->auto_loaded = 0;

    /* Check for predefined databases */
    if ((flags & HID_DATABASE_TYPE_MASK) && path == NULL)
    {
        switch (flags & HID_DATABASE_TYPE_MASK)
        {
            case SDB_DATABASE_MAIN_SHIM: name = shim; break;
            case SDB_DATABASE_MAIN_MSI: name = msi; break;
            case SDB_DATABASE_MAIN_DRIVERS: name = drivers; break;
            default:
                SdbReleaseDatabase(hsdb);
                return NULL;
        }
        SdbGetAppPatchDir(NULL, buffer, 128);
        memcpy(buffer + lstrlenW(buffer), name, SdbpStrsize(name));
        flags = HID_DOS_PATHS;
    }

    hsdb->db = SdbOpenDatabase(path ? path : buffer, (flags & 0xF) - 1);

    /* If database could not be loaded, a handle doesn't make sense either */
    if (!hsdb->db)
    {
        SdbReleaseDatabase(hsdb);
        return NULL;
    }

    return hsdb;
}

/**
 * Closes shim database opened by SdbInitDatabase.
 *
 * @param [in]  hsdb    Handle to the shim database.
 */
void WINAPI SdbReleaseDatabase(HSDB hsdb)
{
    SdbCloseDatabase(hsdb->db);
    SdbFree(hsdb);
}

/**
 * Queries database for a specified exe If hsdb is NULL default database shall be loaded and
 * searched.
 *
 * @param [in]  hsdb        Handle to the shim database.
 * @param [in]  path        Path to executable for which we query database.
 * @param [in]  module_name Unused.
 * @param [in]  env         The environment block to use
 * @param [in]  flags       0 or SDBGMEF_IGNORE_ENVIRONMENT.
 * @param [out] result      Pointer to structure in which query result shall be stored.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbGetMatchingExe(HSDB hsdb, LPCWSTR path, LPCWSTR module_name,
                              LPCWSTR env, DWORD flags, PSDBQUERYRESULT result)
{
    BOOL ret = FALSE;
    TAGID database, iter, name;
    PATTRINFO attribs = NULL;
    /*DWORD attr_count;*/
    RTL_UNICODE_STRING_BUFFER DosApplicationName = { { 0 } };
    WCHAR DosPathBuffer[MAX_PATH];
    ULONG PathType = 0;
    LPWSTR file_name;
    WCHAR wszLayers[MAX_LAYER_LENGTH];
    DWORD dwSize;
    PDB db;

    /* Load default database if one is not specified */
    if (!hsdb)
    {
        /* To reproduce windows behaviour HID_DOS_PATHS needs
         * to be specified when loading default database */
        hsdb = SdbInitDatabase(HID_DOS_PATHS | SDB_DATABASE_MAIN_SHIM, NULL);
        if (hsdb)
            hsdb->auto_loaded = TRUE;
    }

    ZeroMemory(result, sizeof(*result));

    /* No database could be loaded */
    if (!hsdb || !path)
        return FALSE;

    /* We do not support multiple db's yet! */
    db = hsdb->db;

    RtlInitUnicodeString(&DosApplicationName.String, path);
    RtlInitBuffer(&DosApplicationName.ByteBuffer, (PUCHAR)DosPathBuffer, sizeof(DosPathBuffer));
    if (!NT_SUCCESS(RtlEnsureBufferSize(RTL_SKIP_BUFFER_COPY, &DosApplicationName.ByteBuffer, DosApplicationName.String.MaximumLength)))
    {
        SHIM_ERR("Failed to convert allocate buffer.");
        goto Cleanup;
    }
    /* Update the internal buffer to contain the string */
    memcpy(DosApplicationName.ByteBuffer.Buffer, path, DosApplicationName.String.MaximumLength);
    /* Make sure the string uses our internal buffer (we want to modify the buffer,
        and RtlNtPathNameToDosPathName does not always modify the String to point to the Buffer)! */
    DosApplicationName.String.Buffer = (PWSTR)DosApplicationName.ByteBuffer.Buffer;

    if (!NT_SUCCESS(RtlNtPathNameToDosPathName(0, &DosApplicationName, &PathType, NULL)))
    {
        SHIM_ERR("Failed to convert %S to DOS Path.", path);
        goto Cleanup;
    }


    /* Extract file name */
    file_name = strrchrW(DosApplicationName.String.Buffer, '\\');
    if (!file_name)
    {
        SHIM_ERR("Failed to find Exe name in %wZ.", &DosApplicationName.String);
        goto Cleanup;
    }

    /* We will use the buffer for exe name and directory. */
    *(file_name++) = UNICODE_NULL;

    /* Get information about executable required to match it with database entry */
    /*if (!SdbGetFileAttributes(path, &attribs, &attr_count))
        return FALSE;*/

    /* DATABASE is list TAG which contains all executables */
    database = SdbFindFirstTag(db, TAGID_ROOT, TAG_DATABASE);
    if (database == TAGID_NULL)
    {
        goto Cleanup;
    }

    /* EXE is list TAG which contains data required to match executable */
    iter = SdbFindFirstTag(db, database, TAG_EXE);

    /* Search for entry in database, we should look into indexing tags! */
    while (iter != TAGID_NULL)
    {
        LPWSTR foundName;
        /* Check if exe name matches */
        name = SdbFindFirstTag(db, iter, TAG_NAME);
        /* If this is a malformed DB, (no TAG_NAME), we should not crash. */
        foundName = SdbGetStringTagPtr(db, name);
        if (foundName && !lstrcmpiW(foundName, file_name))
        {
            /* We have a null terminator before the application name, so DosApplicationName only contains the path. */
            if (SdbpMatchExe(db, iter, DosApplicationName.String.Buffer))
            {
                ret = TRUE;
                SdbpAddExeMatch(hsdb, db, iter, result);
            }
        }

        /* Continue iterating */
        iter = SdbFindNextTag(db, database, iter);
    }

    /* Restore the full path. */
    *(--file_name) = L'\\';

    dwSize = sizeof(wszLayers);
    if (SdbGetPermLayerKeys(DosApplicationName.String.Buffer, wszLayers, &dwSize, GPLK_MACHINE | GPLK_USER))
    {
        SdbpAddLayerMatches(hsdb, wszLayers, dwSize, result);
        ret = TRUE;
    }

    if (!(flags & SDBGMEF_IGNORE_ENVIRONMENT))
    {
        if (SdbpPropagateEnvLayers(hsdb, (LPWSTR)env, result))
        {
            ret = TRUE;
            result->dwFlags |= SHIMREG_HAS_ENVIRONMENT;
        }
    }

Cleanup:
    RtlFreeBuffer(&DosApplicationName.ByteBuffer);
    if (attribs)
        SdbFreeFileAttributes(attribs);
    if (hsdb->auto_loaded)
        SdbReleaseDatabase(hsdb);
    return ret;
}

/**
 * Retrieves AppPatch directory.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [out] path    Pointer to memory in which path shall be written.
 * @param [in]  size    Size of the buffer in characters.
 */
BOOL WINAPI SdbGetAppPatchDir(HSDB db, LPWSTR path, DWORD size)
{
    static WCHAR* default_dir = NULL;
    static CONST WCHAR szAppPatch[] = {'\\','A','p','p','P','a','t','c','h',0};

    /* In case function fails, path holds empty string */
    if (size > 0)
        *path = 0;

    if (!default_dir)
    {
        WCHAR* tmp = NULL;
        UINT len = GetSystemWindowsDirectoryW(NULL, 0) + lstrlenW(szAppPatch);
        tmp = SdbAlloc((len + 1)* sizeof(WCHAR));
        if (tmp)
        {
            UINT r = GetSystemWindowsDirectoryW(tmp, len+1);
            if (r && r < len)
            {
                if (SUCCEEDED(StringCchCatW(tmp, len+1, szAppPatch)))
                {
                    if (InterlockedCompareExchangePointer((void**)&default_dir, tmp, NULL) == NULL)
                        tmp = NULL;
                }
            }
            if (tmp)
                SdbFree(tmp);
        }
        if (!default_dir)
        {
            SHIM_ERR("Unable to obtain default AppPatch directory\n");
            return FALSE;
        }
    }

    if (!db)
    {
        return SUCCEEDED(StringCchCopyW(path, size, default_dir));
    }
    else
    {
        SHIM_ERR("Unimplemented for db != NULL\n");
        return FALSE;
    }
}


/**
 * Translates the given trWhich to a specific database / tagid
 *
 * @param [in]      hsdb        Handle to the database.
 * @param [in]      trWhich     Tagref to find
 * @param [out,opt] ppdb        The Shim database that trWhich belongs to.
 * @param [out,opt] ptiWhich    The tagid that trWhich corresponds to.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbTagRefToTagID(HSDB hsdb, TAGREF trWhich, PDB* ppdb, TAGID* ptiWhich)
{
    if (trWhich & 0xf0000000)
    {
        SHIM_ERR("Multiple shim databases not yet implemented!\n");
        if (ppdb)
            *ppdb = NULL;
        if (ptiWhich)
            *ptiWhich = TAG_NULL;
        return FALSE;
    }

    /* There seems to be no range checking on trWhich.. */
    if (ppdb)
        *ppdb = hsdb->db;
    if (ptiWhich)
        *ptiWhich = trWhich & 0x0fffffff;

    return TRUE;
}

/**
 * Translates the given trWhich to a specific database / tagid
 *
 * @param [in]      hsdb        Handle to the database.
 * @param [in]      pdb         The Shim database that tiWhich belongs to.
 * @param [in]      tiWhich     Path to executable for which we query database.
 * @param [out,opt] ptrWhich    The tagid that tiWhich corresponds to.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbTagIDToTagRef(HSDB hsdb, PDB pdb, TAGID tiWhich, TAGREF* ptrWhich)
{
    if (pdb != hsdb->db)
    {
        SHIM_ERR("Multiple shim databases not yet implemented!\n");
        if (ptrWhich)
            *ptrWhich = TAGREF_NULL;
        return FALSE;
    }

    if (ptrWhich)
        *ptrWhich = tiWhich & 0x0fffffff;

    return TRUE;
}



