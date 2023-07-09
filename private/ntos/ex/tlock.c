/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tlock.c

Abstract:

    Test program for exinterlockedincrement/decrement routines

Environment:

    This program can either be compiled into a kernel mode test,
    OR you can link intrloc2.obj or i386\intrlock.obj (or whatever)
    into it and run it in user mode.

Author:

    Bryan Willman (bryanwi) 3-Aug-90

Revision History:


--*/

#include "exp.h"


main()
{
    INTERLOCKED_RESULT  RetVal;
    LONG                SpinVar;            // talk about a hack...
    LONG                LongVar;
    SHORT               ShortVar;
    KSPIN_LOCK          Lock;

    Lock = &SpinVar;

    LongVar = 0;
    ShortVar = 0;

    RetVal = ExInterlockedDecrementLong(&LongVar, &Lock);
    if ((RetVal != ResultNegative) ||
        (LongVar != -1)) {
        DbgPrint("t&Lock failure #L1\n");
    }

    RetVal = ExInterlockedDecrementLong(&LongVar, &Lock);
    if ((RetVal != ResultNegative) ||
        (LongVar != -2)) {
        DbgPrint("t&Lock failure #L2\n");
    }

    RetVal = ExInterlockedIncrementLong(&LongVar, &Lock);
    if ((RetVal != ResultNegative) ||
        (LongVar != -1)) {
        DbgPrint("t&Lock failure #L3\n");
    }

    RetVal = ExInterlockedIncrementLong(&LongVar, &Lock);
    if ((RetVal != ResultZero) ||
        (LongVar != 0)) {
        DbgPrint("t&Lock failure #L4\n");
    }

    RetVal = ExInterlockedIncrementLong(&LongVar, &Lock);
    if ((RetVal != ResultPositive) ||
        (LongVar != 1)) {
        DbgPrint("t&Lock failure #L5\n");
    }

    RetVal = ExInterlockedIncrementLong(&LongVar, &Lock);
    if ((RetVal != ResultPositive) ||
        (LongVar != 2)) {
        DbgPrint("t&Lock failure #L6\n");
    }

    RetVal = ExInterlockedDecrementLong(&LongVar, &Lock);
    if ((RetVal != ResultPositive) ||
        (LongVar != 1)) {
        DbgPrint("t&Lock failure #L7\n");
    }

    RetVal = ExInterlockedDecrementLong(&LongVar, &Lock);
    if ((RetVal != ResultZero) ||
        (LongVar != 0)) {
        DbgPrint("t&Lock failure #L8\n");
    }
}
