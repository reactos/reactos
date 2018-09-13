/*
 * findbc.c - Briefcase enumeration module.
 */

/*



*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "findbc.h"


/* Macros
 *********/

/* macro for translating an LRESULT to a TWINRESULT */

#define LRESULTToTWINRESULT(lr, TR)    case lr: tr = TR; break


/* Constants
 ************/

/* briefcase registry keys */

#define HKEY_BRIEFCASE_ROOT         HKEY_CURRENT_USER
#ifdef DEBUG
#define HKEY_BRIEFCASE_ROOT_STRING  TEXT("HKEY_CURRENT_USER")
#endif

#define BRIEFCASE_SUBKEY            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Briefcase\\Briefcases")

/* maximum briefcase value name length, including null terminator */

#define MAX_VALUE_NAME_LEN          (8 + 1)


/* Types
 ********/

/* EnumBriefcases() callback function */

typedef LONG (*ENUMBRIEFCASESPROC)(PCLINKINFO, PCVOID, PBOOL);

/* briefcase iterator */

typedef struct _brfcaseiter
{
   HPTRARRAY hpa;

   ARRAYINDEX aiNext;
}
BRFCASEITER;
DECLARE_STANDARD_TYPES(BRFCASEITER);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE COMPARISONRESULT LinkInfoSortCmp(PCVOID, PCVOID);
PRIVATE_CODE COMPARISONRESULT LinkInfoSearchCmp(PCVOID, PCVOID);
PRIVATE_CODE TWINRESULT TranslateLRESULTToTWINRESULT(LONG);
PRIVATE_CODE LONG AllocateValueDataBuffer(HKEY, PVOID *, PDWORD);
PRIVATE_CODE LONG EnumBriefcases(HKEY, ENUMBRIEFCASESPROC, PCVOID, LPTSTR, PBOOL);
PRIVATE_CODE LONG GetUnusedBriefcaseValueName(HKEY, LPTSTR);
PRIVATE_CODE TWINRESULT CreateBriefcaseIterator(PBRFCASEITER *);
PRIVATE_CODE TWINRESULT GetNextBriefcaseIterator(PBRFCASEITER, PBRFCASEINFO);
PRIVATE_CODE void DestroyBriefcaseIterator(PBRFCASEITER);
PRIVATE_CODE LONG AddBriefcaseToIteratorProc(PCLINKINFO, PCVOID, PBOOL);
PRIVATE_CODE LONG CompareLinkInfoProc(PCLINKINFO, PCVOID, PBOOL);
PRIVATE_CODE TWINRESULT MyAddBriefcaseToSystem(PCLINKINFO);
PRIVATE_CODE TWINRESULT MyRemoveBriefcaseFromSystem(PCLINKINFO);
PRIVATE_CODE TWINRESULT UpdateBriefcaseLinkInfo(PCLINKINFO, PCLINKINFO);

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL IsValidPCBRFCASEITER(PCBRFCASEITER);

#endif

#ifdef EXPV

PRIVATE_CODE BOOL IsValidHBRFCASEITER(HBRFCASEITER);

#endif


/*
** LinkInfoSortCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** LinkInfo structures are sorted by:
**    1) LinkInfo referent
**    2) pointer
*/
PRIVATE_CODE COMPARISONRESULT LinkInfoSortCmp(PCVOID pcli1, PCVOID pcli2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcli1, CLINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcli2, CLINKINFO));

   cr = CompareLinkInfoReferents((PCLINKINFO)pcli1, (PCLINKINFO)pcli2);

   if (cr == CR_EQUAL)
      cr = ComparePointers(pcli1, pcli2);

   return(cr);
}


/*
** LinkInfoSearchCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** LinkInfo structures are searched by:
**    1) LinkInfo referent
*/
PRIVATE_CODE COMPARISONRESULT LinkInfoSearchCmp(PCVOID pcliTarget,
                                                PCVOID pcliCurrent)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcliTarget, CLINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcliCurrent, CLINKINFO));

   return(CompareLinkInfoReferents(pcliTarget, pcliCurrent));
}


