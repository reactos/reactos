/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.h
 * PURPOSE:         Regression testing
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <ntos.h>

/* Valid values for Command parameter of TestRoutine */
#define TESTCMD_RUN       0   /* Buffer contains information about what failed */
#define TESTCMD_TESTNAME  1   /* Buffer contains description of test */

/* Valid values for return values of TestRoutine */
#define TS_EXCEPTION     -1
#define TS_OK             0
#define TS_FAILED         1

/*
 * Test routine prototype
 * Command - The command to process
 * Buffer - Pointer to buffer in which to return context information
 */
typedef int (*TestRoutine)(int Command, char *Buffer);

/* Test driver entry routine */
typedef VOID STDCALL (*TestDriverMain)(); 

typedef struct _ROS_TEST
{
  LIST_ENTRY ListEntry;
  TestRoutine Routine;
} ROS_TEST, *PROS_TEST;

extern LIST_ENTRY AllTests;

extern VOID InitializeTests();
extern VOID RegisterTests();
extern VOID PerformTests();

/* Routines provided by the driver */
extern PVOID AllocateMemory(ULONG Size);
extern VOID FreeMemory(PVOID Base);
