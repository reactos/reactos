//////////////////////////////////////////////////////////////////////////////
/*  File: subclass.cpp

    Description: Helps with subclassing a window.  See header comments in 
        subclass.h for details.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/16/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "subclass.h"
#include "msgbox.h"

//
// Window property name for storing the "this" ptr per window instance.
//
const TCHAR WindowSubclass::m_szPropThis[] = TEXT("WSCPROP_THISPTR");

WindowSubclass::WindowSubclass(
    void
    ) : m_hwnd(NULL),
        m_lpfnWndProc(NULL)
{

}


WindowSubclass::~WindowSubclass(
    void
    )
{
    Cancel();
}


bool
WindowSubclass::Initialize(
    HWND hwnd
    )
{
    bool bResult = false;
    m_hwnd = hwnd;
    if (m_hwnd)
        bResult = Resume();

    return bResult;
}


bool
WindowSubclass::Resume(
    void
    )
{
    bool bResult = false;
    if (NULL != m_hwnd)
    {
        if (SetProp(m_hwnd, m_szPropThis, (HANDLE)this))
        {
            SetLastError(0);
            m_lpfnWndProc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (INT_PTR)WndProc);
            bResult = (ERROR_SUCCESS == GetLastError());
        }
    }
    return bResult;
}


void
WindowSubclass::Cancel(
    void
    )
{
    if (NULL != m_hwnd)
    {
        RemoveProp(m_hwnd, m_szPropThis);
        if (NULL != m_lpfnWndProc)
        {
            SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (INT_PTR)m_lpfnWndProc);
            m_lpfnWndProc = NULL;
        }
        m_hwnd = NULL;
    }
}        


LRESULT CALLBACK
WindowSubclass::WndProc(
    HWND hwnd, 
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    WindowSubclass *pThis = (WindowSubclass *)GetProp(hwnd, m_szPropThis);
   
    try
    {
        pThis->HandleMessages(hwnd, message, wParam, lParam);
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in WindowSubclass::WndProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in WindowSubclass::WndProc")));
        CscWin32Message(NULL, DISP_E_EXCEPTION, CSCUI::SEV_ERROR);
    }
    return CallWindowProc(pThis->m_lpfnWndProc, hwnd, message, wParam, lParam);
}

