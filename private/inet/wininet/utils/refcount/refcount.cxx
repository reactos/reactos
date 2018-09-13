//=--------------------------------------------------------------------------=
//  (C) Copyright 1997-1998 Microsoft Corporation. All Rights Reserved.
//	TriEdit SDK team
//	Author: Yury Polyakovsky
//	contact: a-yurip@microsof.com
//=--------------------------------------------------------------------------=
// Refcount support for DLLs
// Call:	refcount < Install | Uninstall | CopyToClient | SetClient | ClearClient | Copy >, <registry key> [, <file name>[, <client subdirectory> I <destination for Copy>] | <component full path>]
// Call refcount.exe from an application's setup: 
// Install : Increment ref-count for the component if it was not installed before by the app
// Uninstall : Decrement ref-count for the component if it was installed before by the app, deletes it if ref-count goes to 0
// CopyToClient : Copies the component to the application's directory (from the registry)
// SetClient : Set the flag in the registry that the component was installed by the application
// CreateDir : Checks on the directory  "C:\Program Files\Common Files" and create one if it's not there
// ClearClient :  Clear the flag in the registry that the component was installed by the application
// Copy: Copy to specified destination.
#include <ctype.h>
#include <windef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <crtdbg.h>
#include <shlwapi.h>
#include "RefCount.h"

#define STRINGOP(func, param1, param2) 	(func(param1, param2, sizeof(param1)/sizeof(param1[0])))
#define STRINGOPL(func, param1, param2) 	(func(param1, param2, strlen(param1)))
#define SKIPSPACES(psz)	{for (++psz; *psz == ' ' && *psz != '\0'; ++psz); if (!*psz) psz=NULL;}
BOOL PASCAL ReplaceFileOnReboot (LPCTSTR pszExisting, LPCTSTR pszNew);

