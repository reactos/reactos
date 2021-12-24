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
