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

#include <sys/types.h>
#include "wine/list.h"


typedef struct _SecureProviderAp
{
    struct list             entry;
    BOOL                    loaded;
    PWSTR                   moduleName;
    HMODULE                 lib;
    SecurityFunctionTableA  fnTableA;
    SecurityFunctionTableW  fnTableW;
} SecureProviderAp;

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

typedef struct _SecurePackageTable
{
    DWORD numPackages;
    DWORD numAllocated;
    struct list table;
} SecurePackageTable;

typedef struct _SecureProviderTable
{
    DWORD numProviders;
    DWORD numAllocated;
    struct list table;
} SecureProviderTable;

/* Tries to load moduleName as a provider.  If successful, enumerates what
 * packages it can and adds them to the package and provider tables.  Resizes
 * tables as necessary.
 */
BOOL LoadSSPProvider(PWSTR moduleName);

BOOL LoadSSPAPProvider(PWSTR moduleName);


/* Allocates space for and initializes a new provider.  If fnTableA or fnTableW
 * is non-NULL, assumes the provider is built-in, and if moduleName is non-NULL,
 * means must load the LSA/user mode functions tables from external SSP/AP module.
 * Otherwise moduleName must not be NULL.
 * Returns a pointer to the stored provider entry, for use adding packages.
 */
SecureProvider *SECUR32_addProvider(const SecurityFunctionTableA *fnTableA,
 const SecurityFunctionTableW *fnTableW, PCWSTR moduleName);

/* Allocates space for and adds toAdd packages with the given provider.
 * provider must not be NULL, and either infoA or infoW may be NULL, but not
 * both.
 */
void SECUR32_addPackages(SecureProvider *provider, ULONG toAdd,
 const SecPkgInfoA *infoA, const SecPkgInfoW *infoW);

/* Tries to find the package named packageName.  If it finds it, implicitly
 * loads the package if it isn't already loaded.
 */
SecurePackage *SECUR32_findPackageW(PCWSTR packageName);

/* Tries to find the package named packageName.  (Thunks to _findPackageW)
 */
SecurePackage *SECUR32_findPackageA(PCSTR packageName);

/* A few string helpers; will return NULL if str is NULL.  Free return with
 * HeapFree */
PWSTR SECUR32_AllocWideFromMultiByte(PCSTR str);
PSTR  SECUR32_AllocMultiByteFromWide(PCWSTR str);

#endif
