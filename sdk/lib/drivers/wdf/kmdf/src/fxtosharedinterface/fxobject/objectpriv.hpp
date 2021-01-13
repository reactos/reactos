/*++

Copyright (c) Microsoft Corporation

Module Name:

    corepriv.hpp

Abstract:

    Private header file for FxToSharedInterface\FxObject directory
    It is then included in objectpch.hpp

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

