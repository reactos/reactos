/*
 * secur32 private definitions.
 *
 * Copyright (C) 2004 Juan Lang
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

#ifndef __SECUR32_PRIV_H__
#define __SECUR32_PRIV_H__

#include <wine/list.h>

typedef struct _SecureProvider
{
    struct list             entry;
    BOOL                    loaded;
    PWSTR                   moduleName;
    HMODULE                 lib;
    SecurityFunctionTableA  fnTableA;
    SecurityFunctionTableW  fnTableW;
} SecureProvider;

typedef struct _SecurePackage
{
    struct list     entry;
    SecPkgInfoW     infoW;
    SecureProvider *provider;
} SecurePackage;

/* Tries to find the package named packageName.  If it finds it, implicitly
 * loads the package if it isn't already loaded.
 */
SecurePackage *SECUR32_findPackageW(PCWSTR packageName) DECLSPEC_HIDDEN;

/* Tries to find the package named packageName.  (Thunks to _findPackageW)
 */
SecurePackage *SECUR32_findPackageA(PCSTR packageName) DECLSPEC_HIDDEN;

/* A few string helpers; will return NULL if str is NULL.  Free return with
 * HeapFree */
PWSTR SECUR32_AllocWideFromMultiByte(PCSTR str) DECLSPEC_HIDDEN;
PSTR  SECUR32_AllocMultiByteFromWide(PCWSTR str) DECLSPEC_HIDDEN;

#endif /* ndef __SECUR32_PRIV_H__ */
