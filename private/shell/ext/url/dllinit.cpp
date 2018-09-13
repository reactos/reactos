/*
 * dllinit.cpp - Initialization and termination routines.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "autodial.hpp"
#include "inetcpl.h"
#include "init.h"

#ifdef _X86_
BOOL g_bRunningOnNT = FALSE;
#endif

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL MyAttachProcess(HMODULE hmod);
PRIVATE_CODE BOOL MyDetachProcess(HMODULE hmod);


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

PUBLIC_DATA PCSTR g_pcszIniFile = "ohare.ini";
PUBLIC_DATA PCSTR g_pcszIniSection = "URLDebugOptions";

/* module name used by debspew.c!SpewOut() */

PUBLIC_DATA PCSTR g_pcszSpewModule = "URL";

#pragma data_seg()

#endif


/***************************** Private Functions *****************************/


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE BOOL MyAttachProcess(HMODULE hmod)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   DebugEntry(MyAttachProcess);

   bResult = (InitMemoryManagerModule() &&
              InitDataObjectModule());

#ifdef _X86_
   // Remember whether we're running on NT or not
   g_bRunningOnNT = (0 == (GetVersion() & 0x80000000));
#endif

#ifndef DEBUG

   // We don't need to get called on DLL_THREAD_ATTACH, etc.  This
   // is for perf gains.
   DisableThreadLibraryCalls(hmod);

#endif

   ASSERT(NULL == g_cserctrl.AttachThread &&
          NULL == g_cserctrl.DetachThread);

   DebugExitBOOL(MyAttachProcess, bResult);

   return(bResult);
}


PRIVATE_CODE BOOL MyDetachProcess(HMODULE hmod)
{
   BOOL bResult = TRUE;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   DebugEntry(MyDetachProcess);

   ExitInternetCPLModule();

   ExitDataObjectModule();

   ExitMemoryManagerModule();

   DebugExitBOOL(MyDetachProcess, bResult);

   return(bResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/****************************** Public Functions *****************************/


#ifdef DEBUG

PUBLIC_CODE BOOL SetAllIniSwitches(void)
{
   BOOL bResult;

   bResult = SetDebugModuleIniSwitches();
   bResult = SetSerialModuleIniSwitches() && bResult;
   bResult = SetMemoryManagerModuleIniSwitches() && bResult;

   return(bResult);
}

#endif

