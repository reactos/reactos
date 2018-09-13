//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msgbox.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCDLL_MSGBOX_H
#define _INC_CSCDLL_MSGBOX_H

class Win32Error
{
    public:
        explicit Win32Error(DWORD dwWin32Error)
            : m_dwError(dwWin32Error) { }

        DWORD Code(void) const
            { return m_dwError; }

    private:
        DWORD m_dwError;
};


namespace CSCUI 
{
    enum Severity { SEV_ERROR = 0, 
                    SEV_WARNING, 
                    SEV_INFORMATION };
}



INT CscMessageBox(HWND hwndParent, UINT uType, const Win32Error& error);
INT CscMessageBox(HWND hwndParent, UINT uType, const Win32Error& error, LPCTSTR pszMsgText);
INT CscMessageBox(HWND hwndParent, UINT uType, const Win32Error& error, HINSTANCE hInstance, UINT idMsgText, ...);
INT CscMessageBox(HWND hwndParent, UINT uType, LPCTSTR pszMsgText);
INT CscMessageBox(HWND hwndParent, UINT uType, HINSTANCE hInstance, UINT idMsgText, ...);
INT CscWin32Message(HWND hwndParent, DWORD dwError, CSCUI::Severity severity);



#endif // INC_CSCDLL_MSGBOX_H

