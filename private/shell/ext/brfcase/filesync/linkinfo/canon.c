/*
 * canon.c - Canonical path manipulation module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL GetCNRInfoForDevice(LPTSTR, LPTSTR, PDWORD, PDWORD);
PRIVATE_CODE BOOL GetDrivePathInfo(LPTSTR, PDWORD, LPTSTR, LPTSTR *);
PRIVATE_CODE BOOL GetRemotePathInfo(LPTSTR, PDWORD, LPTSTR, LPTSTR *);
PRIVATE_CODE void CanonicalizeTrailingSlash(LPTSTR);

#ifdef DEBUG

PRIVATE_CODE BOOL CheckFullPathInfo(LPCTSTR, PDWORD, LPCTSTR, LPCTSTR *);

#endif


/*
** GetCNRInfoForDevice()
**
**
**
** Arguments:
**
** Returns:       BOOL
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetCNRInfoForDevice(LPTSTR pszDeviceName, LPTSTR pszNameBuf,
                                      PDWORD pdwcbLen, PDWORD pdwOutFlags)
{
   DWORD dwNetResult;
   BOOL bResult;
   /* "X:" + null terminator */
   TCHAR rgchDrive[2 + 1];

   ASSERT(IS_VALID_STRING_PTR(pszDeviceName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbLen, DWORD));
   ASSERT(*pdwcbLen > 0);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszNameBuf, TCHAR, (UINT)(*pdwcbLen)));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));

   /* WNetGetConnection requires the device name to have no trailing
   ** backslash.
   */
   MyLStrCpyN(rgchDrive, pszDeviceName, ARRAYSIZE(rgchDrive));
   dwNetResult = WNetGetConnection(rgchDrive, pszNameBuf, pdwcbLen);

   switch (dwNetResult)
   {
      case NO_ERROR:
         *pdwOutFlags = GCPI_OFL_REMOTE;
         bResult = TRUE;
         TRACE_OUT((TEXT("GetCNRInfoForDevice(): %s is redirected to net resource \"%s\"."),
                    pszDeviceName,
                    pszNameBuf));
         break;

      case ERROR_NOT_CONNECTED:
         *pdwOutFlags = 0;
         bResult = TRUE;
         TRACE_OUT((TEXT("GetCNRInfoForDevice(): %s is not redirected."),
                    pszDeviceName));
         break;

      default:
         WARNING_OUT((TEXT("GetCNRInfoForDevice(): WNetGetConnection() on %s returned %lu."),
                      pszDeviceName,
                      dwNetResult));
         bResult = FALSE;
         break;
   }

   ASSERT(! bResult ||
          FLAGS_ARE_VALID(*pdwOutFlags, ALL_GCPI_OFLAGS) &&
          (IS_FLAG_CLEAR(*pdwOutFlags, GCPI_OFL_REMOTE) ||
           IsValidCNRName(pszNameBuf)));

   return(bResult);
}


/*
** GetDrivePathInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetDrivePathInfo(LPTSTR pszDrivePath, PDWORD pdwOutFlags,
                                   LPTSTR pszNetResourceNameBuf,
                                   LPTSTR *ppszRootPathSuffix)
{
   BOOL bResult;
   /* "X:\" + null terminator. */
   TCHAR rgchDriveRootPath[3 + 1];

   ASSERT(IsDrivePath(pszDrivePath));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszNetResourceNameBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(ppszRootPathSuffix, LPTSTR));

   ASSERT(lstrlen(pszDrivePath) >= 3);

   *pdwOutFlags = 0;

   MyLStrCpyN(rgchDriveRootPath, pszDrivePath, ARRAYSIZE(rgchDriveRootPath));

   ASSERT(IsDriveRootPath(rgchDriveRootPath));

   /* Do we need to get the CNR name for this drive path? */

   if (GetDriveType(rgchDriveRootPath) != DRIVE_REMOTE)
      /* No. */
      bResult = TRUE;
   else
   {
      DWORD dwcbBufLen = MAX_PATH_LEN;

      /* Yes. */

      bResult = GetCNRInfoForDevice(rgchDriveRootPath, pszNetResourceNameBuf,
                                    &dwcbBufLen, pdwOutFlags);
   }

   *ppszRootPathSuffix = pszDrivePath + 3;

   ASSERT(! bResult ||
          CheckFullPathInfo(pszDrivePath, pdwOutFlags, pszNetResourceNameBuf,
                            ppszRootPathSuffix));

   return(bResult);
}