/*
** TranslateLRESULTToTWINRESULT()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT TranslateLRESULTToTWINRESULT(LONG lResult)
{
   TWINRESULT tr;

   switch (lResult)
   {
      LRESULTToTWINRESULT(ERROR_SUCCESS, TR_SUCCESS);

      default:
         tr = TR_OUT_OF_MEMORY;
         if (lResult != ERROR_OUTOFMEMORY)
            WARNING_OUT((TEXT("TranslateLRESULTToTWINRESULT(): Translating unlisted LRESULT %ld to TWINRESULT %s."),
                         lResult,
                         GetTWINRESULTString(tr)));
         break;
   }

   return(tr);
}


/*
** AllocateValueDataBuffer()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
LONG PRIVATE_CODE AllocateValueDataBuffer(HKEY hkey, PVOID *ppvData,
                                          PDWORD pdwcbLen)
{
   LONG lResult;

   ASSERT(IS_VALID_HANDLE(hkey, KEY));
   ASSERT(IS_VALID_WRITE_PTR(ppvData, PVOID));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbLen, DWORD));

   lResult = RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, pdwcbLen, NULL, NULL);

   if (lResult == ERROR_SUCCESS)
   {
      if (! AllocateMemory(*pdwcbLen, ppvData))
         lResult = ERROR_OUTOFMEMORY;
   }

   ASSERT(lResult != ERROR_SUCCESS ||
          IS_VALID_WRITE_BUFFER_PTR(*ppvData, VOID, *pdwcbLen));

   return(lResult);
}


/*
** EnumBriefcases()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE LONG EnumBriefcases(HKEY hkeyBriefcases, ENUMBRIEFCASESPROC ebcp,
                                 PCVOID pcvRefData, LPTSTR pszValueNameBuf,
                                 PBOOL pbAbort)
{
   LONG lResult;
   DWORD dwcbMaxValueDataLen;
   PLINKINFO pli;

   /* pcvRefData may be any value. */

   ASSERT(IS_VALID_HANDLE(hkeyBriefcases, KEY));
   ASSERT(IS_VALID_CODE_PTR(ebcp, ENUMBRIEFCASESPROC));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszValueNameBuf, STR, MAX_VALUE_NAME_LEN));
   ASSERT(IS_VALID_WRITE_PTR(pbAbort, BOOL));

   /* Allocate a buffer to hold the largest value's data. */

   lResult = AllocateValueDataBuffer(hkeyBriefcases, &pli,
                                     &dwcbMaxValueDataLen);

   if (lResult == ERROR_SUCCESS)
   {
      DWORD dwiValue;

      /* Look through the briefcases looking for matching LinkInfo. */

      *pbAbort = FALSE;
      dwiValue = 0;

      do
      {
         DWORD dwcbValueNameLen;
         DWORD dwType;
         DWORD dwcbDataLen;

         dwcbValueNameLen = MAX_VALUE_NAME_LEN;
         dwcbDataLen = dwcbMaxValueDataLen;
         lResult = RegEnumValue(hkeyBriefcases, dwiValue, pszValueNameBuf,
                                &dwcbValueNameLen, NULL, &dwType, (PBYTE)pli,
                                &dwcbDataLen);

         switch (lResult)
         {
            case ERROR_SUCCESS:
               if (dwcbDataLen >= sizeof(pli->ucbSize) &&
                   pli->ucbSize == dwcbDataLen)
                  lResult = (*ebcp)(pli, pcvRefData, pbAbort);
               else
                  WARNING_OUT((TEXT("EnumBriefcases(): Value %s under %s\\%s is not a valid LinkInfo structure."),
                               pszValueNameBuf,
                               HKEY_BRIEFCASE_ROOT_STRING,
                               BRIEFCASE_SUBKEY));
               break;

            case ERROR_MORE_DATA:
               /*
                * Watch out for value names that are too long, and added
                * data values that are too long.
                */

               /* (+ 1) for null terminator. */

               if (dwcbValueNameLen >= MAX_VALUE_NAME_LEN)
                  WARNING_OUT((TEXT("EnumBriefcases(): Value %s under %s\\%s is too long.  %u bytes > %u bytes."),
                               pszValueNameBuf,
                               HKEY_BRIEFCASE_ROOT_STRING,
                               BRIEFCASE_SUBKEY,
                               dwcbValueNameLen + 1,
                               MAX_VALUE_NAME_LEN));
               if (dwcbDataLen > dwcbMaxValueDataLen)
                  WARNING_OUT((TEXT("EnumBriefcases(): Value %s's data under %s\\%s is too long.  %u bytes > %u bytes."),
                               pszValueNameBuf,
                               HKEY_BRIEFCASE_ROOT_STRING,
                               BRIEFCASE_SUBKEY,
                               dwcbDataLen,
                               dwcbMaxValueDataLen));

               /* Skip this value. */

               lResult = ERROR_SUCCESS;
               break;

            default:
               break;
         }
      } while (lResult == ERROR_SUCCESS &&
               ! *pbAbort &&
               dwiValue++ < DWORD_MAX);

      if (lResult == ERROR_NO_MORE_ITEMS)
         lResult = ERROR_SUCCESS;

      FreeMemory(pli);
   }

   return(lResult);
}


