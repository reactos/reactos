//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       paddebug.hxx
//
//  Contents:   CDebugWindow class.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_PADDEBUG_HXX_
#define X_PADDEBUG_HXX_
#include "paddebug.hxx"
#endif

BOOL    CDebugWindow::s_fDebugWndClassRegistered = FALSE;


//+-------------------------------------------------------------------
//
// Member:    CDebugWindow ::CDebugWindow
//
// Synopsis:
//
//--------------------------------------------------------------------
CDebugWindow::CDebugWindow (CPadDoc * pDoc)
{     
    _pDoc = pDoc;
}


CDebugWindow::~CDebugWindow ()
{     
}


HRESULT
CDebugWindow::RegisterDebugWndClass()
{
    WNDCLASS    wc;

    // Protect s_fDebugWndClassRegistered
    
    if (!s_fDebugWndClassRegistered)
    {
        LOCK_GLOBALS;

        if (!s_fDebugWndClassRegistered)
        {
            memset(&wc, 0, sizeof(wc));
            wc.lpfnWndProc = CDebugWindow::WndProc;                                                                            // windows of this class.
            wc.hInstance = g_hInstCore;
            wc.hIcon = LoadIcon(g_hInstResource, MAKEINTRESOURCE(IDR_PADICON));
            wc.lpszMenuName =  NULL;
            wc.lpszClassName = SZ_APPLICATION_NAME TEXT(" Debug Window");
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);

            if (!RegisterClass(&wc))
            {
                return E_FAIL;
            }
            s_fDebugWndClassRegistered = TRUE;
        }
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
// Member:    CDebugWindow ::Init
//
// Synopsis:
//
//--------------------------------------------------------------------
HRESULT
CDebugWindow::Init()
{
    
    HRESULT     hr = S_OK;
    RECT        rc;
    
    hr = THR(RegisterDebugWndClass());
    if (hr)
        goto Error;

   _hwnd = CreateWindowEx(
            0,
            SZ_APPLICATION_NAME TEXT(" Debug Window"),
            SZ_APPLICATION_NAME TEXT(" Debug Window"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            NULL,
            NULL,
            g_hInstCore,
            this);

    if (!_hwnd)
    {
        hr = E_FAIL;
        goto Error;
    }


    GetClientRect (_hwnd, &rc);

    _hwndText = CreateWindowEx(
        NULL,
        TEXT("Edit"),
        NULL,
        WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | 
            WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        0, 0, rc.left - rc.right, rc.top - rc.bottom,
        _hwnd,                 
        0,               
        g_hInstCore,          
        NULL);    
    
    if(!_hwndText)
    {
        hr = E_FAIL;
        goto Error;             
    }
  
    SetWindowLongPtr(_hwndText, GWLP_USERDATA, (LONG_PTR) this);
    _wpOrigEditProc = (WNDPROC) SetWindowLongPtr(_hwndText, GWLP_WNDPROC, (LONG_PTR) DebugTextWndProc); 
 
    RRETURN(hr);

Error:
    if(_hwnd)
        DestroyWindow(_hwnd); 
    if(_hwndText)
        DestroyWindow(_hwndText); 
    RRETURN(hr);
}

void
CDebugWindow::Destroy()
{
    if(_hwndText)
        DestroyWindow(_hwndText);
    if(_hwnd)
        DestroyWindow(_hwnd);
}


LRESULT
CDebugWindow::ExecuteScript ()
{
    TCHAR achCommand[256], achString[256];
    LONG cch, lLine;
    DWORD dwStart, dwEnd;
    //HWND hwndAct;
        
    // Retrieve current cursor position
    SendMessage(_hwndText, EM_GETSEL,(WPARAM) &dwStart, (LPARAM) &dwEnd); 
    
    // Retrieve current line index
    lLine = SendMessage(_hwndText, EM_LINEFROMCHAR,(WPARAM) dwStart, 0);
    
    // Retrieve line text
    achCommand[0] = (TCHAR) ARRAY_SIZE(achCommand)-1; 
    cch = SendMessage(_hwndText, EM_GETLINE, lLine, (LPARAM)achCommand);

    // if the end of the buffer has been overwritten!!!
    Assert(cch < ARRAY_SIZE(achCommand)-1);

    achCommand[cch] = TEXT('\0');

    // Replace ? at start of script by PrintDebug command
    cch = 0;

    while (achCommand[cch] == TEXT(' '))
        cch++;

    if (achCommand[cch] == TEXT('?')) 
    {
        wcscpy(achString,TEXT ("PrintDebug"));
        wcscat(achString,achCommand + 1);
        wcscpy(achCommand,achString);
    }
            
    // Move cursor to next line, creating one if necessary
    NextLine();
            
    // BUGBUG: chrisf - necessary if SendKeys goes to focus window
    //hwndAct = SetActiveWindow(_pDoc->_hwnd);

    IGNORE_HR(_pDoc->ExecuteTopLevelScriptlet(achCommand));
    
    //SetActiveWindow(hwndAct);

    return 0;
}


LRESULT CALLBACK
CDebugWindow::DebugTextWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    CDebugWindow * pDebug = (CDebugWindow *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
                    
    switch (wm)
    {
    case WM_CHAR :
        if (wParam == '\r')      //carriage return
        {
            return pDebug->ExecuteScript();
        }
        break;
    }

    return CallWindowProc(pDebug->_wpOrigEditProc, hwnd, wm, wParam, lParam); 
   
}

HRESULT
CDebugWindow::Print(BSTR PrintValue)
{
    LONG lLine;
    LONG lLineLength;
    LONG lLineStart;
    DWORD dwStart, dwEnd;

    CDebugWindow * pDebug = (CDebugWindow *) GetWindowLongPtr(_hwnd, GWLP_USERDATA);

    // Get cursor position
    SendMessage(pDebug->_hwndText, EM_GETSEL, (WPARAM) &dwStart, (LPARAM) &dwEnd); 
    
    // Retrieve line at cursor position
    lLine = SendMessage (pDebug->_hwndText, EM_LINEFROMCHAR, (WPARAM) dwStart, 0);
    
    // Retrieve line start and length
    lLineLength = SendMessage(_hwndText, EM_LINELENGTH, (WPARAM) dwStart, 0);
    lLineStart = SendMessage(_hwndText, EM_LINEINDEX, (WPARAM) lLine, 0);

    // Selection entire line
    SendMessage(_hwndText, EM_SETSEL, lLineStart, lLineStart + lLineLength);
    
    // And replace with string to print
    SendMessage(
        pDebug->_hwndText, EM_REPLACESEL, 0, LPARAM( STRVAL( PrintValue ) ) );

    // Then move to next line
    NextLine();
    
    return S_OK;
}

HRESULT
CDebugWindow::NextLine()
{ 
    DWORD dwStart, dwEnd;
    LONG lLine, lLineLength, lLineStart;
    LONG lCount;

    lCount = SendMessage(_hwndText, EM_GETLINECOUNT,0, 0L);
    SendMessage(_hwndText, EM_GETSEL, (WPARAM) &dwStart, (LPARAM) &dwEnd); 
    lLine = SendMessage(_hwndText, EM_LINEFROMCHAR, (WPARAM) dwStart, 0);
    lLineLength = SendMessage(_hwndText, EM_LINELENGTH, (WPARAM) dwStart, 0);
    lLineStart = SendMessage(_hwndText, EM_LINEINDEX, (WPARAM) lLine, 0);

    if (lLine + 1 < lCount)
    {
        SendMessage(_hwndText, EM_SETSEL, lLineStart + lLineLength + 2, lLineStart + lLineLength + 2);
    }
    else
    {
        SendMessage(_hwndText, EM_SETSEL, lLineStart + lLineLength, lLineStart + lLineLength);
        CallWindowProc(_wpOrigEditProc, _hwndText, WM_CHAR, '\r', 1); 
    }

    return S_OK;
}


LRESULT
CDebugWindow::DebugWindowOnClose()
{
    ::ShowWindow(_hwnd, SW_HIDE);
    return 0;
}


LRESULT CALLBACK
CDebugWindow::WndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{

    CDebugWindow * pDebug = (CDebugWindow *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (wm)
    {
    case WM_NCCREATE:
        {
            pDebug = (CDebugWindow *) ((LPCREATESTRUCTW) lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pDebug);
            pDebug->_hwnd = hwnd;
        }
        break;
    case WM_NCDESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            pDebug->_pDoc->_pDebugWindow = NULL;
            delete pDebug;
        }
        break;
    case WM_CLOSE:
        pDebug->DebugWindowOnClose();
        return 0;
    case WM_SIZE:
        ::MoveWindow (pDebug->_hwndText,0,0,LOWORD (lParam), HIWORD (lParam), TRUE);
        return 0;
    case WM_ACTIVATE:
        if(wParam != WA_INACTIVE)
        {
            SetFocus(pDebug->_hwndText);
        }
        return 0;
    }
    return DefWindowProc(hwnd, wm, wParam, lParam);
}
