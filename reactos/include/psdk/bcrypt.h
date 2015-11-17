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

#endif  /* __WINE_BCRYPT_H */
