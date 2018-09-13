/*
 * server.c - Server vtable functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "server.h"


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/*
 * Assume that we don't need to serialize access to MhinstServerDLL and Msvt
 * since they are only modified during first PROCESS_ATTACH.  Access to shared
 * data is protected during AttachProcess().
 */

PRIVATE_DATA HINSTANCE MhinstServerDLL = NULL;

PRIVATE_DATA SERVERVTABLE Msvt =
{
   NULL,
   NULL
};

#pragma data_seg()


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCSERVERVTABLE(PCSERVERVTABLE);

#endif


#ifdef DEBUG

/*
** IsValidPCSERVERVTABLE()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCSERVERVTABLE(PCSERVERVTABLE pcsvt)
{
   return(IS_VALID_READ_PTR(pcsvt, CSERVERVTABLE) &&
          IS_VALID_CODE_PTR(pcsvt->GetNetResourceFromLocalPath, PFNGETNETRESOURCEFROMLOCALPATH) &&
          IS_VALID_CODE_PTR(pcsvt->GetLocalPathFromNetResource, PFNGETLOCALPATHFROMNETRESOURCE));
}

#endif


/****************************** Public Functions *****************************/


/*
** ProcessInitServerModule()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ProcessInitServerModule(void)
{
   TCHAR rgchDLLPath[MAX_PATH_LEN];
   LONG lcb;

   /* Load server DLL. */

   lcb = SIZEOF(rgchDLLPath);

   if (RegQueryValue(HKEY_CLASSES_ROOT, TEXT("Network\\SharingHandler"), rgchDLLPath,
                     &lcb) == ERROR_SUCCESS)
   {
      if (rgchDLLPath[0])
      {
         HINSTANCE hinst;

         hinst = LoadLibrary(rgchDLLPath);

         if (hinst)
         {
            PFNGETNETRESOURCEFROMLOCALPATH GetNetResourceFromLocalPath;
            PFNGETLOCALPATHFROMNETRESOURCE GetLocalPathFromNetResource;

#ifdef UNICODE
            GetNetResourceFromLocalPath = (PFNGETNETRESOURCEFROMLOCALPATH)
					GetProcAddress(hinst, "GetNetResourceFromLocalPathW");
            GetLocalPathFromNetResource = (PFNGETLOCALPATHFROMNETRESOURCE)
					GetProcAddress(hinst, "GetLocalPathFromNetResourceW");
#else // UNICODE
#ifdef WINNT
            GetNetResourceFromLocalPath = (PFNGETNETRESOURCEFROMLOCALPATH)
					GetProcAddress(hinst, "GetNetResourceFromLocalPathA");
            GetLocalPathFromNetResource = (PFNGETLOCALPATHFROMNETRESOURCE)
					GetProcAddress(hinst, "GetLocalPathFromNetResourceA");
#else // WINNT
            GetNetResourceFromLocalPath = (PFNGETNETRESOURCEFROMLOCALPATH)
					GetProcAddress(hinst, "GetNetResourceFromLocalPath");
            GetLocalPathFromNetResource = (PFNGETLOCALPATHFROMNETRESOURCE)
					GetProcAddress(hinst, "GetLocalPathFromNetResource");
#endif // WINNT
#endif // UNICODE

            if (GetNetResourceFromLocalPath && GetLocalPathFromNetResource)
            {
               ASSERT(AccessIsExclusive());

               Msvt.GetNetResourceFromLocalPath = GetNetResourceFromLocalPath;
               Msvt.GetLocalPathFromNetResource = GetLocalPathFromNetResource;

               MhinstServerDLL = hinst;

               ASSERT(IS_VALID_STRUCT_PTR((PCSERVERVTABLE)&Msvt, CSERVERVTABLE));
               ASSERT(IS_VALID_HANDLE(MhinstServerDLL, INSTANCE));

               TRACE_OUT((TEXT("ProcessInitServerModule(): Loaded sharing handler DLL %s."),
                          rgchDLLPath));
            }
         }
      }
   }

   return(TRUE);
}


/*
** ProcessExitServerModule()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ProcessExitServerModule(void)
{
   /* Unload server DLL. */

   if (MhinstServerDLL)
   {
      ASSERT(IS_VALID_HANDLE(MhinstServerDLL, INSTANCE));
      EVAL(FreeLibrary(MhinstServerDLL));
      MhinstServerDLL = NULL;

      TRACE_OUT((TEXT("ProcessExitServerModule(): Unloaded sharing handler DLL.")));
   }

   return;
}


/*
** GetServerVTable()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetServerVTable(PCSERVERVTABLE *ppcsvt)
{
   BOOL bResult;

   ASSERT(IS_VALID_WRITE_PTR(ppcsvt, PCSERVERVTABLE));

   if (MhinstServerDLL)
   {
      *ppcsvt = &Msvt;

      bResult = TRUE;
   }
   else
      bResult = FALSE;

   ASSERT(! bResult || IS_VALID_STRUCT_PTR(*ppcsvt, CSERVERVTABLE));

   return(bResult);
}