/*
** GetUnusedBriefcaseValueName()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE LONG GetUnusedBriefcaseValueName(HKEY hkeyBriefcases,
                                              LPTSTR pszValueNameBuf)
{
   LONG lResult;
   DWORD dwValueNumber;
   BOOL bFound;

   ASSERT(IS_VALID_HANDLE(hkeyBriefcases, KEY));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszValueNameBuf, STR, MAX_VALUE_NAME_LEN));

   dwValueNumber = 0;
   bFound = FALSE;

   do
   {
      wsprintf(pszValueNameBuf, TEXT("%lu"), dwValueNumber);
      ASSERT((DWORD)lstrlen(pszValueNameBuf) < MAX_VALUE_NAME_LEN);

      lResult = RegQueryValueEx(hkeyBriefcases, pszValueNameBuf, NULL, NULL,
                                NULL, NULL);

      switch (lResult)
      {
         case ERROR_SUCCESS:
            /* Used value name.  Continue searching. */
            TRACE_OUT((TEXT("GetUnusedBriefcaseValueName(): Found used briefcase value name %s."),
                       pszValueNameBuf));
            break;

         case ERROR_FILE_NOT_FOUND:
            /* Unused value name.  Stop searching. */
            lResult = ERROR_SUCCESS;
            bFound = TRUE;
            TRACE_OUT((TEXT("GetUnusedBriefcaseValueName(): Found unused briefcase value name %s."),
                       pszValueNameBuf));
            break;

         default:
            WARNING_OUT((TEXT("GetUnusedBriefcaseValueName(): RegQueryValueEx() failed, returning %ld."),
                         lResult));
            break;
      }
   } while (lResult == ERROR_SUCCESS &&
            ! bFound &&
            dwValueNumber++ < DWORD_MAX);

   if (dwValueNumber == DWORD_MAX)
   {
      ASSERT(lResult == ERROR_SUCCESS &&
             ! bFound);
      WARNING_OUT((TEXT("GetUnusedBriefcaseValueName(): All value names in use.")));

      lResult = ERROR_CANTWRITE;
   }

   return(lResult);
}


