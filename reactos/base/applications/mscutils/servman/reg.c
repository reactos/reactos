/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/reg.c
 * PURPOSE:     functions for querying a services registry key
 * COPYRIGHT:   Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static HKEY
OpenServiceKey(LPTSTR lpServiceName)
{
    HKEY hKey = NULL;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR buf[300];

    _sntprintf(buf, sizeof(buf) / sizeof(TCHAR), Path, lpServiceName);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     buf,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        return hKey;
    }
    else
    {
        return NULL;
    }
}

BOOL 
SetDescription(LPTSTR lpServiceName,
               LPTSTR lpDescription)
{
    HKEY hKey;
    TCHAR szBuf[MAX_PATH];
    BOOL bRet = FALSE;

    hKey = OpenServiceKey(lpServiceName);
    if (hKey)
    {
       if (RegSetValueEx(hKey,
                         _T("Description"),
                         0,
                         REG_SZ,
                         (LPBYTE)lpDescription,
                         (DWORD)(_tcslen(szBuf) + 1 ) * sizeof(TCHAR)) == ERROR_SUCCESS)
       {
           bRet = TRUE;
       }

        RegCloseKey(hKey);
    }

    return bRet;
}


LPTSTR
GetDescription(LPTSTR lpServiceName)
{
    HKEY hKey;
    LPTSTR lpDescription = NULL;
    DWORD dwValueSize = 0;

    hKey = OpenServiceKey(lpServiceName);
    if (hKey)
    {
        if (RegQueryValueEx(hKey,
                            _T("Description"),
                            NULL,
                            NULL,
                            NULL,
                            &dwValueSize) == ERROR_SUCCESS)
        {
            lpDescription = HeapAlloc(ProcessHeap,
                                      0,
                                      dwValueSize);
            if (lpDescription)
            {
                if(RegQueryValueEx(hKey,
                                   _T("Description"),
                                   NULL,
                                   NULL,
                                   (LPBYTE)lpDescription,
                                   &dwValueSize) != ERROR_SUCCESS)
                {
                    HeapFree(ProcessHeap,
                             0,
                             lpDescription);
                }
            }
        }

        RegCloseKey(hKey);
    }

    return lpDescription;
}