int WINAPI WinMain(  HINSTANCE hInstance,  // handle to current instance
  HINSTANCE hPrevInstance,  // handle to previous instance
  LPSTR lpCmdLine,      // pointer to command line
  int nCmdShow)          // show state of window);int main(int argc, char** argv)
{
	_ASSERT(*lpCmdLine);
	if (!*lpCmdLine)
	{
		// No command line
		return 1;
	}

	CRefCount	RefCount;
	char szRegKey[MAX_PATH] = "";
	char szRegInstalling[MAX_PATH] = "";
	char szRegInstalled[MAX_PATH] = "";
	char sz_AppPath[MAX_PATH] = "";
	char sz_Application[MAX_PATH] = "";
	char sz_Dir[MAX_PATH] = "";
	char *pszAppPath = sz_AppPath;
	char *pszApplication = sz_Application;
	BOOL bMultiInstall = FALSE;
	BOOL *pbInstallMode = &bMultiInstall;
	DWORD dwValueSize = sizeof(sz_AppPath);

	//_ASSERT(FALSE);

	if (!STRINGOPL(_strnicmp, "CreateDir", lpCmdLine/*argv[1]*/))
	{
		dwValueSize = sizeof(sz_Application);
		RefCount.ValueGet("SOFTWARE\\Microsoft\\Windows\\CurrentVersion", "CommonFilesDir", (LPBYTE *)&pszApplication, &dwValueSize);
		_ASSERT(pszApplication);
		if (pszApplication)
		{
			if (!CreateDirectory(pszApplication, NULL))
				DWORD dwError = GetLastError();
		}
		return 0;
	}

	LPSTR pszRegName = strchr(lpCmdLine, ',');
	_ASSERT(pszRegName);
	if (!pszRegName)
		// command line not complete
		return 1;

	SKIPSPACES(pszRegName);
	_ASSERT(pszRegName);
	if (!pszRegName)
		// command line not complete
		return 1;

	LPSTR pszPathName = strchr(pszRegName, ',');
	if (pszPathName)
	{
		SKIPSPACES(pszPathName);
	}
	STRINGOP(strncpy, szRegKey, pszRegName/*argv[2]*/);
	if (LPSTR pszEndRegName = strchr(szRegKey, ','))
		*pszEndRegName = '\0';
	STRINGOP(strncpy, szRegInstalling, szRegKey/*argv[2]*/);
	STRINGOP(strncat , szRegInstalling, "\\InstallingClient");
	RefCount.ValueGet(szRegInstalling, "Path", (LPBYTE *)&pszAppPath, &dwValueSize);
	dwValueSize = sizeof(sz_Application);
	RefCount.ValueGet(szRegInstalling, "Application", (LPBYTE *)&pszApplication, &dwValueSize);
	dwValueSize = sizeof(BOOL);
	RefCount.ValueGet(szRegInstalling, "MultiInstall", (LPBYTE *)&pbInstallMode, &dwValueSize);
	if (pszAppPath && ((pbInstallMode && bMultiInstall) || !STRINGOPL(_strnicmp, "CopyToClient", lpCmdLine/*argv[1]*/)))
	{
		STRINGOP(strncat, pszAppPath, "\\");
		STRINGOP(strncpy, sz_Dir, pszAppPath);
		STRINGOP(strncat, sz_AppPath, sz_Application);
	}
	else 
		STRINGOP(strncpy, sz_AppPath, sz_Application);

	// FInd the path in InstalledClients
	STRINGOP(strncpy, szRegInstalled, szRegKey/*argv[2]*/);
	STRINGOP(strncat, szRegInstalled, "\\InstalledClients");

	char szTmp[MAX_PATH];
	strcpy(szTmp, pszPathName);
	char* x = strchr(szTmp, ',');
	char szGUID[MAX_PATH];
	char* pszGUID = NULL;

	if( x )
	{
		// IE passed component guid so we need to check...
		strcpy(szGUID, x);
		*x = '\0';
		strcpy(pszPathName, szTmp);

		// now szGuid = " , {aab-cc-dd-ee}", need to stript white space and ','
		pszGUID = szGUID;
		for( int i = 0; i < MAX_PATH; i++)
		{
			if( *pszGUID != ' ' && *pszGUID != ',')
				break;
			pszGUID++;
		}
		if( i == MAX_PATH || *pszGUID == '\0' )
        {
			pszGUID = NULL;
        }
	}


	if (!STRINGOPL(_strnicmp, "CopyToClient", lpCmdLine/*argv[1]*/))
	{
		_ASSERT(pszPathName);
		if (!pszPathName)
			return 2;
		else
		{
			LPSTR pszDestSubDir = strchr(pszPathName, ',');
			if (pszDestSubDir)
			{
				SKIPSPACES(pszDestSubDir);
				STRINGOP(strncat, sz_Dir, pszDestSubDir);
				STRINGOP(strncat, sz_Dir, "\\");
			}
			if (LPSTR pszPathNameEnd = strchr(pszPathName, ','))
				*pszPathNameEnd = '\0';
			// Copy files we need for Uninstall to the client location
			STRINGOP(strncat, sz_Dir, pszPathName);
			if (LPSTR pszDestDirEnd = strchr(sz_Dir, ','))
				*pszDestDirEnd = '\0';
			if (!CopyFile(pszPathName/*argv[3]*/, sz_Dir, FALSE))
			{
				DWORD dwError = GetLastError();
				_ASSERTE(!dwError);
				return 2;
			}
			return 0;
		}
	}
	else if (!STRINGOPL(_strnicmp, "Copy", lpCmdLine/*argv[1]*/))
	{
		_ASSERT(pszPathName);
		if (!pszPathName)
			return 2;
		else
		{
			LPSTR pszDestSubDir = strchr(pszPathName, ',');
			if (pszDestSubDir)
			{
				SKIPSPACES(pszDestSubDir);
				STRINGOP(strncpy, sz_Dir, pszDestSubDir);
			}
			if (LPSTR pszPathNameEnd = strchr(pszPathName, ','))
				*pszPathNameEnd = '\0';
			// Copy files we need for Uninstall to the client location
			if (LPSTR pszDestDirEnd = strchr(sz_Dir, ','))
				*pszDestDirEnd = '\0';
			if (!CopyFile(pszPathName/*argv[3]*/, sz_Dir, FALSE))
			{
				DWORD dwError = GetLastError();
				_ASSERTE(!dwError);
				return 2;
			}
			return 0;
		}
	}
		else if (!RefCount.ValueExist(szRegInstalled, sz_AppPath) && !STRINGOPL(_strnicmp, "Install", lpCmdLine/*argv[1]*/))
		{
			// Increment ref-count
			if( pszGUID )
			{
				// some check needed...
				// is the component installed?
				HKEY hkInstalled = NULL;
				char szKeyName[MAX_PATH];
				strcpy(szKeyName, "SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\");
				strcat(szKeyName, pszGUID);

				if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_ALL_ACCESS, &hkInstalled) 
				)
				{
					DWORD dwType = 0;
					BYTE bValueData[16];
					DWORD cValueData = sizeof(bValueData);;
					if (ERROR_SUCCESS == RegQueryValueEx( hkInstalled, "IsInstalled", NULL, &dwType, bValueData, &cValueData))
					{
						if( *(LPDWORD)(bValueData) )
						{
							// this app has been installed, we need to do nothing... (do not perform refcount, just quit)
							RegCloseKey(hkInstalled);
							return 0;
						}
						
					}
					RegCloseKey(hkInstalled);
				}
			}

			// we are here because 
			// 1. no guid passed from cmd line
			// 2. or component not installed (the RegQueryValue failed..)
			// so we continue to do refcount...

			RefCount.SetInstalFlag(TRUE);
		}
		else if (!RefCount.ValueExist(szRegInstalled, sz_AppPath) && !STRINGOPL(_strnicmp, "SetClient", lpCmdLine/*argv[1]*/))
		{
			// Set the app's installed flag
			RefCount.ValueSet(szRegInstalled, sz_AppPath);
			RegDeleteKey (HKEY_LOCAL_MACHINE, szRegInstalling); // we don't need it anymore
			return 0;
		}
		else if (!STRINGOPL(_strnicmp, "Uninstall", lpCmdLine/*argv[1]*/))
		{
			// Decrement ref-count
			RefCount.SetInstalFlag(FALSE);
		}
		else if (!STRINGOPL(_strnicmp, "ClearClient", lpCmdLine/*argv[1]*/))
		{
			// Remove the app's installed flag
			RefCount.ValueClear(szRegInstalled, sz_AppPath);
			RegDeleteKey(HKEY_LOCAL_MACHINE, szRegInstalling); // we don't need it anymore
			return 0;
		}
		else if (!STRINGOPL(_strnicmp, "SetClient", lpCmdLine/*argv[1]*/))
		{
			// Subsequent Installation
			RegDeleteKey(HKEY_LOCAL_MACHINE, szRegInstalling); // we don't need it anymore
			return 0;
		}
		else
			// Subsequent Installation
			return 0;

	HKEY hkRef;    // address of handle to open key
	DWORD dwDisposition;   // address of disposition value buffer
	LONG lRet;

	lRet = RegCreateKeyEx  (HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs", 
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkRef, &dwDisposition);
	_ASSERT(REG_OPENED_EXISTING_KEY == dwDisposition);
	_ASSERT(ERROR_SUCCESS == lRet);
	if (ERROR_SUCCESS != lRet)
	{
		return 3;
	}

	_ASSERT(pszPathName);
	if (pszPathName)
	{
		RefCount.Change(pszPathName/*argv[3]*/, &hkRef);
		if (RefCount.GetCount() <= 0)
		{
			_ASSERT(RefCount.GetCount() == 0);
			//RegDeleteKey(HKEY_LOCAL_MACHINE, szRegInstalling); // we don't need it anymore
			//RegDeleteKey(HKEY_LOCAL_MACHINE, szRegInstalled); // we don't need it anymore
			//RegDeleteKey(HKEY_LOCAL_MACHINE, szRegKey); // we don't need it anymore
		}
	}

	RegCloseKey(hkRef);

	return 0;
}

void CRefCount::Change(char *szName, PHKEY phkRef)
{
	DWORD dwIndex = 0;
	DWORD cValueName = 0;
	DWORD dwType = 0;
	BYTE bValueData[16];
	DWORD cValueData = sizeof(bValueData);;

	// Check if the conponent exits
	if (ERROR_SUCCESS == RegQueryValueEx( *phkRef, szName, NULL, &dwType, bValueData, &cValueData))
	{
		// Found
		_ASSERT(dwType == REG_DWORD);
		m_dwRefCount = *(LPDWORD)(bValueData);
		(m_fInstall) ? ++m_dwRefCount : --m_dwRefCount;
	}
	else if (!m_fInstall)
	{
		// Trying to Uninstall the component that was not ref-counted before
		// Just delete it
//		_ASSERT(m_fInstall);
		if (!DeleteFile(szName))
		{
			if (!ReplaceFileOnReboot(szName, NULL))
			{
				DWORD dwError = GetLastError();
				_ASSERTE(!dwError);
			}
		}
		return;
	}
	// If not found, create new else just overwrite it.
	*(LPDWORD)(bValueData) = m_dwRefCount;
	if (m_dwRefCount == 0)
	{
        char szDllFullPath[MAX_PATH+1];
        DWORD dwLen = 0;
        dwLen = strlen(szName);
        char* p = NULL;
        if( dwLen <= MAX_PATH )
        {
            strcpy(szDllFullPath, szName);
            for( int i = dwLen; i >= 0; i--)
            {
                if( szDllFullPath[i] == '\\' ) 
                {
                    p = &(szDllFullPath[i+1]);    
                    break;
                }
            }
        }

		if( p && 
            !_stricmp(p, "msdapml.dll") && 
            !_stricmp(p, "msonsext.dll") &&
            !_stricmp(p, "ragent.tlb") &&
            !_stricmp(p, "ragent.dll") &&
            !_stricmp(p, "fp4autl.dll") &&
            !_stricmp(p, "fp4anwi.dll") 
        ) {
		    // Last rererence deleted
		    HINSTANCE hInst = LoadLibrary(szName);
		    FARPROC pDllUnregisterServer = NULL;
		    if (hInst && (pDllUnregisterServer = GetProcAddress(hInst, "DllUnregisterServer")))
		    {
			    pDllUnregisterServer();
		    }
		    else
		    {
			    DWORD dwError = GetLastError();
		    }

		    FreeLibrary(hInst);
        }

		RegDeleteValue(*phkRef, szName);

		if (!DeleteFile(szName))
		{
			if (!ReplaceFileOnReboot(szName, NULL))
			{
				DWORD dwError = GetLastError();
				_ASSERTE(!dwError);
			}
		}
        

		if( p && !_stricmp(p, "msonsext.dll") )
		{
	        LONG lRet =0;

			lRet = RegDeleteKey (HKEY_CLASSES_ROOT,"CLSID\\{BDEADF00-C265-11d0-BCED-00A0C90AB50F}");
			lRet = RegDeleteKey (HKEY_CLASSES_ROOT,"CLSID\\{BDEADF04-C265-11d0-BCED-00A0C90AB50F}");
			lRet = RegDeleteKey (HKEY_CLASSES_ROOT,"Publishing Folder");
			lRet = RegDeleteKey (HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{BDEADF00-C265-11d0-BCED-00A0C90AB50F}");

	        HKEY hkRef = NULL;  
			lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0, KEY_ALL_ACCESS, &hkRef);
	
	        if (ERROR_SUCCESS == lRet)
	        {
		        RegDeleteValue(hkRef, "{BDEADF00-C265-11d0-BCED-00A0C90AB50F}"); 
                RegCloseKey(hkRef);
	        }
		}
	}
	else
		RegSetValueEx(*phkRef, szName, 0, REG_DWORD, bValueData, sizeof(DWORD));
}

