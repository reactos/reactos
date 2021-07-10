/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ntlm globals definitions (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#ifndef _MSV1_0_NTLM_GLOBALS_H_
#define _MSV1_0_NTLM_GLOBALS_H_

typedef enum _NTLM_MODE
{
    NtlmUnknownMode = 0,
    NtlmLsaMode = 1,
    NtlmUserMode
} NTLM_MODE, *PNTLM_MODE;

extern NTLM_MODE NtlmMode;

extern LSA_DISPATCH_TABLE DispatchTable;

/* functions provided by LSA in SpInitialize */
extern PLSA_SECPKG_FUNCTION_TABLE LsaFunctions;
/* functions we provide to LSA in SpLsaModeInitialize */
extern SECPKG_FUNCTION_TABLE NtlmLsaFn[1];


#endif
