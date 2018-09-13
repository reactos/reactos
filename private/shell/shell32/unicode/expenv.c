/****************************************************************************/
/*                                                                          */
/*  expenv.c -                                                              */
/*                                                                          */
/*      Routines for expanding environment strings                          */
/*                                                                          */
/****************************************************************************/

#define UNICODE 1

#include "precomp.h"
#pragma  hdrstop

#ifndef WIN32
#define GetEnvironmentStrings GetDOSEnvironment
#endif

#ifdef UNICODE
#define tstrlen wcslen
#else
#define tstrlen lstrlen
#endif

//-------------------------------------------------------------------------
LPTSTR WINAPI FindEnvironmentString
    // Look up the given env var and pass back a pointer to it's value.
    // Returns NULL iff the variable isn't in the environment.
    (
    LPTSTR szEnvVar  // The environment variable to look for, null
                     // terminated.
    )
{
  LPTSTR lpEnv;        // The environment.
  LPTSTR lpEnvItem;        // The environment value.
  int nLen;

  nLen = tstrlen(szEnvVar);
  for (lpEnv=GetEnvironmentStrings(); *lpEnv; lpEnv+=tstrlen(lpEnv)+1)
    {
      /* Check if the two strings start the same
       */
      if (!StrCmpNI(lpEnv, szEnvVar, nLen))
        {
          /* Put the pointer on the NULL if no '='
           * then return if the lengths are equal
           */
          if (!(lpEnvItem=StrChr(lpEnv, TEXT('='))))
              lpEnvItem = lpEnv + tstrlen(lpEnv);
          if (lpEnvItem-lpEnv == nLen)
              return(*lpEnvItem ? lpEnvItem+1 : lpEnvItem);
        }
    }

  // Couldn't find it - just return NULL.
  return NULL;
}

//-------------------------------------------------------------------------
// The given string is parsed and all environment variables
// are expanded. If the expansion doesn't over fill the buffer
// then the length of the new string will be returned in the
// hiword and TRUE in the low word.  If the expansion would over
// fill the buffer then the original string is left unexpanded,
// the original length in the high word and FALSE in the low word.
// The length of the string is in bytes and excludes the terminating
// NULL.
//
// NOTE 1: This function must now handle environment variables in Quotes
//
// NOTE 2: There is no need for this API since NT has the equivalent APIs such
//       as ExpandEnvironmentStrings. But must keep it since it is a public
//       API in Win3.1.
//       Instead of doing all the work here, just call ExpandEnvironmentStrings.
//-------------------------------------------------------------------------

DWORD  APIENTRY
DoEnvironmentSubstA(
   LPSTR sz,    // The input string.
   UINT cchSz)  // The limit of characters in the input string inc null.
{
   LPSTR ExpandedSz;
   DWORD cchExpandedSz;

   ExpandedSz = (LPSTR)LocalAlloc(LPTR, cchSz);
   if (!ExpandedSz) {
       goto ExitFalse;
   }

   cchExpandedSz = ExpandEnvironmentStringsA(sz, ExpandedSz, cchSz);
   if (cchExpandedSz > (DWORD)cchSz) {
      goto ExitFree;
   }

   lstrcpyA(sz, ExpandedSz);
   LocalFree(ExpandedSz);

   return(MAKELONG(lstrlenA(sz),TRUE));

ExitFree:

   LocalFree(ExpandedSz);

ExitFalse:

   return(MAKELONG(cchSz,FALSE));

}

DWORD  APIENTRY
DoEnvironmentSubstW(
   LPWSTR sz,    // The input string.
   UINT cchSz    // The limit of characters in the input string inc null.
    )
{
   LPWSTR ExpandedSz;
   DWORD cchExpandedSz;

   if (!(ExpandedSz = (LPWSTR)LocalAlloc(LPTR, cchSz * sizeof(WCHAR)))) {
      goto ExitFalse;
   }

   cchExpandedSz = ExpandEnvironmentStringsW(sz, ExpandedSz, cchSz);
   if (cchExpandedSz > (DWORD)cchSz) {
      goto ExitFree;
   }

   wcscpy(sz, ExpandedSz);

   LocalFree(ExpandedSz);

   return(MAKELONG(wcslen(sz),TRUE));

ExitFree:

   LocalFree(ExpandedSz);

ExitFalse:

   return(MAKELONG(cchSz,FALSE));

}
