/*
 * persist.cpp - IPersist, IPersistFile, and IPersistStream implementations for
 *               URL class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "resource.h"

#include <mluisupp.h>

/* Global Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

extern const UINT g_ucMaxURLLen                    = 1024;

extern const char g_cszURLPrefix[]                 = "url:";
extern const UINT g_ucbURLPrefixLen                = sizeof(g_cszURLPrefix) - 1;

extern const char g_cszURLExt[]                    = ".url";
extern const char g_cszURLDefaultFileNamePrompt[]  = "*.url";

extern const char g_cszCRLF[]                      = "\r\n";

#pragma data_seg()


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

// case-insensitive

PRIVATE_DATA const char s_cszInternetShortcutSection[] = "InternetShortcut";

PRIVATE_DATA const char s_cszURLKey[]              = "URL";
PRIVATE_DATA const char s_cszIconFileKey[]         = "IconFile";
PRIVATE_DATA const char s_cszIconIndexKey[]        = "IconIndex";
PRIVATE_DATA const char s_cszHotkeyKey[]           = "Hotkey";
PRIVATE_DATA const char s_cszWorkingDirectoryKey[] = "WorkingDirectory";
PRIVATE_DATA const char s_cszShowCmdKey[]          = "ShowCommand";

PRIVATE_DATA const UINT s_ucMaxIconIndexLen        = 1 + 10 + 1; // -2147483647
PRIVATE_DATA const UINT s_ucMaxHotkeyLen           = s_ucMaxIconIndexLen;
PRIVATE_DATA const UINT s_ucMaxShowCmdLen          = s_ucMaxIconIndexLen;

#pragma data_seg()


/***************************** Private Functions *****************************/


PRIVATE_CODE BOOL DeletePrivateProfileString(PCSTR pcszSection, PCSTR pcszKey,
                                             PCSTR pcszFile)
{
   ASSERT(IS_VALID_STRING_PTR(pcszSection, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszKey, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   return(WritePrivateProfileString(pcszSection, pcszKey, NULL, pcszFile));
}

#define SHDeleteIniString(pcszSection, pcszKey, pcszFile) \
           SHSetIniString(pcszSection, pcszKey, NULL, pcszFile)

PRIVATE_CODE HRESULT MassageURL(PSTR pszURL)
{
   HRESULT hr = E_FAIL;

   ASSERT(IS_VALID_STRING_PTR(pszURL, STR));

   TrimWhiteSpace(pszURL);

   PSTR pszBase = pszURL;
   PSTR psz;

   // Skip over any "url:" prefix.

   if (! lstrnicmp(pszBase, g_cszURLPrefix, g_ucbURLPrefixLen))
      pszBase += g_ucbURLPrefixLen;

   lstrcpy(pszURL, pszBase);
   hr = S_OK;

   TRACE_OUT(("MassageURL(): Massaged URL to %s.",
              pszURL));

   ASSERT(FAILED(hr) ||
          IS_VALID_STRING_PTR(pszURL, STR));

   return(hr);
}


PRIVATE_CODE HRESULT ReadURLFromFile(PCSTR pcszFile, PSTR *ppszURL)
{
   HRESULT hr;
   PSTR pszNewURL;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszURL, PSTR));

   *ppszURL = NULL;

   pszNewURL = new(char[g_ucMaxURLLen]);

   if (pszNewURL)
   {
      DWORD dwcValueLen;

      dwcValueLen = SHGetIniString(s_cszInternetShortcutSection,
                                   s_cszURLKey,
                                   pszNewURL, g_ucMaxURLLen, pcszFile);

      if (dwcValueLen > 0)
      {
         hr = MassageURL(pszNewURL);

         if (hr == S_OK)
         {
            PSTR pszShorterURL;

            // (+ 1) for null terminator.

            if (ReallocateMemory(pszNewURL, lstrlen(pszNewURL) + 1,
                                 (PVOID *)&pszShorterURL))
            {
               *ppszURL = pszShorterURL;

               hr = S_OK;
            }
            else
               hr = E_OUTOFMEMORY;
         }
      }
      else
      {
         hr = S_FALSE;

         WARNING_OUT(("ReadURLFromFile: No URL found in file %s.",
                      pcszFile));
      }
   }
   else
      hr = E_OUTOFMEMORY;

   if (FAILED(hr) ||
       hr == S_FALSE)
   {
      if (pszNewURL)
      {
         delete pszNewURL;
         pszNewURL = NULL;
      }
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszURL, STR)) ||
          (hr != S_OK &&
           ! *ppszURL));

   return(hr);
}


