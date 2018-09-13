/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    osdef.h

Abstract:

    This file contains miscellaneous operating system specific definitions.

Author:

    Keith Moore keithmo@microsoft.com   03-OCT-1995

Revision History:


--*/

#ifndef _OSDEF_
#define _OSDEF_


#include <ntverp.h>


//
// Protocol catalog mutex name. This isn't really operating system specific,
// but it needs to go somewhere shared between WS2_32.DLL and WSOCK32.DLL
// (for Setup Migration).
//

#define CATALOG_MUTEX_NAME  "Winsock2ProtocolCatalogMutex"


//
// Winsock configuration registry root key name (lives under HKLM).
//

#define WINSOCK_REGISTRY_ROOT \
    "System\\CurrentControlSet\\Services\\WinSock2\\Parameters"


//
// Registry version info.
//

#define WINSOCK_REGISTRY_VERSION_NAME "WinSock_Registry_Version"
#define WINSOCK_REGISTRY_VERSION_VALUE "2.0"

#define WINSOCK_CURRENT_PROTOCOL_CATALOG_NAME "Current_Protocol_Catalog"
#define WINSOCK_CURRENT_NAMESPACE_CATALOG_NAME "Current_NameSpace_Catalog"


//
// Enable tracing on debug builds.
//

#if DBG
#define DEBUG_TRACING
#define TRACING
#define BUILD_TAG_STRING    "Windows NT " VER_PRODUCTVERSION_STR
#endif


#endif  // _OSDEF_

