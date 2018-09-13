/****************************************************************************/
/*                                                                          */
/*  MYCRT.C -                                                               */
/*                                                                          */
/*       My special Unicode workaround file for CRT functions calls         */
/*       from WIN32 Shell applications.                                     */
/*                                                                          */
/*       Created by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*                                                                          */
/*       This is a temporary fix and needs to be modified when Unicode      */
/*       is fully supported by CRT.                                         */
/*                                                                          */
/****************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <direct.h>

#ifdef UNICODE
#include <wchar.h>
#else
#include <string.h>
#endif


#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40

//*****************************************************************
//
//   MyAtoi and MyAtol
//
//   Purpose     : To convert from Unicode to ANSI string before
//                 calling CRT atoi and atol functions.
//
//*****************************************************************

INT MyAtoi (LPTSTR  string)
{
   CHAR   szAnsi [INT_SIZE_LENGTH];
   BOOL   fDefCharUsed;
#ifdef UNICODE
   BOOL   bDBCS;
   LCID   lcid = GetThreadLocale();

   bDBCS = ( (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE) ||
             (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_KOREAN)   ||
             (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_CHINESE)
            );

    if (bDBCS)
    {
        //fix mskkbug: #3871
        //When Control panel calling this function, the string to be converted
        //has extra charcters (not able to be converted integer value, such as 'a','b'.
        //In that case, memory allocation error causes. So we have to make sure of it.

        int  iCnt, rValue;
        LPBYTE   mptr;

        if( !(iCnt = WideCharToMultiByte (CP_ACP, 0, string, -1, szAnsi,
                                    INT_SIZE_LENGTH, NULL, &fDefCharUsed)) )
        {

            iCnt = WideCharToMultiByte (CP_ACP, 0, string, -1, NULL,
                                       0, NULL, &fDefCharUsed );

            mptr = (LPBYTE)LocalAlloc( LMEM_FIXED, iCnt );

            WideCharToMultiByte (CP_ACP, 0, string, -1, mptr,
                                       iCnt, NULL, &fDefCharUsed );
            rValue = atoi(mptr);
            LocalFree( (HLOCAL)mptr );
            return( rValue);
         }
         else
         {
             //on the safe side
             if( iCnt >= INT_SIZE_LENGTH ) szAnsi[INT_SIZE_LENGTH-1] = 0;
             return( atoi(szAnsi) );
         }
    }
    else
    {
        WideCharToMultiByte (CP_ACP, 0, string, INT_SIZE_LENGTH,
                             szAnsi, INT_SIZE_LENGTH, NULL, &fDefCharUsed);

        return (atoi (szAnsi));
    }
#else
   return (atoi (string));
#endif

} // end of MyAtoi()

LONG MyAtol (LPTSTR  string)
{
   CHAR   szAnsi [LONG_SIZE_LENGTH];
   BOOL   fDefCharUsed;
#ifdef UNICODE
   BOOL   bDBCS;
   LCID   lcid = GetThreadLocale();

   bDBCS = ( (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE) ||
             (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_KOREAN)   ||
             (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_CHINESE)
            );

   if (bDBCS)
   {
       int  iCnt;

       if( !(iCnt = WideCharToMultiByte (CP_ACP, 0, string, -1,
                            szAnsi, LONG_SIZE_LENGTH, NULL, &fDefCharUsed)) )
       {
          return FALSE;
       }

       //on the safe side
       if( iCnt >= LONG_SIZE_LENGTH )
           szAnsi[LONG_SIZE_LENGTH-1] = 0;

   }
   else
   {
       WideCharToMultiByte (CP_ACP, 0, string, LONG_SIZE_LENGTH,
                            szAnsi, LONG_SIZE_LENGTH, NULL, &fDefCharUsed);
   }

   return (atol (szAnsi));
#else
   return (atol (string));
#endif

} // end of MyAtol()

//*****************************************************************
//
//   MyItoa
//
//   Purpose     : To convert from ANSI to Unicode string after
//                 calling CRT itoa function.
//
//*****************************************************************

LPTSTR MyItoa (INT  value, LPTSTR  string, INT  radix)
{
   CHAR   szAnsi [INT_SIZE_LENGTH];

#ifdef UNICODE
   _itoa (value, szAnsi, radix);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                        string, INT_SIZE_LENGTH);
#else
   _itoa (value, string, radix);
#endif

   return (string);

} // end of MyItoa()

LPTSTR MyUltoa (unsigned long  value, LPTSTR  string, INT  radix)
{
   CHAR   szAnsi [LONG_SIZE_LENGTH];

#ifdef UNICODE
   _ultoa (value, szAnsi, radix);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                        string, LONG_SIZE_LENGTH);
#else
   _ultoa (value, string, radix);
#endif

   return (string);

} // end of MyUltoa()


//*****************************************************************
//
//   MySplitpath
//
//   Purpose     : To convert from ANSI to Unicode string before
//                 calling CRT strtok function.
//
//*****************************************************************

VOID MySplitpath (LPTSTR  path,
                  LPTSTR  drive,
                  LPTSTR  dir,
                  LPTSTR  fname,
                  LPTSTR  ext)
{
  register LPTSTR p;
  LPTSTR   last_slash = NULL, dot = NULL;
  WORD     len;

    /* we assume that the path argument has the following form, where any
     * or all of the components may be missing.
     *
     *      <drive><dir><fname><ext>
     *
     * and each of the components has the following expected form(s)
     *
     *  drive:
     *      0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
     *      ':'
     *  dir:
     *      0 to _MAX_DIR-1 characters in the form of an absolute path
     *      (leading '/' or '\') or relative path, the last of which, if
     *      any, must be a '/' or '\'.  E.g -
     *      absolute path:
     *          \top\next\last\     ; or
     *          /top/next/last/
     *      relative path:
     *          top\next\last\      ; or
     *          top/next/last/
     *      Mixed use of '/' and '\' within a path is also tolerated
     *  fname:
     *      0 to _MAX_FNAME-1 characters not including the '.' character
     *  ext:
     *      0 to _MAX_EXT-1 characters where, if any, the first must be a
     *      '.'
     *
     */

    /* extract drive letter and :, if any */

    if (*(path + _MAX_DRIVE - 2) == TEXT(':'))
    {
       if (drive)
       {
          _tcsncpy(drive, path, _MAX_DRIVE - 1);
          *(drive + _MAX_DRIVE-1) = TEXT('\0');
       }
       path += _MAX_DRIVE - 1;
    }
    else if (drive)
       *drive = TEXT('\0');

    /* extract path string, if any.  Path now points to the first character
     * of the path, if any, or the filename or extension, if no path was
     * specified.  Scan ahead for the last occurence, if any, of a '/' or
     * '\' path separator character.  If none is found, there is no path.
     * We will also note the last '.' character found, if any, to aid in
     * handling the extension.
     */

    for (last_slash = NULL, p = path; *p; p++)
    {
       if (*p == TEXT('/') || *p == TEXT('\\')) /* point to one beyond for later copy */
          last_slash = p + 1;
       else if (*p == TEXT('.'))
          dot = p;
    }

    if (last_slash)
    {
       /* found a path - copy up through last_slash or max. characters
        * allowed, whichever is smaller
        */

       if (dir)
       {
          len = (WORD)__min((WORD)(last_slash - path) / sizeof(TCHAR), _MAX_DIR - 1);
          _tcsncpy(dir, path, len);
          *(dir + len) = TEXT('\0');
       }
       path = last_slash;
    }
    else if (dir)  /* no path found */
       *dir = TEXT('\0');

    /* extract file name and extension, if any.  Path now points to the
     * first character of the file name, if any, or the extension if no
     * file name was given.  Dot points to the '.' beginning the extension,
     * if any.
     */

    if (dot && (dot >= path))
    {
       /* found the marker for an extension - copy the file name up to
        * the '.'.
        */

       if (fname)
       {
          len = (WORD)__min((WORD)(dot - path) / sizeof(TCHAR), _MAX_FNAME - 1);
          _tcsncpy(fname, path, len);
          *(fname + len) = TEXT('\0');
       }

       /* now we can get the extension - remember that p still points
        * to the terminating nul character of path.
        */

       if (ext)
       {
          len = (WORD)__min((WORD)(p - dot) / sizeof(TCHAR), _MAX_EXT - 1);
          _tcsncpy(ext, dot, len);
          *(ext + len) = TEXT('\0');
       }
    }
    else
    {
        /* found no extension, give empty extension and copy rest of
         * string into fname.
         */
        if (fname)
        {
           len = (WORD)__min((WORD)(p - path) / sizeof (TCHAR), _MAX_FNAME - 1);
           _tcsncpy(fname, path, len);
           *(fname + len) = TEXT('\0');
        }
        if (ext)
           *ext = TEXT('\0');
    }

} // end of MySplitpath()


LPTSTR SkipProgramName (LPTSTR lpCmdLine)
{
    LPTSTR  p = lpCmdLine;
    BOOL    bInQuotes = FALSE;

    //
    // Skip executable name
    //
    for (p; *p; p = CharNext(p))
    {
       if ((*p == TEXT(' ') || *p == TEXT('\t')) && !bInQuotes)
          break;

       if (*p == TEXT('\"'))
          bInQuotes = !bInQuotes;
    }

    while (*p == TEXT(' ') || *p == TEXT('\t'))
       p++;

    return (p);
}

