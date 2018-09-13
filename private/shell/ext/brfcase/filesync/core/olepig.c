/*
 * olepig.c - Module for indirect calling of OLE32.DLL functions.
 */


/*

   This sucks.  OLE32.DLL should be redesigned and reimplemented so that it can
be dynalinked to like a well-behaved DLL.  OLE32.DLL is currently so slow and
piggy that we are forced to delay loading it until absolutely necessary.

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include <ole2ver.h>


/* Constants
 ************/

#define OLE_PIG_MODULE              TEXT("ole32.dll")


/* Types
 ********/

/* OLE APIs */

typedef struct _olevtbl
{
   DWORD   (STDAPICALLTYPE *CoBuildVersion)(void);
   HRESULT (STDAPICALLTYPE *CoCreateInstance)(REFCLSID, PIUnknown, DWORD, REFIID, PVOID *);
   HRESULT (STDAPICALLTYPE *CoGetMalloc)(DWORD, PIMalloc *);
   HRESULT (STDAPICALLTYPE *CreateBindCtx)(DWORD, PIBindCtx *);
   HRESULT (STDAPICALLTYPE *CreateFileMoniker)(LPCOLESTR, PIMoniker *);
   HRESULT (STDAPICALLTYPE *OleInitialize)(PIMalloc);
   HRESULT (STDAPICALLTYPE *StgOpenStorage)(const OLECHAR *, PIStorage, DWORD, SNB, DWORD, PIStorage *);
}
OLEVTBL;
DECLARE_STANDARD_TYPES(OLEVTBL);


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* OLE module handle */

PRIVATE_DATA HANDLE MhmodOLE = NULL;

/* pointer to vtable of OLE functions */

PRIVATE_DATA POLEVTBL Mpolevtbl = NULL;

/* TLS slot used to store OLE thread initialization state */

PRIVATE_DATA DWORD MdwOLEInitSlot = TLS_OUT_OF_INDEXES;

#pragma data_seg()


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL IsOLELoaded(void);
PRIVATE_CODE BOOL LoadOLE(void);
PRIVATE_CODE void UnloadOLE(void);
PRIVATE_CODE BOOL InitializeOLE(void);
PRIVATE_CODE BOOL GetOLEProc(LPSTR, PROC *);
PRIVATE_CODE BOOL FillOLEVTable(void);

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCOLEVTBL(PCOLEVTBL);
PRIVATE_CODE BOOL OLELoadedStateOK(void);
PRIVATE_CODE BOOL OLENotLoadedStateOK(void);
PRIVATE_CODE BOOL OLEStateOk(void);

#endif


/*
** IsOLELoaded()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsOLELoaded(void)
{
   ASSERT(OLEStateOk());

   return(MhmodOLE != NULL);
}


/*
** LoadOLE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL LoadOLE(void)
{
   BOOL bResult;

   if (IsOLELoaded())
      bResult = TRUE;
   else
   {
      bResult = FALSE;

      MhmodOLE = LoadLibrary(OLE_PIG_MODULE);

      if (MhmodOLE)
      {
         if (FillOLEVTable())
         {
            DWORD dwBuildVersion;

            dwBuildVersion = Mpolevtbl->CoBuildVersion();

            /* Require same major version and same or newer minor version. */

            if (HIWORD(dwBuildVersion) == rmm &&
                LOWORD(dwBuildVersion) >= rup)
            {
               bResult = TRUE;

               TRACE_OUT((TEXT("LoadOLE(): %s loaded.  Oink oink!"),
                          OLE_PIG_MODULE));
            }
            else
               WARNING_OUT((TEXT("LoadOLE(): Bad %s version %u.%u.  This module was built with %s version %u.%u."),
                            OLE_PIG_MODULE,
                            (UINT)HIWORD(dwBuildVersion),
                            (UINT)LOWORD(dwBuildVersion),
                            OLE_PIG_MODULE,
                            (UINT)rmm,
                            (UINT)rup));
         }
         else
            WARNING_OUT((TEXT("LoadOLE(): FillOLEVTable() failed.")));
      }
      else
         WARNING_OUT((TEXT("LoadOLE(): LoadLibrary(%s) failed."),
                      OLE_PIG_MODULE));

      if (! bResult)
         UnloadOLE();
   }

   if (bResult)
   {
      bResult = InitializeOLE();

      if (! bResult)
         WARNING_OUT((TEXT("LoadOLE(): %s loaded, but InitializeOLE() failed."),
                      OLE_PIG_MODULE));
   }

   ASSERT(OLEStateOk());

   return(bResult);
}


