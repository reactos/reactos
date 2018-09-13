/*
 * dllinit.c - Initialization and termination routines.
 */

/*

Implementation Notes
--------------------

   Here are a few conventions I have attempted to follow in the object
synchronization engine:

1) Functions have only one exit point.

2) When calling a function that takes one or more pointers to variables to be
filled in with a result, the caller may only depend upon the variables being
filled in correctly if the function returns success.

3) AllocateMemory() and FreeMemory() are called instead of _fmalloc() and
_ffree() to allow debug manipulation of the heap.

4) Two layers of parameter validation have been implemented - validation of
parameters passed in from external callers and validation of parameters passed
in from internal callers.  #defining EXPV enables the external parameter
validation layer.  The internal parameter validation layer is only included in
the debug build.  The external parameter validation layer fails any call with
an invalid parameter, returning TR_INVALID_PARAMETER.  The internal parameter
validation layer displays a debug message when a call is made with an invalid
parameter, but allows the call to proceed.  External parameter validation is
available in all builds.  Internal parameter validation is only available in
the DEBUG build.

5) In addition to the two layers of parameter validation, validation of fields
of structures passed as arguments may be enabled by #defining VSTF.  Full
parent and child structure field validation can be quite time-consuming.  Field
validation for external structure parameters is available in all builds.  Field
validation for internal structure parameters is only available in the DEBUG
build.  (Full parameter and structure field validation has proven very valuable
in debugging.)

6) Some debug bounds check ASSERT()s use floating point math.  These floating
point bounds checks are only enabled if DBLCHECK is #defined.  Defining
DBLCHECK requires linking with the CRT library for floating point support.

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "init.h"


/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL MyAttachProcess(HMODULE);
PRIVATE_CODE BOOL MyDetachProcess(HMODULE);


/* Global Variables
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

/* serialization control structure */

PUBLIC_DATA CSERIALCONTROL g_cserctrl =
{
   MyAttachProcess,
   MyDetachProcess,
   NULL,
   NULL
};

#pragma data_seg()

#ifdef DEBUG

#pragma data_seg(DATA_SEG_READ_ONLY)

/* .ini file name and section used by inifile.c!SetIniSwitches() */

PUBLIC_DATA LPCTSTR GpcszIniFile = TEXT("rover.ini");
PUBLIC_DATA LPCTSTR GpcszIniSection = TEXT("SyncEngineDebugOptions");

/* module name used by debug.c!SpewOut() */

PUBLIC_DATA LPCTSTR GpcszSpewModule = TEXT("SyncEng");

#pragma data_seg()

#endif


/***************************** Private Functions *****************************/


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

/*
** MyAttachProcess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL MyAttachProcess(HMODULE hmod)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   DebugEntry(MyAttachProcess);

   bResult = (ProcessInitOLEPigModule() &&
              ProcessInitStorageModule());

   DebugExitBOOL(MyAttachProcess, bResult);

   return(bResult);
}


/*
** MyDetachProcess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL MyDetachProcess(HMODULE hmod)
{
   BOOL bResult = TRUE;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   DebugEntry(MyDetachProcess);

   ProcessExitStorageModule();

   ProcessExitOLEPigModule();

   DebugExitBOOL(MyDetachProcess, bResult);

   return(bResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/****************************** Public Functions *****************************/


#ifdef DEBUG

/*
** SetAllIniSwitches()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SetAllIniSwitches(void)
{
   BOOL bResult;

   bResult = SetDebugModuleIniSwitches();
   bResult = SetSerialModuleIniSwitches() && bResult;
   bResult = SetMemoryManagerModuleIniSwitches() && bResult;
   bResult = SetBriefcaseModuleIniSwitches() && bResult;

   return(bResult);
}

#endif


/*
** InitializeDLL()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL InitializeDLL(void)
{
   BOOL bResult;

#ifdef DEBUG
   DebugEntry(InitializeDLL);

   EVAL(InitDebugModule());

#endif

   bResult = (InitMemoryManagerModule() &&
              InitBriefcaseModule());

#ifdef DEBUG

   SpewHeapSummary(0);

#endif

   DebugExitBOOL(InitializeDLL, bResult);

   return(bResult);
}


/*
** TerminateDLL()
**
**
**
** Arguments:
**
** Returns:       TRUE
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL TerminateDLL(void)
{
   BOOL bResult = TRUE;

   DebugEntry(TerminateDLL);

#ifdef DEBUG

   SpewHeapSummary(0);

   TRACE_OUT((TEXT("TerminateDLL(): Starting heap cleanup.")));

#endif

   ExitBriefcaseModule();

#ifdef DEBUG

   TRACE_OUT((TEXT("TerminateDLL(): Heap cleanup complete.")));

   SpewHeapSummary(SHS_FL_SPEW_USED_INFO);

#endif

   ExitMemoryManagerModule();

#ifdef DEBUG

   ExitDebugModule();

#endif

   DebugExitBOOL(TerminateDLL, bResult);

   return(bResult);
}

