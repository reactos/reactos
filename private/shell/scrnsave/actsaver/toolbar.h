/////////////////////////////////////////////////////////////////////////////
// TOOLBAR.H
//
// Definition of CToolbarWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     05/15/97    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__

class CToolbarWindow;

#include "wnd.h"
#include "sswnd.h"

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow
/////////////////////////////////////////////////////////////////////////////
class CToolbarWindow : public CWindow
{
friend CWindow;

// Construction/destruction
public:
    CToolbarWindow();
    virtual ~CToolbarWindow();

    virtual BOOL Create(const RECT & rect, CScreenSaverWindow * pParentWnd);

    void ShowToolbar(BOOL bShow);

// Data
protected:
    CScreenSaverWindow *    m_pSSWnd;
    int                     m_cy;
    HWND                    m_hwndProperties;
    HWND                    m_hwndClose;

// Overrides
protected:
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnEraseBkgnd(HDC hDC);
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT * plResult); 
    virtual void OnShowWindow(BOOL bShow, int nStatus);
};

#endif  // __TOOLBAR_H__