PRIVATE_CODE HRESULT WriteURLToFile(PCSTR pcszFile, PCSTR pcszURL)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszURL ||
          IS_VALID_STRING_PTR(pcszURL, CSTR));

   if (AnyMeat(pcszURL))
   {
      int ncbLen;

      ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
      ASSERT(IS_VALID_STRING_PTR(pcszURL, PSTR));

      hr = (SHSetIniString(s_cszInternetShortcutSection, s_cszURLKey, pcszURL, pcszFile))
           ? S_OK
           : E_FAIL;
   }
   else
      hr = (SHDeleteIniString(s_cszInternetShortcutSection, s_cszURLKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}


PRIVATE_CODE HRESULT ReadIconLocationFromFile(PCSTR pcszFile,
                                              PSTR *ppszIconFile, PINT pniIcon)
{
   HRESULT hr;
   char rgchNewIconFile[MAX_PATH_LEN];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszIconFile, PSTR));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

   *ppszIconFile = NULL;
   *pniIcon = 0;

   dwcValueLen = SHGetIniString(s_cszInternetShortcutSection,
                                s_cszIconFileKey,
                                rgchNewIconFile,
                                sizeof(rgchNewIconFile), pcszFile);

   if (dwcValueLen > 0)
   {
      char rgchNewIconIndex[s_ucMaxIconIndexLen];

      dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                            s_cszIconIndexKey,
                                            EMPTY_STRING, rgchNewIconIndex,
                                            sizeof(rgchNewIconIndex),
                                            pcszFile);

      if (dwcValueLen > 0)
      {
         int niIcon;

         if (StrToIntEx(rgchNewIconIndex, 0, &niIcon))
         {
            // (+ 1) for null terminator.

            *ppszIconFile = new(char[lstrlen(rgchNewIconFile) + 1]);

            if (*ppszIconFile)
            {
               lstrcpy(*ppszIconFile, rgchNewIconFile);
               *pniIcon = niIcon;

               hr = S_OK;
            }
            else
               hr = E_OUTOFMEMORY;
         }
         else
         {
            hr = S_FALSE;

            WARNING_OUT(("ReadIconLocationFromFile(): Bad icon index \"%s\" found in file %s.",
                         rgchNewIconIndex,
                         pcszFile));
         }
      }
      else
      {
         hr = S_FALSE;

         WARNING_OUT(("ReadIconLocationFromFile(): No icon index found in file %s.",
                      pcszFile));
      }
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadIconLocationFromFile(): No icon file found in file %s.",
                 pcszFile));
   }

   ASSERT(IsValidIconIndex(hr, *ppszIconFile, MAX_PATH_LEN, *pniIcon));

   return(hr);
}


PRIVATE_CODE HRESULT WriteIconLocationToFile(PCSTR pcszFile,
                                             PCSTR pcszIconFile,
                                             int niIcon)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszIconFile ||
          IS_VALID_STRING_PTR(pcszIconFile, CSTR));
   ASSERT(IsValidIconIndex((pcszIconFile ? S_OK : S_FALSE), pcszIconFile, MAX_PATH_LEN, niIcon));

   if (AnyMeat(pcszIconFile))
   {
      char rgchIconIndexRHS[s_ucMaxIconIndexLen];
      int ncLen;

      ncLen = wsprintf(rgchIconIndexRHS, "%d", niIcon);
      ASSERT(ncLen > 0);
      ASSERT(ncLen < sizeof(rgchIconIndexRHS));
      ASSERT(ncLen == lstrlen(rgchIconIndexRHS));

      hr = (SHSetIniString(s_cszInternetShortcutSection,
                           s_cszIconFileKey, pcszIconFile,
                           pcszFile) &&
            WritePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszIconIndexKey, rgchIconIndexRHS,
                                      pcszFile))
           ? S_OK
           : E_FAIL;
   }
   else
      hr = (SHDeleteIniString(s_cszInternetShortcutSection,
                              s_cszIconFileKey, pcszFile) &&
            DeletePrivateProfileString(s_cszInternetShortcutSection,
                                       s_cszIconIndexKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}


PRIVATE_CODE HRESULT ReadHotkeyFromFile(PCSTR pcszFile, PWORD pwHotkey)
{
   HRESULT hr = S_FALSE;
   char rgchHotkey[s_ucMaxHotkeyLen];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pwHotkey, WORD));

   *pwHotkey = 0;

   dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                         s_cszHotkeyKey, EMPTY_STRING,
                                         rgchHotkey, sizeof(rgchHotkey),
                                         pcszFile);

   if (dwcValueLen > 0)
   {
      UINT uHotkey;

      if (StrToIntEx(rgchHotkey, 0, (int *)&uHotkey))
      {
         *pwHotkey = (WORD)uHotkey;

         hr = S_OK;
      }
      else
         WARNING_OUT(("ReadHotkeyFromFile(): Bad hotkey \"%s\" found in file %s.",
                      rgchHotkey,
                      pcszFile));
   }
   else
      WARNING_OUT(("ReadHotkeyFromFile(): No hotkey found in file %s.",
                   pcszFile));

   ASSERT((hr == S_OK &&
           IsValidHotkey(*pwHotkey)) ||
          (hr == S_FALSE &&
           ! *pwHotkey));

   return(hr);
}


