/////////////////////////////////////////////////////////////////////////////
// PREVIEW.H
//
// Declaration of CPreviewWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
// jaym     02/03/97    Updated to use new CDIB class
/////////////////////////////////////////////////////////////////////////////
#ifndef __PREVIEW_H__
#define __PREVIEW_H__

class CPreviewWindow;

#include "wnd.h"
#include "dib.h"

/////////////////////////////////////////////////////////////////////////////
// CPreviewWindow
/////////////////////////////////////////////////////////////////////////////
class CPreviewWindow : public CWindow
{
// Construction/destruction
public:
    CPreviewWindow();
    virtual ~CPreviewWindow();

    BOOL Create(const RECT & rect, HWND hwndParent);

protected:
    void OnDestroy();
    void OnPaint(HDC hDC, PAINTSTRUCT * ps);
    void OnPaletteChanged(HWND hwndPalChng);
    BOOL OnQueryNewPalette();

protected:
    CDIB * m_pDIB;
};

#endif  // __PREVIEW_H__
