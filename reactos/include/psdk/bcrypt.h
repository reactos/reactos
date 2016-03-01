/*
 * Copyright (C) 2007 Francois Gouget
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

#ifndef __WINE_BCRYPT_H
#define __WINE_BCRYPT_H

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef _NTDEF_
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
#endif

#define BCRYPT_ALGORITHM_NAME (const WCHAR []){'A','l','g','o','r','i','t','h','m','N','a','m','e',0}
#define BCRYPT_AUTH_TAG_LENGTH (const WCHAR []){'A','u','t','h','T','a','g','L','e','n','g','t','h',0}
#define BCRYPT_BLOCK_LENGTH (const WCHAR []){'B','l','o','c','k','L','e','n','g','t','h',0}
#define BCRYPT_BLOCK_SIZE_LIST (const WCHAR []){'B','l','o','c','k','S','i','z','e','L','i','s','t',0}
#define BCRYPT_CHAINING_MODE (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e',0}
#define BCRYPT_EFFECTIVE_KEY_LENGTH (const WCHAR []){'E','f','f','e','c','t','i','v','e','K','e','y','L','e','n','g','t','h',0}
#define BCRYPT_HASH_BLOCK_LENGTH (const WCHAR []){'H','a','s','h','B','l','o','c','k','L','e','n','g','t','h',0}
#define BCRYPT_HASH_LENGTH (const WCHAR []){'H','a','s','h','D','i','g','e','s','t','L','e','n','g','t','h',0}
#define BCRYPT_HASH_OID_LIST (const WCHAR []){'H','a','s','h','O','I','D','L','i','s','t',0}
#define BCRYPT_KEY_LENGTH (const WCHAR []){'K','e','y','L','e','n','g','t','h',0}
#define BCRYPT_KEY_LENGTHS (const WCHAR []){'K','e','y','L','e','n','g','t','h','s',0}
#define BCRYPT_KEY_OBJECT_LENGTH (const WCHAR []){'K','e','y','O','b','j','e','c','t','L','e','n','g','t','h',0}
#define BCRYPT_KEY_STRENGTH (const WCHAR []){'K','e','y','S','t','r','e','n','g','t','h',0}
#define BCRYPT_OBJECT_LENGTH (const WCHAR []){'O','b','j','e','c','t','L','e','n','g','t','h',0}
#define BCRYPT_PADDING_SCHEMES (const WCHAR []){'P','a','d','d','i','n','g','S','c','h','e','m','e','s',0}
#define BCRYPT_PROVIDER_HANDLE (const WCHAR []){'P','r','o','v','i','d','e','r','H','a','n','d','l','e',0}
#define BCRYPT_SIGNATURE_LENGTH (const WCHAR []){'S','i','g','n','a','t','u','r','e','L','e','n','g','t','h',0}

#define MS_PRIMITIVE_PROVIDER (const WCHAR [])\
    {'M','i','c','r','o','s','o','f','t',' ','P','r','i','m','i','t','i','v','e',' ','P','r','o','v','i','d','e','r',0}
#define MS_PLATFORM_CRYPTO_PROVIDER (const WCHAR [])\
    {'M','i','c','r','o','s','o','f','t',' ','P','l','a','t','f','o','r','m',' ','C','r','y','p','t','o',' ','P','r','o','v','i','d','e','r',0}

#define BCRYPT_SHA1_ALGORITHM       (const WCHAR []){'S','H','A','1',0}
#define BCRYPT_SHA256_ALGORITHM     (const WCHAR []){'S','H','A','2','5','6',0}
#define BCRYPT_SHA384_ALGORITHM     (const WCHAR []){'S','H','A','3','8','4',0}
#define BCRYPT_SHA512_ALGORITHM     (const WCHAR []){'S','H','A','5','1','2',0}

typedef struct _BCRYPT_ALGORITHM_IDENTIFIER
{
    LPWSTR pszName;
    ULONG  dwClass;
    ULONG  dwFlags;
} BCRYPT_ALGORITHM_IDENTIFIER;

typedef PVOID BCRYPT_ALG_HANDLE;
typedef PVOID BCRYPT_HANDLE;
typedef PVOID BCRYPT_HASH_HANDLE;

#define BCRYPT_RNG_USE_ENTROPY_IN_BUFFER 0x00000001
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG  0x00000002

NTSTATUS WINAPI BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE, ULONG);
NTSTATUS WINAPI BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptDestroyHash(BCRYPT_HASH_HANDLE);
NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG, ULONG *, BCRYPT_ALGORITHM_IDENTIFIER **, ULONG);
NTSTATUS WINAPI BCryptFinishHash(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptGetFipsAlgorithmMode(BOOLEAN *);
NTSTATUS WINAPI BCryptGetProperty(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptHashData(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, ULONG);

#endif  /* __WINE_BCRYPT_H */
