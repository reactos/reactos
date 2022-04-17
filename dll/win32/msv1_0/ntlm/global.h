/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NTLM globals definitions (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier <staubim@quantentunnel.de>
 */

#pragma once

/* functions provided by LSA in SpInitialize */
extern PLSA_SECPKG_FUNCTION_TABLE LsaFunctions;
/* functions we provide to LSA in SpLsaModeInitialize */
extern SECPKG_FUNCTION_TABLE NtlmLsaFn[1];
/* functions provided by LSA in SpInstanceInit */
extern PSECPKG_DLL_FUNCTIONS UsrFunctions;
/* functions we provide to LSA in SpUserModeInitialize */
extern SECPKG_USER_FUNCTION_TABLE NtlmUsrFn[1];

extern LSA_DISPATCH_TABLE DispatchTable;

typedef enum _NTLM_MODE
{
    NtlmUnknownMode = 0,
    NtlmLsaMode = 1,
    NtlmUserMode
} NTLM_MODE, *PNTLM_MODE;

extern NTLM_MODE NtlmMode;