/*
** UnloadOLE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void UnloadOLE(void)
{
   if (Mpolevtbl)
   {
      FreeMemory(Mpolevtbl);
      Mpolevtbl = NULL;

      TRACE_OUT((TEXT("UnloadOLE(): Freed %s vtable."),
                 OLE_PIG_MODULE));
   }

   if (MhmodOLE)
   {
      /* Don't call CoUninitialize() here.  OLE32.DLL will. */

      FreeLibrary(MhmodOLE);
      MhmodOLE = NULL;

      TRACE_OUT((TEXT("UnloadOLE(): Freed %s."),
                 OLE_PIG_MODULE));
   }

   ASSERT(OLENotLoadedStateOK());

   return;
}


/*
** InitializeOLE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL InitializeOLE(void)
{
   BOOL bResult;

   ASSERT(IsOLELoaded());
   ASSERT(MdwOLEInitSlot != TLS_OUT_OF_INDEXES);

   if (TlsGetValue(MdwOLEInitSlot))
      bResult = TRUE;
   else
   {
      HRESULT hr;

      hr = Mpolevtbl->OleInitialize(NULL);

      bResult = (SUCCEEDED(hr) ||
                 hr == CO_E_ALREADYINITIALIZED);

      if (hr == CO_E_ALREADYINITIALIZED)
         WARNING_OUT((TEXT("InitializeOLE(): OLE already initialized for thread %lx.  OleInitialize() returned %s."),
                      GetCurrentThreadId(),
                      GetHRESULTString(hr)));

      if (bResult)
      {
         EVAL(TlsSetValue(MdwOLEInitSlot, (PVOID)TRUE));

         TRACE_OUT((TEXT("InitializeOLE(): OLE initialized for thread %lx.  Using apartment threading model."),
                    GetCurrentThreadId()));
      }
      else
         WARNING_OUT((TEXT("InitializeOLE(): OleInitialize() failed for thread %lx, returning %s."),
                      GetCurrentThreadId(),
                      GetHRESULTString(hr)));
   }

   return(bResult);
}


/*
** GetOLEProc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetOLEProc(LPSTR pcszProc, PROC *pfp)
{
   //ASSERT(IS_VALID_STRING_PTR(pcszProc, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pfp, PROC));

   ASSERT(IS_VALID_HANDLE(MhmodOLE, MODULE));

   *pfp = GetProcAddress(MhmodOLE, pcszProc);

   if (*pfp)
      TRACE_OUT((TEXT("GetOLEProc(): Got address of %s!%s."),
                 OLE_PIG_MODULE,
                 pcszProc));
   else
      WARNING_OUT((TEXT("GetOLEProc(): Failed to get address of %s!%s."),
                   OLE_PIG_MODULE,
                   pcszProc));

   ASSERT(! *pfp ||
          IS_VALID_CODE_PTR(*pfp, PROC));

   return(*pfp != NULL);
}


/*
** FillOLEVTable()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL FillOLEVTable(void)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(MhmodOLE, MODULE));

   bResult = AllocateMemory(sizeof(*Mpolevtbl), &Mpolevtbl);

   if (bResult)
   {
      bResult = (GetOLEProc("CoBuildVersion",      &(PROC)(Mpolevtbl->CoBuildVersion)) &&
                 GetOLEProc("CoCreateInstance",    &(PROC)(Mpolevtbl->CoCreateInstance)) &&
                 GetOLEProc("CoGetMalloc",         &(PROC)(Mpolevtbl->CoGetMalloc)) &&
                 GetOLEProc("CreateBindCtx",       &(PROC)(Mpolevtbl->CreateBindCtx)) &&
                 GetOLEProc("CreateFileMoniker",   &(PROC)(Mpolevtbl->CreateFileMoniker)) &&
                 GetOLEProc("OleInitialize",       &(PROC)(Mpolevtbl->OleInitialize)) &&
                 GetOLEProc("StgOpenStorage",      &(PROC)(Mpolevtbl->StgOpenStorage)));


      if (bResult)
         TRACE_OUT((TEXT("FillOLEVTable(): OLE vtable filled successfully.")));
      else
      {
         FreeMemory(Mpolevtbl);
         Mpolevtbl = NULL;

         WARNING_OUT((TEXT("FillOLEVTable(): Failed to fill OLE vtable.")));
      }
   }
   else
      WARNING_OUT((TEXT("FillOLEVTable(): Out of memory.")));

   ASSERT(! bResult ||
          OLELoadedStateOK());

   return(bResult);
}


#ifdef DEBUG

/*
** IsValidPCOLEVTBL()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCOLEVTBL(PCOLEVTBL pcolevtbl)
{
   return(IS_VALID_READ_PTR(pcolevtbl, PCOLEVTBL) &&
          IS_VALID_CODE_PTR(pcolevtbl->CoBuildVersion, CoBuildVersion) &&
          IS_VALID_CODE_PTR(pcolevtbl->CoCreateInstance, CoCreateInstance) &&
          IS_VALID_CODE_PTR(pcolevtbl->CoGetMalloc, CoGetMalloc) &&
          IS_VALID_CODE_PTR(pcolevtbl->CreateBindCtx, CreateBindCtx) &&
          IS_VALID_CODE_PTR(pcolevtbl->CreateFileMoniker, CreateFileMoniker) &&
          IS_VALID_CODE_PTR(pcolevtbl->OleInitialize, OleInitialize) &&
          IS_VALID_CODE_PTR(pcolevtbl->StgOpenStorage, StgOpenStorage));
}


/*
** OLELoadedStateOK()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL OLELoadedStateOK(void)
{
   return(IS_VALID_HANDLE(MhmodOLE, MODULE) &&
          IS_VALID_STRUCT_PTR(Mpolevtbl, COLEVTBL));
}


/*
** OLENotLoadedStateOK()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL OLENotLoadedStateOK(void)
{
   return(! MhmodOLE &&
          ! Mpolevtbl);
}


/*
** OLEStateOk()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL OLEStateOk(void)
{
   return(OLELoadedStateOK() ||
          OLENotLoadedStateOK);
}

#endif


/****************************** Public Functions *****************************/


