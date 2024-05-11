/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shim matching / data (un)packing
 * COPYRIGHT:   Copyright 2011 André Hentschel
 *              Copyright 2013 Mislav Blaževic
 *              Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include "windows.h"
#include "ntndk.h"
#include "strsafe.h"
#include "apphelp.h"
#include "compat_undoc.h"

#define MAX_LAYER_LENGTH            256
#define GPLK_USER                   1
#define GPLK_MACHINE                2

typedef struct _ShimData
{
    WCHAR szModule[MAX_PATH];
    DWORD dwSize;
    DWORD dwMagic;
    SDBQUERYRESULT Query;
    WCHAR szLayer[MAX_LAYER_LENGTH];
    DWORD dwRosProcessCompatVersion;  // ReactOS specific
} ShimData;

#define SHIMDATA_MAGIC  0xAC0DEDAB
#define REACTOS_COMPATVERSION_IGNOREMANIFEST 0xffffffff

C_ASSERT(SHIMDATA_MAGIC == REACTOS_SHIMDATA_MAGIC);
C_ASSERT(sizeof(ShimData) == sizeof(ReactOS_ShimData));
C_ASSERT(offsetof(ShimData, dwMagic) == offsetof(ReactOS_ShimData, dwMagic));
C_ASSERT(offsetof(ShimData, dwRosProcessCompatVersion) == offsetof(ReactOS_ShimData, dwRosProcessCompatVersion));


