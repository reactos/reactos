/*
 * extricon.cpp - IExtractIcon implementation for URL class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "assoc.h"


/* Global Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

extern const char g_cszURLDefaultIconKey[]         = "InternetShortcut\\DefaultIcon";

extern const HKEY g_hkeyURLSettings                = HKEY_LOCAL_MACHINE;

#pragma data_seg()


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA const char s_cszDefaultIconSubKey[]   = "DefaultIcon";

PRIVATE_DATA const char s_cszGenericURLIconFile[]  = "url.dll";
PRIVATE_DATA const int s_ciGenericURLIcon          = 0;

// IExtractIcon::GetIconLocation() flag combinations

PRIVATE_DATA const int ALL_GIL_IN_FLAGS            = (GIL_OPENICON |
                                                      GIL_FORSHELL);

PRIVATE_DATA const int ALL_GIL_OUT_FLAGS           = (GIL_SIMULATEDOC |
                                                      GIL_PERINSTANCE |
                                                      GIL_PERCLASS |
                                                      GIL_NOTFILENAME |
                                                      GIL_DONTCACHE);

#pragma data_seg()


/***************************** Private Functions *****************************/


/*
** ParseIconEntry()
**
**
**
** Arguments:
**
** Returns:       S_OK if icon entry parsed successfully.
**                E_FAIL if not.
**
** Side Effects:  The contents of pszIconEntry are destroyed.
**
** pszIconEntry and pszIconFile may be the same.
*/
PRIVATE_CODE HRESULT ParseIconEntry(PSTR pszIconEntry, PSTR pszIconFile,
                                    UINT ucbIconFileBufLen, PINT pniIcon)
{
   HRESULT hr;
   PSTR pszComma;

   ASSERT(IS_VALID_STRING_PTR(pszIconEntry, STR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszIconFile, STR, ucbIconFileBufLen));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

   pszComma = StrChr(pszIconEntry, ',');

   if (pszComma)
   {
      *pszComma++ = '\0';
      TrimWhiteSpace(pszComma);
      *pniIcon = StringToInt(pszComma);
   }
   else
   {
      *pniIcon = 0;

      WARNING_OUT(("ParseIconEntry(): No icon index in entry %s.  Using icon index 0.",
                   pszIconEntry));
   }

   TrimWhiteSpace(pszIconEntry);

   if ((UINT)lstrlen(pszIconEntry) < ucbIconFileBufLen)
   {
      lstrcpy(pszIconFile, pszIconEntry);
      hr = S_OK;

      TRACE_OUT(("ParseIconEntry(): Parsed icon file %s, index %d.",
                 pszIconFile,
                 *pniIcon));
   }
   else
   {
      hr = S_FALSE;

      // (+ 1) for null terminator.

      WARNING_OUT(("ParseIconEntry(): Icon file buffer too small for icon file %s (%u bytes supplied, %lu bytes required).",
                   pszIconEntry,
                   ucbIconFileBufLen,
                   lstrlen(pszIconEntry) + 1));
   }

   ASSERT(IsValidIconIndex(hr, pszIconFile, ucbIconFileBufLen, *pniIcon));

   return(hr);
}


/*
** GetURLIcon()
**
**
**
** Arguments:
**
** Returns:       S_OK if icon information retrieved successfully.
**                S_FALSE if no icon entry for this URL.
**                Otherwise error.
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT GetURLIcon(HKEY hkey, PCSTR pcszKey, PSTR pszIconFile,
                                UINT ucbIconFileBufLen, PINT pniIcon)
{
   HRESULT hr;
   DWORD dwcbLen = ucbIconFileBufLen;

   ASSERT(IS_VALID_HANDLE(hkey, KEY));
   ASSERT(IS_VALID_STRING_PTR(pcszKey, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszIconFile, STR, ucbIconFileBufLen));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

   if (GetDefaultRegKeyValue(hkey, pcszKey, pszIconFile, &dwcbLen)
       == ERROR_SUCCESS)
      hr = ParseIconEntry(pszIconFile, pszIconFile, ucbIconFileBufLen,
                          pniIcon);
   else
   {
      // No protocol handler.

      hr = S_FALSE;

      TRACE_OUT(("GetURLIcon(): Couldn't get default value for key %s.",
                 pcszKey));
   }

   ASSERT(IsValidIconIndex(hr, pszIconFile, ucbIconFileBufLen, *pniIcon));

   return(hr);
}


/*
** GetFallBackGenericURLIcon()
**
**
**
** Arguments:
**
** Returns:       S_OK if fallback generic icon information retrieved
**                successfully.
**                E_FAIL if not.
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT GetFallBackGenericURLIcon(PSTR pszIconFile,
                                               UINT ucbIconFileBufLen,
                                               PINT pniIcon)
{
   HRESULT hr;

   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszIconFile, STR, ucbIconFileBufLen));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

   // Fall back to first icon in this module.

   if (ucbIconFileBufLen >= sizeof(s_cszGenericURLIconFile))
   {
      lstrcpy(pszIconFile, s_cszGenericURLIconFile);
      *pniIcon = s_ciGenericURLIcon;

      hr = S_OK;

      TRACE_OUT(("GetFallBackGenericURLIcon(): Using generic URL icon file %s, index %d.",
                 s_cszGenericURLIconFile,
                 s_ciGenericURLIcon));
   }
   else
   {
      hr = E_FAIL;

      WARNING_OUT(("GetFallBackGenericURLIcon(): Icon file buffer too small for generic icon file %s (%u bytes supplied, %lu bytes required).",
                   s_cszGenericURLIconFile,
                   ucbIconFileBufLen,
                   sizeof(s_cszGenericURLIconFile)));
   }

   ASSERT(IsValidIconIndex(hr, pszIconFile, ucbIconFileBufLen, *pniIcon));

   return(hr);
}


/*
** GetGenericURLIcon()
**
**
**
** Arguments:
**
** Returns:       S_OK if generic icon information retrieved successfully.
**                Otherwise error.
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT GetGenericURLIcon(PSTR pszIconFile,
                                       UINT ucbIconFileBufLen, PINT pniIcon)
{
   HRESULT hr;

   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszIconFile, STR, ucbIconFileBufLen));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

   hr = GetURLIcon(g_hkeyURLProtocols, g_cszURLDefaultIconKey, pszIconFile,
                   ucbIconFileBufLen, pniIcon);

   if (hr == S_FALSE)
      hr = GetFallBackGenericURLIcon(pszIconFile, ucbIconFileBufLen, pniIcon);

   ASSERT(IsValidIconIndex(hr, pszIconFile, ucbIconFileBufLen, *pniIcon));

   return(hr);
}


/****************************** Public Functions *****************************/


/*
** StringToInt()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Stops at first non-digit character encountered.
*/
PUBLIC_CODE int StringToInt(PCSTR pcsz)
{
   int nResult = 0;
   BOOL bNegative;
#ifdef DEBUG
   PCSTR pcszStart = pcsz;
#endif

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   if (*pcsz == '-')
   {
      bNegative = TRUE;
      pcsz++;
   }
   else
      bNegative = FALSE;

   while (IsDigit(*pcsz))
   {
      ASSERT(nResult <= INT_MAX / 10);
      nResult *= 10;
      ASSERT(nResult <= INT_MAX - (*pcsz - '0'));
      nResult += *pcsz++ - '0';
   }

   if (*pcsz) {
      WARNING_OUT(("StringToInt(): Stopped at non-digit character %c in string %s.",
                   *pcsz,
                   pcszStart));
   }

   // nResult may be any value.

   return(bNegative ? - nResult : nResult);
}


PUBLIC_CODE BOOL IsWhiteSpace(char ch)
{
   return((ch && StrChr(g_cszWhiteSpace, ch)) ? TRUE : FALSE);
}


PUBLIC_CODE BOOL AnyMeat(PCSTR pcsz)
{
   ASSERT(! pcsz ||
          IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz ? StrSpn(pcsz, g_cszWhiteSpace) < lstrlen(pcsz) : FALSE);
}


PUBLIC_CODE HRESULT CopyURLProtocol(PCSTR pcszURL, PSTR *ppszProtocol)
{
   HRESULT hr;
   PARSEDURL pu;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszProtocol, PSTR));

   *ppszProtocol = NULL;

   pu.cbSize = sizeof(pu);
   hr = ParseURL(pcszURL, &pu);

   if (hr == S_OK)
   {
      // (+ 1) for null terminator.
      *ppszProtocol = new(char[pu.cchProtocol + 1]);

      if (*ppszProtocol)
      {
         // (+ 1) for null terminator.
         lstrcpyn(*ppszProtocol, pu.pszProtocol, pu.cchProtocol + 1);
         ASSERT((UINT)lstrlen(*ppszProtocol) == pu.cchProtocol);
      }
      else
         hr = E_OUTOFMEMORY;
   }

   ASSERT(FAILED(hr) ||
          (hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszProtocol, STR)));

   return(hr);
}


PUBLIC_CODE HRESULT CopyURLSuffix(PCSTR pcszURL, PSTR *ppszSuffix)
{
   HRESULT hr;
   PARSEDURL pu;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszSuffix, PSTR));

   *ppszSuffix = NULL;

   hr = ParseURL(pcszURL, &pu);

   if (hr == S_OK)
   {
      // (+ 1) for null terminator.
      *ppszSuffix = new(char[pu.cchSuffix + 1]);

      if (*ppszSuffix)
      {
         // (+ 1) for null terminator.
         lstrcpyn(*ppszSuffix, pu.pszSuffix, pu.cchSuffix + 1);
         ASSERT((UINT)lstrlen(*ppszSuffix) == pu.cchSuffix);

         hr = S_OK;
      }
      else
         hr = E_OUTOFMEMORY;
   }

   ASSERT(FAILED(hr) ||
          IS_VALID_STRING_PTR(*ppszSuffix, STR));

   return(hr);
}


