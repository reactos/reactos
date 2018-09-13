/*
 * shlexhk.cpp - IShellExecuteHook implementation for URL class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA const char s_cszOpenVerb[]   = "open";

#pragma data_seg()


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE InternetShortcut::Execute(PSHELLEXECUTEINFO pei)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::Execute);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pei, CSHELLEXECUTEINFO));

   if (! pei->lpVerb ||
       ! lstrcmpi(pei->lpVerb, s_cszOpenVerb))
   {
      if (pei->lpFile)
      {
         PSTR pszTranslatedURL;

         hr = TranslateURL(pei->lpFile, TRANSLATEURL_FL_GUESS_PROTOCOL,
                           &pszTranslatedURL);

         if (SUCCEEDED(hr))
         {
            PCSTR pcszURLToUse;

            pcszURLToUse = (hr == S_OK) ? pszTranslatedURL : pei->lpFile;

            hr = ValidateURL(pcszURLToUse);

            if (hr == S_OK)
            {
               hr = SetURL(pcszURLToUse, 0);

               if (hr == S_OK)
               {
                  URLINVOKECOMMANDINFO urlici;

                  EVAL(SetShowCmd(pei->nShow) == S_OK);

                  urlici.dwcbSize = sizeof(urlici);
                  urlici.hwndParent = pei->hwnd;
                  urlici.pcszVerb = NULL;

                  urlici.dwFlags = IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB;

                  if (IS_FLAG_CLEAR(pei->fMask, SEE_MASK_FLAG_NO_UI))
                     SET_FLAG(urlici.dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI);

                  hr = InvokeCommand(&urlici);

                  if (hr != S_OK)
                  {
                     SET_FLAG(pei->fMask, SEE_MASK_FLAG_NO_UI);

                     TRACE_OUT(("InternetShortcut::Execute(): InternetShortcut::InvokeCommand() failed, returning %s.  Clearing SEE_MASK_FLAG_NO_UI to avoid error ui from ShellExecute().",
                                GetHRESULTString(hr)));
                  }
               }
            }

            if (pszTranslatedURL)
            {
               LocalFree(pszTranslatedURL);
               pszTranslatedURL = NULL;
            }
         }
      }
      else
         // This hook only handles execution of file string, not IDList.
         hr = S_FALSE;
   }
   else
      // Unrecognized verb.
      hr = S_FALSE;

   if (hr == S_OK)
      pei->hInstApp = (HINSTANCE)42;
   else if (FAILED(hr))
   {
      switch (hr)
      {
         case URL_E_INVALID_SYNTAX:
         case URL_E_UNREGISTERED_PROTOCOL:
            hr = S_FALSE;
            break;

         case E_OUTOFMEMORY:
            pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
            hr = E_FAIL;
            break;

         case IS_E_EXEC_FAILED:
            // Translate execution failure into "file not found".
            pei->hInstApp = (HINSTANCE)SE_ERR_FNF;
            hr = E_FAIL;
            break;

         default:
            // pei->lpFile is bogus.  Treat as file not found.
            ASSERT(hr == E_POINTER);
            // We should never get E_FLAGS from TranslateURL() here.
            pei->hInstApp = (HINSTANCE)SE_ERR_FNF;
            hr = E_FAIL;
            break;
      }
   }
   else
      ASSERT(hr == S_FALSE);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pei, CSHELLEXECUTEINFO));
   ASSERT(hr == S_OK ||
          hr == S_FALSE ||
          hr == E_FAIL);

   DebugExitHRESULT(InternetShortcut::Execute, hr);

   return(hr);
}

