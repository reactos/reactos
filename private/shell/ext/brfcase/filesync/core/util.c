/*
 * util.c - Miscellaneous utility functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#ifdef WINNT
#include <uastrfnc.h>  // for ualstrcpyn (used on unaligned UNICODE strings)
#endif

/****************************** Public Functions *****************************/


/*
** NotifyShell()
**
** Notifies the Shell of an event.
**
** Arguments:     pcszPath - path string related to event
**                nse - event
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void NotifyShell(LPCTSTR pcszPath, NOTIFYSHELLEVENT nse)
{

#pragma data_seg(DATA_SEG_READ_ONLY)

   /*
    * N.b., these events must match the enumerated NOTIFYSHELLEVENT values in
    * util.h.
    */
   static const LONG SrgclShellEvents[] =
   {
      SHCNE_CREATE,
      SHCNE_DELETE,
      SHCNE_MKDIR,
      SHCNE_RMDIR,
      SHCNE_UPDATEITEM,
      SHCNE_UPDATEDIR
   };

#ifdef DEBUG

   static const LPCTSTR SrgpcszShellEvents[] =
   {
      TEXT("create item"),
      TEXT("delete item"),
      TEXT("create folder"),
      TEXT("delete folder"),
      TEXT("update item"),
      TEXT("update folder")
   };

#endif

#pragma data_seg()

   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(nse < ARRAY_ELEMENTS(SrgclShellEvents));
   ASSERT(nse < ARRAY_ELEMENTS(SrgpcszShellEvents));

   TRACE_OUT((TEXT("NotifyShell(): Sending %s notification for %s."),
              SrgpcszShellEvents[nse],
              pcszPath));

   SHChangeNotify(SrgclShellEvents[nse], SHCNF_PATH, pcszPath, NULL);
}


/*
** ComparePathStringsByHandle()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT ComparePathStringsByHandle(HSTRING hsFirst,
                                                        HSTRING hsSecond)
{
   ASSERT(IS_VALID_HANDLE(hsFirst, STRING));
   ASSERT(IS_VALID_HANDLE(hsSecond, STRING));

   return(CompareStringsI(hsFirst, hsSecond));
}


/*
** MyLStrCmpNI()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT MyLStrCmpNI(LPCTSTR pcsz1, LPCTSTR pcsz2, int ncbLen)
{
   int n = 0;

   ASSERT(IS_VALID_STRING_PTR(pcsz1, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcsz2, CSTR));
   ASSERT(ncbLen >= 0);

   while (ncbLen > 0 &&
          ! (n = PtrToUlong(CharLower((LPTSTR)(ULONG)*pcsz1))
               - PtrToUlong(CharLower((LPTSTR)(ULONG)*pcsz2))) &&
          *pcsz1)
   {
      pcsz1++;
      pcsz2++;
      ncbLen--;
   }

   return(MapIntToComparisonResult(n));
}


/*

/*
** ComposePath()
**
** Composes a path string given a folder and a filename.
**
** Arguments:     pszBuffer - path string that is created
**                pcszFolder - path string of the folder
**                pcszName - path to append
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., truncates path to MAX_PATH_LEN bytes in length.
*/
PUBLIC_CODE void ComposePath(LPTSTR pszBuffer, LPCTSTR pcszFolder, LPCTSTR pcszName)
{
   ASSERT(IS_VALID_STRING_PTR(pszBuffer, STR));
   ASSERT(IS_VALID_STRING_PTR(pcszFolder, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszBuffer, STR, MAX_PATH_LEN));

#ifdef WINNT
   //
   // BUGBUG - BobDay - We should figure out who needs this unaligned thing
   // and remove it from here.  The function prototype doesn't mention any
   // unaligned stuff so we must have a bug somewhere.
   // Consider adding a debug check here for an unaligned pcszFolder pointer
   // and assert when it occurs so we can debug it.
   //
   ualstrcpyn(pszBuffer, pcszFolder, MAX_PATH_LEN);
#else      
   MyLStrCpyN(pszBuffer, pcszFolder, MAX_PATH_LEN);
#endif

   CatPath(pszBuffer, pcszName);

   ASSERT(IS_VALID_STRING_PTR(pszBuffer, STR));

   return;
}


