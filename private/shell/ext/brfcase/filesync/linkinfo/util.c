/*
 * util.c - Miscellaneous utility functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/****************************** Public Functions *****************************/


/*
** IsLocalDrivePath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsLocalDrivePath(LPCTSTR pcszFullPath)
{
   BOOL bResult;

   ASSERT(IsFullPath(pcszFullPath));

   bResult = IsDrivePath(pcszFullPath);

   if (bResult)
   {
      TCHAR rgchRootPath[DRIVE_ROOT_PATH_LEN];

      ASSERT(IsCharAlpha(*pcszFullPath));

      lstrcpy(rgchRootPath, TEXT("A:\\"));
      rgchRootPath[0] = *pcszFullPath;

      bResult = (GetDriveType(rgchRootPath) != DRIVE_REMOTE);
   }

   return(bResult);
}


/*
** IsUNCPath()
**
** Determines whether or not a path is in "\\server\share" UNC form.
**
** Arguments:     pcszPath - path to examine
**
** Returns:       TRUE if path is a UNC path.  FALSE if not.
**
** Side Effects:  none
**
** A UNC path is a string of the form two slashes, one or more non-slashes, one
** slash, one or more non-slashes
*/
PUBLIC_CODE BOOL IsUNCPath(LPCTSTR pcszFullPath)
{
   BOOL bResult = FALSE;

   ASSERT(IsFullPath(pcszFullPath));

   if (lstrlen(pcszFullPath) >= 5 &&
       IS_SLASH(pcszFullPath[0]) &&
       IS_SLASH(pcszFullPath[1]) &&
       ! IS_SLASH(pcszFullPath[2]))
   {
      LPCTSTR pcsz;

      for (pcsz = &(pcszFullPath[2]); *pcsz; pcsz = CharNext(pcsz))
      {
         if (IS_SLASH(*pcsz))
         {
            bResult = (*(pcsz + 1) &&
                       ! IS_SLASH(*(pcsz + 1)));

            break;
         }
      }
   }

   return(bResult);
}


/*
** DeleteLastDrivePathElement()
**
** Deletes the last path element from a drive path.
**
** Arguments:     pszDrivePath - drive path whose last element is to be deleted
**
** Returns:       TRUE if path element deleted.  FALSE if not, i.e., given path
**                is root path.
**
** Side Effects:  none
**
** Examples:
**
**    input path                    output path
**    ----------                    -----------
**    c:\                           c:\
**    c:\foo                        c:\
**    c:\foo\bar                    c:\foo
**    c:\foo\bar\                   c:\foo\bar
**
** N.b., this function does not perform any validity tests on the format of the
** input path string.
*/
PUBLIC_CODE BOOL DeleteLastDrivePathElement(LPTSTR pszDrivePath)
{
   BOOL bHackIt;
   LPTSTR pszEndOfDriveSpec;

   ASSERT(IsDrivePath(pszDrivePath));

   pszEndOfDriveSpec = pszDrivePath + 3;

   /* Is this a a root path? */

   bHackIt = *pszEndOfDriveSpec;

   if (bHackIt)
      DeleteLastPathElement(pszEndOfDriveSpec);

   ASSERT(IsDrivePath(pszDrivePath));

   return(bHackIt);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsContained()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsContained(PCVOID pcvJar, UINT ucbJarLen, PCVOID pcvJelly,
                        UINT ucbJellyLen)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_READ_BUFFER_PTR(pcvJar, CVOID, ucbJarLen));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcvJelly, CVOID, ucbJellyLen));

   if (EVAL(pcvJelly >= pcvJar))
   {
      UINT ucbJellyOffset;

      ucbJellyOffset = (UINT)((PCBYTE)pcvJelly - (PCBYTE)pcvJar);

      if (EVAL(ucbJellyOffset < ucbJarLen) &&
          EVAL(ucbJellyLen < ucbJarLen - ucbJellyOffset))
         bResult = TRUE;
   }

   return(bResult);
}


/*
** IsValidCNRName()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidCNRName(LPCTSTR pcszCNRName)
{
   BOOL bResult;

   /* Any valid string < MAX_PATH_LEN bytes long is a valid CNR name. */

   bResult = (IS_VALID_STRING_PTR(pcszCNRName, CSTR) &&
              EVAL(lstrlen(pcszCNRName) < MAX_PATH_LEN));

#ifdef DEBUG

   /*
    * RIP if a CNR name ends in a slash 
    */

   if (bResult)
   {
      if (IsUNCPath(pcszCNRName))
      {
         ASSERT(! IS_SLASH(*(CharPrev(pcszCNRName, pcszCNRName + lstrlen(pcszCNRName)))));
      }
   }

#endif

   return(bResult);
}

#endif


#ifdef DEBUG

/*
** IsDriveRootPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsDriveRootPath(LPCTSTR pcszPath)
{
   return(IsDrivePath(pcszPath) &&
          lstrlen(pcszPath) == 3);
}

#endif
