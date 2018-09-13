/*
 * storage.c - Storage ADT module.
 */

/*

   The HSTGIFACE ADT is provided to insulate the caller from the details of
which storage interface is used for serialization, and how it is called.
Storage interfaces are tried in the following order:

1) IPersistFile
2) IPersistStorage

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Constants
 ************/

/* flags specified when opening a storage */

#define STORAGE_OPEN_MODE_FLAGS        (STGM_TRANSACTED |\
                                        STGM_READWRITE |\
                                        STGM_SHARE_EXCLUSIVE)


/* Macros
 *********/

/* access STGIFACE fields */

#define STGI_TYPE(pstgi)               ((pstgi)->stgit)
#define STGI_IPERSISTFILE(pstgi)       ((pstgi)->stgi.pipfile)
#define STGI_ISTORAGE(pstgi)           ((pstgi)->stgi.stg.pistg)
#define STGI_IPERSISTSTORAGE(pstgi)    ((pstgi)->stgi.stg.pipstg)


/* Types
 ********/

/* storage interface types */

typedef enum _storageinterfacetype
{
   STGIT_IPERSISTFILE,

   STGIT_IPERSISTSTORAGE
}
STGIFACETYPE;

/* storage interface structure */

typedef struct _storageinterface
{
   STGIFACETYPE stgit;

   union
   {
      PIPersistFile pipfile;

      struct
      {
         PIStorage pistg;

         PIPersistStorage pipstg;
      } stg;
   } stgi;
}
STGIFACE;
DECLARE_STANDARD_TYPES(STGIFACE);


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* memory manager interface */

PRIVATE_DATA PIMalloc Mpimalloc = NULL;

#pragma data_seg()


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCSTGIFACE(PCSTGIFACE);

#endif


#ifdef DEBUG

/*
** IsValidPCSTGIFACE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCSTGIFACE(PCSTGIFACE pcstgi)
{
   BOOL bResult = FALSE;

   if (IS_VALID_READ_PTR(pcstgi, CSTGIFACE))
   {
      switch (STGI_TYPE(pcstgi))
      {
         case STGIT_IPERSISTSTORAGE:
            bResult = ((! STGI_ISTORAGE(pcstgi) ||
                        IS_VALID_STRUCT_PTR(STGI_ISTORAGE(pcstgi), CIStorage)) &&
                       IS_VALID_STRUCT_PTR(STGI_IPERSISTSTORAGE(pcstgi), CIPersistStorage));
            break;

         default:
            ASSERT(STGI_TYPE(pcstgi) == STGIT_IPERSISTFILE);
            bResult = IS_VALID_STRUCT_PTR(STGI_IPERSISTFILE(pcstgi), CIPersistFile);
            break;
      }
   }

   return(bResult);
}

#endif


/****************************** Public Functions *****************************/


/*
** ProcessInitStorageModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ProcessInitStorageModule(void)
{
   return(TRUE);
}


/*
** ProcessExitStorageModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ProcessExitStorageModule(void)
{
   ASSERT(! Mpimalloc ||
          IS_VALID_STRUCT_PTR(Mpimalloc, CIMalloc));

   if (Mpimalloc)
   {
      Mpimalloc->lpVtbl->Release(Mpimalloc);
      Mpimalloc = NULL;

      TRACE_OUT((TEXT("ProcessExitStorageModule(): Released IMalloc.")));
   }

   return;
}


/*
** GetStorageInterface()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HRESULT GetStorageInterface(PIUnknown piunk, PHSTGIFACE phstgi)
{
   HRESULT hr;
   PSTGIFACE pstgi;

   ASSERT(IS_VALID_STRUCT_PTR(piunk, CIUnknown));
   ASSERT(IS_VALID_WRITE_PTR(phstgi, HSTGIFACE));

   if (AllocateMemory(sizeof(*pstgi), &pstgi))
   {
      PVOID pvInterface;

      /* Ask for a storage interface. */

      hr = piunk->lpVtbl->QueryInterface(piunk, &IID_IPersistFile,
                                         &pvInterface);

      if (SUCCEEDED(hr))
      {
         /* Use IPersistFile. */

         STGI_TYPE(pstgi) = STGIT_IPERSISTFILE;
         STGI_IPERSISTFILE(pstgi) = pvInterface;
      }
      else
      {
         hr = piunk->lpVtbl->QueryInterface(piunk, &IID_IPersistStorage,
                                            &pvInterface);

         if (SUCCEEDED(hr))
         {
            /* Use IPersistStorage. */

            STGI_TYPE(pstgi) = STGIT_IPERSISTSTORAGE;
            STGI_ISTORAGE(pstgi) = NULL;
            STGI_IPERSISTSTORAGE(pstgi) = pvInterface;
         }
         else
            FreeMemory(pstgi);
      }

      if (SUCCEEDED(hr))
         *phstgi = (HSTGIFACE)pstgi;
   }
   else
      hr = E_OUTOFMEMORY;

   ASSERT(FAILED(hr) ||
          IS_VALID_HANDLE(*phstgi, STGIFACE));

   return(hr);
}


