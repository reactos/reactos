/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/ntlm/protocol.h
 * PURPOSE:     ntlm globals definitions (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include "../precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* globals */

PSECPKG_DLL_FUNCTIONS UsrFunctions = NULL;
SECPKG_USER_FUNCTION_TABLE NtlmUsrFn[1];
