//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msgbox.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "msgbox.h"


//
// Helper to report system errors.
// Merely ensures a little consistency with respect to message box
// flags.
//
// Example: 
//
//      CscWin32Message(hwndMain,
//                      ERROR_NOT_ENOUGH_MEMORY,
//                      CSCUI::SEV_ERROR);
// 
INT
CscWin32Message(
    HWND hwndParent,
    DWORD dwError,    // From GetLastError().
    CSCUI::Severity severity
    )
{
    UINT uType = MB_OK;

    switch(severity)
    {
        case CSCUI::SEV_ERROR:       
            uType |= MB_ICONERROR;   
            break;
        case CSCUI::SEV_WARNING:     
            uType |= MB_ICONWARNING; 
            break;
        case CSCUI::SEV_INFORMATION: 
            uType |= MB_ICONINFORMATION; 
            break;
        default: break;
    }

    return CscMessageBox(hwndParent, uType, Win32Error(dwError));
}


INT
CscMBLoadString(
    HINSTANCE hInstance,
    UINT idStr,
    LPTSTR *ppszOut
    )
{
    int cch = 0;
    TCHAR szBuffer[MAX_PATH];
    int cchResource = SizeofStringResource(hInstance, idStr);
    if (0 < cchResource)
    {
        cchResource++; // Add for nul term.
        *ppszOut = (LPTSTR)LocalAlloc(LMEM_FIXED, cchResource * sizeof(TCHAR));
        if (NULL != *ppszOut)
        {
            cch = LoadString(hInstance, idStr, *ppszOut, cchResource);
            if (0 == cch)
            {
                LocalFree(*ppszOut);
                *ppszOut = NULL;
            }
        }
    }
    return cch;
}


INT
CscMBFormatMessage(
    HINSTANCE hInstance,
    UINT idStr,
    LPTSTR *ppszOut,
    va_list *pargs
    )
{
    INT iResult   = -1;
    LPTSTR pszFmt = NULL;
    int cchLoaded = CscMBLoadString(hInstance, idStr, &pszFmt);
    if (0 == cchLoaded)
    {
        TraceMsg("String resource error");
        throw "resource error"; // Don't want to return a value.
    }

    cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                pszFmt,
                                0,
                                0,
                                (LPTSTR)ppszOut,
                                1,
                                pargs);
    LocalFree(pszFmt);
    return cchLoaded;
}

//
// Display a system error message in a message box.
// The Win32Error class was created to eliminate any signature ambiguities
// with the other versions of CscMessageBox.
//
// Example:  
//
//  CscMessageBox(hwndMain, 
//                MB_OK | MB_ICONERROR, 
//                Win32Error(ERROR_NOT_ENOUGH_MEMORY));
//
INT
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    const Win32Error& error
    )
{
    INT iResult = -1;
    LPTSTR pszBuffer = NULL;
    INT cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
                                    FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    NULL,
                                    error.Code(),
                                    0,
                                    (LPTSTR)&pszBuffer,
                                    1,
                                    NULL);

    if (0 == cchLoaded)
    {
        TraceMsg("String resource error");
        throw "resource error"; // Don't want to return a value.
    }
    if (NULL != pszBuffer)
    {
        iResult = CscMessageBox(hwndParent, uType, pszBuffer);
        LocalFree(pszBuffer);
    }
    return iResult;
}



//
// Display a system error message in a message box with additional
// text.  
//
// Example:  
//
//  CscMessageBox(hwndMain, 
//                MB_OK | MB_ICONERROR, 
//                Win32Error(ERROR_NOT_ENOUGH_MEMORY),
//                IDS_FMT_LOADINGFILE,
//                pszFile);
//
INT 
CscMessageBox(
    HWND hwndParent, 
    UINT uType, 
    const Win32Error& error, 
    LPCTSTR pszMsgText
    )
{
    INT iResult = -1;
    int cchMsg = lstrlen(pszMsgText);
    LPTSTR pszBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, (cchMsg + MAX_PATH) * sizeof(TCHAR));
    if (NULL != pszBuffer)
    {
        lstrcpy(pszBuffer, pszMsgText);
        *(pszBuffer + cchMsg++) = TEXT('\n');
        *(pszBuffer + cchMsg++) = TEXT('\n');
        int cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                       NULL,
                                       error.Code(),
                                       0,
                                       pszBuffer + cchMsg,
                                       MAX_PATH - 2,         // 2 for newlines.
                                       NULL);
        if (0 != cchLoaded)
        {
            iResult = CscMessageBox(hwndParent, uType, pszBuffer);
        }
        LocalFree(pszBuffer);
    }
    return iResult;
}


INT
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    const Win32Error& error,
    HINSTANCE hInstance,
    UINT idMsgText,
    va_list *pargs
    )
{
    INT iResult      = -1;
    LPTSTR pszBuffer = NULL;
    int cchLoaded = CscMBFormatMessage(hInstance,
                                       idMsgText,
                                       &pszBuffer,
                                       pargs);
    if (0 == cchLoaded)
    {
        TraceMsg("String resource error");
        throw "resource error"; // Don't want to return a value.
    }

    iResult = CscMessageBox(hwndParent, uType, error, pszBuffer);
    LocalFree(pszBuffer);
    return iResult;
}


INT 
CscMessageBox(
    HWND hwndParent, 
    UINT uType, 
    const Win32Error& error, 
    HINSTANCE hInstance,
    UINT idMsgText, 
    ...
    )
{
    INT iResult = -1;
    va_list args;
    va_start(args, idMsgText);
    iResult = CscMessageBox(hwndParent, uType, error, hInstance, idMsgText, &args);
    va_end(args);
    return iResult;
}



//
// Example:  
//
//  CscMessageBox(hwndMain, 
//                MB_OK | MB_ICONWARNING, 
//                TEXT("File %1 could not be deleted"), pszFilename);
//
INT
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    HINSTANCE hInstance,
    UINT idMsgText,
    va_list *pargs
    )
{
    INT iResult   = -1;
    LPTSTR pszFmt = NULL;
    int cchLoaded = CscMBLoadString(hInstance, idMsgText, &pszFmt);
    if (0 == cchLoaded)
    {
        TraceMsg("String resource error");
        throw "resource error"; // Don't want to return a value.
    }

    LPTSTR pszMsg = NULL;
    cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                pszFmt,
                                0,
                                0,
                                (LPTSTR)&pszMsg,
                                1,
                                pargs);
    LocalFree(pszFmt);
    if (0 == cchLoaded)
    {
        TraceMsg("String resource error");
        throw "resource error"; // Don't want to return a value.
    }

    iResult = CscMessageBox(hwndParent, uType, pszMsg);
    LocalFree(pszMsg);
    return iResult;
}


INT
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    HINSTANCE hInstance,
    UINT idMsgText,
    ...
    )
{
    INT iResult = -1;
    va_list args;
    va_start(args, idMsgText);
    iResult = CscMessageBox(hwndParent, uType, hInstance, idMsgText, &args);
    va_end(args);
    return iResult;
}


//
// All of the other variations of CscMessageBox() end up calling this one.
// 
INT
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    LPCTSTR pszMsgText
    )
{

    TCHAR szCaption[80] = { TEXT('\0') };
    LoadString(g_hInstance, IDS_APPLICATION, szCaption, ARRAYSIZE(szCaption));

    return MessageBox(hwndParent, pszMsgText, szCaption, uType);
}


