/*++




Copyright (c) 1992  Microsoft Corporation

Module Name:

    regnccls.h

Abstract:

    This file contains declarations needed for handling 
    change notifications in the classes portion of the registry

Author:

    Adam Edwards (adamed) 19-Aug-1998

Notes:

--*/

#if defined( LOCAL )

NTSTATUS BaseRegNotifyClassKey(
    IN  HKEY                     hKey,
    IN  HANDLE                   hEvent,
    IN  PIO_STATUS_BLOCK         pLocalIoStatusBlock,
    IN  DWORD                    dwNotifyFilter,
    IN  BOOLEAN                  fWatchSubtree,
    IN  BOOLEAN                  fAsynchronous);

NTSTATUS BaseRegGetBestAncestor(
    IN SKeySemantics*     pKeySemantics,
    IN HKEY               hkUser,
    IN HKEY               hkMachine,
    IN POBJECT_ATTRIBUTES pObja);


#endif // defined( LOCAL )