/*
** CreateBriefcaseIterator()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT CreateBriefcaseIterator(PBRFCASEITER *ppbciter)
{
   TWINRESULT tr;
   LONG lResult;
   HKEY hkeyBriefcases;

   ASSERT(IS_VALID_WRITE_PTR(ppbciter, PBRFCASEITER));

   lResult = RegOpenKeyEx(HKEY_BRIEFCASE_ROOT, BRIEFCASE_SUBKEY, 0,
                          (KEY_QUERY_VALUE | KEY_SET_VALUE), &hkeyBriefcases);

   if (lResult == ERROR_SUCCESS)
   {
      DWORD dwcBriefcases;

      lResult = RegQueryInfoKey(hkeyBriefcases, NULL, NULL, NULL, NULL, NULL,
                                NULL, NULL, &dwcBriefcases, NULL, NULL, NULL);

      if (lResult == ERROR_SUCCESS)
      {
         if (dwcBriefcases > 0)
         {
            tr = TR_OUT_OF_MEMORY;

            if (AllocateMemory(sizeof(**ppbciter), ppbciter))
            {
               NEWPTRARRAY npa;

               npa.aicInitialPtrs = dwcBriefcases;
               npa.aicAllocGranularity = 1;
               npa.dwFlags = NPA_FL_SORTED_ADD;

               if (CreatePtrArray(&npa, &((*ppbciter)->hpa)))
               {
                  TCHAR rgchValueName[MAX_VALUE_NAME_LEN];
                  BOOL bAbort;

                  (*ppbciter)->aiNext = 0;

                  tr = TranslateLRESULTToTWINRESULT(
                           EnumBriefcases(hkeyBriefcases,
                                          &AddBriefcaseToIteratorProc,
                                          *ppbciter, rgchValueName, &bAbort));

                  if (tr == TR_SUCCESS)
                     ASSERT(! bAbort);
                  else
                  {
                     DestroyPtrArray((*ppbciter)->hpa);
CREATEBRIEFCASEITERATOR_BAIL:
                     FreeMemory(*ppbciter);
                  }
               }
               else
                  goto CREATEBRIEFCASEITERATOR_BAIL;
            }
         }
         else
            tr = TR_NO_MORE;
      }
      else
         tr = TranslateLRESULTToTWINRESULT(lResult);
   }
   else
   {
      /* ERROR_FILE_NOT_FOUND is returned for a non-existent key. */

      if (lResult == ERROR_FILE_NOT_FOUND)
         tr = TR_NO_MORE;
      else
         /* RAIDRAID: (16279) We should map to other TWINRESULTs here. */
         tr = TR_OUT_OF_MEMORY;
   }

   ASSERT(tr != TR_SUCCESS ||
          IS_VALID_STRUCT_PTR(*ppbciter, CBRFCASEITER));

   return(tr);
}


/*
** GetNextBriefcaseIterator()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT GetNextBriefcaseIterator(PBRFCASEITER pbciter,
                                                 PBRFCASEINFO pbcinfo)
{
   TWINRESULT tr = TR_NO_MORE;
   ARRAYINDEX aicBriefcases;

   ASSERT(IS_VALID_STRUCT_PTR(pbciter, CBRFCASEITER));
   ASSERT(IS_VALID_WRITE_PTR(pbcinfo, BRFCASEINFO));
   ASSERT(pbcinfo->ulSize == sizeof(*pbcinfo));

   aicBriefcases = GetPtrCount(pbciter->hpa);

   while (pbciter->aiNext < aicBriefcases)
   {
      PCLINKINFO pcli;
      DWORD dwOutFlags;
      PLINKINFO pliUpdated;
      BOOL bRemoveBriefcase = FALSE;

      pcli = GetPtr(pbciter->hpa, pbciter->aiNext);

      if (ResolveLinkInfo(pcli, pbcinfo->rgchDatabasePath,
                          (RLI_IFL_UPDATE | RLI_IFL_LOCAL_SEARCH), NULL,
                          &dwOutFlags, &pliUpdated))
      {
         if (PathExists(pbcinfo->rgchDatabasePath))
         {
            /* Found an existing briefcase database. */

            if (IS_FLAG_SET(dwOutFlags, RLI_OFL_UPDATED))
            {
               if (UpdateBriefcaseLinkInfo(pcli, pliUpdated))
                  TRACE_OUT((TEXT("GetNextBriefcaseIterator(): Updated LinkInfo for briefcase database %s."),
                             pbcinfo->rgchDatabasePath));
               else
                  WARNING_OUT((TEXT("GetNextBriefcaseIterator(): Failed to update LinkInfo for briefcase database %s."),
                               pbcinfo->rgchDatabasePath));
            }

            tr = TR_SUCCESS;
         }
         else
            bRemoveBriefcase = TRUE;

         if (IS_FLAG_SET(dwOutFlags, RLI_OFL_UPDATED))
            DestroyLinkInfo(pliUpdated);
      }
      else
      {
         /*
          * GetLastError() here to differentiate an out of memory condition and
          * all other errors.  Remove the briefcase from the system for all
          * errors except out of memory, e.g., unavailable volume or invalid
          * parameter.
          */

         if (GetLastError() != ERROR_OUTOFMEMORY)
            bRemoveBriefcase = TRUE;
      }

      if (bRemoveBriefcase)
      {
         if (MyRemoveBriefcaseFromSystem(pcli) == TR_SUCCESS)
            TRACE_OUT((TEXT("GetNextBriefcaseIterator(): Unavailable/missing briefcase removed from system.")));
         else
            WARNING_OUT((TEXT("GetNextBriefcaseIterator(): Failed to remove unavailable/missing briefcase from system.")));
      }

      ASSERT(pbciter->aiNext < ARRAYINDEX_MAX);
      pbciter->aiNext++;

      if (tr == TR_SUCCESS)
         break;
   }

   return(tr);
}