/*
** GetRemotePathInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetRemotePathInfo(LPTSTR pszRemotePath, PDWORD pdwOutFlags,
                                    LPTSTR pszNetResourceNameBuf,
                                    LPTSTR *ppszRootPathSuffix)
{
   BOOL bResult;

   ASSERT(IsFullPath(pszRemotePath));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszNetResourceNameBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(ppszRootPathSuffix, LPTSTR));

   /* Is this a "\\server\share" name? */

   bResult = IsUNCPath(pszRemotePath);

   if (bResult)
   {
      LPTSTR psz;

      *pdwOutFlags = 0;

      /*
       * Yes.  Skip two leading slashes and look for end of \\server\share
       * specification.
       */

      /* Assume (as above) that a slash cannot be a DBCS lead byte. */

      for (psz = pszRemotePath + 2; ! IS_SLASH(*psz); psz = CharNext(psz))
         ASSERT(*psz);

      ASSERT(IS_SLASH(*psz));

      /*
       * Found first slash after double slash.  Find end of string or next
       * slash as end of root specification.
       */

      for (psz = CharNext(psz); *psz; psz = CharNext(psz))
      {
         if (IS_SLASH(*psz))
            break;
      }

      ASSERT(psz >= pszRemotePath);

      /* Add trailing slash for UNC root path. */

      if (! *psz)
      {
         *psz = SLASH;
         *(psz + 1) = TEXT('\0');
      }

      *ppszRootPathSuffix = (LPTSTR)psz + 1;

      ASSERT(! IS_SLASH(**ppszRootPathSuffix));

      /* (+ 1) for null terminator. */

      MyLStrCpyN(pszNetResourceNameBuf, pszRemotePath, (int)(psz - pszRemotePath + 1));

      CharUpper(pszNetResourceNameBuf);

      SET_FLAG(*pdwOutFlags, GCPI_OFL_REMOTE);
      bResult = TRUE;
   }
   else
      /* Not a UNC path. */
      SetLastError(ERROR_BAD_PATHNAME);

   ASSERT(! bResult ||
          CheckFullPathInfo(pszRemotePath, pdwOutFlags, pszNetResourceNameBuf,
                            ppszRootPathSuffix));

   return(bResult);
}


/*
** CanonicalizeTrailingSlash()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void CanonicalizeTrailingSlash(LPTSTR pszRootPathSuffix)
{
   LPTSTR pszLast;

   ASSERT(IS_VALID_STRING_PTR(pszRootPathSuffix, STR));

   ASSERT(! IS_SLASH(*pszRootPathSuffix));

   /* No path suffix should end in a slash. */

   pszLast = CharPrev(pszRootPathSuffix,
                      pszRootPathSuffix + lstrlen(pszRootPathSuffix));

   if (IS_SLASH(*pszLast))
      *pszLast = TEXT('\0');

   ASSERT(IsValidPathSuffix(pszRootPathSuffix));

   return;
}


#ifdef DEBUG

/*
** CheckFullPathInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CheckFullPathInfo(LPCTSTR pcszFullPath,
                                    PDWORD pdwOutFlags,
                                    LPCTSTR pcszNetResourceName,
                                    LPCTSTR *ppcszRootPathSuffix)
{
   return(EVAL(IsFullPath(pcszFullPath)) &&
          FLAGS_ARE_VALID(*pdwOutFlags, ALL_GCPI_OFLAGS) &&
          (IS_FLAG_CLEAR(*pdwOutFlags, GCPI_OFL_REMOTE) ||
           (EVAL(IsValidCNRName(pcszNetResourceName)) &&
            EVAL(lstrlen(pcszNetResourceName) < MAX_PATH_LEN))) &&
          (IS_FLAG_SET(*pdwOutFlags, GCPI_OFL_REMOTE) ||
           EVAL(IsLocalDrivePath(pcszFullPath))) &&
          IS_VALID_STRING_PTR(*ppcszRootPathSuffix, CSTR) &&
          EVAL(IsStringContained(pcszFullPath, *ppcszRootPathSuffix)));
}

#endif


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc LINKINFOAPI

@func BOOL | GetCanonicalPathInfo | Retrieves information about the canonical
form of a path.

@parm PCSTR | pcszPath | A pointer to the path string whose canonical form
information is to be retrieved.

@parm PSTR | pszCanonicalBuf | A pointer to a buffer to be filled in with the
full canonical form of the path.  This buffer must be at least MAX_PATH_LEN
bytes long.

@parm PDWORD | pdwOutFlags | A pointer to a DWORD bit mask of flags to be
filled in with flags from the <t GETCANONICALPATHINFOOUTFLAGS> enumeration.

@parm PSTR | pszNetResourceNameBuf | A pointer to a buffer to be filled in with
the name of the net resource parent of the path.  This buffer must be at least
MAX_PATH_LEN bytes long.  This buffer is only filled in if GCPI_OFL_REMOTE is
set in *pdwOutFlags.

@parm PSTR * | ppszRootPathSuffix | A pointer to a PSTR to be filled in with a
pointer to the file system root path suffix, not including the leading slash,
of the canonical path in pszCanonicalBuf's buffer.

@rdesc If the function completed successfully, TRUE is returned.  Otherwise,
FALSE is returned.  The reason for failure may be determined by calling
GetLastError().

******************************************************************************/