BOOL CRefCount::ValueExist(char *sz_RegSubkey, char *sz_RegValue)
{
	HKEY hkRef = NULL;    // address of handle to open key
	LONG lRet =0;
	BOOL fret = FALSE;

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz_RegSubkey, 
		0, KEY_ALL_ACCESS, &hkRef);
	
	if (ERROR_SUCCESS != lRet)
	{
		return FALSE;
	}
	else 
	{
		lRet = RegQueryValueEx(hkRef, sz_RegValue, 0, NULL, NULL, NULL);
		if (ERROR_SUCCESS != lRet)
			fret = FALSE;
		else
			fret = TRUE;
	}
	RegCloseKey(hkRef);
	return fret;
}

void CRefCount::ValueSet(char *sz_RegSubkey, char *sz_RegValue)
{
	HKEY hkRef = NULL;    // address of handle to open key
	LONG lRet =0;
	DWORD dwValiue = 1;
	DWORD dwDisposition;   // address of disposition value buffer

	lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, sz_RegSubkey, 
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkRef, &dwDisposition);
	
	_ASSERT(ERROR_SUCCESS == lRet);
	if (ERROR_SUCCESS != lRet)
	{
		return;
	}
	else 
	{
		RegSetValueEx(hkRef, sz_RegValue, 0, REG_DWORD, (BYTE  *)&dwValiue, sizeof(dwValiue));
	}
}

void CRefCount::ValueGet(char *sz_RegSubkey,  char *sz_ValueName, LPBYTE *p_Value, DWORD *pdwValueSize)
{
	HKEY hkRef = NULL;    // address of handle to open key
	LONG lRet =0;
	DWORD dwValiue = 1;

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz_RegSubkey, 
		0, KEY_ALL_ACCESS, &hkRef);
	
	if (ERROR_SUCCESS != lRet)
	{
		*p_Value = NULL;
	}
	else 
	{
		lRet = RegQueryValueEx(hkRef, sz_ValueName, 0, NULL, *p_Value, pdwValueSize);
		if (ERROR_SUCCESS != lRet)
		{
			*p_Value = NULL;
		}
	}
}