/*
** ExtractFileName()
**
** Extracts the file name from a path name.
**
** Arguments:     pcszPathName - path string from which to extract file name
**
** Returns:       Pointer to file name in path string.
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR ExtractFileName(LPCTSTR pcszPathName)
{
   LPCTSTR pcszLastComponent;
   LPCTSTR pcsz;

   ASSERT(IS_VALID_STRING_PTR(pcszPathName, CSTR));

   for (pcszLastComponent = pcsz = pcszPathName;
        *pcsz;
        pcsz = CharNext(pcsz))
   {
      if (IS_SLASH(*pcsz) || *pcsz == COLON)
         pcszLastComponent = CharNext(pcsz);
   }

   ASSERT(IS_VALID_STRING_PTR(pcszLastComponent, CSTR));

   return(pcszLastComponent);
}


/*
** ExtractExtension()
**
** Extracts the extension from a file.
**
** Arguments:     pcszName - name whose extension is to be extracted
**
** Returns:       If the name contains an extension, a pointer to the period at
**                the beginning of the extension is returned.  If the name has
**                no extension, a pointer to the name's null terminator is
**                returned.
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR ExtractExtension(LPCTSTR pcszName)
{
   LPCTSTR pcszLastPeriod;

   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));

   /* Make sure we have an isolated file name. */

   pcszName = ExtractFileName(pcszName);

   pcszLastPeriod = NULL;

   while (*pcszName)
   {
      if (*pcszName == PERIOD)
         pcszLastPeriod = pcszName;

      pcszName = CharNext(pcszName);
   }

   if (! pcszLastPeriod)
   {
      /* Point at null terminator. */

      pcszLastPeriod = pcszName;
      ASSERT(! *pcszLastPeriod);
   }
   else
      /* Point at period at beginning of extension. */
      ASSERT(*pcszLastPeriod == PERIOD);

   ASSERT(IS_VALID_STRING_PTR(pcszLastPeriod, CSTR));

   return(pcszLastPeriod);
}


/*
** GetHashBucketIndex()
**
** Calculates the hash bucket index for a string.
**
** Arguments:     pcsz - pointer to string whose hash bucket index is to be
**                        calculated
**                hbc - number of hash buckets in string table
**
** Returns:       Hash bucket index for string.
**
** Side Effects:  none
**
** The hashing function used is the sum of the byte values in the string modulo
** the number of buckets in the hash table.
*/
PUBLIC_CODE HASHBUCKETCOUNT GetHashBucketIndex(LPCTSTR pcsz,
                                               HASHBUCKETCOUNT hbc)
{
   ULONG ulSum;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(hbc > 0);

   /* Don't worry about overflow here. */

   for (ulSum = 0; *pcsz; pcsz++)
      ulSum += *pcsz;

   return((HASHBUCKETCOUNT)(ulSum % hbc));
}


/*
** RegKeyExists()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL RegKeyExists(HKEY hkeyParent, LPCTSTR pcszSubKey)
{
   BOOL bResult;
   HKEY hkeySubKey;

   ASSERT(IS_VALID_HANDLE(hkeyParent, KEY));
   ASSERT(IS_VALID_STRING_PTR(pcszSubKey, CSTR));

   bResult = (RegOpenKeyEx(hkeyParent, pcszSubKey, 0, KEY_QUERY_VALUE,
                           &hkeySubKey)
              == ERROR_SUCCESS);

   if (bResult)
      EVAL(RegCloseKey(hkeySubKey) == ERROR_SUCCESS);

   return(bResult);
}


/*
** CopyLinkInfo()
**
** Copies LinkInfo into local memory.
**
** Arguments:     pcliSrc - source LinkInfo
**                ppliDest - pointer to PLINKINFO to be filled in with pointer
**                           to local copy
**
** Returns:       TRUE if successful.  FALSE if not.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CopyLinkInfo(PCLINKINFO pcliSrc, PLINKINFO *ppliDest)
{
   BOOL bResult;
   DWORD dwcbSize;

   ASSERT(IS_VALID_STRUCT_PTR(pcliSrc, CLINKINFO));
   ASSERT(IS_VALID_WRITE_PTR(ppliDest, PLINKINFO));

   dwcbSize = *(PDWORD)pcliSrc;

   bResult = AllocateMemory(dwcbSize, ppliDest);

   if (bResult)
      CopyMemory(*ppliDest, pcliSrc, dwcbSize);

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppliDest, CLINKINFO));

   return(bResult);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidPCLINKINFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCLINKINFO(PCLINKINFO pcli)
{
   BOOL bResult;

   if (IS_VALID_READ_BUFFER_PTR(pcli, CDWORD, sizeof(DWORD)) &&
       IS_VALID_READ_BUFFER_PTR(pcli, CLINKINFO, (UINT)*(PDWORD)pcli))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}

#endif
