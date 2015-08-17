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

#include "internet.h"

#include <shlobj.h>
#include <shellapi.h>

static const char urlcache_ver_prefix[] = "WINE URLCache Ver ";
static const char urlcache_ver[] = "0.2012001";

#ifndef CHAR_BIT
#define CHAR_BIT    (8 * sizeof(CHAR))
#endif

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
#define MAX_BLOCK_NO            (ALLOCATION_TABLE_SIZE * CHAR_BIT)
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

#define URLCACHE_FIND_ENTRY_HANDLE_MAGIC 0xF389ABCD

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

struct hash_entry
{
    DWORD key;
    DWORD offset;
};

typedef struct
{
    entry_header header;
    DWORD next;
    DWORD id;
    struct hash_entry hash_table[HASHTABLE_SIZE];
} entry_hash_table;

typedef struct
{
    char signature[28];
    DWORD size;
    DWORD hash_table_off;
    DWORD capacity_in_blocks;
    DWORD blocks_in_use;
    DWORD unk1;
    ULARGE_INTEGER cache_limit;
    ULARGE_INTEGER cache_usage;
    ULARGE_INTEGER exempt_usage;
    DWORD dirs_no;
    struct _directory_data
    {
        DWORD files_no;
        char name[DIR_LENGTH];
    } directory_data[MAX_DIR_NO];
    DWORD options[0x21];
    BYTE allocation_table[ALLOCATION_TABLE_SIZE];
} urlcache_header;

typedef struct
{
    HANDLE file;
    CHAR url[1];
} stream_handle;

typedef struct
{
    struct list entry; /* part of a list */
    char *cache_prefix; /* string that has to be prefixed for this container to be used */
    LPWSTR path; /* path to url container directory */
    HANDLE mapping; /* handle of file mapping */
    DWORD file_size; /* size of file when mapping was opened */
    HANDLE mutex; /* handle of mutex */
    DWORD default_entry_type;
} cache_container;

typedef struct
{
    DWORD magic;
    char *url_search_pattern;
    DWORD container_idx;
    DWORD hash_table_idx;
    DWORD hash_entry_idx;
} find_handle;

/* List of all containers available */
static struct list UrlContainers = LIST_INIT(UrlContainers);
/* ReactOS r54992 */
BOOL bDefaultContainersAdded = FALSE;

