/////////////////////////////////////////////////////////////////////////////
// PREVIEW.CPP
//
// Implementation of CPreviewWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
// jaym     02/03/97    Updated to use new CDIB class
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "dib.h"
#include "resource.h"
#include "preview.h"

/////////////////////////////////////////////////////////////////////////////
// CPreviewWindow
/////////////////////////////////////////////////////////////////////////////
CPreviewWindow::CPreviewWindow
(
)
{
    m_pDIB = NULL;
}

CPreviewWindow::~CPreviewWindow
(
)
{
    if (m_pDIB != NULL)
        delete m_pDIB;
}

/////////////////////////////////////////////////////////////////////////////
// CPreviewWindow::Create
/////////////////////////////////////////////////////////////////////////////
BOOL CPreviewWindow::Create
(
    const RECT &    rect,
    HWND            hwndParent
)
{
    BOOL bResult = FALSE;

    for (;;)
    {
        m_pDIB = new CDIB;

        if (m_pDIB == NULL)
            break;

        if (!m_pDIB->LoadFromResource(  _pModule->GetResourceInstance(),
                                        IDB_PREVIEW))
        {
            break;
        }

        bResult = CWindow::Create(  NULL,
                                    ((hwndParent != NULL) ? WS_CHILD
                                                          : WS_POPUP),
                                    rect,
                                    hwndParent,
                                    0);
        break;
    }

    if (!bResult)
    {
        // Cleanup
        if (m_pDIB != NULL)
            delete m_pDIB;
    }
    else
        ShowWindow(SW_SHOWNORMAL);

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CPreviewWindow::OnDestroy
/////////////////////////////////////////////////////////////////////////////
void CPreviewWindow::OnDestroy
(
)
{
    CWindow::OnDestroy();
    PostQuitMessage(0);
}

/////////////////////////////////////////////////////////////////////////////
// CPreviewWindow::OnPaint
/////////////////////////////////////////////////////////////////////////////
 void CPreviewWindow::OnPaint
(
    HDC             hDC,
    PAINTSTRUCT *   ps
)
{
    if (m_pDIB != NULL)
    {
        RECT rectDraw;

        GetClientRect(&rectDraw);
        m_pDIB->Draw(hDC, NULL, &rectDraw);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CPreviewWindow::OnPaletteChanged
/////////////////////////////////////////////////////////////////////////////
void CPreviewWindow::OnPaletteChanged
(
    HWND hwndPalChng
)
{
    // If we caused the palette change, do nothing.
    if (hwndPalChng == m_hWnd)
        return;

    // Re-realize the palette.
    OnQueryNewPalette();
}

/////////////////////////////////////////////////////////////////////////////
// CPreviewWindow::OnQueryNewPalette
/////////////////////////////////////////////////////////////////////////////
BOOL CPreviewWindow::OnQueryNewPalette
(
)
{
    HDC         hDC;
    HPALETTE    hPalOld;
    UINT        uRemapCount;

    if ((hDC = GetDC()) == NULL)
        return FALSE;

    hPalOld = SelectPalette(hDC, m_pDIB->m_hPalette, FALSE);
    uRemapCount = RealizePalette(hDC);

    if (hPalOld != NULL)
        SelectPalette(hDC, hPalOld, TRUE);

    if (uRemapCount != 0)
        SysPalChanged();

    ReleaseDC(hDC);

    return (BOOL)uRemapCount;
}

