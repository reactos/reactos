/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NTLM globals definitions (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier <staubim@quantentunnel.de>
 */

#include "../precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* globals */
NTLM_MODE NtlmMode = NtlmUnknownMode;

LSA_DISPATCH_TABLE DispatchTable;

PLSA_SECPKG_FUNCTION_TABLE LsaFunctions = NULL;
/* msv1_0 (XP, win2k) returns NULL for
 * InitializePackage, LsaLogonUser,LsaLogonUserEx,
 * SpQueryContextAttributes and SpAddCredentials */
SECPKG_FUNCTION_TABLE NtlmLsaFn[1] = 
{
    {
        .InitializePackage = NULL,
        .LsaLogonUser = NULL,
        .CallPackage = LsaApCallPackage,
        .LogonTerminated = LsaApLogonTerminated,
        .CallPackageUntrusted = LsaApCallPackageUntrusted,
        .CallPackagePassthrough = LsaApCallPackagePassthrough,
        .LogonUserEx = NULL,
        .LogonUserEx2 = LsaApLogonUserEx2,
        .Initialize = SpInitialize,
        .Shutdown = LsaSpShutDown,
        .GetInfo = LsaSpGetInfoW,
        .AcceptCredentials = SpAcceptCredentials,
        .SpAcquireCredentialsHandle = LsaSpAcquireCredentialsHandle,
        .SpQueryCredentialsAttributes = LsaSpQueryCredentialsAttributes,
        .FreeCredentialsHandle = LsaSpFreeCredentialsHandle,
        .SaveCredentials = LsaSpSaveCredentials,
        .GetCredentials = LsaSpGetCredentials,
        .DeleteCredentials = LsaSpDeleteCredentials,
        .InitLsaModeContext = LsaSpInitLsaModeContext,
        .AcceptLsaModeContext = LsaSpAcceptLsaModeContext,
        .DeleteContext = LsaSpDeleteContext,
        .ApplyControlToken = LsaSpApplyControlToken,
        .GetUserInfo = LsaSpGetUserInfo,
        .GetExtendedInformation = LsaSpGetExtendedInformation,
        .SpQueryContextAttributes = NULL,
        .SpAddCredentials = NULL,
        .SetExtendedInformation = LsaSpSetExtendedInformation
    }
};

PSECPKG_DLL_FUNCTIONS UsrFunctions = NULL;
SECPKG_USER_FUNCTION_TABLE NtlmUsrFn[1] =
{
    {
        .InstanceInit = SpInstanceInit,
        .InitUserModeContext = UsrSpInitUserModeContext,
        .MakeSignature = UsrSpMakeSignature,
        .VerifySignature = UsrSpVerifySignature,
        .SealMessage = UsrSpSealMessage,
        .UnsealMessage = UsrSpUnsealMessage,
        .GetContextToken = UsrSpGetContextToken,
        .SpQueryContextAttributes = UsrSpQueryContextAttributes,
        .CompleteAuthToken = UsrSpCompleteAuthToken,
        .DeleteUserModeContext = UsrSpDeleteUserModeContext,
        .FormatCredentials = UsrSpFormatCredentials,
        .MarshallSupplementalCreds = UsrSpMarshallSupplementalCreds,
        .ExportContext = UsrSpExportSecurityContext,
        .ImportContext = UsrSpImportSecurityContext
    }
};
