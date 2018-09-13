/*
 * clsiface.c - Class interface cache ADT module.
 */


/*



*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "oleutil.h"


/* Constants
 ************/

/* class interface cache pointer array allocation parameters */

#define NUM_START_CLS_IFACES           (0)
#define NUM_CLS_IFACES_TO_ADD          (16)


/* Types
 ********/

/* class interface cache */

typedef struct _clsifacecache
{
   HPTRARRAY hpa;
}
CLSIFACECACHE;
DECLARE_STANDARD_TYPES(CLSIFACECACHE);

/* class interface */

typedef struct _clsiface
{
   /* class ID */

   PCCLSID pcclsid;

   /* interface ID */

   PCIID pciid;

   /* interface */

   PVOID pvInterface;
}
CLSIFACE;
DECLARE_STANDARD_TYPES(CLSIFACE);

/* class interface search structure for ClassInterfaceSearchCmp() */

typedef struct _clsifacesearchinfo
{
   /* class ID */

   PCCLSID pcclsid;

   /* interface ID */

   PCIID pciid;
}
CLSIFACESEARCHINFO;
DECLARE_STANDARD_TYPES(CLSIFACESEARCHINFO);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL CreateClassInterfacePtrArray(PHPTRARRAY);
PRIVATE_CODE void DestroyClassInterfacePtrArray(HPTRARRAY);
PRIVATE_CODE HRESULT CreateClassInterface(PCCLSIFACECACHE, PCCLSID, PCIID, PCLSIFACE *);
PRIVATE_CODE void DestroyClassInterface(PCLSIFACE);
PRIVATE_CODE COMPARISONRESULT ClassInterfaceSortCmp(PCVOID, PCVOID);
PRIVATE_CODE COMPARISONRESULT ClassInterfaceSearchCmp(PCVOID, PCVOID);

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCCLSIFACECACHE(PCCLSIFACECACHE);
PRIVATE_CODE BOOL IsValidPCCLSIFACE(PCCLSIFACE);
PRIVATE_CODE BOOL IsValidPCCLSIFACESEARCHINFO(PCCLSIFACESEARCHINFO);

#endif


/*
** CreateClassInterfacePtrArray()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateClassInterfacePtrArray(PHPTRARRAY phpa)
{
   NEWPTRARRAY npa;

   ASSERT(IS_VALID_WRITE_PTR(phpa, HPTRARRAY));

   npa.aicInitialPtrs = NUM_START_CLS_IFACES;
   npa.aicAllocGranularity = NUM_CLS_IFACES_TO_ADD;
   npa.dwFlags = NPA_FL_SORTED_ADD;

   return(CreatePtrArray(&npa, phpa));
}


/*
** DestroyClassInterfacePtrArray()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyClassInterfacePtrArray(HPTRARRAY hpa)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* First free all class interfaces in array. */

   aicPtrs = GetPtrCount(hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyClassInterface(GetPtr(hpa, ai));

   /* Now wipe out the array. */

   DestroyPtrArray(hpa);

   return;
}


/*
** CreateClassInterface()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT CreateClassInterface(PCCLSIFACECACHE pccic,
                                          PCCLSID pcclsid, PCIID pciid,
                                          PCLSIFACE *ppci)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRUCT_PTR(pccic, CCLSIFACECACHE));
   ASSERT(IS_VALID_STRUCT_PTR(pcclsid, CCLSID));
   ASSERT(IS_VALID_STRUCT_PTR(pciid, CIID));
   ASSERT(IS_VALID_WRITE_PTR(ppci, PCLSIFACE));

   if (AllocateMemory(sizeof(**ppci), ppci))
   {
      /* Use inproc servers and local servers. */

      hr = CoCreateInstance(pcclsid, NULL, CLSCTX_SERVER, pciid,
                            &((*ppci)->pvInterface));

      if (SUCCEEDED(hr))
      {
         ARRAYINDEX ai;

         (*ppci)->pcclsid = pcclsid;
         (*ppci)->pciid = pciid;

         if (! AddPtr(pccic->hpa, ClassInterfaceSortCmp, *ppci, &ai))
         {
            hr = E_OUTOFMEMORY;
CREATECLASSINTERFACE_BAIL:
            FreeMemory(*ppci);
         }
      }
      else
      {
         WARNING_OUT((TEXT("CreateClassInterface(): CoCreateInstance() failed, returning %s."),
                      GetHRESULTString(hr)));

         goto CREATECLASSINTERFACE_BAIL;
      }
   }
   else
      hr = E_OUTOFMEMORY;

   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(*ppci, CCLSIFACE));

   return(hr);
}


/*
** DestroyClassInterface()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyClassInterface(PCLSIFACE pci)
{
   ASSERT(IS_VALID_STRUCT_PTR(pci, CCLSIFACE));

   ((PCIUnknown)(pci->pvInterface))->lpVtbl->Release(pci->pvInterface);
   FreeMemory(pci);

   return;
}


/*
** ClassInterfaceSortCmp()
**
** Pointer comparison function used to sort an array of pointers to class
** interfaces.
**
** Arguments:     pcci1 - pointer to first class interface
**                pcci2 - pointer to second class interface
**
** Returns:
**
** Side Effects:  none
**
** The class interfaces are sorted by:
**    1) CLSID
**    2) IID
*/
PRIVATE_CODE COMPARISONRESULT ClassInterfaceSortCmp(PCVOID pcci1, PCVOID pcci2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcci1, CCLSIFACE));
   ASSERT(IS_VALID_STRUCT_PTR(pcci2, CCLSIFACE));

   cr = CompareClassIDs(((PCCLSIFACE)pcci1)->pcclsid,
                        ((PCCLSIFACE)pcci2)->pcclsid);

   if (cr == CR_EQUAL)
      cr = CompareInterfaceIDs(((PCCLSIFACE)pcci1)->pciid,
                               ((PCCLSIFACE)pcci2)->pciid);

   return(cr);
}


