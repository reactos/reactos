/*
 * url.cpp - IUniformResourceLocator implementation for InternetShortcut class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "assoc.h"
#include "resource.h"


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA const char s_cszURLSeparator[]        = ":";

PRIVATE_DATA const char s_cchURLSuffixSlash        = '/';

PRIVATE_DATA const char s_cszURLPrefixesKey[]      = "Software\\Microsoft\\Windows\\CurrentVersion\\URL\\Prefixes";
PRIVATE_DATA const char s_cszDefaultURLPrefixKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\URL\\DefaultPrefix";

#pragma data_seg()


/***************************** Private Functions *****************************/


PRIVATE_CODE PCSTR SkipLeadingSlashes(PCSTR pcszURL)
{
   PCSTR pcszURLStart;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));

   // Skip two leading slashes.

   if (pcszURL[0] == s_cchURLSuffixSlash &&
       pcszURL[1] == s_cchURLSuffixSlash)
   {
      pcszURLStart = CharNext(pcszURL);
      pcszURLStart = CharNext(pcszURLStart);
   }
   else
      pcszURLStart = pcszURL;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR) &&
          EVAL(IsStringContained(pcszURL, pcszURLStart)));

   return(pcszURLStart);
}


PRIVATE_CODE HRESULT ApplyURLPrefix(PCSTR pcszURL, PSTR *ppszTranslatedURL)
{
   HRESULT hr = S_FALSE;
   HKEY hkeyPrefixes;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszTranslatedURL, PSTR));

   *ppszTranslatedURL = NULL;

   if (RegOpenKey(g_hkeyURLSettings, s_cszURLPrefixesKey, &hkeyPrefixes)
       == ERROR_SUCCESS)
   {
      PCSTR pcszURLStart;
      DWORD dwiValue;
      char rgchValueName[MAX_PATH_LEN];
      DWORD dwcbValueNameLen;
      DWORD dwType;
      char rgchPrefix[MAX_PATH_LEN];
      DWORD dwcbPrefixLen;

      pcszURLStart = SkipLeadingSlashes(pcszURL);

      dwcbValueNameLen = sizeof(rgchValueName);
      dwcbPrefixLen = sizeof(rgchPrefix);

      for (dwiValue = 0;
           RegEnumValue(hkeyPrefixes, dwiValue, rgchValueName,
                        &dwcbValueNameLen, NULL, &dwType, (PBYTE)rgchPrefix,
                        &dwcbPrefixLen) == ERROR_SUCCESS;
           dwiValue++)
      {
         if (! lstrnicmp(pcszURLStart, rgchValueName, dwcbValueNameLen))
         {
            DWORD cbUrl = lstrlen(pcszURLStart);

            // If the url==prefix, then we're calling the executable.
            
            if (cbUrl >= dwcbPrefixLen)
            {
                // dwcbPrefixLen includes null terminator.

                *ppszTranslatedURL = new(char[dwcbPrefixLen + cbUrl]);

                if (*ppszTranslatedURL)
                {
                   lstrcpy(*ppszTranslatedURL, rgchPrefix);
                   lstrcat(*ppszTranslatedURL, pcszURLStart);
                   // (+ 1) for null terminator.
                   ASSERT(lstrlen(*ppszTranslatedURL) + 1 == (int)dwcbPrefixLen + lstrlen(pcszURLStart));

                   hr = S_OK;
                }
                else
                   hr = E_OUTOFMEMORY;
            }

            break;
         }

         dwcbValueNameLen = sizeof(rgchValueName);
         dwcbPrefixLen = sizeof(rgchPrefix);
      }

      RegCloseKey(hkeyPrefixes);

      switch (hr)
      {
         case S_OK:
            TRACE_OUT(("ApplyURLPrefix(): Prefix %s prepended to prefix %s of %s to yield URL %s.",
                       rgchValueName,
                       rgchPrefix,
                       pcszURL,
                       *ppszTranslatedURL));
            break;

         case S_FALSE:
            TRACE_OUT(("ApplyURLPrefix(): No matching prefix found for URL %s.",
                       pcszURL));
            break;

         default:
            ASSERT(hr == E_OUTOFMEMORY);
            break;
      }
   }
   else
      TRACE_OUT(("ApplyURLPrefix(): No URL prefixes registered."));

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszTranslatedURL, STR)) ||
          ((hr == S_FALSE ||
            hr == E_OUTOFMEMORY) &&
           ! *ppszTranslatedURL));

   return(hr);
}


