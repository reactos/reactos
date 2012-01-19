/*
 * User-mode functions of the SChannel security provider
 *
 * Copyright 2007 Yuval Fledel
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(schannel);

static SECPKG_USER_FUNCTION_TABLE secPkgUserTables[2] =
{ {
    NULL, /* InstanceInit */
    NULL, /* InitUserModeContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    NULL, /* SealMessage */
    NULL, /* UnsealMessage */
    NULL, /* GetContextToken */
    NULL, /* SpQueryContextAttributes */
    NULL, /* CompleteAuthToken */
    NULL, /* DeleteUserModeContext */
    NULL, /* FormatCredentials */
    NULL, /* MarshallSupplementalCreds */
    NULL, /* ExportContext */
    NULL, /* ImportContext */
  }, {
    NULL, /* InstanceInit */
    NULL, /* InitUserModeContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    NULL, /* SealMessage */
    NULL, /* UnsealMessage */
    NULL, /* GetContextToken */
    NULL, /* SpQueryContextAttributes */
    NULL, /* CompleteAuthToken */
    NULL, /* DeleteUserModeContext */
    NULL, /* FormatCredentials */
    NULL, /* MarshallSupplementalCreds */
    NULL, /* ExportContext */
    NULL, /* ImportContext */
  }
};

/***********************************************************************
 *              SpUserModeInitialize (SCHANNEL.@)
 */
NTSTATUS WINAPI SpUserModeInitialize(ULONG LsaVersion, PULONG PackageVersion,
  PSECPKG_USER_FUNCTION_TABLE *ppTables, PULONG pcTables)
{
    TRACE("(%u, %p, %p, %p)\n", LsaVersion, PackageVersion, ppTables, pcTables);

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
        return STATUS_INVALID_PARAMETER;

    *PackageVersion = SECPKG_INTERFACE_VERSION;
    *pcTables = 2;
    *ppTables = secPkgUserTables;

    return STATUS_SUCCESS;
}