/*
** ReleaseStorageInterface()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ReleaseStorageInterface(HSTGIFACE hstgi)
{
   PCSTGIFACE pcstgi;

   ASSERT(IS_VALID_HANDLE(hstgi, STGIFACE));

   pcstgi = (PCSTGIFACE)hstgi;

   switch (STGI_TYPE(pcstgi))
   {
      case STGIT_IPERSISTSTORAGE:
         STGI_IPERSISTSTORAGE(pcstgi)->lpVtbl->Release(STGI_IPERSISTSTORAGE(pcstgi));
         if (STGI_ISTORAGE(pcstgi))
            STGI_ISTORAGE(pcstgi)->lpVtbl->Release(STGI_ISTORAGE(pcstgi));
         break;

      default:
         ASSERT(STGI_TYPE(pcstgi) == STGIT_IPERSISTFILE);
         STGI_IPERSISTFILE(pcstgi)->lpVtbl->Release(STGI_IPERSISTFILE(pcstgi));
         break;
   }

   FreeMemory(hstgi);

   return;
}


/*
** LoadFromStorage()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HRESULT LoadFromStorage(HSTGIFACE hstgi, LPCTSTR pcszPath)
{
   HRESULT hr = S_OK;
   WCHAR rgwchUnicodePath[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hstgi, STGIFACE));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

#ifdef UNICODE
   
   // REVIEW useless strcpy

   lstrcpy(rgwchUnicodePath, pcszPath);
   
#else

   /* Translate ANSI string into Unicode for OLE. */

   if (MultiByteToWideChar(CP_ACP, 0, pcszPath, -1, rgwchUnicodePath,
                           ARRAY_ELEMENTS(rgwchUnicodePath)))
   {
       hr = MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
   }

#endif

   if (S_OK == hr)
   {
      PSTGIFACE pstgi;

      pstgi = (PSTGIFACE)hstgi;

      switch (STGI_TYPE(pstgi))
      {
         case STGIT_IPERSISTSTORAGE:
         {
            PIStorage pistg;

            hr = StgOpenStorage(rgwchUnicodePath, NULL,
                                STORAGE_OPEN_MODE_FLAGS, NULL, 0, &pistg);

            if (SUCCEEDED(hr))
            {
               hr = STGI_IPERSISTSTORAGE(pstgi)->lpVtbl->Load(STGI_IPERSISTSTORAGE(pstgi),
                                                              pistg);

               if (SUCCEEDED(hr))
                  STGI_ISTORAGE(pstgi) = pistg;
               else
                  pistg->lpVtbl->Release(pistg);
            }
            else
               WARNING_OUT((TEXT("LoadFromStorage(): StgOpenStorage() on %s failed, returning %s."),
                            pcszPath,
                            GetHRESULTString(hr)));

            break;
         }

         default:
            ASSERT(STGI_TYPE(pstgi) == STGIT_IPERSISTFILE);
            hr = STGI_IPERSISTFILE(pstgi)->lpVtbl->Load(STGI_IPERSISTFILE(pstgi),
                                                        rgwchUnicodePath,
                                                        STORAGE_OPEN_MODE_FLAGS);
            break;
      }
   }
   
   return(hr);
}


