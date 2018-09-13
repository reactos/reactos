/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    globals.c

Abstract:

    MSAFDEXT globals.

Author:

    Keith Moore (keithmo) 20-May-1996

Revision History:

--*/


#include "msafdext.h"




//
// Public globals.
//

EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
BOOLEAN                ExtensionApisInitialized = FALSE;
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;

NTSD_EXTENSION_APIS    NtsdExtensionApis;
HANDLE                 ExtensionCurrentProcess;

#ifdef _AFD_SAN_SWITCH_

ULONG                  Options;

#endif // _AFD_SAN_SWITCH_