static inline char *heap_strdupWtoUTF8(LPCWSTR str)
{
    char *ret = NULL;

    if(str) {
        DWORD size = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
        ret = heap_alloc(size);
        if(ret)
            WideCharToMultiByte(CP_UTF8, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

/***********************************************************************
 *           urlcache_block_is_free (Internal)
 *
 *  Is the specified block number free?
 *
 * RETURNS
 *    zero if free
 *    non-zero otherwise
 *
 */
static inline BYTE urlcache_block_is_free(BYTE *allocation_table, DWORD block_number)
{
    BYTE mask = 1 << (block_number%CHAR_BIT);
    return (allocation_table[block_number/CHAR_BIT] & mask) == 0;
}

/***********************************************************************
 *           urlcache_block_free (Internal)
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
static inline void urlcache_block_free(BYTE *allocation_table, DWORD block_number)
{
    BYTE mask = ~(1 << (block_number%CHAR_BIT));
    allocation_table[block_number/CHAR_BIT] &= mask;
}

/***********************************************************************
 *           urlcache_block_alloc (Internal)
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
static inline void urlcache_block_alloc(BYTE *allocation_table, DWORD block_number)
{
    BYTE mask = 1 << (block_number%CHAR_BIT);
    allocation_table[block_number/CHAR_BIT] |= mask;
}

/***********************************************************************
 *           urlcache_entry_alloc (Internal)
 *
 *  Finds and allocates the first block of free space big enough and
 * sets entry to point to it.
 *
 * RETURNS
 *    ERROR_SUCCESS when free memory block was found
 *    Any other Win32 error code if the entry could not be added
 *
 */
static DWORD urlcache_entry_alloc(urlcache_header *header, DWORD blocks_needed, entry_header **entry)
{
    DWORD block, block_size;

    for(block=0; block<header->capacity_in_blocks; block+=block_size+1)
    {
        block_size = 0;
        while(block_size<blocks_needed && block_size+block<header->capacity_in_blocks
                && urlcache_block_is_free(header->allocation_table, block+block_size))
            block_size++;

        if(block_size == blocks_needed)
        {
            DWORD index;

            TRACE("Found free blocks starting at no. %d (0x%x)\n", block, ENTRY_START_OFFSET+block*BLOCKSIZE);

            for(index=0; index<blocks_needed; index++)
                urlcache_block_alloc(header->allocation_table, block+index);

            *entry = (entry_header*)((BYTE*)header+ENTRY_START_OFFSET+block*BLOCKSIZE);
            for(index=0; index<blocks_needed*BLOCKSIZE/sizeof(DWORD); index++)
                ((DWORD*)*entry)[index] = 0xdeadbeef;
            (*entry)->blocks_used = blocks_needed;

            header->blocks_in_use += blocks_needed;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_HANDLE_DISK_FULL;
}

/***********************************************************************
 *           urlcache_entry_free (Internal)
 *
 *  Deletes the specified entry and frees the space allocated to it
 *
 * RETURNS
 *    TRUE if it succeeded
 *    FALSE if it failed
 *
 */
static BOOL urlcache_entry_free(urlcache_header *header, entry_header *entry)
{
    DWORD start_block, block;

    /* update allocation table */
    start_block = ((DWORD)((BYTE*)entry - (BYTE*)header) - ENTRY_START_OFFSET) / BLOCKSIZE;
    for(block = start_block; block < start_block+entry->blocks_used; block++)
        urlcache_block_free(header->allocation_table, block);

    header->blocks_in_use -= entry->blocks_used;
    return TRUE;
}

/***********************************************************************
 *           urlcache_create_hash_table (Internal)
 *
 *  Creates a new hash table in free space and adds it to the chain of existing
 * hash tables.
 *
 * RETURNS
 *    ERROR_SUCCESS if the hash table was created
 *    ERROR_DISK_FULL if the hash table could not be created
 *
 */
static DWORD urlcache_create_hash_table(urlcache_header *header, entry_hash_table *hash_table_prev, entry_hash_table **hash_table)
{
    DWORD dwOffset, error;
    int i;

    if((error = urlcache_entry_alloc(header, 0x20, (entry_header**)hash_table)) != ERROR_SUCCESS)
        return error;

    dwOffset = (BYTE*)*hash_table-(BYTE*)header;

    if(hash_table_prev)
        hash_table_prev->next = dwOffset;
    else
        header->hash_table_off = dwOffset;

    (*hash_table)->header.signature = HASH_SIGNATURE;
    (*hash_table)->next = 0;
    (*hash_table)->id = hash_table_prev ? hash_table_prev->id+1 : 0;
    for(i = 0; i < HASHTABLE_SIZE; i++) {
        (*hash_table)->hash_table[i].offset = HASHTABLE_FREE;
        (*hash_table)->hash_table[i].key = HASHTABLE_FREE;
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           cache_container_create_object_name (Internal)
 *
 *  Converts a path to a name suitable for use as a Win32 object name.
 * Replaces '\\' characters in-place with the specified character
 * (usually '_' or '!')
 *
 * RETURNS
 *    nothing
 *
 */
static void cache_container_create_object_name(LPWSTR lpszPath, WCHAR replace)
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
    cache_container_create_object_name(mapping_name, '_');

    mapping = OpenFileMappingW(FILE_MAP_WRITE, FALSE, mapping_name);
    if(mapping) {
        if(validate) *validate = FALSE;
        return mapping;
    }

    if(validate) *validate = TRUE;
    return CreateFileMappingW(file, NULL, PAGE_READWRITE, 0, 0, mapping_name);
}

/* Caller must hold container lock */
static DWORD cache_container_set_size(cache_container *container, HANDLE file, DWORD blocks_no)
{
    static const WCHAR cache_content_key[] = {'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s','\\',
        'C','a','c','h','e','\\','C','o','n','t','e','n','t',0};
    static const WCHAR cache_limit[] = {'C','a','c','h','e','L','i','m','i','t',0};

    DWORD file_size = FILE_SIZE(blocks_no);
    WCHAR dir_path[MAX_PATH], *dir_name;
    entry_hash_table *hashtable_entry;
    urlcache_header *header;
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
        if(file_size > header->size)
            memset((char*)header+header->size, 0, file_size-header->size);
        header->size = file_size;
        header->capacity_in_blocks = blocks_no;

        UnmapViewOfFile(header);
        CloseHandle(container->mapping);
        container->mapping = mapping;
        container->file_size = file_size;
        return ERROR_SUCCESS;
    }

    memset(header, 0, file_size);
    /* First set some constants and defaults in the header */
    memcpy(header->signature, urlcache_ver_prefix, sizeof(urlcache_ver_prefix)-1);
    memcpy(header->signature+sizeof(urlcache_ver_prefix)-1, urlcache_ver, sizeof(urlcache_ver)-1);
    header->size = file_size;
    header->capacity_in_blocks = blocks_no;
    /* 127MB - taken from default for Windows 2000 */
    header->cache_limit.QuadPart = 0x07ff5400;
    /* Copied from a Windows 2000 cache index */
    header->dirs_no = container->default_entry_type==NORMAL_CACHE_ENTRY ? 4 : 0;

    /* If the registry has a cache size set, use the registry value */
    if(RegOpenKeyW(HKEY_CURRENT_USER, cache_content_key, &key) == ERROR_SUCCESS) {
        DWORD dw, len = sizeof(dw), keytype;

        if(RegQueryValueExW(key, cache_limit, NULL, &keytype, (BYTE*)&dw, &len) == ERROR_SUCCESS &&
                keytype == REG_DWORD)
            header->cache_limit.QuadPart = (ULONGLONG)dw * 1024;
        RegCloseKey(key);
    }

    urlcache_create_hash_table(header, NULL, &hashtable_entry);

    /* Last step - create the directories */
    strcpyW(dir_path, container->path);
    dir_name = dir_path + strlenW(dir_path);
    dir_name[8] = 0;

    GetSystemTimeAsFileTime(&ft);

    for(i=0; i<header->dirs_no; ++i) {
        header->directory_data[i].files_no = 0;
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
                    header->directory_data[i].name[k] = dir_name[k];
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
    CloseHandle(container->mapping);
    container->mapping = mapping;
    container->file_size = file_size;
    return ERROR_SUCCESS;
}

static BOOL cache_container_is_valid(urlcache_header *header, DWORD file_size)
{
    DWORD allocation_size, count_bits, i;

    if(file_size < FILE_SIZE(MIN_BLOCK_NO))
        return FALSE;

    if(file_size != header->size)
        return FALSE;

    if (!memcmp(header->signature, urlcache_ver_prefix, sizeof(urlcache_ver_prefix)-1) &&
            memcmp(header->signature+sizeof(urlcache_ver_prefix)-1, urlcache_ver, sizeof(urlcache_ver)-1))
        return FALSE;

    if(FILE_SIZE(header->capacity_in_blocks) != file_size)
        return FALSE;

    allocation_size = 0;
    for(i=0; i<header->capacity_in_blocks/8; i++) {
        for(count_bits = header->allocation_table[i]; count_bits!=0; count_bits>>=1) {
            if(count_bits & 1)
                allocation_size++;
        }
    }
    if(allocation_size != header->blocks_in_use)
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
 *  Opens the index file and saves mapping handle
 *
 * RETURNS
 *    ERROR_SUCCESS if succeeded
 *    Any other Win32 error code if failed
 *
 */
static DWORD cache_container_open_index(cache_container *container, DWORD blocks_no)
{
    static const WCHAR index_dat[] = {'i','n','d','e','x','.','d','a','t',0};

    HANDLE file;
    WCHAR index_path[MAX_PATH];
    DWORD file_size;
    BOOL validate;

    WaitForSingleObject(container->mutex, INFINITE);

    if(container->mapping) {
        ReleaseMutex(container->mutex);
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
        ReleaseMutex(container->mutex);
        return GetLastError();
    }

    file_size = GetFileSize(file, NULL);
    if(file_size == INVALID_FILE_SIZE) {
        CloseHandle(file);
	ReleaseMutex(container->mutex);
        return GetLastError();
    }

    if(blocks_no < MIN_BLOCK_NO)
        blocks_no = MIN_BLOCK_NO;
    else if(blocks_no > MAX_BLOCK_NO)
        blocks_no = MAX_BLOCK_NO;

    if(file_size < FILE_SIZE(blocks_no)) {
        DWORD ret = cache_container_set_size(container, file, blocks_no);
        CloseHandle(file);
        ReleaseMutex(container->mutex);
        return ret;
    }

    container->file_size = file_size;
    container->mapping = cache_container_map_index(file, container->path, file_size, &validate);
    CloseHandle(file);
    if(container->mapping && validate) {
        urlcache_header *header = MapViewOfFile(container->mapping, FILE_MAP_WRITE, 0, 0, 0);

        if(header && !cache_container_is_valid(header, file_size)) {
            WARN("detected old or broken index.dat file\n");
            UnmapViewOfFile(header);
            FreeUrlCacheSpaceW(container->path, 100, 0);
        }else if(header) {
            UnmapViewOfFile(header);
        }else {
            CloseHandle(container->mapping);
            container->mapping = NULL;
        }
    }

    if(!container->mapping)
    {
        ERR("Couldn't create file mapping (error is %d)\n", GetLastError());
        ReleaseMutex(container->mutex);
        return GetLastError();
    }

    ReleaseMutex(container->mutex);
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
static void cache_container_close_index(cache_container *pContainer)
{
    CloseHandle(pContainer->mapping);
    pContainer->mapping = NULL;
}

static BOOL cache_containers_add(const char *cache_prefix, LPCWSTR path,
        DWORD default_entry_type, LPWSTR mutex_name)
{
    cache_container *pContainer = heap_alloc(sizeof(cache_container));
    int cache_prefix_len = strlen(cache_prefix);

    if (!pContainer)
    {
        return FALSE;
    }

    pContainer->mapping = NULL;
    pContainer->file_size = 0;
    pContainer->default_entry_type = default_entry_type;

    pContainer->path = heap_strdupW(path);
    if (!pContainer->path)
    {
        heap_free(pContainer);
        return FALSE;
    }

    pContainer->cache_prefix = heap_alloc(cache_prefix_len+1);
    if (!pContainer->cache_prefix)
    {
        heap_free(pContainer->path);
        heap_free(pContainer);
        return FALSE;
    }

    memcpy(pContainer->cache_prefix, cache_prefix, cache_prefix_len+1);

    CharLowerW(mutex_name);
    cache_container_create_object_name(mutex_name, '!');

    if ((pContainer->mutex = CreateMutexW(NULL, FALSE, mutex_name)) == NULL)
    {
        ERR("couldn't create mutex (error is %d)\n", GetLastError());
        heap_free(pContainer->path);
        heap_free(pContainer);
        return FALSE;
    }

    list_add_head(&UrlContainers, &pContainer->entry);

    return TRUE;
}

static void cache_container_delete_container(cache_container *pContainer)
{
    list_remove(&pContainer->entry);

    cache_container_close_index(pContainer);
    CloseHandle(pContainer->mutex);
    heap_free(pContainer->path);
    heap_free(pContainer->cache_prefix);
    heap_free(pContainer);
}

static void cache_containers_init(void)
{
    static const WCHAR UrlSuffix[] = {'C','o','n','t','e','n','t','.','I','E','5',0};
    static const WCHAR HistorySuffix[] = {'H','i','s','t','o','r','y','.','I','E','5',0};
    static const WCHAR CookieSuffix[] = {0};
    /* ReactOS r50916 */
    static const WCHAR UserProfile[] = {'U','S','E','R','P','R','O','F','I','L','E',0};
    static const struct
    {
        int nFolder; /* CSIDL_* constant */
        const WCHAR *shpath_suffix; /* suffix on path returned by SHGetSpecialFolderPath */
        const char *cache_prefix; /* prefix used to reference the container */
        DWORD default_entry_type;
    } DefaultContainerData[] = 
    {
        { CSIDL_INTERNET_CACHE, UrlSuffix, "", NORMAL_CACHE_ENTRY },
        { CSIDL_HISTORY, HistorySuffix, "Visited:", URLHISTORY_CACHE_ENTRY },
        { CSIDL_COOKIES, CookieSuffix, "Cookie:", COOKIE_CACHE_ENTRY },
    };
    DWORD i;

    /* ReactOS r50916 */
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
        BOOL def_char;

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

        if (!WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wszCachePath, path_len,
                    NULL, 0, NULL, &def_char) || def_char)
        {
            WCHAR tmp[MAX_PATH];

            /* cannot convert path to ANSI code page */
            if (!(path_len = GetShortPathNameW(wszCachePath, tmp, MAX_PATH)) ||
                !WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, tmp, path_len,
                    NULL, 0, NULL, &def_char) || def_char)
                ERR("Can't create container path accessible by ANSI functions\n");
            else
                memcpy(wszCachePath, tmp, (path_len+1)*sizeof(WCHAR));
        }

        cache_containers_add(DefaultContainerData[i].cache_prefix, wszCachePath,
                DefaultContainerData[i].default_entry_type, wszMutexName);
    }

#ifdef __REACTOS__
    bDefaultContainersAdded = TRUE;
#endif
}

static void cache_containers_free(void)
{
    while(!list_empty(&UrlContainers))
        cache_container_delete_container(
            LIST_ENTRY(list_head(&UrlContainers), cache_container, entry)
        );
}

static DWORD cache_containers_find(const char *url, cache_container **ret)
{
    cache_container *container;

    TRACE("searching for prefix for URL: %s\n", debugstr_a(url));

    if(!url)
        return ERROR_INVALID_PARAMETER;

#ifdef __REACTOS__
    /* ReactOS r54992 */
    if (!bDefaultContainersAdded)
        cache_containers_init();
#endif

    LIST_FOR_EACH_ENTRY(container, &UrlContainers, cache_container, entry)
    {
        int prefix_len = strlen(container->cache_prefix);

        if(!strncmp(container->cache_prefix, url, prefix_len)) {
            TRACE("found container with prefix %s\n", debugstr_a(container->cache_prefix));
            *ret = container;
            return ERROR_SUCCESS;
        }
    }

    ERR("no container found\n");
    return ERROR_FILE_NOT_FOUND;
}

static BOOL cache_containers_enum(char *search_pattern, DWORD index, cache_container **ret)
{
    DWORD i = 0;
    cache_container *container;

    TRACE("searching for prefix: %s\n", debugstr_a(search_pattern));

    /* non-NULL search pattern only returns one container ever */
    if (search_pattern && index > 0)
        return FALSE;

#ifdef __REACTOS__
    /* ReactOS r54992 */
    if (!bDefaultContainersAdded)
        cache_containers_init();
#endif

    LIST_FOR_EACH_ENTRY(container, &UrlContainers, cache_container, entry)
    {
        if (search_pattern)
        {
            if (!strcmp(container->cache_prefix, search_pattern))
            {
                TRACE("found container with prefix %s\n", debugstr_a(container->cache_prefix));
                *ret = container;
                return TRUE;
            }
        }
        else
        {
            if (i == index)
            {
                TRACE("found container with prefix %s\n", debugstr_a(container->cache_prefix));
                *ret = container;
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
static urlcache_header* cache_container_lock_index(cache_container *pContainer)
{
    BYTE index;
    LPVOID pIndexData;
    urlcache_header* pHeader;
    DWORD error;

    /* acquire mutex */
    WaitForSingleObject(pContainer->mutex, INFINITE);

    pIndexData = MapViewOfFile(pContainer->mapping, FILE_MAP_WRITE, 0, 0, 0);

    if (!pIndexData)
    {
        ReleaseMutex(pContainer->mutex);
        ERR("Couldn't MapViewOfFile. Error: %d\n", GetLastError());
        return NULL;
    }
    pHeader = (urlcache_header*)pIndexData;

    /* file has grown - we need to remap to prevent us getting
     * access violations when we try and access beyond the end
     * of the memory mapped file */
    if (pHeader->size != pContainer->file_size)
    {
        UnmapViewOfFile( pHeader );
        cache_container_close_index(pContainer);
        error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
        if (error != ERROR_SUCCESS)
        {
            ReleaseMutex(pContainer->mutex);
            SetLastError(error);
            return NULL;
        }
        pIndexData = MapViewOfFile(pContainer->mapping, FILE_MAP_WRITE, 0, 0, 0);

        if (!pIndexData)
        {
            ReleaseMutex(pContainer->mutex);
            ERR("Couldn't MapViewOfFile. Error: %d\n", GetLastError());
            return NULL;
        }
        pHeader = (urlcache_header*)pIndexData;
    }

    TRACE("Signature: %s, file size: %d bytes\n", pHeader->signature, pHeader->size);

    for (index = 0; index < pHeader->dirs_no; index++)
    {
        TRACE("Directory[%d] = \"%.8s\"\n", index, pHeader->directory_data[index].name);
    }
    
    return pHeader;
}

/***********************************************************************
 *           cache_container_unlock_index (Internal)
 *
 */
static BOOL cache_container_unlock_index(cache_container *pContainer, urlcache_header *pHeader)
{
    /* release mutex */
    ReleaseMutex(pContainer->mutex);
    return UnmapViewOfFile(pHeader);
}

/***********************************************************************
 *           urlcache_create_file_pathW (Internal)
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
static BOOL urlcache_create_file_pathW(
    const cache_container *pContainer,
    const urlcache_header *pHeader,
    LPCSTR szLocalFileName,
    BYTE Directory,
    LPWSTR wszPath,
    LPLONG lpBufferSize,
    BOOL trunc_name)
{
    LONG nRequired;
    int path_len = strlenW(pContainer->path);
    int file_name_len = MultiByteToWideChar(CP_ACP, 0, szLocalFileName, -1, NULL, 0);
    if (Directory!=CACHE_CONTAINER_NO_SUBDIR && Directory>=pHeader->dirs_no)
    {
        *lpBufferSize = 0;
        return FALSE;
    }

    nRequired = (path_len + file_name_len) * sizeof(WCHAR);
    if(Directory != CACHE_CONTAINER_NO_SUBDIR)
        nRequired += (DIR_LENGTH + 1) * sizeof(WCHAR);
    if (trunc_name && nRequired >= *lpBufferSize)
        nRequired = *lpBufferSize;
    if (nRequired <= *lpBufferSize)
    {
        int dir_len;

        memcpy(wszPath, pContainer->path, path_len * sizeof(WCHAR));
        if (Directory != CACHE_CONTAINER_NO_SUBDIR)
        {
            dir_len = MultiByteToWideChar(CP_ACP, 0, pHeader->directory_data[Directory].name, DIR_LENGTH, wszPath + path_len, DIR_LENGTH);
            wszPath[dir_len + path_len] = '\\';
            dir_len++;
        }
        else
        {
            dir_len = 0;
        }
        MultiByteToWideChar(CP_ACP, 0, szLocalFileName, -1, wszPath + dir_len + path_len,
                *lpBufferSize/sizeof(WCHAR)-dir_len-path_len);
        wszPath[*lpBufferSize/sizeof(WCHAR)-1] = 0;
        *lpBufferSize = nRequired;
        return TRUE;
    }
    *lpBufferSize = nRequired;
    return FALSE;
}

/***********************************************************************
 *           urlcache_create_file_pathA (Internal)
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
static BOOL urlcache_create_file_pathA(
    const cache_container *pContainer,
    const urlcache_header *pHeader,
    LPCSTR szLocalFileName,
    BYTE Directory,
    LPSTR szPath,
    LPLONG lpBufferSize)
{
    LONG nRequired;
    int path_len, file_name_len, dir_len;

    if (Directory!=CACHE_CONTAINER_NO_SUBDIR && Directory>=pHeader->dirs_no)
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
    if (nRequired <= *lpBufferSize)
    {
        WideCharToMultiByte(CP_ACP, 0, pContainer->path, -1, szPath, path_len, NULL, NULL);
        if(dir_len) {
            memcpy(szPath+path_len, pHeader->directory_data[Directory].name, dir_len-1);
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
static void file_time_to_dos_date_time(const FILETIME *ft, WORD *fatdate,
                                           WORD *fattime)
{
    if (!ft->dwLowDateTime && !ft->dwHighDateTime)
        *fatdate = *fattime = 0;
    else
        FileTimeToDosDateTime(ft, fatdate, fattime);
}

/***********************************************************************
 *           urlcache_delete_file (Internal)
 */
static DWORD urlcache_delete_file(const cache_container *container,
        urlcache_header *header, entry_url *url_entry)
{
    WIN32_FILE_ATTRIBUTE_DATA attr;
    WCHAR path[MAX_PATH];
    LONG path_size = sizeof(path);
    DWORD err;
    WORD date, time;

    if(!url_entry->local_name_off)
        goto succ;

    if(!urlcache_create_file_pathW(container, header,
                (LPCSTR)url_entry+url_entry->local_name_off,
                url_entry->cache_dir, path, &path_size, FALSE))
        goto succ;

    if(!GetFileAttributesExW(path, GetFileExInfoStandard, &attr))
        goto succ;
    file_time_to_dos_date_time(&attr.ftLastWriteTime, &date, &time);
    if(date != url_entry->write_date || time != url_entry->write_time)
        goto succ;

    err = (DeleteFileW(path) ? ERROR_SUCCESS : GetLastError());
    if(err == ERROR_ACCESS_DENIED || err == ERROR_SHARING_VIOLATION)
        return err;

succ:
    if (url_entry->cache_dir < header->dirs_no)
    {
        if (header->directory_data[url_entry->cache_dir].files_no)
            header->directory_data[url_entry->cache_dir].files_no--;
    }
    if (url_entry->cache_entry_type & STICKY_CACHE_ENTRY)
    {
        if (url_entry->size.QuadPart < header->exempt_usage.QuadPart)
            header->exempt_usage.QuadPart -= url_entry->size.QuadPart;
        else
            header->exempt_usage.QuadPart = 0;
    }
    else
    {
        if (url_entry->size.QuadPart < header->cache_usage.QuadPart)
            header->cache_usage.QuadPart -= url_entry->size.QuadPart;
        else
            header->cache_usage.QuadPart = 0;
    }

    return ERROR_SUCCESS;
}

static BOOL urlcache_clean_leaked_entries(cache_container *container, urlcache_header *header)
{
    DWORD *leak_off;
    BOOL freed = FALSE;

    leak_off = &header->options[CACHE_HEADER_DATA_ROOT_LEAK_OFFSET];
    while(*leak_off) {
        entry_url *url_entry = (entry_url*)((LPBYTE)header + *leak_off);

        if(SUCCEEDED(urlcache_delete_file(container, header, url_entry))) {
            *leak_off = url_entry->exempt_delta;
            urlcache_entry_free(header, &url_entry->header);
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
static DWORD cache_container_clean_index(cache_container *container, urlcache_header **file_view)
{
    urlcache_header *header = *file_view;
    DWORD ret;

    TRACE("(%s %s)\n", debugstr_a(container->cache_prefix), debugstr_w(container->path));

    if(urlcache_clean_leaked_entries(container, header))
        return ERROR_SUCCESS;

    if(header->size >= ALLOCATION_TABLE_SIZE*8*BLOCKSIZE + ENTRY_START_OFFSET) {
        WARN("index file has maximal size\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    cache_container_close_index(container);
    ret = cache_container_open_index(container, header->capacity_in_blocks*2);
    if(ret != ERROR_SUCCESS)
        return ret;
    header = MapViewOfFile(container->mapping, FILE_MAP_WRITE, 0, 0, 0);
    if(!header)
        return GetLastError();

    UnmapViewOfFile(*file_view);
    *file_view = header;
    return ERROR_SUCCESS;
}

/* Just like DosDateTimeToFileTime, except that it also maps the special
 * case of a DOS date/time of (0,0) to a filetime of (0,0).
 */
static void dos_date_time_to_file_time(WORD fatdate, WORD fattime,
                                           FILETIME *ft)
{
    if (!fatdate && !fattime)
        ft->dwLowDateTime = ft->dwHighDateTime = 0;
    else
        DosDateTimeToFileTime(fatdate, fattime, ft);
}

static int urlcache_decode_url(const char *url, WCHAR *decoded_url, int decoded_len)
{
    URL_COMPONENTSA uc;
    DWORD len, part_len;
    WCHAR *host_name;

    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.dwHostNameLength = 1;
    if(!InternetCrackUrlA(url, 0, 0, &uc))
        uc.nScheme = INTERNET_SCHEME_UNKNOWN;

    if(uc.nScheme!=INTERNET_SCHEME_HTTP && uc.nScheme!=INTERNET_SCHEME_HTTPS)
        return MultiByteToWideChar(CP_UTF8, 0, url, -1, decoded_url, decoded_len);

    if(!decoded_url)
        decoded_len = 0;

    len = MultiByteToWideChar(CP_UTF8, 0, url, uc.lpszHostName-url, decoded_url, decoded_len);
    if(!len)
        return 0;
    if(decoded_url)
        decoded_len -= len;

    host_name = heap_alloc(uc.dwHostNameLength*sizeof(WCHAR));
    if(!host_name)
        return 0;
    if(!MultiByteToWideChar(CP_UTF8, 0, uc.lpszHostName, uc.dwHostNameLength,
                host_name, uc.dwHostNameLength)) {
        heap_free(host_name);
        return 0;
    }
    part_len = IdnToUnicode(0, host_name, uc.dwHostNameLength,
            decoded_url ? decoded_url+len : NULL, decoded_len);
    heap_free(host_name);
    if(!part_len) {
        SetLastError(ERROR_INTERNET_INVALID_URL);
        return 0;
    }
    len += part_len;
    if(decoded_url)
        decoded_len -= part_len;

    part_len = MultiByteToWideChar(CP_UTF8, 0,
            uc.lpszHostName+uc.dwHostNameLength,
            -1, decoded_url ? decoded_url+len : NULL, decoded_len);
    if(!part_len)
        return 0;
    len += part_len;

    return len;
}

/***********************************************************************
 *           urlcache_copy_entry (Internal)
 *
 *  Copies an entry from the cache index file to the Win32 structure
 *
 * RETURNS
 *    ERROR_SUCCESS if the buffer was big enough
 *    ERROR_INSUFFICIENT_BUFFER if the buffer was too small
 *
 */
static DWORD urlcache_copy_entry(cache_container *container, const urlcache_header *header,
        INTERNET_CACHE_ENTRY_INFOA *entry_info, DWORD *info_size, const entry_url *url_entry, BOOL unicode)
{
    int url_len;
    DWORD size = sizeof(*entry_info);

    if(*info_size >= size) {
        entry_info->lpHeaderInfo = NULL;
        entry_info->lpszFileExtension = NULL;
        entry_info->lpszLocalFileName = NULL;
        entry_info->lpszSourceUrlName = NULL;
        entry_info->CacheEntryType = url_entry->cache_entry_type;
        entry_info->u.dwExemptDelta = url_entry->exempt_delta;
        entry_info->dwHeaderInfoSize = url_entry->header_info_size;
        entry_info->dwHitRate = url_entry->hit_rate;
        entry_info->dwSizeHigh = url_entry->size.u.HighPart;
        entry_info->dwSizeLow = url_entry->size.u.LowPart;
        entry_info->dwStructSize = sizeof(*entry_info);
        entry_info->dwUseCount = url_entry->use_count;
        dos_date_time_to_file_time(url_entry->expire_date, url_entry->expire_time, &entry_info->ExpireTime);
        entry_info->LastAccessTime = url_entry->access_time;
        entry_info->LastModifiedTime = url_entry->modification_time;
        dos_date_time_to_file_time(url_entry->sync_date, url_entry->sync_time, &entry_info->LastSyncTime);
    }

    if(unicode)
        url_len = urlcache_decode_url((const char*)url_entry+url_entry->url_off, NULL, 0);
    else
        url_len = strlen((LPCSTR)url_entry+url_entry->url_off) + 1;
    size += url_len * (unicode ? sizeof(WCHAR) : sizeof(CHAR));

    if(*info_size >= size) {
        DWORD url_size = url_len * (unicode ? sizeof(WCHAR) : sizeof(CHAR));

        entry_info->lpszSourceUrlName = (LPSTR)entry_info+size-url_size;
        if(unicode)
            urlcache_decode_url((const char*)url_entry+url_entry->url_off, (WCHAR*)entry_info->lpszSourceUrlName, url_len);
        else
            memcpy(entry_info->lpszSourceUrlName, (LPCSTR)url_entry+url_entry->url_off, url_size);
    }

    if(size%4 && size<*info_size)
        ZeroMemory((LPBYTE)entry_info+size, 4-size%4);
    size = DWORD_ALIGN(size);

    if(url_entry->local_name_off) {
        LONG file_name_size;
        LPSTR file_name;
        file_name = (LPSTR)entry_info+size;
        file_name_size = *info_size-size;
        if((unicode && urlcache_create_file_pathW(container, header, (LPCSTR)url_entry+url_entry->local_name_off,
                        url_entry->cache_dir, (LPWSTR)file_name, &file_name_size, FALSE)) ||
            (!unicode && urlcache_create_file_pathA(container, header, (LPCSTR)url_entry+url_entry->local_name_off,
                        url_entry->cache_dir, file_name, &file_name_size))) {
            entry_info->lpszLocalFileName = file_name;
        }
        size += file_name_size;

        if(size%4 && size<*info_size)
            ZeroMemory((LPBYTE)entry_info+size, 4-size%4);
        size = DWORD_ALIGN(size);
    }

    if(url_entry->header_info_off) {
        DWORD header_len;

        if(unicode)
            header_len = MultiByteToWideChar(CP_UTF8, 0, (const char*)url_entry+url_entry->header_info_off,
                    url_entry->header_info_size, NULL, 0);
        else
            header_len = url_entry->header_info_size;
        size += header_len * (unicode ? sizeof(WCHAR) : sizeof(CHAR));

        if(*info_size >= size) {
            DWORD header_size = header_len * (unicode ? sizeof(WCHAR) : sizeof(CHAR));
            entry_info->lpHeaderInfo = (LPBYTE)entry_info+size-header_size;
            if(unicode)
                MultiByteToWideChar(CP_UTF8, 0, (const char*)url_entry+url_entry->header_info_off,
                        url_entry->header_info_size, (LPWSTR)entry_info->lpHeaderInfo, header_len);
            else
                memcpy(entry_info->lpHeaderInfo, (LPCSTR)url_entry+url_entry->header_info_off, header_len);
        }
        if(size%4 && size<*info_size)
            ZeroMemory((LPBYTE)entry_info+size, 4-size%4);
        size = DWORD_ALIGN(size);
    }

    if(url_entry->file_extension_off) {
        int ext_len;

        if(unicode)
            ext_len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)url_entry+url_entry->file_extension_off, -1, NULL, 0);
        else
            ext_len = strlen((LPCSTR)url_entry+url_entry->file_extension_off) + 1;
        size += ext_len * (unicode ? sizeof(WCHAR) : sizeof(CHAR));

        if(*info_size >= size) {
            DWORD ext_size = ext_len * (unicode ? sizeof(WCHAR) : sizeof(CHAR));
            entry_info->lpszFileExtension = (LPSTR)entry_info+size-ext_size;
            if(unicode)
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)url_entry+url_entry->file_extension_off, -1, (LPWSTR)entry_info->lpszFileExtension, ext_len);
            else
                memcpy(entry_info->lpszFileExtension, (LPCSTR)url_entry+url_entry->file_extension_off, ext_len*sizeof(CHAR));
        }

        if(size%4 && size<*info_size)
            ZeroMemory((LPBYTE)entry_info+size, 4-size%4);
        size = DWORD_ALIGN(size);
    }

    if(size > *info_size) {
        *info_size = size;
        return ERROR_INSUFFICIENT_BUFFER;
    }
    *info_size = size;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           urlcache_set_entry_info (Internal)
 *
 *  Helper for SetUrlCacheEntryInfo{A,W}. Sets fields in URL entry
 * according to the flags set by field_control.
 *
 * RETURNS
 *    ERROR_SUCCESS if the buffer was big enough
 *    ERROR_INSUFFICIENT_BUFFER if the buffer was too small
 *
 */
static DWORD urlcache_set_entry_info(entry_url *url_entry, const INTERNET_CACHE_ENTRY_INFOA *entry_info, DWORD field_control)
{
    if (field_control & CACHE_ENTRY_ACCTIME_FC)
        url_entry->access_time = entry_info->LastAccessTime;
    if (field_control & CACHE_ENTRY_ATTRIBUTE_FC)
        url_entry->cache_entry_type = entry_info->CacheEntryType;
    if (field_control & CACHE_ENTRY_EXEMPT_DELTA_FC)
        url_entry->exempt_delta = entry_info->u.dwExemptDelta;
    if (field_control & CACHE_ENTRY_EXPTIME_FC)
        file_time_to_dos_date_time(&entry_info->ExpireTime, &url_entry->expire_date, &url_entry->expire_time);
    if (field_control & CACHE_ENTRY_HEADERINFO_FC)
        FIXME("CACHE_ENTRY_HEADERINFO_FC unimplemented\n");
    if (field_control & CACHE_ENTRY_HITRATE_FC)
        url_entry->hit_rate = entry_info->dwHitRate;
    if (field_control & CACHE_ENTRY_MODTIME_FC)
        url_entry->modification_time = entry_info->LastModifiedTime;
    if (field_control & CACHE_ENTRY_SYNCTIME_FC)
        file_time_to_dos_date_time(&entry_info->LastAccessTime, &url_entry->sync_date, &url_entry->sync_time);

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           urlcache_hash_key (Internal)
 *
 *  Returns the hash key for a given string
 *
 * RETURNS
 *    hash key for the string
 *
 */
static DWORD urlcache_hash_key(LPCSTR lpszKey)
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

    for (lpszKey++; *lpszKey; lpszKey++)
    {
        for (i = 0; i < sizeof(key) / sizeof(key[0]); i++)
            key[i] = lookupTable[*lpszKey ^ key[i]];
    }

    return *(DWORD *)key;
}

static inline entry_hash_table* urlcache_get_hash_table(const urlcache_header *pHeader, DWORD dwOffset)
{
    if(!dwOffset)
        return NULL;
    return (entry_hash_table*)((LPBYTE)pHeader + dwOffset);
}

static BOOL urlcache_find_hash_entry(const urlcache_header *pHeader, LPCSTR lpszUrl, struct hash_entry **ppHashEntry)
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
    DWORD key = urlcache_hash_key(lpszUrl);
    DWORD offset = (key & (HASHTABLE_NUM_ENTRIES-1)) * HASHTABLE_BLOCKSIZE;
    entry_hash_table* pHashEntry;
    DWORD id = 0;

    key >>= HASHTABLE_FLAG_BITS;

    for (pHashEntry = urlcache_get_hash_table(pHeader, pHeader->hash_table_off);
         pHashEntry; pHashEntry = urlcache_get_hash_table(pHeader, pHashEntry->next))
    {
        int i;
        if (pHashEntry->id != id++)
        {
            ERR("Error: not right hash table number (%d) expected %d\n", pHashEntry->id, id);
            continue;
        }
        /* make sure that it is in fact a hash entry */
        if (pHashEntry->header.signature != HASH_SIGNATURE)
        {
            ERR("Error: not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&pHashEntry->header.signature);
            continue;
        }

        for (i = 0; i < HASHTABLE_BLOCKSIZE; i++)
        {
            struct hash_entry *pHashElement = &pHashEntry->hash_table[offset + i];
            if (key == pHashElement->key>>HASHTABLE_FLAG_BITS)
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

/***********************************************************************
 *           urlcache_hash_entry_set_flags (Internal)
 *
 *  Sets special bits in hash key
 *
 * RETURNS
 *    nothing
 *
 */
static void urlcache_hash_entry_set_flags(struct hash_entry *pHashEntry, DWORD dwFlag)
{
    pHashEntry->key = (pHashEntry->key >> HASHTABLE_FLAG_BITS << HASHTABLE_FLAG_BITS) | dwFlag;
}

/***********************************************************************
 *           urlcache_hash_entry_delete (Internal)
 *
 *  Searches all the hash tables in the index for the given URL and
 * then if found deletes the entry.
 *
 * RETURNS
 *    TRUE if the entry was found
 *    FALSE if the entry could not be found
 *
 */
static BOOL urlcache_hash_entry_delete(struct hash_entry *pHashEntry)
{
    pHashEntry->key = HASHTABLE_DEL;
    return TRUE;
}

/***********************************************************************
 *           urlcache_hash_entry_create (Internal)
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
static DWORD urlcache_hash_entry_create(urlcache_header *pHeader, LPCSTR lpszUrl, DWORD dwOffsetEntry, DWORD dwFieldType)
{
    /* see urlcache_find_hash_entry for structure of hash tables */

    DWORD key = urlcache_hash_key(lpszUrl);
    DWORD offset = (key & (HASHTABLE_NUM_ENTRIES-1)) * HASHTABLE_BLOCKSIZE;
    entry_hash_table* pHashEntry, *pHashPrev = NULL;
    DWORD id = 0;
    DWORD error;

    key = ((key >> HASHTABLE_FLAG_BITS) << HASHTABLE_FLAG_BITS) + dwFieldType;

    for (pHashEntry = urlcache_get_hash_table(pHeader, pHeader->hash_table_off);
         pHashEntry; pHashEntry = urlcache_get_hash_table(pHeader, pHashEntry->next))
    {
        int i;
        pHashPrev = pHashEntry;

        if (pHashEntry->id != id++)
        {
            ERR("not right hash table number (%d) expected %d\n", pHashEntry->id, id);
            break;
        }
        /* make sure that it is in fact a hash entry */
        if (pHashEntry->header.signature != HASH_SIGNATURE)
        {
            ERR("not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&pHashEntry->header.signature);
            break;
        }

        for (i = 0; i < HASHTABLE_BLOCKSIZE; i++)
        {
            struct hash_entry *pHashElement = &pHashEntry->hash_table[offset + i];
            if (pHashElement->key==HASHTABLE_FREE || pHashElement->key==HASHTABLE_DEL) /* if the slot is free */
            {
                pHashElement->key = key;
                pHashElement->offset = dwOffsetEntry;
                return ERROR_SUCCESS;
            }
        }
    }
    error = urlcache_create_hash_table(pHeader, pHashPrev, &pHashEntry);
    if (error != ERROR_SUCCESS)
        return error;

    pHashEntry->hash_table[offset].key = key;
    pHashEntry->hash_table[offset].offset = dwOffsetEntry;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           urlcache_enum_hash_tables (Internal)
 *
 *  Enumerates the hash tables in a container.
 *
 * RETURNS
 *    TRUE if an entry was found
 *    FALSE if there are no more tables to enumerate.
 *
 */
static BOOL urlcache_enum_hash_tables(const urlcache_header *pHeader, DWORD *id, entry_hash_table **ppHashEntry)
{
    for (*ppHashEntry = urlcache_get_hash_table(pHeader, pHeader->hash_table_off);
         *ppHashEntry; *ppHashEntry = urlcache_get_hash_table(pHeader, (*ppHashEntry)->next))
    {
        TRACE("looking at hash table number %d\n", (*ppHashEntry)->id);
        if ((*ppHashEntry)->id != *id)
            continue;
        /* make sure that it is in fact a hash entry */
        if ((*ppHashEntry)->header.signature != HASH_SIGNATURE)
        {
            ERR("Error: not right signature (\"%.4s\") - expected \"HASH\"\n", (LPCSTR)&(*ppHashEntry)->header.signature);
            (*id)++;
            continue;
        }

        TRACE("hash table number %d found\n", *id);
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           urlcache_enum_hash_table_entries (Internal)
 *
 *  Enumerates entries in a hash table and returns the next non-free entry.
 *
 * RETURNS
 *    TRUE if an entry was found
 *    FALSE if the hash table is empty or there are no more entries to
 *     enumerate.
 *
 */
static BOOL urlcache_enum_hash_table_entries(const urlcache_header *pHeader, const entry_hash_table *pHashEntry,
                                          DWORD * index, const struct hash_entry **ppHashEntry)
{
    for (; *index < HASHTABLE_SIZE ; (*index)++)
    {
        if (pHashEntry->hash_table[*index].key==HASHTABLE_FREE || pHashEntry->hash_table[*index].key==HASHTABLE_DEL)
            continue;

        *ppHashEntry = &pHashEntry->hash_table[*index];
        TRACE("entry found %d\n", *index);
        return TRUE;
    }
    TRACE("no more entries (%d)\n", *index);
    return FALSE;
}

/***********************************************************************
 *           cache_container_delete_dir (Internal)
 *
 *  Erase a directory containing an URL cache.
 *
 * RETURNS
 *    TRUE success, FALSE failure/aborted.
 *
 */
static BOOL cache_container_delete_dir(LPCWSTR lpszPath)
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
 *           urlcache_hash_entry_is_locked (Internal)
 *
 *  Checks if entry is locked. Unlocks it if possible.
 */
static BOOL urlcache_hash_entry_is_locked(struct hash_entry *hash_entry, entry_url *url_entry)
{
    FILETIME cur_time;
    ULARGE_INTEGER acc_time, time;

    if ((hash_entry->key & ((1<<HASHTABLE_FLAG_BITS)-1)) != HASHTABLE_LOCK)
        return FALSE;

    GetSystemTimeAsFileTime(&cur_time);
    time.u.LowPart = cur_time.dwLowDateTime;
    time.u.HighPart = cur_time.dwHighDateTime;

    acc_time.u.LowPart = url_entry->access_time.dwLowDateTime;
    acc_time.u.HighPart = url_entry->access_time.dwHighDateTime;

    time.QuadPart -= acc_time.QuadPart;

    /* check if entry was locked for at least a day */
    if(time.QuadPart > (ULONGLONG)24*60*60*FILETIME_SECOND) {
        urlcache_hash_entry_set_flags(hash_entry, HASHTABLE_URL);
        url_entry->use_count = 0;
        return FALSE;
    }

    return TRUE;
}

static BOOL urlcache_get_entry_info(const char *url, void *entry_info,
        DWORD *size, DWORD flags, BOOL unicode)
{
    urlcache_header *header;
    struct hash_entry *hash_entry;
    const entry_url *url_entry;
    cache_container *container;
    DWORD error;

    TRACE("(%s, %p, %p, %x, %x)\n", debugstr_a(url), entry_info, size, flags, unicode);

    if(flags & ~GET_INSTALLED_ENTRY)
        FIXME("ignoring unsupported flags: %x\n", flags);

    error = cache_containers_find(url, &container);
    if(error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(container, MIN_BLOCK_NO);
    if(error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    if(!(header = cache_container_lock_index(container)))
        return FALSE;

    if(!urlcache_find_hash_entry(header, url, &hash_entry)) {
        cache_container_unlock_index(container, header);
        WARN("entry %s not found!\n", debugstr_a(url));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    url_entry = (const entry_url*)((LPBYTE)header + hash_entry->offset);
    if(url_entry->header.signature != URL_SIGNATURE) {
        cache_container_unlock_index(container, header);
        FIXME("Trying to retrieve entry of unknown format %s\n",
                debugstr_an((LPCSTR)&url_entry->header.signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    TRACE("Found URL: %s\n", debugstr_a((LPCSTR)url_entry + url_entry->url_off));
    TRACE("Header info: %s\n", debugstr_an((LPCSTR)url_entry +
                url_entry->header_info_off, url_entry->header_info_size));

    if((flags & GET_INSTALLED_ENTRY) && !(url_entry->cache_entry_type & INSTALLED_CACHE_ENTRY)) {
        cache_container_unlock_index(container, header);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if(size) {
        if(!entry_info)
            *size = 0;

        error = urlcache_copy_entry(container, header, entry_info, size, url_entry, unicode);
        if(error != ERROR_SUCCESS) {
            cache_container_unlock_index(container, header);
            SetLastError(error);
            return FALSE;
        }
        if(url_entry->local_name_off)
            TRACE("Local File Name: %s\n", debugstr_a((LPCSTR)url_entry + url_entry->local_name_off));
    }

    cache_container_unlock_index(container, header);
    return TRUE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExA(LPCSTR lpszUrl,
        LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
        LPDWORD lpdwCacheEntryInfoBufSize, LPSTR lpszReserved,
        LPDWORD lpdwReserved, LPVOID lpReserved, DWORD dwFlags)
{
    if(lpszReserved!=NULL || lpdwReserved!=NULL || lpReserved!=NULL) {
        ERR("Reserved value was not 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return urlcache_get_entry_info(lpszUrl, lpCacheEntryInfo,
            lpdwCacheEntryInfoBufSize, dwFlags, FALSE);
}

/***********************************************************************
 *           GetUrlCacheEntryInfoA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoA(LPCSTR lpszUrlName,
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    LPDWORD lpdwCacheEntryInfoBufferSize)
{
    return GetUrlCacheEntryInfoExA(lpszUrlName, lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize, NULL, NULL, NULL, 0);
}

static int urlcache_encode_url(const WCHAR *url, char *encoded_url, int encoded_len)
{
    URL_COMPONENTSW uc;
    DWORD len, part_len;
    WCHAR *punycode;

    TRACE("%s\n", debugstr_w(url));

    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.dwHostNameLength = 1;
    if(!InternetCrackUrlW(url, 0, 0, &uc))
        uc.nScheme = INTERNET_SCHEME_UNKNOWN;

    if(uc.nScheme!=INTERNET_SCHEME_HTTP && uc.nScheme!=INTERNET_SCHEME_HTTPS)
        return WideCharToMultiByte(CP_UTF8, 0, url, -1, encoded_url, encoded_len, NULL, NULL);

    len = WideCharToMultiByte(CP_UTF8, 0, url, uc.lpszHostName-url,
            encoded_url, encoded_len, NULL, NULL);
    if(!len)
        return 0;
    if(encoded_url)
        encoded_len -= len;

    part_len = IdnToAscii(0, uc.lpszHostName, uc.dwHostNameLength, NULL, 0);
    if(!part_len) {
        SetLastError(ERROR_INTERNET_INVALID_URL);
        return 0;
    }

    punycode = heap_alloc(part_len*sizeof(WCHAR));
    if(!punycode)
        return 0;

    part_len = IdnToAscii(0, uc.lpszHostName, uc.dwHostNameLength, punycode, part_len);
    if(!part_len) {
        heap_free(punycode);
        return 0;
    }

    part_len = WideCharToMultiByte(CP_UTF8, 0, punycode, part_len,
            encoded_url ? encoded_url+len : NULL, encoded_len, NULL, NULL);
    heap_free(punycode);
    if(!part_len)
        return 0;
    if(encoded_url)
        encoded_len -= part_len;
    len += part_len;

    part_len = WideCharToMultiByte(CP_UTF8, 0, uc.lpszHostName+uc.dwHostNameLength,
            -1, encoded_url ? encoded_url+len : NULL, encoded_len, NULL, NULL);
    if(!part_len)
        return 0;
    len += part_len;

    TRACE("got (%d)%s\n", len, debugstr_a(encoded_url));
    return len;
}

static BOOL urlcache_encode_url_alloc(const WCHAR *url, char **encoded_url)
{
    DWORD encoded_len;
    char *ret;

    encoded_len = urlcache_encode_url(url, NULL, 0);
    if(!encoded_len)
        return FALSE;

    ret = heap_alloc(encoded_len*sizeof(WCHAR));
    if(!ret)
        return FALSE;

    encoded_len = urlcache_encode_url(url, ret, encoded_len);
    if(!encoded_len) {
        heap_free(ret);
        return FALSE;
    }

    *encoded_url = ret;
    return TRUE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExW (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExW(LPCWSTR lpszUrl,
        LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
        LPDWORD lpdwCacheEntryInfoBufSize, LPWSTR lpszReserved,
        LPDWORD lpdwReserved, LPVOID lpReserved, DWORD dwFlags)
{
    char *url;
    BOOL ret;

    if(lpszReserved!=NULL || lpdwReserved!=NULL || lpReserved!=NULL) {
        ERR("Reserved value was not 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Ignore GET_INSTALLED_ENTRY flag in unicode version of function */
    dwFlags &= ~GET_INSTALLED_ENTRY;

    if(!urlcache_encode_url_alloc(lpszUrl, &url))
        return FALSE;

    ret = urlcache_get_entry_info(url, lpCacheEntryInfo,
            lpdwCacheEntryInfoBufSize, dwFlags, TRUE);
    heap_free(url);
    return ret;
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
 *           SetUrlCacheEntryInfoA (WININET.@)
 */
BOOL WINAPI SetUrlCacheEntryInfoA(LPCSTR lpszUrlName,
        LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
        DWORD dwFieldControl)
{
    urlcache_header *pHeader;
    struct hash_entry *pHashEntry;
    entry_header *pEntry;
    cache_container *pContainer;
    DWORD error;

    TRACE("(%s, %p, 0x%08x)\n", debugstr_a(lpszUrlName), lpCacheEntryInfo, dwFieldControl);

    error = cache_containers_find(lpszUrlName, &pContainer);
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

    if (!urlcache_find_hash_entry(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        WARN("entry %s not found!\n", debugstr_a(lpszUrlName));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->offset);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    urlcache_set_entry_info((entry_url*)pEntry, lpCacheEntryInfo, dwFieldControl);

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           SetUrlCacheEntryInfoW (WININET.@)
 */
BOOL WINAPI SetUrlCacheEntryInfoW(LPCWSTR lpszUrl,
        LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
        DWORD dwFieldControl)
{
    char *url;
    BOOL ret;

    if(!urlcache_encode_url_alloc(lpszUrl, &url))
        return FALSE;

    ret = SetUrlCacheEntryInfoA(url, (INTERNET_CACHE_ENTRY_INFOA*)lpCacheEntryInfo, dwFieldControl);
    heap_free(url);
    return ret;
}

static BOOL urlcache_entry_get_file(const char *url, void *entry_info, DWORD *size, BOOL unicode)
{
    urlcache_header *header;
    struct hash_entry *hash_entry;
    entry_url *url_entry;
    cache_container *container;
    DWORD error;

    TRACE("(%s, %p, %p, %x)\n", debugstr_a(url), entry_info, size, unicode);

    if(!url || !size || (!entry_info && *size)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    error = cache_containers_find(url, &container);
    if(error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(container, MIN_BLOCK_NO);
    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    if (!(header = cache_container_lock_index(container)))
        return FALSE;

    if (!urlcache_find_hash_entry(header, url, &hash_entry)) {
        cache_container_unlock_index(container, header);
        TRACE("entry %s not found!\n", url);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    url_entry = (entry_url*)((LPBYTE)header + hash_entry->offset);
    if(url_entry->header.signature != URL_SIGNATURE) {
        cache_container_unlock_index(container, header);
        FIXME("Trying to retrieve entry of unknown format %s\n",
                debugstr_an((LPSTR)&url_entry->header.signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if(!url_entry->local_name_off) {
        cache_container_unlock_index(container, header);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    TRACE("Found URL: %s\n", debugstr_a((LPCSTR)url_entry + url_entry->url_off));
    TRACE("Header info: %s\n", debugstr_an((LPCSTR)url_entry + url_entry->header_info_off,
                url_entry->header_info_size));

    error = urlcache_copy_entry(container, header, entry_info,
            size, url_entry, unicode);
    if(error != ERROR_SUCCESS) {
        cache_container_unlock_index(container, header);
        SetLastError(error);
        return FALSE;
    }
    TRACE("Local File Name: %s\n", debugstr_a((LPCSTR)url_entry + url_entry->local_name_off));

    url_entry->hit_rate++;
    url_entry->use_count++;
    urlcache_hash_entry_set_flags(hash_entry, HASHTABLE_LOCK);
    GetSystemTimeAsFileTime(&url_entry->access_time);

    cache_container_unlock_index(container, header);

    return TRUE;
}

/***********************************************************************
 *           RetrieveUrlCacheEntryFileA (WININET.@)
 *
 */
BOOL WINAPI RetrieveUrlCacheEntryFileA(LPCSTR lpszUrlName,
        LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
        LPDWORD lpdwCacheEntryInfoBufferSize, DWORD dwReserved)
{
    return urlcache_entry_get_file(lpszUrlName, lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize, FALSE);
}

/***********************************************************************
 *           RetrieveUrlCacheEntryFileW (WININET.@)
 *
 */
BOOL WINAPI RetrieveUrlCacheEntryFileW(LPCWSTR lpszUrlName,
        LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
        LPDWORD lpdwCacheEntryInfoBufferSize, DWORD dwReserved)
{
    char *url;
    BOOL ret;

    if(!urlcache_encode_url_alloc(lpszUrlName, &url))
        return FALSE;

    ret = urlcache_entry_get_file(url, lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize, TRUE);
    heap_free(url);
    return ret;
}

static BOOL urlcache_entry_delete(const cache_container *pContainer,
        urlcache_header *pHeader, struct hash_entry *pHashEntry)
{
    entry_header *pEntry;
    entry_url * pUrlEntry;

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->offset);
    if (pEntry->signature != URL_SIGNATURE)
    {
        FIXME("Trying to delete entry of unknown format %s\n",
              debugstr_an((LPCSTR)&pEntry->signature, sizeof(DWORD)));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pUrlEntry = (entry_url *)pEntry;
    if(urlcache_hash_entry_is_locked(pHashEntry, pUrlEntry))
    {
        TRACE("Trying to delete locked entry\n");
        pUrlEntry->cache_entry_type |= PENDING_DELETE_CACHE_ENTRY;
        SetLastError(ERROR_SHARING_VIOLATION);
        return FALSE;
    }

    if(!urlcache_delete_file(pContainer, pHeader, pUrlEntry))
    {
        urlcache_entry_free(pHeader, pEntry);
    }
    else
    {
        /* Add entry to leaked files list */
        pUrlEntry->header.signature = LEAK_SIGNATURE;
        pUrlEntry->exempt_delta = pHeader->options[CACHE_HEADER_DATA_ROOT_LEAK_OFFSET];
        pHeader->options[CACHE_HEADER_DATA_ROOT_LEAK_OFFSET] = pHashEntry->offset;
    }

    urlcache_hash_entry_delete(pHashEntry);
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
static BOOL urlcache_next_entry(urlcache_header *header, DWORD *hash_table_off, DWORD *hash_table_entry,
        struct hash_entry **hash_entry, entry_header **entry)
{
    entry_hash_table *hashtable_entry;

    *hash_entry = NULL;
    *entry = NULL;

    if(!*hash_table_off) {
        *hash_table_off = header->hash_table_off;
        *hash_table_entry = 0;

        hashtable_entry = urlcache_get_hash_table(header, *hash_table_off);
    }else {
        if(*hash_table_off >= header->size) {
            *hash_table_off = 0;
            return FALSE;
        }

        hashtable_entry = urlcache_get_hash_table(header, *hash_table_off);
    }

    if(hashtable_entry->header.signature != HASH_SIGNATURE) {
        *hash_table_off = 0;
        return FALSE;
    }

    while(1) {
        if(*hash_table_entry >= HASHTABLE_SIZE) {
            *hash_table_off = hashtable_entry->next;
            if(!*hash_table_off) {
                *hash_table_off = 0;
                return FALSE;
            }

            hashtable_entry = urlcache_get_hash_table(header, *hash_table_off);
            *hash_table_entry = 0;
        }

        if(hashtable_entry->hash_table[*hash_table_entry].key != HASHTABLE_DEL &&
            hashtable_entry->hash_table[*hash_table_entry].key != HASHTABLE_FREE) {
            *hash_entry = &hashtable_entry->hash_table[*hash_table_entry];
            *entry = (entry_header*)((LPBYTE)header + hashtable_entry->hash_table[*hash_table_entry].offset);
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
    cache_container *container;
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
        LIST_FOR_EACH_ENTRY(container, &UrlContainers, cache_container, entry)
        {
            /* When cache_path==NULL only clean Temporary Internet Files */
            if((!path_len && container->cache_prefix[0]==0) ||
                    (path_len && !strncmpiW(container->path, cache_path, path_len) &&
                     (container->path[path_len]=='\0' || container->path[path_len]=='\\')))
            {
                BOOL ret_del;

                WaitForSingleObject(container->mutex, INFINITE);

                /* unlock, delete, recreate and lock cache */
                cache_container_close_index(container);
                ret_del = cache_container_delete_dir(container->path);
                err = cache_container_open_index(container, MIN_BLOCK_NO);

                ReleaseMutex(container->mutex);
                if(!ret_del || (err != ERROR_SUCCESS))
                    return FALSE;
            }
        }

        return TRUE;
    }

    LIST_FOR_EACH_ENTRY(container, &UrlContainers, cache_container, entry)
    {
        urlcache_header *header;
        struct hash_entry *hash_entry;
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

        desired_size = header->cache_limit.QuadPart*(100-size)/100;
        cur_size = header->cache_usage.QuadPart+header->exempt_usage.QuadPart;
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
                urlcache_entry_delete(container, header, hash_entry);

                if(header->cache_usage.QuadPart+header->exempt_usage.QuadPart <= desired_size)
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
                wine_dbgstr_longlong(header->cache_usage.QuadPart+header->exempt_usage.QuadPart),
                wine_dbgstr_longlong(header->cache_limit.QuadPart));
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
BOOL WINAPI UnlockUrlCacheEntryFileA(LPCSTR lpszUrlName, DWORD dwReserved)
{
    urlcache_header *pHeader;
    struct hash_entry *pHashEntry;
    entry_header *pEntry;
    entry_url * pUrlEntry;
    cache_container *pContainer;
    DWORD error;

    TRACE("(%s, 0x%08x)\n", debugstr_a(lpszUrlName), dwReserved);

    if (dwReserved)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    error = cache_containers_find(lpszUrlName, &pContainer);
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

    if (!urlcache_find_hash_entry(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", lpszUrlName);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    pEntry = (entry_header*)((LPBYTE)pHeader + pHashEntry->offset);
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
        urlcache_hash_entry_set_flags(pHashEntry, HASHTABLE_URL);
        if (pUrlEntry->cache_entry_type & PENDING_DELETE_CACHE_ENTRY)
            urlcache_entry_delete(pContainer, pHeader, pHashEntry);
    }

    cache_container_unlock_index(pContainer, pHeader);

    return TRUE;
}

/***********************************************************************
 *           UnlockUrlCacheEntryFileW (WININET.@)
 *
 */
BOOL WINAPI UnlockUrlCacheEntryFileW(LPCWSTR lpszUrlName, DWORD dwReserved)
{
    char *url;
    BOOL ret;

    if(!urlcache_encode_url_alloc(lpszUrlName, &url))
        return FALSE;

    ret = UnlockUrlCacheEntryFileA(url, dwReserved);
    heap_free(url);
    return ret;
}

static BOOL urlcache_entry_create(const char *url, const char *ext, WCHAR *full_path)
{
    cache_container *container;
    urlcache_header *header;
    char file_name[MAX_PATH];
    WCHAR extW[MAX_PATH];
    BYTE cache_dir;
    LONG full_path_len, ext_len = 0;
    BOOL generate_name = FALSE;
    DWORD error;
    HANDLE file;
    FILETIME ft;
    URL_COMPONENTSA uc;
    int i;

    TRACE("(%s, %s, %p)\n", debugstr_a(url), debugstr_a(ext), full_path);

    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.dwUrlPathLength = 1;
    uc.dwExtraInfoLength = 1;
    if(!InternetCrackUrlA(url, 0, 0, &uc))
        uc.dwUrlPathLength = 0;

    if(!uc.dwUrlPathLength) {
        file_name[0] = 0;
    }else {
        char *p, *e;

        p = e = uc.lpszUrlPath+uc.dwUrlPathLength;
        while(p>uc.lpszUrlPath && *(p-1)!='/' && *(p-1)!='\\' && *(p-1)!='.')
            p--;
        if(p>uc.lpszUrlPath && *(p-1)=='.') {
            e = p-1;
            while(p>uc.lpszUrlPath && *(p-1)!='/' && *(p-1)!='\\')
                p--;
        }

        if(e-p >= MAX_PATH)
            e = p+MAX_PATH-1;
        memcpy(file_name, p, e-p);
        file_name[e-p] = 0;

        for(p=file_name; *p; p++) {
            switch(*p) {
            case '<': case '>':
            case ':': case '"':
            case '|': case '?':
            case '*':
                *p = '_'; break;
            default: break;
            }
        }
    }

    if(!file_name[0])
        generate_name = TRUE;

    error = cache_containers_find(url, &container);
    if(error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(container, MIN_BLOCK_NO);
    if(error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    if(!(header = cache_container_lock_index(container)))
        return FALSE;

    if(header->dirs_no)
        cache_dir = (BYTE)(rand() % header->dirs_no);
    else
        cache_dir = CACHE_CONTAINER_NO_SUBDIR;

    full_path_len = MAX_PATH * sizeof(WCHAR);
    if(!urlcache_create_file_pathW(container, header, file_name, cache_dir, full_path, &full_path_len, TRUE)) {
        WARN("Failed to get full path for filename %s, needed %u bytes.\n",
                debugstr_a(file_name), full_path_len);
        cache_container_unlock_index(container, header);
        return FALSE;
    }
    full_path_len = full_path_len/sizeof(WCHAR) - 1;

    cache_container_unlock_index(container, header);

    if(ext) {
        WCHAR *p;

        extW[0] = '.';
        ext_len = MultiByteToWideChar(CP_ACP, 0, ext, -1, extW+1, MAX_PATH-1);

        for(p=extW; *p; p++) {
            switch(*p) {
            case '<': case '>':
            case ':': case '"':
            case '|': case '?':
            case '*':
                *p = '_'; break;
            default: break;
            }
        }
        if(p[-1]==' ' || p[-1]=='.')
            p[-1] = '_';
    }else {
        extW[0] = '\0';
    }

    if(!generate_name && full_path_len+5+ext_len>=MAX_PATH) { /* strlen("[255]") = 5 */
        full_path_len = MAX_PATH-5-ext_len-1;
    }

    for(i=0; i<255 && !generate_name; i++) {
        static const WCHAR format[] = {'[','%','u',']','%','s',0};

        wsprintfW(full_path+full_path_len, format, i, extW);

        TRACE("Trying: %s\n", debugstr_w(full_path));
        file = CreateFileW(full_path, GENERIC_READ, 0, NULL, CREATE_NEW, 0, NULL);
        if(file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
            return TRUE;
        }
    }

    if(full_path_len+8+ext_len >= MAX_PATH)
        full_path_len = MAX_PATH-8-ext_len-1;

    /* Try to generate random name */
    GetSystemTimeAsFileTime(&ft);
    strcpyW(full_path+full_path_len+8, extW);

    for(i=0; i<255; i++) {
        int j;
        ULONGLONG n = ft.dwHighDateTime;
        n <<= 32;
        n += ft.dwLowDateTime;
        n ^= (ULONGLONG)i<<48;

        for(j=0; j<8; j++) {
            int r = (n % 36);
            n /= 37;
            full_path[full_path_len+j] = (r < 10 ? '0' + r : 'A' + r - 10);
        }

        TRACE("Trying: %s\n", debugstr_w(full_path));
        file = CreateFileW(full_path, GENERIC_READ, 0, NULL, CREATE_NEW, 0, NULL);
        if(file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
            return TRUE;
        }
    }

    WARN("Could not find a unique filename\n");
    return FALSE;
}

/***********************************************************************
 *           CreateUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI CreateUrlCacheEntryA(LPCSTR lpszUrlName, DWORD dwExpectedFileSize,
        LPCSTR lpszFileExtension, LPSTR lpszFileName, DWORD dwReserved)
{
    WCHAR file_name[MAX_PATH];

    if(dwReserved)
        FIXME("dwReserved 0x%08x\n", dwReserved);

    if(!urlcache_entry_create(lpszUrlName, lpszFileExtension, file_name))
        return FALSE;

    if(!WideCharToMultiByte(CP_ACP, 0, file_name, -1, lpszFileName, MAX_PATH, NULL, NULL))
        return FALSE;
    return TRUE;
}
/***********************************************************************
 *           CreateUrlCacheEntryW (WININET.@)
 *
 */
BOOL WINAPI CreateUrlCacheEntryW(LPCWSTR lpszUrlName, DWORD dwExpectedFileSize,
        LPCWSTR lpszFileExtension, LPWSTR lpszFileName, DWORD dwReserved)
{
    char *url, *ext = NULL;
    BOOL ret;

    if(dwReserved)
        FIXME("dwReserved 0x%08x\n", dwReserved);

    if(lpszFileExtension) {
        ext = heap_strdupWtoUTF8(lpszFileExtension);
        if(!ext)
            return FALSE;
    }

    if(!urlcache_encode_url_alloc(lpszUrlName, &url)) {
        heap_free(ext);
        return FALSE;
    }

    ret = urlcache_entry_create(url, ext, lpszFileName);
    heap_free(ext);
    heap_free(url);
    return ret;
}

static BOOL urlcache_entry_commit(const char *url, const WCHAR *file_name,
    FILETIME expire_time, FILETIME modify_time, DWORD entry_type,
    BYTE *header_info, DWORD header_size, const char *file_ext,
    const char *original_url)
{
    cache_container *container;
    urlcache_header *header;
    struct hash_entry *hash_entry;
    entry_header *entry;
    entry_url *url_entry;
    DWORD url_entry_offset;
    DWORD size = DWORD_ALIGN(sizeof(*url_entry));
    DWORD file_name_off = 0;
    DWORD header_info_off = 0;
    DWORD file_ext_off = 0;
    WIN32_FILE_ATTRIBUTE_DATA file_attr;
    LARGE_INTEGER file_size;
    BYTE dir_id;
    char file_name_no_container[MAX_PATH];
    char *local_file_name = 0;
    DWORD hit_rate = 0;
    DWORD exempt_delta = 0;
    DWORD error;

    TRACE("(%s, %s, ..., ..., %x, %p, %d, %s, %s)\n", debugstr_a(url), debugstr_w(file_name),
            entry_type, header_info, header_size, debugstr_a(file_ext), debugstr_a(original_url));

    if(entry_type & STICKY_CACHE_ENTRY && !file_name) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if(original_url)
        WARN(": original_url ignored\n");

    memset(&file_attr, 0, sizeof(file_attr));
    if(file_name) {
        if(!GetFileAttributesExW(file_name, GetFileExInfoStandard, &file_attr))
            return FALSE;
    }
    file_size.u.LowPart = file_attr.nFileSizeLow;
    file_size.u.HighPart = file_attr.nFileSizeHigh;

    error = cache_containers_find(url, &container);
    if(error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    error = cache_container_open_index(container, MIN_BLOCK_NO);
    if(error != ERROR_SUCCESS) {
        SetLastError(error);
        return FALSE;
    }

    if(!(header = cache_container_lock_index(container)))
        return FALSE;

    if(urlcache_find_hash_entry(header, url, &hash_entry)) {
        entry_url *url_entry = (entry_url*)((LPBYTE)header + hash_entry->offset);

        if(urlcache_hash_entry_is_locked(hash_entry, url_entry)) {
            TRACE("Trying to overwrite locked entry\n");
            cache_container_unlock_index(container, header);
            SetLastError(ERROR_SHARING_VIOLATION);
            return FALSE;
        }

        hit_rate = url_entry->hit_rate;
        exempt_delta = url_entry->exempt_delta;
        urlcache_entry_delete(container, header, hash_entry);
    }

    if(header->dirs_no)
        dir_id = 0;
    else
        dir_id = CACHE_CONTAINER_NO_SUBDIR;

    if(file_name) {
        BOOL bFound = FALSE;

        if(strncmpW(file_name, container->path, lstrlenW(container->path))) {
            ERR("path %s must begin with cache content path %s\n", debugstr_w(file_name), debugstr_w(container->path));
            cache_container_unlock_index(container, header);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        /* skip container path prefix */
        file_name += lstrlenW(container->path);

        WideCharToMultiByte(CP_ACP, 0, file_name, -1, file_name_no_container, MAX_PATH, NULL, NULL);
	local_file_name = file_name_no_container;

        if(header->dirs_no) {
            for(dir_id = 0; dir_id < header->dirs_no; dir_id++) {
                if(!strncmp(header->directory_data[dir_id].name, local_file_name, DIR_LENGTH)) {
                    bFound = TRUE;
                    break;
                }
            }

            if(!bFound) {
                ERR("cache directory not found in path %s\n", debugstr_w(file_name));
                cache_container_unlock_index(container, header);
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            file_name += DIR_LENGTH + 1;
            local_file_name += DIR_LENGTH + 1;
        }
    }

    size = DWORD_ALIGN(size + strlen(url) + 1);
    if(file_name) {
        file_name_off = size;
        size = DWORD_ALIGN(size + strlen(local_file_name) + 1);
    }
    if(header_info && header_size) {
        header_info_off = size;
        size = DWORD_ALIGN(size + header_size);
    }
    if(file_ext && (file_ext_off = strlen(file_ext))) {
        DWORD len = file_ext_off;

        file_ext_off = size;
        size = DWORD_ALIGN(size + len + 1);
    }

    /* round up to next block */
    if(size % BLOCKSIZE) {
        size -= size % BLOCKSIZE;
        size += BLOCKSIZE;
    }

    error = urlcache_entry_alloc(header, size / BLOCKSIZE, &entry);
    while(error == ERROR_HANDLE_DISK_FULL) {
        error = cache_container_clean_index(container, &header);
        if(error == ERROR_SUCCESS)
            error = urlcache_entry_alloc(header, size / BLOCKSIZE, &entry);
    }
    if(error != ERROR_SUCCESS) {
        cache_container_unlock_index(container, header);
        SetLastError(error);
        return FALSE;
    }

    /* FindFirstFreeEntry fills in blocks used */
    url_entry = (entry_url *)entry;
    url_entry_offset = (LPBYTE)url_entry - (LPBYTE)header;
    url_entry->header.signature = URL_SIGNATURE;
    url_entry->cache_dir = dir_id;
    url_entry->cache_entry_type = entry_type | container->default_entry_type;
    url_entry->header_info_size = header_size;
    if((entry_type & STICKY_CACHE_ENTRY) && !exempt_delta) {
        /* Sticky entries have a default exempt time of one day */
        exempt_delta = 86400;
    }
    url_entry->exempt_delta = exempt_delta;
    url_entry->hit_rate = hit_rate+1;
    url_entry->file_extension_off = file_ext_off;
    url_entry->header_info_off = header_info_off;
    url_entry->local_name_off = file_name_off;
    url_entry->url_off = DWORD_ALIGN(sizeof(*url_entry));
    url_entry->size.QuadPart = file_size.QuadPart;
    url_entry->use_count = 0;
    GetSystemTimeAsFileTime(&url_entry->access_time);
    url_entry->modification_time = modify_time;
    file_time_to_dos_date_time(&url_entry->access_time, &url_entry->sync_date, &url_entry->sync_time);
    file_time_to_dos_date_time(&expire_time, &url_entry->expire_date, &url_entry->expire_time);
    file_time_to_dos_date_time(&file_attr.ftLastWriteTime, &url_entry->write_date, &url_entry->write_time);

    /*** Unknowns ***/
    url_entry->unk1 = 0;
    url_entry->unk2 = 0;
    url_entry->unk3 = 0x60;
    url_entry->unk4 = 0;
    url_entry->unk5 = 0x1010;
    url_entry->unk7 = 0;
    url_entry->unk8 = 0;


    strcpy((LPSTR)url_entry + url_entry->url_off, url);
    if(file_name_off)
        strcpy((LPSTR)((LPBYTE)url_entry + file_name_off), local_file_name);
    if(header_info_off)
        memcpy((LPBYTE)url_entry + header_info_off, header_info, header_size);
    if(file_ext_off)
        strcpy((LPSTR)((LPBYTE)url_entry + file_ext_off), file_ext);

    error = urlcache_hash_entry_create(header, url, url_entry_offset, HASHTABLE_URL);
    while(error == ERROR_HANDLE_DISK_FULL) {
        error = cache_container_clean_index(container, &header);
        if(error == ERROR_SUCCESS) {
            url_entry = (entry_url *)((LPBYTE)header + url_entry_offset);
            error = urlcache_hash_entry_create(header, url,
                    url_entry_offset, HASHTABLE_URL);
        }
    }
    if(error != ERROR_SUCCESS) {
        urlcache_entry_free(header, &url_entry->header);
        cache_container_unlock_index(container, header);
        SetLastError(error);
        return FALSE;
    }

    if(url_entry->cache_dir < header->dirs_no)
        header->directory_data[url_entry->cache_dir].files_no++;
    if(entry_type & STICKY_CACHE_ENTRY)
        header->exempt_usage.QuadPart += file_size.QuadPart;
    else
        header->cache_usage.QuadPart += file_size.QuadPart;
    if(header->cache_usage.QuadPart+header->exempt_usage.QuadPart > header->cache_limit.QuadPart)
            handle_full_cache();

    cache_container_unlock_index(container, header);
    return TRUE;
}

/***********************************************************************
 *           CommitUrlCacheEntryA (WININET.@)
 */
BOOL WINAPI CommitUrlCacheEntryA(LPCSTR lpszUrlName, LPCSTR lpszLocalFileName,
        FILETIME ExpireTime, FILETIME LastModifiedTime, DWORD CacheEntryType,
        LPBYTE lpHeaderInfo, DWORD dwHeaderSize, LPCSTR lpszFileExtension, LPCSTR lpszOriginalUrl)
{
    WCHAR *file_name = NULL;
    BOOL ret;

    if(lpszLocalFileName) {
        file_name = heap_strdupAtoW(lpszLocalFileName);
        if(!file_name)
            return FALSE;
    }

    ret = urlcache_entry_commit(lpszUrlName, file_name, ExpireTime, LastModifiedTime,
            CacheEntryType, lpHeaderInfo, dwHeaderSize, lpszFileExtension, lpszOriginalUrl);
    heap_free(file_name);
    return ret;
}

/***********************************************************************
 *           CommitUrlCacheEntryW (WININET.@)
 */
BOOL WINAPI CommitUrlCacheEntryW(LPCWSTR lpszUrlName, LPCWSTR lpszLocalFileName,
        FILETIME ExpireTime, FILETIME LastModifiedTime, DWORD CacheEntryType,
        LPWSTR lpHeaderInfo, DWORD dwHeaderSize, LPCWSTR lpszFileExtension, LPCWSTR lpszOriginalUrl)
{
    char *url, *original_url=NULL, *file_ext=NULL, *header_info=NULL;
    BOOL ret;

    if(!urlcache_encode_url_alloc(lpszUrlName, &url))
        return FALSE;

    if(lpHeaderInfo) {
        header_info = heap_strdupWtoUTF8(lpHeaderInfo);
        if(!header_info) {
            heap_free(url);
            return FALSE;
        }
        dwHeaderSize = strlen(header_info);
    }

    if(lpszFileExtension) {
        file_ext = heap_strdupWtoA(lpszFileExtension);
        if(!file_ext) {
            heap_free(url);
            heap_free(header_info);
            return FALSE;
        }
    }

    if(lpszOriginalUrl && !urlcache_encode_url_alloc(lpszOriginalUrl, &original_url)) {
        heap_free(url);
        heap_free(header_info);
        heap_free(file_ext);
        return FALSE;
    }

    ret = urlcache_entry_commit(url, lpszLocalFileName, ExpireTime, LastModifiedTime,
            CacheEntryType, (BYTE*)header_info, dwHeaderSize, file_ext, original_url);
    heap_free(url);
    heap_free(header_info);
    heap_free(file_ext);
    heap_free(original_url);
    return ret;
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
    stream_handle *pStream = (stream_handle*)hUrlCacheStream;

    if (dwReserved != 0)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IsBadReadPtr(pStream, sizeof(*pStream)) || IsBadStringPtrA(pStream->url, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (SetFilePointer(pStream->file, dwLocation, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        return FALSE;
    return ReadFile(pStream->file, lpBuffer, *lpdwLen, lpdwLen, NULL);
}

/***********************************************************************
 *           RetrieveUrlCacheEntryStreamA (WININET.@)
 *
 */
HANDLE WINAPI RetrieveUrlCacheEntryStreamA(LPCSTR lpszUrlName,
        LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
        LPDWORD lpdwCacheEntryInfoBufferSize, BOOL fRandomRead, DWORD dwReserved)
{
    /* NOTE: this is not the same as the way that the native
     * version allocates 'stream' handles. I did it this way
     * as it is much easier and no applications should depend
     * on this behaviour. (Native version appears to allocate
     * indices into a table)
     */
    stream_handle *stream;
    HANDLE file;

    TRACE("(%s, %p, %p, %x, 0x%08x)\n", debugstr_a(lpszUrlName), lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize, fRandomRead, dwReserved);

    if(!RetrieveUrlCacheEntryFileA(lpszUrlName, lpCacheEntryInfo,
                lpdwCacheEntryInfoBufferSize, dwReserved))
        return NULL;

    file = CreateFileA(lpCacheEntryInfo->lpszLocalFileName, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, fRandomRead ? FILE_FLAG_RANDOM_ACCESS : 0, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        UnlockUrlCacheEntryFileA(lpszUrlName, 0);
        return NULL;
    }

    /* allocate handle storage space */
    stream = heap_alloc(sizeof(stream_handle) + strlen(lpszUrlName) * sizeof(CHAR));
    if(!stream) {
        CloseHandle(file);
        UnlockUrlCacheEntryFileA(lpszUrlName, 0);
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    stream->file = file;
    strcpy(stream->url, lpszUrlName);
    return stream;
}

/***********************************************************************
 *           RetrieveUrlCacheEntryStreamW (WININET.@)
 *
 */
HANDLE WINAPI RetrieveUrlCacheEntryStreamW(LPCWSTR lpszUrlName,
        LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
        LPDWORD lpdwCacheEntryInfoBufferSize, BOOL fRandomRead, DWORD dwReserved)
{
    DWORD len;
    /* NOTE: this is not the same as the way that the native
     * version allocates 'stream' handles. I did it this way
     * as it is much easier and no applications should depend
     * on this behaviour. (Native version appears to allocate
     * indices into a table)
     */
    stream_handle *stream;
    HANDLE file;

    TRACE("(%s, %p, %p, %x, 0x%08x)\n", debugstr_w(lpszUrlName), lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize, fRandomRead, dwReserved);

    if(!(len = urlcache_encode_url(lpszUrlName, NULL, 0)))
        return NULL;

    if(!RetrieveUrlCacheEntryFileW(lpszUrlName, lpCacheEntryInfo,
                lpdwCacheEntryInfoBufferSize, dwReserved))
        return NULL;

    file = CreateFileW(lpCacheEntryInfo->lpszLocalFileName, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, fRandomRead ? FILE_FLAG_RANDOM_ACCESS : 0, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        UnlockUrlCacheEntryFileW(lpszUrlName, 0);
        return NULL;
    }

    /* allocate handle storage space */
    stream = heap_alloc(sizeof(stream_handle) + len*sizeof(WCHAR));
    if(!stream) {
        CloseHandle(file);
        UnlockUrlCacheEntryFileW(lpszUrlName, 0);
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    stream->file = file;
    if(!urlcache_encode_url(lpszUrlName, stream->url, len)) {
        CloseHandle(file);
        UnlockUrlCacheEntryFileW(lpszUrlName, 0);
        heap_free(stream);
        return NULL;
    }
    return stream;
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
    stream_handle *pStream = (stream_handle*)hUrlCacheStream;

    if (dwReserved != 0)
    {
        ERR("dwReserved != 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IsBadReadPtr(pStream, sizeof(*pStream)) || IsBadStringPtrA(pStream->url, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!UnlockUrlCacheEntryFileA(pStream->url, 0))
        return FALSE;

    CloseHandle(pStream->file);
    heap_free(pStream);
    return TRUE;
}


/***********************************************************************
 *           DeleteUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI DeleteUrlCacheEntryA(LPCSTR lpszUrlName)
{
    cache_container *pContainer;
    urlcache_header *pHeader;
    struct hash_entry *pHashEntry;
    DWORD error;
    BOOL ret;

    TRACE("(%s)\n", debugstr_a(lpszUrlName));

    error = cache_containers_find(lpszUrlName, &pContainer);
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

    if (!urlcache_find_hash_entry(pHeader, lpszUrlName, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        TRACE("entry %s not found!\n", lpszUrlName);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    ret = urlcache_entry_delete(pContainer, pHeader, pHashEntry);

    cache_container_unlock_index(pContainer, pHeader);

    return ret;
}

/***********************************************************************
 *           DeleteUrlCacheEntryW (WININET.@)
 *
 */
BOOL WINAPI DeleteUrlCacheEntryW(LPCWSTR lpszUrlName)
{
    char *url;
    BOOL ret;

    if(!urlcache_encode_url_alloc(lpszUrlName, &url))
        return FALSE;

    ret = DeleteUrlCacheEntryA(url);
    heap_free(url);
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

/***********************************************************************
 *           FindFirstUrlCacheEntryA (WININET.@)
 *
 */
INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryA(LPCSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
    find_handle *pEntryHandle;

    TRACE("(%s, %p, %p)\n", debugstr_a(lpszUrlSearchPattern), lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize);

    pEntryHandle = heap_alloc(sizeof(*pEntryHandle));
    if (!pEntryHandle)
        return NULL;

    pEntryHandle->magic = URLCACHE_FIND_ENTRY_HANDLE_MAGIC;
    if (lpszUrlSearchPattern)
    {
        pEntryHandle->url_search_pattern = heap_strdupA(lpszUrlSearchPattern);
        if (!pEntryHandle->url_search_pattern)
        {
            heap_free(pEntryHandle);
            return NULL;
        }
    }
    else
        pEntryHandle->url_search_pattern = NULL;
    pEntryHandle->container_idx = 0;
    pEntryHandle->hash_table_idx = 0;
    pEntryHandle->hash_entry_idx = 0;

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
    find_handle *pEntryHandle;

    TRACE("(%s, %p, %p)\n", debugstr_w(lpszUrlSearchPattern), lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize);

    pEntryHandle = heap_alloc(sizeof(*pEntryHandle));
    if (!pEntryHandle)
        return NULL;

    pEntryHandle->magic = URLCACHE_FIND_ENTRY_HANDLE_MAGIC;
    if (lpszUrlSearchPattern)
    {
        pEntryHandle->url_search_pattern = heap_strdupWtoA(lpszUrlSearchPattern);
        if (!pEntryHandle->url_search_pattern)
        {
            heap_free(pEntryHandle);
            return NULL;
        }
    }
    else
        pEntryHandle->url_search_pattern = NULL;
    pEntryHandle->container_idx = 0;
    pEntryHandle->hash_table_idx = 0;
    pEntryHandle->hash_entry_idx = 0;

    if (!FindNextUrlCacheEntryW(pEntryHandle, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize))
    {
        heap_free(pEntryHandle);
        return NULL;
    }
    return pEntryHandle;
}

static BOOL urlcache_find_next_entry(
  HANDLE hEnumHandle,
  LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo,
  LPDWORD lpdwNextCacheEntryInfoBufferSize,
  BOOL unicode)
{
    find_handle *pEntryHandle = (find_handle*)hEnumHandle;
    cache_container *pContainer;

    if (pEntryHandle->magic != URLCACHE_FIND_ENTRY_HANDLE_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    for (; cache_containers_enum(pEntryHandle->url_search_pattern, pEntryHandle->container_idx, &pContainer);
         pEntryHandle->container_idx++, pEntryHandle->hash_table_idx = 0)
    {
        urlcache_header *pHeader;
        entry_hash_table *pHashTableEntry;
        DWORD error;

        error = cache_container_open_index(pContainer, MIN_BLOCK_NO);
        if (error != ERROR_SUCCESS)
        {
            SetLastError(error);
            return FALSE;
        }

        if (!(pHeader = cache_container_lock_index(pContainer)))
            return FALSE;

        for (; urlcache_enum_hash_tables(pHeader, &pEntryHandle->hash_table_idx, &pHashTableEntry);
             pEntryHandle->hash_table_idx++, pEntryHandle->hash_entry_idx = 0)
        {
            const struct hash_entry *pHashEntry = NULL;
            for (; urlcache_enum_hash_table_entries(pHeader, pHashTableEntry, &pEntryHandle->hash_entry_idx, &pHashEntry);
                 pEntryHandle->hash_entry_idx++)
            {
                const entry_url *pUrlEntry;
                const entry_header *pEntry = (const entry_header*)((LPBYTE)pHeader + pHashEntry->offset);

                if (pEntry->signature != URL_SIGNATURE)
                    continue;

                pUrlEntry = (const entry_url *)pEntry;
                TRACE("Found URL: %s\n",
                      debugstr_a((LPCSTR)pUrlEntry + pUrlEntry->url_off));
                TRACE("Header info: %s\n",
                        debugstr_an((LPCSTR)pUrlEntry + pUrlEntry->header_info_off,
                            pUrlEntry->header_info_size));

                error = urlcache_copy_entry(
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
                pEntryHandle->hash_entry_idx++;
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

    return urlcache_find_next_entry(hEnumHandle, lpNextCacheEntryInfo,
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

    return urlcache_find_next_entry(hEnumHandle,
            (LPINTERNET_CACHE_ENTRY_INFOA)lpNextCacheEntryInfo,
            lpdwNextCacheEntryInfoBufferSize, TRUE /* UNICODE */);
}

/***********************************************************************
 *           FindCloseUrlCache (WININET.@)
 */
BOOL WINAPI FindCloseUrlCache(HANDLE hEnumHandle)
{
    find_handle *pEntryHandle = (find_handle*)hEnumHandle;

    TRACE("(%p)\n", hEnumHandle);

    if (!pEntryHandle || pEntryHandle->magic != URLCACHE_FIND_ENTRY_HANDLE_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pEntryHandle->magic = 0;
    heap_free(pEntryHandle->url_search_pattern);
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

static BOOL urlcache_entry_is_expired(const entry_url *pUrlEntry,
        FILETIME *pftLastModified)
{
    BOOL ret;
    FILETIME now, expired;

    *pftLastModified = pUrlEntry->modification_time;
    GetSystemTimeAsFileTime(&now);
    dos_date_time_to_file_time(pUrlEntry->expire_date,
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
BOOL WINAPI IsUrlCacheEntryExpiredA(LPCSTR url, DWORD dwFlags, FILETIME* pftLastModified)
{
    urlcache_header *pHeader;
    struct hash_entry *pHashEntry;
    const entry_header *pEntry;
    const entry_url * pUrlEntry;
    cache_container *pContainer;
    BOOL expired;

    TRACE("(%s, %08x, %p)\n", debugstr_a(url), dwFlags, pftLastModified);

    if (!url || !pftLastModified)
        return TRUE;
    if (dwFlags)
        FIXME("unknown flags 0x%08x\n", dwFlags);

    /* Any error implies that the URL is expired, i.e. not in the cache */
    if (cache_containers_find(url, &pContainer))
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

    if (!urlcache_find_hash_entry(pHeader, url, &pHashEntry))
    {
        cache_container_unlock_index(pContainer, pHeader);
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        TRACE("entry %s not found!\n", url);
        return TRUE;
    }

    pEntry = (const entry_header*)((LPBYTE)pHeader + pHashEntry->offset);
    if (pEntry->signature != URL_SIGNATURE)
    {
        cache_container_unlock_index(pContainer, pHeader);
        memset(pftLastModified, 0, sizeof(*pftLastModified));
        FIXME("Trying to retrieve entry of unknown format %s\n", debugstr_an((LPCSTR)&pEntry->signature, sizeof(DWORD)));
        return TRUE;
    }

    pUrlEntry = (const entry_url *)pEntry;
    expired = urlcache_entry_is_expired(pUrlEntry, pftLastModified);

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
BOOL WINAPI IsUrlCacheEntryExpiredW(LPCWSTR url, DWORD dwFlags, FILETIME* pftLastModified)
{
    char *encoded_url;
    BOOL ret;

    if(!urlcache_encode_url_alloc(url, &encoded_url))
        return FALSE;

    ret = IsUrlCacheEntryExpiredA(encoded_url, dwFlags, pftLastModified);
    heap_free(encoded_url);
    return ret;
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

#ifndef __REACTOS__
    cache_containers_init();
#endif
    return TRUE;
}

void free_urlcache(void)
{
    SetEvent(dll_unload_event);
    WaitForSingleObject(free_cache_running, INFINITE);
    ReleaseSemaphore(free_cache_running, 1, NULL);
    CloseHandle(free_cache_running);
    CloseHandle(dll_unload_event);

    cache_containers_free();
}

/***********************************************************************
 *           LoadUrlCacheContent (WININET.@)
 */
BOOL WINAPI LoadUrlCacheContent(void)
{
    FIXME("stub!\n");
    return FALSE;
}
