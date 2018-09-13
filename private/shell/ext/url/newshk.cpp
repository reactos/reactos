/*
 * newshk.cpp - INewShortcutHook implementation for URL class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "resource.h"

#include <mluisupp.h>

extern "C"{
extern BOOL  lPathYetAnotherMakeUniqueNameA(LPSTR,LPCSTR,LPCSTR,LPCSTR);
}

/********************************** Methods **********************************/


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::SetReferent(PCSTR pcszReferent,
                                                        HWND hwndParent)
{
   HRESULT hr;
   PSTR pszTranslatedURL;

   DebugEntry(InternetShortcut::SetReferent);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszReferent, CSTR));
   ASSERT(IS_VALID_HANDLE(hwndParent, WND));

   hr = TranslateURL(pcszReferent, TRANSLATEURL_FL_GUESS_PROTOCOL,
                     &pszTranslatedURL);

   if (SUCCEEDED(hr))
   {
      PCSTR pcszURLToUse;

      pcszURLToUse = (hr == S_OK) ? pszTranslatedURL : pcszReferent;

      hr = ValidateURL(pcszURLToUse);

      if (hr == S_OK)
         hr = SetURL(pcszURLToUse, 0);

      if (pszTranslatedURL)
      {
         LocalFree(pszTranslatedURL);
         pszTranslatedURL = NULL;
      }
   }

   if (hr == S_OK)
      TRACE_OUT(("InternetShortcut::SetReferent(): Set referent %s as URL %s.",
                 pcszReferent,
                 m_pszURL));
   else
   {
      ASSERT(FAILED(hr));

      switch (hr)
      {
         case URL_E_INVALID_SYNTAX:
         case URL_E_UNREGISTERED_PROTOCOL:
            hr = S_FALSE;
            break;

         default:
            break;
      }

      TRACE_OUT(("InternetShortcut::SetReferent(): Failed to set referent to %s.",
                 pcszReferent));
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetReferent, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::GetReferent(PSTR pszReferent,
                                                        int ncReferentBufLen)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetReferent);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszReferent, STR, ncReferentBufLen));

   if (m_pszURL)
   {
      if (lstrlen(m_pszURL) < ncReferentBufLen)
      {
         lstrcpy(pszReferent, m_pszURL);

         hr = S_OK;

         TRACE_OUT(("InternetShortcut::GetReferent(): Returning referent %s.",
                    pszReferent));
      }
      else
         hr = E_FAIL;
   }
   else
      hr = S_FALSE;

   if (hr != S_OK)
   {
      if (ncReferentBufLen > 0)
         *pszReferent = '\0';
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(pszReferent, STR) &&
           EVAL(lstrlen(pszReferent) < ncReferentBufLen)) ||
          ((hr == S_FALSE ||
            hr == E_FAIL) &&
           EVAL(! ncReferentBufLen ||
                ! *pszReferent)));

   DebugExitHRESULT(InternetShortcut::GetReferent, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetFolder(PCSTR pcszFolder)
{
   HRESULT hr;
   PSTR pszFolderCopy;

   DebugEntry(InternetShortcut::SetFolder);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IsPathDirectory(pcszFolder));

   // (+ 1) for null terminator.
   pszFolderCopy = new(char[lstrlen(pcszFolder) + 1]);

   if (pszFolderCopy)
   {
      lstrcpy(pszFolderCopy, pcszFolder);

      if (m_pszFolder)
         delete m_pszFolder;

      m_pszFolder = pszFolderCopy;

      hr = S_OK;

      TRACE_OUT(("InternetShortcut::SetFolder(): Set folder to %s.",
                 m_pszFolder));
   }
   else
      hr = E_OUTOFMEMORY;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetFolder, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetFolder(PSTR pszFolder,
                                                      int ncFolderBufLen)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetFolder);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszFolder, STR, ncFolderBufLen));

   if (m_pszFolder)
   {
      if (lstrlen(m_pszFolder) < ncFolderBufLen)
      {
         lstrcpy(pszFolder, m_pszFolder);

         hr = S_OK;

         TRACE_OUT(("InternetShortcut::GetFolder(): Returning folder %s.",
                    pszFolder));
      }
      else
         hr = E_FAIL;
   }
   else
      hr = S_FALSE;

   if (hr != S_OK)
   {
      if (ncFolderBufLen > 0)
         *pszFolder = '\0';
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(pszFolder, STR) &&
           EVAL(lstrlen(pszFolder) < ncFolderBufLen)) ||
          ((hr == S_FALSE ||
            hr == E_FAIL) &&
           EVAL(! ncFolderBufLen ||
                ! *pszFolder)));

   DebugExitHRESULT(InternetShortcut::GetFolder, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetName(PSTR pszName,
                                                    int ncNameBufLen)
{
   HRESULT hr = E_FAIL;
   char rgchShortName[MAX_PATH_LEN];

   DebugEntry(InternetShortcut::GetName);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszName, STR, ncNameBufLen));

   hr = E_FAIL;

   if (MLLoadStringA(IDS_SHORT_NEW_INTERNET_SHORTCUT,
                  rgchShortName, sizeof(rgchShortName)))
   {
      char rgchLongName[MAX_PATH_LEN];

      if (MLLoadStringA(IDS_NEW_INTERNET_SHORTCUT,
                     rgchLongName, sizeof(rgchLongName)))
      {
         char rgchCurDir[MAX_PATH_LEN];
         PCSTR pcszFolderToUse;

         // Use current directory if m_pszFolder has not been set.

         pcszFolderToUse = m_pszFolder;

         if (! pcszFolderToUse)
         {
            if (GetCurrentDirectory(sizeof(rgchCurDir), rgchCurDir) > 0)
               pcszFolderToUse = rgchCurDir;
         }

         if (pcszFolderToUse)
         {
            char rgchUniqueName[MAX_PATH_LEN];

            if (lPathYetAnotherMakeUniqueNameA(rgchUniqueName, pcszFolderToUse,
                                             rgchShortName, rgchLongName))
            {
               PSTR pszFileName;
               PSTR pszRemoveExt;

               pszFileName = (PSTR)ExtractFileName(rgchUniqueName);
               pszRemoveExt = (PSTR)ExtractExtension(pszFileName);
               *pszRemoveExt = '\0';

               if (lstrlen(pszFileName) < ncNameBufLen)
               {
                  lstrcpy(pszName, pszFileName);

                  hr = S_OK;
               }
            }
         }
      }
   }

   if (hr == S_OK) {
      TRACE_OUT(("InternetShortcut::GetName(): Returning %s.",
                 pszName));
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(pszName, STR) &&
           EVAL(lstrlen(pszName) < ncNameBufLen)) ||
          (hr == E_FAIL &&
           (! ncNameBufLen ||
            ! *pszName)));

   DebugExitHRESULT(InternetShortcut::GetName, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetExtension(PSTR pszExtension,
                                                         int ncExtensionBufLen)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetExtension);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszExtension, STR, ncExtensionBufLen));

   if (lstrlen(g_cszURLExt) < ncExtensionBufLen)
   {
      lstrcpy(pszExtension, g_cszURLExt);

      hr = S_OK;

      TRACE_OUT(("InternetShortcut::GetExtension(): Returning extension %s.",
                 pszExtension));
   }
   else
   {
      if (ncExtensionBufLen > 0)
         *pszExtension = '\0';

      hr = E_FAIL;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(pszExtension, STR) &&
           EVAL(lstrlen(pszExtension) < ncExtensionBufLen)) ||
          (hr == E_FAIL &&
           EVAL(! ncExtensionBufLen ||
                ! *pszExtension)));

   DebugExitHRESULT(InternetShortcut::GetExtension, hr);

   return(hr);
}

