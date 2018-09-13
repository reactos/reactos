/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    data.h

Abstract:

    This module contains global data definitions.

Author:
    Michael Tsang (MikeTs) 1-Sep-1998

Environment:

    Kernel mode


Revision History:


--*/

#ifndef _DATA_H
#define _DATA_H

extern CONST GUID IoepGuid;
extern KSPIN_LOCK IoepErrListLock;
extern LIST_ENTRY IoepErrThreadListHead;
extern LIST_ENTRY IoepErrModuleListHead;
extern LIST_ENTRY IoepSaveDataListHead;
extern UNICODE_STRING IoepRegKeyStrIoErr;
extern PERRCASEDB IoepErrCaseDB;

#endif  //ifndef _DATA_H