LINKINFOAPI BOOL WINAPI GetCanonicalPathInfo(LPCTSTR pcszPath,
                                             LPTSTR pszCanonicalBuf,
                                             PDWORD pdwOutFlags,
                                             LPTSTR pszNetResourceNameBuf,
                                             LPTSTR *ppszRootPathSuffix)
{
   BOOL bResult;
   LPTSTR pszFileName;
   DWORD dwPathLen;

   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszCanonicalBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));
   ASSERT(IS_VALID_WRITE_PTR(ppszRootPathSuffix, LPTSTR));

   dwPathLen = GetFullPathName(pcszPath, MAX_PATH_LEN, pszCanonicalBuf,
                               &pszFileName);

   if (dwPathLen > 0 && dwPathLen < MAX_PATH_LEN)
   {
      /*
       * Assume that GetFullPathName() changed all back slashes ('/') to
       * forward slashes ('\\').
       */

      ASSERT(! MyStrChr(pszCanonicalBuf, TEXT('/'), NULL));

      if (IsDrivePath(pszCanonicalBuf))
         bResult = GetDrivePathInfo(pszCanonicalBuf, pdwOutFlags,
                                    pszNetResourceNameBuf,
                                    ppszRootPathSuffix);
      else
         bResult = GetRemotePathInfo(pszCanonicalBuf, pdwOutFlags,
                                     pszNetResourceNameBuf,
                                     ppszRootPathSuffix);

      if (bResult)
         CanonicalizeTrailingSlash(*ppszRootPathSuffix);
   }
   else
   {
      // BOGUS ASSERT:  We can also get here if the resulting full path
      // is bigger than MAX_PATH_LEN.
      // ASSERT(! dwPathLen);

      WARNING_OUT((TEXT("GetFullPathName() failed on path %s, returning %lu."),
                   pcszPath,
                   dwPathLen));

      bResult = FALSE;
   }

   ASSERT(! bResult ||
          (CheckFullPathInfo(pszCanonicalBuf, pdwOutFlags,
                             pszNetResourceNameBuf, ppszRootPathSuffix) &&
           IsValidPathSuffix(*ppszRootPathSuffix)));

   return(bResult);
}

#ifdef UNICODE
LINKINFOAPI BOOL WINAPI GetCanonicalPathInfoA(LPCSTR pcszPath,
                                             LPSTR pszCanonicalBuf,
                                             PDWORD pdwOutFlags,
                                             LPSTR pszNetResourceNameBuf,
                                             LPSTR *ppszRootPathSuffix)
{
    LPWSTR  pcszWidePath;
    UINT    cchPath;
    WCHAR   szWideCanonicalBuf[MAX_PATH];
    WCHAR   szWideNetResourceNameBuf[MAX_PATH];
    LPWSTR  pszWideRootPathSuffix;
    UINT_PTR chOffset;
    BOOL    fCanonical;

    cchPath = lstrlenA(pcszPath) + 1;

    pcszWidePath = (LPWSTR)_alloca(cchPath*SIZEOF(WCHAR));

    if ( MultiByteToWideChar( CP_ACP, 0,
                              pcszPath, cchPath,
                              pcszWidePath, cchPath) == 0)
    {
        return FALSE;
    }
    fCanonical = GetCanonicalPathInfo( pcszWidePath,
                                       szWideCanonicalBuf,
                                       pdwOutFlags,
                                       szWideNetResourceNameBuf,
                                       &pszWideRootPathSuffix );
    if ( fCanonical )
    {
        if ( WideCharToMultiByte( CP_ACP, 0,
                                  szWideCanonicalBuf, -1,
                                  pszCanonicalBuf, MAX_PATH,
                                  NULL, NULL ) == 0)
        {
            return FALSE;
        }
        if ( *pdwOutFlags & GCPI_OFL_REMOTE )
        {
            if ( WideCharToMultiByte( CP_ACP, 0,
                                      szWideNetResourceNameBuf, -1,
                                      pszNetResourceNameBuf, MAX_PATH,
                                      NULL, NULL ) == 0)
            {
                return FALSE;
            }
        }
        chOffset = pszWideRootPathSuffix - szWideCanonicalBuf;
        *ppszRootPathSuffix = pszCanonicalBuf;
        while ( chOffset-- )
        {
            *ppszRootPathSuffix = CharNextA(*ppszRootPathSuffix);
        }
    }

    return(fCanonical);
}
#endif