PRIVATE_CODE HRESULT WriteHotkeyToFile(PCSTR pcszFile, WORD wHotkey)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! wHotkey ||
          IsValidHotkey(wHotkey));

   if (wHotkey)
   {
      char rgchHotkeyRHS[s_ucMaxHotkeyLen];
      int ncLen;

      ncLen = wsprintf(rgchHotkeyRHS, "%u", (UINT)wHotkey);
      ASSERT(ncLen > 0);
      ASSERT(ncLen < sizeof(rgchHotkeyRHS));
      ASSERT(ncLen == lstrlen(rgchHotkeyRHS));

      hr = WritePrivateProfileString(s_cszInternetShortcutSection,
                                     s_cszHotkeyKey, rgchHotkeyRHS,
                                     pcszFile)
           ? S_OK
           : E_FAIL;
   }
   else
      hr = DeletePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszHotkeyKey, pcszFile)
           ? S_OK
           : E_FAIL;

   return(hr);
}


PRIVATE_CODE HRESULT ReadWorkingDirectoryFromFile(PCSTR pcszFile,
                                                  PSTR *ppszWorkingDirectory)
{
   HRESULT hr;
   char rgchDirValue[MAX_PATH_LEN];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszWorkingDirectory, PSTR));

   *ppszWorkingDirectory = NULL;

   dwcValueLen = SHGetIniString(s_cszInternetShortcutSection,
                                s_cszWorkingDirectoryKey,
                                rgchDirValue,
                                sizeof(rgchDirValue), pcszFile);

   if (dwcValueLen > 0)
   {
      char rgchFullPath[MAX_PATH_LEN];
      PSTR pszFileName;

      if (GetFullPathName(rgchDirValue, sizeof(rgchFullPath), rgchFullPath,
                          &pszFileName) > 0)
      {
         // (+ 1) for null terminator.

         *ppszWorkingDirectory = new(char[lstrlen(rgchFullPath) + 1]);

         if (*ppszWorkingDirectory)
         {
            lstrcpy(*ppszWorkingDirectory, rgchFullPath);

            hr = S_OK;
         }
         else
            hr = E_OUTOFMEMORY;
      }
      else
         hr = E_FAIL;
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadWorkingDirectoryFromFile: No working directory found in file %s.",
                 pcszFile));
   }

   ASSERT(IsValidPathResult(hr, *ppszWorkingDirectory, MAX_PATH_LEN));

   return(hr);
}


PRIVATE_CODE HRESULT WriteWorkingDirectoryToFile(PCSTR pcszFile,
                                                 PCSTR pcszWorkingDirectory)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszWorkingDirectory ||
          IS_VALID_STRING_PTR(pcszWorkingDirectory, CSTR));

   if (AnyMeat(pcszWorkingDirectory))
      hr = (SHSetIniString(s_cszInternetShortcutSection,
                           s_cszWorkingDirectoryKey,
                           pcszWorkingDirectory, pcszFile))
           ? S_OK
           : E_FAIL;
   else
      hr = (SHDeleteIniString(s_cszInternetShortcutSection,
                              s_cszWorkingDirectoryKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}


PRIVATE_CODE HRESULT ReadShowCmdFromFile(PCSTR pcszFile, PINT pnShowCmd)
{
   HRESULT hr;
   char rgchNewShowCmd[s_ucMaxShowCmdLen];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pnShowCmd, INT));

   *pnShowCmd = g_nDefaultShowCmd;

   dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                         s_cszShowCmdKey, EMPTY_STRING,
                                         rgchNewShowCmd,
                                         sizeof(rgchNewShowCmd), pcszFile);

   if (dwcValueLen > 0)
   {
      int nShowCmd;

      if (StrToIntEx(rgchNewShowCmd, 0, &nShowCmd))
      {
         *pnShowCmd = nShowCmd;

         hr = S_OK;
      }
      else
      {
         hr = S_FALSE;

         WARNING_OUT(("ReadShowCmdFromFile: Invalid show command \"%s\" found in file %s.",
                      rgchNewShowCmd,
                      pcszFile));
      }
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadShowCmdFromFile: No show command found in file %s.",
                 pcszFile));
   }

   ASSERT((hr == S_OK &&
           EVAL(IsValidShowCmd(*pnShowCmd))) ||
          (hr == S_FALSE &&
           EVAL(*pnShowCmd == g_nDefaultShowCmd)));

   return(hr);
}