PUBLIC_CODE HRESULT GetProtocolKey(PCSTR pcszProtocol, PCSTR pcszSubKey,
                                   PSTR *ppszKey)
{
   HRESULT hr;
   ULONG ulcbKeyLen;

   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, STR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubKey, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszKey, PSTR));

   // (+ 1) for possible separator.
   // (+ 1) for null terminator.
   ulcbKeyLen = lstrlen(pcszProtocol) + 1 + lstrlen(pcszSubKey) + 1;

   *ppszKey = new(char[ulcbKeyLen]);

   if (*ppszKey)
   {
      lstrcpy(*ppszKey, pcszProtocol);
      PathAppend(*ppszKey, pcszSubKey);

      ASSERT((UINT)lstrlen(*ppszKey) < ulcbKeyLen);

      hr = S_OK;
   }
   else
      hr = E_OUTOFMEMORY;

   ASSERT((hr == S_OK &&
             IS_VALID_STRING_PTR(*ppszKey, STR)) ||
          hr == E_OUTOFMEMORY);

   return(hr);
}


PUBLIC_CODE HRESULT GetURLKey(PCSTR pcszURL, PCSTR pcszSubKey, PSTR *pszKey)
{
   HRESULT hr;
   PSTR pszProtocol;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubKey, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pszKey, PSTR));

   *pszKey = NULL;

   hr = CopyURLProtocol(pcszURL, &pszProtocol);

   if (hr == S_OK)
   {
      hr = GetProtocolKey(pszProtocol, pcszSubKey, pszKey);

      delete pszProtocol;
      pszProtocol = NULL;
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*pszKey, STR)) ||
          FAILED(hr));

   return(hr);
}


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE InternetShortcut::GetIconLocation(
                                                      UINT uInFlags,
                                                      PSTR pszIconFile,
                                                      UINT ucbIconFileBufLen,
                                                      PINT pniIcon,
                                                      PUINT puOutFlags)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetIconLocation);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(FLAGS_ARE_VALID(uInFlags, ALL_GIL_IN_FLAGS));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszIconFile, STR, ucbIconFileBufLen));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));
   ASSERT(IS_VALID_WRITE_PTR(puOutFlags, UINT));

   if (IS_FLAG_CLEAR(uInFlags, GIL_OPENICON))
   {
      hr = GetIconLocation(pszIconFile, ucbIconFileBufLen, pniIcon);

      if (hr != S_OK)
      {
         if (m_pszURL)
         {
            PSTR pszDefaultIconKey;

            // Look up URL icon based on protocol handler.

            hr = GetURLKey(m_pszURL, s_cszDefaultIconSubKey,
                           &pszDefaultIconKey);

            if (hr == S_OK)
            {
               hr = GetURLIcon(g_hkeyURLProtocols, pszDefaultIconKey,
                               pszIconFile, ucbIconFileBufLen, pniIcon);

               delete pszDefaultIconKey;
               pszDefaultIconKey = NULL;
            }
         }
         else
            hr = S_FALSE;

         if (hr == S_FALSE)
         {
            // Use generic URL icon.

            hr = GetGenericURLIcon(pszIconFile, ucbIconFileBufLen, pniIcon);

            if (hr == S_OK) {
               TRACE_OUT(("InternetShortcut::GetIconLocation(): Using generic URL icon."));
            }
         }

         if (hr == S_OK)
         {
            char rgchFullPath[MAX_PATH_LEN];

            hr = FullyQualifyPath(pszIconFile, rgchFullPath,
                                  sizeof(rgchFullPath));

            if (hr == S_OK)
            {
               if ((UINT)lstrlen(rgchFullPath) < ucbIconFileBufLen)
                  lstrcpy(pszIconFile, rgchFullPath);
               else
                  hr = E_FAIL;
            }
         }
      }
   }
   else
      // No "open look" icon.
      hr = S_FALSE;

   if (hr != S_OK)
   {
      if (ucbIconFileBufLen > 0)
         *pszIconFile = '\0';

      *pniIcon = 0;
   }

   *puOutFlags = 0;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IsValidIconIndex(hr, pszIconFile, ucbIconFileBufLen, *pniIcon) &&
          FLAGS_ARE_VALID(*puOutFlags, ALL_GIL_OUT_FLAGS));

   DebugExitHRESULT(InternetShortcut::GetIconLocation, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::Extract(PCSTR pcszIconFile,
                                                    UINT uiIcon,
                                                    PHICON phiconLarge,
                                                    PHICON phiconSmall,
                                                    UINT ucIconSize)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::Extract);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszIconFile, CSTR));
   ASSERT(IsValidIconIndex(S_OK, pcszIconFile, MAX_PATH_LEN, uiIcon));
   ASSERT(IS_VALID_WRITE_PTR(phiconLarge, HICON));
   ASSERT(IS_VALID_WRITE_PTR(phiconSmall, HICON));
   // BUGBUG: Validate ucIconSize here.

   *phiconLarge = NULL;
   *phiconSmall = NULL;

   // Use caller's default implementation of ExtractIcon().

   hr = S_FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_HANDLE(*phiconLarge, ICON) &&
           IS_VALID_HANDLE(*phiconSmall, ICON)) ||
          (hr != S_OK &&
           ! *phiconLarge &&
           ! *phiconSmall));

   DebugExitHRESULT(InternetShortcut::Extract, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */

