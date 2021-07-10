/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ntlm globals definitions (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include "../precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* globals */
NTLM_MODE NtlmMode = NtlmUnknownMode;

LSA_DISPATCH_TABLE DispatchTable;

PLSA_SECPKG_FUNCTION_TABLE LsaFunctions = NULL;

SECPKG_FUNCTION_TABLE NtlmLsaFn[1];