PRIVATE_CODE HRESULT WriteShowCmdToFile(PCSTR pcszFile, int nShowCmd)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IsValidShowCmd(nShowCmd));

   if (nShowCmd != g_nDefaultShowCmd)
   {
      char rgchShowCmdRHS[s_ucMaxShowCmdLen];
      int ncLen;

      ncLen = wsprintf(rgchShowCmdRHS, "%d", nShowCmd);
      ASSERT(ncLen > 0);
      ASSERT(ncLen < sizeof(rgchShowCmdRHS));
      ASSERT(ncLen == lstrlen(rgchShowCmdRHS));

      hr = (WritePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszShowCmdKey, rgchShowCmdRHS,
                                      pcszFile))
           ? S_OK
           : E_FAIL;
   }
   else
      hr = (DeletePrivateProfileString(s_cszInternetShortcutSection,
                                       s_cszShowCmdKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}


/****************************** Public Functions *****************************/


PUBLIC_CODE HRESULT UnicodeToANSI(LPCOLESTR pcwszUnicode, PSTR *ppszANSI)
{
   HRESULT hr;
   int ncbLen;

   // BUGBUG: Need OLESTR validation function to validate pcwszUnicode here.
   ASSERT(IS_VALID_WRITE_PTR(ppszANSI, PSTR));

   *ppszANSI = NULL;

   // Get length of translated string.

   ncbLen = WideCharToMultiByte(CP_ACP, 0, pcwszUnicode, -1, NULL, 0, NULL,
                                NULL);

   if (ncbLen > 0)
   {
      PSTR pszNewANSI;

      // (+ 1) for null terminator.

      pszNewANSI = new(char[ncbLen]);

      if (pszNewANSI)
      {
         // Translate string.

         if (WideCharToMultiByte(CP_ACP, 0, pcwszUnicode, -1, pszNewANSI,
                                 ncbLen, NULL, NULL) > 0)
         {
            *ppszANSI = pszNewANSI;
            hr = S_OK;
         }
         else
         {
            delete pszNewANSI;
            pszNewANSI = NULL;

            hr = E_UNEXPECTED;

            WARNING_OUT(("UnicodeToANSI(): Failed to translate Unicode string to ANSI."));
         }
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
   {
      hr = E_UNEXPECTED;

      WARNING_OUT(("UnicodeToANSI(): Failed to get length of translated ANSI string."));
   }

   ASSERT(FAILED(hr) ||
          IS_VALID_STRING_PTR(*ppszANSI, STR));

   return(hr);
}


PUBLIC_CODE HRESULT ANSIToUnicode(PCSTR pcszANSI, LPOLESTR *ppwszUnicode)
{
   HRESULT hr;
   int ncbWideLen;

   ASSERT(IS_VALID_STRING_PTR(pcszANSI, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppwszUnicode, LPOLESTR));

   *ppwszUnicode = NULL;

   // Get length of translated string.

   ncbWideLen = MultiByteToWideChar(CP_ACP, 0, pcszANSI, -1, NULL, 0);

   if (ncbWideLen > 0)
   {
      PWSTR pwszNewUnicode;

      // (+ 1) for null terminator.

      pwszNewUnicode = new(WCHAR[ncbWideLen]);

      if (pwszNewUnicode)
      {
         // Translate string.

         if (MultiByteToWideChar(CP_ACP, 0, pcszANSI, -1, pwszNewUnicode,
                                 ncbWideLen) > 0)
         {
            *ppwszUnicode = pwszNewUnicode;
            hr = S_OK;
         }
         else
         {
            delete pwszNewUnicode;
            pwszNewUnicode = NULL;

            hr = E_UNEXPECTED;

            WARNING_OUT(("ANSIToUnicode(): Failed to translate ANSI path string to Unicode."));
         }
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
   {
      hr = E_UNEXPECTED;

      WARNING_OUT(("ANSIToUnicode(): Failed to get length of translated Unicode string."));
   }

   // BUGBUG: Need OLESTR validation function to validate *ppwszUnicode here.

   return(hr);
}


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE InternetShortcut::SaveToFile(PCSTR pcszFile,
                                                       BOOL bRemember)
{
   HRESULT hr;
   PSTR pszURL;

   DebugEntry(InternetShortcut::SaveToFile);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   hr = GetURL(&pszURL);

   if (SUCCEEDED(hr))
   {
      hr = WriteURLToFile(pcszFile, pszURL);

      if (pszURL)
      {
         SHFree(pszURL);
         pszURL = NULL;
      }

      if (hr == S_OK)
      {
         char rgchBuf[MAX_PATH_LEN];
         int niIcon;

         hr = GetIconLocation(rgchBuf, sizeof(rgchBuf), &niIcon);

         if (SUCCEEDED(hr))
         {
            hr = WriteIconLocationToFile(pcszFile, rgchBuf, niIcon);

            if (hr == S_OK)
            {
               WORD wHotkey;

               hr = GetHotkey(&wHotkey);

               if (SUCCEEDED(hr))
               {
                  hr = WriteHotkeyToFile(pcszFile, wHotkey);

                  if (hr == S_OK)
                  {
                     hr = GetWorkingDirectory(rgchBuf, sizeof(rgchBuf));

                     if (SUCCEEDED(hr))
                     {
                        hr = WriteWorkingDirectoryToFile(pcszFile, rgchBuf);

                        if (hr == S_OK)
                        {
                           int nShowCmd;

                           GetShowCmd(&nShowCmd);

                           hr = WriteShowCmdToFile(pcszFile, nShowCmd);

                           if (hr == S_OK)
                           {
                              /* Remember file if requested. */

                              if (bRemember)
                              {
                                 PSTR pszFileCopy;

                                 if (StringCopy(pcszFile, &pszFileCopy))
                                 {
                                    if (m_pszFile)
                                       delete m_pszFile;

                                    m_pszFile = pszFileCopy;

                                    TRACE_OUT(("InternetShortcut::SaveToFile(): Remembering file %s, as requested.",
                                               m_pszFile));
                                 }
                                 else
                                    hr = E_OUTOFMEMORY;
                              }

                              if (hr == S_OK)
                              {
                                 Dirty(FALSE);

                                 SHChangeNotify(SHCNE_UPDATEITEM,
                                                (SHCNF_PATH | SHCNF_FLUSH), pcszFile,
                                                NULL);

#ifdef DEBUG
                                 TRACE_OUT(("InternetShortcut::SaveToFile(): Internet Shortcut saved to file %s:",
                                            pcszFile));
                                 Dump();
#endif
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SaveToFile, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::LoadFromFile(PCSTR pcszFile,
                                                         BOOL bRemember)
{
   HRESULT hr;
   PSTR pszURL;

   DebugEntry(InternetShortcut::LoadFromFile);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   hr = ReadURLFromFile(pcszFile, &pszURL);

   if (SUCCEEDED(hr))
   {
      hr = SetURL(pszURL, (IURL_SETURL_FL_GUESS_PROTOCOL |
                           IURL_SETURL_FL_USE_DEFAULT_PROTOCOL));

      if (pszURL)
      {
         delete pszURL;
         pszURL = NULL;
      }

      if (hr == S_OK)
      {
         PSTR pszIconFile;
         int niIcon;

         hr = ReadIconLocationFromFile(pcszFile, &pszIconFile, &niIcon);

         if (SUCCEEDED(hr))
         {
            hr = SetIconLocation(pszIconFile, niIcon);

            if (pszIconFile)
            {
               delete pszIconFile;
               pszIconFile = NULL;
            }

            if (hr == S_OK)
            {
               WORD wHotkey;

               hr = ReadHotkeyFromFile(pcszFile, &wHotkey);

               if (SUCCEEDED(hr))
               {
                  hr = SetHotkey(wHotkey);

                  if (hr == S_OK)
                  {
                     PSTR pszWorkingDirectory;

                     hr = ReadWorkingDirectoryFromFile(pcszFile,
                                                       &pszWorkingDirectory);

                     if (SUCCEEDED(hr))
                     {
                        hr = SetWorkingDirectory(pszWorkingDirectory);

                        if (pszWorkingDirectory)
                        {
                           delete pszWorkingDirectory;
                           pszWorkingDirectory = NULL;
                        }

                        if (hr == S_OK)
                        {
                           int nShowCmd;

                           hr = ReadShowCmdFromFile(pcszFile, &nShowCmd);

                           if (SUCCEEDED(hr))
                           {
                              /* Remember file if requested. */

                              if (bRemember)
                              {
                                 PSTR pszFileCopy;

                                 if (StringCopy(pcszFile, &pszFileCopy))
                                 {
                                    if (m_pszFile)
                                       delete m_pszFile;

                                    m_pszFile = pszFileCopy;

                                    TRACE_OUT(("InternetShortcut::LoadFromFile(): Remembering file %s, as requested.",
                                               m_pszFile));
                                 }
                                 else
                                    hr = E_OUTOFMEMORY;
                              }

                              if (SUCCEEDED(hr))
                              {
                                 SetShowCmd(nShowCmd);

                                 Dirty(FALSE);

                                 hr = S_OK;

#ifdef DEBUG
                                 TRACE_OUT(("InternetShortcut::LoadFromFile(): Internet Shortcut loaded from file %s:",
                                            pcszFile));
                                 Dump();
#endif
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::LoadFromFile, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetCurFile(PSTR pszFile,
                                                       UINT ucbLen)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetCurFile);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszFile, STR, ucbLen));

   if (m_pszFile)
   {
      lstrcpyn(pszFile, m_pszFile, ucbLen);

      TRACE_OUT(("InternetShortcut::GetCurFile(): Current file name is %s.",
                 pszFile));

      hr = S_OK;
   }
   else
      hr = S_FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pszFile, STR) &&
          EVAL((UINT)lstrlen(pszFile) < ucbLen));
   ASSERT(hr == S_OK ||
          hr == S_FALSE);

   DebugExitHRESULT(InternetShortcut::GetCurFile, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::Dirty(BOOL bDirty)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::Dirty);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   if (bDirty)
   {
      if (IS_FLAG_CLEAR(m_dwFlags, INTSHCUT_FL_DIRTY)) {
         TRACE_OUT(("InternetShortcut::Dirty(): Now dirty."));
      }

      SET_FLAG(m_dwFlags, INTSHCUT_FL_DIRTY);
   }
   else
   {
      if (IS_FLAG_SET(m_dwFlags, INTSHCUT_FL_DIRTY)) {
         TRACE_OUT(("InternetShortcut::Dirty(): Now clean."));
      }

      CLEAR_FLAG(m_dwFlags, INTSHCUT_FL_DIRTY);
   }

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(hr == S_OK);

   DebugExitVOID(InternetShortcut::Dirty);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetClassID(PCLSID pclsid)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetClassID);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pclsid, CCLSID));

   *pclsid = CLSID_InternetShortcut;
   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(pclsid, CCLSID));

   DebugExitHRESULT(InternetShortcut::GetClassID, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::IsDirty(void)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::IsDirty);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   if (IS_FLAG_SET(m_dwFlags, INTSHCUT_FL_DIRTY))
      hr = S_OK;
   else
      hr = S_FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::IsDirty, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::Save(LPCOLESTR pcwszFile,
                                                 BOOL bRemember)
{
   HRESULT hr;
   PSTR pszFile;

   DebugEntry(InternetShortcut::Save);

   // bRemember may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   // BUGBUG: Need OLESTR validation function to validate pcwszFile here.

   if (pcwszFile)
   {
      hr = UnicodeToANSI(pcwszFile, &pszFile);

      if (hr == S_OK)
      {
         hr = SaveToFile(pszFile, bRemember);

         delete pszFile;
         pszFile = NULL;
      }
   }
   else if (m_pszFile)
      // Ignore bRemember.
      hr = SaveToFile(m_pszFile, FALSE);
   else
      hr = E_FAIL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::Save, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::SaveCompleted(LPCOLESTR pcwszFile)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SaveCompleted);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   // BUGBUG: Need OLESTR validation function to validate pcwszFile here.

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SaveCompleted, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::Load(LPCOLESTR pcwszFile,
                                                 DWORD dwMode)
{
   HRESULT hr;
   PSTR pszFile;

   DebugEntry(InternetShortcut::Load);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   // BUGBUG: Need OLESTR validation function to validate pcwszFile here.
   // BUGBUG: Validate dwMode here.

   // BUGBUG: Implement dwMode flag support.

   hr = UnicodeToANSI(pcwszFile, &pszFile);

   if (hr == S_OK)
   {
      hr = LoadFromFile(pszFile, TRUE);

      delete pszFile;
      pszFile = NULL;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::Load, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::GetCurFile(LPOLESTR *ppwszFile)
{
   HRESULT hr;
   LPOLESTR pwszTempFile;

   DebugEntry(InternetShortcut::GetCurFile);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_PTR(ppwszFile, LPOLESTR));

   if (m_pszFile)
   {
      hr = ANSIToUnicode(m_pszFile, &pwszTempFile);

      if (hr == S_OK) {
         TRACE_OUT(("InternetShortcut::GetCurFile(): Current file name is %s.",
                    m_pszFile));
      }
   }
   else
   {
      hr = ANSIToUnicode(g_cszURLDefaultFileNamePrompt, &pwszTempFile);

      if (hr == S_OK)
      {
         hr = S_FALSE;

         TRACE_OUT(("InternetShortcut::GetCurFile(): No current file name.  Returning default file name prompt %s.",
                    g_cszURLDefaultFileNamePrompt));
      }
   }

   if (SUCCEEDED(hr))
   {
      // We should really call OleGetMalloc() to get the process IMalloc here.
      // Use SHAlloc() here instead to avoid loading ole32.dll.
      // SHAlloc() / SHFree() turn in to IMalloc::Alloc() and IMalloc::Free()
      // once ole32.dll is loaded.

      // N.b., lstrlenW() returns the length of the given string in characters,
      // not bytes.

      // (+ 1) for null terminator.
      *ppwszFile = (LPOLESTR)SHAlloc((lstrlenW(pwszTempFile) + 1) *
                                     sizeof(*pwszTempFile));

      if (*ppwszFile)
         lstrcpyW(*ppwszFile, pwszTempFile);
      else
         hr = E_OUTOFMEMORY;

      delete pwszTempFile;
      pwszTempFile = NULL;
   }

   // BUGBUG: Need OLESTR validation function to validate *ppwszFile here.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::GetCurFile, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::Load(PIStream pistr)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::Load);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_INTERFACE_PTR(pistr, IStream));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::Load, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::Save(PIStream pistr,
                                                 BOOL bClearDirty)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::Save);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_INTERFACE_PTR(pistr, IStream));

   // BUGBUG: Yes, this is an awful hack, but that's what we get when
   // no one implements a needed interface and we need to get a product
   // shipped.  (Actually, the hack isn't that bad, as it's what happens in
   // TransferFileContents, except we're writing to a stream and not memory).
   
   const static TCHAR s_cszNewLine[] = TEXT("\r\n");
   const static TCHAR s_cszPrefix[] = TEXT("[InternetShortcut]\r\nURL=");
   LPTSTR pszBuf;
   DWORD cb;

   pszBuf = (LPTSTR)LocalAlloc(LPTR, lstrlen(m_pszURL) + lstrlen(s_cszPrefix) + lstrlen(s_cszNewLine) + 1);
   
   wsprintf(pszBuf, TEXT("%s%s%s"), s_cszPrefix, m_pszURL ? m_pszURL : TEXT("") , s_cszNewLine);
      
   hr = pistr->Write(pszBuf, lstrlen(pszBuf), &cb);

   LocalFree(pszBuf);
   
   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::Save, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetSizeMax(PULARGE_INTEGER pcbSize)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetSizeMax);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_PTR(pcbSize, ULARGE_INTEGER));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::GetSizeMax, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


DWORD STDMETHODCALLTYPE InternetShortcut::GetFileContentsSize(void)
{
   DWORD dwcbLen;

   DebugEntry(InternetShortcut::GetFileContentsSize);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   // Section length.

   // (- 1) for each null terminator.

   HRESULT hr = CreateURLFileContents(m_pszURL, NULL);

   // BUGBUG: (DavidDi 3/29/95) We need to save more than just the URL string
   // here, i.e., icon file and index, working directory, and show command.

   dwcbLen = SUCCEEDED(hr) ? hr : 0;

   dwcbLen++;       // + 1 for final null terminator

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitDWORD(InternetShortcut::GetFileContentsSize, dwcbLen);

   return(dwcbLen);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::TransferUniformResourceLocator(
                                                            PFORMATETC pfmtetc,
                                                            PSTGMEDIUM pstgmed)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::TransferUniformResourceLocator);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
   ASSERT(IS_VALID_WRITE_PTR(pstgmed, STGMEDIUM));

   ASSERT(pfmtetc->dwAspect == DVASPECT_CONTENT);
   ASSERT(pfmtetc->lindex == -1);

   ZeroMemory(pstgmed, sizeof(*pstgmed));

   if (IS_FLAG_SET(pfmtetc->tymed, TYMED_HGLOBAL))
   {
      if (m_pszURL)
      {
         HGLOBAL hgURL;

         hr = E_OUTOFMEMORY;

         // (+ 1) for null terminator.
         hgURL = GlobalAlloc(0, lstrlen(m_pszURL) + 1);

         if (hgURL)
         {
            PSTR pszURL;

            pszURL = (PSTR)GlobalLock(hgURL);

            if (EVAL(pszURL))
            {
               lstrcpy(pszURL, m_pszURL);

               pstgmed->tymed = TYMED_HGLOBAL;
               pstgmed->hGlobal = hgURL;
               ASSERT(! pstgmed->pUnkForRelease);

               hr = S_OK;

               GlobalUnlock(hgURL);
               pszURL = NULL;
            }

            if (hr != S_OK)
            {
               GlobalFree(hgURL);
               hgURL = NULL;
            }
         }
      }
      else
         hr = DV_E_FORMATETC;
   }
   else
      hr = DV_E_TYMED;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM)) ||
          (FAILED(hr) &&
           (EVAL(pstgmed->tymed == TYMED_NULL) &&
            EVAL(! pstgmed->hGlobal) &&
            EVAL(! pstgmed->pUnkForRelease))));

   DebugExitHRESULT(InternetShortcut::TransferUniformResourceLocator, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::TransferText(PFORMATETC pfmtetc,
                                                         PSTGMEDIUM pstgmed)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::TransferText);

   // Assume InternetShortcut::TransferUniformResourceLocator() will perform
   // input and output validation.

   hr = TransferUniformResourceLocator(pfmtetc, pstgmed);

   DebugExitHRESULT(InternetShortcut::TransferText, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::TransferFileGroupDescriptor(
                                                            PFORMATETC pfmtetc,
                                                            PSTGMEDIUM pstgmed)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::TransferFileGroupDescriptor);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
   ASSERT(IS_VALID_WRITE_PTR(pstgmed, STGMEDIUM));

   ASSERT(pfmtetc->dwAspect == DVASPECT_CONTENT);
   ASSERT(pfmtetc->lindex == -1);

   pstgmed->tymed = TYMED_NULL;
   pstgmed->hGlobal = NULL;
   pstgmed->pUnkForRelease = NULL;

   if (IS_FLAG_SET(pfmtetc->tymed, TYMED_HGLOBAL))
   {
      HGLOBAL hgFileGroupDesc;

      hr = E_OUTOFMEMORY;

      hgFileGroupDesc = GlobalAlloc(GMEM_ZEROINIT,
                                    sizeof(FILEGROUPDESCRIPTOR));

      if (hgFileGroupDesc)
      {
         PFILEGROUPDESCRIPTOR pfgd;

         pfgd = (PFILEGROUPDESCRIPTOR)GlobalLock(hgFileGroupDesc);

         if (EVAL(pfgd))
         {
            PFILEDESCRIPTOR pfd = &(pfgd->fgd[0]);

            // Do we already have a file name to use?

            if (m_pszFile)
            {
               lstrcpyn(pfd->cFileName, ExtractFileName(m_pszFile),
                        SIZECHARS(pfd->cFileName));

               hr = S_OK;
            }
            else
            {
               if (EVAL(MLLoadStringA(
                                   IDS_NEW_INTERNET_SHORTCUT, pfd->cFileName,
                                   sizeof(pfd->cFileName))))
                  hr = S_OK;
            }

            if (hr == S_OK)
            {
               pfd->dwFlags = (FD_FILESIZE |
                               FD_LINKUI);
               pfd->nFileSizeHigh = 0;
               pfd->nFileSizeLow = GetFileContentsSize();

               pfgd->cItems = 1;

               pstgmed->tymed = TYMED_HGLOBAL;
               pstgmed->hGlobal = hgFileGroupDesc;
               ASSERT(! pstgmed->pUnkForRelease);
            }

            GlobalUnlock(hgFileGroupDesc);
            pfgd = NULL;
         }

         if (hr != S_OK)
         {
            GlobalFree(hgFileGroupDesc);
            hgFileGroupDesc = NULL;
         }
      }
   }
   else
      hr = DV_E_TYMED;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM)) ||
          (FAILED(hr) &&
           (EVAL(pstgmed->tymed == TYMED_NULL) &&
            EVAL(! pstgmed->hGlobal) &&
            EVAL(! pstgmed->pUnkForRelease))));

   DebugExitHRESULT(InternetShortcut::TransferFileGroupDescriptor, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::TransferFileContents(
                                                            PFORMATETC pfmtetc,
                                                            PSTGMEDIUM pstgmed)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::TransferFileContents);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
   ASSERT(IS_VALID_WRITE_PTR(pstgmed, STGMEDIUM));

   ASSERT(pfmtetc->dwAspect == DVASPECT_CONTENT);
   ASSERT(! pfmtetc->lindex);

   pstgmed->tymed = TYMED_NULL;
   pstgmed->hGlobal = NULL;
   pstgmed->pUnkForRelease = NULL;

   if (IS_FLAG_SET(pfmtetc->tymed, TYMED_HGLOBAL))
   {
      HGLOBAL hgFileContents;
      hr = CreateURLFileContents(m_pszURL, (LPSTR *)&hgFileContents);

      if (SUCCEEDED(hr))
      {
         // Note some apps don't pay attention to the nFileSizeLow
         // field; fortunately, CreateURLFileContents adds a final
         // null terminator to prevent trailing garbage.

         pstgmed->tymed = TYMED_HGLOBAL;
         pstgmed->hGlobal = hgFileContents;
         ASSERT(! pstgmed->pUnkForRelease);

         hr = S_OK;
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
      hr = DV_E_TYMED;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM)) ||
          (FAILED(hr) &&
           (EVAL(pstgmed->tymed == TYMED_NULL) &&
            EVAL(! pstgmed->hGlobal) &&
            EVAL(! pstgmed->pUnkForRelease))));

   DebugExitHRESULT(InternetShortcut::TransferFileContents, hr);

   return(hr);
}


#ifdef DEBUG

void STDMETHODCALLTYPE InternetShortcut::Dump(void)
{
   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   PLAIN_TRACE_OUT(("%sm_dwFlags = %#08lx",
                    INDENT_STRING,
                    m_dwFlags));
   PLAIN_TRACE_OUT(("%sm_pszFile = \"%s\"",
                    INDENT_STRING,
                    CHECK_STRING(m_pszFile)));
   PLAIN_TRACE_OUT(("%sm_pszURL = \"%s\"",
                    INDENT_STRING,
                    CHECK_STRING(m_pszURL)));
   PLAIN_TRACE_OUT(("%sm_pszIconFile = \"%s\"",
                    INDENT_STRING,
                    CHECK_STRING(m_pszIconFile)));
   PLAIN_TRACE_OUT(("%sm_niIcon = %d",
                    INDENT_STRING,
                    m_niIcon));
   PLAIN_TRACE_OUT(("%sm_wHotkey = %#04x",
                    INDENT_STRING,
                    (UINT)m_wHotkey));
   PLAIN_TRACE_OUT(("%sm_pszWorkingDirectory = \"%s\"",
                    INDENT_STRING,
                    CHECK_STRING(m_pszWorkingDirectory)));
   PLAIN_TRACE_OUT(("%sm_nShowCmd = %d",
                    INDENT_STRING,
                    m_nShowCmd));

   return;
}

#endif
