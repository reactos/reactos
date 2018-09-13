/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    data.h

Abstract:

    Global data definitions for the MSAFD NTSD Debugger Extensions.

Author:

    Keith Moore (keithmo) 20-May-1996.

Environment:

    User Mode.

--*/


#ifndef _DATA_H_
#define _DATA_H_


extern EXT_API_VERSION        ApiVersion;
extern BOOLEAN                ExtensionApisInitialized;
extern WINDBG_EXTENSION_APIS  ExtensionApis;
extern ULONG                  STeip;
extern ULONG                  STebp;
extern ULONG                  STesp;
extern USHORT                 SavedMajorVersion;
extern USHORT                 SavedMinorVersion;

extern NTSD_EXTENSION_APIS    NtsdExtensionApis;
extern HANDLE                 ExtensionCurrentProcess;

#ifdef _AFD_SAN_SWITCH_

extern ULONG                  Options;

#endif // _AFD_SAN_SWITCH_

#endif  // _DATA_H_