/*
** ClassInterfaceSearchCmp()
**
** Pointer comparison function used to search an array of pointers to class
** interfaces.
**
** Arguments:     pccisi - pointer to class interface search information
**                pcci - pointer to class interface to examine
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT ClassInterfaceSearchCmp(PCVOID pccisi,
                                                      PCVOID pcci)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pccisi, CCLSIFACESEARCHINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcci, CCLSIFACE));

   cr = CompareClassIDs(((PCCLSIFACESEARCHINFO)pccisi)->pcclsid,
                        ((PCCLSIFACE)pcci)->pcclsid);

   if (cr == CR_EQUAL)
      cr = CompareInterfaceIDs(((PCCLSIFACESEARCHINFO)pccisi)->pciid,
                               ((PCCLSIFACE)pcci)->pciid);

   return(cr);
}


#ifdef DEBUG

/*
** IsValidPCCLSIFACECACHE()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCCLSIFACECACHE(PCCLSIFACECACHE pccic)
{
   return(IS_VALID_READ_PTR(pccic, CLSIFACECACHE) &&
          IS_VALID_HANDLE(pccic->hpa, PTRARRAY));
}


/*
** IsValidPCCLSIFACE()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCCLSIFACE(PCCLSIFACE pcci)
{
   return(IS_VALID_READ_PTR(pcci, CCLSIFACE) &&
          IS_VALID_STRUCT_PTR(pcci->pcclsid, CCLSID) &&
          IS_VALID_STRUCT_PTR(pcci->pciid, CIID) &&
          IS_VALID_STRUCT_PTR(pcci->pvInterface, CInterface));
}


/*
** IsValidPCCLSIFACESEARCHINFO()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCCLSIFACESEARCHINFO(PCCLSIFACESEARCHINFO pccisi)
{
   return(IS_VALID_READ_PTR(pccisi, CCLSIFACESEARCHINFO) &&
          IS_VALID_STRUCT_PTR(pccisi->pcclsid, CCLSID) &&
          IS_VALID_STRUCT_PTR(pccisi->pciid, CIID));
}

#endif


/****************************** Public Functions *****************************/


/*
** CreateClassInterfaceCache()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateClassInterfaceCache(PHCLSIFACECACHE phcic)
{
   BOOL bResult = FALSE;
   PCLSIFACECACHE pcic;

   ASSERT(IS_VALID_WRITE_PTR(phcic, HCLSIFACECACHE));

   if (AllocateMemory(sizeof(*pcic), &pcic))
   {
      if (CreateClassInterfacePtrArray(&(pcic->hpa)))
      {
         *phcic = (HCLSIFACECACHE)pcic;
         bResult = TRUE;
      }
      else
         FreeMemory(pcic);
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phcic, CLSIFACECACHE));

   return(bResult);
}


/*
** DestroyClassInterfaceCache()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyClassInterfaceCache(HCLSIFACECACHE hcic)
{
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));

   DestroyClassInterfacePtrArray(((PCLSIFACECACHE)hcic)->hpa);
   FreeMemory(hcic);

   return;
}


/*
** GetClassInterface()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** N.b., this function assumes that pcclsid and pciid are valid until hcic is
** destroyed with DestroyClassInterfaceCache().
*/
PUBLIC_CODE HRESULT GetClassInterface(HCLSIFACECACHE hcic, PCCLSID pcclsid,
                                      PCIID pciid, PVOID *ppvInterface)
{
   HRESULT hr;
   CLSIFACESEARCHINFO cisi;
   ARRAYINDEX ai;
   PCLSIFACE pci;

   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));

   /* Is this class interface already in the cache? */

   cisi.pcclsid = pcclsid;
   cisi.pciid = pciid;

   if (SearchSortedArray(((PCCLSIFACECACHE)hcic)->hpa,
                         &ClassInterfaceSearchCmp, &cisi, &ai))
   {
      /* Yes.  Use it. */

      pci = GetPtr(((PCCLSIFACECACHE)hcic)->hpa, ai);

      hr = S_OK;
   }
   else
      /* No.  Add it. */
      hr = CreateClassInterface((PCCLSIFACECACHE)hcic, pcclsid, pciid, &pci);

   if (SUCCEEDED(hr))
   {
      ASSERT(IS_VALID_STRUCT_PTR(pci, CCLSIFACE));

      *ppvInterface = pci->pvInterface;
   }

   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(*ppvInterface, CInterface));

   return(hr);
}


#ifdef DEBUG

/*
** IsValidHCLSIFACECACHE()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHCLSIFACECACHE(HCLSIFACECACHE hcic)
{
   return(IS_VALID_STRUCT_PTR((PCCLSIFACECACHE)hcic, CCLSIFACECACHE));
}

#endif

