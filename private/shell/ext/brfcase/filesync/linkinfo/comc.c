/*
 * comc.c - Shared routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/****************************** Public Functions *****************************/


/*
** CatPath()
**
** Appends a filename to a path string.
**
** Arguments:     pszPath - path string that file name is to be appended to
**                pcszSubPath - path to append
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., truncates path to MAX_PATH_LEN characters in length.
**
** Examples:
**
**    input path        input file name      output path
**    ----------        ---------------      -----------
**    c:\               foo                  c:\foo
**    c:                foo                  c:foo
**    c:\foo\bar\       goo                  c:\foo\bar\goo
**    c:\foo\bar\       \goo                 c:\foo\bar\goo
**    c:\foo\bar\       goo\shoe             c:\foo\bar\goo\shoe
**    c:\foo\bar\       \goo\shoe\           c:\foo\bar\goo\shoe\
**    foo\bar\          goo                  foo\bar\goo
**    <empty string>    <empty string>       <empty string>
**    <empty string>    foo                  foo
**    foo               <empty string>       foo
**    fred              bird                 fred\bird
*/
PUBLIC_CODE void CatPath(LPTSTR pszPath, LPCTSTR pcszSubPath)
{
   LPTSTR pcsz;
   LPTSTR pcszLast;

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPath, STR, MAX_PATH_LEN - lstrlen(pszPath)));

   /* Find last character in path string. */

   for (pcsz = pcszLast = pszPath; *pcsz; pcsz = CharNext(pcsz))
      pcszLast = pcsz;

   if (IS_SLASH(*pcszLast) && IS_SLASH(*pcszSubPath))
      pcszSubPath++;
   else if (! IS_SLASH(*pcszLast) && ! IS_SLASH(*pcszSubPath))
   {
      if (*pcszLast && *pcszLast != COLON && *pcszSubPath)
         *pcsz++ = TEXT('\\');
   }

   MyLStrCpyN(pcsz, pcszSubPath, MAX_PATH_LEN - (int)(pcsz - pszPath));

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));

   return;
}


/*
** MapIntToComparisonResult()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT MapIntToComparisonResult(int nResult)
{
   COMPARISONRESULT cr;

   /* Any integer is valid input. */

   if (nResult < 0)
      cr = CR_FIRST_SMALLER;
   else if (nResult > 0)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


/*
** MyLStrCpyN()
**
** Like lstrcpy(), but the copy is limited to ucb bytes.  The destination
** string is always null-terminated.
**
** Arguments:     pszDest - pointer to destination buffer
**                pcszSrc - pointer to source string
**                ncb - maximum number of bytes to copy, including null
**                      terminator
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., this function behaves quite differently than strncpy()!  It does not
** pad out the destination buffer with null characters, and it always null
** terminates the destination string.
*/
PUBLIC_CODE void MyLStrCpyN(LPTSTR pszDest, LPCTSTR pcszSrc, int ncch)
{
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszDest, STR, ncch * sizeof(TCHAR)));
   ASSERT(IS_VALID_STRING_PTR(pcszSrc, CSTR));
   ASSERT(ncch > 0);

   while (ncch > 1)
   {
      ncch--;

      *pszDest = *pcszSrc;

      if (*pcszSrc)
      {
         pszDest++;
         pcszSrc++;
      }
      else
         break;
   }

   if (ncch == 1)
      *pszDest = TEXT('\0');

   ASSERT(IS_VALID_STRING_PTR(pszDest, STR));
   ASSERT(lstrlen(pszDest) < ncch);
   ASSERT(lstrlen(pszDest) <= lstrlen(pcszSrc));

   return;
}


#ifdef DEBUG

/*
** IsStringContained()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsStringContained(LPCTSTR pcszBigger, LPCTSTR pcszSuffix)
{
   ASSERT(IS_VALID_STRING_PTR(pcszBigger, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSuffix, CSTR));

   return(pcszSuffix >= pcszBigger &&
          pcszSuffix <= pcszBigger + lstrlen(pcszBigger));
}

#endif


#if defined(_SYNCENG_) || defined(_LINKINFO_)

/*
** DeleteLastPathElement()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DeleteLastPathElement(LPTSTR pszPath)
{
   LPTSTR psz;
   LPTSTR pszLastSep;

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));

   psz = pszPath;
   pszLastSep = psz;

   while (*psz)
   {
      if (*psz == TEXT('\\'))
         pszLastSep = psz;

      psz = CharNext(psz);
   }

   /*
    * Now truncate the path at the last separator found, or the beginning of
    * the path if no path separators were found.
    */

   *pszLastSep = TEXT('\0');

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));

   return;
}