/*
** SaveToStorage()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HRESULT SaveToStorage(HSTGIFACE hstgi)
{
   HRESULT hr;
   PCSTGIFACE pcstgi;

   ASSERT(IS_VALID_HANDLE(hstgi, STGIFACE));

   pcstgi = (PCSTGIFACE)hstgi;

   switch (STGI_TYPE(pcstgi))
   {
      case STGIT_IPERSISTSTORAGE:
         hr = STGI_IPERSISTSTORAGE(pcstgi)->lpVtbl->IsDirty(STGI_IPERSISTSTORAGE(pcstgi));
         if (hr == S_OK)
         {
            hr = STGI_IPERSISTSTORAGE(pcstgi)->lpVtbl->Save(STGI_IPERSISTSTORAGE(pcstgi),
                                                            STGI_ISTORAGE(pcstgi),
                                                            TRUE);

            if (SUCCEEDED(hr))
            {
               HRESULT hrNext;

               HandsOffStorage((HSTGIFACE)pcstgi);

               hr = STGI_ISTORAGE(pcstgi)->lpVtbl->Commit(STGI_ISTORAGE(pcstgi),
                                                          STGC_DEFAULT);

               hrNext = STGI_IPERSISTSTORAGE(pcstgi)->lpVtbl->SaveCompleted(STGI_IPERSISTSTORAGE(pcstgi),
                                                                            NULL);

               if (SUCCEEDED(hr))
                  hr = hrNext;
            }
         }
         break;

      default:
         ASSERT(STGI_TYPE(pcstgi) == STGIT_IPERSISTFILE);
         hr = STGI_IPERSISTFILE(pcstgi)->lpVtbl->IsDirty(STGI_IPERSISTFILE(pcstgi));
         if (hr == S_OK)
         {
            LPOLESTR posPath;

            hr = STGI_IPERSISTFILE(pcstgi)->lpVtbl->GetCurFile(STGI_IPERSISTFILE(pcstgi),
                                                               &posPath);

            if (hr == S_OK)
            {
               PIMalloc pimalloc;

               hr = STGI_IPERSISTFILE(pcstgi)->lpVtbl->Save(STGI_IPERSISTFILE(pcstgi),
                                                            posPath, FALSE);

               if (SUCCEEDED(hr))
                  hr = STGI_IPERSISTFILE(pcstgi)->lpVtbl->SaveCompleted(STGI_IPERSISTFILE(pcstgi),
                                                                        posPath);

               if (EVAL(GetIMalloc(&pimalloc)))
                  pimalloc->lpVtbl->Free(pimalloc, posPath);
                  /* Do not release pimalloc. */
            }
         }
         break;
   }

   return(hr);
}


/*
** HandsOffStorage()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void HandsOffStorage(HSTGIFACE hstgi)
{
   PCSTGIFACE pcstgi;

   ASSERT(IS_VALID_HANDLE(hstgi, STGIFACE));

   pcstgi = (PCSTGIFACE)hstgi;

   switch (STGI_TYPE(pcstgi))
   {
      case STGIT_IPERSISTSTORAGE:
         EVAL(STGI_IPERSISTSTORAGE(pcstgi)->lpVtbl->HandsOffStorage(STGI_IPERSISTSTORAGE(pcstgi))
              == S_OK);
         break;

      default:
         ASSERT(STGI_TYPE(pcstgi) == STGIT_IPERSISTFILE);
         break;
   }

   return;
}


/*
** GetIMalloc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetIMalloc(PIMalloc *ppimalloc)
{
   BOOL bResult;

   ASSERT(IS_VALID_WRITE_PTR(ppimalloc, PIMalloc));

   ASSERT(! Mpimalloc ||
          IS_VALID_STRUCT_PTR(Mpimalloc, CIMalloc));

   if (! Mpimalloc)
   {
      HRESULT hr;

      hr = CoGetMalloc(MEMCTX_TASK, &Mpimalloc);

      if (SUCCEEDED(hr))
         ASSERT(IS_VALID_STRUCT_PTR(Mpimalloc, CIMalloc));
      else
      {
         ASSERT(! Mpimalloc);

         WARNING_OUT((TEXT("GetIMalloc(): CoGetMalloc() failed, returning %s."),
                      GetHRESULTString(hr)));
      }
   }

   if (Mpimalloc)
   {
      *ppimalloc = Mpimalloc;
      bResult = TRUE;
   }
   else
      bResult = FALSE;

   return(bResult);
}


#ifdef DEBUG

/*
** IsValidHSTGIFACE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHSTGIFACE(HSTGIFACE hstgi)
{
   return(IS_VALID_STRUCT_PTR((PCSTGIFACE)hstgi, CSTGIFACE));
}

#endif

