/*
 * init.c - DLL startup routines module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "init.h"


/****************************** Public Functions *****************************/


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

/*
** LibMain()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL APIENTRY LibMain(HANDLE hModule, DWORD dwReason, PVOID pvReserved)
{
   BOOL bResult;

   DebugEntry(LibMain);

   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
         bResult = AttachProcess(hModule);
         break;

      case DLL_PROCESS_DETACH:
         bResult = DetachProcess(hModule);
         break;

      case DLL_THREAD_ATTACH:
         bResult = AttachThread(hModule);
         break;

      case DLL_THREAD_DETACH:
         bResult = DetachThread(hModule);
         break;

      default:
         ERROR_OUT((TEXT("LibMain() called with unrecognized dwReason %lu."),
                    dwReason));
         bResult = FALSE;
         break;
   }

   DebugExitBOOL(LibMain, bResult);

   return(bResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */

