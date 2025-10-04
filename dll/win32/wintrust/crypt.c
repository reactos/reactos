/*
 * WinTrust Cryptography functions
 *
 * Copyright 2006 James Hawkins
 * Copyright 2000-2002 Stuart Caie
 * Copyright 2002 Patrik Stridvall
 * Copyright 2003 Greg Turner
 * Copyright 2008 Juan Lang
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
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wintrust.h"
#include "winver.h"
#include "mscat.h"
#include "mssip.h"
#include "imagehlp.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

#define CATADMIN_MAGIC 0x43415441 /* 'CATA' */
#define CRYPTCAT_MAGIC 0x43415443 /* 'CATC' */
#define CATINFO_MAGIC  0x43415449 /* 'CATI' */

struct cryptcat
{
    DWORD     magic;
    HCRYPTMSG msg;
    DWORD     encoding;
    CTL_INFO *inner;
    DWORD     inner_len;
    GUID      subject;
    DWORD     attr_count;
    CRYPTCATATTRIBUTE *attr;
};

struct catadmin
{
    DWORD magic;
    WCHAR path[MAX_PATH];
    HANDLE find;
    ALG_ID alg;
    const WCHAR *providerName;
    DWORD providerType;
};

struct catinfo
{
    DWORD magic;
    WCHAR file[MAX_PATH];
};

static HCATINFO create_catinfo(const WCHAR *filename)
{
    struct catinfo *ci;

    if (!(ci = malloc(sizeof(*ci))))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return INVALID_HANDLE_VALUE;
    }
    lstrcpyW(ci->file, filename);
    ci->magic = CATINFO_MAGIC;
    return ci;
}

/***********************************************************************
 *      CryptCATAdminAcquireContext (WINTRUST.@)
 *
 * Get a catalog administrator context handle.
 *
 * PARAMS
 *   catAdmin  [O] Pointer to the context handle.
 *   sys       [I] Pointer to a GUID for the needed subsystem.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE. catAdmin contains the context handle.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminAcquireContext(HCATADMIN *catAdmin,
                                        const GUID *sys, DWORD dwFlags)
{
    TRACE("%p %s %lx\n", catAdmin, debugstr_guid(sys), dwFlags);
    return CryptCATAdminAcquireContext2(catAdmin, sys, NULL, NULL, dwFlags);
}

/***********************************************************************
 *             CryptCATAdminAcquireContext2 (WINTRUST.@)
 * Get a catalog administrator context handle.
 *
 * PARAMS
 *   catAdmin  [O] Pointer to the context handle.
 *   sys       [I] Pointer to a GUID for the needed subsystem.
 *   algorithm [I] String of hashing algorithm to use for catalog (SHA1/SHA256).
 *   policy    [I] Pointer to policy structure for checking strong signatures.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE. catAdmin contains the context handle.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminAcquireContext2(HCATADMIN *catAdmin, const GUID *sys, const WCHAR *algorithm,
                                         const CERT_STRONG_SIGN_PARA *policy, DWORD dwFlags)
{
    static const WCHAR catroot[] =
        {'\\','c','a','t','r','o','o','t',0};
    static const WCHAR fmt[] =
        {'%','s','\\','{','%','0','8','x','-','%','0','4','x','-','%','0',
         '4','x','-','%','0','2','x','%','0','2','x','-','%','0','2','x',
         '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x',
         '%','0','2','x','}',0};
    static const GUID defsys =
        {0x127d0a1d,0x4ef2,0x11d1,{0x86,0x08,0x00,0xc0,0x4f,0xc2,0x95,0xee}};

    WCHAR catroot_dir[MAX_PATH];
    struct catadmin *ca;
    ALG_ID alg;
    const WCHAR *providerName;
    DWORD providerType;

    TRACE("%p %s %s %p %lx\n", catAdmin, debugstr_guid(sys), debugstr_w(algorithm), policy, dwFlags);

    if (!catAdmin || dwFlags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (policy != NULL)
        FIXME("strong policy parameter is unimplemented\n");

    if (algorithm == NULL || wcscmp(algorithm, BCRYPT_SHA1_ALGORITHM) == 0)
    {
        alg = CALG_SHA1;
        providerName = MS_DEF_PROV_W;
        providerType = PROV_RSA_FULL;
    }
    else if (wcscmp(algorithm, BCRYPT_SHA256_ALGORITHM) == 0)
    {
        alg = CALG_SHA_256;
        providerName = MS_ENH_RSA_AES_PROV_W;
        providerType = PROV_RSA_AES;
    }
    else
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }

    if (!(ca = malloc(sizeof(*ca))))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    ca->alg = alg;
    ca->providerName = providerName;
    ca->providerType = providerType;

    GetSystemDirectoryW(catroot_dir, MAX_PATH);
    lstrcatW(catroot_dir, catroot);

    /* create the directory if it doesn't exist */
    CreateDirectoryW(catroot_dir, NULL);

    if (!sys) sys = &defsys;
    swprintf(ca->path, ARRAY_SIZE(ca->path), fmt, catroot_dir, sys->Data1, sys->Data2,
             sys->Data3, sys->Data4[0], sys->Data4[1], sys->Data4[2],
             sys->Data4[3], sys->Data4[4], sys->Data4[5], sys->Data4[6],
             sys->Data4[7]);

    /* create the directory if it doesn't exist */
    CreateDirectoryW(ca->path, NULL);

    ca->magic = CATADMIN_MAGIC;
    ca->find = INVALID_HANDLE_VALUE;

    *catAdmin = ca;
    return TRUE;
}

/***********************************************************************
 *             CryptCATAdminAddCatalog (WINTRUST.@)
 */
