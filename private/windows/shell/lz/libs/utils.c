/*
** utils.c - Miscellaneous utility routines used in compression / expansion
**           modules.  Theoretically these functions are DBCS-enabled.
**
** Author:  DavidDi
*/


// Headers
///////////

#ifndef LZA_DLL
   #include <ctype.h>
   #include <string.h>
#endif

#include "common.h"


/*
** char ARG_PTR *ExtractFileName(char ARG_PTR pszPathName);
**
** Find the file name in a fully specified path name.
**
** Arguments:  pszPathName - path string from which to extract file name
**
** Returns:    char ARG_PTR * - Pointer to file name in pszPathName.
**
** Globals:    none
*/
LPWSTR
ExtractFileNameW(
    LPWSTR pszPathName)
{
   LPWSTR pszLastComponent, psz;

   for (pszLastComponent = psz = pszPathName; *psz != L'\0'; psz++)
   {
      if (*psz == L'\\' || *psz == L':')
         pszLastComponent = psz + 1;
   }

   return(pszLastComponent);
}


CHAR ARG_PTR *ExtractFileName(CHAR ARG_PTR *pszPathName)
{
   CHAR ARG_PTR *pszLastComponent, ARG_PTR *psz;

   for (pszLastComponent = psz = pszPathName; *psz != '\0'; psz = CharNext(psz))
   {
      if (! IsDBCSLeadByte(*psz) && (ISSLASH(*psz) || *psz == COLON))
         pszLastComponent = CharNext(psz);
   }

   return(pszLastComponent);
}


/*
** char ARG_PTR *ExtractExtension(char ARG_PTR *pszFileName);
**
** Find the extension of a file name.
**
** Arguments:  pszFileName - file name to examine
**
** Returns:    char ARG_PTR * - Pointer to file name extension if one exists.
**                              NULL if the file name doesn't include an
**                              extension.
**
** Globals:    none
*/
LPWSTR
ExtractExtensionW(
    LPWSTR pszFileName)
{
   WCHAR *psz;

   // Make sure we have an isolated file name.
   psz = ExtractFileNameW(pszFileName);

   while (*psz != L'\0' && *psz != L'.')
      psz++;

   if (*psz == L'.')
      return(psz + 1);
   else
      return(NULL);
}


CHAR ARG_PTR *ExtractExtension(CHAR ARG_PTR *pszFileName)
{
   CHAR ARG_PTR *psz;

   // Make sure we have an isolated file name.
   psz = ExtractFileName(pszFileName);

   while (IsDBCSLeadByte(*psz) || (*psz != '\0' && *psz != PERIOD))
      psz = CharNext(psz);

   if (*psz == PERIOD)
      return(psz + 1);
   else
      return(NULL);
}


/*
** void MakePathName(char ARG_PTR *pszPath, char ARG_PTR *pszFileName);
**
** Append a filename to a path string.
**
** Arguments:  pszPath     - path string to which pszFileName will be appended
**             pszFileName - file name to append
**
** Returns:    void
**
** Globals:    none
*/
VOID MakePathName(CHAR ARG_PTR *pszPath, CHAR ARG_PTR *pszFileName)
{
   CHAR chLastPathChar;

   // Make sure we have an isolated file name.
   pszFileName = ExtractFileName(pszFileName);

   // Dont append to a NULL string or a single ".".
   if (*pszFileName != '\0' &&
       ! (! IsDBCSLeadByte(pszFileName[0]) && pszFileName[0] == PERIOD &&
          ! IsDBCSLeadByte(pszFileName[1]) && pszFileName[1] == '\0'))
   {
      chLastPathChar = *CharPrev(pszPath, pszPath + STRLEN(pszPath));

      //NTRAID:MSKKBUG #3411    Nov.09.93   v-hajimy
      if (! ISSLASH(chLastPathChar) && chLastPathChar != COLON)
         STRCAT(pszPath, SEP_STR);

      STRCAT(pszPath, pszFileName);
   }
}


/*
** char MakeCompressedName(char ARG_PTR *pszFileName);
**
** Make a file name into the corresponding compressed file name.
**
** Arguments:  pszOriginalName - file name to convert to compressed file name
**
** Returns:    char - Uncompressed file name extension character that was
**                    replaced.  '\0' if no character needed to be replaced.
**
** Globals:    none
**
** N.b., assumes pszFileName's buffer is long enough to hold an extra two
** characters ("._").
**
** For DBCS filenames, we know we can have at most one DBCS character in the
** extension.  So instead of just blindly replacing the last character of a
** three-byte extension with an underscore, we replace the last single-byte
** character with an underscore.
*/

#define chEXTENSION_CHARW      L'_'
#define pszEXTENSION_STRW      L"_"
#define pszNULL_EXTENSIONW     L"._"

WCHAR
MakeCompressedNameW(
    LPWSTR pszFileName)
{
   WCHAR chReplaced = L'\0';
   WCHAR ARG_PTR *pszExt;

   if ((pszExt = ExtractExtensionW(pszFileName)) != NULL)
   {
      if (lstrlenW(pszExt) >= 3)
      {
         chReplaced = pszExt[lstrlenW(pszExt) - 1];
         pszExt[lstrlenW(pszExt) - 1] = chEXTENSION_CHARW;
      }
      else
         lstrcatW(pszExt, pszEXTENSION_STRW);
   }
   else
      lstrcatW(pszFileName, pszNULL_EXTENSIONW);

   return(chReplaced);
}



CHAR MakeCompressedName(CHAR ARG_PTR *pszFileName)
{
    CHAR chReplaced = '\0';
    ULONG NameLength = STRLEN( pszFileName );
    ULONG DotIndex   = NameLength;

    while (( DotIndex > 0 ) && ( pszFileName[ --DotIndex ] != '.' )) {
        if (( pszFileName[ DotIndex ] == '\\' ) ||
            ( pszFileName[ DotIndex ] == ':' )) {   // end of filename part of path
            DotIndex = 0;                       // name has no extension
            break;
            }
        }

    if ( DotIndex > 0 ) {                       // name has an extension
        if (( NameLength - DotIndex ) <= 3 ) {  // extension less than 3 chars
            pszFileName[ NameLength++ ] = '_';  // append '_' to extension
            pszFileName[ NameLength ] = 0;      // terminate
            }
        else {                                  // extension more than 3 chars
            chReplaced = pszFileName[ NameLength - 1 ]; // return original
            pszFileName[ NameLength - 1 ] = '_';   // replace last char with '_'
            }
        }
    else {                                      // name has no extension
        pszFileName[ NameLength++ ] = '.';      // append '.'
        pszFileName[ NameLength++ ] = '_';      // append '_'
        pszFileName[ NameLength ] = 0;          // terminate
        }

    return(chReplaced);
}


/*
** void MakeExpandedName(char ARG_PTR *pszFileName, BYTE byteExtensionChar);
**
** Create expanded output file name.
**
** Arguments:  pszFileName       - expanded file name to change
**             byteExtensionChar - expanded file name extension character to
**                                 use
**
** Returns:    void
**
** Globals:    none
*/
VOID MakeExpandedName(CHAR ARG_PTR *pszFileName, BYTE byteExtensionChar)
{
   CHAR ARG_PTR *pszExt;
   INT nExtLen;

   // Is there any extension to change?
   if ((pszExt = ExtractExtension(pszFileName)) != NULL)
   {
      // Determine case of extension character.  Match case of first non-DB
      // character in name.  If all characters are DB, leave case alone.

      if (ISLETTER(byteExtensionChar))
      {
         // Find first alphabetic character in name.
         while (*pszFileName)
         {
            if (IsDBCSLeadByte(*pszFileName))
               pszFileName += 2;
            else if (ISLETTER(*pszFileName))
               break;
            else
               pszFileName++;
         }

         // Here pszFileName points to the first alphabetic character in the
         // name or to the null terminator.  Set the case of the extension
         // character.

         if (ISLOWER(*pszFileName))
            byteExtensionChar = (BYTE)TOLOWERCASE(byteExtensionChar);
         else if (ISUPPER(*pszFileName))
            byteExtensionChar = (BYTE)TOUPPERCASE(byteExtensionChar);
      }

      if ((nExtLen = STRLEN(pszExt)) > 0)
      {
         // Find the underscore character to replace, if it exists.

         // Assert: The underscore is either the last character in the
         // extension, or it is the first character in the extension followed
         // by a double-byte character.

         if (! IsDBCSLeadByte(*pszExt) && *pszExt == chEXTENSION_CHAR &&
             IsDBCSLeadByte(pszExt[1]))
            // Here the underscore is followed by a double-byte character.
            *pszExt = byteExtensionChar;
         else
         {
            // Here the underscore is the last character in the extension, if
            // there is an underscore at all.
            CHAR ARG_PTR *psz, *pszPrevious;

            for (psz = pszPrevious = pszExt; *psz != '\0'; psz = CharNext(psz))
               pszPrevious = psz;

            if (! IsDBCSLeadByte(*pszPrevious) &&
                *pszPrevious == chEXTENSION_CHAR)
               *pszPrevious = byteExtensionChar;
         }
      }

      // Get rid of trailing dot with no extension.
      if (*pszExt == '\0' && *(pszExt - 1) == PERIOD)
         *(pszExt - 1) = '\0';
   }
}


/*
** int CopyDateTimeStamp(INT_PTR doshFrom, INT_PTR doshTo);
**
** Copy date and time stamp from one file to another.
**
** Arguments:  doshFrom - date and time stamp source DOS file handle
**             doshTo   - target DOS file handle
**
** Returns:    TRUE if successful.  LZERROR_BADINHANDLE or
**             LZERROR_BADOUTHANDLE if unsuccessful.
**
** Globals:    none
**
** N.b., stream-style I/O routines like fopen() and fclose() may counter the
** intended effect of this function.  fclose() writes the current date to any
** file it's called with which was opened in write "w" or append "a" mode.
** One way to get around this in order to modify the date of a file opened
** for writing or appending by fopen() is to fclose() the file and fopen() it
** again in read "r" mode.  Then set its date and time stamp with
** CopyDateTimeStamp().
*/
INT CopyDateTimeStamp(INT_PTR doshFrom, INT_PTR doshTo)
{
#ifdef ORGCODE
   // DOS prototypes from <dos.h>
   extern DWORD _dos_getftime(INT dosh, DWORD *puDate, DWORD *puTime);
   extern DWORD _dos_setftime(INT dosh, DWORD uDate, DWORD uTime);

#ifdef LZA_DLL
   static
#endif
   DWORD uFrom_date,    // temporary storage for date
         uFrom_time;    // and time stamps

   if (_dos_getftime(doshFrom, &uFrom_date, &uFrom_time) != 0u)
      return((INT)LZERROR_BADINHANDLE);

   if (_dos_setftime(doshTo, uFrom_date, uFrom_time) != 0u)
      return((INT)LZERROR_BADOUTHANDLE);
#else

    FILETIME lpCreationTime, lpLastAccessTime, lpLastWriteTime;

   if(!GetFileTime((HANDLE) doshFrom, &lpCreationTime, &lpLastAccessTime,
                    &lpLastWriteTime)){
      return((INT)LZERROR_BADINHANDLE);
   }
   if(!SetFileTime((HANDLE) doshTo, &lpCreationTime, &lpLastAccessTime,
                    &lpLastWriteTime)){
      return((INT)LZERROR_BADINHANDLE);
   }

#endif
   return(TRUE);
}

