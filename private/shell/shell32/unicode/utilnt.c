#include "precomp.h"
#pragma  hdrstop

BOOL APIENTRY
IsStringInList(
   LPWSTR lpS,
   LPWSTR lpList
   )
{
   while (*lpList) {
      if (!_wcsicmp(lpS,lpList)) {
         return(TRUE);
      }
      lpList += wcslen(lpList) + 1;
   }
   return FALSE;
}

LPWSTR APIENTRY
SheRemoveQuotesW(
   LPWSTR sz)
{
   LPWSTR lpT;

   if (WCHAR_QUOTE == *sz) {
      for (lpT = sz+1; *lpT && WCHAR_QUOTE != *lpT; lpT++) {
         *(lpT-1) = *lpT;
      }
      if (WCHAR_QUOTE == *lpT) {
         *(lpT-1) = WCHAR_NULL;
      }
   }
   return(sz);
}

LPSTR APIENTRY
SheRemoveQuotesA(
   LPSTR sz)
{
   LPSTR lpT;

   if (CHAR_QUOTE == *sz) {
      for (lpT = sz+1; *lpT && CHAR_QUOTE != *lpT; lpT++) {
         *(lpT-1) = *lpT;
#if (defined(DBCS) || defined(FE_SB))
         if (IsDBCSLeadByte(*lpT)) {
         lpT++;
            *(lpT-1) = *lpT;
       }
#endif
      }
      if (CHAR_QUOTE == *lpT) {
         *(lpT-1) = CHAR_NULL;
      }
   }
   return(sz);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     SheShortenPathA
//
// Synopsis: Thunk to ShortenPathW
//
/////////////////////////////////////////////////////////////////////

BOOL APIENTRY
SheShortenPathA(LPSTR pPath, BOOL bShorten)
{
   WCHAR pPathW[MAX_PATH];
   BOOL bRetVal;

   MultiByteToWideChar(CP_ACP, 0, pPath, -1, pPathW, MAX_PATH);

   bRetVal = SheShortenPathW(pPathW, bShorten);

   WideCharToMultiByte(CP_ACP, 0, pPathW, -1, pPath, MAX_PATH,
      NULL, NULL);

   return bRetVal;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     SheShortenPath
//
// Synopsis: Takes a pathname and converts all dirs to shortnames/longnames
//
// INOUT:    lpszPath  -- Path to shorten/lengthen (May be in DQUOTES)
//                        Must not be a commandline!
//
//           bShorten  -- T=shorten, F=Lengthen
//
// Return:   BOOL  T=Converted,
//                 F=ran out of space, buffer left alone
//
//
// Assumes:  lpszPath takes the form {"}?:\{f\}*f{"}  or {"}\\f\f\{f\}*f{"}
//           COUNTOF pSrc buffer >= MAXPATHELN
//
// Effects:  Strips quotes out of pPath, if any
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

BOOL APIENTRY
SheShortenPathW(LPWSTR pPath, BOOL bShorten)
{
   WCHAR szDest[MAX_PATH];
   LPWSTR pSrcNextSpec, pReplaceSpec;
   LPWSTR pDest, pNewName, p;
   LPWSTR pSrc;
   DWORD cchPathOffset;
   HANDLE hFind;
   WIN32_FIND_DATA FindData;

   UINT i;
   INT nSpaceLeft = MAX_PATH-1;

   pSrc = pPath;

   //
   // Eliminate d-quotes
   //
   for (p = pDest =  pSrc; *p; p++, pDest++) {
      if (WCHAR_QUOTE == *p)
         p++;

      *pDest = *p;
   }

   *pDest = WCHAR_NULL;

   //
   // Strip out leading spaces
   //
   while (WCHAR_SPACE == *pSrc)
      pSrc++;

   //
   // Initialize pNewName so it is calculated once.
   //
   pNewName = bShorten ?
      FindData.cAlternateFileName :
      FindData.cFileName;

   //
   // Skip past \\foo\bar or <drive>:
   //
   pDest = szDest;
   pSrcNextSpec = pSrc;

   // reuse shell32 internal api that calculates path
   // offset.  cchPathOffset will be the offset that when
   // added to the pointer will result in a pointer to the
   // backslash before the first part of the path
   cchPathOffset = SheGetPathOffsetW(pSrc);

   // Check to see if it's valid.  If pSrc is not of the \\foo\bar
   // or <drive>: form we just do nothing
   if (0xFFFFFFFF == cchPathOffset) {
      return TRUE;
   }

   // cchPathOffset will then always be atleast 1 and is the
   // number of characters - 1 that we want to copy (that is, if 0
   // was permissible, it would denote 1 character).
   do {

      *pDest++ = *pSrcNextSpec++;

      if (!--nSpaceLeft)
         return FALSE;

   } while (cchPathOffset--);

   //
   // At this point, we have just the filenames that we can shorten:
   // \\foo\bar\it\is\here ->  it\is\here
   // c:\angry\lions       ->  angry\lions
   //

   while(pSrcNextSpec) {

      //
      // pReplaceSpec holds the current spec we need to replace.
      // By default, if we can't find the altname, then just use this.
      //

      pReplaceSpec = pSrcNextSpec;

      //
      // Search for trailing "\"
      // pSrcNextSpec will point to the next spec to fix (*pSrcNextSpec=NULL if done)
      //
      for(;*pSrcNextSpec && WCHAR_BSLASH != *pSrcNextSpec; pSrcNextSpec++)
         ;


      if (*pSrcNextSpec) {

         //
         // If there is more, then pSrcNextSpec should point to it.
         // Also delimit this spec.
         //
         *pSrcNextSpec = WCHAR_NULL;

      } else {

         pSrcNextSpec = NULL;
      }

      hFind = FindFirstFile(pSrc, &FindData);

      //
      // We could exit as soon as this FindFirstFileFails,
      // but there's the special case of having execute
      // without read permission.  This would fail since the lfn
      // is valid for lfn apps.
      //


      if (INVALID_HANDLE_VALUE != hFind) {

         FindClose(hFind);

         if (pNewName[0]) {

            //
            // We have found an altname.
            // Use it instead.
            //
            pReplaceSpec = pNewName;
         }
      }

      i = wcslen(pReplaceSpec);
      nSpaceLeft -= i;

      if (nSpaceLeft <= 0)
         return FALSE;

      wcscpy(pDest, pReplaceSpec);
      pDest+=i;

      //
      // Now replace the WCHAR_NULL with a slash if necessary
      //
      if (pSrcNextSpec) {
         *pSrcNextSpec++ = WCHAR_BSLASH;

         //
         // Also add backslash to dest
         //
         *pDest++ = WCHAR_BSLASH;
         nSpaceLeft--;
      }
   }

   wcscpy(pPath, szDest);

   return TRUE;
}


/*
 * Reads the list of program strings from win.ini
 */
LPWSTR GetPrograms()
{
   static LPWSTR lpPrograms = WCHAR_NULL;
   LPWSTR lpT,lpS;

   if (lpPrograms) {
      return lpPrograms;
   }

   if (!(lpPrograms = (LPWSTR)LocalAlloc(LPTR, (MAX_PATH+1) * sizeof(WCHAR)))) {
      return(NULL);
   } else {

      GetProfileString(L"windows",L"programs",WSTR_BLANK,lpPrograms,MAX_PATH);

      for (lpS = lpT = lpPrograms; *lpT; lpT++) {
         if (*lpT == WCHAR_SPACE) {
            while (*lpT == WCHAR_SPACE) {
               lpT++;
            }
            lpT--;
            *lpS++ = 0;
         } else {
            *lpS++ = *lpT;
         }
      }

      *lpS++ = WCHAR_NULL;
      *lpS++ = WCHAR_NULL;
      return(lpPrograms);
   }
}

/*
 * Determines if an extension is a program
 */

BOOL IsProgram(LPWSTR lpExt)
{
   LPWSTR lpPrograms = GetPrograms();
   return(IsStringInList(lpExt,lpPrograms));
}


/* finds a file along the path. Returns the error code or 0 if success.
 */

WORD
SearchForFile(
   LPCWSTR lpDir,
   LPWSTR lpFile,
   LPWSTR lpFullPath,
   DWORD cchFullPath,
   LPWSTR lpExt)
{
   LPWSTR lpT;
   LPWSTR lpD;
   LPWSTR lpExts;
   WCHAR szFile[MAX_PATH+1];
   DWORD cchPath;

   if (*lpFile == WCHAR_QUOTE) {
      lpFile = SheRemoveQuotes(lpFile);
   }
   if (NULL != (lpT=StrRChrW(lpFile, NULL, WCHAR_BSLASH))) {
      ++lpT;
   } else if (NULL != (lpT=StrRChrW(lpFile, NULL, WCHAR_COLON))) {
      ++lpT;
   } else {
      lpT = lpFile;
   }

   if (NULL != (lpT=StrRChrW(lpT, NULL, WCHAR_DOT))) {
      int n;

      n = wcslen(lpT + 1);
      StrCpyN(lpExt, lpT+1, n < 64 ? n+1 : 65);  // max extension
   } else {
      *lpExt = WCHAR_NULL;
   }

   // If there's no extension then just use programs list don't
   // try searc   hing for the app sans extension. This fixes the bogus
   // file.run stuff.
   if (!*lpExt) {
      goto UseDefExts;
   }

   //
   // NOTE: Do NOT call CharUpper for any of the strings in this routine.
   //       It will cause problems for the Turkish locale.
   //

   cchPath = SearchPath(lpDir, lpFile, NULL, cchFullPath, lpFullPath, &lpT);

   if (!cchPath) {
      cchPath = SearchPath(NULL, lpFile, NULL, cchFullPath, lpFullPath, &lpT);
   }

   if (cchPath >= cchFullPath) {
      return(SE_ERR_OOM);
   }

   if (cchPath == 0) {
      return(SE_ERR_FNF);
   }

   CheckEscapes(lpFullPath, cchFullPath);
   return 0;

UseDefExts:
   wcscpy(szFile,lpFile);
   lpFile = szFile;

   wcscat(lpFile,WSTR_DOT);
   lpD = lpFile + wcslen(lpFile);

   if (NULL != (lpExts = GetPrograms())) {
       // We want to pass through the loop twice checking whether the
       // file is in lpDir first, and then if it's in the sysdirs, via SearchPath(NULL, ...)
       // Add some state and extend the while loop
       LPCWSTR lpTempDir = lpDir;
       LPWSTR lpTempExts = lpExts;
       BOOL bCheckedSysDirs = FALSE;

       while (*lpTempExts || !bCheckedSysDirs) {

           // After the first pass, lpTempExts will be NULL
           // Reset it and loop through again with lpTempDir = NULL so that
           // SearchPath looks at the system dirs

           if (!*lpTempExts) {
              bCheckedSysDirs = TRUE;
              lpTempExts = lpExts;
              lpTempDir = NULL;
           }

           wcscpy(lpD,lpTempExts);
           wcscpy(lpExt,lpTempExts);

           cchPath = SearchPath(lpTempDir, lpFile, NULL, cchFullPath, lpFullPath, &lpT);
           if (cchPath >= cchFullPath) {
              return(SE_ERR_OOM);
           }

           if (cchPath != 0) {
              CheckEscapes(lpFullPath, cchFullPath);
              return 0;
           }

           lpTempExts += wcslen(lpTempExts)+1;
       }
   }
   return(SE_ERR_FNF);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     QualifyAppName
//
// Synopsis: Creates a fully qualified path to the app in a commandline
//
// INC       lpCmdLine     Command line to qualify
//                         (Must have DQuotes if has spaces)
// OUT       lpImage       Fully qualified result
// OUT       ppArgs        Pointer to args in lpCmdLine, _incl_ leading space
//                         OPTIONAL
//
// Return:   DWORD length of path, 0 = fail
//
//
// Assumes:  len of executable in lpCmdLine is < MAX_PATH
//           len of exts are < 64
//
// Effects:
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

DWORD
QualifyAppName(
   IN LPCWSTR lpCmdLine,
   OUT LPWSTR lpImage,
   OPTIONAL OUT LPCWSTR* ppArgs)
{
   LPWSTR lpAppName;
   BOOL bAppNameInQuotes = FALSE;
   DWORD cch = 0;

   lpAppName = lpImage;

   // sanity check
   if (!lpCmdLine) {
      return(0);
   }

   while (*lpCmdLine &&
         (*lpCmdLine != WCHAR_SPACE || bAppNameInQuotes)) {

      if (*lpCmdLine == WCHAR_QUOTE) {
         bAppNameInQuotes = !bAppNameInQuotes;
         lpCmdLine++;
         continue;
      }

      *lpAppName++ = *lpCmdLine++;
      cch++;
   }

   *lpAppName = WCHAR_NULL;

   //
   // Save the pointer to the argument list
   //
   if (ppArgs) {
      *ppArgs = lpCmdLine;
   }

   if (SheGetPathOffsetW(lpImage) == -1) {
      WCHAR szTemp[MAX_PATH];

      lstrcpy((LPWSTR)szTemp, lpImage);

      if (StrChrW(lpImage, WCHAR_DOT)) {
          LPWSTR lpFileName;

          return(SearchPath(NULL, szTemp, NULL, MAX_PATH, lpImage, &lpFileName));
      }
      else {
         WCHAR  szExt[65];

         *lpImage = WCHAR_NULL;
         if (SearchForFile(NULL, (LPWSTR)szTemp, lpImage, MAX_PATH, szExt)) {
            return(0);
         }

         return(lstrlen(lpImage));
      }
   }

   return(cch);
}


BOOL
SheConvertPathW(
    LPWSTR lpCmdLine,
    LPWSTR lpFile,
    UINT   cchCmdBuf)
/*++

Routine Description:

   Takes a command line and file and shortens both if the app in the
   command line is dos/wow.

Returns: BOOL T=converted

Arguments:

    INOUT     lpCmdLine  Command line to test
                         exe must be in DQuotes if it has spaces,
                         on return, will have DQuotes if necessary
    INOUT     lpFile     Fully qualified file to shorten
                         May be in DQuotes, but on return will not
                         have DQuotes (since single file)

    IN        cchCmdBuf  Size of buffer in characters

Return Value:

    VOID, but lpFile shortened (in place) if lpCmdLine is dos/wow.

    There are pathalogoical "lfns" (Single unicode chars) that can
    actually get longer when they are shortened.  In this case, we
    won't AV, but we will truncate the parms!

    // Qualify path assumes that the second parm is a buffer of
    // size atleast MAX_PATH, which is nicely equivalent to MAX_PATH
    // needs cleanup!

--*/

{
    LPWSTR lpszFullPath;
    LONG lBinaryType;
    BOOL bInQuote = FALSE;
    LPWSTR lpArgs;
    UINT cchNewLen;
    BOOL bRetVal = FALSE;

    lpszFullPath = (LPWSTR) LocalAlloc(LMEM_FIXED,
                                       cchCmdBuf*sizeof(*lpCmdLine));

    if (!lpszFullPath)
       return bRetVal;

    //
    // We must do the swap here since we need to copy the
    // parms back to lpCmdLine.
    //
    lstrcpy(lpszFullPath, lpCmdLine);

    if (QualifyAppName(lpszFullPath, lpCmdLine, &lpArgs)) {

        if (!GetBinaryType(lpCmdLine, &lBinaryType) ||
            lBinaryType == SCS_DOS_BINARY ||
            lBinaryType == SCS_WOW_BINARY) {

            SheShortenPath(lpCmdLine, TRUE);

            if (lpFile) {
                SheShortenPath(lpFile, TRUE);
            }
            bRetVal = TRUE;
        }

        //
        // Must readd quotes
        //
        CheckEscapes(lpCmdLine, cchCmdBuf);

        cchNewLen = lstrlen(lpCmdLine);
        StrNCpy(lpCmdLine+cchNewLen, lpArgs, cchCmdBuf-cchNewLen);
    } else {
        //
        // QualifyAppName failed, restore the command line back
        // to the original state.
        //

        lstrcpy(lpCmdLine, lpszFullPath);
    }

    LocalFree((HLOCAL)lpszFullPath);

    return bRetVal;
}