HCATINFO WINAPI CryptCATAdminAddCatalog(HCATADMIN catAdmin, PWSTR catalogFile,
                                        PWSTR selectBaseName, DWORD flags)
{
    static const WCHAR slashW[] = {'\\',0};
    struct catadmin *ca = catAdmin;
    struct catinfo *ci;
    WCHAR *target;
    DWORD len;

    TRACE("%p %s %s %ld\n", catAdmin, debugstr_w(catalogFile),
          debugstr_w(selectBaseName), flags);

    if (!selectBaseName)
    {
        FIXME("NULL basename not handled\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (!ca || ca->magic != CATADMIN_MAGIC || !catalogFile || flags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    len = lstrlenW(ca->path) + lstrlenW(selectBaseName) + 2;
    if (!(target = malloc(len * sizeof(WCHAR))))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }
    lstrcpyW(target, ca->path);
    lstrcatW(target, slashW);
    lstrcatW(target, selectBaseName);

    if (!CopyFileW(catalogFile, target, FALSE))
    {
        free(target);
        return NULL;
    }
    SetFileAttributesW(target, FILE_ATTRIBUTE_SYSTEM);

    if (!(ci = malloc(sizeof(*ci))))
    {
        free(target);
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }
    ci->magic = CATINFO_MAGIC;
    lstrcpyW(ci->file, target);

    free(target);
    return ci;
}

static BOOL pe_image_hash( HANDLE file, HCRYPTHASH hash )
{
    UINT32 size, offset, file_size, sig_pos;
    HANDLE mapping;
    BYTE *view;
    IMAGE_NT_HEADERS *nt;
    BOOL ret = FALSE;

    if ((file_size = GetFileSize( file, NULL )) == INVALID_FILE_SIZE) return FALSE;

    if ((mapping = CreateFileMappingW( file, NULL, PAGE_READONLY, 0, 0, NULL )) == INVALID_HANDLE_VALUE)
        return FALSE;

    if (!(view = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 )) || !(nt = ImageNtHeader( view ))) goto done;

    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_NT_HEADERS64 *nt64 = (const IMAGE_NT_HEADERS64 *)nt;

        /* offset from start of file to checksum */
        offset = (BYTE *)&nt64->OptionalHeader.CheckSum - view;

        /* area between checksum and security directory entry */
        size = FIELD_OFFSET( IMAGE_OPTIONAL_HEADER64, DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] ) -
               FIELD_OFFSET( IMAGE_OPTIONAL_HEADER64, Subsystem );

        if (nt64->OptionalHeader.NumberOfRvaAndSizes < IMAGE_FILE_SECURITY_DIRECTORY + 1) goto done;
        sig_pos = nt64->OptionalHeader.DataDirectory[IMAGE_FILE_SECURITY_DIRECTORY].VirtualAddress;
    }
    else if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        const IMAGE_NT_HEADERS32 *nt32 = (const IMAGE_NT_HEADERS32 *)nt;

        /* offset from start of file to checksum */
        offset = (BYTE *)&nt32->OptionalHeader.CheckSum - view;

        /* area between checksum and security directory entry */
        size = FIELD_OFFSET( IMAGE_OPTIONAL_HEADER32, DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] ) -
               FIELD_OFFSET( IMAGE_OPTIONAL_HEADER32, Subsystem );

        if (nt32->OptionalHeader.NumberOfRvaAndSizes < IMAGE_FILE_SECURITY_DIRECTORY + 1) goto done;
        sig_pos = nt32->OptionalHeader.DataDirectory[IMAGE_FILE_SECURITY_DIRECTORY].VirtualAddress;
    }
    else goto done;

    if (!CryptHashData( hash, view, offset, 0 )) goto done;
    offset += sizeof(DWORD); /* skip checksum */
    if (!CryptHashData( hash, view + offset, size, 0 )) goto done;

    offset += size + sizeof(IMAGE_DATA_DIRECTORY); /* skip security entry */
    if (offset > file_size) goto done;
    if (sig_pos)
    {
        if (sig_pos < offset) goto done;
        if (sig_pos > file_size) goto done;
        size = sig_pos - offset; /* exclude signature */
    }
    else size = file_size - offset;

    if (!CryptHashData( hash, view + offset, size, 0 )) goto done;
    ret = TRUE;

    if (!sig_pos && (size = file_size % 8))
    {
        static const BYTE pad[7];
        ret = CryptHashData( hash, pad, 8 - size, 0 );
    }

done:
    UnmapViewOfFile( view );
    CloseHandle( mapping );
    return ret;
}

