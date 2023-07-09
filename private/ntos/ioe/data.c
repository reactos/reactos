/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This module contains the declarations of global data

Author:

    Michael Tsang (MikeTs) 2-Sep-1998

Environment:

    Kernel mode

Revision History:


--*/

#include "pch.h"

CONST GUID IoepGuid = {0x07dea000, 0x5fdc, 0x11d2,
                       {0x9b, 0xae, 0x00, 0xc0, 0x4f, 0x8e, 0xcb, 0x68}};
KSPIN_LOCK IoepErrListLock = {0};
LIST_ENTRY IoepErrThreadListHead = {0};
LIST_ENTRY IoepErrModuleListHead = {0};
LIST_ENTRY IoepSaveDataListHead = {0};
UNICODE_STRING IoepRegKeyStrIoErr = {0};
PERRCASEDB IoepErrCaseDB = NULL;