void CRefCount::ValueClear(char *sz_RegSubkey, char *sz_RegValue)
{
	HKEY hkRef = NULL;    // address of handle to open key
	LONG lRet =0;
	DWORD dwValiue = 1;

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz_RegSubkey, 
		0, KEY_ALL_ACCESS, &hkRef);
	
	if (ERROR_SUCCESS != lRet)
	{
		return;
	}
	else 
	{
		RegDeleteValue(hkRef, sz_RegValue);
	}
}

BOOL PASCAL ReplaceFileOnReboot (LPCTSTR pszExisting, LPCTSTR pszNew) 
{
//	_ASSERT(FALSE);
   // First, attempt to use the MoveFileEx function.
   BOOL fOk = MoveFileEx(pszExisting, pszNew, MOVEFILE_DELAY_UNTIL_REBOOT);
   if (fOk) return(fOk);

   // If MoveFileEx failed, we are running on Windows 95 and need to add
   // entries to the WININIT.INI file (an ANSI file).
   // Start a new scope for local variables.
   {
   char szRenameLine[1024];   
   TCHAR szExistingShort[_MAX_PATH];

   GetShortPathName(pszExisting, szExistingShort, sizeof(szExistingShort) / sizeof(szExistingShort[0]));
   int cchRenameLine = wsprintfA(szRenameLine, 
#ifdef UNICODE
      "%ls=%ls\r\n", 
#else
      "%hs=%hs\r\n", 
#endif
      (pszNew == NULL) ? __TEXT("NUL") : pszNew, szExistingShort);
      char szRenameSec[] = "[Rename]\r\n";
      int cchRenameSec = sizeof(szRenameSec) - 1;
      HANDLE hfile, hfilemap;
      DWORD dwFileSize, dwRenameLinePos;
      TCHAR szPathnameWinInit[_MAX_PATH];

      // Construct the full pathname of the WININIT.INI file.
      GetWindowsDirectory(szPathnameWinInit, _MAX_PATH);
      lstrcat(szPathnameWinInit, __TEXT("\\WinInit.Ini"));

      // Open/Create the WININIT.INI file.
      hfile = CreateFile(szPathnameWinInit,      
         GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

      if (hfile == INVALID_HANDLE_VALUE) 
         return(fOk);	// It is still FALSE

      // Create a file mapping object that is the current size of 
      // the WININIT.INI file plus the length of the additional string
      // that we're about to insert into it plus the length of the section
      // header (which we might have to add).
      dwFileSize = GetFileSize(hfile, NULL);
      hfilemap = CreateFileMapping(hfile, NULL, PAGE_READWRITE, 0, 
         dwFileSize + cchRenameLine + cchRenameSec, NULL);

      if (hfilemap != NULL) {

         // Map the WININIT.INI file into memory.  Note: The contents 
         // of WININIT.INI are always ANSI; never Unicode.
         LPSTR pszWinInit = (LPSTR) MapViewOfFile(hfilemap, 
            FILE_MAP_WRITE, 0, 0, 0);
		 pszWinInit[dwFileSize] = 0;	// Make sure it is null terminated. Yury

         if (pszWinInit != NULL) {

            // Search for the [Rename] section in the file.
            LPSTR pszRenameSecInFile = strstr(pszWinInit, szRenameSec);

            if (pszRenameSecInFile == NULL) {

               // There is no [Rename] section in the WININIT.INI file.
               // We must add the section too.
               dwFileSize += wsprintfA(&pszWinInit[dwFileSize], "%s",
                                       szRenameSec);
               dwRenameLinePos = dwFileSize;

            } else {

               // We found the [Rename] section, shift all the lines down
               PSTR pszFirstRenameLine = strchr(pszRenameSecInFile, '\n');
               // Shift the contents of the file down to make room for 
               // the newly added line.  The new line is always added 
               // to the top of the list.
               pszFirstRenameLine++;   // 1st char on the next line
               memmove(pszFirstRenameLine + cchRenameLine, pszFirstRenameLine, 
                  pszWinInit + dwFileSize - pszFirstRenameLine);                  
               dwRenameLinePos = pszFirstRenameLine - pszWinInit;
            }

            // Insert the new line
            memcpy(&pszWinInit[dwRenameLinePos], szRenameLine, cchRenameLine);

            UnmapViewOfFile(pszWinInit);

            // Calculate the true, new size of the file.
            dwFileSize += cchRenameLine;

            // Everything was successful.
            fOk = TRUE; 
         }
         CloseHandle(hfilemap);
      }

      // Force the end of the file to be the calculated, new size.
      SetFilePointer(hfile, dwFileSize, NULL, FILE_BEGIN);
      SetEndOfFile(hfile);

      CloseHandle(hfile);
   }

   return(fOk);
}