PRIVATE_CODE HRESULT ApplyDefaultURLPrefix(PCSTR pcszURL,
                                           PSTR *ppszTranslatedURL)
{
   HRESULT hr;
   char rgchDefaultURLPrefix[MAX_PATH_LEN];
   DWORD dwcbDefaultURLPrefixLen;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszTranslatedURL, PSTR));

   *ppszTranslatedURL = NULL;

   dwcbDefaultURLPrefixLen = sizeof(rgchDefaultURLPrefix);

   if (GetDefaultRegKeyValue(g_hkeyURLSettings, s_cszDefaultURLPrefixKey,
                             rgchDefaultURLPrefix, &dwcbDefaultURLPrefixLen)
       == ERROR_SUCCESS)
   {
      // (+ 1) for null terminator.

      *ppszTranslatedURL = new(char[dwcbDefaultURLPrefixLen +
                                    lstrlen(pcszURL) + 1]);

      if (*ppszTranslatedURL)
      {
         PCSTR pcszURLStart;

         pcszURLStart = SkipLeadingSlashes(pcszURL);

         lstrcpy(*ppszTranslatedURL, rgchDefaultURLPrefix);
         lstrcat(*ppszTranslatedURL, pcszURLStart);
         // (+ 1) for null terminator.
         ASSERT(lstrlen(*ppszTranslatedURL) + 1 == (int)dwcbDefaultURLPrefixLen + lstrlen(pcszURLStart));

         hr = S_OK;
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
      hr = S_FALSE;

   switch (hr)
   {
      case S_OK:
         TRACE_OUT(("ApplyDefaultURLPrefix(): Default prefix %s prepended to %s to yield URL %s.",
                    rgchDefaultURLPrefix,
                    pcszURL,
                    *ppszTranslatedURL));
         break;

      case S_FALSE:
         TRACE_OUT(("ApplyDefaultURLPrefix(): No default URL prefix registered."));
         break;

      default:
         ASSERT(hr == E_OUTOFMEMORY);
         break;
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszTranslatedURL, STR)) ||
          ((hr == S_FALSE ||
            hr == E_OUTOFMEMORY) &&
           ! *ppszTranslatedURL));

   return(hr);
}


PRIVATE_CODE HRESULT MyTranslateURL(PCSTR pcszURL, DWORD dwFlags,
                                    PSTR *ppszTranslatedURL)
{
   HRESULT hr;
   PARSEDURL pu;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_TRANSLATEURL_FLAGS));
   ASSERT(IS_VALID_WRITE_PTR(ppszTranslatedURL, PSTR));

   // Check URL syntax.

   pu.cbSize = sizeof(pu);
   if (ParseURL(pcszURL, &pu) == S_OK)
   {
      *ppszTranslatedURL = NULL;

      hr = S_FALSE;
   }
   else
   {
      if (IS_FLAG_SET(dwFlags, TRANSLATEURL_FL_GUESS_PROTOCOL))
         hr = ApplyURLPrefix(pcszURL, ppszTranslatedURL);
      else
         hr = S_FALSE;

      if (hr == S_FALSE &&
          IS_FLAG_SET(dwFlags, TRANSLATEURL_FL_USE_DEFAULT_PROTOCOL))
         hr = ApplyDefaultURLPrefix(pcszURL, ppszTranslatedURL);
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszTranslatedURL, STR)) ||
          ((hr == S_FALSE ||
            hr == E_OUTOFMEMORY) &&
           ! *ppszTranslatedURL));

   return(hr);
}


