/*
 * LSA-mode functions of the SChannel security provider
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
#include "schannel.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(schannel);

/***********************************************************************
 *              SpGetInfoUnified
 */
static NTSTATUS WINAPI SpGetInfoUnified(PSecPkgInfoW PackageInfo)
{
    TRACE("(%p)\n", PackageInfo);

    PackageInfo->fCapabilities = SECPKG_FLAG_MUTUAL_AUTH |
        SECPKG_FLAG_INTEGRITY | SECPKG_FLAG_PRIVACY |
        SECPKG_FLAG_CONNECTION | SECPKG_FLAG_MULTI_REQUIRED |
        SECPKG_FLAG_EXTENDED_ERROR | SECPKG_FLAG_IMPERSONATION |
        SECPKG_FLAG_ACCEPT_WIN32_NAME | SECPKG_FLAG_STREAM;
    PackageInfo->wVersion   = 1;
    PackageInfo->wRPCID     = UNISP_RPC_ID;
    PackageInfo->cbMaxToken = 0x4000;
    PackageInfo->Name       = (LPWSTR)UNISP_NAME_W;
    PackageInfo->Comment    = (LPWSTR)UNISP_NAME_W;

    return STATUS_SUCCESS;
}

static SEC_WCHAR schannelCommentW[] = { 'S','c','h','a','n','n','e','l',
     ' ','S','e','c','u','r','i','t','y',' ','P','a','c','k','a','g','e',0 };

/***********************************************************************
 *              SpGetInfoSChannel
 */
static NTSTATUS WINAPI SpGetInfoSChannel(PSecPkgInfoW PackageInfo)
{
    TRACE("(%p)\n", PackageInfo);

    PackageInfo->fCapabilities = SECPKG_FLAG_MUTUAL_AUTH |
        SECPKG_FLAG_INTEGRITY | SECPKG_FLAG_PRIVACY |
        SECPKG_FLAG_CONNECTION | SECPKG_FLAG_MULTI_REQUIRED |
        SECPKG_FLAG_EXTENDED_ERROR | SECPKG_FLAG_IMPERSONATION |
        SECPKG_FLAG_ACCEPT_WIN32_NAME | SECPKG_FLAG_STREAM;
    PackageInfo->wVersion   = 1;
    PackageInfo->wRPCID     = UNISP_RPC_ID;
    PackageInfo->cbMaxToken = 0x4000;
    PackageInfo->Name       = (LPWSTR)SCHANNEL_NAME_W;
    PackageInfo->Comment    = schannelCommentW;

    return STATUS_SUCCESS;
}

static SECPKG_FUNCTION_TABLE secPkgFunctionTable[2] =
{ {
    NULL, /* InitializePackage */
    NULL, /* LsaLogonUser */
    NULL, /* CallPackage */
    NULL, /* LogonTerminated */
    NULL, /* CallPackageUntrusted */
    NULL, /* CallPackagePassthrough */
    NULL, /* LogonUserEx */
    NULL, /* LogonUserEx2 */
    NULL, /* Initialize */
    NULL, /* Shutdown */
    SpGetInfoUnified,
    NULL, /* AcceptCredentials */
    NULL, /* SpAcquireCredentialsHandle */
    NULL, /* SpQueryCredentialsAttributes */
    NULL, /* FreeCredentialsHandle */
    NULL, /* SaveCredentials */
    NULL, /* GetCredentials */
    NULL, /* DeleteCredentials */
    NULL, /* InitLsaModeContext */
    NULL, /* AcceptLsaModeContext */
    NULL, /* DeleteContext */
    NULL, /* ApplyControlToken */
    NULL, /* GetUserInfo */
    NULL, /* GetExtendedInformation */
    NULL, /* SpQueryContextAttributes */
    NULL, /* SpAddCredentials */
    NULL, /* SetExtendedInformation */
    NULL, /* SetContextAttributes */
    NULL, /* SetCredentialsAttributes */
  }, {
    NULL, /* InitializePackage */
    NULL, /* LsaLogonUser */
    NULL, /* CallPackage */
    NULL, /* LogonTerminated */
    NULL, /* CallPackageUntrusted */
    NULL, /* CallPackagePassthrough */
    NULL, /* LogonUserEx */
    NULL, /* LogonUserEx2 */
    NULL, /* Initialize */
    NULL, /* Shutdown */
    SpGetInfoSChannel,
    NULL, /* AcceptCredentials */
    NULL, /* SpAcquireCredentialsHandle */
    NULL, /* SpQueryCredentialsAttributes */
    NULL, /* FreeCredentialsHandle */
    NULL, /* SaveCredentials */
    NULL, /* GetCredentials */
    NULL, /* DeleteCredentials */
    NULL, /* InitLsaModeContext */
    NULL, /* AcceptLsaModeContext */
    NULL, /* DeleteContext */
    NULL, /* ApplyControlToken */
    NULL, /* GetUserInfo */
    NULL, /* GetExtendedInformation */
    NULL, /* SpQueryContextAttributes */
    NULL, /* SpAddCredentials */
    NULL, /* SetExtendedInformation */
    NULL, /* SetContextAttributes */
    NULL, /* SetCredentialsAttributes */
  }
};

/***********************************************************************
 *              SpLsaModeInitialize (SCHANNEL.@)
 */
NTSTATUS WINAPI SpLsaModeInitialize(ULONG LsaVersion, PULONG PackageVersion,
                                    PSECPKG_FUNCTION_TABLE *ppTables, PULONG pcTables)
{
    TRACE("(%u, %p, %p, %p)\n", LsaVersion, PackageVersion, ppTables, pcTables);

    *PackageVersion = SECPKG_INTERFACE_VERSION_3;
    *pcTables = 2;
    *ppTables = secPkgFunctionTable;

    return STATUS_SUCCESS;
}