/*
** ProcessInitOLEPigModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ProcessInitOLEPigModule(void)
{
   BOOL bResult;

   ASSERT(MdwOLEInitSlot == TLS_OUT_OF_INDEXES);

   MdwOLEInitSlot = TlsAlloc();

   bResult = (MdwOLEInitSlot != TLS_OUT_OF_INDEXES);

   if (bResult)
   {
      EVAL(TlsSetValue(MdwOLEInitSlot, (PVOID)FALSE));

      TRACE_OUT((TEXT("ProcessInitOLEPigModule(): Using thread local storage slot %lu for OLE initialization state."),
                 MdwOLEInitSlot));
   }
   else
      ERROR_OUT((TEXT("ProcessInitOLEPigModule(): TlsAlloc() failed to allocate thread local storage for OLE initialization state.")));

   return(bResult);
}


/*
** ProcessExitOLEPigModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ProcessExitOLEPigModule(void)
{
   UnloadOLE();

   if (MdwOLEInitSlot != TLS_OUT_OF_INDEXES)
   {
      EVAL(TlsFree(MdwOLEInitSlot));
      MdwOLEInitSlot= TLS_OUT_OF_INDEXES;
   }

   return;
}


/*
** CoCreateInstance()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
HRESULT STDAPICALLTYPE CoCreateInstance(REFCLSID rclsid, PIUnknown piunkOuter,
                                        DWORD dwClsCtx, REFIID riid,
                                        PVOID *ppv)
{
   HRESULT hr;

   if (LoadOLE())
      hr = Mpolevtbl->CoCreateInstance(rclsid, piunkOuter, dwClsCtx, riid, ppv);
   else
      hr = E_FAIL;

   return(hr);
}


/*
** CoGetMalloc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
HRESULT STDAPICALLTYPE CoGetMalloc(DWORD dwMemContext, PIMalloc *ppimalloc)
{
   HRESULT hr;

   if (LoadOLE())
      hr = Mpolevtbl->CoGetMalloc(dwMemContext, ppimalloc);
   else
      hr = E_FAIL;

   return(hr);
}


/*
** CreateBindCtx()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
HRESULT STDAPICALLTYPE CreateBindCtx(DWORD dwReserved, PIBindCtx *ppibindctx)
{
   HRESULT hr;

   if (LoadOLE())
      hr = Mpolevtbl->CreateBindCtx(dwReserved, ppibindctx);
   else
      hr = E_FAIL;

   return(hr);
}


/*
** CreateFileMoniker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
HRESULT STDAPICALLTYPE CreateFileMoniker(LPCOLESTR pwszPath, PIMoniker *ppimk)
{
   HRESULT hr;

   if (LoadOLE())
      hr = Mpolevtbl->CreateFileMoniker(pwszPath, ppimk);
   else
      hr = E_FAIL;

   return(hr);
}


/*
** StgOpenStorage()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
HRESULT STDAPICALLTYPE StgOpenStorage(LPCOLESTR pwszName,
                                      PIStorage pistgPriority, DWORD dwMode,
                                      SNB snbExclude, DWORD dwReserved,
                                      PIStorage *ppistgOpen)
{
   HRESULT hr;

   if (LoadOLE())
      hr = Mpolevtbl->StgOpenStorage(pwszName, pistgPriority, dwMode,
                                     snbExclude, dwReserved, ppistgOpen);
   else
      hr = E_FAIL;

   return(hr);
}