static BOOL WINAPI SdbpFileExists(LPCWSTR path)
{
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

/* Given a 'MATCHING_FILE' tag and an ATTRINFO array,
   check all tags defined in the MATCHING_FILE against the ATTRINFO */
static BOOL SdbpMatchFileAttributes(PDB pdb, TAGID matching_file, PATTRINFO attribs, DWORD attr_count)
{
    TAGID child;

    for (child = SdbGetFirstChild(pdb, matching_file);
         child != TAGID_NULL; child = SdbGetNextChild(pdb, matching_file, child))
    {
        TAG tag = SdbGetTagFromTagID(pdb, child);
        DWORD n;

        /* Already handled! */
        if (tag == TAG_NAME)
            continue;

        if (tag == TAG_UPTO_BIN_FILE_VERSION ||
            tag == TAG_UPTO_BIN_PRODUCT_VERSION ||
            tag == TAG_UPTO_LINK_DATE)
        {
            SHIM_WARN("Unimplemented TAG_UPTO_XXXXX\n");
            continue;
        }

        for (n = 0; n < attr_count; ++n)
        {
            PATTRINFO attr = attribs + n;
            if (attr->flags == ATTRIBUTE_AVAILABLE && attr->type == tag)
            {
                DWORD dwval;
                WCHAR* lpval;
                QWORD qwval;
                switch (tag & TAG_TYPE_MASK)
                {
                case TAG_TYPE_DWORD:
                    dwval = SdbReadDWORDTag(pdb, child, 0);
                    if (dwval != attr->dwattr)
                        return FALSE;
                    break;
                case TAG_TYPE_STRINGREF:
                    lpval = SdbGetStringTagPtr(pdb, child);
                    if (!lpval || _wcsicmp(attr->lpattr, lpval))
                        return FALSE;
                    break;
                case TAG_TYPE_QWORD:
                    qwval = SdbReadQWORDTag(pdb, child, 0);
                    if (qwval != attr->qwattr)
                        return FALSE;
                    break;
                default:
                    SHIM_WARN("Unhandled type 0x%x MATCHING_FILE\n", (tag & TAG_TYPE_MASK));
                    return FALSE;
                }
            }
        }
        if (n == attr_count)
            SHIM_WARN("Unhandled tag %ws in MATCHING_FILE\n", SdbTagToString(tag));
    }
    return TRUE;
}

/* Given an 'exe' tag and an ATTRINFO array (for the main file),
   verify that the main file and any additional files match */
static BOOL WINAPI SdbpMatchExe(PDB pdb, TAGID exe, const WCHAR* dir, PATTRINFO main_attribs, DWORD main_attr_count)
{
    RTL_UNICODE_STRING_BUFFER FullPathName = { { 0 } };
    WCHAR FullPathBuffer[MAX_PATH];
    UNICODE_STRING UnicodeDir;
    TAGID matching_file;
    PATTRINFO attribs = NULL;
    DWORD attr_count;
    BOOL IsMatch = FALSE;

    RtlInitUnicodeString(&UnicodeDir, dir);
    RtlInitBuffer(&FullPathName.ByteBuffer, (PUCHAR)FullPathBuffer, sizeof(FullPathBuffer));

    for (matching_file = SdbFindFirstTag(pdb, exe, TAG_MATCHING_FILE);
            matching_file != TAGID_NULL; matching_file = SdbFindNextTag(pdb, exe, matching_file))
    {
        TAGID tagName = SdbFindFirstTag(pdb, matching_file, TAG_NAME);
        UNICODE_STRING Name;
        USHORT Len;

        RtlInitUnicodeString(&Name, SdbGetStringTagPtr(pdb, tagName));

        if (!Name.Buffer)
            goto Cleanup;

        /* An '*' here means use the main executable' */
        if (!wcscmp(Name.Buffer, L"*"))
        {
            /* We already have these attributes, so we do not need to retrieve them */
            if (!SdbpMatchFileAttributes(pdb, matching_file, main_attribs, main_attr_count))
                goto Cleanup;
            continue;
        }

        /* Technically, one UNICODE_NULL and one path separator. */
        Len = UnicodeDir.Length + Name.Length + sizeof(UNICODE_NULL) + sizeof(UNICODE_NULL);
        if (!NT_SUCCESS(RtlEnsureBufferSize(RTL_SKIP_BUFFER_COPY, &FullPathName.ByteBuffer, Len)))
            goto Cleanup;

        if (Len > FullPathName.ByteBuffer.Size)
            goto Cleanup;

        RtlInitEmptyUnicodeString(&FullPathName.String, (PWCHAR)FullPathName.ByteBuffer.Buffer, FullPathName.ByteBuffer.Size);

        RtlCopyUnicodeString(&FullPathName.String, &UnicodeDir);
        RtlAppendUnicodeToString(&FullPathName.String, L"\\");
        RtlAppendUnicodeStringToString(&FullPathName.String, &Name);

        /* If the file does not exist, do not bother trying to read it's attributes */
        if (!SdbpFileExists(FullPathName.String.Buffer))
            goto Cleanup;

        /* Do we have some attributes from the previous iteration? */
        if (attribs)
            SdbFreeFileAttributes(attribs);

        if (!SdbGetFileAttributes(FullPathName.String.Buffer, &attribs, &attr_count))
            goto Cleanup;

        if (!SdbpMatchFileAttributes(pdb, matching_file, attribs, attr_count))
            goto Cleanup;
    }

    IsMatch = TRUE;

Cleanup:
    RtlFreeBuffer(&FullPathName.ByteBuffer);
    if (attribs)
        SdbFreeFileAttributes(attribs);

    return IsMatch;
}

/* Add a database guid to the query result */
static void SdbpAddDatabaseGuid(PDB pdb, PSDBQUERYRESULT result)
{
    size_t n;

    for (n = 0; n < _countof(result->rgGuidDB); ++n)
    {
        if (!memcmp(&result->rgGuidDB[n], &pdb->database_id, sizeof(pdb->database_id)))
            return;

        if (result->dwCustomSDBMap & (1<<n))
            continue;

        memcpy(&result->rgGuidDB[n], &pdb->database_id, sizeof(result->rgGuidDB[n]));
        result->dwCustomSDBMap |= (1<<n);
        return;
    }
}

/* Add one layer to the query result */
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

/* Translate a layer name to a tagref + add it to the query result */
static BOOL SdbpAddNamedLayerMatch(HSDB hsdb, PCWSTR layerName, PSDBQUERYRESULT result)
{
    TAGID database, layer;
    TAGREF tr;
    PDB pdb = hsdb->pdb;

    database = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (database == TAGID_NULL)
        return FALSE;

    layer = SdbFindFirstNamedTag(pdb, database, TAG_LAYER, TAG_NAME, layerName);
    if (layer == TAGID_NULL)
        return FALSE;

    if (!SdbTagIDToTagRef(hsdb, pdb, layer, &tr))
        return FALSE;

    if (!SdbpAddSingleLayerMatch(tr, result))
        return FALSE;

    SdbpAddDatabaseGuid(pdb, result);
    return TRUE;
}

/* Add all layers for the exe tag to the query result */
static void SdbpAddExeLayers(HSDB hsdb, PDB pdb, TAGID tagExe, PSDBQUERYRESULT result)
{
    TAGID layer = SdbFindFirstTag(pdb, tagExe, TAG_LAYER);

    while (layer != TAGID_NULL)
    {
        TAGREF tr;
        TAGID layerIdTag = SdbFindFirstTag(pdb, layer, TAG_LAYER_TAGID);
        DWORD tagId = SdbReadDWORDTag(pdb, layerIdTag, TAGID_NULL);

        if (layerIdTag != TAGID_NULL &&
            tagId != TAGID_NULL &&
            SdbTagIDToTagRef(hsdb, pdb, tagId, &tr))
        {
            SdbpAddSingleLayerMatch(tr, result);
        }
        else
        {
            /* Try a name lookup */
            TAGID layerTag = SdbFindFirstTag(pdb, layer, TAG_NAME);
            if (layerTag != TAGID_NULL)
            {
                LPCWSTR layerName = SdbGetStringTagPtr(pdb, layerTag);
                if (layerName)
                {
                    SdbpAddNamedLayerMatch(hsdb, layerName, result);
                }
            }
        }

        layer = SdbFindNextTag(pdb, tagExe, layer);
    }
}

/* Add an exe tag to the query result */
static void SdbpAddExeMatch(HSDB hsdb, PDB pdb, TAGID tagExe, PSDBQUERYRESULT result)
{
    size_t n;
    TAGREF tr;

    if (!SdbTagIDToTagRef(hsdb, pdb, tagExe, &tr))
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

    SdbpAddExeLayers(hsdb, pdb, tagExe, result);

    SdbpAddDatabaseGuid(pdb, result);
}

/* Add all named layers to the query result */
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
 * Opens specified shim database file. Handle returned by this function may only be used by
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
        SdbGetAppPatchDir(NULL, buffer, _countof(buffer));
        StringCchCatW(buffer, _countof(buffer), name);
        flags = HID_DOS_PATHS;
    }

    hsdb->pdb = SdbOpenDatabase(path ? path : buffer, (flags & 0xF) - 1);

    /* If database could not be loaded, a handle doesn't make sense either */
    if (!hsdb->pdb)
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
    if (hsdb)
    {
        SdbCloseDatabase(hsdb->pdb);
        SdbFree(hsdb);
    }
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
    DWORD attr_count;
    RTL_UNICODE_STRING_BUFFER DosApplicationName = { { 0 } };
    WCHAR DosPathBuffer[MAX_PATH];
    ULONG PathType = 0;
    LPWSTR file_name;
    WCHAR wszLayers[MAX_LAYER_LENGTH];
    DWORD dwSize;
    PDB pdb;

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
    pdb = hsdb->pdb;

    RtlInitUnicodeString(&DosApplicationName.String, path);
    RtlInitBuffer(&DosApplicationName.ByteBuffer, (PUCHAR)DosPathBuffer, sizeof(DosPathBuffer));
    if (!NT_SUCCESS(RtlEnsureBufferSize(RTL_SKIP_BUFFER_COPY, &DosApplicationName.ByteBuffer, DosApplicationName.String.MaximumLength)))
    {
        SHIM_ERR("Failed to convert allocate buffer.\n");
        goto Cleanup;
    }
    /* Update the internal buffer to contain the string */
    memcpy(DosApplicationName.ByteBuffer.Buffer, path, DosApplicationName.String.MaximumLength);
    /* Make sure the string uses our internal buffer (we want to modify the buffer,
        and RtlNtPathNameToDosPathName does not always modify the String to point to the Buffer)! */
    DosApplicationName.String.Buffer = (PWSTR)DosApplicationName.ByteBuffer.Buffer;

    if (!NT_SUCCESS(RtlNtPathNameToDosPathName(0, &DosApplicationName, &PathType, NULL)))
    {
        SHIM_ERR("Failed to convert %S to DOS Path.\n", path);
        goto Cleanup;
    }


    /* Extract file name */
    file_name = wcsrchr(DosApplicationName.String.Buffer, '\\');
    if (!file_name)
    {
        SHIM_ERR("Failed to find Exe name in %wZ.\n", &DosApplicationName.String);
        goto Cleanup;
    }

    /* We will use the buffer for exe name and directory. */
    *(file_name++) = UNICODE_NULL;

    /* DATABASE is list TAG which contains all executables */
    database = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (database == TAGID_NULL)
    {
        goto Cleanup;
    }

    /* EXE is list TAG which contains data required to match executable */
    iter = SdbFindFirstTag(pdb, database, TAG_EXE);

    /* Search for entry in database, we should look into indexing tags! */
    while (iter != TAGID_NULL)
    {
        LPWSTR foundName;
        /* Check if exe name matches */
        name = SdbFindFirstTag(pdb, iter, TAG_NAME);
        /* If this is a malformed DB, (no TAG_NAME), we should not crash. */
        foundName = SdbGetStringTagPtr(pdb, name);
        if (foundName && !_wcsicmp(foundName, file_name))
        {
            /* Get information about executable required to match it with database entry */
            if (!attribs)
            {
                if (!SdbGetFileAttributes(path, &attribs, &attr_count))
                    goto Cleanup;
            }


            /* We have a null terminator before the application name, so DosApplicationName only contains the path. */
            if (SdbpMatchExe(pdb, iter, DosApplicationName.String.Buffer, attribs, attr_count))
            {
                ret = TRUE;
                SdbpAddExeMatch(hsdb, pdb, iter, result);
            }
        }

        /* Continue iterating */
        iter = SdbFindNextTag(pdb, database, iter);
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
 * @param [in]  pdb     Handle to the shim database.
 * @param [out] path    Pointer to memory in which path shall be written.
 * @param [in]  size    Size of the buffer in characters.
 */
HRESULT WINAPI SdbGetAppPatchDir(HSDB hsdb, LPWSTR path, DWORD size)
{
    static WCHAR* default_dir = NULL;
    static CONST WCHAR szAppPatch[] = {'\\','A','p','p','P','a','t','c','h',0};

    /* In case function fails, path holds empty string */
    if (size > 0)
        *path = 0;

    if (!default_dir)
    {
        WCHAR* tmp;
        HRESULT hr = E_FAIL;
        UINT len = GetSystemWindowsDirectoryW(NULL, 0) + SdbpStrlen(szAppPatch);
        tmp = SdbAlloc((len + 1)* sizeof(WCHAR));
        if (tmp)
        {
            UINT r = GetSystemWindowsDirectoryW(tmp, len+1);
            if (r && r < len)
            {
                hr = StringCchCatW(tmp, len+1, szAppPatch);
                if (SUCCEEDED(hr))
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
            SHIM_ERR("Unable to obtain default AppPatch directory (0x%x)\n", hr);
            return hr;
        }
    }

    if (!hsdb)
    {
        return StringCchCopyW(path, size, default_dir);
    }
    else
    {
        SHIM_ERR("Unimplemented for hsdb != NULL\n");
        return E_NOTIMPL;
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
        *ppdb = hsdb->pdb;
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
    if (pdb != hsdb->pdb)
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


/* Convert a query result to shim data that will be loaded in the child process */
BOOL WINAPI SdbPackAppCompatData(HSDB hsdb, PSDBQUERYRESULT pQueryResult, PVOID* ppData, DWORD *pdwSize)
{
    ShimData* pData;
    HRESULT hr;
    DWORD n;
    BOOL bCloseDatabase = FALSE;

    if (!pQueryResult || !ppData || !pdwSize)
    {
        SHIM_WARN("Invalid params: %p, %p, %p\n", pQueryResult, ppData, pdwSize);
        return FALSE;
    }

    pData = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ShimData));
    if (!pData)
    {
        SHIM_WARN("Unable to allocate %d bytes\n", sizeof(ShimData));
        return FALSE;
    }

    GetSystemWindowsDirectoryW(pData->szModule, _countof(pData->szModule));
    hr = StringCchCatW(pData->szModule, _countof(pData->szModule), L"\\system32\\apphelp.dll");
    if (!SUCCEEDED(hr))
    {
        SHIM_ERR("Unable to append module name (0x%x)\n", hr);
        RtlFreeHeap(RtlGetProcessHeap(), 0, pData);
        return FALSE;
    }

    pData->dwSize = sizeof(*pData);
    pData->dwMagic = SHIMDATA_MAGIC;
    pData->Query = *pQueryResult;
    pData->dwRosProcessCompatVersion = 0;
    pData->szLayer[0] = UNICODE_NULL;   /* TODO */

    SHIM_INFO("\ndwFlags    0x%x\ndwMagic    0x%x\ntrExe      0x%x\ntrLayer    0x%x\n",
              pData->Query.dwFlags, pData->dwMagic, pData->Query.atrExes[0], pData->Query.atrLayers[0]);

    /* Database List */
    /* 0x0 {GUID} NAME: Use to open HSDB */
    if (hsdb == NULL)
    {
        hsdb = SdbInitDatabase(HID_DOS_PATHS | SDB_DATABASE_MAIN_SHIM, NULL);
        bCloseDatabase = TRUE;
    }

    for (n = 0; n < pQueryResult->dwLayerCount; ++n)
    {
        DWORD dwValue = 0, dwType;
        DWORD dwValueSize = sizeof(dwValue);
        SHIM_INFO("Layer 0x%x\n", pQueryResult->atrLayers[n]);

        if (SdbQueryData(hsdb, pQueryResult->atrLayers[n], L"SHIMVERSIONNT", &dwType, &dwValue, &dwValueSize) == ERROR_SUCCESS &&
            dwType == REG_DWORD && dwValueSize == sizeof(dwValue))
        {
            if (dwValue != REACTOS_COMPATVERSION_IGNOREMANIFEST)
                dwValue = (dwValue % 100) | ((dwValue / 100) << 8);
            if (dwValue > pData->dwRosProcessCompatVersion)
                pData->dwRosProcessCompatVersion = dwValue;
        }
    }

    if (pData->dwRosProcessCompatVersion)
        SHIM_INFO("Setting ProcessCompatVersion 0x%x\n", pData->dwRosProcessCompatVersion);

    if (bCloseDatabase)
        SdbReleaseDatabase(hsdb);

    *ppData = pData;
    *pdwSize = pData->dwSize;

    return TRUE;
}

BOOL WINAPI SdbUnpackAppCompatData(HSDB hsdb, LPCWSTR pszImageName, PVOID pData, PSDBQUERYRESULT pQueryResult)
{
    ShimData* pShimData = pData;

    if (!pShimData || pShimData->dwMagic != SHIMDATA_MAGIC || pShimData->dwSize < sizeof(ShimData))
        return FALSE;

    if (!pQueryResult)
        return FALSE;

    /* szLayer? */

    *pQueryResult = pShimData->Query;
    return TRUE;
}

DWORD WINAPI SdbGetAppCompatDataSize(ShimData* pData)
{
    if (!pData || pData->dwMagic != SHIMDATA_MAGIC)
        return 0;

    return pData->dwSize;
}


/**
* Retrieve a Data entry
*
* @param [in]  hsdb                    The multi-database.
* @param [in]  trExe                   The tagRef to start at
* @param [in,opt]  lpszDataName        The name of the Data entry to find, or NULL to return all.
* @param [out,opt]  lpdwDataType       Any of REG_SZ, REG_QWORD, REG_DWORD, ...
* @param [out]  lpBuffer               The output buffer
* @param [in,out,opt]  lpcbBufferSize  The size of lpBuffer in bytes
* @param [out,opt]  ptrData            The tagRef of the data
*
* @return  ERROR_SUCCESS
*/
DWORD WINAPI SdbQueryDataEx(HSDB hsdb, TAGREF trWhich, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize, TAGREF *ptrData)
{
    PDB pdb;
    TAGID tiWhich, tiData;
    DWORD dwResult;

    if (!SdbTagRefToTagID(hsdb, trWhich, &pdb, &tiWhich))
    {
        SHIM_WARN("Unable to translate trWhich=0x%x\n", trWhich);
        return ERROR_NOT_FOUND;
    }

    dwResult = SdbQueryDataExTagID(pdb, tiWhich, lpszDataName, lpdwDataType, lpBuffer, lpcbBufferSize, &tiData);

    if (dwResult == ERROR_SUCCESS && ptrData)
        SdbTagIDToTagRef(hsdb, pdb, tiData, ptrData);

    return dwResult;
}


/**
* Retrieve a Data entry
*
* @param [in]  hsdb                    The multi-database.
* @param [in]  trExe                   The tagRef to start at
* @param [in,opt]  lpszDataName        The name of the Data entry to find, or NULL to return all.
* @param [out,opt]  lpdwDataType       Any of REG_SZ, REG_QWORD, REG_DWORD, ...
* @param [out]  lpBuffer               The output buffer
* @param [in,out,opt]  lpcbBufferSize  The size of lpBuffer in bytes
*
* @return  ERROR_SUCCESS
*/
DWORD WINAPI SdbQueryData(HSDB hsdb, TAGREF trWhich, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
    return SdbQueryDataEx(hsdb, trWhich, lpszDataName, lpdwDataType, lpBuffer, lpcbBufferSize, NULL);
}