/*
** DestroyBriefcaseIterator()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyBriefcaseIterator(PBRFCASEITER pbciter)
{
   ARRAYINDEX ai;
   ARRAYINDEX aicPtrs;

   ASSERT(IS_VALID_STRUCT_PTR(pbciter, CBRFCASEITER));

   aicPtrs = GetPtrCount(pbciter->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      FreeMemory(GetPtr(pbciter->hpa, ai));

   DestroyPtrArray(pbciter->hpa);
   FreeMemory(pbciter);

   return;
}


/*
** AddBriefcaseToIteratorProc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE LONG AddBriefcaseToIteratorProc(PCLINKINFO pcli, PCVOID pcbciter,
                                             PBOOL pbAbort)
{
   LONG lResult;
   PLINKINFO pliCopy;

   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcbciter, CBRFCASEITER));
   ASSERT(IS_VALID_WRITE_PTR(pbAbort, BOOL));

   /* Add this briefcase database's LinkInfo to the iterator's list. */

   *pbAbort = TRUE;
   lResult = ERROR_OUTOFMEMORY;

   if (CopyLinkInfo(pcli, &pliCopy))
   {
      ARRAYINDEX ai;

      if (AddPtr(((PCBRFCASEITER)pcbciter)->hpa, LinkInfoSortCmp, pliCopy, &ai))
      {
         *pbAbort = FALSE;
         lResult = ERROR_SUCCESS;
      }
      else
         FreeMemory(pliCopy);
   }

   if (lResult == ERROR_SUCCESS)
      TRACE_OUT((TEXT("AddBriefcaseToIteratorProc(): Added LinkInfo for briefcase to briefcase iterator %#lx."),
                 pcbciter));
   else
      WARNING_OUT((TEXT("AddBriefcaseToIteratorProc(): Failed to add LinkInfo for briefcase to briefcase iterator %#lx."),
                   pcbciter));

   return(lResult);
}


/*
** CompareLinkInfoProc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE LONG CompareLinkInfoProc(PCLINKINFO pcli, PCVOID pcliTarget,
                                      PBOOL pbAbort)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcliTarget, CLINKINFO));
   ASSERT(IS_VALID_WRITE_PTR(pbAbort, BOOL));

   /* Does this LinkInfo match our target LinkInfo? */

   *pbAbort = (LinkInfoSearchCmp(pcli, pcliTarget) == CR_EQUAL);

   return(ERROR_SUCCESS);
}


/*
** MyAddBriefcaseToSystem()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT MyAddBriefcaseToSystem(PCLINKINFO pcli)
{
   LONG lResult;
   HKEY hkeyBriefcases;
   DWORD dwDisposition;

   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));

   /* Open briefcase list registry key for common access. */

   lResult = RegCreateKeyEx(HKEY_BRIEFCASE_ROOT, BRIEFCASE_SUBKEY, 0, NULL,
                            REG_OPTION_NON_VOLATILE,
                            (KEY_QUERY_VALUE | KEY_SET_VALUE), NULL,
                            &hkeyBriefcases, &dwDisposition);

   if (lResult == ERROR_SUCCESS)
   {
      TCHAR rgchValueName[MAX_VALUE_NAME_LEN];
      BOOL bFound;
      LONG lClose;

      lResult = EnumBriefcases(hkeyBriefcases, &CompareLinkInfoProc, pcli,
                               rgchValueName, &bFound);

      if (lResult == ERROR_SUCCESS)
      {
         if (bFound)
            TRACE_OUT((TEXT("AddBriefcaseToSystem(): Briefcase database already in registry list as value %s under %s\\%s."),
                       rgchValueName,
                       HKEY_BRIEFCASE_ROOT_STRING,
                       BRIEFCASE_SUBKEY));
         else
         {
            lResult = GetUnusedBriefcaseValueName(hkeyBriefcases,
                                                  rgchValueName);

            if (lResult == ERROR_SUCCESS)
            {
               lResult = RegSetValueEx(hkeyBriefcases, rgchValueName, 0,
                                       REG_BINARY, (PCBYTE)pcli,
                                       pcli->ucbSize);

               if (lResult == ERROR_SUCCESS)
                  TRACE_OUT((TEXT("AddBriefcaseToSystem(): Briefcase database added to registry list as value %s under %s\\%s."),
                             rgchValueName,
                             HKEY_BRIEFCASE_ROOT_STRING,
                             BRIEFCASE_SUBKEY));
            }
         }
      }

      lClose = RegCloseKey(hkeyBriefcases);

      if (lResult == ERROR_SUCCESS)
         lResult = lClose;
   }

   return(TranslateLRESULTToTWINRESULT(lResult));
}