static BOOL catadmin_calc_hash_from_filehandle(HCATADMIN catAdmin, HANDLE hFile, DWORD *pcbHash,
                                               BYTE *pbHash, DWORD dwFlags)
{
    BOOL ret = FALSE;
    struct catadmin *ca = catAdmin;
    ALG_ID alg = CALG_SHA1;
    const WCHAR *providerName = MS_DEF_PROV_W;
    DWORD providerType = PROV_RSA_FULL;
    DWORD hashLength;

    if (!hFile || !pcbHash || dwFlags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (ca)
    {
        alg = ca->alg;
        providerName = ca->providerName;
        providerType = ca->providerType;
    }

    switch (alg)
    {
        case CALG_SHA1:
            hashLength = 20;
            break;
        case CALG_SHA_256:
            hashLength = 32;
            break;
        default:
            FIXME("unsupported algorithm %x\n", alg);
            return FALSE;
    }

    if (*pcbHash < hashLength)
    {
        *pcbHash = hashLength;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return TRUE;
    }

    *pcbHash = hashLength;
    if (pbHash)
    {
        HCRYPTPROV prov;
        HCRYPTHASH hash;
        DWORD bytes_read;
        BYTE *buffer;

        if (!(buffer = malloc(4096)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        ret = CryptAcquireContextW(&prov, NULL, providerName, providerType, CRYPT_VERIFYCONTEXT);
        if (!ret)
        {
            free(buffer);
            return FALSE;
        }
        ret = CryptCreateHash(prov, alg, 0, 0, &hash);
        if (!ret)
        {
            free(buffer);
            CryptReleaseContext(prov, 0);
            return FALSE;
        }

        if (!(ret = pe_image_hash(hFile, hash)))
        {
            while ((ret = ReadFile(hFile, buffer, 4096, &bytes_read, NULL)) && bytes_read)
            {
                CryptHashData(hash, buffer, bytes_read, 0);
            }
        }
        if (ret) ret = CryptGetHashParam(hash, HP_HASHVAL, pbHash, pcbHash, 0);

        free(buffer);
        CryptDestroyHash(hash);
        CryptReleaseContext(prov, 0);
    }
    return ret;
}

/***********************************************************************
 *             CryptCATAdminCalcHashFromFileHandle (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE hFile, DWORD *pcbHash, BYTE *pbHash, DWORD dwFlags)
{
    TRACE("%p %p %p %lx\n", hFile, pcbHash, pbHash, dwFlags);
    return catadmin_calc_hash_from_filehandle(NULL, hFile, pcbHash, pbHash, dwFlags);
}

/***********************************************************************
 *    CryptCATAdminCalcHashFromFileHandle2 (WINTRUST.@)
 *
 * Calculate hash for a specific file using a catalog administrator context.
 *
 * PARAMS
 *   catAdmin  [I] Catalog administrator context handle.
 *   hFile     [I] Handle for the file to hash.
 *   pcbHash   [I] Pointer to the length of the hash.
 *   pbHash    [O] Pointer to the buffer that will store that hash
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE. If pcbHash is too small, LastError will be set to ERROR_INSUFFICIENT_BUFFER.
 *                  pbHash contains the computed hash, if supplied.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminCalcHashFromFileHandle2(HCATADMIN catAdmin, HANDLE hFile,  DWORD *pcbHash,
                                                 BYTE *pbHash, DWORD dwFlags)
{
    TRACE("%p %p %p %p %lx\n", catAdmin, hFile, pcbHash, pbHash, dwFlags);

    if (!catAdmin || ((struct catadmin *)catAdmin)->magic != CATADMIN_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return catadmin_calc_hash_from_filehandle(catAdmin, hFile, pcbHash, pbHash, dwFlags);
}


/***********************************************************************
 *             CryptCATAdminEnumCatalogFromHash (WINTRUST.@)
 */
HCATINFO WINAPI CryptCATAdminEnumCatalogFromHash(HCATADMIN hCatAdmin, BYTE* pbHash,
                                                 DWORD cbHash, DWORD dwFlags,
                                                 HCATINFO* phPrevCatInfo )
{
    static const WCHAR slashW[] = {'\\',0};
    static const WCHAR globW[]  = {'\\','*','.','c','a','t',0};

    struct catadmin *ca = hCatAdmin;
    WIN32_FIND_DATAW data;
    HCATINFO prev = NULL;
    HCRYPTPROV prov;
    DWORD size;
    BOOL ret;

    TRACE("%p %p %ld %lx %p\n", hCatAdmin, pbHash, cbHash, dwFlags, phPrevCatInfo);

    if (!ca || ca->magic != CATADMIN_MAGIC || !pbHash || cbHash != 20 || dwFlags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (phPrevCatInfo) prev = *phPrevCatInfo;

    ret = CryptAcquireContextW(&prov, NULL, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (!ret) return NULL;

    if (!prev)
    {
        WCHAR *path;

        size = lstrlenW(ca->path) * sizeof(WCHAR) + sizeof(globW);
        if (!(path = malloc(size)))
        {
            CryptReleaseContext(prov, 0);
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
        lstrcpyW(path, ca->path);
        lstrcatW(path, globW);

        FindClose(ca->find);
        ca->find = FindFirstFileW(path, &data);

        free(path);
        if (ca->find == INVALID_HANDLE_VALUE)
        {
            CryptReleaseContext(prov, 0);
            return NULL;
        }
    }
    else if (!FindNextFileW(ca->find, &data))
    {
        CryptCATAdminReleaseCatalogContext(hCatAdmin, prev, 0);
        CryptReleaseContext(prov, 0);
        return NULL;
    }

    while (1)
    {
        WCHAR *filename;
        CRYPTCATMEMBER *member = NULL;
        struct catinfo *ci;
        HANDLE hcat;

        size = (lstrlenW(ca->path) + lstrlenW(data.cFileName) + 2) * sizeof(WCHAR);
        if (!(filename = malloc(size)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
        lstrcpyW(filename, ca->path);
        lstrcatW(filename, slashW);
        lstrcatW(filename, data.cFileName);

        hcat = CryptCATOpen(filename, CRYPTCAT_OPEN_EXISTING, prov, 0, 0);
        if (hcat == INVALID_HANDLE_VALUE)
        {
            WARN("couldn't open %s (%lu)\n", debugstr_w(filename), GetLastError());
            continue;
        }
        while ((member = CryptCATEnumerateMember(hcat, member)))
        {
            if (member->pIndirectData->Digest.cbData != cbHash)
            {
                WARN("amount of hash bytes differs: %lu/%lu\n", member->pIndirectData->Digest.cbData, cbHash);
                continue;
            }
            if (!memcmp(member->pIndirectData->Digest.pbData, pbHash, cbHash))
            {
                TRACE("file %s matches\n", debugstr_w(data.cFileName));

                CryptCATClose(hcat);
                CryptReleaseContext(prov, 0);
                if (!phPrevCatInfo)
                {
                    FindClose(ca->find);
                    ca->find = INVALID_HANDLE_VALUE;
                }
                ci = create_catinfo(filename);
                free(filename);
                return ci;
            }
        }
        CryptCATClose(hcat);
        free(filename);

        if (!FindNextFileW(ca->find, &data))
        {
            FindClose(ca->find);
            ca->find = INVALID_HANDLE_VALUE;
            CryptReleaseContext(prov, 0);
            return NULL;
        }
    }
    return NULL;
}

/***********************************************************************
 *      CryptCATAdminReleaseCatalogContext (WINTRUST.@)
 *
 * Release a catalog context handle.
 *
 * PARAMS
 *   hCatAdmin [I] Context handle.
 *   hCatInfo  [I] Catalog handle.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminReleaseCatalogContext(HCATADMIN hCatAdmin,
                                               HCATINFO hCatInfo,
                                               DWORD dwFlags)
{
    struct catinfo *ci = hCatInfo;
    struct catadmin *ca = hCatAdmin;

    TRACE("%p %p %lx\n", hCatAdmin, hCatInfo, dwFlags);

    if (!ca || ca->magic != CATADMIN_MAGIC || !ci || ci->magic != CATINFO_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory(&ci->magic, sizeof(ci->magic));
    free(ci);
    return TRUE;
}

/***********************************************************************
 *      CryptCATAdminReleaseContext (WINTRUST.@)
 *
 * Release a catalog administrator context handle.
 *
 * PARAMS
 *   catAdmin  [I] Context handle.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminReleaseContext(HCATADMIN hCatAdmin, DWORD dwFlags )
{
    struct catadmin *ca = hCatAdmin;

    TRACE("%p %lx\n", hCatAdmin, dwFlags);

    if (!ca || ca->magic != CATADMIN_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (ca->find != INVALID_HANDLE_VALUE) FindClose(ca->find);
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory(&ca->magic, sizeof(ca->magic));
    free(ca);
    return TRUE;
}

/***********************************************************************
 *      CryptCATAdminRemoveCatalog (WINTRUST.@)
 *
 * Remove a catalog file.
 *
 * PARAMS
 *   catAdmin         [I] Context handle.
 *   pwszCatalogFile  [I] Catalog file.
 *   dwFlags          [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminRemoveCatalog(HCATADMIN hCatAdmin, LPCWSTR pwszCatalogFile, DWORD dwFlags)
{
    struct catadmin *ca = hCatAdmin;

    TRACE("%p %s %lx\n", hCatAdmin, debugstr_w(pwszCatalogFile), dwFlags);

    if (!ca || ca->magic != CATADMIN_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Only delete when there is a filename and no path */
    if (pwszCatalogFile && pwszCatalogFile[0] != 0 &&
        !wcschr(pwszCatalogFile, '\\') && !wcschr(pwszCatalogFile, '/') &&
        !wcschr(pwszCatalogFile, ':'))
    {
        static const WCHAR slashW[] = {'\\',0};
        WCHAR *target;
        DWORD len;

        len = lstrlenW(ca->path) + lstrlenW(pwszCatalogFile) + 2;
        if (!(target = malloc(len * sizeof(WCHAR))))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        lstrcpyW(target, ca->path);
        lstrcatW(target, slashW);
        lstrcatW(target, pwszCatalogFile);

        DeleteFileW(target);

        free(target);
    }

    return TRUE;
}

/***********************************************************************
 *      CryptCATAdminResolveCatalogPath  (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminResolveCatalogPath(HCATADMIN hcatadmin, WCHAR *catalog_file,
                                            CATALOG_INFO *info, DWORD flags)
{
    static const WCHAR slashW[] = {'\\',0};
    struct catadmin *ca = hcatadmin;

    TRACE("%p %s %p %lx\n", hcatadmin, debugstr_w(catalog_file), info, flags);

    if (!ca || ca->magic != CATADMIN_MAGIC || !info || info->cbStruct != sizeof(*info) || flags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    lstrcpyW(info->wszCatalogFile, ca->path);
    lstrcatW(info->wszCatalogFile, slashW);
    lstrcatW(info->wszCatalogFile, catalog_file);

    return TRUE;
}

/***********************************************************************
 *      CryptCATClose  (WINTRUST.@)
 */
BOOL WINAPI CryptCATClose(HANDLE hCatalog)
{
    struct cryptcat *cc = hCatalog;

    TRACE("(%p)\n", hCatalog);

    if (!hCatalog || hCatalog == INVALID_HANDLE_VALUE || cc->magic != CRYPTCAT_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    free(cc->attr);
    free(cc->inner);
    CryptMsgClose(cc->msg);

    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory(&cc->magic, sizeof(cc->magic));
    free(cc);
    return TRUE;
}

/***********************************************************************
 *      CryptCATGetAttrInfo  (WINTRUST.@)
 */
CRYPTCATATTRIBUTE * WINAPI CryptCATGetAttrInfo(HANDLE hCatalog, CRYPTCATMEMBER *member, LPWSTR tag)
{
    struct cryptcat *cc = hCatalog;

    FIXME("%p, %p, %s\n", hCatalog, member, debugstr_w(tag));

    if (!hCatalog || hCatalog == INVALID_HANDLE_VALUE || cc->magic != CRYPTCAT_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

/***********************************************************************
 *      CryptCATGetCatAttrInfo  (WINTRUST.@)
 */
CRYPTCATATTRIBUTE * WINAPI CryptCATGetCatAttrInfo(HANDLE hCatalog, LPWSTR tag)
{
    struct cryptcat *cc = hCatalog;

    FIXME("%p, %s\n", hCatalog, debugstr_w(tag));

    if (!hCatalog || hCatalog == INVALID_HANDLE_VALUE || cc->magic != CRYPTCAT_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

CRYPTCATMEMBER * WINAPI CryptCATGetMemberInfo(HANDLE hCatalog, LPWSTR tag)
{
    struct cryptcat *cc = hCatalog;

    FIXME("%p, %s\n", hCatalog, debugstr_w(tag));

    if (!hCatalog || hCatalog == INVALID_HANDLE_VALUE || cc->magic != CRYPTCAT_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

/***********************************************************************
 *      CryptCATEnumerateAttr  (WINTRUST.@)
 */
CRYPTCATATTRIBUTE * WINAPI CryptCATEnumerateAttr(HANDLE hCatalog, CRYPTCATMEMBER *member, CRYPTCATATTRIBUTE *prev)
{
    struct cryptcat *cc = hCatalog;

    FIXME("%p, %p, %p\n", hCatalog, member, prev);

    if (!hCatalog || hCatalog == INVALID_HANDLE_VALUE || cc->magic != CRYPTCAT_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

/***********************************************************************
 *      CryptCATEnumerateCatAttr  (WINTRUST.@)
 */
CRYPTCATATTRIBUTE * WINAPI CryptCATEnumerateCatAttr(HANDLE hCatalog, CRYPTCATATTRIBUTE *prev)
{
    struct cryptcat *cc = hCatalog;

    FIXME("%p, %p\n", hCatalog, prev);

    if (!hCatalog || hCatalog == INVALID_HANDLE_VALUE || cc->magic != CRYPTCAT_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

/***********************************************************************
 *      CryptCATEnumerateMember  (WINTRUST.@)
 */
CRYPTCATMEMBER * WINAPI CryptCATEnumerateMember(HANDLE hCatalog, CRYPTCATMEMBER *prev)
{
    struct cryptcat *cc = hCatalog;
    CRYPTCATMEMBER *member = prev;
    CTL_ENTRY *entry;
    DWORD size, i;

    TRACE("%p, %p\n", hCatalog, prev);

    if (!hCatalog || hCatalog == INVALID_HANDLE_VALUE || cc->magic != CRYPTCAT_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* dumping the contents makes me think that dwReserved is the iteration number */
    if (!member)
    {
        if (!(member = malloc(sizeof(*member))))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
        member->cbStruct = sizeof(*member);
        member->pwszFileName = member->pwszReferenceTag = NULL;
        member->dwReserved = 0;
        member->hReserved = NULL;
        member->gSubjectType = cc->subject;
        member->fdwMemberFlags = 0;
        member->pIndirectData = NULL;
        member->dwCertVersion = cc->inner->dwVersion;
    }
    else member->dwReserved++;

    if (member->dwReserved >= cc->inner->cCTLEntry)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto error;
    }

    /* list them backwards, like native */
    entry = &cc->inner->rgCTLEntry[cc->inner->cCTLEntry - member->dwReserved - 1];

    member->sEncodedIndirectData.cbData = member->sEncodedMemberInfo.cbData = 0;
    member->sEncodedIndirectData.pbData = member->sEncodedMemberInfo.pbData = NULL;
    free(member->pIndirectData);
    member->pIndirectData = NULL;

    for (i = 0; i < entry->cAttribute; i++)
    {
        CRYPT_ATTRIBUTE *attr = entry->rgAttribute + i;

        if (attr->cValue != 1)
        {
            ERR("Can't handle attr->cValue of %lu\n", attr->cValue);
            continue;
        }
        if (!strcmp(attr->pszObjId, CAT_MEMBERINFO_OBJID))
        {
            CAT_MEMBERINFO *mi;
            BOOL ret;

            member->sEncodedMemberInfo.cbData = attr->rgValue->cbData;
            member->sEncodedMemberInfo.pbData = attr->rgValue->pbData;

            CryptDecodeObject(cc->encoding, CAT_MEMBERINFO_OBJID, attr->rgValue->pbData, attr->rgValue->cbData, 0, NULL, &size);

            if (!(mi = malloc(size)))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                goto error;
            }
            ret = CryptDecodeObject(cc->encoding, CAT_MEMBERINFO_OBJID, attr->rgValue->pbData, attr->rgValue->cbData, 0, mi, &size);
            if (ret)
            {
                UNICODE_STRING guid;

                member->dwCertVersion = mi->dwCertVersion;
                RtlInitUnicodeString(&guid, mi->pwszSubjGuid);
                if (RtlGUIDFromString(&guid, &member->gSubjectType))
                {
                    free(mi);
                    goto error;
                }
            }
            free(mi);
            if (!ret) goto error;
        }
        else if (!strcmp(attr->pszObjId, SPC_INDIRECT_DATA_OBJID))
        {
            /* SPC_INDIRECT_DATA_CONTENT is equal to SIP_INDIRECT_DATA */

            member->sEncodedIndirectData.cbData = attr->rgValue->cbData;
            member->sEncodedIndirectData.pbData = attr->rgValue->pbData;

            CryptDecodeObject(cc->encoding, SPC_INDIRECT_DATA_OBJID, attr->rgValue->pbData, attr->rgValue->cbData, 0, NULL, &size);

            if (!(member->pIndirectData = malloc(size)))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                goto error;
            }
            CryptDecodeObject(cc->encoding, SPC_INDIRECT_DATA_OBJID, attr->rgValue->pbData, attr->rgValue->cbData, 0, member->pIndirectData, &size);
        }
        else
            /* this object id should probably be handled in CryptCATEnumerateAttr */
            FIXME("unhandled object id \"%s\"\n", attr->pszObjId);
    }

    if (!member->sEncodedMemberInfo.cbData || !member->sEncodedIndirectData.cbData)
    {
        ERR("Corrupted catalog entry?\n");
        SetLastError(CRYPT_E_ATTRIBUTES_MISSING);
        goto error;
    }
    size = (2 * member->pIndirectData->Digest.cbData + 1) * sizeof(WCHAR);
    member->pwszReferenceTag = realloc(member->pwszReferenceTag, size);

    if (!member->pwszReferenceTag)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto error;
    }
    /* FIXME: reference tag is usually the file hash but doesn't have to be */
    for (i = 0; i < member->pIndirectData->Digest.cbData; i++)
    {
        DWORD sub;

        sub = member->pIndirectData->Digest.pbData[i] >> 4;
        member->pwszReferenceTag[i * 2] = (sub < 10 ? '0' + sub : 'A' + sub - 10);
        sub = member->pIndirectData->Digest.pbData[i] & 0xf;
        member->pwszReferenceTag[i * 2 + 1] = (sub < 10 ? '0' + sub : 'A' + sub - 10);
    }
    member->pwszReferenceTag[i * 2] = 0;
    return member;

error:
    free(member->pIndirectData);
    free(member->pwszReferenceTag);
    free(member);
    return NULL;
}

static CTL_INFO *decode_inner_content(HANDLE hmsg, DWORD encoding, DWORD *len)
{
    DWORD size;
    LPSTR oid = NULL;
    BYTE *buffer = NULL;
    CTL_INFO *inner = NULL;

    if (!CryptMsgGetParam(hmsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, NULL, &size)) return NULL;
    if (!(oid = malloc(size)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }
    if (!CryptMsgGetParam(hmsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, oid, &size)) goto out;
    if (!CryptMsgGetParam(hmsg, CMSG_CONTENT_PARAM, 0, NULL, &size)) goto out;
    if (!(buffer = malloc(size)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto out;
    }
    if (!CryptMsgGetParam(hmsg, CMSG_CONTENT_PARAM, 0, buffer, &size)) goto out;
    if (!CryptDecodeObject(encoding, oid, buffer, size, 0, NULL, &size)) goto out;
    if (!(inner = malloc(size)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto out;
    }
    if (!CryptDecodeObject(encoding, oid, buffer, size, 0, inner, &size)) goto out;
    *len = size;

out:
    free(oid);
    free(buffer);
    return inner;
}

/***********************************************************************
 *      CryptCATCatalogInfoFromContext  (WINTRUST.@)
 */
BOOL WINAPI CryptCATCatalogInfoFromContext(HCATINFO hcatinfo, CATALOG_INFO *info, DWORD flags)
{
    struct catinfo *ci = hcatinfo;

    TRACE("%p, %p, %lx\n", hcatinfo, info, flags);

    if (!hcatinfo || hcatinfo == INVALID_HANDLE_VALUE || ci->magic != CATINFO_MAGIC ||
        flags || !info || info->cbStruct != sizeof(*info))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    lstrcpyW(info->wszCatalogFile, ci->file);
    return TRUE;
}

/***********************************************************************
 *      CryptCATPutAttrInfo  (WINTRUST.@)
 */
CRYPTCATATTRIBUTE * WINAPI CryptCATPutAttrInfo(HANDLE catalog, CRYPTCATMEMBER *member,
        WCHAR *name, DWORD flags, DWORD size, BYTE *data)
{
    FIXME("catalog %p, member %p, name %s, flags %#lx, size %lu, data %p, stub!\n",
            catalog, member, debugstr_w(name), flags, size, data);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/***********************************************************************
 *      CryptCATPutCatAttrInfo  (WINTRUST.@)
 */
CRYPTCATATTRIBUTE * WINAPI CryptCATPutCatAttrInfo(HANDLE catalog,
        WCHAR *name, DWORD flags, DWORD size, BYTE *data)
{
    FIXME("catalog %p, name %s, flags %#lx, size %lu, data %p, stub!\n",
            catalog, debugstr_w(name), flags, size, data);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/***********************************************************************
 *      CryptCATPutMemberInfo  (WINTRUST.@)
 */
CRYPTCATMEMBER * WINAPI CryptCATPutMemberInfo(HANDLE catalog, WCHAR *filename,
        WCHAR *member, GUID *subject, DWORD version, DWORD size, BYTE *data)
{
    FIXME("catalog %p, filename %s, member %s, subject %s, version %lu, size %lu, data %p, stub!\n",
            catalog, debugstr_w(filename), debugstr_w(member), debugstr_guid(subject), version, size, data);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/***********************************************************************
 *      CryptCATPersistStore  (WINTRUST.@)
 */
BOOL WINAPI CryptCATPersistStore(HANDLE catalog)
{
    FIXME("catalog %p, stub!\n", catalog);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *      CryptCATOpen  (WINTRUST.@)
 */
HANDLE WINAPI CryptCATOpen(WCHAR *filename, DWORD flags, HCRYPTPROV hProv,
                           DWORD dwPublicVersion, DWORD dwEncodingType)
{
    HANDLE file, hmsg;
    BYTE *buffer = NULL;
    DWORD size, open_mode = OPEN_ALWAYS;
    struct cryptcat *cc;
    BOOL valid;

    TRACE("filename %s, flags %#lx, provider %#Ix, version %#lx, type %#lx\n",
          debugstr_w(filename), flags, hProv, dwPublicVersion, dwEncodingType);

    if (!filename)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (!dwEncodingType)  dwEncodingType  = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;

    if (flags == CRYPTCAT_OPEN_EXISTING)
        open_mode = OPEN_EXISTING;
    if (flags & CRYPTCAT_OPEN_CREATENEW)
        open_mode = CREATE_ALWAYS;

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, open_mode, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    size = GetFileSize(file, NULL);
    if (!(buffer = malloc(size)))
    {
        CloseHandle(file);
        SetLastError(ERROR_OUTOFMEMORY);
        return INVALID_HANDLE_VALUE;
    }
    if (!(hmsg = CryptMsgOpenToDecode(dwEncodingType, 0, 0, hProv, NULL, NULL)))
    {
        CloseHandle(file);
        free(buffer);
        return INVALID_HANDLE_VALUE;
    }
    if (!size) valid = FALSE;
    else if (!ReadFile(file, buffer, size, &size, NULL))
    {
        CloseHandle(file);
        free(buffer);
        CryptMsgClose(hmsg);
        return INVALID_HANDLE_VALUE;
    }
    else valid = CryptMsgUpdate(hmsg, buffer, size, TRUE);
    free(buffer);
    CloseHandle(file);

    size = sizeof(DWORD);
    if (!(cc = calloc(1, sizeof(*cc))))
    {
        CryptMsgClose(hmsg);
        SetLastError(ERROR_OUTOFMEMORY);
        return INVALID_HANDLE_VALUE;
    }

    cc->msg = hmsg;
    cc->encoding = dwEncodingType;
    if (!valid)
    {
        cc->magic = CRYPTCAT_MAGIC;
        SetLastError(ERROR_SUCCESS);
        return cc;
    }
    else if (CryptMsgGetParam(hmsg, CMSG_ATTR_CERT_COUNT_PARAM, 0, &cc->attr_count, &size))
    {
        DWORD i, sum = 0;
        BYTE *p;

        for (i = 0; i < cc->attr_count; i++)
        {
            if (!CryptMsgGetParam(hmsg, CMSG_ATTR_CERT_PARAM, i, NULL, &size))
            {
                CryptMsgClose(hmsg);
                free(cc);
                return INVALID_HANDLE_VALUE;
            }
            sum += size;
        }
        if (!(cc->attr = malloc(sizeof(*cc->attr) * cc->attr_count + sum)))
        {
            CryptMsgClose(hmsg);
            free(cc);
            SetLastError(ERROR_OUTOFMEMORY);
            return INVALID_HANDLE_VALUE;
        }
        p = (BYTE *)(cc->attr + cc->attr_count);
        for (i = 0; i < cc->attr_count; i++)
        {
            if (!CryptMsgGetParam(hmsg, CMSG_ATTR_CERT_PARAM, i, NULL, &size))
            {
                CryptMsgClose(hmsg);
                free(cc->attr);
                free(cc);
                return INVALID_HANDLE_VALUE;
            }
            if (!CryptMsgGetParam(hmsg, CMSG_ATTR_CERT_PARAM, i, p, &size))
            {
                CryptMsgClose(hmsg);
                free(cc->attr);
                free(cc);
                return INVALID_HANDLE_VALUE;
            }
            p += size;
        }
        cc->inner = decode_inner_content(hmsg, dwEncodingType, &cc->inner_len);
        if (!cc->inner || !CryptSIPRetrieveSubjectGuid(filename, NULL, &cc->subject))
        {
            CryptMsgClose(hmsg);
            free(cc->attr);
            free(cc->inner);
            free(cc);
            return INVALID_HANDLE_VALUE;
        }
        cc->magic = CRYPTCAT_MAGIC;
        SetLastError(ERROR_SUCCESS);
        return cc;
    }
    free(cc);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *      CryptSIPCreateIndirectData  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPCreateIndirectData(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pcbIndirectData,
                                       SIP_INDIRECT_DATA* pIndirectData)
{
    FIXME("(%p %p %p) stub\n", pSubjectInfo, pcbIndirectData, pIndirectData);
 
    return FALSE;
}


/***********************************************************************
 *      CryptCATCDFClose  (WINTRUST.@)
 */
BOOL WINAPI CryptCATCDFClose(CRYPTCATCDF *pCDF)
{
    FIXME("(%p) stub\n", pCDF);

    return FALSE;
}

/***********************************************************************
 *      CryptCATCDFEnumCatAttributes  (WINTRUST.@)
 */
CRYPTCATATTRIBUTE * WINAPI CryptCATCDFEnumCatAttributes(CRYPTCATCDF *pCDF,
                                                        CRYPTCATATTRIBUTE *pPrevAttr,
                                                        PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    FIXME("(%p %p %p) stub\n", pCDF, pPrevAttr, pfnParseError);

    return NULL;
}

/***********************************************************************
 *      CryptCATCDFEnumMembersByCDFTagEx  (WINTRUST.@)
 */
LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTagEx(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                               PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                               CRYPTCATMEMBER **ppMember, BOOL fContinueOnError,
                                               LPVOID pvReserved)
{
    FIXME("(%p %s %p %p %d %p) stub\n", pCDF, debugstr_w(pwszPrevCDFTag), pfnParseError,
          ppMember, fContinueOnError, pvReserved);

    return NULL;
}

/***********************************************************************
 *      CryptCATCDFOpen  (WINTRUST.@)
 */
CRYPTCATCDF * WINAPI CryptCATCDFOpen(LPWSTR pwszFilePath,
                                     PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    FIXME("(%s %p) stub\n", debugstr_w(pwszFilePath), pfnParseError);

    return NULL;
}

static BOOL WINTRUST_GetSignedMsgFromPEFile(SIP_SUBJECTINFO *pSubjectInfo,
 DWORD *pdwEncodingType, DWORD dwIndex, DWORD *pcbSignedDataMsg,
 BYTE *pbSignedDataMsg)
{
    BOOL ret;
    WIN_CERTIFICATE *pCert = NULL;
    HANDLE file;

    TRACE("(%p %p %ld %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);

    if(pSubjectInfo->hFile && pSubjectInfo->hFile!=INVALID_HANDLE_VALUE)
        file = pSubjectInfo->hFile;
    else
    {
        file = CreateFileW(pSubjectInfo->pwsFileName, GENERIC_READ,
                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if(file == INVALID_HANDLE_VALUE)
            return FALSE;
    }
 
    if (!pbSignedDataMsg)
    {
        WIN_CERTIFICATE cert;

        /* app hasn't passed buffer, just get the length */
        ret = ImageGetCertificateHeader(file, dwIndex, &cert);
        if (ret)
        {
            switch (cert.wCertificateType)
            {
            case WIN_CERT_TYPE_X509:
            case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
                *pcbSignedDataMsg = cert.dwLength;
                break;
            default:
                WARN("unknown certificate type %d\n", cert.wCertificateType);
                ret = FALSE;
            }
        }
    }
    else
    {
        DWORD len = 0;

        ret = ImageGetCertificateData(file, dwIndex, NULL, &len);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            goto error;
        pCert = malloc(len);
        if (!pCert)
        {
            ret = FALSE;
            goto error;
        }
        ret = ImageGetCertificateData(file, dwIndex, pCert, &len);
        if (!ret)
            goto error;
        pCert->dwLength -= FIELD_OFFSET(WIN_CERTIFICATE, bCertificate);
        if (*pcbSignedDataMsg < pCert->dwLength)
        {
            *pcbSignedDataMsg = pCert->dwLength;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
        else
        {
            memcpy(pbSignedDataMsg, pCert->bCertificate, pCert->dwLength);
            *pcbSignedDataMsg = pCert->dwLength;
            switch (pCert->wCertificateType)
            {
            case WIN_CERT_TYPE_X509:
                *pdwEncodingType = X509_ASN_ENCODING;
                break;
            case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
                *pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
                break;
            default:
                WARN("don't know what to do for encoding type %d\n",
                 pCert->wCertificateType);
                *pdwEncodingType = 0;
                ret = FALSE;
            }
        }
    }
error:
    if(pSubjectInfo->hFile != file)
        CloseHandle(file);
    free(pCert);
    return ret;
}

static BOOL WINTRUST_PutSignedMsgToPEFile(SIP_SUBJECTINFO* pSubjectInfo, DWORD pdwEncodingType,
        DWORD* pdwIndex, DWORD cbSignedDataMsg, BYTE* pbSignedDataMsg)
{
    WIN_CERTIFICATE *cert;
    HANDLE file;
    DWORD size;
    BOOL ret;

    if(pSubjectInfo->hFile && pSubjectInfo->hFile!=INVALID_HANDLE_VALUE)
        file = pSubjectInfo->hFile;
    else
    {
        file = CreateFileW(pSubjectInfo->pwsFileName, GENERIC_READ|GENERIC_WRITE,
                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if(file == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    /* int aligned WIN_CERTIFICATE structure with cbSignedDataMsg+1 bytes of data */
    size = FIELD_OFFSET(WIN_CERTIFICATE, bCertificate[cbSignedDataMsg+4]) & (~3);
    cert = calloc(1, size);
    if(!cert)
        return FALSE;

    cert->dwLength = size;
    cert->wRevision = WIN_CERT_REVISION_2_0;
    cert->wCertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
    memcpy(cert->bCertificate, pbSignedDataMsg, cbSignedDataMsg);
    ret = ImageAddCertificate(file, cert, pdwIndex);

    free(cert);
    if(file != pSubjectInfo->hFile)
        CloseHandle(file);
    return ret;
}

/* structure offsets */
#define cfhead_Signature         (0x00)
#define cfhead_CabinetSize       (0x08)
#define cfhead_MinorVersion      (0x18)
#define cfhead_MajorVersion      (0x19)
#define cfhead_Flags             (0x1E)
#define cfhead_SIZEOF            (0x24)
#define cfheadext_HeaderReserved (0x00)
#define cfheadext_SIZEOF         (0x04)
#define cfsigninfo_CertOffset    (0x04)
#define cfsigninfo_CertSize      (0x08)
#define cfsigninfo_SIZEOF        (0x0C)

/* flags */
#define cfheadRESERVE_PRESENT          (0x0004)

/* endian-neutral reading of little-endian data */
#define EndGetI32(a)  ((((a)[3])<<24)|(((a)[2])<<16)|(((a)[1])<<8)|((a)[0]))
#define EndGetI16(a)  ((((a)[1])<<8)|((a)[0]))

/* For documentation purposes only:  this is the structure in the reserved
 * area of a signed cabinet file.  The cert offset indicates where in the
 * cabinet file the signature resides, and the count indicates its size.
 */
typedef struct _CAB_SIGNINFO
{
    WORD unk0; /* always 0? */
    WORD unk1; /* always 0x0010? */
    DWORD dwCertOffset;
    DWORD cbCertBlock;
} CAB_SIGNINFO, *PCAB_SIGNINFO;

static BOOL WINTRUST_GetSignedMsgFromCabFile(SIP_SUBJECTINFO *pSubjectInfo,
 DWORD *pdwEncodingType, DWORD dwIndex, DWORD *pcbSignedDataMsg,
 BYTE *pbSignedDataMsg)
{
    int header_resv;
    LONG base_offset, cabsize;
    USHORT flags;
    BYTE buf[64];
    DWORD cert_offset, cert_size, dwRead;

    TRACE("(%p %p %ld %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);

    /* get basic offset & size info */
    base_offset = SetFilePointer(pSubjectInfo->hFile, 0L, NULL, SEEK_CUR);

    if (SetFilePointer(pSubjectInfo->hFile, 0, NULL, SEEK_END) == INVALID_SET_FILE_POINTER)
    {
        TRACE("seek error\n");
        return FALSE;
    }

    cabsize = SetFilePointer(pSubjectInfo->hFile, 0L, NULL, SEEK_CUR);
    if ((cabsize == -1) || (base_offset == -1) ||
     (SetFilePointer(pSubjectInfo->hFile, 0, NULL, SEEK_SET) == INVALID_SET_FILE_POINTER))
    {
        TRACE("seek error\n");
        return FALSE;
    }

    /* read in the CFHEADER */
    if (!ReadFile(pSubjectInfo->hFile, buf, cfhead_SIZEOF, &dwRead, NULL) ||
     dwRead != cfhead_SIZEOF)
    {
        TRACE("reading header failed\n");
        return FALSE;
    }

    /* check basic MSCF signature */
    if (EndGetI32(buf+cfhead_Signature) != 0x4643534d)
    {
        WARN("cabinet signature not present\n");
        return FALSE;
    }

    /* Ignore the number of folders and files and the set and cabinet IDs */

    /* check the header revision */
    if ((buf[cfhead_MajorVersion] > 1) ||
        (buf[cfhead_MajorVersion] == 1 && buf[cfhead_MinorVersion] > 3))
    {
        WARN("cabinet format version > 1.3\n");
        return FALSE;
    }

    /* pull the flags out */
    flags = EndGetI16(buf+cfhead_Flags);

    if (!(flags & cfheadRESERVE_PRESENT))
    {
        TRACE("no header present, not signed\n");
        return FALSE;
    }

    if (!ReadFile(pSubjectInfo->hFile, buf, cfheadext_SIZEOF, &dwRead, NULL) ||
     dwRead != cfheadext_SIZEOF)
    {
        ERR("bunk reserve-sizes?\n");
        return FALSE;
    }

    header_resv = EndGetI16(buf+cfheadext_HeaderReserved);
    if (!header_resv)
    {
        TRACE("no header_resv, not signed\n");
        return FALSE;
    }
    else if (header_resv < cfsigninfo_SIZEOF)
    {
        TRACE("header_resv too small, not signed\n");
        return FALSE;
    }

    if (header_resv > 60000)
    {
        WARN("WARNING; header reserved space > 60000\n");
    }

    if (!ReadFile(pSubjectInfo->hFile, buf, cfsigninfo_SIZEOF, &dwRead, NULL) ||
     dwRead != cfsigninfo_SIZEOF)
    {
        ERR("couldn't read reserve\n");
        return FALSE;
    }

    cert_offset = EndGetI32(buf+cfsigninfo_CertOffset);
    TRACE("cert_offset: %ld\n", cert_offset);
    cert_size = EndGetI32(buf+cfsigninfo_CertSize);
    TRACE("cert_size: %ld\n", cert_size);

    /* The redundant checks are to avoid wraparound */
    if (cert_offset > cabsize || cert_size > cabsize ||
     cert_offset + cert_size > cabsize)
    {
        WARN("offset beyond file, not attempting to read\n");
        return FALSE;
    }

    SetFilePointer(pSubjectInfo->hFile, base_offset, NULL, SEEK_SET);
    if (!pbSignedDataMsg)
    {
        *pcbSignedDataMsg = cert_size;
        return TRUE;
    }
    if (*pcbSignedDataMsg < cert_size)
    {
        *pcbSignedDataMsg = cert_size;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    if (SetFilePointer(pSubjectInfo->hFile, cert_offset, NULL, SEEK_SET) == INVALID_SET_FILE_POINTER)
    {
        ERR("couldn't seek to cert location\n");
        return FALSE;
    }
    if (!ReadFile(pSubjectInfo->hFile, pbSignedDataMsg, cert_size, &dwRead,
     NULL) || dwRead != cert_size)
    {
        ERR("couldn't read cert\n");
        SetFilePointer(pSubjectInfo->hFile, base_offset, NULL, SEEK_SET);
        return FALSE;
    }
    /* The encoding of the files I've seen appears to be in ASN.1
     * format, and there isn't a field indicating the type, so assume it
     * always is.
     */
    *pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    /* Restore base offset */
    SetFilePointer(pSubjectInfo->hFile, base_offset, NULL, SEEK_SET);
    return TRUE;
}

static BOOL WINTRUST_GetSignedMsgFromCatFile(SIP_SUBJECTINFO *pSubjectInfo,
 DWORD *pdwEncodingType, DWORD dwIndex, DWORD *pcbSignedDataMsg,
 BYTE *pbSignedDataMsg)
{
    BOOL ret;

    TRACE("(%p %p %ld %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);

    if (!pbSignedDataMsg)
    {
        *pcbSignedDataMsg = GetFileSize(pSubjectInfo->hFile, NULL);
         ret = TRUE;
    }
    else
    {
        DWORD len = GetFileSize(pSubjectInfo->hFile, NULL);

        if (*pcbSignedDataMsg < len)
        {
            *pcbSignedDataMsg = len;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
        else
        {
            ret = ReadFile(pSubjectInfo->hFile, pbSignedDataMsg, len,
             pcbSignedDataMsg, NULL);
            if (ret)
                *pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
        }
    }
    return ret;
}

/* GUIDs used by CryptSIPGetSignedDataMsg and CryptSIPPutSignedDataMsg */
static const GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,
    0x00,0xC0,0x4F,0xC2,0x95,0xEE } };
static const GUID cabGUID = { 0xC689AABA, 0x8E78, 0x11D0, { 0x8C,0x47,
    0x00,0xC0,0x4F,0xC2,0x95,0xEE } };
static const GUID catGUID = { 0xDE351A43, 0x8E59, 0x11D0, { 0x8C,0x47,
     0x00,0xC0,0x4F,0xC2,0x95,0xEE }};

/***********************************************************************
 *      CryptSIPGetSignedDataMsg  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPGetSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pdwEncodingType,
                                       DWORD dwIndex, DWORD* pcbSignedDataMsg, BYTE* pbSignedDataMsg)
{
    BOOL ret;

    TRACE("(%p %p %ld %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);

    if(!pSubjectInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!memcmp(pSubjectInfo->pgSubjectType, &unknown, sizeof(unknown)))
        ret = WINTRUST_GetSignedMsgFromPEFile(pSubjectInfo, pdwEncodingType,
         dwIndex, pcbSignedDataMsg, pbSignedDataMsg);
    else if (!memcmp(pSubjectInfo->pgSubjectType, &cabGUID, sizeof(cabGUID)))
        ret = WINTRUST_GetSignedMsgFromCabFile(pSubjectInfo, pdwEncodingType,
         dwIndex, pcbSignedDataMsg, pbSignedDataMsg);
    else if (!memcmp(pSubjectInfo->pgSubjectType, &catGUID, sizeof(catGUID)))
        ret = WINTRUST_GetSignedMsgFromCatFile(pSubjectInfo, pdwEncodingType,
         dwIndex, pcbSignedDataMsg, pbSignedDataMsg);
    else
    {
        FIXME("unimplemented for subject type %s\n",
         debugstr_guid(pSubjectInfo->pgSubjectType));
        ret = FALSE;
    }

    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *      CryptSIPPutSignedDataMsg  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPPutSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD pdwEncodingType,
        DWORD* pdwIndex, DWORD cbSignedDataMsg, BYTE* pbSignedDataMsg)
{
    TRACE("(%p %ld %p %ld %p)\n", pSubjectInfo, pdwEncodingType, pdwIndex,
          cbSignedDataMsg, pbSignedDataMsg);

    if(!pSubjectInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(!memcmp(pSubjectInfo->pgSubjectType, &unknown, sizeof(unknown)))
        return WINTRUST_PutSignedMsgToPEFile(pSubjectInfo, pdwEncodingType,
                pdwIndex, cbSignedDataMsg, pbSignedDataMsg);
    else
        FIXME("unimplemented for subject type %s\n",
                debugstr_guid(pSubjectInfo->pgSubjectType));

    return FALSE;
}

/***********************************************************************
 *      CryptSIPRemoveSignedDataMsg  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPRemoveSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo,
                                       DWORD dwIndex)
{
    FIXME("(%p %ld) stub\n", pSubjectInfo, dwIndex);
 
    return FALSE;
}

/***********************************************************************
 *      CryptSIPVerifyIndirectData  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPVerifyIndirectData(SIP_SUBJECTINFO* pSubjectInfo,
                                       SIP_INDIRECT_DATA* pIndirectData)
{
    FIXME("(%p %p) stub\n", pSubjectInfo, pIndirectData);
 
    return FALSE;
}
