/*
 * dllinit.c - Initialization and termination routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "..\core\init.h"
#include "server.h"
#include "cnrlink.h"


/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL MyAttachProcess(HMODULE);
PRIVATE_CODE BOOL MyDetachProcess(HMODULE);


/* Global Variables
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

/* serialization control structure */
/* note no thread attach or thread detach procs here so we can optimize... */

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
PUBLIC_DATA LPCTSTR GpcszIniSection = TEXT("LinkInfoDebugOptions");

/* module name used by debug.c!SpewOut() */

PUBLIC_DATA LPCTSTR GpcszSpewModule = TEXT("LinkInfo");

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

   // Don't care about thread attach/detach.
   DisableThreadLibraryCalls(hmod);
   bResult = ProcessInitServerModule();

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

   ProcessExitServerModule();

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
   bResult |= SetSerialModuleIniSwitches();
   bResult |= SetMemoryManagerModuleIniSwitches();

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

   DebugEntry(InitializeDLL);

   bResult = InitMemoryManagerModule();

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
   BOOL bResult;

   DebugEntry(TerminateDLL);

   ExitMemoryManagerModule();

   bResult = TRUE;

   DebugExitBOOL(TerminateDLL, bResult);

   return(bResult);
}
