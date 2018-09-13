
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name

   usrbench.h

Abstract:

    USER performance numbers

Author:

   Dan Almosnino (danalm) 25-July-1996
   Based on code by Mark Enstrom   (marke)  13-Apr-1995

Enviornment:

   User Mode

Revision History:

    Dan Almosnino (danalm) 20-Sept-1995

    Added some default values for text string-related tests

    Dan Almosnino (danalm) 17-Oct-1995

    Added some default values, globals and new functions for batch mode execution

    Dan Almosnino (danalm) 20-Nov-1995

    Included header files for Pentium Cycle Counter and Statistics module.
    Added some variables for statistic processing.

    Dan Almosnino (danalm) 25-July-1996

    Adapted from GDIbench to USERbench
--*/



int PASCAL
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR szCmdLine,
    int cmdShow
);

LRESULT FAR
PASCAL WndProc(
    HWND        hWnd,
    unsigned    msg,
    WPARAM      wParam,
    LPARAM      lParam);


ULONGLONG
msSetBkColor(
   HDC   hdc,
   ULONG iter
   );

ULONGLONG
msGetBkColor(
   HDC   hdc,
   ULONG iter
   );

ULONGLONG
msCreateDCW(
   HDC   hdc,
   ULONG iter
   );

ULONGLONG
msCreateDCA(
   HDC   hdc,
   ULONG iter
   );

INT_PTR
APIENTRY
ResultsDlgProc(
    HWND,
    UINT,
    WPARAM,
    LPARAM);

INT_PTR
APIENTRY
HelpDlgProc(
    HWND,
    UINT,
    WPARAM,
    LPARAM);


VOID
SaveResults();

char *
SelectOutFileName(
    HWND hWnd
    );

VOID
WriteBatchResults(
    FILE *fpOut,
    int  TestType,
    int  cycle
    );

int
SyncMenuChecks(
    HWND hWnd,
    int Last_Checked,
    int New_Checked
    );

int
Std_Parse(
    char *txtbuf,
    int limit,
    int *array);

#ifdef _X86_
#include "cycle.h"
#endif

#include "stats.h"

typedef ULONG (*PFN_MS)(HDC,ULONG);

typedef struct _TEST_ENTRY
{
    PUCHAR Api;
    PFN_MS pfn;
    ULONG  Iter;
    ULONG  Result;

}TEST_ENTRY,*PTEST_ENTRY;

#define CMD_IS(x) (NULL != strstr(szCmdLine,x))

#define INIT_TIMER    ULONGLONG StartTime,StopTime; \
                      ULONG ix = Iter; \
                      ULONGLONG overhead = 0; \
                      ULONGLONG ov1, ov2


#define START_TIMER   UpdateWindow(ghwndMDIClient); \
                      StartTime = BeginTimeMeasurement()

#define END_TIMER_NO_RETURN \
     StopTime = EndTimeMeasurement(StartTime + overhead, Iter)

#define RETURN_STOP_TIME        return StopTime

#define END_TIMER \
    StopTime = EndTimeMeasurement(StartTime + overhead, Iter); \
    return StopTime

#ifdef _X86_
#define START_OVERHEAD \
    if(gfPentium) \
        ov1 = GetCycleCount(); \
    else \
        QueryPerformanceCounter((LARGE_INTEGER *)&ov1)
#define END_OVERHEAD \
    if(gfPentium) \
        ov2 = GetCycleCount(); \
    else \
        QueryPerformanceCounter((LARGE_INTEGER *)&ov2); \
    overhead += ov2 - ov1

#else // !_X86_

#define START_OVERHEAD \
    QueryPerformanceCounter((LARGE_INTEGER *)&ov1)
#define END_OVERHEAD \
    QueryPerformanceCounter((LARGE_INTEGER *)&ov2); \
    overhead += ov2 - ov1

#endif


#define FIRST_TEXT_FUNCTION 10
#define LAST_TEXT_FUNCTION 19

#define DEFAULT_STRING_LENGTH 32
#define DEFAULT_A_STRING    "This is just a silly test string"
#define DEFAULT_W_STRING    L"This is just a silly test string"

#define ALL             11
#define QUICK           12
#define TEXT_SUITE      13
#define SELECT          14
#define POINTS_PER_INCH 72

extern ULONG gNumTests;
extern ULONG gNumQTests;
extern TEST_ENTRY  gTestEntry[];


#define NUM_TESTS  gNumTests
#define NUM_QTESTS gNumQTests
#define NUM_SAMPLES     10      // Number of test samples to be taken, each performing the test
                                    // TEST_DEFAULT times
#define VAR_LIMIT       3           // Desired Variation Coefficient (StdDev/Average) in percents

#ifdef MAIN_MODULE
#define PUBLIC
#else
#define PUBLIC  extern
#endif

PUBLIC TEST_STATS TestStats[200];           // Sample Array [of size at least as number of test entries]
PUBLIC long Detailed_Data[200][NUM_SAMPLES];        // Storage for detailed sample data

PUBLIC _int64   PerformanceFreq;        /* Timer Frequency  */

// Text String Tests Related

PUBLIC size_t   StrLen;

PUBLIC char     SourceString[129];
PUBLIC wchar_t SourceStringW[129];
PUBLIC char     DestString[256];
PUBLIC wchar_t DestStringW[256];
PUBLIC wchar_t WCstrbuf[256];


PUBLIC BYTE    DisplayHelp;

//  Batch Mode Related

PUBLIC BYTE     TextSuiteFlag;
PUBLIC BYTE     BatchFlag;
PUBLIC int  BatchCycle;
PUBLIC BYTE     Finish_Message;
PUBLIC BYTE     Dont_Close_App;
PUBLIC BYTE     SelectedFontTransparent;
PUBLIC BYTE     String_Length_Warn;
PUBLIC BYTE Print_Detailed;

PUBLIC FILE *fpIniFile;
PUBLIC FILE *fpOutFile;
PUBLIC char IniFileName[80];
PUBLIC char *OutFileName;
