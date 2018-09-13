/*
 * inetcpl.c - Indirect calls to inetcpl.cpl.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "inetcpl.h"


/* Types
 ********/

/* Internet CPL vtable */

typedef struct internetcplvtbl
{
   HRESULT (WINAPI *AddInternetPropertySheets)(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam, PUINT pucRefCount, LPFNPSPCALLBACK pfnCallback);
}
INTERNETCPLVTBL;
DECLARE_STANDARD_TYPES(INTERNETCPLVTBL);


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA CCHAR s_cszInternetCPL[]               = "inetcpl.cpl";
PRIVATE_DATA CCHAR s_cszAddInternetPropertySheets[] = "AddInternetPropertySheets";

#pragma data_seg()


/* Module Variables
 *******************/

/* Internet CPL reference count */
PRIVATE_DATA UINT s_ucRef = 0;

PRIVATE_DATA HINSTANCE s_hinstInternetCPL    = NULL;

/* Internet CPL vtable */

PRIVATE_DATA PINTERNETCPLVTBL s_pinetcplvtbl = NULL;


/***************************** Private Functions *****************************/


#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCINTERNETCPLVTBL(PCINTERNETCPLVTBL pcinetcplctbl)
{
   return(IS_VALID_READ_PTR(pcinetcplctbl, CINTERNETCPLVTBL) &&
          IS_VALID_CODE_PTR(pcinetcplctbl->AddInternetPropertySheets, AddInternetPropertySheets));
}


PRIVATE_CODE BOOL InternetCPLNotLoadedStateOK(void)
{
   return(! s_hinstInternetCPL &&
          ! s_pinetcplvtbl);
}


PRIVATE_CODE BOOL InternetCPLLoadedStateOK(void)
{
   return(IS_VALID_HANDLE(s_hinstInternetCPL, INSTANCE) &&
          IS_VALID_STRUCT_PTR(s_pinetcplvtbl, CINTERNETCPLVTBL));
}


PRIVATE_CODE BOOL InternetCPLStateOK(void)
{
   return(InternetCPLNotLoadedStateOK() ||
          InternetCPLLoadedStateOK());
}

#endif   /* DEBUG */


PRIVATE_CODE BOOL IsInternetCPLLoaded(void)
{
   ASSERT(InternetCPLStateOK());

   return(s_hinstInternetCPL != NULL);
}


PRIVATE_CODE BOOL GetInternetCPLProc(PCSTR pcszProc, FARPROC *pfp)
{
   ASSERT(IS_VALID_STRING_PTR(pcszProc, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pfp, FARPROC));

   ASSERT(IS_VALID_HANDLE(s_hinstInternetCPL, INSTANCE));

   *pfp = GetProcAddress(s_hinstInternetCPL, pcszProc);

   if (*pfp)
      TRACE_OUT(("GetInternetCPLProc(): Got address of %s!%s.",
                 s_cszInternetCPL,
                 pcszProc));
   else
      WARNING_OUT(("GetOLEProc(): Failed to get address of %s!%s.",
                   s_cszInternetCPL,
                   pcszProc));

   ASSERT(! *pfp ||
          IS_VALID_CODE_PTR(*pfp, FARPROC));

   return(*pfp != NULL);
}


PRIVATE_CODE BOOL FillInternetCPLVTable(void)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(s_hinstInternetCPL, INSTANCE));

   if (AllocateMemory(sizeof(*s_pinetcplvtbl), &s_pinetcplvtbl))
   {
      bResult = GetInternetCPLProc(s_cszAddInternetPropertySheets,
                                   &(FARPROC)(s_pinetcplvtbl->AddInternetPropertySheets));

      if (bResult)
         TRACE_OUT(("FillInternetCPLVTable(): Internet CPL vtable filled successfully."));
      else
      {
         FreeMemory(s_pinetcplvtbl);
         s_pinetcplvtbl = NULL;
      }
   }
   else
      bResult = FALSE;

   ASSERT(! bResult ||
          InternetCPLLoadedStateOK());

   return(bResult);
}


PRIVATE_CODE BOOL LoadInternetCPL(void)
{
   BOOL bResult;

   bResult = IsInternetCPLLoaded();

   if (! bResult)
   {
      s_hinstInternetCPL = LoadLibrary(s_cszInternetCPL);

      if (s_hinstInternetCPL)
      {
         bResult = FillInternetCPLVTable();

         if (bResult)
            TRACE_OUT(("LoadInternetCPL(): %s loaded.",
                       s_cszInternetCPL));
      }
      else
         WARNING_OUT(("LoadInternetCPL(): LoadLibrary(%s) failed.",
                      s_cszInternetCPL));

      if (! bResult)
         UnloadInternetCPL();
   }

   ASSERT(InternetCPLStateOK());

   return(bResult);
}


/****************************** Public Functions *****************************/



PUBLIC_CODE void UnloadInternetCPL(void)
{
    if (s_pinetcplvtbl)
    {
        FreeMemory(s_pinetcplvtbl);
        s_pinetcplvtbl = NULL;

        TRACE_OUT(("UnloadInternetCPL(): Freed %s vtable.",
                   s_cszInternetCPL));
    }

    if (s_hinstInternetCPL)
    {
        FreeLibrary(s_hinstInternetCPL);
        s_hinstInternetCPL = NULL;

        TRACE_OUT(("UnloadInternetCPL(): Freed %s.",
                   s_cszInternetCPL));
    }


    return;
}


HRESULT InternetCPLCanUnloadNow(void)
{
    return ((s_ucRef > 0) ? S_FALSE : S_OK);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */
PUBLIC_CODE HRESULT WINAPI AddInternetPropertySheets(
                                             LPFNADDPROPSHEETPAGE pfnAddPage,
                                             LPARAM lparam, PUINT pucRefCount,
                                             LPFNPSPCALLBACK pfnCallback)
{
    HRESULT hr;

    /*
     * Increment the Internet CPL's reference count around this call it so that
     * it is not unloaded during the call.
     */

    if (LoadInternetCPL())
        hr = s_pinetcplvtbl->AddInternetPropertySheets(pfnAddPage, lparam,
                                                       &s_ucRef,
                                                       pfnCallback);
    else
        hr = E_FAIL;

    return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */
