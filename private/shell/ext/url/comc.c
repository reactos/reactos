/*
 * comc.c - Shared routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "assoc.h"
#pragma warning(disable:4001) /* "single line comment" warning */
#include "filetype.h"
#pragma warning(default:4001) /* "single line comment" warning */


/* Global Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PUBLIC_DATA const char g_cszWhiteSpace[]           = " \t";

PUBLIC_DATA const char g_cszSlashes[]              = "/\\";

PUBLIC_DATA const char g_cszPathSeparators[]       = ":/\\";

PUBLIC_DATA const char g_cszEditFlags[]            = "EditFlags";

#pragma data_seg()


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA CCHAR s_cszMIMETypeSubKeyFmt[]        = "MIME\\Database\\Content Type\\%s";

PRIVATE_DATA CCHAR s_cszDefaultVerbSubKeyFmt[]     = "%s\\Shell";
PRIVATE_DATA CCHAR s_cszShellOpenCmdSubKey[]       = "Shell\\Open\\Command";

PRIVATE_DATA CCHAR s_cszContentType[]              = "Content Type";
PRIVATE_DATA CCHAR s_cszExtension[]                = "Extension";

PRIVATE_DATA const char s_cszAppCmdLineFmt[]       = "\"%s\" %s";
PRIVATE_DATA const char s_cszQuotesAppCmdLineFmt[] = "\"%s\" \"%s\"";

#pragma data_seg()


/***************************** Private Functions *****************************/


/*
** GetMIMETypeStringValue()
**
** Retrieves the string for a registered MIME type's value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetMIMETypeStringValue(PCSTR pcszMIMEType, PCSTR pcszValue,
                                         PSTR pszBuf, UINT ucBufLen)
{
   BOOL bResult;
   DWORD dwValueType;
   DWORD dwcbLen = ucBufLen;

   /* GetMIMEValue() will verify parameters. */

   bResult = (GetMIMEValue(pcszMIMEType, pcszValue, &dwValueType,
                           (PBYTE)pszBuf, &dwcbLen) &&
              dwValueType == REG_SZ);

   if (! bResult)
   {
      if (ucBufLen > 0)
         *pszBuf = '\0';
   }

   ASSERT(! ucBufLen ||
          IS_VALID_STRING_PTR(pszBuf, STR));

   return(bResult);
}


/****************************** Public Functions *****************************/


PUBLIC_CODE BOOL DataCopy(PCBYTE pcbyteSrc, ULONG ulcbLen, PBYTE *ppbyteDest)
{
   BOOL bResult;

   ASSERT(IS_VALID_READ_BUFFER_PTR(pcbyteSrc, CBYTE, ulcbLen));
   ASSERT(IS_VALID_WRITE_PTR(ppbyteDest, PBYTE));

   bResult = AllocateMemory(ulcbLen, ppbyteDest);

   if (bResult)
      CopyMemory(*ppbyteDest, pcbyteSrc, ulcbLen);
   else
      *ppbyteDest = NULL;

   ASSERT((bResult &&
           IS_VALID_READ_BUFFER_PTR(*ppbyteDest, BYTE, ulcbLen)) ||
          (! bResult &&
           EVAL(! *ppbyteDest)));

   return(bResult);
}


PUBLIC_CODE BOOL StringCopy(PCSTR pcszSrc, PSTR *ppszCopy)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRING_PTR(pcszSrc, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszCopy, PSTR));

   /* (+ 1) for null terminator. */

   bResult = DataCopy((PCBYTE)pcszSrc, lstrlen(pcszSrc) + 1, (PBYTE *)ppszCopy);

   ASSERT(! bResult ||
          IS_VALID_STRING_PTR(*ppszCopy, STR));

   return(bResult);
}


/*
** GetMIMETypeSubKey()
**
** Generates the HKEY_CLASSES_ROOT subkey for a MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetMIMETypeSubKey(PCSTR pcszMIMEType,
                                          PSTR pszSubKeyBuf,
                                          UINT ucSubKeyBufLen)
{
   BOOL bResult;

   bResult = ((UINT)lstrlen(s_cszMIMETypeSubKeyFmt) +
              (UINT)lstrlen(pcszMIMEType) < ucSubKeyBufLen);

   if (bResult)
      EVAL((UINT)wsprintf(pszSubKeyBuf, s_cszMIMETypeSubKeyFmt,
                          pcszMIMEType) < ucSubKeyBufLen);
   else
   {
      if (ucSubKeyBufLen > 0)
         *pszSubKeyBuf = '\0';

      WARNING_OUT(("GetMIMETypeSubKey(): Given sub key buffer of length %u is too short to hold sub key for MIME type %s.",
                   ucSubKeyBufLen,
                   pcszMIMEType));
   }

   ASSERT(! ucSubKeyBufLen ||
          (IS_VALID_STRING_PTR(pszSubKeyBuf, STR) &&
           (UINT)lstrlen(pszSubKeyBuf) < ucSubKeyBufLen));
   ASSERT(bResult ||
          ! ucSubKeyBufLen ||
          ! *pszSubKeyBuf);

   return(bResult);
}


/*
** GetMIMEValue()
**
** Retrieves the data for a value of a MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetMIMEValue(PCSTR pcszMIMEType, PCSTR pcszValue,
                              PDWORD pdwValueType, PBYTE pbyteValueBuf,
                              PDWORD pdwcbValueBufLen)
{
   BOOL bResult;
   char szMIMETypeSubKey[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, CSTR));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   bResult = (GetMIMETypeSubKey(pcszMIMEType, szMIMETypeSubKey,
                                       sizeof(szMIMETypeSubKey)) &&
              GetRegKeyValue(HKEY_CLASSES_ROOT, szMIMETypeSubKey,
                             pcszValue, pdwValueType, pbyteValueBuf,
                             pdwcbValueBufLen) == ERROR_SUCCESS);

   return(bResult);
}


/*
** GetFileTypeValue()
**
** Retrieves the data for a value of the file class associated with an
** extension.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetFileTypeValue(PCSTR pcszExtension, PCSTR pcszSubKey,
                                  PCSTR pcszValue, PDWORD pdwValueType,
                                  PBYTE pbyteValueBuf, PDWORD pdwcbValueBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IsValidExtension(pcszExtension));
   ASSERT(! pcszSubKey ||
          IS_VALID_STRING_PTR(pcszSubKey, CSTR));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   if (EVAL(*pcszExtension))
   {
      char szSubKey[MAX_PATH_LEN];
      DWORD dwcbLen = sizeof(szSubKey);

      /* Get extension's file type. */

      if (GetDefaultRegKeyValue(HKEY_CLASSES_ROOT, pcszExtension, szSubKey,
                                &dwcbLen) == ERROR_SUCCESS &&
          *szSubKey)
      {
         /* Any sub key to append? */

         if (pcszSubKey)
         {
            /* Yes. */

            /* (+ 1) for possible key separator. */

            bResult = EVAL(lstrlen(szSubKey) + 1 + lstrlen(pcszSubKey)
                           < sizeof(szSubKey));

            if (bResult)
            {
               CatPath(szSubKey, pcszSubKey);
               ASSERT(lstrlen(szSubKey) < sizeof(szSubKey));
            }
         }
         else
            /* No. */
            bResult = TRUE;

         if (bResult)
            /* Get file type's value string. */
            bResult = (GetRegKeyValue(HKEY_CLASSES_ROOT, szSubKey, pcszValue,
                                      pdwValueType, pbyteValueBuf,
                                      pdwcbValueBufLen) == ERROR_SUCCESS);
      }
      else
         TRACE_OUT(("GetFileTypeValue(): No file type registered for extension %s.",
                    pcszExtension));
   }
   else
      WARNING_OUT(("GetFileTypeValue(): No extension given."));

   return(bResult);
}


/*
** GetMIMEFileTypeValue()
**
** Retrieves the data for a value of the file class associated with a MIME
** type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetMIMEFileTypeValue(PCSTR pcszMIMEType, PCSTR pcszSubKey,
                                      PCSTR pcszValue, PDWORD pdwValueType,
                                      PBYTE pbyteValueBuf,
                                      PDWORD pdwcbValueBufLen)
{
   BOOL bResult;
   char szExtension[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, CSTR));
   ASSERT(! pcszSubKey ||
          IS_VALID_STRING_PTR(pcszSubKey, CSTR));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   /* Get file name extension associated with MIME type. */

   if (MIME_GetExtension(pcszMIMEType, szExtension, sizeof(szExtension)))
      bResult = GetFileTypeValue(szExtension, pcszSubKey, pcszValue, pdwValueType,
                                 pbyteValueBuf, pdwcbValueBufLen);
   else
   {
      bResult = FALSE;

      TRACE_OUT(("GetMIMEFileTypeValue(): No extension registered for MIME type %s.",
                 pcszMIMEType));
   }

   return(bResult);
}


/*
** MIME_IsExternalHandlerRegistered()
**
** Determines whether or not an external handler is registered for a MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL MIME_IsExternalHandlerRegistered(PCSTR pcszMIMEType)
{
   BOOL bResult;
   DWORD dwValueType;
   char szOpenCmd[MAX_PATH_LEN];
   DWORD dwcbOpenCmdLen = sizeof(szOpenCmd);

   /* GetMIMEFileTypeValue() will verify parameters. */

   /* Look up the open command of the MIME type's associated file type. */

   bResult = (GetMIMEFileTypeValue(pcszMIMEType, s_cszShellOpenCmdSubKey,
                                   NULL, &dwValueType, (PBYTE)szOpenCmd,
                                   &dwcbOpenCmdLen) &&
              dwValueType == REG_SZ);

   TRACE_OUT(("MIME_IsExternalHandlerRegistered(): %s external handler is registered for MIME type %s.",
              bResult ? "An" : "No",
              pcszMIMEType));

   return(bResult);
}


/*
** MIME_GetExtension()
**
** Determines the file name extension to be used when writing a file of a MIME
** type to the file system.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL MIME_GetExtension(PCSTR pcszMIMEType, PSTR pszExtensionBuf,
                                   UINT ucExtensionBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszExtensionBuf, STR, ucExtensionBufLen));

   if (EVAL(ucExtensionBufLen > 2))
   {
      /* Leave room for possible leading period. */

      if (GetMIMETypeStringValue(pcszMIMEType, s_cszExtension,
                                 pszExtensionBuf + 1, ucExtensionBufLen - 1))
      {
         if (pszExtensionBuf[1])
         {
            /* Prepend period if necessary. */

            if (pszExtensionBuf[1] == PERIOD)
               /* (+ 1) for null terminator. */
               MoveMemory(pszExtensionBuf, pszExtensionBuf + 1,
                          lstrlen(pszExtensionBuf + 1) + 1);
            else
               pszExtensionBuf[0] = PERIOD;

            bResult = TRUE;
         }
      }
   }

   if (! bResult)
   {
      if (ucExtensionBufLen > 0)
         *pszExtensionBuf = '\0';
   }

   if (bResult)
      TRACE_OUT(("MIME_GetExtension(): Extension %s registered as default extension for MIME type %s.",
                 pszExtensionBuf,
                 pcszMIMEType));
   else
      TRACE_OUT(("MIME_GetExtension(): No default extension registered for MIME type %s.",
                 pcszMIMEType));

   ASSERT((bResult &&
           IsValidExtension(pszExtensionBuf)) ||
          (! bResult &&
           (! ucExtensionBufLen ||
            ! *pszExtensionBuf)));
   ASSERT(! ucExtensionBufLen ||
          (UINT)lstrlen(pszExtensionBuf) < ucExtensionBufLen);

   return(bResult);
}


/*
** MIME_GetMIMETypeFromExtension()
**
** Determines the MIME type associated with a file extension.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL MIME_GetMIMETypeFromExtension(PCSTR pcszPath,
                                                      PSTR pszMIMETypeBuf,
                                                      UINT ucMIMETypeBufLen)
{
   BOOL bResult;
   PCSTR pcszExtension;

   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszMIMETypeBuf, STR, ucMIMETypeBufLen));

   pcszExtension = ExtractExtension(pcszPath);

   if (*pcszExtension)
   {
      DWORD dwcLen = ucMIMETypeBufLen;

      bResult = (GetRegKeyStringValue(HKEY_CLASSES_ROOT, pcszExtension,
                                      s_cszContentType, pszMIMETypeBuf,
                                      &dwcLen) == ERROR_SUCCESS);

      if (bResult)
         TRACE_OUT(("MIME_GetMIMETypeFromExtension(): MIME type for extension %s is %s.",

                    pcszExtension,
                    pszMIMETypeBuf));
      else
         TRACE_OUT(("MIME_GetMIMETypeFromExtension(): No MIME type registered for extension %s.",
                    pcszExtension));
   }
   else
   {
      bResult = FALSE;

      TRACE_OUT(("MIME_GetMIMETypeFromExtension(): No extension in path %s.",
                 pcszPath));
   }

   if (! bResult)
   {
      if (ucMIMETypeBufLen > 0)
         *pszMIMETypeBuf = '\0';
   }

   ASSERT(! ucMIMETypeBufLen ||
          (IS_VALID_STRING_PTR(pszMIMETypeBuf, STR) &&
           (UINT)lstrlen(pszMIMETypeBuf) < ucMIMETypeBufLen));
   ASSERT(bResult ||
          ! ucMIMETypeBufLen ||
          ! *pszMIMETypeBuf);

   return(bResult);
}


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
PUBLIC_CODE void CatPath(PSTR pszPath, PCSTR pcszSubPath)
{
   PSTR pcsz;
   PSTR pcszLast;

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   /* (+ 1) for possible separator. */
   /* (+ 1) for null terminator. */
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPath, STR, lstrlen(pszPath) + 1 + lstrlen(pcszSubPath) + 1));

   /* Find last character in path string. */

   for (pcsz = pcszLast = pszPath; *pcsz; pcsz = CharNext(pcsz))
      pcszLast = pcsz;

   if (IS_SLASH(*pcszLast) && IS_SLASH(*pcszSubPath))
      pcszSubPath++;
   else if (! IS_SLASH(*pcszLast) && ! IS_SLASH(*pcszSubPath))
   {
      if (*pcszLast && *pcszLast != COLON && *pcszSubPath)
         *pcsz++ = '\\';
   }

   lstrcpy(pcsz, pcszSubPath);

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));

   return;
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
PUBLIC_CODE void MyLStrCpyN(PSTR pszDest, PCSTR pcszSrc, int ncb)
{
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszDest, STR, ncb));
   ASSERT(IS_VALID_STRING_PTR(pcszSrc, CSTR));
   ASSERT(ncb > 0);

   while (ncb > 1)
   {
      ncb--;

      *pszDest = *pcszSrc;

      if (*pcszSrc)
      {
         pszDest++;
         pcszSrc++;
      }
      else
         break;
   }

   if (ncb == 1)
      *pszDest = '\0';

   ASSERT(IS_VALID_STRING_PTR(pszDest, STR));
   ASSERT(lstrlen(pszDest) < ncb);
   ASSERT(lstrlen(pszDest) <= lstrlen(pcszSrc));

   return;
}


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
** TrimWhiteSpace()
**
** Trims leading and trailing white space from a string in place.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void TrimWhiteSpace(PSTR pszTrimMe)
{
   ASSERT(IS_VALID_STRING_PTR(pszTrimMe, STR));

   TrimString(pszTrimMe, g_cszWhiteSpace);

   /* TrimString() validates pszTrimMe on output. */

   return;
}


/*
** TrimSlashes()
**
** Trims leading and trailing slashes from a string in place.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void TrimSlashes(PSTR pszTrimMe)
{
   ASSERT(IS_VALID_STRING_PTR(pszTrimMe, STR));

   TrimString(pszTrimMe, g_cszSlashes);

   /* TrimString() validates pszTrimMe on output. */

   return;
}


PUBLIC_CODE void TrimString(PSTR pszTrimMe, PCSTR pszTrimChars)
{
   PSTR psz;
   PSTR pszStartMeat;

   ASSERT(IS_VALID_STRING_PTR(pszTrimMe, STR));
   ASSERT(IS_VALID_STRING_PTR(pszTrimChars, CSTR));

   if ( !pszTrimMe )
      return;

   /* Trim leading characters. */

   psz = pszTrimMe;

   while (*psz && strchr(pszTrimChars, *psz))
      psz = CharNext(psz);

   pszStartMeat = psz;

   /* Trim trailing characters. */

   if (*psz)
   {
      psz += lstrlen(psz);

      psz = CharPrev(pszStartMeat, psz);

      if (psz > pszStartMeat)
      {
         while (strchr(pszTrimChars, *psz))
            psz = CharPrev(pszStartMeat, psz);

         psz = CharNext(psz);

         ASSERT(psz > pszStartMeat);

         *psz = '\0';
      }
   }

   /* Relocate stripped string. */

   if (pszStartMeat > pszTrimMe)
      /* (+ 1) for null terminator. */
      MoveMemory(pszTrimMe, pszStartMeat, lstrlen(pszStartMeat) + 1);
   else
      ASSERT(pszStartMeat == pszTrimMe);

   ASSERT(IS_VALID_STRING_PTR(pszTrimMe, STR));

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
PUBLIC_CODE PCSTR ExtractFileName(PCSTR pcszPathName)
{
   PCSTR pcszLastComponent;
   PCSTR pcsz;

   ASSERT(IS_VALID_STRING_PTR(pcszPathName, CSTR));

   for (pcszLastComponent = pcsz = pcszPathName;
        *pcsz;
        pcsz = CharNext(pcsz))
   {
      if (IS_SLASH(*pcsz) || *pcsz == COLON)
         pcszLastComponent = CharNext(pcsz);
   }

   ASSERT(IsValidPath(pcszLastComponent));

   return(pcszLastComponent);
}


/*
** ExtractExtension()
**
** Extracts the extension from a name.
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
PUBLIC_CODE PCSTR ExtractExtension(PCSTR pcszName)
{
   PCSTR pcszLastPeriod;

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

   ASSERT(! *pcszLastPeriod ||
          IsValidExtension(pcszLastPeriod));

   return(pcszLastPeriod);
}


/*
** SetRegKeyValue()
**
** Sets the data associated with a registry key's value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LONG    SetRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey,
                                   PCSTR pcszValue, DWORD dwType,
                                   PCBYTE pcbyte, DWORD dwcb)
{
   LONG lResult;
   HKEY hkeySubKey;

   ASSERT(IS_VALID_HANDLE(hkeyParent, KEY));
   ASSERT(IS_VALID_STRING_PTR(pcszSubKey, CSTR));
   ASSERT(IsValidRegistryValueType(dwType));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, CSTR));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcbyte, CBYTE, dwcb));

   lResult = RegCreateKeyEx(hkeyParent, pcszSubKey, 0, NULL, 0, KEY_SET_VALUE,
                            NULL, &hkeySubKey, NULL);

   if (lResult == ERROR_SUCCESS)
   {
      LONG lResultClose;

      lResult = RegSetValueEx(hkeySubKey, pcszValue, 0, dwType, pcbyte, dwcb);

      lResultClose = RegCloseKey(hkeySubKey);

      if (lResult == ERROR_SUCCESS)
         lResult = lResultClose;
   }

   return(lResult);
}


/*
** GetRegKeyValue()
**
** Retrieves the data from a registry key's value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LONG    GetRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey,
                                   PCSTR pcszValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen)
{
   LONG lResult;
   HKEY hkeySubKey;

   ASSERT(IS_VALID_HANDLE(hkeyParent, KEY));
   ASSERT(! pcszSubKey ||
          IS_VALID_STRING_PTR(pcszSubKey, CSTR));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, CSTR));
   ASSERT(! pdwValueType ||
          IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(! pbyteBuf ||
          IS_VALID_WRITE_BUFFER_PTR(pbyteBuf, BYTE, *pdwcbBufLen));

   lResult = RegOpenKeyEx(hkeyParent, pcszSubKey, 0, KEY_QUERY_VALUE,
                          &hkeySubKey);

   if (lResult == ERROR_SUCCESS)
   {
      LONG lResultClose;

      lResult = RegQueryValueEx(hkeySubKey, pcszValue, NULL, pdwValueType,
                                pbyteBuf, pdwcbBufLen);

      lResultClose = RegCloseKey(hkeySubKey);

      if (lResult == ERROR_SUCCESS)
         lResult = lResultClose;
   }

   return(lResult);
}


/*
** GetRegKeyStringValue()
**
** Retrieves the data from a registry key's string value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LONG    GetRegKeyStringValue(HKEY hkeyParent, PCSTR pcszSubKey,
                                         PCSTR pcszValue, PSTR pszBuf,
                                         PDWORD pdwcbBufLen)
{
   LONG lResult;
   DWORD dwValueType;

   /* GetRegKeyValue() will verify the parameters. */

   lResult = GetRegKeyValue(hkeyParent, pcszSubKey, pcszValue, &dwValueType,
                            (PBYTE)pszBuf, pdwcbBufLen);

   if (lResult == ERROR_SUCCESS &&
       dwValueType != REG_SZ)
      lResult = ERROR_CANTREAD;

   return(lResult);
}