#ifdef DEBUG

PRIVATE_CODE IsValidPCURLINVOKECOMMANDINFO(PCURLINVOKECOMMANDINFO pcurlici)
{
   return(IS_VALID_READ_PTR(pcurlici, CURLINVOKECOMMANDINFO) &&
          EVAL(pcurlici->dwcbSize >= sizeof(*pcurlici)) &&
          FLAGS_ARE_VALID(pcurlici->dwFlags, ALL_IURL_INVOKECOMMAND_FLAGS) &&
          (IS_FLAG_CLEAR(pcurlici->dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI) ||
           IS_VALID_HANDLE(pcurlici->hwndParent, WND)) &&
          (IS_FLAG_SET(pcurlici->dwFlags, IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB) ||
           IS_VALID_STRING_PTR(pcurlici->pcszVerb, CSTR)));
}

#endif


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE InternetShortcut::RegisterProtocolHandler(
                                                            HWND hwndParent,
                                                            PSTR pszAppBuf,
                                                            UINT ucAppBufLen)
{
   HRESULT hr;
   DWORD dwFlags = 0;

   DebugEntry(InternetShortcut::RegisterProtocolHandler);

   ASSERT(! hwndParent ||
          IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszAppBuf, STR, ucAppBufLen));

   ASSERT(IS_VALID_STRING_PTR(m_pszURL, STR));

   SET_FLAG(dwFlags, URLASSOCDLG_FL_REGISTER_ASSOC);

   if (! m_pszFile)
      SET_FLAG(dwFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME);

   hr = URLAssociationDialog(hwndParent, dwFlags, m_pszFile, m_pszURL,
                             pszAppBuf, ucAppBufLen);

   switch (hr)
   {
      case S_FALSE:
         TRACE_OUT(("InternetShortcut::RegisterProtocolHandler(): One time execution of %s via %s requested.",
                    m_pszURL,
                    pszAppBuf));
         break;

      case S_OK:
         TRACE_OUT(("InternetShortcut::RegisterProtocolHandler(): Protocol handler registered for %s.",
                    m_pszURL));
         break;

      default:
         ASSERT(FAILED(hr));
         break;
   }

   ASSERT(! ucAppBufLen ||
          (IS_VALID_STRING_PTR(pszAppBuf, STR) &&
           (UINT)lstrlen(pszAppBuf) < ucAppBufLen));

   DebugExitHRESULT(InternetShortcut::RegisterProtocolHandler, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetURL(PCSTR pcszURL,
                                                   DWORD dwInFlags)
{
   HRESULT hr;
   BOOL bChanged;
   PSTR pszNewURL = NULL;

   DebugEntry(InternetShortcut::SetURL);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(! pcszURL ||
          IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_IURL_SETURL_FLAGS));

   bChanged = ! ((! pcszURL && ! m_pszURL) ||
                 (pcszURL && m_pszURL &&
                  ! lstrcmp(pcszURL, m_pszURL)));

   if (bChanged && pcszURL)
   {
      DWORD dwTranslateURLFlags;
      PSTR pszTranslatedURL;

      dwTranslateURLFlags = 0;

      if (IS_FLAG_SET(dwInFlags, IURL_SETURL_FL_GUESS_PROTOCOL))
         SET_FLAG(dwTranslateURLFlags, TRANSLATEURL_FL_GUESS_PROTOCOL);

      if (IS_FLAG_SET(dwInFlags, IURL_SETURL_FL_USE_DEFAULT_PROTOCOL))
         SET_FLAG(dwTranslateURLFlags, TRANSLATEURL_FL_USE_DEFAULT_PROTOCOL);

      hr = TranslateURL(pcszURL, dwTranslateURLFlags, &pszTranslatedURL);

      if (SUCCEEDED(hr))
      {
         PCSTR pcszURLToUse;

         // Still different?

         if (hr == S_OK)
         {
            bChanged = (lstrcmp(pszTranslatedURL, m_pszURL) != 0);

            pcszURLToUse = pszTranslatedURL;
         }
         else
         {
            ASSERT(hr == S_FALSE);

            pcszURLToUse = pcszURL;
         }

         if (bChanged)
         {
            PARSEDURL pu;

            // Validate URL syntax.

			pu.cbSize = sizeof(pu);
            hr = ParseURL(pcszURLToUse, &pu);

            if (hr == S_OK)
            {
               pszNewURL = new(char[lstrlen(pcszURLToUse) + 1]);

               if (pszNewURL)
                  lstrcpy(pszNewURL, pcszURLToUse);
               else
                  hr = E_OUTOFMEMORY;
            }
         }
         else
            hr = S_OK;
      }

      if (pszTranslatedURL)
      {
         LocalFree(pszTranslatedURL);
         pszTranslatedURL = NULL;
      }
   }
   else
      hr = S_OK;

   if (hr == S_OK)
   {
      if (bChanged)
      {
         if (m_pszURL)
            delete m_pszURL;

         m_pszURL = pszNewURL;

         Dirty(TRUE);

         TRACE_OUT(("InternetShortcut::SetURL(): Set URL to %s.",
                    CHECK_STRING(m_pszURL)));
      }
      else
         TRACE_OUT(("InternetShortcut::SetURL(): URL already %s.",
                    CHECK_STRING(m_pszURL)));
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(hr == S_OK ||
          hr == E_OUTOFMEMORY ||
          hr == URL_E_INVALID_SYNTAX);

   DebugExitHRESULT(InternetShortcut::SetURL, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetURL(PSTR *ppszURL)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetURL);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_PTR(ppszURL, PSTR));

   *ppszURL = NULL;

   if (m_pszURL)
   {
      // (+ 1) for null terminator.
      *ppszURL = (PSTR)SHAlloc(lstrlen(m_pszURL) + 1);

      if (*ppszURL)
      {
         lstrcpy(*ppszURL, m_pszURL);

         hr = S_OK;

         TRACE_OUT(("InternetShortcut::GetURL(): Got URL %s.",
                    *ppszURL));
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
      // No URL.
      hr = S_FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszURL, STR)) ||
          ((hr == S_FALSE ||
            hr == E_OUTOFMEMORY) &&
           ! *ppszURL));

   DebugExitHRESULT(InternetShortcut::GetURL, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::InvokeCommand(
                                                PURLINVOKECOMMANDINFO purlici)
{
   HRESULT hr = E_INVALIDARG;
   char szOneShotApp[MAX_PATH_LEN];
   BOOL bExecFailedWhine = FALSE;

   DebugEntry(InternetShortcut::InvokeCommand);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(purlici, CURLINVOKECOMMANDINFO));

   if (purlici && EVAL(sizeof(*purlici) == purlici->dwcbSize))
   {
      if (m_pszURL)
      {
         PSTR pszProtocol;

         hr = CopyURLProtocol(m_pszURL, &pszProtocol);

         if (hr == S_OK)
         {
            hr = IsProtocolRegistered(pszProtocol);

            if (hr == URL_E_UNREGISTERED_PROTOCOL &&
                IS_FLAG_SET(purlici->dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI))
            {
               TRACE_OUT(("InternetShortcut::InvokeCommand(): Unregistered URL protocol %s.  Invoking URL protocol handler association dialog.",
                          pszProtocol));

               hr = RegisterProtocolHandler(purlici->hwndParent, szOneShotApp,
                                            sizeof(szOneShotApp));
            }

            switch (hr)
            {
               case S_OK:
               {
                  SHELLEXECUTEINFO sei;
                  char szDefaultVerb[MAX_PATH_LEN];

                  // Execute URL via registered protocol handler.

                  ZeroMemory(&sei, sizeof(sei));

                  sei.fMask = (SEE_MASK_CLASSNAME | SEE_MASK_NO_HOOKS);

                  if (IS_FLAG_CLEAR(purlici->dwFlags,
                                    IURL_INVOKECOMMAND_FL_ALLOW_UI))
                     SET_FLAG(sei.fMask, SEE_MASK_FLAG_NO_UI);

                  if (IS_FLAG_CLEAR(purlici->dwFlags,
                                    IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB))
                     sei.lpVerb = purlici->pcszVerb;
                  else
                  {
                     if (GetClassDefaultVerb(pszProtocol, szDefaultVerb,
                                             sizeof(szDefaultVerb)))
                        sei.lpVerb = szDefaultVerb;
                     else
                        ASSERT(! sei.lpVerb);
                  }

                  sei.cbSize = sizeof(sei);
                  sei.hwnd = purlici->hwndParent;
                  sei.lpFile = m_pszURL;
                  sei.lpDirectory = m_pszWorkingDirectory;
                  sei.nShow = m_nShowCmd;
                  sei.lpClass = pszProtocol;

                  TRACE_OUT(("InternetShortcut::InvokeCommand(): Invoking %s verb on URL %s.",
                             sei.lpVerb ? sei.lpVerb : "open",
                             sei.lpFile));

                  hr = ShellExecuteEx(&sei) ? S_OK : IS_E_EXEC_FAILED;

                  if (hr == S_OK)
                     TRACE_OUT(("InternetShortcut::InvokeCommand(): ShellExecuteEx() via registered protcol handler succeeded for %s.",
                                m_pszURL));
                  else
                     WARNING_OUT(("InternetShortcut::InvokeCommand(): ShellExecuteEx() via registered protcol handler failed for %s.",
                                  m_pszURL));

                  break;
               }

               case S_FALSE:
                  hr = MyExecute(szOneShotApp, m_pszURL, 0);
                  switch (hr)
                  {
                     case E_FAIL:
                        bExecFailedWhine = TRUE;
                        hr = IS_E_EXEC_FAILED;
                        break;

                     default:
                        break;
                  }
                  break;

               default:
                  ASSERT(FAILED(hr));
                  break;
            }

            delete pszProtocol;
            pszProtocol = NULL;
         }
      }
      else
         // No URL.  Not an error.
         hr = S_FALSE;

      if (FAILED(hr) &&
          IS_FLAG_SET(purlici->dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI))
      {
         int nResult;

         switch (hr)
         {
            case IS_E_EXEC_FAILED:
               if (bExecFailedWhine)
               {
                  ASSERT(IS_VALID_STRING_PTR(szOneShotApp, STR));

                  if (MyMsgBox(purlici->hwndParent,
                               MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                               MAKEINTRESOURCE(IDS_EXEC_FAILED),
                               (MB_OK | MB_ICONEXCLAMATION), &nResult,
                               szOneShotApp)) {
                     ASSERT(nResult == IDOK);
                  }
               }
               break;

            case URL_E_INVALID_SYNTAX:
               ASSERT(IS_VALID_STRING_PTR(m_pszURL, STR));

               if (MyMsgBox(purlici->hwndParent,
                            MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                            MAKEINTRESOURCE(IDS_EXEC_INVALID_SYNTAX),
                            (MB_OK | MB_ICONEXCLAMATION), &nResult, m_pszURL)) {
                  ASSERT(nResult == IDOK);
               }

               break;

            case URL_E_UNREGISTERED_PROTOCOL:
            {
               PSTR pszProtocol;

               ASSERT(IS_VALID_STRING_PTR(m_pszURL, STR));

               if (CopyURLProtocol(m_pszURL, &pszProtocol) == S_OK)
               {
                  if (MyMsgBox(purlici->hwndParent,
                               MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                               MAKEINTRESOURCE(IDS_EXEC_UNREGISTERED_PROTOCOL),
                               (MB_OK | MB_ICONEXCLAMATION), &nResult,
                               pszProtocol)) {
                     ASSERT(nResult == IDOK);
                  }

                  delete pszProtocol;
                  pszProtocol = NULL;
               }

               break;
            }

            case E_OUTOFMEMORY:
               if (MyMsgBox(purlici->hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                            MAKEINTRESOURCE(IDS_EXEC_OUT_OF_MEMORY),
                            (MB_OK | MB_ICONEXCLAMATION), &nResult)) {
                  ASSERT(nResult == IDOK);
               }
               break;

            default:
               ASSERT(hr == E_ABORT);
               break;
         }
      }
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(hr == S_OK ||
          hr == E_ABORT ||
          hr == E_OUTOFMEMORY ||
          hr == E_INVALIDARG ||
          hr == URL_E_INVALID_SYNTAX ||
          hr == URL_E_UNREGISTERED_PROTOCOL ||
          hr == IS_E_EXEC_FAILED);

   DebugExitHRESULT(InternetShortcut::InvokeCommand, hr);

   return(hr);
}


/***************************** Exported Functions ****************************/


INTSHCUTAPI HRESULT WINAPI TranslateURLA(PCSTR pcszURL, DWORD dwInFlags,
                                         PSTR *ppszTranslatedURL)
{
   HRESULT hr;

   DebugEntry(TranslateURLA);

   *ppszTranslatedURL = NULL;

#ifdef EXPV
   /* Verify parameters. */

   if (IS_VALID_STRING_PTR(pcszURL, CSTR) &&
       IS_VALID_WRITE_PTR(ppszTranslatedURL, PSTR))
   {
      if (FLAGS_ARE_VALID(dwInFlags, ALL_TRANSLATEURL_FLAGS))
#endif
      {
         PSTR pszTempTranslatedURL;

         hr = MyTranslateURL(pcszURL, dwInFlags, &pszTempTranslatedURL);

         if (hr == S_OK)
         {
            // (+ 1) for null terminator.
            *ppszTranslatedURL = (PSTR)LocalAlloc(
                                            LMEM_FIXED,
                                            lstrlen(pszTempTranslatedURL) + 1);

            if (*ppszTranslatedURL)
            {
               lstrcpy(*ppszTranslatedURL, pszTempTranslatedURL);
               ASSERT(lstrlen(*ppszTranslatedURL) == lstrlen(pszTempTranslatedURL));
            }
            else
               hr = E_OUTOFMEMORY;

            delete pszTempTranslatedURL;
            pszTempTranslatedURL = NULL;
         }
      }
#ifdef EXPV
      else
         hr = E_FLAGS;
   }
   else
      hr = E_POINTER;
#endif

   switch (hr)
   {
      case S_FALSE:
         TRACE_OUT(("TranslateURLA(): URL %s does not require translation.",
                    pcszURL));
         break;

      case S_OK:
         TRACE_OUT(("TranslateURLA(): URL %s translated to URL %s.",
                    pcszURL,
                    *ppszTranslatedURL));
         break;

      default:
         ASSERT(hr == E_OUTOFMEMORY ||
                hr == E_POINTER);
         break;
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszTranslatedURL, STR)) ||
          ((hr == S_FALSE ||
            hr == E_OUTOFMEMORY ||
            hr == E_POINTER ||
            hr == E_FLAGS) &&
           ! *ppszTranslatedURL));

   DebugExitHRESULT(TranslateURLA, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

INTSHCUTAPI HRESULT WINAPI TranslateURLW(PCWSTR pcszURL, DWORD dwInFlags,
                                         PWSTR UNALIGNED *ppszTranslatedURL)
{
   HRESULT hr;

   DebugEntry(TranslateURLW);

   SetLastError(ERROR_NOT_SUPPORTED);
   hr = E_NOTIMPL;

   DebugExitHRESULT(TranslateURLW, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */

