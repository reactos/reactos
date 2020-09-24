/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxKm.h

Abstract:

    This file includes ntddk.h and
    kernel mode versions of mode agnostic headers

    It also contains definitions pulled out from wdm.h

Author:



Revision History:



--*/

#pragma once

#include <ntddk.h>
#include <procgrp.h>
#include <wdmsec.h>

#include <wmikm.h>
#include <ntwmi.h>

typedef KDEFERRED_ROUTINE MdDeferredRoutineType, *MdDeferredRoutine;
typedef EXT_CALLBACK MdExtCallbackType, *MdExtCallback;
#define FX_DEVICEMAP_PATH L"\\REGISTRY\\MACHINE\\HARDWARE\\DEVICEMAP\\"

#include "MxGeneralKm.h"
#include "MxLockKm.h"
#include "MxPagedLockKm.h"
#include "MxEventKm.h"
#include "MxMemoryKm.h"
#include "MxTimerKm.h"
#include "MxWorkItemKm.h"
#include "MxDriverObjectKm.h"
#include "MxDeviceObjectKm.h"
#include "MxFileObjectKm.h"