/*
** GetDefaultRegKeyValue()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LONG GetDefaultRegKeyValue(HKEY hkeyParent, LPCTSTR pcszSubKey,
                                  LPTSTR pszBuf, PDWORD pdwcbBufLen)
{
   LONG lResult;
   HKEY hkeySubKey;

   ASSERT(IS_VALID_HANDLE(hkeyParent, KEY));
   ASSERT(IS_VALID_STRING_PTR(pcszSubKey, CSTR));
   ASSERT(! pszBuf ||
          IS_VALID_WRITE_BUFFER_PTR(pszBuf, STR, *pdwcbBufLen));

   lResult = RegOpenKeyEx(hkeyParent, pcszSubKey, 0, KEY_QUERY_VALUE,
                          &hkeySubKey);

   if (lResult == ERROR_SUCCESS)
   {
      DWORD dwValueType;

      lResult = RegQueryValueEx(hkeySubKey, NULL, NULL, &dwValueType,
                                (PBYTE)pszBuf, pdwcbBufLen);

      if (lResult == ERROR_SUCCESS)
      {
         ASSERT(dwValueType == REG_SZ);
         /* (+ 1) for null terminator. */
         ASSERT(! pszBuf ||
                (DWORD)(lstrlen(pszBuf) + 1) * sizeof(TCHAR) == *pdwcbBufLen);

         TRACE_OUT((TEXT("GetDefaultRegKeyValue(): Default key value for subkey %s is \"%s\"."),
                    pcszSubKey,
                    pszBuf));
      }
      else
         TRACE_OUT((TEXT("GetDefaultRegKeyValue(): RegQueryValueEx() for subkey %s failed, returning %ld."),
                    pcszSubKey,
                    lResult));

      EVAL(RegCloseKey(hkeySubKey) == ERROR_SUCCESS);
   }
   else
      TRACE_OUT((TEXT("GetDefaultRegKeyValue(): RegOpenKeyEx() for subkey %s failed, returning %ld."),
                 pcszSubKey,
                 lResult));

   return(lResult);
}


/*
** StringCopy()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL StringCopy(LPCTSTR pcszSrc, LPTSTR *ppszCopy)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRING_PTR(pcszSrc, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszCopy, LPTSTR));

   /* (+ 1) for null terminator. */

   bResult = AllocateMemory((lstrlen(pcszSrc) + 1) * sizeof(TCHAR), ppszCopy);

   if (bResult)
      lstrcpy(*ppszCopy, pcszSrc);

   ASSERT(! bResult ||
          IS_VALID_STRING_PTR(*ppszCopy, STR));

   return(bResult);
}


/*
** ComparePathStrings()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT ComparePathStrings(LPCTSTR pcszFirst, LPCTSTR pcszSecond)
{
   ASSERT(IS_VALID_STRING_PTR(pcszFirst, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSecond, CSTR));

   return(MapIntToComparisonResult(lstrcmpi(pcszFirst, pcszSecond)));
}


/*
** MyStrChr()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL MyStrChr(LPCTSTR pcsz, TCHAR chTarget, LPCTSTR *ppcszTarget)
{
   LPCTSTR pcszFound;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(! ppcszTarget || IS_VALID_WRITE_PTR(ppcszTarget, LPCTSTR));

   /* This works correctly if chTarget is the null terminator '\0'. */

   while (*pcsz && *pcsz != chTarget)
      pcsz = CharNext(pcsz);

   if (*pcsz == chTarget)
      pcszFound = pcsz;
   else
      pcszFound = NULL;

   if (ppcszTarget)
      *ppcszTarget = pcszFound;

   return(pcszFound != NULL);
}