/*
** GetDefaultRegKeyValue()
**
** Retrieves the data from a registry key's default string value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LONG    GetDefaultRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey,
                                          PSTR pszBuf, PDWORD pdwcbBufLen)
{
   /* GetRegKeyStringValue() will verify the parameters. */

   return(GetRegKeyStringValue(hkeyParent, pcszSubKey, NULL, pszBuf,
                               pdwcbBufLen));
}


/*
** FullyQualifyPath()
**
** Fully qualifies a path.
**
** Arguments:
**
** Returns:       S_OK
**
**                E_FILE_NOT_FOUND
**
** Side Effects:  none
*/
PUBLIC_CODE HRESULT FullyQualifyPath(PCSTR pcszPath,
                                     PSTR pszFullyQualifiedPath,
                                     UINT ucFullyQualifiedPathBufLen)
{
   HRESULT hr = E_FILE_NOT_FOUND;
   PSTR pszFileName;

   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

   /* Any path separators? */

   if (! strpbrk(pcszPath, g_cszPathSeparators))
   {
      /* No.  Search for file. */

      TRACE_OUT(("FullyQualifyPath(): Searching PATH for %s.",
                 pcszPath));

      if (SearchPath(NULL, pcszPath, NULL, ucFullyQualifiedPathBufLen,
                     pszFullyQualifiedPath, &pszFileName) > 0)
         hr = S_OK;
      else
         TRACE_OUT(("FullyQualifyPath(): %s not found on PATH.",
                    pcszPath));
   }

   if (hr != S_OK &&
       GetFullPathName(pcszPath, ucFullyQualifiedPathBufLen,
                       pszFullyQualifiedPath, &pszFileName) > 0)
      hr = S_OK;

   if (hr == S_OK)
      TRACE_OUT(("FullyQualifyPath(): %s qualified as %s.",
                 pcszPath,
                 pszFullyQualifiedPath));
   else
   {
      if (ucFullyQualifiedPathBufLen > 0)
         pszFullyQualifiedPath = '\0';

      WARNING_OUT(("FullyQualifyPath(): Failed to qualify %s.",
                   pcszPath));
   }

   ASSERT((hr == S_OK &&
           EVAL(IsFullPath(pszFullyQualifiedPath))) ||
          (hr == E_FILE_NOT_FOUND &&
           (! ucFullyQualifiedPathBufLen ||
            ! *pszFullyQualifiedPath)));

   return(hr);
}


/*
** MyExecute()
**
** Calls CreateProcess() politely.
**
** Arguments:
**
** Returns:       S_OK
**
**                E_FAIL
**                E_FILE_NOT_FOUND
**                E_OUTOFMEMORY
**
** Side Effects:  none
*/
PUBLIC_CODE HRESULT MyExecute(PCSTR pcszApp, PCSTR pcszArgs, DWORD dwInFlags)
{
   HRESULT hr;
   char szFullApp[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcszApp, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszArgs, CSTR));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_ME_IN_FLAGS));

   hr = FullyQualifyPath(pcszApp, szFullApp, sizeof(szFullApp));

   if (hr == S_OK)
   {
      DWORD dwcbCmdLineLen;
      PSTR pszCmdLine;

      /* (+ 1) for null terminator. */
      dwcbCmdLineLen = max(sizeof(s_cszAppCmdLineFmt),
                           sizeof(s_cszQuotesAppCmdLineFmt)) +
                       + lstrlen(szFullApp) + lstrlen(pcszArgs) + 1;

      if (AllocateMemory(dwcbCmdLineLen * sizeof(*pszCmdLine), &pszCmdLine))
      {
         PCSTR pcszFmt;
         STARTUPINFO si;
         PROCESS_INFORMATION pi;

         /* Execute URL via one-shot app. */

         pcszFmt = (IS_FLAG_SET(dwInFlags, ME_IFL_QUOTE_ARGS) &&
                    strpbrk(pcszArgs, g_cszWhiteSpace) != NULL)
                    ? s_cszQuotesAppCmdLineFmt : s_cszAppCmdLineFmt;

         EVAL((DWORD)wsprintf(pszCmdLine, pcszFmt, szFullApp, pcszArgs)
              < dwcbCmdLineLen);

         ZeroMemory(&si, sizeof(si));
         si.cb = sizeof(si);

         /* Specify command line exactly as given to app. */

         if (CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL,
                           &si, &pi))
         {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            hr = S_OK;

            TRACE_OUT(("MyExecute(): CreateProcess() \"%s\" succeeded.",
                       pszCmdLine));
         }
         else
         {
            hr = E_FAIL;

            WARNING_OUT(("MyExecute(): CreateProcess() \"%s\" failed.",
                         pszCmdLine));
         }

         FreeMemory(pszCmdLine);
         pszCmdLine = NULL;
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
      WARNING_OUT(("MyExecute(): Unable to find app %s.",
                   pcszApp));

   return(hr);
}


