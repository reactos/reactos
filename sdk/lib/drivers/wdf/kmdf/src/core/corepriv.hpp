/*++

Copyright (c) Microsoft Corporation

Module Name:

    corepriv.hpp

Abstract:

    Main driver framework private header.

Author:



Environment:

    Kernel mode only

Revision History:

--*/


extern "C" {
#include <ntddk.h>
#include "wdf.h"
}

#define WDF_REGISTRY_BASE_PATH L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf"

