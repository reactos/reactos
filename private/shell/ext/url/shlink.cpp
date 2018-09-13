/*
 * shlink.cpp - IShellLink implementation for InternetShortcut class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop


/* Types
 ********/

typedef enum isl_getpath_flags
{
   // flag combinations

   ALL_ISL_GETPATH_FLAGS   = (SLGP_SHORTPATH |
                              SLGP_UNCPRIORITY)
}
ISL_GETPATH_FLAGS;

typedef enum isl_resolve_flags
{
   // flag combinations

   ALL_ISL_RESOLVE_FLAGS   = (SLR_NO_UI |
                              SLR_ANY_MATCH |
                              SLR_UPDATE)
}
ISL_RESOLVE_FLAGS;


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE InternetShortcut::SetPath(PCSTR pcszPath)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SetPath);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

   // Treat path as literal URL.

   hr = SetURL(pcszPath, 0);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetPath, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::GetPath(PSTR pszFile,
                                                    int ncFileBufLen,
                                                    PWIN32_FIND_DATA pwfd,
                                                    DWORD dwFlags)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetPath);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszFile, STR, ncFileBufLen));
   ASSERT(NULL == pwfd || IS_VALID_STRUCT_PTR(pwfd, CWIN32_FIND_DATA));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_ISL_GETPATH_FLAGS));

   // Ignore dwFlags.

   if (pwfd)
      ZeroMemory(pwfd, sizeof(*pwfd));

   if (m_pszURL)
   {
      lstrcpyn(pszFile, m_pszURL, ncFileBufLen);

      hr = S_OK;
   }
   else
   {
      // No URL.

      if (ncFileBufLen > 0)
         *pszFile = '\0';

      hr = S_FALSE;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           (ncFileBufLen < 1 ||
            IS_VALID_STRING_PTR(pszFile, STR))) ||
          (hr == S_FALSE &&
           (ncFileBufLen < 1 ||
            ! *pszFile)));

   DebugExitHRESULT(InternetShortcut::GetPath, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetRelativePath(
                                                      PCSTR pcszRelativePath,
                                                      DWORD dwReserved)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SetRelativePath);

   // dwReserved may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszRelativePath, CSTR));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetRelativePath, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetIDList(LPCITEMIDLIST pcidl)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SetIDList);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pcidl, CITEMIDLIST));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetIDList, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::GetIDList(LPITEMIDLIST *ppidl)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetIDList);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_PTR(ppidl, PITEMIDLIST));

   *ppidl = NULL;

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::GetIDList, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetDescription(
                                                         PCSTR pcszDescription)
{
   HRESULT hr;
   BOOL bDifferent;
   PSTR pszFileCopy;

   DebugEntry(InternetShortcut::SetDescription);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszDescription, CSTR));

   // Set m_pszFile to description.

   bDifferent = (! m_pszFile ||
                 lstrcmp(pcszDescription, m_pszFile) != 0);

   if (StringCopy(pcszDescription, &pszFileCopy))
   {
      if (m_pszFile)
      {
         delete m_pszFile;
         m_pszFile = NULL;
      }

      m_pszFile = pszFileCopy;

      if (bDifferent)
         Dirty(TRUE);

      hr = S_OK;
   }
   else
      hr = E_OUTOFMEMORY;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetDescription, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetDescription(
                                                      PSTR pszDescription,
                                                      int ncDesciptionBufLen)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetDescription);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszDescription, STR, ncDesciptionBufLen));

   // Get description from m_pszFile.

   if (m_pszFile)
      lstrcpyn(pszDescription, m_pszFile, ncDesciptionBufLen);
   else
   {
      if (ncDesciptionBufLen > 0)
         pszDescription = '\0';
   }

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(hr == S_OK &&
          (ncDesciptionBufLen <= 0 ||
           (IS_VALID_STRING_PTR(pszDescription, STR) &&
            EVAL(lstrlen(pszDescription) < ncDesciptionBufLen))));

   DebugExitHRESULT(InternetShortcut::GetDescription, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::SetArguments(PCSTR pcszArgs)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SetArguments);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRING_PTR(pcszArgs, CSTR));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetArguments, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::GetArguments(PSTR pszArgs,
                                                         int ncArgsBufLen)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetArguments);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszArgs, STR, ncArgsBufLen));

   if (ncArgsBufLen > 0)
      *pszArgs = '\0';

   hr = E_NOTIMPL;

   WARNING_OUT(("InternetShortcut::GetArguments(): Arguments not maintained for CLSID_InternetShortcut."));

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::GetArguments, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetWorkingDirectory(
                                                   PCSTR pcszWorkingDirectory)
{
   HRESULT hr = S_OK;
   char rgchNewPath[MAX_PATH_LEN];
   BOOL bChanged = FALSE;
   PSTR pszNewWorkingDirectory = NULL;

   DebugEntry(InternetShortcut::SetWorkingDirectory);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(! pcszWorkingDirectory ||
          IS_VALID_STRING_PTR(pcszWorkingDirectory, CSTR));

   if (! AnyMeat(pcszWorkingDirectory))
      pcszWorkingDirectory = NULL;

   if (pcszWorkingDirectory)
   {
      PSTR pszFileName;

      if (GetFullPathName(pcszWorkingDirectory, sizeof(rgchNewPath),
                          rgchNewPath, &pszFileName) > 0)
         pcszWorkingDirectory = rgchNewPath;
      else
         hr = E_PATH_NOT_FOUND;
   }

   if (hr == S_OK)
   {
      bChanged = ! ((! pcszWorkingDirectory && ! m_pszWorkingDirectory) ||
                    (pcszWorkingDirectory && m_pszWorkingDirectory &&
                     ! lstrcmp(pcszWorkingDirectory, m_pszWorkingDirectory)));

      if (bChanged && pcszWorkingDirectory)
      {
         // (+ 1) for null terminator.

         pszNewWorkingDirectory = new(char[lstrlen(pcszWorkingDirectory) + 1]);

         if (pszNewWorkingDirectory)
            lstrcpy(pszNewWorkingDirectory, pcszWorkingDirectory);
         else
            hr = E_OUTOFMEMORY;
      }
   }

   if (hr == S_OK)
   {
      if (bChanged)
      {
         if (m_pszWorkingDirectory)
            delete m_pszWorkingDirectory;

         m_pszWorkingDirectory = pszNewWorkingDirectory;

         Dirty(TRUE);

         TRACE_OUT(("InternetShortcut::SetWorkingDirectory(): Set working directory to %s.",
                    CHECK_STRING(m_pszWorkingDirectory)));
      }
      else
         TRACE_OUT(("InternetShortcut::SetWorkingDirectory(): Working directory already %s.",
                    CHECK_STRING(m_pszWorkingDirectory)));
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(hr == S_OK ||
          hr == E_OUTOFMEMORY ||
          hr == E_PATH_NOT_FOUND);

   DebugExitHRESULT(InternetShortcut::SetWorkingDirectory, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetWorkingDirectory(
                                                PSTR pszWorkingDirectory,
                                                int ncbWorkingDirectoryBufLen)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetWorkingDirectory);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszWorkingDirectory, STR, ncbWorkingDirectoryBufLen));

   if (m_pszWorkingDirectory)
   {
      lstrcpyn(pszWorkingDirectory, m_pszWorkingDirectory,
                 ncbWorkingDirectoryBufLen);

      hr = S_OK;
   }
   else
   {
      if (ncbWorkingDirectoryBufLen > 0)
         *pszWorkingDirectory = '\0';

      hr = S_FALSE;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IsValidPathResult(hr, pszWorkingDirectory, ncbWorkingDirectoryBufLen));
   ASSERT(hr == S_OK ||
          hr == S_FALSE);

   DebugExitHRESULT(InternetShortcut::GetWorkingDirectory, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetHotkey(WORD wHotkey)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SetHotkey);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(! wHotkey ||
          EVAL(IsValidHotkey(wHotkey)));

   if (wHotkey != m_wHotkey)
      Dirty(TRUE);

   m_wHotkey = wHotkey;

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetHotkey, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetHotkey(PWORD pwHotkey)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetHotkey);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_PTR(pwHotkey, WORD));

   *pwHotkey = m_wHotkey;

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(! *pwHotkey ||
          EVAL(IsValidHotkey(*pwHotkey)));

   DebugExitHRESULT(InternetShortcut::GetHotkey, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetShowCmd(int nShowCmd)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SetShowCmd);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IsValidShowCmd(nShowCmd));

   if (nShowCmd != m_nShowCmd)
   {
      m_nShowCmd = nShowCmd;

      Dirty(TRUE);

      TRACE_OUT(("InternetShortcut::SetShowCmd(): Set show command to %d.",
                 m_nShowCmd));
   }
   else
      TRACE_OUT(("InternetShortcut::SetShowCmd(): Show command already %d.",
                 m_nShowCmd));

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(hr == S_OK);

   DebugExitHRESULT(InternetShortcut::SetShowCmd, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetShowCmd(PINT pnShowCmd)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetShowCmd);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_PTR(pnShowCmd, INT));

   *pnShowCmd = m_nShowCmd;

   hr = S_OK;

   ASSERT(IsValidShowCmd(m_nShowCmd));

   DebugExitHRESULT(InternetShortcut::GetShowCmd, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::SetIconLocation(PCSTR pcszIconFile,
                                                            int niIcon)
{
   HRESULT hr = S_OK;
   BOOL bNewMeat;
   char rgchNewPath[MAX_PATH_LEN];

   DebugEntry(InternetShortcut::SetIconLocation);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IsValidIconIndex(pcszIconFile ? S_OK : S_FALSE, pcszIconFile, MAX_PATH_LEN, niIcon));

   bNewMeat = AnyMeat(pcszIconFile);

   if (bNewMeat)
      hr = FullyQualifyPath(pcszIconFile, rgchNewPath, sizeof(rgchNewPath));

   if (hr == S_OK)
   {
      char rgchOldPath[MAX_PATH_LEN];
      int niOldIcon;
      UINT uFlags;

      hr = GetIconLocation(0, rgchOldPath, sizeof(rgchOldPath), &niOldIcon,
                           &uFlags);

      if (SUCCEEDED(hr))
      {
         BOOL bOldMeat;
         BOOL bChanged = FALSE;
         PSTR pszNewIconFile = NULL;
         int niNewIcon = 0;

         bOldMeat = AnyMeat(rgchOldPath);

         ASSERT(! *rgchOldPath ||
                bOldMeat);

         bChanged = ((! bOldMeat && bNewMeat) ||
                     (bOldMeat && ! bNewMeat) ||
                     (bOldMeat && bNewMeat &&
                      (lstrcmp(rgchOldPath, rgchNewPath) != 0 ||
                       niIcon != niOldIcon)));

         if (bChanged && bNewMeat)
         {
            // (+ 1) for null terminator.

            pszNewIconFile = new(char[lstrlen(rgchNewPath) + 1]);

            if (pszNewIconFile)
            {
               lstrcpy(pszNewIconFile, rgchNewPath);
               niNewIcon = niIcon;

               hr = S_OK;
            }
            else
               hr = E_OUTOFMEMORY;
         }
         else
            hr = S_OK;

         if (hr == S_OK)
         {
            if (bChanged)
            {
               if (m_pszIconFile)
                  delete m_pszIconFile;

               m_pszIconFile = pszNewIconFile;
               m_niIcon = niNewIcon;

               Dirty(TRUE);

               TRACE_OUT(("InternetShortcut::SetIconLocation(): Set icon location to %d in %s.",
                          m_niIcon,
                          CHECK_STRING(m_pszIconFile)));
            }
            else
               TRACE_OUT(("InternetShortcut::SetIconLocation(): Icon location already %d in %s.",
                          m_niIcon,
                          CHECK_STRING(m_pszIconFile)));
         }
      }
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(hr == S_OK ||
          hr == E_OUTOFMEMORY ||
          hr == E_FILE_NOT_FOUND);

   DebugExitHRESULT(InternetShortcut::SetIconLocation, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetIconLocation(
                                                         PSTR pszIconFile,
                                                         int ncbIconFileBufLen,
                                                         PINT pniIcon)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetIconLocation);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszIconFile, STR, ncbIconFileBufLen));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, int));

   if (m_pszIconFile)
   {
      lstrcpyn(pszIconFile, m_pszIconFile, ncbIconFileBufLen);
      *pniIcon = m_niIcon;

      hr = S_OK;
   }
   else
   {
      if (ncbIconFileBufLen > 0)
         *pszIconFile = '\0';
      *pniIcon = 0;

      hr = S_FALSE;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IsValidIconIndex(hr, pszIconFile, ncbIconFileBufLen, *pniIcon));

   DebugExitHRESULT(InternetShortcut::GetIconLocation, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::Resolve(HWND hwnd, DWORD dwFlags)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::Resolve);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_HANDLE(hwnd, WND));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_ISL_RESOLVE_FLAGS));

   hr = S_OK;

   WARNING_OUT(("InternetShortcut::Resolve(): This method is a NOP for CLSID_InternetShortcut."));

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::Resolve, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */

