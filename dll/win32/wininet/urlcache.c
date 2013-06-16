/*
 * Wininet - Url Cache functions
 *
 * Copyright 2001,2002 CodeWeavers
 * Copyright 2003-2008 Robert Shearman
 *
 * Eric Kohl
 * Aric Stewart
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
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>
//#include "wine/port.h"

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdarg.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
//#include <time.h>

#include <windef.h>
#include <winbase.h>
//#include "winuser.h"
#include <wininet.h>
#include <winineti.h>
//#include "winerror.h"
#include <winreg.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#endif

#include "internet.h"

//#include "wine/unicode.h"
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

static const char urlcache_ver_prefix[] = "WINE URLCache Ver ";
static const char urlcache_ver[] = "0.2012001";

#define ENTRY_START_OFFSET      0x4000
#define DIR_LENGTH              8
#define MAX_DIR_NO              0x20
#define BLOCKSIZE               128
#define HASHTABLE_SIZE          448
#define HASHTABLE_NUM_ENTRIES   64 /* this needs to be power of 2, that divides HASHTABLE_SIZE */
#define HASHTABLE_BLOCKSIZE     (HASHTABLE_SIZE / HASHTABLE_NUM_ENTRIES)
#define ALLOCATION_TABLE_OFFSET 0x250
#define ALLOCATION_TABLE_SIZE   (ENTRY_START_OFFSET - ALLOCATION_TABLE_OFFSET)
#define MIN_BLOCK_NO            0x80
#define MAX_BLOCK_NO            (ALLOCATION_TABLE_SIZE * 8)
#define FILE_SIZE(blocks)       ((blocks) * BLOCKSIZE + ENTRY_START_OFFSET)

#define HASHTABLE_URL           0
#define HASHTABLE_DEL           1
#define HASHTABLE_LOCK          2
#define HASHTABLE_FREE          3
#define HASHTABLE_REDR          5
#define HASHTABLE_FLAG_BITS     6

#define PENDING_DELETE_CACHE_ENTRY  0x00400000
#define INSTALLED_CACHE_ENTRY       0x10000000
#define GET_INSTALLED_ENTRY         0x200
#define CACHE_CONTAINER_NO_SUBDIR   0xFE

#define CACHE_HEADER_DATA_ROOT_LEAK_OFFSET 0x16

#define FILETIME_SECOND 10000000

#define DWORD_SIG(a,b,c,d)  (a | (b << 8) | (c << 16) | (d << 24))
#define URL_SIGNATURE   DWORD_SIG('U','R','L',' ')
#define REDR_SIGNATURE  DWORD_SIG('R','E','D','R')
#define LEAK_SIGNATURE  DWORD_SIG('L','E','A','K')
#define HASH_SIGNATURE  DWORD_SIG('H','A','S','H')

#define DWORD_ALIGN(x) ( (DWORD)(((DWORD)(x)+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD) )

typedef struct
{
    DWORD signature;
    DWORD blocks_used; /* number of 128byte blocks used by this entry */
} entry_header;

typedef struct
{
    entry_header header;
    FILETIME modification_time;
    FILETIME access_time;
    WORD expire_date; /* expire date in dos format */
    WORD expire_time; /* expire time in dos format */
    DWORD unk1; /* usually zero */
    ULARGE_INTEGER size; /* see INTERNET_CACHE_ENTRY_INFO::dwSizeLow/High */
    DWORD unk2; /* usually zero */
    DWORD exempt_delta; /* see INTERNET_CACHE_ENTRY_INFO::dwExemptDelta */
    DWORD unk3; /* usually 0x60 */
    DWORD url_off; /* offset of start of url from start of entry */
    BYTE cache_dir; /* index of cache directory this url is stored in */
    BYTE unk4; /* usually zero */
    WORD unk5; /* usually 0x1010 */
    DWORD local_name_off; /* offset of start of local filename from start of entry */
    DWORD cache_entry_type; /* see INTERNET_CACHE_ENTRY_INFO::CacheEntryType */
    DWORD header_info_off; /* offset of start of header info from start of entry */
    DWORD header_info_size;
    DWORD file_extension_off; /* offset of start of file extension from start of entry */
    WORD sync_date; /* last sync date in dos format */
    WORD sync_time; /* last sync time in dos format */
    DWORD hit_rate; /* see INTERNET_CACHE_ENTRY_INFO::dwHitRate */
    DWORD use_count; /* see INTERNET_CACHE_ENTRY_INFO::dwUseCount */
    WORD write_date;
    WORD write_time;
    DWORD unk7; /* usually zero */
    DWORD unk8; /* usually zero */
    /* packing to dword align start of next field */
    /* CHAR szSourceUrlName[]; (url) */
    /* packing to dword align start of next field */
    /* CHAR szLocalFileName[]; (local file name excluding path) */
    /* packing to dword align start of next field */
    /* CHAR szHeaderInfo[]; (header info) */
} entry_url;

struct _HASH_ENTRY
{
    DWORD dwHashKey;
    DWORD dwOffsetEntry;
};

typedef struct _HASH_CACHEFILE_ENTRY
{
    entry_header CacheFileEntry;
    DWORD dwAddressNext;
    DWORD dwHashTableNumber;
    struct _HASH_ENTRY HashTable[HASHTABLE_SIZE];
} HASH_CACHEFILE_ENTRY;

typedef struct _DIRECTORY_DATA
{
    DWORD dwNumFiles;
    char filename[DIR_LENGTH];
} DIRECTORY_DATA;

typedef struct _URLCACHE_HEADER
{
    char szSignature[28];
    DWORD dwFileSize;
    DWORD dwOffsetFirstHashTable;
    DWORD dwIndexCapacityInBlocks;
    DWORD dwBlocksInUse;
    DWORD dwUnknown1;
    ULARGE_INTEGER CacheLimit;
    ULARGE_INTEGER CacheUsage;
    ULARGE_INTEGER ExemptUsage;
    DWORD DirectoryCount;
    DIRECTORY_DATA directory_data[MAX_DIR_NO];
    DWORD options[0x21];
    BYTE allocation_table[ALLOCATION_TABLE_SIZE];
} URLCACHE_HEADER, *LPURLCACHE_HEADER;
typedef const URLCACHE_HEADER *LPCURLCACHE_HEADER;

typedef struct _STREAM_HANDLE
{
    HANDLE hFile;
    CHAR lpszUrl[1];
} STREAM_HANDLE;

typedef struct _URLCACHECONTAINER
{
    struct list entry; /* part of a list */
    LPWSTR cache_prefix; /* string that has to be prefixed for this container to be used */
    LPWSTR path; /* path to url container directory */
    HANDLE hMapping; /* handle of file mapping */
    DWORD file_size; /* size of file when mapping was opened */
    HANDLE hMutex; /* handle of mutex */
    DWORD default_entry_type;
} URLCACHECONTAINER;


/* List of all containers available */
static struct list UrlContainers = LIST_INIT(UrlContainers);
// ReactOS r54992
BOOL bDefaultContainersAdded = FALSE;

static DWORD URLCache_CreateHashTable(LPURLCACHE_HEADER pHeader, HASH_CACHEFILE_ENTRY *pPrevHash, HASH_CACHEFILE_ENTRY **ppHash);

/***********************************************************************
 *           URLCache_PathToObjectName (Internal)
 *
 *  Converts a path to a name suitable for use as a Win32 object name.
 * Replaces '\\' characters in-place with the specified character
 * (usually '_' or '!')
 *
 * RETURNS
 *    nothing
 *
 */
static void URLCache_PathToObjectName(LPWSTR lpszPath, WCHAR replace)
{
    for (; *lpszPath; lpszPath++)
    {
        if (*lpszPath == '\\')
            *lpszPath = replace;
    }
}

/* Caller must hold container lock */
static HANDLE cache_container_map_index(HANDLE file, const WCHAR *path, DWORD size, BOOL *validate)
{
    static const WCHAR mapping_name_format[]
        = {'%','s','i','n','d','e','x','.','d','a','t','_','%','l','u',0};
    WCHAR mapping_name[MAX_PATH];
    HANDLE mapping;

    wsprintfW(mapping_name, mapping_name_format, path, size);
    URLCache_PathToObjectName(mapping_name, '_');

    mapping = OpenFileMappingW(FILE_MAP_WRITE, FALSE, mapping_name);
    if(mapping) {
        if(validate) *validate = FALSE;
        return mapping;
    }

    if(validate) *validate = TRUE;
    return CreateFileMappingW(file, NULL, PAGE_READWRITE, 0, 0, mapping_name);
}

/* Caller must hold container lock */
static DWORD cache_container_set_size(URLCACHECONTAINER *container, HANDLE file, DWORD blocks_no)
{
    static const WCHAR cache_content_key[] = {'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s','\\',
        'C','a','c','h','e','\\','C','o','n','t','e','n','t',0};
    static const WCHAR cache_limit[] = {'C','a','c','h','e','L','i','m','i','t',0};

    DWORD file_size = FILE_SIZE(blocks_no);
    WCHAR dir_path[MAX_PATH], *dir_name;
    HASH_CACHEFILE_ENTRY *hash_entry;
    URLCACHE_HEADER *header;
    HANDLE mapping;
    FILETIME ft;
    HKEY key;
    int i, j;

    if(SetFilePointer(file, file_size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        return GetLastError();

    if(!SetEndOfFile(file))
        return GetLastError();

    mapping = cache_container_map_index(file, container->path, file_size, NULL);
    if(!mapping)
        return GetLastError();

    header = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    if(!header) {
        CloseHandle(mapping);
        return GetLastError();
    }

    if(blocks_no != MIN_BLOCK_NO) {
        if(file_size > header->dwFileSize)
            memset((char*)header+header->dwFileSize, 0, file_size-header->dwFileSize);
        header->dwFileSize = file_size;
        header->dwIndexCapacityInBlocks = blocks_no;

        UnmapViewOfFile(header);
        CloseHandle(container->hMapping);
        container->hMapping = mapping;
        container->file_size = file_size;
        return ERROR_SUCCESS;
    }

    memset(header, 0, file_size);
    /* First set some constants and defaults in the header */
    memcpy(header->szSignature, urlcache_ver_prefix, sizeof(urlcache_ver_prefix)-1);
    memcpy(header->szSignature+sizeof(urlcache_ver_prefix)-1, urlcache_ver, sizeof(urlcache_ver)-1);
    header->dwFileSize = file_size;
    header->dwIndexCapacityInBlocks = blocks_no;
    /* 127MB - taken from default for Windows 2000 */
    header->CacheLimit.QuadPart = 0x07ff5400;
    /* Copied from a Windows 2000 cache index */
    header->DirectoryCount = container->default_entry_type==NORMAL_CACHE_ENTRY ? 4 : 0;

    /* If the registry has a cache size set, use the registry value */
    if(RegOpenKeyW(HKEY_CURRENT_USER, cache_content_key, &key) == ERROR_SUCCESS) {
        DWORD dw, len = sizeof(dw), keytype;

        if(RegQueryValueExW(key, cache_limit, NULL, &keytype, (BYTE*)&dw, &len) == ERROR_SUCCESS &&
                keytype == REG_DWORD)
            header->CacheLimit.QuadPart = (ULONGLONG)dw * 1024;
        RegCloseKey(key);
    }

    URLCache_CreateHashTable(header, NULL, &hash_entry);

    /* Last step - create the directories */
    strcpyW(dir_path, container->path);
    dir_name = dir_path + strlenW(dir_path);
    dir_name[8] = 0;

    GetSystemTimeAsFileTime(&ft);

    for(i=0; i<header->DirectoryCount; ++i) {
        header->directory_data[i].dwNumFiles = 0;
        for(j=0;; ++j) {
            ULONGLONG n = ft.dwHighDateTime;
            int k;

            /* Generate a file name to attempt to create.
             * This algorithm will create what will appear
             * to be random and unrelated directory names
             * of up to 9 characters in length.
             */
            n <<= 32;
            n += ft.dwLowDateTime;
            n ^= ((ULONGLONG) i << 56) | ((ULONGLONG) j << 48);

            for(k = 0; k < 8; ++k) {
                int r = (n % 36);

                /* Dividing by a prime greater than 36 helps
                 * with the appearance of randomness
                 */
                n /= 37;

                if(r < 10)
                    dir_name[k] = '0' + r;
                else
                    dir_name[k] = 'A' + (r - 10);
            }

            if(CreateDirectoryW(dir_path, 0)) {
                /* The following is OK because we generated an
                 * 8 character directory name made from characters
                 * [A-Z0-9], which are equivalent for all code
                 * pages and for UTF-16
                 */
                for (k = 0; k < 8; ++k)
                    header->directory_data[i].filename[k] = dir_name[k];
                break;
            }else if(j >= 255) {
                /* Give up. The most likely cause of this
                 * is a full disk, but whatever the cause
                 * is, it should be more than apparent that
                 * we won't succeed.
                 */
                UnmapViewOfFile(header);
                CloseHandle(mapping);
                return GetLastError();
            }
        }
    }

    UnmapViewOfFile(header);
    CloseHandle(container->hMapping);
    container->hMapping = mapping;
    container->file_size = file_size;
    return ERROR_SUCCESS;
}

static BOOL cache_container_is_valid(URLCACHE_HEADER *header, DWORD file_size)
{
    DWORD allocation_size, count_bits, i;

    if(file_size < FILE_SIZE(MIN_BLOCK_NO))
        return FALSE;

    if(file_size != header->dwFileSize)
        return FALSE;

    if (!memcmp(header->szSignature, urlcache_ver_prefix, sizeof(urlcache_ver_prefix)-1) &&
            memcmp(header->szSignature+sizeof(urlcache_ver_prefix)-1, urlcache_ver, sizeof(urlcache_ver)-1))
        return FALSE;

    if(FILE_SIZE(header->dwIndexCapacityInBlocks) != file_size)
        return FALSE;

    allocation_size = 0;
    for(i=0; i<header->dwIndexCapacityInBlocks/8; i++) {
        for(count_bits = header->allocation_table[i]; count_bits!=0; count_bits>>=1) {
            if(count_bits & 1)
                allocation_size++;
        }
    }
    if(allocation_size != header->dwBlocksInUse)
        return FALSE;

    for(; i<ALLOCATION_TABLE_SIZE; i++) {
        if(header->allocation_table[i])
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           cache_container_open_index (Internal)
 *
 *  Opens the index file and saves mapping handle in hMapping
 *
 * RETURNS
 *    ERROR_SUCCESS if succeeded
 *    Any other Win32 error code if failed
 *
 */
static DWORD cache_container_open_index(URLCACHECONTAINER *container, DWORD blocks_no)
{
    static const WCHAR index_dat[] = {'i','n','d','e','x','.','d','a','t',0};

    HANDLE file;
    WCHAR index_path[MAX_PATH];
    DWORD file_size;
    BOOL validate;

    WaitForSingleObject(container->hMutex, INFINITE);

    if(container->hMapping) {
        ReleaseMutex(container->hMutex);
        return ERROR_SUCCESS;
    }

    strcpyW(index_path, container->path);
    strcatW(index_path, index_dat);

    file = CreateFileW(index_path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);
    if(file == INVALID_HANDLE_VALUE) {
	/* Maybe the directory wasn't there? Try to create it */
	if(CreateDirectoryW(container->path, 0))
            file = CreateFileW(index_path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);
    }
    if(file == INVALID_HANDLE_VALUE) {
        TRACE("Could not open or create cache index file \"%s\"\n", debugstr_w(index_path));
        ReleaseMutex(container->hMutex);
        return GetLastError();
    }

    file_size = GetFileSize(file, NULL);
    if(file_size == INVALID_FILE_SIZE) {
        CloseHandle(file);
	ReleaseMutex(container->hMutex);
        return GetLastError();
    }

    if(blocks_no < MIN_BLOCK_NO)
        blocks_no = MIN_BLOCK_NO;
    else if(blocks_no > MAX_BLOCK_NO)
        blocks_no = MAX_BLOCK_NO;

    if(file_size < FILE_SIZE(blocks_no)) {
        DWORD ret = cache_container_set_size(container, file, blocks_no);
        CloseHandle(file);
        ReleaseMutex(container->hMutex);
        return ret;
    }

    container->file_size = file_size;
    container->hMapping = cache_container_map_index(file, container->path, file_size, &validate);
    CloseHandle(file);
    if(container->hMapping && validate) {
        URLCACHE_HEADER *header = MapViewOfFile(container->hMapping, FILE_MAP_WRITE, 0, 0, 0);

        if(header && !cache_container_is_valid(header, file_size)) {
            WARN("detected old or broken index.dat file\n");
            UnmapViewOfFile(header);
            FreeUrlCacheSpaceW(container->path, 100, 0);
        }else if(header) {
            UnmapViewOfFile(header);
        }else {
            CloseHandle(container->hMapping);
            container->hMapping = NULL;
        }
    }

    if(!container->hMapping)
    {
        ERR("Couldn't create file mapping (error is %d)\n", GetLastError());
        ReleaseMutex(container->hMutex);
        return GetLastError();
    }

    ReleaseMutex(container->hMutex);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           cache_container_close_index (Internal)
 *
 *  Closes the index
 *
 * RETURNS
 *    nothing
 *
 */
static void cache_container_close_index(URLCACHECONTAINER * pContainer)
{
    CloseHandle(pContainer->hMapping);
    pContainer->hMapping = NULL;
}

static BOOL URLCacheContainers_AddContainer(LPCWSTR cache_prefix,
        LPCWSTR path, DWORD default_entry_type, LPWSTR mutex_name)
{
    URLCACHECONTAINER * pContainer = heap_alloc(sizeof(URLCACHECONTAINER));
    int cache_prefix_len = strlenW(cache_prefix);

    if (!pContainer)
    {
        return FALSE;
    }

    pContainer->hMapping = NULL;
    pContainer->file_size = 0;
    pContainer->default_entry_type = default_entry_type;

    pContainer->path = heap_strdupW(path);
    if (!pContainer->path)
    {
        heap_free(pContainer);
        return FALSE;
    }

    pContainer->cache_prefix = heap_alloc((cache_prefix_len + 1) * sizeof(WCHAR));
    if (!pContainer->cache_prefix)
    {
        heap_free(pContainer->path);
        heap_free(pContainer);
        return FALSE;
    }

    memcpy(pContainer->cache_prefix, cache_prefix, (cache_prefix_len + 1) * sizeof(WCHAR));

    CharLowerW(mutex_name);
    URLCache_PathToObjectName(mutex_name, '!');

    if ((pContainer->hMutex = CreateMutexW(NULL, FALSE, mutex_name)) == NULL)
    {
        ERR("couldn't create mutex (error is %d)\n", GetLastError());
        heap_free(pContainer->path);
        heap_free(pContainer);
        return FALSE;
    }

    list_add_head(&UrlContainers, &pContainer->entry);

    return TRUE;
}

static void cache_container_delete_container(URLCACHECONTAINER * pContainer)
{
    list_remove(&pContainer->entry);

    cache_container_close_index(pContainer);
    CloseHandle(pContainer->hMutex);
    heap_free(pContainer->path);
    heap_free(pContainer->cache_prefix);
    heap_free(pContainer);
}

static void URLCacheContainers_CreateDefaults(void)
{
    static const WCHAR UrlSuffix[] = {'C','o','n','t','e','n','t','.','I','E','5',0};
    static const WCHAR UrlPrefix[] = {0};
    static const WCHAR HistorySuffix[] = {'H','i','s','t','o','r','y','.','I','E','5',0};
    static const WCHAR HistoryPrefix[] = {'V','i','s','i','t','e','d',':',0};
    static const WCHAR CookieSuffix[] = {0};
    static const WCHAR CookiePrefix[] = {'C','o','o','k','i','e',':',0};
    // ReactOS r50916
    static const WCHAR UserProfile[] = {'U','S','E','R','P','R','O','F','I','L','E',0};
    static const struct
    {
        int nFolder; /* CSIDL_* constant */
        const WCHAR * shpath_suffix; /* suffix on path returned by SHGetSpecialFolderPath */
        const WCHAR * cache_prefix; /* prefix used to reference the container */
        DWORD default_entry_type;
    } DefaultContainerData[] = 
    {
        { CSIDL_INTERNET_CACHE, UrlSuffix, UrlPrefix, NORMAL_CACHE_ENTRY },
        { CSIDL_HISTORY, HistorySuffix, HistoryPrefix, URLHISTORY_CACHE_ENTRY },
        { CSIDL_COOKIES, CookieSuffix, CookiePrefix, COOKIE_CACHE_ENTRY },
    };
    DWORD i;

    // ReactOS r50916
    if (GetEnvironmentVariableW(UserProfile, NULL, 0) == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
    {
        TRACE("Environment variable 'USERPROFILE' does not exist!\n");
        return;
    }

    for (i = 0; i < sizeof(DefaultContainerData) / sizeof(DefaultContainerData[0]); i++)
    {
        WCHAR wszCachePath[MAX_PATH];
        WCHAR wszMutexName[MAX_PATH];
        int path_len, suffix_len;

        if (!SHGetSpecialFolderPathW(NULL, wszCachePath, DefaultContainerData[i].nFolder, TRUE))
        {
            ERR("Couldn't get path for default container %u\n", i);
            continue;
        }
        path_len = strlenW(wszCachePath);
        suffix_len = strlenW(DefaultContainerData[i].shpath_suffix);

        if (path_len + suffix_len + 2 > MAX_PATH)
        {
            ERR("Path too long\n");
            continue;
        }

        wszCachePath[path_len] = '\\';
        wszCachePath[path_len+1] = 0;

        strcpyW(wszMutexName, wszCachePath);
        
        if (suffix_len)
        {
            memcpy(wszCachePath + path_len + 1, DefaultContainerData[i].shpath_suffix, (suffix_len + 1) * sizeof(WCHAR));
            wszCachePath[path_len + suffix_len + 1] = '\\';
            wszCachePath[path_len + suffix_len + 2] = '\0';
        }

        URLCacheContainers_AddContainer(DefaultContainerData[i].cache_prefix, wszCachePath,
                DefaultContainerData[i].default_entry_type, wszMutexName);
    }
}

static void URLCacheContainers_DeleteAll(void)
{
    while(!list_empty(&UrlContainers))
        cache_container_delete_container(
            LIST_ENTRY(list_head(&UrlContainers), URLCACHECONTAINER, entry)
        );
}

static DWORD URLCacheContainers_FindContainerW(LPCWSTR lpwszUrl, URLCACHECONTAINER ** ppContainer)
{
    URLCACHECONTAINER * pContainer;

    TRACE("searching for prefix for URL: %s\n", debugstr_w(lpwszUrl));

    if(!lpwszUrl)
        return ERROR_INVALID_PARAMETER;

    // ReactOS r54992
    if (!bDefaultContainersAdded)
        URLCacheContainers_CreateDefaults();

    LIST_FOR_EACH_ENTRY(pContainer, &UrlContainers, URLCACHECONTAINER, entry)
    {
        int prefix_len = strlenW(pContainer->cache_prefix);
        if (!strncmpW(pContainer->cache_prefix, lpwszUrl, prefix_len))
        {
            TRACE("found container with prefix %s for URL %s\n", debugstr_w(pContainer->cache_prefix), debugstr_w(lpwszUrl));
            *ppContainer = pContainer;
            return ERROR_SUCCESS;
        }
    }
    ERR("no container found\n");
    return ERROR_FILE_NOT_FOUND;
}

static DWORD URLCacheContainers_FindContainerA(LPCSTR lpszUrl, URLCACHECONTAINER ** ppContainer)
{
    LPWSTR url = NULL;
    DWORD ret;

    if (lpszUrl && !(url = heap_strdupAtoW(lpszUrl)))
        return ERROR_OUTOFMEMORY;

    ret = URLCacheContainers_FindContainerW(url, ppContainer);
    heap_free(url);
    return ret;
}

static BOOL URLCacheContainers_Enum(LPCWSTR lpwszSearchPattern, DWORD dwIndex, URLCACHECONTAINER ** ppContainer)
{
    DWORD i = 0;
    URLCACHECONTAINER * pContainer;

    TRACE("searching for prefix: %s\n", debugstr_w(lpwszSearchPattern));

    /* non-NULL search pattern only returns one container ever */
    if (lpwszSearchPattern && dwIndex > 0)
        return FALSE;

    // ReactOS r54992
    if (!bDefaultContainersAdded)
        URLCacheContainers_CreateDefaults();

    LIST_FOR_EACH_ENTRY(pContainer, &UrlContainers, URLCACHECONTAINER, entry)
    {
        if (lpwszSearchPattern)
        {
            if (!strcmpW(pContainer->cache_prefix, lpwszSearchPattern))
            {
                TRACE("found container with prefix %s\n", debugstr_w(pContainer->cache_prefix));
                *ppContainer = pContainer;
                return TRUE;
            }
        }
        else
        {
            if (i == dwIndex)
            {
                TRACE("found container with prefix %s\n", debugstr_w(pContainer->cache_prefix));
                *ppContainer = pContainer;
                return TRUE;
            }
        }
        i++;
    }
    return FALSE;
}

/***********************************************************************
 *           cache_container_lock_index (Internal)
 *
 * Locks the index for system-wide exclusive access.
 *
 * RETURNS
 *  Cache file header if successful
 *  NULL if failed and calls SetLastError.
 */
static LPURLCACHE_HEADER cache_container_lock_index(URLCACHECONTAINER * pContainer)
{
    BYTE index;
    LPVOID pIndexData;
    URLCACHE_HEADER * pHeader;
    DWORD error;

    /* acquire mutex */
    WaitForSingleObject(pContainer->hMutex, INFINITE);

    pIndexData = MapViewOfFile(pContainer->hMapping, FILE_MAP_WRITE, 0, 0, 0);

    if (!pIndexData)
    {
        ReleaseMutex(pContainer->hMutex);
        ERR("Couldn't MapViewOfFile. Error: %d\n", GetLastError());
        return NULL;
    }
    pHeader = (URLCACHE_HEADER *)pIndexData;

    /* file has grown - we need to remap to prevent us getting
     * access violations when we try and access beyond the end
     * of the memory mapped file */
    if (pHeader->dwFileSize != pContainer->file_size)
    {
        UnmapViewOfFile( pHeader );
        cache_container_close_index(pContainer);
        error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
        if (error != ERROR_SUCCESS)
        {
            ReleaseMutex(pContainer->hMutex);
            SetLastError(error);
            return NULL;
        }
        pIndexData = MapViewOfFile(pContainer->hMapping, FILE_MAP_WRITE, 0, 0, 0);

        if (!pIndexData)
        {
            ReleaseMutex(pContainer->hMutex);
            ERR("Couldn't MapViewOfFile. Error: %d\n", GetLastError());
            return NULL;
        }
        pHeader = (URLCACHE_HEADER *)pIndexData;
    }

    TRACE("Signature: %s, file size: %d bytes\n", pHeader->szSignature, pHeader->dwFileSize);

    for (index = 0; index < pHeader->DirectoryCount; index++)
    {
        TRACE("Directory[%d] = \"%.8s\"\n", index, pHeader->directory_data[index].filename);
    }
    
    return pHeader;
}

/***********************************************************************
 *           cache_container_unlock_index (Internal)
 *
 */
static BOOL cache_container_unlock_index(URLCACHECONTAINER * pContainer, LPURLCACHE_HEADER pHeader)
{
    /* release mutex */
    ReleaseMutex(pContainer->hMutex);
    return UnmapViewOfFile(pHeader);
}

#ifndef CHAR_BIT
#define CHAR_BIT    (8 * sizeof(CHAR))
#endif

/***********************************************************************
 *           URLCache_Allocation_BlockIsFree (Internal)
 *
 *  Is the specified block number free?
 *
 * RETURNS
 *    zero if free
 *    non-zero otherwise
 *
 */
static inline BYTE URLCache_Allocation_BlockIsFree(BYTE * AllocationTable, DWORD dwBlockNumber)
{
    BYTE mask = 1 << (dwBlockNumber % CHAR_BIT);
    return (AllocationTable[dwBlockNumber / CHAR_BIT] & mask) == 0;
}

/***********************************************************************
 *           URLCache_Allocation_BlockFree (Internal)
 *
 *  Marks the specified block as free
 *
 * CAUTION
 *    this function is not updating used blocks count
 *
 * RETURNS
 *    nothing
 *
 */
static inline void URLCache_Allocation_BlockFree(BYTE * AllocationTable, DWORD dwBlockNumber)
{
    BYTE mask = ~(1 << (dwBlockNumber % CHAR_BIT));
    AllocationTable[dwBlockNumber / CHAR_BIT] &= mask;
}

/***********************************************************************
 *           URLCache_Allocation_BlockAllocate (Internal)
 *
 *  Marks the specified block as allocated
 *
 * CAUTION
 *     this function is not updating used blocks count
 *
 * RETURNS
 *    nothing
 *
 */
static inline void URLCache_Allocation_BlockAllocate(BYTE * AllocationTable, DWORD dwBlockNumber)
{
    BYTE mask = 1 << (dwBlockNumber % CHAR_BIT);
    AllocationTable[dwBlockNumber / CHAR_BIT] |= mask;
}

/***********************************************************************
 *           URLCache_FindFirstFreeEntry (Internal)
 *
 *  Finds and allocates the first block of free space big enough and
 * sets ppEntry to point to it.
 *
 * RETURNS
 *    ERROR_SUCCESS when free memory block was found
 *    Any other Win32 error code if the entry could not be added
 *
 */
static DWORD URLCache_FindFirstFreeEntry(URLCACHE_HEADER * pHeader, DWORD dwBlocksNeeded, entry_header **ppEntry)
{
    DWORD dwBlockNumber;
    DWORD dwFreeCounter;
    for (dwBlockNumber = 0; dwBlockNumber < pHeader->dwIndexCapacityInBlocks; dwBlockNumber++)
    {
        for (dwFreeCounter = 0; 
            dwFreeCounter < dwBlocksNeeded &&
              dwFreeCounter + dwBlockNumber < pHeader->dwIndexCapacityInBlocks &&
              URLCache_Allocation_BlockIsFree(pHeader->allocation_table, dwBlockNumber + dwFreeCounter);
            dwFreeCounter++)
                TRACE("Found free block at no. %d (0x%x)\n", dwBlockNumber, ENTRY_START_OFFSET + dwBlockNumber * BLOCKSIZE);

        if (dwFreeCounter == dwBlocksNeeded)
        {
            DWORD index;
            TRACE("Found free blocks starting at no. %d (0x%x)\n", dwBlockNumber, ENTRY_START_OFFSET + dwBlockNumber * BLOCKSIZE);
            for (index = 0; index < dwBlocksNeeded; index++)
                URLCache_Allocation_BlockAllocate(pHeader->allocation_table, dwBlockNumber + index);
            *ppEntry = (entry_header*)((LPBYTE)pHeader + ENTRY_START_OFFSET + dwBlockNumber * BLOCKSIZE);
            for (index = 0; index < dwBlocksNeeded * BLOCKSIZE / sizeof(DWORD); index++)
                ((DWORD*)*ppEntry)[index] = 0xdeadbeef;
            (*ppEntry)->blocks_used = dwBlocksNeeded;
            pHeader->dwBlocksInUse += dwBlocksNeeded;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_HANDLE_DISK_FULL;
}

/***********************************************************************
 *           URLCache_DeleteEntry (Internal)
 *
 *  Deletes the specified entry and frees the space allocated to it
 *
 * RETURNS
 *    TRUE if it succeeded
 *    FALSE if it failed
 *
 */
static BOOL URLCache_DeleteEntry(LPURLCACHE_HEADER pHeader, entry_header *pEntry)
{
    DWORD dwStartBlock;
    DWORD dwBlock;

    /* update allocation table */
    dwStartBlock = ((DWORD)((BYTE *)pEntry - (BYTE *)pHeader) - ENTRY_START_OFFSET) / BLOCKSIZE;
    for (dwBlock = dwStartBlock; dwBlock < dwStartBlock + pEntry->blocks_used; dwBlock++)
        URLCache_Allocation_BlockFree(pHeader->allocation_table, dwBlock);

    pHeader->dwBlocksInUse -= pEntry->blocks_used;
    return TRUE;
}

/***********************************************************************
 *           URLCache_LocalFileNameToPathW (Internal)
 *
 *  Copies the full path to the specified buffer given the local file
 * name and the index of the directory it is in. Always sets value in
 * lpBufferSize to the required buffer size (in bytes).
 *
 * RETURNS
 *    TRUE if the buffer was big enough
 *    FALSE if the buffer was too small
 *
 */
static BOOL URLCache_LocalFileNameToPathW(
    const URLCACHECONTAINER * pContainer,
    LPCURLCACHE_HEADER pHeader,
    LPCSTR szLocalFileName,
    BYTE Directory,
    LPWSTR wszPath,
    LPLONG lpBufferSize)
{
    LONG nRequired;
    int path_len = strlenW(pContainer->path);
    int file_name_len = MultiByteToWideChar(CP_ACP, 0, szLocalFileName, -1, NULL, 0);
    if (Directory!=CACHE_CONTAINER_NO_SUBDIR && Directory>=pHeader->DirectoryCount)
    {
        *lpBufferSize = 0;
        return FALSE;
    }

    nRequired = (path_len + file_name_len) * sizeof(WCHAR);
    if(Directory != CACHE_CONTAINER_NO_SUBDIR)
        nRequired += (DIR_LENGTH + 1) * sizeof(WCHAR);
    if (nRequired <= *lpBufferSize)
    {
        int dir_len;

        memcpy(wszPath, pContainer->path, path_len * sizeof(WCHAR));
        if (Directory != CACHE_CONTAINER_NO_SUBDIR)
        {
            dir_len = MultiByteToWideChar(CP_ACP, 0, pHeader->directory_data[Directory].filename, DIR_LENGTH, wszPath + path_len, DIR_LENGTH);
            wszPath[dir_len + path_len] = '\\';
            dir_len++;
        }
        else
        {
            dir_len = 0;
        }
        MultiByteToWideChar(CP_ACP, 0, szLocalFileName, -1, wszPath + dir_len + path_len, file_name_len);
        *lpBufferSize = nRequired;
        return TRUE;
    }
    *lpBufferSize = nRequired;
    return FALSE;
}

/***********************************************************************
 *           URLCache_LocalFileNameToPathA (Internal)
 *
 *  Copies the full path to the specified buffer given the local file
 * name and the index of the directory it is in. Always sets value in
 * lpBufferSize to the required buffer size.
 *
 * RETURNS
 *    TRUE if the buffer was big enough
 *    FALSE if the buffer was too small
 *
 */
static BOOL URLCache_LocalFileNameToPathA(
    const URLCACHECONTAINER * pContainer,
    LPCURLCACHE_HEADER pHeader,
    LPCSTR szLocalFileName,
    BYTE Directory,
    LPSTR szPath,
    LPLONG lpBufferSize)
{
    LONG nRequired;
    int path_len, file_name_len, dir_len;

    if (Directory!=CACHE_CONTAINER_NO_SUBDIR && Directory>=pHeader->DirectoryCount)
    {
        *lpBufferSize = 0;
        return FALSE;
    }

    path_len = WideCharToMultiByte(CP_ACP, 0, pContainer->path, -1, NULL, 0, NULL, NULL) - 1;
    file_name_len = strlen(szLocalFileName) + 1 /* for nul-terminator */;
    if (Directory!=CACHE_CONTAINER_NO_SUBDIR)
        dir_len = DIR_LENGTH+1;
    else
        dir_len = 0;

    nRequired = (path_len + dir_len + file_name_len) * sizeof(char);
    if (nRequired < *lpBufferSize)
    {
        WideCharToMultiByte(CP_ACP, 0, pContainer->path, -1, szPath, path_len, NULL, NULL);
        if(dir_len) {
            memcpy(szPath+path_len, pHeader->directory_data[Directory].filename, dir_len-1);
            szPath[path_len + dir_len-1] = '\\';
        }
        memcpy(szPath + path_len + dir_len, szLocalFileName, file_name_len);
        *lpBufferSize = nRequired;
        return TRUE;
    }
    *lpBufferSize = nRequired;
    return FALSE;
}

/* Just like FileTimeToDosDateTime, except that it also maps the special
 * case of a filetime of (0,0) to a DOS date/time of (0,0).
 */
static void URLCache_FileTimeToDosDateTime(const FILETIME *ft, WORD *fatdate,
                                           WORD *fattime)
{
    if (!ft->dwLowDateTime && !ft->dwHighDateTime)
        *fatdate = *fattime = 0;
    else
        FileTimeToDosDateTime(ft, fatdate, fattime);
}

/***********************************************************************
 *           URLCache_DeleteFile (Internal)
 */
static DWORD URLCache_DeleteFile(const URLCACHECONTAINER *container,
        URLCACHE_HEADER *header, entry_url *url_entry)
{
    WIN32_FILE_ATTRIBUTE_DATA attr;
    WCHAR path[MAX_PATH];
    LONG path_size = sizeof(path);
    DWORD err;
    WORD date, time;

    if(!url_entry->local_name_off)
        goto succ;

    if(!URLCache_LocalFileNameToPathW(container, header,
                (LPCSTR)url_entry+url_entry->local_name_off,
                url_entry->cache_dir, path, &path_size))
        goto succ;

    if(!GetFileAttributesExW(path, GetFileExInfoStandard, &attr))
        goto succ;
    URLCache_FileTimeToDosDateTime(&attr.ftLastWriteTime, &date, &time);
    if(date != url_entry->write_date || time != url_entry->write_time)
        goto succ;

    err = (DeleteFileW(path) ? ERROR_SUCCESS : GetLastError());
    if(err == ERROR_ACCESS_DENIED || err == ERROR_SHARING_VIOLATION)
        return err;

succ:
    if (url_entry->cache_dir < header->DirectoryCount)
    {
        if (header->directory_data[url_entry->cache_dir].dwNumFiles)
            header->directory_data[url_entry->cache_dir].dwNumFiles--;
    }
    if (url_entry->cache_entry_type & STICKY_CACHE_ENTRY)
    {
        if (url_entry->size.QuadPart < header->ExemptUsage.QuadPart)
            header->ExemptUsage.QuadPart -= url_entry->size.QuadPart;
        else
            header->ExemptUsage.QuadPart = 0;
    }
    else
    {
        if (url_entry->size.QuadPart < header->CacheUsage.QuadPart)
            header->CacheUsage.QuadPart -= url_entry->size.QuadPart;
        else
            header->CacheUsage.QuadPart = 0;
    }

    return ERROR_SUCCESS;
}

static BOOL urlcache_clean_leaked_entries(URLCACHECONTAINER *container, URLCACHE_HEADER *header)
{
    DWORD *leak_off;
    BOOL freed = FALSE;

    leak_off = &header->options[CACHE_HEADER_DATA_ROOT_LEAK_OFFSET];
    while(*leak_off) {
        entry_url *url_entry = (entry_url*)((LPBYTE)header + *leak_off);

        if(SUCCEEDED(URLCache_DeleteFile(container, header, url_entry))) {
            *leak_off = url_entry->exempt_delta;
            URLCache_DeleteEntry(header, &url_entry->header);
            freed = TRUE;
        }else {
            leak_off = &url_entry->exempt_delta;
        }
    }

    return freed;
}

/***********************************************************************
 *           cache_container_clean_index (Internal)
 *
 * This function is meant to make place in index file by removing leaked
 * files entries and resizing the file.
 *
 * CAUTION: file view may get mapped to new memory
 *
 * RETURNS
 *     ERROR_SUCCESS when new memory is available
 *     error code otherwise
 */
static DWORD cache_container_clean_index(URLCACHECONTAINER *container, URLCACHE_HEADER **file_view)
{
    URLCACHE_HEADER *header = *file_view;
    DWORD ret;

    TRACE("(%s %s)\n", debugstr_w(container->cache_prefix), debugstr_w(container->path));

    if(urlcache_clean_leaked_entries(container, header))
        return ERROR_SUCCESS;

    if(header->dwFileSize >= ALLOCATION_TABLE_SIZE*8*BLOCKSIZE + ENTRY_START_OFFSET) {
        WARN("index file has maximal size\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    cache_container_close_index(container);
    ret = cache_container_open_index(container, header->dwIndexCapacityInBlocks*2);
    if(ret != ERROR_SUCCESS)
        return ret;
    header = MapViewOfFile(container->hMapping, FILE_MAP_WRITE, 0, 0, 0);
    if(!header)
        return GetLastError();

    UnmapViewOfFile(*file_view);
    *file_view = header;
    return ERROR_SUCCESS;
}

/* Just like DosDateTimeToFileTime, except that it also maps the special
 * case of a DOS date/time of (0,0) to a filetime of (0,0).
 */
static void URLCache_DosDateTimeToFileTime(WORD fatdate, WORD fattime,
                                           FILETIME *ft)
{
    if (!fatdate && !fattime)
        ft->dwLowDateTime = ft->dwHighDateTime = 0;
    else
        DosDateTimeToFileTime(fatdate, fattime, ft);
}

/***********************************************************************
 *           URLCache_CopyEntry (Internal)
 *
 *  Copies an entry from the cache index file to the Win32 structure
 *
 * RETURNS
 *    ERROR_SUCCESS if the buffer was big enough
 *    ERROR_INSUFFICIENT_BUFFER if the buffer was too small
 *
 */
static DWORD URLCache_CopyEntry(
    URLCACHECONTAINER * pContainer,
    LPCURLCACHE_HEADER pHeader, 
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo, 
    LPDWORD lpdwBufferSize, 
    const entry_url * pUrlEntry,
    BOOL bUnicode)
{
    int lenUrl;
    DWORD dwRequiredSize = sizeof(*lpCacheEntryInfo);

    if (*lpdwBufferSize >= dwRequiredSize)
    {
        lpCacheEntryInfo->lpHeaderInfo = NULL;
        lpCacheEntryInfo->lpszFileExtension = NULL;
        lpCacheEntryInfo->lpszLocalFileName = NULL;
        lpCacheEntryInfo->lpszSourceUrlName = NULL;
        lpCacheEntryInfo->CacheEntryType = pUrlEntry->cache_entry_type;
        lpCacheEntryInfo->u.dwExemptDelta = pUrlEntry->exempt_delta;
        lpCacheEntryInfo->dwHeaderInfoSize = pUrlEntry->header_info_size;
        lpCacheEntryInfo->dwHitRate = pUrlEntry->hit_rate;
        lpCacheEntryInfo->dwSizeHigh = pUrlEntry->size.u.HighPart;
        lpCacheEntryInfo->dwSizeLow = pUrlEntry->size.u.LowPart;
        lpCacheEntryInfo->dwStructSize = sizeof(*lpCacheEntryInfo);
        lpCacheEntryInfo->dwUseCount = pUrlEntry->use_count;
        URLCache_DosDateTimeToFileTime(pUrlEntry->expire_date, pUrlEntry->expire_time, &lpCacheEntryInfo->ExpireTime);
        lpCacheEntryInfo->LastAccessTime = pUrlEntry->access_time;
        lpCacheEntryInfo->LastModifiedTime = pUrlEntry->modification_time;
        URLCache_DosDateTimeToFileTime(pUrlEntry->sync_date, pUrlEntry->sync_time, &lpCacheEntryInfo->LastSyncTime);
    }

    if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
        ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
    dwRequiredSize = DWORD_ALIGN(dwRequiredSize);
    if (bUnicode)
        lenUrl = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pUrlEntry + pUrlEntry->url_off, -1, NULL, 0);
    else
        lenUrl = strlen((LPCSTR)pUrlEntry + pUrlEntry->url_off);
    dwRequiredSize += (lenUrl + 1) * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

    /* FIXME: is source url optional? */
    if (*lpdwBufferSize >= dwRequiredSize)
    {
        DWORD lenUrlBytes = (lenUrl+1) * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

        lpCacheEntryInfo->lpszSourceUrlName = (LPSTR)lpCacheEntryInfo + dwRequiredSize - lenUrlBytes;
        if (bUnicode)
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pUrlEntry + pUrlEntry->url_off, -1, (LPWSTR)lpCacheEntryInfo->lpszSourceUrlName, lenUrl + 1);
        else
            memcpy(lpCacheEntryInfo->lpszSourceUrlName, (LPCSTR)pUrlEntry + pUrlEntry->url_off, lenUrlBytes);
    }

    if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
        ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
    dwRequiredSize = DWORD_ALIGN(dwRequiredSize);

    if (pUrlEntry->local_name_off)
    {
        LONG nLocalFilePathSize;
        LPSTR lpszLocalFileName;
        lpszLocalFileName = (LPSTR)lpCacheEntryInfo + dwRequiredSize;
        nLocalFilePathSize = *lpdwBufferSize - dwRequiredSize;
        if ((bUnicode && URLCache_LocalFileNameToPathW(pContainer, pHeader, (LPCSTR)pUrlEntry + pUrlEntry->local_name_off, pUrlEntry->cache_dir, (LPWSTR)lpszLocalFileName, &nLocalFilePathSize)) ||
            (!bUnicode && URLCache_LocalFileNameToPathA(pContainer, pHeader, (LPCSTR)pUrlEntry + pUrlEntry->local_name_off, pUrlEntry->cache_dir, lpszLocalFileName, &nLocalFilePathSize)))
        {
            lpCacheEntryInfo->lpszLocalFileName = lpszLocalFileName;
        }
        dwRequiredSize += nLocalFilePathSize * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR)) ;

        if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
            ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
        dwRequiredSize = DWORD_ALIGN(dwRequiredSize);
    }
    dwRequiredSize += pUrlEntry->header_info_size + 1;

    if (*lpdwBufferSize >= dwRequiredSize)
    {
        lpCacheEntryInfo->lpHeaderInfo = (LPBYTE)lpCacheEntryInfo + dwRequiredSize - pUrlEntry->header_info_size - 1;
        memcpy(lpCacheEntryInfo->lpHeaderInfo, (LPCSTR)pUrlEntry + pUrlEntry->header_info_off, pUrlEntry->header_info_size);
        ((LPBYTE)lpCacheEntryInfo)[dwRequiredSize - 1] = '\0';
    }
    if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
        ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
    dwRequiredSize = DWORD_ALIGN(dwRequiredSize);

    if (pUrlEntry->file_extension_off)
    {
        int lenExtension;

        if (bUnicode)
            lenExtension = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pUrlEntry + pUrlEntry->file_extension_off, -1, NULL, 0);
        else
            lenExtension = strlen((LPCSTR)pUrlEntry + pUrlEntry->file_extension_off) + 1;
        dwRequiredSize += lenExtension * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

        if (*lpdwBufferSize >= dwRequiredSize)
        {
            lpCacheEntryInfo->lpszFileExtension = (LPSTR)lpCacheEntryInfo + dwRequiredSize - lenExtension;
            if (bUnicode)
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pUrlEntry + pUrlEntry->file_extension_off, -1, (LPWSTR)lpCacheEntryInfo->lpszSourceUrlName, lenExtension);
            else
                memcpy(lpCacheEntryInfo->lpszFileExtension, (LPCSTR)pUrlEntry + pUrlEntry->file_extension_off, lenExtension * sizeof(CHAR));
        }

        if ((dwRequiredSize % 4) && (dwRequiredSize < *lpdwBufferSize))
            ZeroMemory((LPBYTE)lpCacheEntryInfo + dwRequiredSize, 4 - (dwRequiredSize % 4));
        dwRequiredSize = DWORD_ALIGN(dwRequiredSize);
    }

    if (dwRequiredSize > *lpdwBufferSize)
    {
        *lpdwBufferSize = dwRequiredSize;
        return ERROR_INSUFFICIENT_BUFFER;
    }
    *lpdwBufferSize = dwRequiredSize;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           URLCache_SetEntryInfo (Internal)
 *
 *  Helper for SetUrlCacheEntryInfo{A,W}. Sets fields in URL entry
 * according to the flags set by dwFieldControl.
 *
 * RETURNS
 *    ERROR_SUCCESS if the buffer was big enough
 *    ERROR_INSUFFICIENT_BUFFER if the buffer was too small
 *
 */
static DWORD URLCache_SetEntryInfo(entry_url * pUrlEntry, const INTERNET_CACHE_ENTRY_INFOW * lpCacheEntryInfo, DWORD dwFieldControl)
{
    if (dwFieldControl & CACHE_ENTRY_ACCTIME_FC)
        pUrlEntry->access_time = lpCacheEntryInfo->LastAccessTime;
    if (dwFieldControl & CACHE_ENTRY_ATTRIBUTE_FC)
        pUrlEntry->cache_entry_type = lpCacheEntryInfo->CacheEntryType;
    if (dwFieldControl & CACHE_ENTRY_EXEMPT_DELTA_FC)
        pUrlEntry->exempt_delta = lpCacheEntryInfo->u.dwExemptDelta;
    if (dwFieldControl & CACHE_ENTRY_EXPTIME_FC)
        URLCache_FileTimeToDosDateTime(&lpCacheEntryInfo->ExpireTime, &pUrlEntry->expire_date, &pUrlEntry->expire_time);
    if (dwFieldControl & CACHE_ENTRY_HEADERINFO_FC)
        FIXME("CACHE_ENTRY_HEADERINFO_FC unimplemented\n");
    if (dwFieldControl & CACHE_ENTRY_HITRATE_FC)
        pUrlEntry->hit_rate = lpCacheEntryInfo->dwHitRate;
    if (dwFieldControl & CACHE_ENTRY_MODTIME_FC)
        pUrlEntry->modification_time = lpCacheEntryInfo->LastModifiedTime;
    if (dwFieldControl & CACHE_ENTRY_SYNCTIME_FC)
        URLCache_FileTimeToDosDateTime(&lpCacheEntryInfo->LastAccessTime, &pUrlEntry->sync_date, &pUrlEntry->sync_time);

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           URLCache_HashKey (Internal)
 *
 *  Returns the hash key for a given string
 *
 * RETURNS
 *    hash key for the string
 *
 */
static DWORD URLCache_HashKey(LPCSTR lpszKey)
{
    /* NOTE: this uses the same lookup table as SHLWAPI.UrlHash{A,W}
     * but the algorithm and result are not the same!
     */
    static const unsigned char lookupTable[256] = 
    {
        0x01, 0x0E, 0x6E, 0x19, 0x61, 0xAE, 0x84, 0x77,
        0x8A, 0xAA, 0x7D, 0x76, 0x1B, 0xE9, 0x8C, 0x33,
        0x57, 0xC5, 0xB1, 0x6B, 0xEA, 0xA9, 0x38, 0x44,
        0x1E, 0x07, 0xAD, 0x49, 0xBC, 0x28, 0x24, 0x41,
        0x31, 0xD5, 0x68, 0xBE, 0x39, 0xD3, 0x94, 0xDF,
        0x30, 0x73, 0x0F, 0x02, 0x43, 0xBA, 0xD2, 0x1C,
        0x0C, 0xB5, 0x67, 0x46, 0x16, 0x3A, 0x4B, 0x4E,
        0xB7, 0xA7, 0xEE, 0x9D, 0x7C, 0x93, 0xAC, 0x90,
        0xB0, 0xA1, 0x8D, 0x56, 0x3C, 0x42, 0x80, 0x53,
        0x9C, 0xF1, 0x4F, 0x2E, 0xA8, 0xC6, 0x29, 0xFE,
        0xB2, 0x55, 0xFD, 0xED, 0xFA, 0x9A, 0x85, 0x58,
        0x23, 0xCE, 0x5F, 0x74, 0xFC, 0xC0, 0x36, 0xDD,
        0x66, 0xDA, 0xFF, 0xF0, 0x52, 0x6A, 0x9E, 0xC9,
        0x3D, 0x03, 0x59, 0x09, 0x2A, 0x9B, 0x9F, 0x5D,
        0xA6, 0x50, 0x32, 0x22, 0xAF, 0xC3, 0x64, 0x63,
        0x1A, 0x96, 0x10, 0x91, 0x04, 0x21, 0x08, 0xBD,
        0x79, 0x40, 0x4D, 0x48, 0xD0, 0xF5, 0x82, 0x7A,
        0x8F, 0x37, 0x69, 0x86, 0x1D, 0xA4, 0xB9, 0xC2,
        0xC1, 0xEF, 0x65, 0xF2, 0x05, 0xAB, 0x7E, 0x0B,
        0x4A, 0x3B, 0x89, 0xE4, 0x6C, 0xBF, 0xE8, 0x8B,
        0x06, 0x18, 0x51, 0x14, 0x7F, 0x11, 0x5B, 0x5C,
        0xFB, 0x97, 0xE1, 0xCF, 0x15, 0x62, 0x71, 0x70,
        0x54, 0xE2, 0x12, 0xD6, 0xC7, 0xBB, 0x0D, 0x20,
        0x5E, 0xDC, 0xE0, 0xD4, 0xF7, 0xCC, 0xC4, 0x2B,
        0xF9, 0xEC, 0x2D, 0xF4, 0x6F, 0xB6, 0x99, 0x88,
        0x81, 0x5A, 0xD9, 0xCA, 0x13, 0xA5, 0xE7, 0x47,
        0xE6, 0x8E, 0x60, 0xE3, 0x3E, 0xB3, 0xF6, 0x72,
        0xA2, 0x35, 0xA0, 0xD7, 0xCD, 0xB4, 0x2F, 0x6D,
        0x2C, 0x26, 0x1F, 0x95, 0x87, 0x00, 0xD8, 0x34,
        0x3F, 0x17, 0x25, 0x45, 0x27, 0x75, 0x92, 0xB8,
        0xA3, 0xC8, 0xDE, 0xEB, 0xF8, 0xF3, 0xDB, 0x0A,
        0x98, 0x83, 0x7B, 0xE5, 0xCB, 0x4C, 0x78, 0xD1
    };
    BYTE key[4];
    DWORD i;

    for (i = 0; i < sizeof(key) / sizeof(key[0]); i++)
        key[i] = lookupTable[(*lpszKey + i) & 0xFF];

    for (lpszKey++; *lpszKey && ((lpszKey[0] != '/') || (lpszKey[1] != 0)); lpszKey++)
    {
        for (i = 0; i < sizeof(key) / sizeof(key[0]); i++)
            key[i] = lookupTable[*lpszKey ^ key[i]];
    }

    return *(DWORD *)key;
}

static inline HASH_CACHEFILE_ENTRY * URLCache_HashEntryFromOffset(LPCURLCACHE_HEADER pHeader, DWORD dwOffset)
{
    return (HASH_CACHEFILE_ENTRY *)((LPBYTE)pHeader + dwOffset);
}

static inline BOOL URLCache_IsHashEntryValid(LPCURLCACHE_HEADER pHeader, const HASH_CACHEFILE_ENTRY *pHashEntry)
{
    /* check pHashEntry located within acceptable bounds in the URL cache mapping */
    return ((DWORD)((const BYTE*)pHashEntry - (const BYTE*)pHeader) >= ENTRY_START_OFFSET) &&
           ((DWORD)((const BYTE*)pHashEntry - (const BYTE*)pHeader) < pHeader->dwFileSize);
}

static BOOL URLCache_FindHash(LPCURLCACHE_HEADER pHeader, LPCSTR lpszUrl, struct _HASH_ENTRY ** ppHashEntry)
{
    /* structure of hash table:
     *  448 entries divided into 64 blocks
     *  each block therefore contains a chain of 7 key/offset pairs
     * how position in table is calculated:
     *  1. the url is hashed in helper function
     *  2. the key % HASHTABLE_NUM_ENTRIES is the bucket number
     *  3. bucket number * HASHTABLE_BLOCKSIZE is offset of the bucket
     *
     * note:
     *  there can be multiple hash tables in the file and the offset to
     *  the next one is stored in the header of the hash table
     */
    DWORD key = URLCache_HashKey(lpszUrl);
    DWORD offset = (key & (HASHTABLE_NUM_ENTRIES-1)) * HASHTABLE_BLOCKSIZE;
    HASH_CACHEFILE_ENTRY * pHashEntry;
    DWORD dwHashTableNumber = 0;

    key >>= HASHTABLE_FLAG_BITS;

    for (pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHeader->dwOffsetFirstHashTable);
         URLCache_IsHashEntryValid(pHeader, pHashEntry);
         pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHashEntry->dwAddressNext))
    {
        int i;
        if (pHashEntry->dwHashTableNumber != dwHashTableNumber++)
        {
            ERR("Error: not right hash table number (%d) expected %d\n", pHashEntry->dwHashTableNumber, dwHashTableNumber);
            continue;
        }
        /* make sure that it is in fact a hash entry */
        if (pHashEntry->CacheFileEntry.signature != HASH_SIGNATURE)
        {
            ERR("Error: not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&pHashEntry->CacheFileEntry.signature);
            continue;
        }

        for (i = 0; i < HASHTABLE_BLOCKSIZE; i++)
        {
            struct _HASH_ENTRY * pHashElement = &pHashEntry->HashTable[offset + i];
            if (key == pHashElement->dwHashKey>>HASHTABLE_FLAG_BITS)
            {
                /* FIXME: we should make sure that this is the right element
                 * before returning and claiming that it is. We can do this
                 * by doing a simple compare between the URL we were given
                 * and the URL stored in the entry. However, this assumes
                 * we know the format of all the entries stored in the
                 * hash table */
                *ppHashEntry = pHashElement;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL URLCache_FindHashW(LPCURLCACHE_HEADER pHeader, LPCWSTR lpszUrl, struct _HASH_ENTRY ** ppHashEntry)
{
    LPSTR urlA;
    BOOL ret;

    urlA = heap_strdupWtoA(lpszUrl);
    if (!urlA)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    ret = URLCache_FindHash(pHeader, urlA, ppHashEntry);
    heap_free(urlA);
    return ret;
}

/***********************************************************************
 *           URLCache_HashEntrySetFlags (Internal)
 *
 *  Sets special bits in hash key
 *
 * RETURNS
 *    nothing
 *
 */
static void URLCache_HashEntrySetFlags(struct _HASH_ENTRY * pHashEntry, DWORD dwFlag)
{
    pHashEntry->dwHashKey = (pHashEntry->dwHashKey >> HASHTABLE_FLAG_BITS << HASHTABLE_FLAG_BITS) | dwFlag;
}

/***********************************************************************
 *           URLCache_DeleteEntryFromHash (Internal)
 *
 *  Searches all the hash tables in the index for the given URL and
 * then if found deletes the entry.
 *
 * RETURNS
 *    TRUE if the entry was found
 *    FALSE if the entry could not be found
 *
 */
static BOOL URLCache_DeleteEntryFromHash(struct _HASH_ENTRY * pHashEntry)
{
    pHashEntry->dwHashKey = HASHTABLE_DEL;
    return TRUE;
}

/***********************************************************************
 *           URLCache_AddEntryToHash (Internal)
 *
 *  Searches all the hash tables for a free slot based on the offset
 * generated from the hash key. If a free slot is found, the offset and
 * key are entered into the hash table.
 *
 * RETURNS
 *    ERROR_SUCCESS if the entry was added
 *    Any other Win32 error code if the entry could not be added
 *
 */
static DWORD URLCache_AddEntryToHash(LPURLCACHE_HEADER pHeader, LPCSTR lpszUrl, DWORD dwOffsetEntry, DWORD dwFieldType)
{
    /* see URLCache_FindEntryInHash for structure of hash tables */

    DWORD key = URLCache_HashKey(lpszUrl);
    DWORD offset = (key & (HASHTABLE_NUM_ENTRIES-1)) * HASHTABLE_BLOCKSIZE;
    HASH_CACHEFILE_ENTRY * pHashEntry, *pHashPrev = NULL;
    DWORD dwHashTableNumber = 0;
    DWORD error;

    key = ((key >> HASHTABLE_FLAG_BITS) << HASHTABLE_FLAG_BITS) + dwFieldType;

    for (pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHeader->dwOffsetFirstHashTable);
         URLCache_IsHashEntryValid(pHeader, pHashEntry);
         pHashEntry = URLCache_HashEntryFromOffset(pHeader, pHashEntry->dwAddressNext))
    {
        int i;
        pHashPrev = pHashEntry;

        if (pHashEntry->dwHashTableNumber != dwHashTableNumber++)
        {
            ERR("not right hash table number (%d) expected %d\n", pHashEntry->dwHashTableNumber, dwHashTableNumber);
            break;
        }
        /* make sure that it is in fact a hash entry */
        if (pHashEntry->CacheFileEntry.signature != HASH_SIGNATURE)
        {
            ERR("not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&pHashEntry->CacheFileEntry.signature);
            break;
        }

        for (i = 0; i < HASHTABLE_BLOCKSIZE; i++)
        {
            struct _HASH_ENTRY * pHashElement = &pHashEntry->HashTable[offset + i];
            if (pHashElement->dwHashKey==HASHTABLE_FREE || pHashElement->dwHashKey==HASHTABLE_DEL) /* if the slot is free */
            {
                pHashElement->dwHashKey = key;
                pHashElement->dwOffsetEntry = dwOffsetEntry;
                return ERROR_SUCCESS;
            }
        }
    }
    error = URLCache_CreateHashTable(pHeader, pHashPrev, &pHashEntry);
    if (error != ERROR_SUCCESS)
        return error;

    pHashEntry->HashTable[offset].dwHashKey = key;
    pHashEntry->HashTable[offset].dwOffsetEntry = dwOffsetEntry;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           URLCache_CreateHashTable (Internal)
 *
 *  Creates a new hash table in free space and adds it to the chain of existing
 * hash tables.
 *
 * RETURNS
 *    ERROR_SUCCESS if the hash table was created
 *    ERROR_DISK_FULL if the hash table could not be created
 *
 */
static DWORD URLCache_CreateHashTable(LPURLCACHE_HEADER pHeader, HASH_CACHEFILE_ENTRY *pPrevHash, HASH_CACHEFILE_ENTRY **ppHash)
{
    DWORD dwOffset, error;
    int i;

    if ((error = URLCache_FindFirstFreeEntry(pHeader, 0x20, (entry_header**)ppHash)) != ERROR_SUCCESS)
        return error;

    dwOffset = (BYTE *)*ppHash - (BYTE *)pHeader;

    if (pPrevHash)
        pPrevHash->dwAddressNext = dwOffset;
    else
        pHeader->dwOffsetFirstHashTable = dwOffset;
    (*ppHash)->CacheFileEntry.signature = HASH_SIGNATURE;
    (*ppHash)->CacheFileEntry.blocks_used = 0x20;
    (*ppHash)->dwAddressNext = 0;
    (*ppHash)->dwHashTableNumber = pPrevHash ? pPrevHash->dwHashTableNumber + 1 : 0;
    for (i = 0; i < HASHTABLE_SIZE; i++)
    {
        (*ppHash)->HashTable[i].dwOffsetEntry = HASHTABLE_FREE;
        (*ppHash)->HashTable[i].dwHashKey = HASHTABLE_FREE;
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           URLCache_EnumHashTables (Internal)
 *
 *  Enumerates the hash tables in a container.
 *
 * RETURNS
 *    TRUE if an entry was found
 *    FALSE if there are no more tables to enumerate.
 *
 */
static BOOL URLCache_EnumHashTables(LPCURLCACHE_HEADER pHeader, DWORD *pdwHashTableNumber, HASH_CACHEFILE_ENTRY ** ppHashEntry)
{
    for (*ppHashEntry = URLCache_HashEntryFromOffset(pHeader, pHeader->dwOffsetFirstHashTable);
         URLCache_IsHashEntryValid(pHeader, *ppHashEntry);
         *ppHashEntry = URLCache_HashEntryFromOffset(pHeader, (*ppHashEntry)->dwAddressNext))
    {
        TRACE("looking at hash table number %d\n", (*ppHashEntry)->dwHashTableNumber);
        if ((*ppHashEntry)->dwHashTableNumber != *pdwHashTableNumber)
            continue;
        /* make sure that it is in fact a hash entry */
        if ((*ppHashEntry)->CacheFileEntry.signature != HASH_SIGNATURE)
        {
            ERR("Error: not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&(*ppHashEntry)->CacheFileEntry.signature);
            (*pdwHashTableNumber)++;
            continue;
        }

        TRACE("hash table number %d found\n", *pdwHashTableNumber);
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           URLCache_EnumHashTableEntries (Internal)
 *
 *  Enumerates entries in a hash table and returns the next non-free entry.
 *
 * RETURNS
 *    TRUE if an entry was found
 *    FALSE if the hash table is empty or there are no more entries to
 *     enumerate.
 *
 */
static BOOL URLCache_EnumHashTableEntries(LPCURLCACHE_HEADER pHeader, const HASH_CACHEFILE_ENTRY * pHashEntry,
                                          DWORD * index, const struct _HASH_ENTRY ** ppHashEntry)
{
    for (; *index < HASHTABLE_SIZE ; (*index)++)
    {
        if (pHashEntry->HashTable[*index].dwHashKey==HASHTABLE_FREE || pHashEntry->HashTable[*index].dwHashKey==HASHTABLE_DEL)
            continue;

        *ppHashEntry = &pHashEntry->HashTable[*index];
        TRACE("entry found %d\n", *index);
        return TRUE;
    }
    TRACE("no more entries (%d)\n", *index);
    return FALSE;
}

/***********************************************************************
 *           URLCache_DeleteCacheDirectory (Internal)
 *
 *  Erase a directory containing an URL cache.
 *
 * RETURNS
 *    TRUE success, FALSE failure/aborted.
 *
 */
static BOOL URLCache_DeleteCacheDirectory(LPCWSTR lpszPath)
{
    DWORD path_len;
    WCHAR path[MAX_PATH + 1];
    SHFILEOPSTRUCTW shfos;
    int ret;

    path_len = strlenW(lpszPath);
    if (path_len >= MAX_PATH)
        return FALSE;
    strcpyW(path, lpszPath);
    path[path_len + 1] = 0;  /* double-NUL-terminate path */

    shfos.hwnd = NULL;
    shfos.wFunc = FO_DELETE;
    shfos.pFrom = path;
    shfos.pTo = NULL;
    shfos.fFlags = FOF_NOCONFIRMATION;
    shfos.fAnyOperationsAborted = FALSE;
    ret = SHFileOperationW(&shfos);
    if (ret)
        ERR("SHFileOperationW on %s returned %i\n", debugstr_w(path), ret);
    return !(ret || shfos.fAnyOperationsAborted);
}

/***********************************************************************
 *           URLCache_IsLocked (Internal)
 *
 *  Checks if entry is locked. Unlocks it if possible.
 */
static BOOL URLCache_IsLocked(struct _HASH_ENTRY *hash_entry, entry_url *url_entry)
{
    FILETIME cur_time;
    ULARGE_INTEGER acc_time, time;

    if ((hash_entry->dwHashKey & ((1<<HASHTABLE_FLAG_BITS)-1)) != HASHTABLE_LOCK)
        return FALSE;

    GetSystemTimeAsFileTime(&cur_time);
    time.u.LowPart = cur_time.dwLowDateTime;
    time.u.HighPart = cur_time.dwHighDateTime;

    acc_time.u.LowPart = url_entry->access_time.dwLowDateTime;
    acc_time.u.HighPart = url_entry->access_time.dwHighDateTime;

    time.QuadPart -= acc_time.QuadPart;

    /* check if entry was locked for at least a day */
    if(time.QuadPart > (ULONGLONG)24*60*60*FILETIME_SECOND) {
        URLCache_HashEntrySetFlags(hash_entry, HASHTABLE_URL);
        url_entry->use_count = 0;
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExA(
    LPCSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    LPDWORD lpdwCacheEntryInfoBufSize,
    LPSTR lpszReserved,
    LPDWORD lpdwReserved,
    LPVOID lpReserved,
    DWORD dwFlags)
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    const entry_header *pEntry;
    const entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, %p, %p, %p, %p, %p, %x)\n",
        debugstr_a(lpszUrl), 
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufSize,
        lpszReserved,
        lpdwReserved,
        lpReserved,
        dwFlags);

    if ((lpszReserved != NULL) ||
        (lpdwReserved != NULL) ||
        (lpReserved != NULL))
    {
        ERR("Reserved value was not 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (dwFlags & ~GET_INSTALLED_ENTRY)
        FIXME("ignoring unsupported flags: %x\n", dwFlags);

    error = URLCacheContainers_FindContainerA(lpszUrl, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHash(pHeader, lpszUrl, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        WARN("entry %s not found!\n", debugstr_a(lpszUrl));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (const entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n",
                debugstr_an((LPCSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (const entry_url *)pEntry;
    TRACE("Found URL: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->url_off));
    TRACE("Header info: %s\n", debugstr_an((LPCSTR)pUrlEntry +
                pUrlEntry->header_info_off, pUrlEntry->header_info_size));

    if((dwFlags & GET_INSTALLED_ENTRY) && !(pUrlEntry->cache_entry_type & INSTALLED_CACHE_ENTRY))
    {
        cache_container_unlock_index(pContainer, pHeader);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (lpdwCacheEntryInfoBufSize)
    {
        if (!lpCacheEntryInfo)
            *lpdwCacheEntryInfoBufSize = 0;

        error = URLCache_CopyEntry(
            pContainer,
            pHeader,
            lpCacheEntryInfo,
            lpdwCacheEntryInfoBufSize,
            pUrlEntry,
            FALSE /* ANSI */);
        if (error != ERROR_SUCCESS)
        {
            cache_container_unlock_index(pContainer, pHeader);
            SetLastError(error);
            return FALSE;
        }
        if(pUrlEntry->local_name_off)
            TRACE("Local File Name: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->local_name_off));
    }

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
)
{
    return GetUrlCacheEntryInfoExA(lpszUrlName, lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize, NULL, NULL, NULL, 0);
}

/***********************************************************************
 *           GetUrlCacheEntryInfoW (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoW(LPCWSTR lpszUrl,
  LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
  LPDWORD lpdwCacheEntryInfoBufferSize)
{
    return GetUrlCacheEntryInfoExW(lpszUrl, lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize, NULL, NULL, NULL, 0);
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExW (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExW(
    LPCWSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    LPDWORD lpdwCacheEntryInfoBufSize,
    LPWSTR lpszReserved,
    LPDWORD lpdwReserved,
    LPVOID lpReserved,
    DWORD dwFlags)
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    const entry_header *pEntry;
    const entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, %p, %p, %p, %p, %p, %x)\n",
        debugstr_w(lpszUrl),
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufSize,
        lpszReserved,
        lpdwReserved,
        lpReserved,
        dwFlags);

    /* Ignore GET_INSTALLED_ENTRY flag in unicode version of function */
    dwFlags &= ~GET_INSTALLED_ENTRY;

    if ((lpszReserved != NULL) ||
        (lpdwReserved != NULL) ||
        (lpReserved != NULL))
    {
        ERR("Reserved value was not 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (dwFlags)
        FIXME("ignoring unsupported flags: %x\n", dwFlags);

    error = URLCacheContainers_FindContainerW(lpszUrl, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHashW(pHeader, lpszUrl, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        WARN("entry %s not found!\n", debugstr_w(lpszUrl));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (const entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n",
                debugstr_an((LPCSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (const entry_url *)pEntry;
    TRACE("Found URL: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->url_off));
    TRACE("Header info: %s\n", debugstr_an((LPCSTR)pUrlEntry +
                pUrlEntry->header_info_off, pUrlEntry->header_info_size));

    if (lpdwCacheEntryInfoBufSize)
    {
        if (!lpCacheEntryInfo)
            *lpdwCacheEntryInfoBufSize = 0;

        error = URLCache_CopyEntry(
                pContainer,
                pHeader,
                (LPINTERNET_CACHE_ENTRY_INFOA)lpCacheEntryInfo,
                lpdwCacheEntryInfoBufSize,
                pUrlEntry,
                TRUE /* UNICODE */);
        if (error != ERROR_SUCCESS)
        {
            cache_container_unlock_index(pContainer, pHeader);
            SetLastError(error);
            return FALSE;
        }
        if(pUrlEntry->local_name_off)
            TRACE("Local File Name: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->local_name_off));
    }

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           SetUrlCacheEntryInfoA (WININET.@)
 */
BOOL WINAPI SetUrlCacheEntryInfoA(
    LPCSTR lpszUrlName,
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    DWORD dwFieldControl)
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    entry_header *pEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, %p, 0x%08x)\n", debugstr_a(lpszUrlName), lpCacheEntryInfo, dwFieldControl);

    error = URLCacheContainers_FindContainerA(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHash(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        WARN("entry %s not found!\n", debugstr_a(lpszUrlName));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    URLCache_SetEntryInfo(
        (entry_url *)pEntry,
        (const INTERNET_CACHE_ENTRY_INFOW *)lpCacheEntryInfo,
        dwFieldControl);

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           SetUrlCacheEntryInfoW (WININET.@)
 */
BOOL WINAPI SetUrlCacheEntryInfoW(LPCWSTR lpszUrl, LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo, DWORD dwFieldControl)
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    entry_header *pEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, %p, 0x%08x)\n", debugstr_w(lpszUrl), lpCacheEntryInfo, dwFieldControl);

    error = URLCacheContainers_FindContainerW(lpszUrl, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHashW(pHeader, lpszUrl, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        WARN("entry %s not found!\n", debugstr_w(lpszUrl));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    URLCache_SetEntryInfo(
        (entry_url *)pEntry,
        lpCacheEntryInfo,
        dwFieldControl);

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           RetrieveUrlCacheEntryFileA (WININET.@)
 *
 */
BOOL WINAPI RetrieveUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo, 
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    )
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    entry_header *pEntry;
    entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, %p, %p, 0x%08x)\n",
        debugstr_a(lpszUrlName),
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufferSize,
        dwReserved);

    if (!lpszUrlName || !lpdwCacheEntryInfoBufferSize ||
        (!lpCacheEntryInfo && *lpdwCacheEntryInfoBufferSize))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    error = URLCacheContainers_FindContainerA(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHash(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", lpszUrlName);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (entry_url *)pEntry;
    if (!pUrlEntry->local_name_off)
    {
        cache_container_unlock_index(pContainer, pHeader);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    TRACE("Found URL: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->url_off));
    TRACE("Header info: %s\n", debugstr_an((LPCSTR)pUrlEntry + pUrlEntry->header_info_off,
                pUrlEntry->header_info_size));

    error = URLCache_CopyEntry(pContainer, pHeader, lpCacheEntryInfo,
                               lpdwCacheEntryInfoBufferSize, pUrlEntry,
                               FALSE);
    if (error != ERROR_SUCCESS)
    {
        cache_container_unlock_index(pContainer, pHeader);
        SetLastError(error);
        return FALSE;
    }
    TRACE("Local File Name: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->local_name_off));

    pUrlEntry->hit_rate++;
    pUrlEntry->use_count++;
    URLCache_HashEntrySetFlags(pHashEntry, HASHTABLE_LOCK);
    GetSystemTimeAsFileTime(&pUrlEntry->access_time);

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           RetrieveUrlCacheEntryFileW (WININET.@)
 *
 */
BOOL WINAPI RetrieveUrlCacheEntryFileW(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    )
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    entry_header *pEntry;
    entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, %p, %p, 0x%08x)\n",
        debugstr_w(lpszUrlName),
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufferSize,
        dwReserved);

    if (!lpszUrlName || !lpdwCacheEntryInfoBufferSize ||
        (!lpCacheEntryInfo && *lpdwCacheEntryInfoBufferSize))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    error = URLCacheContainers_FindContainerW(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHashW(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", debugstr_w(lpszUrlName));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (entry_url *)pEntry;
    if (!pUrlEntry->local_name_off)
    {
        cache_container_unlock_index(pContainer, pHeader);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    TRACE("Found URL: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->url_off));
    TRACE("Header info: %s\n", debugstr_an((LPCSTR)pUrlEntry + pUrlEntry->header_info_off,
            pUrlEntry->header_info_size));

    error = URLCache_CopyEntry(
        pContainer,
        pHeader,
        (LPINTERNET_CACHE_ENTRY_INFOA)lpCacheEntryInfo,
        lpdwCacheEntryInfoBufferSize,
        pUrlEntry,
        TRUE /* UNICODE */);
    if (error != ERROR_SUCCESS)
    {
        cache_container_unlock_index(pContainer, pHeader);
        SetLastError(error);
        return FALSE;
    }
    TRACE("Local File Name: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->local_name_off));

    pUrlEntry->hit_rate++;
    pUrlEntry->use_count++;
    URLCache_HashEntrySetFlags(pHashEntry, HASHTABLE_LOCK);
    GetSystemTimeAsFileTime(&pUrlEntry->access_time);

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

static BOOL DeleteUrlCacheEntryInternal(const URLCACHECONTAINER * pContainer,
        LPURLCACHE_HEADER pHeader, struct _HASH_ENTRY *pHashEntry)
{
    entry_header *pEntry;
    entry_url * pUrlEntry;

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        FIXME("Trying to delete entry of unknown format %s\n",
              debugstr_an((LPCSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (entry_url *)pEntry;
    if(URLCache_IsLocked(pHashEntry, pUrlEntry))
    {
        TRACE("Trying to delete locked entry\n");
        pUrlEntry->cache_entry_type |= PENDING_DELETE_CACHE_ENTRY;
        SetLastError(ERROR_SHARING_VIOLATION);
        return FALSE;
    }

    if(!URLCache_DeleteFile(pContainer, pHeader, pUrlEntry))
    {
        URLCache_DeleteEntry(pHeader, pEntry);
    }
    else
    {
        /* Add entry to leaked files list */
        pUrlEntry->header.signature = LEAK_SIGNATURE;
        pUrlEntry->exempt_delta = pHeader->options[CACHE_HEADER_DATA_ROOT_LEAK_OFFSET];
        pHeader->options[CACHE_HEADER_DATA_ROOT_LEAK_OFFSET] = pHashEntry->dwOffsetEntry;
    }

    URLCache_DeleteEntryFromHash(pHashEntry);
    return TRUE;
}

static HANDLE free_cache_running;
static HANDLE dll_unload_event;
static DWORD WINAPI handle_full_cache_worker(void *param)
{
    FreeUrlCacheSpaceW(NULL, 20, 0);
    ReleaseSemaphore(free_cache_running, 1, NULL);
    return 0;
}

static void handle_full_cache(void)
{
    if(WaitForSingleObject(free_cache_running, 0) == WAIT_OBJECT_0) {
        if(!QueueUserWorkItem(handle_full_cache_worker, NULL, 0))
            ReleaseSemaphore(free_cache_running, 1, NULL);
    }
}

/* Enumerates entries in cache, allows cache unlocking between calls. */
static BOOL urlcache_next_entry(URLCACHE_HEADER *header, DWORD *hash_table_off, DWORD *hash_table_entry,
        struct _HASH_ENTRY **hash_entry, entry_header **entry)
{
    HASH_CACHEFILE_ENTRY *hashtable_entry;

    *hash_entry = NULL;
    *entry = NULL;

    if(!*hash_table_off) {
        *hash_table_off = header->dwOffsetFirstHashTable;
        *hash_table_entry = 0;

        hashtable_entry = URLCache_HashEntryFromOffset(header, *hash_table_off);
    }else {
        if(*hash_table_off >= header->dwFileSize) {
            *hash_table_off = 0;
            return FALSE;
        }

        hashtable_entry = URLCache_HashEntryFromOffset(header, *hash_table_off);
    }

    if(hashtable_entry->CacheFileEntry.signature != HASH_SIGNATURE) {
        *hash_table_off = 0;
        return FALSE;
    }

    while(1) {
        if(*hash_table_entry >= HASHTABLE_SIZE) {
            *hash_table_off = hashtable_entry->dwAddressNext;
            if(!*hash_table_off) {
                *hash_table_off = 0;
                return FALSE;
            }

            hashtable_entry = URLCache_HashEntryFromOffset(header, *hash_table_off);
            *hash_table_entry = 0;
        }

        if(hashtable_entry->HashTable[*hash_table_entry].dwHashKey != HASHTABLE_DEL &&
            hashtable_entry->HashTable[*hash_table_entry].dwHashKey != HASHTABLE_FREE) {
            *hash_entry = &hashtable_entry->HashTable[*hash_table_entry];
            *entry = (entry_header*)((LPBYTE)header + hashtable_entry->HashTable[*hash_table_entry].dwOffsetEntry);
            (*hash_table_entry)++;
            return TRUE;
        }

        (*hash_table_entry)++;
    }

    *hash_table_off = 0;
    return FALSE;
}

/* Rates an urlcache entry to determine if it can be deleted.
 *
 * Score 0 means that entry can safely be removed, the bigger rating
 * the smaller chance of entry being removed.
 * DWORD_MAX means that entry can't be deleted at all.
 *
 * Rating system is currently not fully compatible with native implementation.
 */
static DWORD urlcache_rate_entry(entry_url *url_entry, FILETIME *cur_time)
{
    ULARGE_INTEGER time, access_time;
    DWORD rating;

    access_time.u.LowPart = url_entry->access_time.dwLowDateTime;
    access_time.u.HighPart = url_entry->access_time.dwHighDateTime;

    time.u.LowPart = cur_time->dwLowDateTime;
    time.u.HighPart = cur_time->dwHighDateTime;

    /* Don't touch entries that were added less than 10 minutes ago */
    if(time.QuadPart < access_time.QuadPart + (ULONGLONG)10*60*FILETIME_SECOND)
        return -1;

    if(url_entry->cache_entry_type & STICKY_CACHE_ENTRY)
        if(time.QuadPart < access_time.QuadPart + (ULONGLONG)url_entry->exempt_delta*FILETIME_SECOND)
            return -1;

    time.QuadPart = (time.QuadPart-access_time.QuadPart)/FILETIME_SECOND;
    rating = 400*60*60*24/(60*60*24+time.QuadPart);

    if(url_entry->hit_rate > 100)
        rating += 100;
    else
        rating += url_entry->hit_rate;

    return rating;
}

static int dword_cmp(const void *p1, const void *p2)
{
    return *(const DWORD*)p1 - *(const DWORD*)p2;
}

/***********************************************************************
 *           FreeUrlCacheSpaceW (WININET.@)
 *
 * Frees up some cache.
 *
 * PARAMETERS
 *   cache_path    [I] Which volume to free up from, or NULL if you don't care.
 *   size          [I] Percentage of the cache that should be free.
 *   filter        [I] Which entries can't be deleted (CacheEntryType)
 *
 * RETURNS
 *   TRUE success. FALSE failure.
 *
 * IMPLEMENTATION
 *   This implementation just retrieves the path of the cache directory, and
 *   deletes its contents from the filesystem. The correct approach would
 *   probably be to implement and use {FindFirst,FindNext,Delete}UrlCacheGroup().
 */
BOOL WINAPI FreeUrlCacheSpaceW(LPCWSTR cache_path, DWORD size, DWORD filter)
{
    URLCACHECONTAINER *container;
    DWORD path_len, err;

    TRACE("(%s, %x, %x)\n", debugstr_w(cache_path), size, filter);

    if(size<1 || size>100) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(cache_path) {
        path_len = strlenW(cache_path);
        if(cache_path[path_len-1] == '\\')
            path_len--;
    }else {
        path_len = 0;
    }

    if(size==100 && !filter) {
        LIST_FOR_EACH_ENTRY(container, &UrlContainers, URLCACHECONTAINER, entry)
        {
            /* When cache_path==NULL only clean Temporary Internet Files */
            if((!path_len && container->cache_prefix[0]==0) ||
                    (path_len && !strncmpiW(container->path, cache_path, path_len) &&
                     (container->path[path_len]=='\0' || container->path[path_len]=='\\')))
            {
                BOOL ret_del;

                WaitForSingleObject(container->hMutex, INFINITE);

                /* unlock, delete, recreate and lock cache */
                cache_container_close_index(container);
                ret_del = URLCache_DeleteCacheDirectory(container->path);
                err = cache_container_open_index(container, MIN_BLOCK_NO);

                ReleaseMutex(container->hMutex);
                if(!ret_del || (err != ERROR_SUCCESS))
                    return FALSE;
            }
        }

        return TRUE;
    }

    LIST_FOR_EACH_ENTRY(container, &UrlContainers, URLCACHECONTAINER, entry)
    {
        URLCACHE_HEADER *header;
        struct _HASH_ENTRY *hash_entry;
        entry_header *entry;
        entry_url *url_entry;
        ULONGLONG desired_size, cur_size;
        DWORD delete_factor, hash_table_off, hash_table_entry;
        DWORD rate[100], rate_no;
        FILETIME cur_time;

        if((path_len || container->cache_prefix[0]!=0) &&
                (!path_len || strncmpiW(container->path, cache_path, path_len) ||
                 (container->path[path_len]!='\0' && container->path[path_len]!='\\')))
            continue;

        err = cache_container_open_index(container, MIN_BLOCK_NO);
        if(err != ERROR_SUCCESS)
            continue;

        header = cache_container_lock_index(container);
        if(!header)
            continue;

        urlcache_clean_leaked_entries(container, header);

        desired_size = header->CacheLimit.QuadPart*(100-size)/100;
        cur_size = header->CacheUsage.QuadPart+header->ExemptUsage.QuadPart;
        if(cur_size <= desired_size)
            delete_factor = 0;
        else
            delete_factor = (cur_size-desired_size)*100/cur_size;

        if(!delete_factor) {
            cache_container_unlock_index(container, header);
            continue;
        }

        hash_table_off = 0;
        hash_table_entry = 0;
        rate_no = 0;
        GetSystemTimeAsFileTime(&cur_time);
        while(rate_no<sizeof(rate)/sizeof(*rate) &&
                urlcache_next_entry(header, &hash_table_off, &hash_table_entry, &hash_entry, &entry)) {
            if(entry->signature != URL_SIGNATURE) {
                WARN("only url entries are currently supported\n");
                continue;
            }

            url_entry = (entry_url*)entry;
            if(url_entry->cache_entry_type & filter)
                continue;

            rate[rate_no] = urlcache_rate_entry(url_entry, &cur_time);
            if(rate[rate_no] != -1)
                rate_no++;
        }

        if(!rate_no) {
            TRACE("nothing to delete\n");
            cache_container_unlock_index(container, header);
            continue;
        }

        qsort(rate, rate_no, sizeof(DWORD), dword_cmp);

        delete_factor = delete_factor*rate_no/100;
        delete_factor = rate[delete_factor];
        TRACE("deleting files with rating %d or less\n", delete_factor);

        hash_table_off = 0;
        while(urlcache_next_entry(header, &hash_table_off, &hash_table_entry, &hash_entry, &entry)) {
            if(entry->signature != URL_SIGNATURE)
                continue;

            url_entry = (entry_url*)entry;
            if(url_entry->cache_entry_type & filter)
                continue;

            if(urlcache_rate_entry(url_entry, &cur_time) <= delete_factor) {
                TRACE("deleting file: %s\n", debugstr_a((char*)url_entry+url_entry->local_name_off));
                DeleteUrlCacheEntryInternal(container, header, hash_entry);

                if(header->CacheUsage.QuadPart+header->ExemptUsage.QuadPart <= desired_size)
                    break;

                /* Allow other threads to use cache while cleaning */
                cache_container_unlock_index(container, header);
                if(WaitForSingleObject(dll_unload_event, 0) == WAIT_OBJECT_0) {
                    TRACE("got dll_unload_event - finishing\n");
                    return TRUE;
                }
                Sleep(0);
                header = cache_container_lock_index(container);
            }
        }

        TRACE("cache size after cleaning 0x%s/0x%s\n",
                wine_dbgstr_longlong(header->CacheUsage.QuadPart+header->ExemptUsage.QuadPart),
                wine_dbgstr_longlong(header->CacheLimit.QuadPart));
        cache_container_unlock_index(container, header);
    }

    return TRUE;
}

/***********************************************************************
 *           FreeUrlCacheSpaceA (WININET.@)
 *
 * See FreeUrlCacheSpaceW.
 */
BOOL WINAPI FreeUrlCacheSpaceA(LPCSTR lpszCachePath, DWORD dwSize, DWORD dwFilter)
{
    BOOL ret = FALSE;
    LPWSTR path = heap_strdupAtoW(lpszCachePath);
    if (lpszCachePath == NULL || path != NULL)
        ret = FreeUrlCacheSpaceW(path, dwSize, dwFilter);
    heap_free(path);
    return ret;
}

/***********************************************************************
 *           UnlockUrlCacheEntryFileA (WININET.@)
 *
 */
BOOL WINAPI UnlockUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName, 
    IN DWORD dwReserved
    )
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    entry_header *pEntry;
    entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, 0x%08x)\n", debugstr_a(lpszUrlName), dwReserved);

    if (dwReserved)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    error = URLCacheContainers_FindContainerA(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
       SetLastError(error);
       return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHash(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", lpszUrlName);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (entry_url *)pEntry;

    if (pUrlEntry->use_count == 0)
    {
        cache_container_unlock_index(pContainer, pHeader);
        return FALSE;
    }
    pUrlEntry->use_count--;
    if (!pUrlEntry->use_count)
    {
        URLCache_HashEntrySetFlags(pHashEntry, HASHTABLE_URL);
        if (pUrlEntry->cache_entry_type & PENDING_DELETE_CACHE_ENTRY)
            DeleteUrlCacheEntryInternal(pContainer, pHeader, pHashEntry);
    }

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           UnlockUrlCacheEntryFileW (WININET.@)
 *
 */
BOOL WINAPI UnlockUrlCacheEntryFileW( LPCWSTR lpszUrlName, DWORD dwReserved )
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    entry_header *pEntry;
    entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    DWORD error;

    TRACE("(%s, 0x%08x)\n", debugstr_w(lpszUrlName), dwReserved);

    if (dwReserved)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    error = URLCacheContainers_FindContainerW(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHashW(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", debugstr_w(lpszUrlName));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (entry_url *)pEntry;

    if (pUrlEntry->use_count == 0)
    {
        cache_container_unlock_index(pContainer, pHeader);
        return FALSE;
    }
    pUrlEntry->use_count--;
    if (!pUrlEntry->use_count)
        URLCache_HashEntrySetFlags(pHashEntry, HASHTABLE_URL);

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           CreateUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI CreateUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
)
{
    WCHAR *url_name;
    WCHAR *file_extension = NULL;
    WCHAR file_name[MAX_PATH];
    BOOL bSuccess = FALSE;
    DWORD dwError = 0;

    TRACE("(%s %d %s %p %d)\n", debugstr_a(lpszUrlName), dwExpectedFileSize,
            debugstr_a(lpszFileExtension), lpszFileName, dwReserved);

    if (lpszUrlName && (url_name = heap_strdupAtoW(lpszUrlName)))
    {
        if (!lpszFileExtension || (file_extension = heap_strdupAtoW(lpszFileExtension)))
        {
            if (CreateUrlCacheEntryW(url_name, dwExpectedFileSize, file_extension, file_name, dwReserved))
            {
                if (WideCharToMultiByte(CP_ACP, 0, file_name, -1, lpszFileName, MAX_PATH, NULL, NULL) < MAX_PATH)
                {
                    bSuccess = TRUE;
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            else
            {
                dwError = GetLastError();
            }
            heap_free(file_extension);
        }
        else
        {
            dwError = GetLastError();
        }
        heap_free(url_name);
        if (!bSuccess) SetLastError(dwError);
    }
    return bSuccess;
}
/***********************************************************************
 *           CreateUrlCacheEntryW (WININET.@)
 *
 */
BOOL WINAPI CreateUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCWSTR lpszFileExtension,
    OUT LPWSTR lpszFileName,
    IN DWORD dwReserved
)
{
    URLCACHECONTAINER * pContainer;
    LPURLCACHE_HEADER pHeader;
    CHAR szFile[MAX_PATH];
    WCHAR szExtension[MAX_PATH];
    LPCWSTR lpszUrlPart;
    LPCWSTR lpszUrlEnd;
    LPCWSTR lpszFileNameExtension;
    LPWSTR lpszFileNameNoPath;
    int i;
    int countnoextension;
    BYTE CacheDir;
    LONG lBufferSize;
    BOOL bFound = FALSE;
    BOOL generate_name = FALSE;
    int count;
    DWORD error;
    HANDLE hFile;
    FILETIME ft;

    static const WCHAR szWWW[] = {'w','w','w',0};

    TRACE("(%s, 0x%08x, %s, %p, 0x%08x)\n",
        debugstr_w(lpszUrlName),
        dwExpectedFileSize,
        debugstr_w(lpszFileExtension),
        lpszFileName,
        dwReserved);

    if (dwReserved)
        FIXME("dwReserved 0x%08x\n", dwReserved);

    lpszUrlEnd = lpszUrlName + strlenW(lpszUrlName);
    
    if (((lpszUrlEnd - lpszUrlName) > 1) && (*(lpszUrlEnd - 1) == '/' || *(lpszUrlEnd - 1) == '\\'))
        lpszUrlEnd--;

    lpszUrlPart = memchrW(lpszUrlName, '?', lpszUrlEnd - lpszUrlName);
    if (!lpszUrlPart)
        lpszUrlPart = memchrW(lpszUrlName, '#', lpszUrlEnd - lpszUrlName);
    if (lpszUrlPart)
        lpszUrlEnd = lpszUrlPart;

    for (lpszUrlPart = lpszUrlEnd; 
        (lpszUrlPart >= lpszUrlName); 
        lpszUrlPart--)
    {
        if ((*lpszUrlPart == '/' || *lpszUrlPart == '\\') && ((lpszUrlEnd - lpszUrlPart) > 1))
        {
            bFound = TRUE;
            lpszUrlPart++;
            break;
        }
    }
    if(!bFound)
        lpszUrlPart++;

    if (!lstrcmpW(lpszUrlPart, szWWW))
    {
        lpszUrlPart += lstrlenW(szWWW);
    }

    count = lpszUrlEnd - lpszUrlPart;

    if (bFound && (count < MAX_PATH))
    {
	int len = WideCharToMultiByte(CP_ACP, 0, lpszUrlPart, count, szFile, sizeof(szFile) - 1, NULL, NULL);
	if (!len)
	    return FALSE;
        szFile[len] = '\0';
        while(len && szFile[--len] == '/') szFile[len] = '\0';

        /* FIXME: get rid of illegal characters like \, / and : */
        TRACE("File name: %s\n", debugstr_a(szFile));
    }
    else
    {
        generate_name = TRUE;
        szFile[0] = 0;
    }

    error = URLCacheContainers_FindContainerW(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if(pHeader->DirectoryCount)
        CacheDir = (BYTE)(rand() % pHeader->DirectoryCount);
    else
        CacheDir = CACHE_CONTAINER_NO_SUBDIR;

    lBufferSize = MAX_PATH * sizeof(WCHAR);
    if (!URLCache_LocalFileNameToPathW(pContainer, pHeader, szFile, CacheDir, lpszFileName, &lBufferSize))
    {
        WARN("Failed to get full path for filename %s, needed %u bytes.\n",
                debugstr_a(szFile), lBufferSize);
        cache_container_unlock_index(pContainer, pHeader);
        return FALSE;
    }

    cache_container_unlock_index(pContainer, pHeader);

    for (lpszFileNameNoPath = lpszFileName + lBufferSize / sizeof(WCHAR) - 2;
        lpszFileNameNoPath >= lpszFileName; 
        --lpszFileNameNoPath)
    {
        if (*lpszFileNameNoPath == '/' || *lpszFileNameNoPath == '\\')
            break;
    }

    countnoextension = lstrlenW(lpszFileNameNoPath);
    lpszFileNameExtension = PathFindExtensionW(lpszFileNameNoPath);
    if (lpszFileNameExtension)
        countnoextension -= lstrlenW(lpszFileNameExtension);
    *szExtension = '\0';

    if (lpszFileExtension)
    {
        szExtension[0] = '.';
        lstrcpyW(szExtension+1, lpszFileExtension);
    }

    for (i = 0; i<255 && !generate_name; i++)
    {
        static const WCHAR szFormat[] = {'[','%','u',']','%','s',0};
        WCHAR *p;

        wsprintfW(lpszFileNameNoPath + countnoextension, szFormat, i, szExtension);
        for (p = lpszFileNameNoPath + 1; *p; p++)
        {
            switch (*p)
            {
            case '<': case '>':
            case ':': case '"':
            case '/': case '\\':
            case '|': case '?':
            case '*':
                *p = '_'; break;
            default: break;
            }
        }
        if (p[-1] == ' ' || p[-1] == '.') p[-1] = '_';

        TRACE("Trying: %s\n", debugstr_w(lpszFileName));
        hFile = CreateFileW(lpszFileName, GENERIC_READ, 0, NULL, CREATE_NEW, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            return TRUE;
        }
    }

    /* Try to generate random name */
    GetSystemTimeAsFileTime(&ft);
    strcpyW(lpszFileNameNoPath+countnoextension+8, szExtension);

    for(i=0; i<255; i++)
    {
        int j;
        ULONGLONG n = ft.dwHighDateTime;
        n <<= 32;
        n += ft.dwLowDateTime;
        n ^= (ULONGLONG)i<<48;

        for(j=0; j<8; j++)
        {
            int r = (n % 36);
            n /= 37;
            lpszFileNameNoPath[countnoextension+j] = (r < 10 ? '0' + r : 'A' + r - 10);
        }

        TRACE("Trying: %s\n", debugstr_w(lpszFileName));
        hFile = CreateFileW(lpszFileName, GENERIC_READ, 0, NULL, CREATE_NEW, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            return TRUE;
        }
    }

    WARN("Could not find a unique filename\n");
    return FALSE;
}

/***********************************************************************
 *           CommitUrlCacheEntryInternal (Compensates for an MS bug)
 *
 *   The bug we are compensating for is that some drongo at Microsoft
 *   used lpHeaderInfo to pass binary data to CommitUrlCacheEntryA.
 *   As a consequence, CommitUrlCacheEntryA has been effectively
 *   redefined as LPBYTE rather than LPCSTR. But CommitUrlCacheEntryW
 *   is still defined as LPCWSTR. The result (other than madness) is
 *   that we always need to store lpHeaderInfo in CP_ACP rather than
 *   in UTF16, and we need to avoid converting lpHeaderInfo in
 *   CommitUrlCacheEntryA to UTF16 and then back to CP_ACP, since the
 *   result will lose data for arbitrary binary data.
 *
 */
static BOOL CommitUrlCacheEntryInternal(
    IN LPCWSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCWSTR lpszFileExtension,
    IN LPCWSTR lpszOriginalUrl
    )
{
    URLCACHECONTAINER * pContainer;
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    entry_header *pEntry;
    entry_url * pUrlEntry;
    DWORD url_entry_offset;
    DWORD dwBytesNeeded = DWORD_ALIGN(sizeof(*pUrlEntry));
    DWORD dwOffsetLocalFileName = 0;
    DWORD dwOffsetHeader = 0;
    DWORD dwOffsetFileExtension = 0;
    WIN32_FILE_ATTRIBUTE_DATA file_attr;
    LARGE_INTEGER file_size;
    BYTE cDirectory;
    char achFile[MAX_PATH];
    LPSTR lpszUrlNameA = NULL;
    LPSTR lpszFileExtensionA = NULL;
    char *pchLocalFileName = 0;
    DWORD hit_rate = 0;
    DWORD exempt_delta = 0;
    DWORD error;

    TRACE("(%s, %s, ..., ..., %x, %p, %d, %s, %s)\n",
        debugstr_w(lpszUrlName),
        debugstr_w(lpszLocalFileName),
        CacheEntryType,
        lpHeaderInfo,
        dwHeaderSize,
        debugstr_w(lpszFileExtension),
        debugstr_w(lpszOriginalUrl));

    if (CacheEntryType & STICKY_CACHE_ENTRY && !lpszLocalFileName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (lpszOriginalUrl)
        WARN(": lpszOriginalUrl ignored\n");

    memset(&file_attr, 0, sizeof(file_attr));
    if (lpszLocalFileName)
    {
        if(!GetFileAttributesExW(lpszLocalFileName, GetFileExInfoStandard, &file_attr))
            return FALSE;
    }
    file_size.u.LowPart = file_attr.nFileSizeLow;
    file_size.u.HighPart = file_attr.nFileSizeHigh;

    error = URLCacheContainers_FindContainerW(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    lpszUrlNameA = heap_strdupWtoA(lpszUrlName);
    if (!lpszUrlNameA)
    {
        error = GetLastError();
        goto cleanup;
    }

    if (lpszFileExtension && !(lpszFileExtensionA = heap_strdupWtoA(lpszFileExtension)))
    {
        error = GetLastError();
        goto cleanup;
    }

    if (URLCache_FindHash(pHeader, lpszUrlNameA, &pHashEntry))
    {
        entry_url *pUrlEntry = (entry_url*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
        if (URLCache_IsLocked(pHashEntry, pUrlEntry))
        {
            TRACE("Trying to overwrite locked entry\n");
            error = ERROR_SHARING_VIOLATION;
            goto cleanup;
        }

        hit_rate = pUrlEntry->hit_rate;
        exempt_delta = pUrlEntry->exempt_delta;
        DeleteUrlCacheEntryInternal(pContainer, pHeader, pHashEntry);
    }

    if (pHeader->DirectoryCount)
        cDirectory = 0;
    else
        cDirectory = CACHE_CONTAINER_NO_SUBDIR;

    if (lpszLocalFileName)
    {
        BOOL bFound = FALSE;

        if (strncmpW(lpszLocalFileName, pContainer->path, lstrlenW(pContainer->path)))
        {
            ERR("path %s must begin with cache content path %s\n", debugstr_w(lpszLocalFileName), debugstr_w(pContainer->path));
            error = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        /* skip container path prefix */
        lpszLocalFileName += lstrlenW(pContainer->path);

        WideCharToMultiByte(CP_ACP, 0, lpszLocalFileName, -1, achFile, MAX_PATH, NULL, NULL);
	pchLocalFileName = achFile;

        if(pHeader->DirectoryCount)
        {
            for (cDirectory = 0; cDirectory < pHeader->DirectoryCount; cDirectory++)
            {
                if (!strncmp(pHeader->directory_data[cDirectory].filename, pchLocalFileName, DIR_LENGTH))
                {
                    bFound = TRUE;
                    break;
                }
            }

            if (!bFound)
            {
                ERR("cache directory not found in path %s\n", debugstr_w(lpszLocalFileName));
                error = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            lpszLocalFileName += DIR_LENGTH + 1;
            pchLocalFileName += DIR_LENGTH + 1;
        }
    }

    dwBytesNeeded = DWORD_ALIGN(dwBytesNeeded + strlen(lpszUrlNameA) + 1);
    if (lpszLocalFileName)
    {
        dwOffsetLocalFileName = dwBytesNeeded;
        dwBytesNeeded = DWORD_ALIGN(dwBytesNeeded + strlen(pchLocalFileName) + 1);
    }
    if (lpHeaderInfo)
    {
        dwOffsetHeader = dwBytesNeeded;
        dwBytesNeeded = DWORD_ALIGN(dwBytesNeeded + dwHeaderSize);
    }
    if (lpszFileExtensionA)
    {
        dwOffsetFileExtension = dwBytesNeeded;
        dwBytesNeeded = DWORD_ALIGN(dwBytesNeeded + strlen(lpszFileExtensionA) + 1);
    }

    /* round up to next block */
    if (dwBytesNeeded % BLOCKSIZE)
    {
        dwBytesNeeded -= dwBytesNeeded % BLOCKSIZE;
        dwBytesNeeded += BLOCKSIZE;
    }

    error = URLCache_FindFirstFreeEntry(pHeader, dwBytesNeeded / BLOCKSIZE, &pEntry);
    while (error == ERROR_HANDLE_DISK_FULL)
    {
        error = cache_container_clean_index(pContainer, &pHeader);
        if (error == ERROR_SUCCESS)
            error = URLCache_FindFirstFreeEntry(pHeader, dwBytesNeeded / BLOCKSIZE, &pEntry);
    }
    if (error != ERROR_SUCCESS)
        goto cleanup;

    /* FindFirstFreeEntry fills in blocks used */
    pUrlEntry = (entry_url *)pEntry;
    url_entry_offset = (LPBYTE)pUrlEntry - (LPBYTE)pHeader;
    pUrlEntry->header.signature = URL_SIGNATURE;
    pUrlEntry->cache_dir = cDirectory;
    pUrlEntry->cache_entry_type = CacheEntryType | pContainer->default_entry_type;
    pUrlEntry->header_info_size = dwHeaderSize;
    if ((CacheEntryType & STICKY_CACHE_ENTRY) && !exempt_delta)
    {
        /* Sticky entries have a default exempt time of one day */
        exempt_delta = 86400;
    }
    pUrlEntry->exempt_delta = exempt_delta;
    pUrlEntry->hit_rate = hit_rate+1;
    pUrlEntry->file_extension_off = dwOffsetFileExtension;
    pUrlEntry->header_info_off = dwOffsetHeader;
    pUrlEntry->local_name_off = dwOffsetLocalFileName;
    pUrlEntry->url_off = DWORD_ALIGN(sizeof(*pUrlEntry));
    pUrlEntry->size.QuadPart = file_size.QuadPart;
    pUrlEntry->use_count = 0;
    GetSystemTimeAsFileTime(&pUrlEntry->access_time);
    pUrlEntry->modification_time = LastModifiedTime;
    URLCache_FileTimeToDosDateTime(&pUrlEntry->access_time, &pUrlEntry->sync_date, &pUrlEntry->sync_time);
    URLCache_FileTimeToDosDateTime(&ExpireTime, &pUrlEntry->expire_date, &pUrlEntry->expire_time);
    URLCache_FileTimeToDosDateTime(&file_attr.ftLastWriteTime, &pUrlEntry->write_date, &pUrlEntry->write_time);

    /*** Unknowns ***/
    pUrlEntry->unk1 = 0;
    pUrlEntry->unk2 = 0;
    pUrlEntry->unk3 = 0x60;
    pUrlEntry->unk4 = 0;
    pUrlEntry->unk5 = 0x1010;
    pUrlEntry->unk7 = 0;
    pUrlEntry->unk8 = 0;


    strcpy((LPSTR)pUrlEntry + pUrlEntry->url_off, lpszUrlNameA);
    if (dwOffsetLocalFileName)
        strcpy((LPSTR)((LPBYTE)pUrlEntry + dwOffsetLocalFileName), pchLocalFileName);
    if (dwOffsetHeader)
        memcpy((LPBYTE)pUrlEntry + dwOffsetHeader, lpHeaderInfo, dwHeaderSize);
    if (dwOffsetFileExtension)
        strcpy((LPSTR)((LPBYTE)pUrlEntry + dwOffsetFileExtension), lpszFileExtensionA);

    error = URLCache_AddEntryToHash(pHeader, lpszUrlNameA, url_entry_offset, HASHTABLE_URL);
    while (error == ERROR_HANDLE_DISK_FULL)
    {
        error = cache_container_clean_index(pContainer, &pHeader);
        if (error == ERROR_SUCCESS)
        {
            pUrlEntry = (entry_url *)((LPBYTE)pHeader + url_entry_offset);
            error = URLCache_AddEntryToHash(pHeader, lpszUrlNameA,
                    url_entry_offset, HASHTABLE_URL);
        }
    }
    if (error != ERROR_SUCCESS)
        URLCache_DeleteEntry(pHeader, &pUrlEntry->header);
    else
    {
        if (pUrlEntry->cache_dir < pHeader->DirectoryCount)
            pHeader->directory_data[pUrlEntry->cache_dir].dwNumFiles++;
        if (CacheEntryType & STICKY_CACHE_ENTRY)
            pHeader->ExemptUsage.QuadPart += file_size.QuadPart;
        else
            pHeader->CacheUsage.QuadPart += file_size.QuadPart;
        if (pHeader->CacheUsage.QuadPart + pHeader->ExemptUsage.QuadPart >
            pHeader->CacheLimit.QuadPart)
            handle_full_cache();
    }

cleanup:
    cache_container_unlock_index(pContainer, pHeader);
    heap_free(lpszUrlNameA);
    heap_free(lpszFileExtensionA);

    if (error == ERROR_SUCCESS)
        return TRUE;
    else
    {
        SetLastError(error);
        return FALSE;
    }
}

/***********************************************************************
 *           CommitUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR lpszOriginalUrl
    )
{
    WCHAR *url_name = NULL;
    WCHAR *local_file_name = NULL;
    WCHAR *original_url = NULL;
    WCHAR *file_extension = NULL;
    BOOL bSuccess = FALSE;

    TRACE("(%s, %s, ..., ..., %x, %p, %d, %s, %s)\n",
        debugstr_a(lpszUrlName),
        debugstr_a(lpszLocalFileName),
        CacheEntryType,
        lpHeaderInfo,
        dwHeaderSize,
        debugstr_a(lpszFileExtension),
        debugstr_a(lpszOriginalUrl));

    url_name = heap_strdupAtoW(lpszUrlName);
    if (!url_name)
        goto cleanup;

    if (lpszLocalFileName)
    {
        local_file_name = heap_strdupAtoW(lpszLocalFileName);
        if (!local_file_name)
            goto cleanup;
    }
    if (lpszFileExtension)
    {
        file_extension = heap_strdupAtoW(lpszFileExtension);
        if (!file_extension)
            goto cleanup;
    }
    if (lpszOriginalUrl)
    {
        original_url = heap_strdupAtoW(lpszOriginalUrl);
        if (!original_url)
            goto cleanup;
    }

    bSuccess = CommitUrlCacheEntryInternal(url_name, local_file_name, ExpireTime, LastModifiedTime,
                                           CacheEntryType, lpHeaderInfo, dwHeaderSize,
                                           file_extension, original_url);

cleanup:
    heap_free(original_url);
    heap_free(file_extension);
    heap_free(local_file_name);
    heap_free(url_name);
    return bSuccess;
}

/***********************************************************************
 *           CommitUrlCacheEntryW (WININET.@)
 *
 */
BOOL WINAPI CommitUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPWSTR lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCWSTR lpszFileExtension,
    IN LPCWSTR lpszOriginalUrl
    )
{
    DWORD dwError = 0;
    BOOL bSuccess = FALSE;
    DWORD len = 0;
    CHAR *header_info = NULL;

    TRACE("(%s, %s, ..., ..., %x, %p, %d, %s, %s)\n",
        debugstr_w(lpszUrlName),
        debugstr_w(lpszLocalFileName),
        CacheEntryType,
        lpHeaderInfo,
        dwHeaderSize,
        debugstr_w(lpszFileExtension),
        debugstr_w(lpszOriginalUrl));

    if (!lpHeaderInfo || (header_info = heap_strdupWtoA(lpHeaderInfo)))
    {
        if(header_info)
            len = strlen(header_info);
	if (CommitUrlCacheEntryInternal(lpszUrlName, lpszLocalFileName, ExpireTime, LastModifiedTime,
				CacheEntryType, (LPBYTE)header_info, len, lpszFileExtension, lpszOriginalUrl))
	{
		bSuccess = TRUE;
	}
	else
	{
		dwError = GetLastError();
	}
	if (header_info)
	{
	    heap_free(header_info);
	    if (!bSuccess)
		SetLastError(dwError);
	}
    }
    return bSuccess;
}

/***********************************************************************
 *           ReadUrlCacheEntryStream (WININET.@)
 *
 */
BOOL WINAPI ReadUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN  DWORD dwLocation,
    IN OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwLen,
    IN DWORD dwReserved
    )
{
    /* Get handle to file from 'stream' */
    STREAM_HANDLE * pStream = (STREAM_HANDLE *)hUrlCacheStream;

    if (dwReserved != 0)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IsBadReadPtr(pStream, sizeof(*pStream)) || IsBadStringPtrA(pStream->lpszUrl, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (SetFilePointer(pStream->hFile, dwLocation, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        return FALSE;
    return ReadFile(pStream->hFile, lpBuffer, *lpdwLen, lpdwLen, NULL);
}

/***********************************************************************
 *           RetrieveUrlCacheEntryStreamA (WININET.@)
 *
 */
HANDLE WINAPI RetrieveUrlCacheEntryStreamA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    )
{
    /* NOTE: this is not the same as the way that the native
     * version allocates 'stream' handles. I did it this way
     * as it is much easier and no applications should depend
     * on this behaviour. (Native version appears to allocate
     * indices into a table)
     */
    STREAM_HANDLE * pStream;
    HANDLE hFile;

    TRACE( "(%s, %p, %p, %x, 0x%08x)\n", debugstr_a(lpszUrlName), lpCacheEntryInfo,
           lpdwCacheEntryInfoBufferSize, fRandomRead, dwReserved );

    if (!RetrieveUrlCacheEntryFileA(lpszUrlName,
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufferSize,
        dwReserved))
    {
        return NULL;
    }

    hFile = CreateFileA(lpCacheEntryInfo->lpszLocalFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        fRandomRead ? FILE_FLAG_RANDOM_ACCESS : 0,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    
    /* allocate handle storage space */
    pStream = heap_alloc(sizeof(STREAM_HANDLE) + strlen(lpszUrlName) * sizeof(CHAR));
    if (!pStream)
    {
        CloseHandle(hFile);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    pStream->hFile = hFile;
    strcpy(pStream->lpszUrl, lpszUrlName);
    return pStream;
}

/***********************************************************************
 *           RetrieveUrlCacheEntryStreamW (WININET.@)
 *
 */
HANDLE WINAPI RetrieveUrlCacheEntryStreamW(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    )
{
    DWORD size;
    int url_len;
    /* NOTE: this is not the same as the way that the native
     * version allocates 'stream' handles. I did it this way
     * as it is much easier and no applications should depend
     * on this behaviour. (Native version appears to allocate
     * indices into a table)
     */
    STREAM_HANDLE * pStream;
    HANDLE hFile;

    TRACE( "(%s, %p, %p, %x, 0x%08x)\n", debugstr_w(lpszUrlName), lpCacheEntryInfo,
           lpdwCacheEntryInfoBufferSize, fRandomRead, dwReserved );

    if (!RetrieveUrlCacheEntryFileW(lpszUrlName,
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufferSize,
        dwReserved))
    {
        return NULL;
    }

    hFile = CreateFileW(lpCacheEntryInfo->lpszLocalFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        fRandomRead ? FILE_FLAG_RANDOM_ACCESS : 0,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    /* allocate handle storage space */
    size = sizeof(STREAM_HANDLE);
    url_len = WideCharToMultiByte(CP_ACP, 0, lpszUrlName, -1, NULL, 0, NULL, NULL);
    size += url_len;
    pStream = heap_alloc(size);
    if (!pStream)
    {
        CloseHandle(hFile);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    pStream->hFile = hFile;
    WideCharToMultiByte(CP_ACP, 0, lpszUrlName, -1, pStream->lpszUrl, url_len, NULL, NULL);
    return pStream;
}

/***********************************************************************
 *           UnlockUrlCacheEntryStream (WININET.@)
 *
 */
BOOL WINAPI UnlockUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD dwReserved
)
{
    STREAM_HANDLE * pStream = (STREAM_HANDLE *)hUrlCacheStream;

    if (dwReserved != 0)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IsBadReadPtr(pStream, sizeof(*pStream)) || IsBadStringPtrA(pStream->lpszUrl, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!UnlockUrlCacheEntryFileA(pStream->lpszUrl, 0))
        return FALSE;

    CloseHandle(pStream->hFile);
    heap_free(pStream);
    return TRUE;
}


/***********************************************************************
 *           DeleteUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI DeleteUrlCacheEntryA(LPCSTR lpszUrlName)
{
    URLCACHECONTAINER * pContainer;
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    DWORD error;
    BOOL ret;

    TRACE("(%s)\n", debugstr_a(lpszUrlName));

    error = URLCacheContainers_FindContainerA(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
        return FALSE;

    if (!URLCache_FindHash(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", lpszUrlName);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    ret = DeleteUrlCacheEntryInternal(pContainer, pHeader, pHashEntry);

    cache_container_unlock_index(pContainer, pHeader);

    return ret;
}

/***********************************************************************
 *           DeleteUrlCacheEntryW (WININET.@)
 *
 */
BOOL WINAPI DeleteUrlCacheEntryW(LPCWSTR lpszUrlName)
{
    URLCACHECONTAINER * pContainer;
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    LPSTR urlA;
    DWORD error;
    BOOL ret;

    TRACE("(%s)\n", debugstr_w(lpszUrlName));

    urlA = heap_strdupWtoA(lpszUrlName);
    if (!urlA)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    error = URLCacheContainers_FindContainerW(lpszUrlName, &pContainer);
    if (error != ERROR_SUCCESS)
    {
        heap_free(urlA);
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS)
    {
        heap_free(urlA);
        SetLastError(error);
        return FALSE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
    {
        heap_free(urlA);
        return FALSE;
    }

    if (!URLCache_FindHash(pHeader, urlA, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", debugstr_a(urlA));
        heap_free(urlA);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    ret = DeleteUrlCacheEntryInternal(pContainer, pHeader, pHashEntry);

    cache_container_unlock_index(pContainer, pHeader);
    heap_free(urlA);
    return ret;
}

BOOL WINAPI DeleteUrlCacheContainerA(DWORD d1, DWORD d2)
{
    FIXME("(0x%08x, 0x%08x) stub\n", d1, d2);
    return TRUE;
}

BOOL WINAPI DeleteUrlCacheContainerW(DWORD d1, DWORD d2)
{
    FIXME("(0x%08x, 0x%08x) stub\n", d1, d2);
    return TRUE;
}

/***********************************************************************
 *           CreateCacheContainerA (WININET.@)
 */
BOOL WINAPI CreateUrlCacheContainerA(DWORD d1, DWORD d2, DWORD d3, DWORD d4,
                                     DWORD d5, DWORD d6, DWORD d7, DWORD d8)
{
    FIXME("(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x) stub\n",
          d1, d2, d3, d4, d5, d6, d7, d8);
    return TRUE;
}

/***********************************************************************
 *           CreateCacheContainerW (WININET.@)
 */
BOOL WINAPI CreateUrlCacheContainerW(DWORD d1, DWORD d2, DWORD d3, DWORD d4,
                                     DWORD d5, DWORD d6, DWORD d7, DWORD d8)
{
    FIXME("(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x) stub\n",
          d1, d2, d3, d4, d5, d6, d7, d8);
    return TRUE;
}

/***********************************************************************
 *           FindFirstUrlCacheContainerA (WININET.@)
 */
HANDLE WINAPI FindFirstUrlCacheContainerA( LPVOID p1, LPVOID p2, LPVOID p3, DWORD d1 )
{
    FIXME("(%p, %p, %p, 0x%08x) stub\n", p1, p2, p3, d1 );
    return NULL;
}

/***********************************************************************
 *           FindFirstUrlCacheContainerW (WININET.@)
 */
HANDLE WINAPI FindFirstUrlCacheContainerW( LPVOID p1, LPVOID p2, LPVOID p3, DWORD d1 )
{
    FIXME("(%p, %p, %p, 0x%08x) stub\n", p1, p2, p3, d1 );
    return NULL;
}

/***********************************************************************
 *           FindNextUrlCacheContainerA (WININET.@)
 */
BOOL WINAPI FindNextUrlCacheContainerA( HANDLE handle, LPVOID p1, LPVOID p2 )
{
    FIXME("(%p, %p, %p) stub\n", handle, p1, p2 );
    return FALSE;
}

/***********************************************************************
 *           FindNextUrlCacheContainerW (WININET.@)
 */
BOOL WINAPI FindNextUrlCacheContainerW( HANDLE handle, LPVOID p1, LPVOID p2 )
{
    FIXME("(%p, %p, %p) stub\n", handle, p1, p2 );
    return FALSE;
}

HANDLE WINAPI FindFirstUrlCacheEntryExA(
  LPCSTR lpszUrlSearchPattern,
  DWORD dwFlags,
  DWORD dwFilter,
  GROUPID GroupId,
  LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
  LPDWORD lpdwFirstCacheEntryInfoBufferSize,
  LPVOID lpReserved,
  LPDWORD pcbReserved2,
  LPVOID lpReserved3
)
{
    FIXME("(%s, 0x%08x, 0x%08x, 0x%08x%08x, %p, %p, %p, %p, %p) stub\n", debugstr_a(lpszUrlSearchPattern),
          dwFlags, dwFilter, (ULONG)(GroupId >> 32), (ULONG)GroupId, lpFirstCacheEntryInfo,
          lpdwFirstCacheEntryInfoBufferSize, lpReserved, pcbReserved2,lpReserved3);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return NULL;
}

HANDLE WINAPI FindFirstUrlCacheEntryExW(
  LPCWSTR lpszUrlSearchPattern,
  DWORD dwFlags,
  DWORD dwFilter,
  GROUPID GroupId,
  LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
  LPDWORD lpdwFirstCacheEntryInfoBufferSize,
  LPVOID lpReserved,
  LPDWORD pcbReserved2,
  LPVOID lpReserved3
)
{
    FIXME("(%s, 0x%08x, 0x%08x, 0x%08x%08x, %p, %p, %p, %p, %p) stub\n", debugstr_w(lpszUrlSearchPattern),
          dwFlags, dwFilter, (ULONG)(GroupId >> 32), (ULONG)GroupId, lpFirstCacheEntryInfo,
          lpdwFirstCacheEntryInfoBufferSize, lpReserved, pcbReserved2,lpReserved3);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return NULL;
}

#define URLCACHE_FIND_ENTRY_HANDLE_MAGIC 0xF389ABCD

typedef struct URLCacheFindEntryHandle
{
    DWORD dwMagic;
    LPWSTR lpszUrlSearchPattern;
    DWORD dwContainerIndex;
    DWORD dwHashTableIndex;
    DWORD dwHashEntryIndex;
} URLCacheFindEntryHandle;

/***********************************************************************
 *           FindFirstUrlCacheEntryA (WININET.@)
 *
 */
INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryA(LPCSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
    URLCacheFindEntryHandle *pEntryHandle;

    TRACE("(%s, %p, %p)\n", debugstr_a(lpszUrlSearchPattern), lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize);

    pEntryHandle = heap_alloc(sizeof(*pEntryHandle));
    if (!pEntryHandle)
        return NULL;

    pEntryHandle->dwMagic = URLCACHE_FIND_ENTRY_HANDLE_MAGIC;
    if (lpszUrlSearchPattern)
    {
        pEntryHandle->lpszUrlSearchPattern = heap_strdupAtoW(lpszUrlSearchPattern);
        if (!pEntryHandle->lpszUrlSearchPattern)
        {
            heap_free(pEntryHandle);
            return NULL;
        }
    }
    else
        pEntryHandle->lpszUrlSearchPattern = NULL;
    pEntryHandle->dwContainerIndex = 0;
    pEntryHandle->dwHashTableIndex = 0;
    pEntryHandle->dwHashEntryIndex = 0;

    if (!FindNextUrlCacheEntryA(pEntryHandle, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize))
    {
        heap_free(pEntryHandle);
        return NULL;
    }
    return pEntryHandle;
}

/***********************************************************************
 *           FindFirstUrlCacheEntryW (WININET.@)
 *
 */
INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryW(LPCWSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
    URLCacheFindEntryHandle *pEntryHandle;

    TRACE("(%s, %p, %p)\n", debugstr_w(lpszUrlSearchPattern), lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize);

    pEntryHandle = heap_alloc(sizeof(*pEntryHandle));
    if (!pEntryHandle)
        return NULL;

    pEntryHandle->dwMagic = URLCACHE_FIND_ENTRY_HANDLE_MAGIC;
    if (lpszUrlSearchPattern)
    {
        pEntryHandle->lpszUrlSearchPattern = heap_strdupW(lpszUrlSearchPattern);
        if (!pEntryHandle->lpszUrlSearchPattern)
        {
            heap_free(pEntryHandle);
            return NULL;
        }
    }
    else
        pEntryHandle->lpszUrlSearchPattern = NULL;
    pEntryHandle->dwContainerIndex = 0;
    pEntryHandle->dwHashTableIndex = 0;
    pEntryHandle->dwHashEntryIndex = 0;

    if (!FindNextUrlCacheEntryW(pEntryHandle, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize))
    {
        heap_free(pEntryHandle);
        return NULL;
    }
    return pEntryHandle;
}

static BOOL FindNextUrlCacheEntryInternal(
  HANDLE hEnumHandle,
  LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo,
  LPDWORD lpdwNextCacheEntryInfoBufferSize,
  BOOL unicode)
{
    URLCacheFindEntryHandle *pEntryHandle = (URLCacheFindEntryHandle *)hEnumHandle;
    URLCACHECONTAINER * pContainer;

    if (pEntryHandle->dwMagic != URLCACHE_FIND_ENTRY_HANDLE_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    for (; URLCacheContainers_Enum(pEntryHandle->lpszUrlSearchPattern, pEntryHandle->dwContainerIndex, &pContainer);
         pEntryHandle->dwContainerIndex++, pEntryHandle->dwHashTableIndex = 0)
    {
        LPURLCACHE_HEADER pHeader;
        HASH_CACHEFILE_ENTRY *pHashTableEntry;
        DWORD error;

        error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
        if (error != ERROR_SUCCESS)
        {
            SetLastError(error);
            return FALSE;
        }

        if (!(pHeader = cache_container_lock_index(pContainer)))
            return FALSE;

        for (; URLCache_EnumHashTables(pHeader, &pEntryHandle->dwHashTableIndex, &pHashTableEntry);
             pEntryHandle->dwHashTableIndex++, pEntryHandle->dwHashEntryIndex = 0)
        {
            const struct _HASH_ENTRY *pHashEntry = NULL;
            for (; URLCache_EnumHashTableEntries(pHeader, pHashTableEntry, &pEntryHandle->dwHashEntryIndex, &pHashEntry);
                 pEntryHandle->dwHashEntryIndex++)
            {
                const entry_url *pUrlEntry;
                const entry_header *pEntry = (const entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);

                if (pEntry->signature != URL_SIGNATURE)
                    continue;

                pUrlEntry = (const entry_url *)pEntry;
                TRACE("Found URL: %s\n",
                      debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->url_off));
                TRACE("Header info: %s\n",
                        debugstr_an((LPCSTR)pUrlEntry + pUrlEntry->header_info_off,
                            pUrlEntry->header_info_size));

                error = URLCache_CopyEntry(
                    pContainer,
                    pHeader,
                    lpNextCacheEntryInfo,
                    lpdwNextCacheEntryInfoBufferSize,
                    pUrlEntry,
                    unicode);
                if (error != ERROR_SUCCESS)
                {
                    cache_container_unlock_index(pContainer, pHeader);
                    SetLastError(error);
                    return FALSE;
                }
                if(pUrlEntry->local_name_off)
                    TRACE("Local File Name: %s\n", debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->local_name_off));

                /* increment the current index so that next time the function
                 * is called the next entry is returned */
                pEntryHandle->dwHashEntryIndex++;
                cache_container_unlock_index(pContainer, pHeader);
                return TRUE;
            }
        }

        cache_container_unlock_index(pContainer, pHeader);
    }

    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
}

/***********************************************************************
 *           FindNextUrlCacheEntryA (WININET.@)
 */
BOOL WINAPI FindNextUrlCacheEntryA(
  HANDLE hEnumHandle,
  LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo,
  LPDWORD lpdwNextCacheEntryInfoBufferSize)
{
    TRACE("(%p, %p, %p)\n", hEnumHandle, lpNextCacheEntryInfo, lpdwNextCacheEntryInfoBufferSize);

    return FindNextUrlCacheEntryInternal(hEnumHandle, lpNextCacheEntryInfo,
            lpdwNextCacheEntryInfoBufferSize, FALSE /* not UNICODE */);
}

/***********************************************************************
 *           FindNextUrlCacheEntryW (WININET.@)
 */
BOOL WINAPI FindNextUrlCacheEntryW(
  HANDLE hEnumHandle,
  LPINTERNET_CACHE_ENTRY_INFOW lpNextCacheEntryInfo,
  LPDWORD lpdwNextCacheEntryInfoBufferSize
)
{
    TRACE("(%p, %p, %p)\n", hEnumHandle, lpNextCacheEntryInfo, lpdwNextCacheEntryInfoBufferSize);

    return FindNextUrlCacheEntryInternal(hEnumHandle,
            (LPINTERNET_CACHE_ENTRY_INFOA)lpNextCacheEntryInfo,
            lpdwNextCacheEntryInfoBufferSize, TRUE /* UNICODE */);
}

/***********************************************************************
 *           FindCloseUrlCache (WININET.@)
 */
BOOL WINAPI FindCloseUrlCache(HANDLE hEnumHandle)
{
    URLCacheFindEntryHandle *pEntryHandle = (URLCacheFindEntryHandle *)hEnumHandle;

    TRACE("(%p)\n", hEnumHandle);

    if (!pEntryHandle || pEntryHandle->dwMagic != URLCACHE_FIND_ENTRY_HANDLE_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pEntryHandle->dwMagic = 0;
    heap_free(pEntryHandle->lpszUrlSearchPattern);
    heap_free(pEntryHandle);
    return TRUE;
}

HANDLE WINAPI FindFirstUrlCacheGroup( DWORD dwFlags, DWORD dwFilter, LPVOID lpSearchCondition,
                                      DWORD dwSearchCondition, GROUPID* lpGroupId, LPVOID lpReserved )
{
    FIXME("(0x%08x, 0x%08x, %p, 0x%08x, %p, %p) stub\n", dwFlags, dwFilter, lpSearchCondition,
          dwSearchCondition, lpGroupId, lpReserved);
    return NULL;
}

BOOL WINAPI FindNextUrlCacheEntryExA(
  HANDLE hEnumHandle,
  LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
  LPDWORD lpdwFirstCacheEntryInfoBufferSize,
  LPVOID lpReserved,
  LPDWORD pcbReserved2,
  LPVOID lpReserved3
)
{
    FIXME("(%p, %p, %p, %p, %p, %p) stub\n", hEnumHandle, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize,
          lpReserved, pcbReserved2, lpReserved3);
    return FALSE;
}

BOOL WINAPI FindNextUrlCacheEntryExW(
  HANDLE hEnumHandle,
  LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
  LPDWORD lpdwFirstCacheEntryInfoBufferSize,
  LPVOID lpReserved,
  LPDWORD pcbReserved2,
  LPVOID lpReserved3
)
{
    FIXME("(%p, %p, %p, %p, %p, %p) stub\n", hEnumHandle, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize,
          lpReserved, pcbReserved2, lpReserved3);
    return FALSE;
}

BOOL WINAPI FindNextUrlCacheGroup( HANDLE hFind, GROUPID* lpGroupId, LPVOID lpReserved )
{
    FIXME("(%p, %p, %p) stub\n", hFind, lpGroupId, lpReserved);
    return FALSE;
}

/***********************************************************************
 *           CreateUrlCacheGroup (WININET.@)
 *
 */
INTERNETAPI GROUPID WINAPI CreateUrlCacheGroup(DWORD dwFlags, LPVOID lpReserved)
{
  FIXME("(0x%08x, %p): stub\n", dwFlags, lpReserved);
  return FALSE;
}

/***********************************************************************
 *           DeleteUrlCacheGroup (WININET.@)
 *
 */
BOOL WINAPI DeleteUrlCacheGroup(GROUPID GroupId, DWORD dwFlags, LPVOID lpReserved)
{
    FIXME("(0x%08x%08x, 0x%08x, %p) stub\n",
          (ULONG)(GroupId >> 32), (ULONG)GroupId, dwFlags, lpReserved);
    return FALSE;
}

/***********************************************************************
 *           DeleteWpadCacheForNetworks (WININET.@)
 *    Undocumented, added in IE8
 */
BOOL WINAPI DeleteWpadCacheForNetworks(DWORD unk1)
{
    FIXME("(%d) stub\n", unk1);
    return FALSE;
}

/***********************************************************************
 *           SetUrlCacheEntryGroupA (WININET.@)
 *
 */
BOOL WINAPI SetUrlCacheEntryGroupA(LPCSTR lpszUrlName, DWORD dwFlags,
  GROUPID GroupId, LPBYTE pbGroupAttributes, DWORD cbGroupAttributes,
  LPVOID lpReserved)
{
    FIXME("(%s, 0x%08x, 0x%08x%08x, %p, 0x%08x, %p) stub\n",
          debugstr_a(lpszUrlName), dwFlags, (ULONG)(GroupId >> 32), (ULONG)GroupId,
          pbGroupAttributes, cbGroupAttributes, lpReserved);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           SetUrlCacheEntryGroupW (WININET.@)
 *
 */
BOOL WINAPI SetUrlCacheEntryGroupW(LPCWSTR lpszUrlName, DWORD dwFlags,
  GROUPID GroupId, LPBYTE pbGroupAttributes, DWORD cbGroupAttributes,
  LPVOID lpReserved)
{
    FIXME("(%s, 0x%08x, 0x%08x%08x, %p, 0x%08x, %p) stub\n",
          debugstr_w(lpszUrlName), dwFlags, (ULONG)(GroupId >> 32), (ULONG)GroupId,
          pbGroupAttributes, cbGroupAttributes, lpReserved);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheConfigInfoW (WININET.@)
 */
BOOL WINAPI GetUrlCacheConfigInfoW(LPINTERNET_CACHE_CONFIG_INFOW CacheInfo, LPDWORD size, DWORD bitmask)
{
    FIXME("(%p, %p, %x)\n", CacheInfo, size, bitmask);
    INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheConfigInfoA (WININET.@)
 */
BOOL WINAPI GetUrlCacheConfigInfoA(LPINTERNET_CACHE_CONFIG_INFOA CacheInfo, LPDWORD size, DWORD bitmask)
{
    FIXME("(%p, %p, %x)\n", CacheInfo, size, bitmask);
    INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL WINAPI GetUrlCacheGroupAttributeA( GROUPID gid, DWORD dwFlags, DWORD dwAttributes,
                                        LPINTERNET_CACHE_GROUP_INFOA lpGroupInfo,
                                        LPDWORD lpdwGroupInfo, LPVOID lpReserved )
{
    FIXME("(0x%08x%08x, 0x%08x, 0x%08x, %p, %p, %p) stub\n",
          (ULONG)(gid >> 32), (ULONG)gid, dwFlags, dwAttributes, lpGroupInfo,
          lpdwGroupInfo, lpReserved);
    return FALSE;
}

BOOL WINAPI GetUrlCacheGroupAttributeW( GROUPID gid, DWORD dwFlags, DWORD dwAttributes,
                                        LPINTERNET_CACHE_GROUP_INFOW lpGroupInfo,
                                        LPDWORD lpdwGroupInfo, LPVOID lpReserved )
{
    FIXME("(0x%08x%08x, 0x%08x, 0x%08x, %p, %p, %p) stub\n",
          (ULONG)(gid >> 32), (ULONG)gid, dwFlags, dwAttributes, lpGroupInfo,
          lpdwGroupInfo, lpReserved);
    return FALSE;
}

BOOL WINAPI SetUrlCacheGroupAttributeA( GROUPID gid, DWORD dwFlags, DWORD dwAttributes,
                                        LPINTERNET_CACHE_GROUP_INFOA lpGroupInfo, LPVOID lpReserved )
{
    FIXME("(0x%08x%08x, 0x%08x, 0x%08x, %p, %p) stub\n",
          (ULONG)(gid >> 32), (ULONG)gid, dwFlags, dwAttributes, lpGroupInfo, lpReserved);
    return TRUE;
}

BOOL WINAPI SetUrlCacheGroupAttributeW( GROUPID gid, DWORD dwFlags, DWORD dwAttributes,
                                        LPINTERNET_CACHE_GROUP_INFOW lpGroupInfo, LPVOID lpReserved )
{
    FIXME("(0x%08x%08x, 0x%08x, 0x%08x, %p, %p) stub\n",
          (ULONG)(gid >> 32), (ULONG)gid, dwFlags, dwAttributes, lpGroupInfo, lpReserved);
    return TRUE;
}

BOOL WINAPI SetUrlCacheConfigInfoA( LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo, DWORD dwFieldControl )
{
    FIXME("(%p, 0x%08x) stub\n", lpCacheConfigInfo, dwFieldControl);
    return TRUE;
}

BOOL WINAPI SetUrlCacheConfigInfoW( LPINTERNET_CACHE_CONFIG_INFOW lpCacheConfigInfo, DWORD dwFieldControl )
{
    FIXME("(%p, 0x%08x) stub\n", lpCacheConfigInfo, dwFieldControl);
    return TRUE;
}

/***********************************************************************
 *           DeleteIE3Cache (WININET.@)
 *
 * Deletes the files used by the IE3 URL caching system.
 *
 * PARAMS
 *   hWnd        [I] A dummy window.
 *   hInst       [I] Instance of process calling the function.
 *   lpszCmdLine [I] Options used by function.
 *   nCmdShow    [I] The nCmdShow value to use when showing windows created, if any.
 */
DWORD WINAPI DeleteIE3Cache(HWND hWnd, HINSTANCE hInst, LPSTR lpszCmdLine, int nCmdShow)
{
    FIXME("(%p, %p, %s, %d)\n", hWnd, hInst, debugstr_a(lpszCmdLine), nCmdShow);
    return 0;
}

static BOOL IsUrlCacheEntryExpiredInternal(const entry_url *pUrlEntry,
        FILETIME *pftLastModified)
{
    BOOL ret;
    FILETIME now, expired;

    *pftLastModified = pUrlEntry->modification_time;
    GetSystemTimeAsFileTime(&now);
    URLCache_DosDateTimeToFileTime(pUrlEntry->expire_date,
            pUrlEntry->expire_time, &expired);
    /* If the expired time is 0, it's interpreted as not expired */
    if (!expired.dwLowDateTime && !expired.dwHighDateTime)
        ret = FALSE;
    else
        ret = CompareFileTime(&expired, &now) < 0;
    return ret;
}

/***********************************************************************
 *           IsUrlCacheEntryExpiredA (WININET.@)
 *
 * PARAMS
 *   url             [I] Url
 *   dwFlags         [I] Unknown
 *   pftLastModified [O] Last modified time
 */
BOOL WINAPI IsUrlCacheEntryExpiredA( LPCSTR url, DWORD dwFlags, FILETIME* pftLastModified )
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    const entry_header *pEntry;
    const entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    BOOL expired;

    TRACE("(%s, %08x, %p)\n", debugstr_a(url), dwFlags, pftLastModified);

    if (!url || !pftLastModified)
        return TRUE;
    if (dwFlags)
        FIXME("unknown flags 0x%08x\n", dwFlags);

    /* Any error implies that the URL is expired, i.e. not in the cache */
    if (URLCacheContainers_FindContainerA(url, &pContainer))
    {
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        return TRUE;
    }

    if (cache_container_open_index(pContainer, MIN_BLOCK_NO))
    {
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        return TRUE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
    {
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        return TRUE;
    }

    if (!URLCache_FindHash(pHeader, url, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        TRACE("entry %s not found!\n", url);
        return TRUE;
    }

    pEntry = (const entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPCSTR)&pEntry->signature, sizeof(DWORD)));
        return TRUE;
    }

    pUrlEntry = (const entry_url *)pEntry;
    expired = IsUrlCacheEntryExpiredInternal(pUrlEntry, pftLastModified);

    cache_container_unlock_index(pContainer, pHeader);

    return expired;
}

/***********************************************************************
 *           IsUrlCacheEntryExpiredW (WININET.@)
 *
 * PARAMS
 *   url             [I] Url
 *   dwFlags         [I] Unknown
 *   pftLastModified [O] Last modified time
 */
BOOL WINAPI IsUrlCacheEntryExpiredW( LPCWSTR url, DWORD dwFlags, FILETIME* pftLastModified )
{
    LPURLCACHE_HEADER pHeader;
    struct _HASH_ENTRY * pHashEntry;
    const entry_header *pEntry;
    const entry_url * pUrlEntry;
    URLCACHECONTAINER * pContainer;
    BOOL expired;

    TRACE("(%s, %08x, %p)\n", debugstr_w(url), dwFlags, pftLastModified);

    if (!url || !pftLastModified)
        return TRUE;
    if (dwFlags)
        FIXME("unknown flags 0x%08x\n", dwFlags);

    /* Any error implies that the URL is expired, i.e. not in the cache */
    if (URLCacheContainers_FindContainerW(url, &pContainer))
    {
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        return TRUE;
    }

    if (cache_container_open_index(pContainer, MIN_BLOCK_NO))
    {
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        return TRUE;
    }

    if (!(pHeader = cache_container_lock_index(pContainer)))
    {
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        return TRUE;
    }

    if (!URLCache_FindHashW(pHeader, url, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        TRACE("entry %s not found!\n", debugstr_w(url));
        return TRUE;
    }

    if (!URLCache_FindHashW(pHeader, url, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        TRACE("entry %s not found!\n", debugstr_w(url));
        return TRUE;
    }

    pEntry = (const entry_header*)((LPBYTE)pHeader + pHashEntry->dwOffsetEntry);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPCSTR)&pEntry->signature, sizeof(DWORD)));
        return TRUE;
    }

    pUrlEntry = (const entry_url *)pEntry;
    expired = IsUrlCacheEntryExpiredInternal(pUrlEntry, pftLastModified);

    cache_container_unlock_index(pContainer, pHeader);

    return expired;
}

/***********************************************************************
 *           GetDiskInfoA (WININET.@)
 */
BOOL WINAPI GetDiskInfoA(PCSTR path, PDWORD cluster_size, PDWORDLONG free, PDWORDLONG total)
{
    BOOL ret;
    ULARGE_INTEGER bytes_free, bytes_total;

    TRACE("(%s, %p, %p, %p)\n", debugstr_a(path), cluster_size, free, total);

    if (!path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((ret = GetDiskFreeSpaceExA(path, NULL, &bytes_total, &bytes_free)))
    {
        if (cluster_size) *cluster_size = 1;
        if (free) *free = bytes_free.QuadPart;
        if (total) *total = bytes_total.QuadPart;
    }
    return ret;
}

/***********************************************************************
 *           RegisterUrlCacheNotification (WININET.@)
 */
DWORD WINAPI RegisterUrlCacheNotification(LPVOID a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f)
{
    FIXME("(%p %x %x %x %x %x)\n", a, b, c, d, e, f);
    return 0;
}

/***********************************************************************
 *           IncrementUrlCacheHeaderData (WININET.@)
 */
BOOL WINAPI IncrementUrlCacheHeaderData(DWORD index, LPDWORD data)
{
    FIXME("(%u, %p)\n", index, data);
    return FALSE;
}

/***********************************************************************
 *           RunOnceUrlCache (WININET.@)
 */

DWORD WINAPI RunOnceUrlCache(HWND hwnd, HINSTANCE hinst, LPSTR cmd, int cmdshow)
{
    FIXME("(%p, %p, %s, %d): stub\n", hwnd, hinst, debugstr_a(cmd), cmdshow);
    return 0;
}

BOOL init_urlcache(void)
{
    dll_unload_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if(!dll_unload_event)
        return FALSE;

    free_cache_running = CreateSemaphoreW(NULL, 1, 1, NULL);
    if(!free_cache_running) {
        CloseHandle(dll_unload_event);
        return FALSE;
    }

    URLCacheContainers_CreateDefaults();
    return TRUE;
}

void free_urlcache(void)
{
    SetEvent(dll_unload_event);
    WaitForSingleObject(free_cache_running, INFINITE);
    ReleaseSemaphore(free_cache_running, 1, NULL);
    CloseHandle(free_cache_running);
    CloseHandle(dll_unload_event);

    URLCacheContainers_DeleteAll();
}

/***********************************************************************
 *           LoadUrlCacheContent (WININET.@)
 */
BOOL WINAPI LoadUrlCacheContent(void)
{
    FIXME("stub!\n");
    return FALSE;
}