/*
** PathExists()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL PathExists(LPCTSTR pcszPath)
{
   DWORD dwErrMode;
   BOOL fResult;

   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

   dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

   fResult = (GetFileAttributes(pcszPath) != -1);

   SetErrorMode(dwErrMode);

   return fResult;
}


/*
** IsDrivePath()
**
** Determines whether or not a path is in "c:\" form.
**
** Arguments:     pcszPath - path to examine
**
** Returns:       TRUE if path is in "c:\" form.  FALSE if not.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsDrivePath(LPCTSTR pcszFullPath)
{
   BOOL bResult;

   ASSERT(IsFullPath(pcszFullPath));

   if (lstrlen(pcszFullPath) >= 3 &&
       IsCharAlpha(pcszFullPath[0]) &&
       pcszFullPath[1] == COLON &&
       IS_SLASH(pcszFullPath[2]))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidDriveType()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidDriveType(UINT uDriveType)
{
   BOOL bResult;

   switch (uDriveType)
   {
      case DRIVE_UNKNOWN:
      case DRIVE_NO_ROOT_DIR:
      case DRIVE_REMOVABLE:
      case DRIVE_FIXED:
      case DRIVE_REMOTE:
      case DRIVE_CDROM:
      case DRIVE_RAMDISK:
         bResult = TRUE;
         break;

      default:
         ERROR_OUT((TEXT("IsValidDriveType(): Invalid drive type %u."),
                    uDriveType));
         bResult = FALSE;
         break;
   }

   return(bResult);
}


/*
** IsValidPathSuffix()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** A path suffix should not begin or end with a slash.
*/
PUBLIC_CODE BOOL IsValidPathSuffix(LPCTSTR pcszPathSuffix)
{
   return(IS_VALID_STRING_PTR(pcszPathSuffix, CSTR) &&
          EVAL(lstrlen(pcszPathSuffix) < MAX_PATH_LEN) &&
          EVAL(! IS_SLASH(*pcszPathSuffix)) &&
          EVAL(! IS_SLASH(*CharPrev(pcszPathSuffix, pcszPathSuffix + lstrlen(pcszPathSuffix)))));
}

#endif   /* DEBUG || VSTF */


#ifdef DEBUG

/*
** IsRootPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsRootPath(LPCTSTR pcszFullPath)
{
   TCHAR rgchCanonicalPath[MAX_PATH_LEN];
   DWORD dwOutFlags;
   TCHAR rgchNetResource[MAX_PATH_LEN];
   LPTSTR pszRootPathSuffix;

   ASSERT(IsFullPath(pcszFullPath));

   return(GetCanonicalPathInfo(pcszFullPath, rgchCanonicalPath, &dwOutFlags,
                               rgchNetResource, &pszRootPathSuffix) &&
          ! *pszRootPathSuffix);
}


/*
** IsTrailingSlashCanonicalized()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsTrailingSlashCanonicalized(LPCTSTR pcszFullPath)
{
   BOOL bResult;
   BOOL bSlashLast;
   LPCTSTR pcszLastPathChar;

   ASSERT(IsFullPath(pcszFullPath));

   /* Make sure that the path only ends in a slash for root paths. */

   pcszLastPathChar = CharPrev(pcszFullPath, pcszFullPath + lstrlen(pcszFullPath));

   ASSERT(pcszLastPathChar >= pcszFullPath);

   bSlashLast = IS_SLASH(*pcszLastPathChar);

   /* Is this a root path? */

   if (IsRootPath(pcszFullPath))
      bResult = bSlashLast;
   else
      bResult = ! bSlashLast;

   return(bResult);
}


/*
** IsFullPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsFullPath(LPCTSTR pcszPath)
{
   BOOL bResult = FALSE;
   TCHAR rgchFullPath[MAX_PATH_LEN];

   if (IS_VALID_STRING_PTR(pcszPath, CSTR) &&
       EVAL(lstrlen(pcszPath) < MAX_PATH_LEN))
   {
      DWORD dwPathLen;
      LPTSTR pszFileName;

      dwPathLen = GetFullPathName(pcszPath, ARRAYSIZE(rgchFullPath), rgchFullPath,
                                  &pszFileName);

      if (EVAL(dwPathLen > 0) &&
          EVAL(dwPathLen < ARRAYSIZE(rgchFullPath)))
         bResult = EVAL(ComparePathStrings(pcszPath, rgchFullPath) == CR_EQUAL);
   }

   return(bResult);
}


/*
** IsCanonicalPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsCanonicalPath(LPCTSTR pcszPath)
{
   return(EVAL(IsFullPath(pcszPath)) &&
          EVAL(IsTrailingSlashCanonicalized(pcszPath)));

}


/*
** IsValidCOMPARISONRESULT()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidCOMPARISONRESULT(COMPARISONRESULT cr)
{
   BOOL bResult;

   switch (cr)
   {
      case CR_FIRST_SMALLER:
      case CR_EQUAL:
      case CR_FIRST_LARGER:
         bResult = TRUE;
         break;

      default:
         WARNING_OUT((TEXT("IsValidCOMPARISONRESULT(): Unknown COMPARISONRESULT %d."),
                      cr));
         bResult = FALSE;
         break;
   }

   return(bResult);
}

#endif   /* DEBUG */

#endif   /* _SYNCENG_ || _LINKINFO_ */