/*
** MyRemoveBriefcaseFromSystem()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT MyRemoveBriefcaseFromSystem(PCLINKINFO pcli)
{
   LONG lResult;
   HKEY hkeyBriefcases;

   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));

   /* Open briefcase list registry key for common access. */

   lResult = RegOpenKeyEx(HKEY_BRIEFCASE_ROOT, BRIEFCASE_SUBKEY, 0,
                          (KEY_QUERY_VALUE | KEY_SET_VALUE), &hkeyBriefcases);

   if (lResult == ERROR_SUCCESS)
   {
      TCHAR rgchValueName[MAX_VALUE_NAME_LEN];
      BOOL bFound;
      LONG lClose;

      lResult = EnumBriefcases(hkeyBriefcases, &CompareLinkInfoProc, pcli,
                               rgchValueName, &bFound);

      if (lResult == ERROR_SUCCESS)
      {
         if (bFound)
         {
            lResult = RegDeleteValue(hkeyBriefcases, rgchValueName);

            if (lResult == ERROR_SUCCESS)
               TRACE_OUT((TEXT("MyRemoveBriefcaseFromSystem(): Briefcase database removed from registry list as value %s under %s\\%s."),
                          rgchValueName,
                          HKEY_BRIEFCASE_ROOT_STRING,
                          BRIEFCASE_SUBKEY));
         }
         else
            WARNING_OUT((TEXT("MyRemoveBriefcaseFromSystem(): Briefcase database not in registry list under %s\\%s."),
                         HKEY_BRIEFCASE_ROOT_STRING,
                         BRIEFCASE_SUBKEY));
      }

      lClose = RegCloseKey(hkeyBriefcases);

      if (lResult == ERROR_SUCCESS)
         lResult = lClose;
   }
   else if (lResult == ERROR_FILE_NOT_FOUND)
   {
      WARNING_OUT((TEXT("MyRemoveBriefcaseFromSystem(): Briefcase key %s\\%s does not exist."),
                   HKEY_BRIEFCASE_ROOT_STRING,
                   BRIEFCASE_SUBKEY));

      lResult = ERROR_SUCCESS;
   }

   return(TranslateLRESULTToTWINRESULT(lResult));
}


