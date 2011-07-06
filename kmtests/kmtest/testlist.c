/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite user-mode test list
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <kmt_test.h>

VOID Test_Create(VOID);
VOID Test_Delete(VOID);
VOID Test_Start(VOID);
VOID Test_Stop(VOID);
VOID Test_Example(VOID);

/* tests with a leading '-' will not be listed */
const KMT_TEST TestList[] =
{
    { "-create",            Test_Create },
    { "-delete",            Test_Delete },
    { "-start",             Test_Start },
    { "-stop",              Test_Stop, },
    { "Example",            Test_Example },
    { NULL,                 NULL },
};
