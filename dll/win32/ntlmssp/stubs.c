/*
 * Copyright 2011 Samuel Serapion
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
 *
 */

#include "ntlmssp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* initialize all to null since we still dont use them */
SECPKG_FUNCTION_TABLE NtLmPkgFuncTable; //functions we provide to LSA in SpLsaModeInitialize
PSECPKG_DLL_FUNCTIONS NtlmPkgDllFuncTable = NULL; //fuctions provided by LSA in SpInstanceInit
SECPKG_USER_FUNCTION_TABLE NtlmUmodeFuncTable; //fuctions we provide via SpUserModeInitialize
PLSA_SECPKG_FUNCTION_TABLE NtlmLsaFuncTable = NULL; // functions provided by LSA in SpInitialize

