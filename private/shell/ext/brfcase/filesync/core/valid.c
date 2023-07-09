/*
 * valid.c - Validation functions module.
*/


/* Headers
*/

#include "project.h"
#pragma hdrstop


/****************************** Public Functions *****************************/


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHANDLE(HANDLE hnd)
{
   return(EVAL(hnd != INVALID_HANDLE_VALUE));
}


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHFILE(HANDLE hf)
{
   return(IsValidHANDLE(hf));
}


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHWND(HWND hwnd)
{
   /* Ask User if this is a valid window. */

   return(IsWindow(hwnd));
}


#if defined(DEBUG) || defined(VSTF)

/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCSECURITY_ATTRIBUTES(PCSECURITY_ATTRIBUTES pcsa)
{
   return(IS_VALID_READ_PTR(pcsa, CSECURITY_ATTRIBUTES));
}


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidFileCreationMode(DWORD dwMode)
{
   BOOL bResult;

   switch (dwMode)
   {
      case CREATE_NEW:
      case CREATE_ALWAYS:
      case OPEN_EXISTING:
      case OPEN_ALWAYS:
      case TRUNCATE_EXISTING:
         bResult = TRUE;
         break;
      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidFileCreationMode(): Invalid file creation mode %#lx."), dwMode));
         break;
   }

   return(bResult);
}


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHTEMPLATEFILE(HANDLE htf)
{
   return(IsValidHANDLE(htf));
}


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCFILETIME(PCFILETIME pcft)
{
   return(IS_VALID_READ_PTR(pcft, CFILETIME));
}

#endif


#ifdef DEBUG

/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHINSTANCE(HINSTANCE hinst)
{
   return(EVAL(hinst));
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHICON(HICON hicon)

{
   /* Any value is a valid HICON. */

   return(TRUE);
}


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHKEY(HKEY hkey)
{
   /* Any value is a valid HKEY. */

   return(TRUE);
}


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHMODULE(HMODULE hmod)
{
   /* Any non-NULL value is a valid HMODULE. */
   return(hmod != NULL);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidShowWindowCmd(int nShow)
{
   BOOL bResult;

   switch (nShow)
   {
      case SW_HIDE:
      case SW_SHOWNORMAL:
      case SW_SHOWMINIMIZED:
      case SW_SHOWMAXIMIZED:
      case SW_SHOWNOACTIVATE:
      case SW_SHOW:
      case SW_MINIMIZE:
      case SW_SHOWMINNOACTIVE:
      case SW_SHOWNA:
      case SW_RESTORE:
      case SW_SHOWDEFAULT:
         bResult = TRUE;
         break;
      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidShowWindowCmd(): Invalid file creation mode %d."), nShow));
         break;
   }

   return(bResult);
}

#endif