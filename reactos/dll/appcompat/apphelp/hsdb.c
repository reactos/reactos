/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Mislav Blažević
 * Copyright 2015,2016 Mark Jansen
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


static BOOL WINAPI SdbpFileExists(LPCWSTR path)
{
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
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
    HSDB sdb;

    sdb = SdbAlloc(sizeof(SDB));
    if (!sdb)
        return NULL;
    sdb->auto_loaded = 0;

    /* Check for predefined databases */
    if ((flags & HID_DATABASE_TYPE_MASK) && path == NULL)
    {
        switch (flags & HID_DATABASE_TYPE_MASK)
        {
            case SDB_DATABASE_MAIN_SHIM: name = shim; break;
            case SDB_DATABASE_MAIN_MSI: name = msi; break;
            case SDB_DATABASE_MAIN_DRIVERS: name = drivers; break;
            default:
                SdbReleaseDatabase(sdb);
                return NULL;
        }
        SdbGetAppPatchDir(NULL, buffer, 128);
        memcpy(buffer + lstrlenW(buffer), name, SdbpStrlen(name));
    }

    sdb->db = SdbOpenDatabase(path ? path : buffer, (flags & 0xF) - 1);

    /* If database could not be loaded, a handle doesn't make sense either */
    if (!sdb->db)
    {
        SdbReleaseDatabase(sdb);
        return NULL;
    }

    return sdb;
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
 * @param [in]  env         Unused.
 * @param [in]  flags       0 or SDBGMEF_IGNORE_ENVIRONMENT.
 * @param [out] result      Pointer to structure in which query result shall be stored.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbGetMatchingExe(HSDB hsdb, LPCWSTR path, LPCWSTR module_name,
                              LPCWSTR env, DWORD flags, PSDBQUERYRESULT result)
{
    static const WCHAR fmt[] = {'%','s','%','s',0};
    BOOL ok, ret;
    TAGID database, iter, attr;
    PATTRINFO attribs = NULL;
    /*DWORD attr_count;*/
    LPWSTR file_name;
    WCHAR dir_path[128];
    WCHAR buffer[256];
    PDB db;

    /* Load default database if one is not specified */
    if (!hsdb)
    {
        /* To reproduce windows behaviour HID_DOS_PATHS needs
         * to be specified when loading default database */
        hsdb = SdbInitDatabase(HID_DOS_PATHS | SDB_DATABASE_MAIN_SHIM, NULL);
        if(hsdb)
            hsdb->auto_loaded = TRUE;
    }

    /* No database could be loaded */
    if (!hsdb)
        return FALSE;

    db = hsdb->db;

    /* Extract file name */
    file_name = strrchrW(path, '\\') + 1;

    /* Extract directory path */
    memcpy(dir_path, path, (size_t)(file_name - path) * sizeof(WCHAR));

    /* Get information about executable required to match it with database entry */
    /*if (!SdbGetFileAttributes(path, &attribs, &attr_count))
        return FALSE;*/

    /* DATABASE is list TAG which contains all executables */
    database = SdbFindFirstTag(db, TAGID_ROOT, TAG_DATABASE);
    if (database == TAGID_NULL)
        return FALSE;

    /* EXE is list TAG which contains data required to match executable */
    iter = SdbFindFirstTag(db, database, TAG_EXE);

    /* Search for entry in database */
    while (iter != TAGID_NULL)
    {
        /* Check if exe name matches */
        attr = SdbFindFirstTag(db, iter, TAG_NAME);
        if (lstrcmpiW(SdbGetStringTagPtr(db, attr), file_name) == 0)
        {
            /* Assume that entry is found (in case there are no "matching files") */
            ok = TRUE;

            /* Check if all "matching files" exist */
            /* TODO: check size/checksum as well */
            for (attr = SdbFindFirstTag(db, attr, TAG_MATCHING_FILE);
                 attr != TAGID_NULL; attr = SdbFindNextTag(db, iter, attr))
            {
                snprintfW(buffer, 256, fmt, dir_path, SdbGetStringTagPtr(db, attr));
                if (!SdbpFileExists(buffer))
                    ok = FALSE;
            }

            /* Found it! */
            if (ok)
            {
                /* TODO: fill result data */
                /* TODO: there may be multiple matches */
                ret = TRUE;
                goto cleanup;
            }
        }

        /* Continue iterating */
        iter = SdbFindNextTag(db, database, iter);
    }

    /* Exe not found */
    ret = FALSE;

cleanup:
    SdbFreeFileAttributes(attribs);
    if (hsdb->auto_loaded) SdbReleaseDatabase(hsdb);
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