/*
** UpdateBriefcaseLinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT UpdateBriefcaseLinkInfo(PCLINKINFO pcliOriginal,
                                                PCLINKINFO pcliUpdated)
{
   LONG lResult;
   HKEY hkeyBriefcases;
   DWORD dwDisposition;

   ASSERT(IS_VALID_STRUCT_PTR(pcliOriginal, CLINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcliUpdated, CLINKINFO));

   /* Open briefcase list registry key for common access. */

   lResult = RegCreateKeyEx(HKEY_BRIEFCASE_ROOT, BRIEFCASE_SUBKEY, 0, NULL,
                            REG_OPTION_NON_VOLATILE,
                            (KEY_QUERY_VALUE | KEY_SET_VALUE), NULL,
                            &hkeyBriefcases, &dwDisposition);

   if (lResult == ERROR_SUCCESS)
   {
      TCHAR rgchValueName[MAX_VALUE_NAME_LEN];
      BOOL bFound;
      LONG lClose;

      lResult = EnumBriefcases(hkeyBriefcases, &CompareLinkInfoProc,
                               pcliOriginal, rgchValueName, &bFound);

      if (lResult == ERROR_SUCCESS)
      {
         if (bFound)
         {
            lResult = RegSetValueEx(hkeyBriefcases, rgchValueName, 0,
                                    REG_BINARY, (PCBYTE)pcliUpdated,
                                    pcliUpdated->ucbSize);

            if (lResult == ERROR_SUCCESS)
               TRACE_OUT((TEXT("UpdateBriefcaseLinkInfo(): Briefcase database LinkInfo updated in registry list as value %s under %s\\%s."),
                          rgchValueName,
                          HKEY_BRIEFCASE_ROOT_STRING,
                          BRIEFCASE_SUBKEY));
         }
         else
            WARNING_OUT((TEXT("UpdateBriefcaseLinkInfo(): Briefcase database LinkInfo not found in registry list under %s\\%s."),
                         HKEY_BRIEFCASE_ROOT_STRING,
                         BRIEFCASE_SUBKEY));
      }

      lClose = RegCloseKey(hkeyBriefcases);

      if (lResult == ERROR_SUCCESS)
         lResult = lClose;
   }

   return(TranslateLRESULTToTWINRESULT(lResult));
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidPCBRFCASEITER()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCBRFCASEITER(PCBRFCASEITER pcbciter)
{
   BOOL bResult = FALSE;

   if (IS_VALID_READ_PTR(pcbciter, CBRFCASEITER) &&
       IS_VALID_HANDLE(pcbciter->hpa, PTRARRAY))
   {
      ARRAYINDEX aicPtrs;
      ARRAYINDEX ai;

      aicPtrs = GetPtrCount(pcbciter->hpa);

      for (ai = 0; ai < aicPtrs; ai++)
      {
         if (! IS_VALID_STRUCT_PTR(GetPtr(pcbciter->hpa, ai), CLINKINFO))
            break;
      }

      bResult = (ai == aicPtrs);
   }

   return(bResult);
}

#endif


#ifdef EXPV

/*
** IsValidHBRFCASEITER()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidHBRFCASEITER(HBRFCASEITER hbciter)
{
   return(IS_VALID_STRUCT_PTR((PCBRFCASEITER)hbciter, CBRFCASEITER));
}

#endif


/****************************** Public Functions *****************************/


/*
** AddBriefcaseToSystem()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT AddBriefcaseToSystem(LPCTSTR pcszBriefcaseDatabase)
{
   TWINRESULT tr;
   PLINKINFO pli;

   ASSERT(IsFullPath(pcszBriefcaseDatabase));

   if (CreateLinkInfo(pcszBriefcaseDatabase, &pli))
   {
      tr = MyAddBriefcaseToSystem(pli);

      DestroyLinkInfo(pli);
   }
   else
   {
      /*
       * GetLastError() here to differentiate between TR_UNAVAILABLE_VOLUME and
       * TR_OUT_OF_MEMORY.
       */

      if (GetLastError() == ERROR_OUTOFMEMORY)
         tr = TR_OUT_OF_MEMORY;
      else
         tr = TR_UNAVAILABLE_VOLUME;
   }

   return(tr);
}


/*
** RemoveBriefcaseFromSystem()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT RemoveBriefcaseFromSystem(LPCTSTR pcszBriefcaseDatabase)
{
   TWINRESULT tr;
   PLINKINFO pli;

   ASSERT(IsFullPath(pcszBriefcaseDatabase));

   if (CreateLinkInfo(pcszBriefcaseDatabase, &pli))
   {
      tr = MyRemoveBriefcaseFromSystem(pli);

      DestroyLinkInfo(pli);
   }
   else
   {
      /*
       * GetLastError() here to differentiate between TR_UNAVAILABLE_VOLUME and
       * TR_OUT_OF_MEMORY.
       */

      if (GetLastError() == ERROR_OUTOFMEMORY)
         tr = TR_OUT_OF_MEMORY;
      else
         tr = TR_UNAVAILABLE_VOLUME;
   }

   return(tr);
}


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | FindFirstBriefcase | Finds the first briefcase in the current
user's list of briefcases.

@parm PHBRFCASEITER | phbciter | A pointer to an HBRFCASEITER to be filled in
with a handle identifying the briefcase enumeration data associated with this
call to FindFirstBriefcase().  This handle may be passed to FindNextBriefcase()
amd FindCloseBriefcase().  This handle is only valid until FindBriefcaseClose()
is called on it.

