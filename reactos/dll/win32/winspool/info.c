/*
 * WINSPOOL functions
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 * Copyright 1999, 2000 Huw D M Davies
 * Copyright 2001 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winreg.h"
#include "shlwapi.h"
#include "winspool.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

/******************************************************************************
 *		GetDefaultPrinterA   (WINSPOOL.@)
 */
BOOL WINAPI GetDefaultPrinterA(LPSTR name, LPDWORD namesize)
{
   char *ptr;

   if (*namesize < 1)
   {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   if (!GetProfileStringA ("windows", "device", "", name, *namesize))
   {
      SetLastError (ERROR_FILE_NOT_FOUND);
      return FALSE;
   }

   if ((ptr = strchr (name, ',')) == NULL)
   {
      SetLastError (ERROR_FILE_NOT_FOUND);
      return FALSE;
   }

   *ptr = '\0';
   *namesize = strlen (name) + 1;
   return TRUE;
}


/******************************************************************************
 *		GetDefaultPrinterW   (WINSPOOL.@)
 */
BOOL WINAPI GetDefaultPrinterW(LPWSTR name, LPDWORD namesize)
{
   char *buf;
   BOOL  ret;

   if (*namesize < 1)
   {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   buf = HeapAlloc (GetProcessHeap (), 0, *namesize);
   ret = GetDefaultPrinterA (buf, namesize);
   if (ret)
   {
       DWORD len = MultiByteToWideChar (CP_ACP, 0, buf, -1, name, *namesize);
       if (!len)
       {
           SetLastError (ERROR_INSUFFICIENT_BUFFER);
           ret = FALSE;
       }
       else *namesize = len;
   }

   HeapFree (GetProcessHeap (), 0, buf);
   return ret;
}

/******************************************************************************
 *		AddPrintProvidorA   (WINSPOOL.@)
 */
BOOL
STDCALL
AddPrintProvidorA(LPSTR Name, DWORD Level, PBYTE Buffer)
{
   if (Name || Level > 2 || Buffer == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if (Level == 1)
   {
      BOOL bRet;
      PROVIDOR_INFO_1W Provider;
      PROVIDOR_INFO_1A  *Prov = (PROVIDOR_INFO_1A*)Buffer;

      if (Prov->pName == NULL || Prov->pDLLName == NULL || Prov->pEnvironment == NULL)
      {
         return FALSE;
      }

      Provider.pDLLName = HeapAlloc(GetProcessHeap(), 0, (strlen(Prov->pDLLName)+1) * sizeof(WCHAR));
      if (Provider.pDLLName)
      {
          MultiByteToWideChar(CP_ACP, 0, Prov->pDLLName, -1, Provider.pDLLName, strlen(Prov->pDLLName)+1);
          Provider.pDLLName[strlen(Prov->pDLLName)] = L'\0';
      }

      Provider.pEnvironment = HeapAlloc(GetProcessHeap(), 0, (strlen(Prov->pEnvironment)+1) * sizeof(WCHAR));
      if (Provider.pEnvironment)
      {
          MultiByteToWideChar(CP_ACP, 0, Prov->pEnvironment, -1, Provider.pEnvironment, strlen(Prov->pEnvironment)+1);
          Provider.pEnvironment[strlen(Prov->pEnvironment)] = L'\0';
      }

      Provider.pName = HeapAlloc(GetProcessHeap(), 0, (strlen(Prov->pName)+1) * sizeof(WCHAR));
      if (Provider.pName)
      {
          MultiByteToWideChar(CP_ACP, 0, Prov->pName, -1, Provider.pName, strlen(Prov->pName)+1);
          Provider.pName[strlen(Prov->pName)] = L'\0';
      }

      bRet = AddPrintProvidorW(NULL, Level, (LPBYTE)&Provider);

      if (Provider.pDLLName)
          HeapFree(GetProcessHeap(), 0, Provider.pDLLName);

      if (Provider.pEnvironment)
          HeapFree(GetProcessHeap(), 0, Provider.pEnvironment);

      if (Provider.pName)
          HeapFree(GetProcessHeap(), 0, Provider.pName);

      return bRet;
   }
   else
   {
      PROVIDOR_INFO_2W Provider;
      PROVIDOR_INFO_2A  *Prov = (PROVIDOR_INFO_2A*)Buffer;

      Provider.pOrder = HeapAlloc(GetProcessHeap(), 0, (strlen(Prov->pOrder)+1) * sizeof(WCHAR));
      if (Provider.pOrder)
      {
          BOOL bRet;
          MultiByteToWideChar(CP_ACP, 0, Prov->pOrder, -1, Provider.pOrder, strlen(Prov->pOrder)+1);
          Provider.pOrder[strlen(Prov->pOrder)] = L'\0';

          bRet = AddPrintProvidorW(NULL, Level, (LPBYTE)&Provider);
          HeapFree(GetProcessHeap(), 0, Provider.pOrder);
          return bRet;
      }
   }

  return FALSE;
}


/******************************************************************************
 *		AddPrintProvidorW   (WINSPOOL.@)
 */
BOOL
STDCALL
AddPrintProvidorW(LPWSTR Name, DWORD Level, PBYTE Buffer)
{
   HKEY hKey;
   LPWSTR pOrder;
   DWORD dwSize, dwType;
   BOOL bRet = FALSE;

   if (Name || Level > 2 || Buffer == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }


   if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Print\\Providers", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
   {
      return FALSE;
   }

   if (RegQueryValueExW(hKey, L"Order", NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
   {
      RegCloseKey(hKey);
      return FALSE;
   }

   pOrder = HeapAlloc(GetProcessHeap(), 0, dwSize);
   if (!pOrder)
   {
      RegCloseKey(hKey);
      return FALSE;
   }

   if (RegQueryValueExW(hKey, L"Order", NULL, &dwType, (LPBYTE)pOrder, &dwSize) != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
   {
      RegCloseKey(hKey);
      return FALSE;
   }

   if (Level == 1)
   {
      LPWSTR pBuffer;
      BOOL bFound = FALSE;
      PROVIDOR_INFO_1W * Prov = (PROVIDOR_INFO_1W*)Buffer;

      if (Prov->pName == NULL || Prov->pDLLName == NULL || Prov->pEnvironment == NULL)
      {
         SetLastError(ERROR_INVALID_PARAMETER);
         RegCloseKey(hKey);
         return FALSE;
      }

      pBuffer = pOrder;

      while(pBuffer[0])
      {
         if (!wcsicmp(pBuffer, Prov->pName))
         {
            bFound = TRUE;
            break;
         }
         pBuffer += wcslen(pBuffer) + 1;
      }

      if (!bFound)
      {
         HKEY hSubKey;
         DWORD dwFullSize = dwSize + (wcslen(Prov->pName)+1) * sizeof(WCHAR);

         if (RegCreateKeyExW(hKey, Prov->pName, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
         {
            RegSetValueExW(hSubKey, L"Name", 0, REG_SZ, (LPBYTE)Prov->pDLLName, (wcslen(Prov->pDLLName)+1) * sizeof(WCHAR));
            RegCloseKey(hSubKey);
         }

         pBuffer = HeapAlloc(GetProcessHeap(), 0, dwFullSize);
         if (pBuffer)
         {
             CopyMemory(pBuffer, pOrder, dwSize);
             wcscpy(&pBuffer[(dwSize/sizeof(WCHAR))-1], Prov->pName);
             pBuffer[(dwSize/sizeof(WCHAR)) + wcslen(Prov->pName)] = L'\0';
             RegSetValueExW(hKey, L"Order", 0, REG_MULTI_SZ, (LPBYTE)pBuffer, dwFullSize);
             HeapFree(GetProcessHeap(), 0, pBuffer);
         }
         bRet = TRUE;
      }

   }

   RegCloseKey(hKey);
   HeapFree(GetProcessHeap(), 0, pOrder);

  return bRet;
}

/******************************************************************************
 *    DeletePrintProvidorA   (WINSPOOL.@)
 */
BOOL
STDCALL
DeletePrintProvidorA(LPSTR Name, LPSTR Environment, LPSTR PrintProvidor)
{
   BOOL bRet;
   LPWSTR Env, Prov;

   if (Name || !Environment || !PrintProvidor)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   Env = HeapAlloc(GetProcessHeap(), 0, (strlen(Environment)+1) * sizeof(WCHAR));
   if (!Env)
   {
      return FALSE;
   }

   MultiByteToWideChar(CP_ACP, 0, Environment, -1, Env, strlen(Environment)+1);
   Env[strlen(Environment)] = L'\0';

   Prov = HeapAlloc(GetProcessHeap(), 0, (strlen(PrintProvidor)+1) * sizeof(WCHAR));
   if (!Prov)
   {
      HeapFree(GetProcessHeap(), 0, Env);
      return FALSE;
   }

   MultiByteToWideChar(CP_ACP, 0, PrintProvidor, -1, Prov, strlen(PrintProvidor)+1);
   Prov[strlen(PrintProvidor)] = L'\0';

   bRet = DeletePrintProvidorW(NULL, Env, Prov);
   HeapFree(GetProcessHeap(), 0, Env);
   HeapFree(GetProcessHeap(), 0, Prov);

  return bRet;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrintProvidorW(LPWSTR Name, LPWSTR Environment, LPWSTR PrintProvidor)
{
   HKEY hKey;
   BOOL bFound;
   DWORD dwType, dwSize, dwOffset, dwLength;
   LPWSTR pOrder, pBuffer, pNew;

   if (Name || !Environment || !PrintProvidor)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Print\\Providers", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
   {
      return FALSE;
   }

   if (RegQueryValueExW(hKey, L"Order", NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
   {
      RegCloseKey(hKey);
      return FALSE;
   }

   pOrder = HeapAlloc(GetProcessHeap(), 0, dwSize);
   if (!pOrder)
   {
      RegCloseKey(hKey);
      return FALSE;
   }

   if (RegQueryValueExW(hKey, L"Order", NULL, &dwType, (LPBYTE)pOrder, &dwSize) != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
   {
      RegCloseKey(hKey);
      return FALSE;
   }


   pBuffer = pOrder;
   bFound = FALSE;
   while(pBuffer[0])
   {
       if (!wcsicmp(pBuffer, PrintProvidor))
       {
            bFound = TRUE;
            break;
         }
         pBuffer += wcslen(pBuffer) + 1;
   }

   if (!bFound)
   {
      RegCloseKey(hKey);
      HeapFree(GetProcessHeap(), 0, pOrder);
      return FALSE;
   }

   pNew = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
   if (!pNew)
   {
      RegCloseKey(hKey);
      HeapFree(GetProcessHeap(), 0, pOrder);
      return FALSE;
   }

   dwOffset = pBuffer - pOrder;
   dwLength = (dwSize / sizeof(WCHAR)) - (dwOffset + wcslen(pBuffer) + 1);
   CopyMemory(pNew, pOrder, dwOffset * sizeof(WCHAR));
   CopyMemory(&pNew[dwOffset], pBuffer + wcslen(pBuffer) + 1, dwLength);

   RegSetValueExW(hKey, L"Order", 0, REG_MULTI_SZ, (LPBYTE)pNew, (dwOffset + dwLength) * sizeof(WCHAR));
   RegDeleteKey(hKey, PrintProvidor);

   HeapFree(GetProcessHeap(), 0, pOrder);
   HeapFree(GetProcessHeap(), 0, pNew);
   RegCloseKey(hKey);

   return TRUE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddMonitorA(LPSTR Name, DWORD Level, PBYTE Monitors)
{
   LPWSTR szName = NULL;
   MONITOR_INFO_2W Monitor;
   MONITOR_INFO_2A *pMonitor;
   BOOL bRet = FALSE;

   if (Level != 2 || !Monitors)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   pMonitor = (MONITOR_INFO_2A*)Monitors;
   if (pMonitor->pDLLName == NULL || pMonitor->pName == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   ZeroMemory(&Monitor, sizeof(Monitor));

   if (Name)
   {
      szName = HeapAlloc(GetProcessHeap(), 0, (strlen(Name) + 1) * sizeof(WCHAR));
      if (!szName)
      {
         return FALSE;
      }
      MultiByteToWideChar(CP_ACP, 0, Name, -1, szName, strlen(Name)+1);
      szName[strlen(Name)] = L'\0';
   }

   Monitor.pDLLName = HeapAlloc(GetProcessHeap(), 0, (strlen(pMonitor->pDLLName)+1) * sizeof(WCHAR));
   if (!Monitor.pDLLName)
   {
      goto cleanup;
   }
   MultiByteToWideChar(CP_ACP, 0, pMonitor->pDLLName, -1, Monitor.pDLLName, strlen(pMonitor->pDLLName)+1);
   pMonitor->pDLLName[strlen(pMonitor->pDLLName)] = L'\0';

   Monitor.pName = HeapAlloc(GetProcessHeap(), 0, (strlen(pMonitor->pName)+1) * sizeof(WCHAR));
   if (!Monitor.pName)
   {
      goto cleanup;
   }
   MultiByteToWideChar(CP_ACP, 0, pMonitor->pName, -1, Monitor.pName, strlen(pMonitor->pName)+1);
   pMonitor->pName[strlen(pMonitor->pName)] = L'\0';


   if (pMonitor->pEnvironment)
   {
      Monitor.pEnvironment = HeapAlloc(GetProcessHeap(), 0, (strlen(pMonitor->pEnvironment)+1) * sizeof(WCHAR));
      if (!Monitor.pEnvironment)
      {
         goto cleanup;
      }
      MultiByteToWideChar(CP_ACP, 0, pMonitor->pEnvironment, -1, Monitor.pEnvironment, strlen(pMonitor->pEnvironment)+1);
      pMonitor->pEnvironment[strlen(pMonitor->pEnvironment)] = L'\0';
   }

   bRet = AddMonitorW(szName, Level, (LPBYTE)&Monitor);

cleanup:

  if (szName)
     HeapFree(GetProcessHeap(), 0, szName);

  if (Monitor.pDLLName)
     HeapFree(GetProcessHeap(), 0, Monitor.pDLLName);

  if (Monitor.pEnvironment)
     HeapFree(GetProcessHeap(), 0, Monitor.pEnvironment);

  if (Monitor.pName)
     HeapFree(GetProcessHeap(), 0, Monitor.pName);

  return bRet;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddMonitorW(LPWSTR Name, DWORD Level, PBYTE Monitors)
{
   WCHAR szPath[MAX_PATH];
   HMODULE hLibrary = NULL;
   FARPROC InitProc;
   HKEY hKey, hSubKey;
   MONITOR_INFO_2W * pMonitor;


   if (Level != 2 || !Monitors)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   pMonitor = (MONITOR_INFO_2W*)Monitors;

   if (pMonitor->pDLLName == NULL || pMonitor->pName == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if (wcschr(pMonitor->pDLLName, L'\\'))
   {
       hLibrary = LoadLibraryExW(pMonitor->pDLLName, NULL, 0);
   }
   else if (GetSystemDirectoryW(szPath, MAX_PATH) && PathAddBackslashW(szPath))
   {
      wcscat(szPath, pMonitor->pDLLName);
      hLibrary = LoadLibraryExW(szPath, NULL, 0);
   }

   if (!hLibrary)
   {
      return FALSE;
   }

   InitProc = GetProcAddress(hLibrary, "InitializePrintMonitor");
   if (!InitProc)
   {
      InitProc = GetProcAddress(hLibrary, "InitializePrintMonitor2");
      if (!InitProc)
      {
         FreeLibrary(hLibrary);
         SetLastError(ERROR_PROC_NOT_FOUND);
         return FALSE;
      }
   }

   // FIXME
   // Initialize monitor
   FreeLibrary(hLibrary);

   if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
   {
      if (RegCreateKeyExW(hKey, pMonitor->pName, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
      {
         RegSetValueExW(hSubKey, L"Driver", 0, REG_SZ, (LPBYTE)pMonitor->pDLLName, (wcslen(pMonitor->pDLLName)+1)*sizeof(WCHAR));
         RegCloseKey(hSubKey);
      }
      RegCloseKey(hKey);
   }
   return TRUE;
}