PUBLIC_CODE BOOL GetClassDefaultVerb(PCSTR pcszClass, PSTR pszDefaultVerbBuf,
                                     UINT ucDefaultVerbBufLen)
{
   BOOL bResult;
   char szDefaultVerbSubKey[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcszClass, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszDefaultVerbBuf, STR, ucDefaultVerbBufLen));

   ASSERT(lstrlen(pcszClass) > 0);

   if (sizeof(s_cszDefaultVerbSubKeyFmt) + lstrlen(pcszClass)
       < sizeof(szDefaultVerbSubKey))
   {
      DWORD dwValueType;
      DWORD dwcbBufLen = ucDefaultVerbBufLen;

      EVAL(wsprintf(szDefaultVerbSubKey, s_cszDefaultVerbSubKeyFmt,
                    pcszClass) < sizeof(szDefaultVerbSubKey));

      bResult = (GetRegKeyValue(HKEY_CLASSES_ROOT, szDefaultVerbSubKey, NULL,
                                &dwValueType, (PBYTE)pszDefaultVerbBuf,
                                &dwcbBufLen) == ERROR_SUCCESS &&
                 dwValueType == REG_SZ &&
                 *pszDefaultVerbBuf);
   }
   else
      bResult = FALSE;

   if (! bResult)
   {
      if (ucDefaultVerbBufLen > 0)
         *pszDefaultVerbBuf = '\0';
   }

   if (bResult)
      TRACE_OUT(("GetClassDefaultVerb(): Default verb for %s class is %s.",
                 pcszClass,
                 pszDefaultVerbBuf));
   else
      TRACE_OUT(("GetClassDefaultVerb(): No default verb for %s class.",
                 pcszClass));

   ASSERT(! ucDefaultVerbBufLen ||
          (IS_VALID_STRING_PTR(pszDefaultVerbBuf, STR) &&
           EVAL((UINT)lstrlen(pszDefaultVerbBuf) < ucDefaultVerbBufLen)));
   ASSERT(bResult ||
          ! ucDefaultVerbBufLen ||
          ! *pszDefaultVerbBuf);

   return(bResult);
}


/*
 * BUGBUG: The need for this function should be obviated by a ShellExecuteEx()
 * flag indicating that the default verb, rather than the open verb, should be
 * executed.  This is broken for folders, for compound document files, for
 * files whose extensions are registered without a file class, for files whose
 * extensions are not unregistered, etc.
 */
PUBLIC_CODE BOOL GetPathDefaultVerb(PCSTR pcszPath, PSTR pszDefaultVerbBuf,
                                    UINT ucDefaultVerbBufLen)
{
   BOOL bResult = FALSE;
   PCSTR pcszExtension;
   char szClass[MAX_PATH_LEN];
   DWORD dwcbLen = ucDefaultVerbBufLen;

   ASSERT(IsValidPath(pcszPath));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszDefaultVerbBuf, STR, ucDefaultVerbBufLen));

   pcszExtension = ExtractExtension(pcszPath);

   bResult = (*pcszExtension &&
              GetDefaultRegKeyValue(HKEY_CLASSES_ROOT, pcszExtension,
                                    szClass, &dwcbLen) == ERROR_SUCCESS &&
              *szClass &&
              GetClassDefaultVerb(szClass, pszDefaultVerbBuf,
                                  ucDefaultVerbBufLen));

   if (! bResult)
   {
      if (ucDefaultVerbBufLen > 0)
         *pszDefaultVerbBuf = '\0';
   }

   ASSERT(! ucDefaultVerbBufLen ||
          (IS_VALID_STRING_PTR(pszDefaultVerbBuf, STR) &&
           EVAL((UINT)lstrlen(pszDefaultVerbBuf) < ucDefaultVerbBufLen)));
   ASSERT(bResult ||
          ! ucDefaultVerbBufLen ||
          ! *pszDefaultVerbBuf);

   return(bResult);
}


/*
** ClassIsSafeToOpen()
**
** Determines whether or not a file class is known safe to open.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ClassIsSafeToOpen(PCSTR pcszClass)
{
   BOOL bSafe;
   DWORD dwValueType;
   DWORD dwEditFlags;
   DWORD dwcbLen = sizeof(dwEditFlags);

   ASSERT(IS_VALID_STRING_PTR(pcszClass, CSTR));

   bSafe = (GetRegKeyValue(HKEY_CLASSES_ROOT, pcszClass, g_cszEditFlags,
                           &dwValueType, (PBYTE)&dwEditFlags, &dwcbLen) == ERROR_SUCCESS &&
            (dwValueType == REG_BINARY ||
             dwValueType == REG_DWORD) &&
            IS_FLAG_SET(dwEditFlags, FTA_OpenIsSafe));

   TRACE_OUT(("ClassIsSafeToOpen(): Class %s %s safe to open.",
              pcszClass,
              bSafe ? "is" : "is not"));

   return(bSafe);
}


/*
** SetClassEditFlags()
**
** Sets or clears EditFlags for a file class.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SetClassEditFlags(PCSTR pcszClass, DWORD dwFlags, BOOL bSet)
{
   BOOL bResult = FALSE;
   DWORD dwValueType;
   DWORD dwEditFlags;
   DWORD dwcbLen = sizeof(dwEditFlags);

   ASSERT(IS_VALID_STRING_PTR(pcszClass, CSTR));

   /* Get current file class flags. */

   if (GetRegKeyValue(HKEY_CLASSES_ROOT, pcszClass, g_cszEditFlags,
                      &dwValueType, (PBYTE)&dwEditFlags, &dwcbLen) != ERROR_SUCCESS ||
       (dwValueType != REG_BINARY &&
        dwValueType != REG_DWORD))
      dwEditFlags = 0;

   /* Set or clear SafeOpen flag for file class. */

   if (bSet)
      SET_FLAG(dwEditFlags, dwFlags);
   else
      CLEAR_FLAG(dwEditFlags, dwFlags);

   /*
    * N.b., we must set this as REG_BINARY because the base Win95 shell32.dll
    * only accepts REG_BINARY EditFlags.
    */

   bResult = (SetRegKeyValue(HKEY_CLASSES_ROOT, pcszClass, g_cszEditFlags,
                             REG_BINARY, (PCBYTE)&dwEditFlags,
                             sizeof(dwEditFlags)) == ERROR_SUCCESS);

   if (bResult)
      TRACE_OUT(("SetClassEditFlags(): Class %s flags %lu %s.",
                 pcszClass,
                 dwFlags,
                 bSet ? "set" : "cleared"));
   else
      WARNING_OUT(("SetClassEditFlags(): Failed to %s class %s flags %lu.",
                   bSet ? "set" : "clear",
                   pcszClass,
                   dwFlags));

   return(bResult);
}


#ifdef DEBUG

PUBLIC_CODE BOOL IsFullPath(PCSTR pcszPath)
{
   BOOL bResult = FALSE;
   char rgchFullPath[MAX_PATH_LEN];

   if (IS_VALID_STRING_PTR(pcszPath, CSTR) &&
       EVAL(lstrlen(pcszPath) < MAX_PATH_LEN))
   {
      DWORD dwPathLen;
      PSTR pszFileName;

      dwPathLen = GetFullPathName(pcszPath, sizeof(rgchFullPath), rgchFullPath,
                                  &pszFileName);

      if (EVAL(dwPathLen > 0) &&
          EVAL(dwPathLen < sizeof(rgchFullPath)))
         bResult = EVAL(! lstrcmpi(pcszPath, rgchFullPath));
   }

   return(bResult);
}

#endif   /* DEBUG */