@parm PBRFCASEINFO | pbcinfo | A pointer to a BRFCASEINFO to be filled in with
information describing the first enumerated briefcase.  The information in
*pbcinfo is only valid until FindBriefcaseClose() is called on *phbciter.

@rdesc If there is at least one existing briefcase in the user's list of
briefcases, TR_SUCCESS is returned, *phbciter is filled in with a handle
identifying the briefcase enumeration data associated with this call, and
*pbcinfo contains information describing the first briefcase in the user's list
of briefcases.  If there are no existing briefcases in the user's list of
briefcases, TR_NO_MORE is returned.  Otherwise, the return value indicates the
error that occurred.  *phbciter and *pbcinfo are only valid if TR_SUCCESS is
returned.

@comm To find the next briefcase in the user's list of briefcases, call
FindNextBriefcase() with *phbciter.  Once the caller is finished enumerating
briefcases, FindBriefcaseClose() should be called with *phbciter to free the
briefcase enumeration data.

@xref FindNextBriefcase() FindBriefcaseClose()

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI FindFirstBriefcase(PHBRFCASEITER phbciter,
                                                PBRFCASEINFO pbcinfo)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(FindFirstBriefcase);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_WRITE_PTR(phbciter, HBRFCASEITER) &&
          IS_VALID_WRITE_PTR(pbcinfo, BRFCASEINFO) &&
          EVAL(pbcinfo->ulSize == sizeof(*pbcinfo)))
#endif
      {
         PBRFCASEITER pbciter;

         tr = CreateBriefcaseIterator(&pbciter);

         if (tr == TR_SUCCESS)
         {
            tr = GetNextBriefcaseIterator(pbciter, pbcinfo);

            if (tr == TR_SUCCESS)
               *phbciter = (HBRFCASEITER)pbciter;
            else
               DestroyBriefcaseIterator(pbciter);
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(FindFirstBriefcase, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | FindNextBriefcase | Finds the next briefcase in the current
user's list of briefcases.

@parm HBRFCASEITER | hbciter | A handle identifying the briefcase enumeration
data associated with a call to FindFirstBriefcase().

@parm PBRFCASEINFO | pbcinfo | A pointer to a BRFCASEINFO to be filled in with
information describing the next enumerated briefcase.  The information in
*pbcinfo is only valid until FindBriefcaseClose() is called on hbciter.

@rdesc If there is at least one more existing briefcase in the user's list of
briefcases, TR_SUCCESS is returned, and *pbcinfo contains information
describing the next briefcase in the user's list of briefcases.  If there are
no more existing briefcases in the user's list of briefcases, TR_NO_MORE is
returned.  Otherwise, the return value indicates the error that occurred.
*pbcinfo is only valid if TR_SUCCESS is returned.

@xref FindFirstBriefcase() FindBriefcaseClose()

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI FindNextBriefcase(HBRFCASEITER hbciter,
                                               PBRFCASEINFO pbcinfo)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(FindNextBriefcase);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbciter, BRFCASEITER) &&
          IS_VALID_WRITE_PTR(pbcinfo, BRFCASEINFO) &&
          EVAL(pbcinfo->ulSize == sizeof(*pbcinfo)))
#endif
      {
         tr = GetNextBriefcaseIterator((PBRFCASEITER)hbciter, pbcinfo);
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(FindNextBriefcase, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | FindBriefcaseClose | Terminates briefcase enumeration started
by FindFirstBriefcase().

@parm HBRFCASEITER | hbciter | A handle identifying the briefcase enumeration
data associated with a call to FindFirstBriefcase().  This handle is invalid
after calling FindBriefcaseClose().

@rdesc If the briefcase enumeration was terminated successfully, TR_SUCCESS is
returned.  Otherwise, the return value indicates the error that occurred.

@comm The information in any BRFCASEINFO structures returned by the call to
FindFirstBriefcase() that returned hbciter, and by any subsequent calls to
FindNextBriefcase() with hbciter, is invalid after FindBriefcaseClose() is
called on hbciter.

@xref FindFirstBriefcase() FindNextBriefcase()

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI FindBriefcaseClose(HBRFCASEITER hbciter)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(FindBriefcaseClose);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbciter, BRFCASEITER))
#endif
      {
         DestroyBriefcaseIterator((PBRFCASEITER)hbciter);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(FindBriefcaseClose, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}

