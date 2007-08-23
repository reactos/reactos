/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/reg.c
 * PURPOSE:     Query service information
 * COPYRIGHT:   Copyright 2005 - 2007 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

/* Sets the service description in the registry */
BOOL SetDescription(LPTSTR ServiceName, LPTSTR Description)
{
    HKEY hKey;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR buf[300];
    TCHAR szBuf[MAX_PATH];
    LONG val;


   /* open the registry key for the service */
    _sntprintf(buf, sizeof(buf) / sizeof(TCHAR), Path, ServiceName);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 buf,
                 0,
                 KEY_WRITE,
                 &hKey);


   if ((val = RegSetValueEx(hKey,
                     _T("Description"),
                     0,
                     REG_SZ,
                     (LPBYTE)Description,
                     (DWORD)lstrlen(szBuf)+1)) != ERROR_SUCCESS)
   {
       //GetError(val);
       return FALSE;
   }


    RegCloseKey(hKey);
    return TRUE;
}



/* Retrives the service description from the registry */
LPTSTR
GetDescription(LPTSTR lpServiceName)
{
    HKEY hKey;
    LPTSTR lpDescription = NULL;
    DWORD dwValueSize = 0;
    LONG ret;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR buf[300];

    /* open the registry key for the service */
    _sntprintf(buf, sizeof(buf) / sizeof(TCHAR), Path, lpServiceName);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 buf,
                 0,
                 KEY_READ,
                 &hKey);

    ret = RegQueryValueEx(hKey,
                          _T("Description"),
                          NULL,
                          NULL,
                          NULL,
                          &dwValueSize);
    if (ret != ERROR_SUCCESS && ret != ERROR_FILE_NOT_FOUND && ret != ERROR_INVALID_HANDLE)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if (ret != ERROR_FILE_NOT_FOUND)
    {
        lpDescription = HeapAlloc(ProcessHeap,
                                  HEAP_ZERO_MEMORY,
                                  dwValueSize);
        if (lpDescription == NULL)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        if(RegQueryValueEx(hKey,
                           _T("Description"),
                           NULL,
                           NULL,
                           (LPBYTE)lpDescription,
                           &dwValueSize))
        {
            HeapFree(ProcessHeap,
                     0,
                     lpDescription);
            RegCloseKey(hKey);
        }
    }

    return lpDescription;
}