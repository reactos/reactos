/*
* PROJECT:     ReactOS Character Map
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        base/applications/charmap/GridView.cpp
* PURPOSE:     Class for for the window which contains the font matrix
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*/


#include "precomp.h"
#include "GridView.h"
#include "Cell.h"


/* DATA *****************************************************/

extern HINSTANCE g_hInstance;


/* PUBLIC METHODS **********************************************/

CGridView::CGridView() :
    m_xNumCells(20),
    m_yNumCells(10),
    m_ScrollPosition(0),
    m_NumRows(0)
{
    m_szMapWndClass = L"CharGridWClass";
}

CGridView::~CGridView()
{
}

bool
CGridView::Create(
    _In_ HWND hParent
    )
{
    WNDCLASSW wc = { 0 };
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MapWndProc;
    wc.cbWndExtra = sizeof(CGridView *);
    wc.hInstance = g_hInstance;
    wc.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = m_szMapWndClass;

    if (RegisterClassW(&wc))
    {
        m_hwnd = CreateWindowExW(0,
                                 m_szMapWndClass,
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL,
                                 0,0,0,0,
                                 hParent,
                                 NULL,
                                 g_hInstance,
                                 this);

    }

    return !!(m_hwnd != NULL);
}

bool
CGridView::SetFont(
    _In_ CAtlString& FontName
    )
{

    // Create a temporary container for the new font
    CurrentFont NewFont = { 0 };
    NewFont.FontName = FontName;

    // Get the DC for the full grid window
    HDC hdc;
    hdc = GetDC(m_hwnd);
    if (hdc == NULL) return false;

    // Setup the logfont structure
    NewFont.Font.lfHeight = 0; // This is set in WM_SIZE
    NewFont.Font.lfCharSet = DEFAULT_CHARSET;
    StringCchCopyW(NewFont.Font.lfFaceName, LF_FACESIZE, FontName);

    // Get a handle to the new font
    NewFont.hFont = CreateFontIndirectW(&NewFont.Font);
    if (NewFont.hFont == NULL)
    {
        ReleaseDC(m_hwnd, hdc);
        return false;
    }

    // Setup an array of all possible non-BMP indices
    WCHAR ch[MAX_GLYPHS];
    for (int i = 0; i < MAX_GLYPHS; i++)
        ch[i] = (WCHAR)i;

    HFONT hOldFont;
    hOldFont = (HFONT)SelectObject(hdc, NewFont.hFont);

    // Translate all the indices into glyphs
    WORD out[MAX_GLYPHS];
    DWORD Status;
    Status = GetGlyphIndicesW(hdc,
                              ch,
                              MAX_GLYPHS,
                              out,
                              GGI_MARK_NONEXISTING_GLYPHS);
    ReleaseDC(m_hwnd, hdc);
    if (Status == GDI_ERROR)
    {
        SelectObject(hdc, hOldFont);
        return false;
    }

    // Loop all the glyphs looking for valid ones
    // and store those in our font data
    int j = 0;
    for (int i = 0; i < MAX_GLYPHS; i++)
    {
        if (out[i] != 0xffff)
        {
            NewFont.ValidGlyphs[j] = ch[i];
            j++;
        }
    }
    NewFont.NumValidGlyphs = j;

    // Calculate the number of rows required to hold all glyphs
    m_NumRows = NewFont.NumValidGlyphs / m_xNumCells;
    if (NewFont.NumValidGlyphs % m_xNumCells)
        m_NumRows += 1;

    // Set the scrollbar in relation to the rows
    SetScrollRange(m_hwnd, SB_VERT, 0, m_NumRows - m_yNumCells, FALSE);

    // We're done, update the current font
    m_CurrentFont = NewFont;

    // We changed the font, we'll need to repaint the whole window
    InvalidateRect(m_hwnd,
                   NULL,
                   TRUE);

    return true;
}



/* PRIVATE METHODS **********************************************/

bool
CGridView::UpdateCellCoordinates(
    )
{
    // Go through all the cells and calculate
    // their coordinates within the grid
    for (int y = 0; y < m_yNumCells; y++)
    for (int x = 0; x < m_xNumCells; x++)
    {
        RECT CellCoordinates;
        CellCoordinates.left = x * m_CellSize.cx;
        CellCoordinates.top = y * m_CellSize.cy;
        CellCoordinates.right = (x + 1) * m_CellSize.cx + 1;
        CellCoordinates.bottom = (y + 1) * m_CellSize.cy + 1;

        m_Cells[y][x]->SetCellCoordinates(CellCoordinates);
    }

    return true;
}

LRESULT
CGridView::OnCreate(
    _In_ HWND hwnd,
    _In_ HWND hParent
    )
{
    m_hwnd = hwnd;
    m_hParent = hParent;

    // C++ doesn't allow : "CCells ***C = new CCell***[x * y]"
    // so we have to build the 2d array up manually
    m_Cells = new CCell**[m_yNumCells]; // rows
    for (int i = 0; i < m_yNumCells; i++)
        m_Cells[i] = new CCell*[m_xNumCells]; // columns

    for (int y = 0; y < m_yNumCells; y++)
    for (int x = 0; x < m_xNumCells; x++)
    {
        m_Cells[y][x] = new CCell(m_hwnd);
    }

    // Give the first cell focus
    SetCellFocus(m_Cells[0][0]);

    return 0;
}

LRESULT
CGridView::OnSize(
    _In_ INT Width,
    _In_ INT Height
    )
{
    // Get the client area of the main dialog
    RECT ParentRect;
    GetClientRect(m_hParent, &ParentRect);

    // Calculate the grid size using the parent
    m_ClientCoordinates.left = ParentRect.left + 25;
    m_ClientCoordinates.top = ParentRect.top + 50;
    m_ClientCoordinates.right = ParentRect.right - m_ClientCoordinates.left - 10;
    m_ClientCoordinates.bottom = ParentRect.bottom - m_ClientCoordinates.top - 70;

    // Resize the grid window
    SetWindowPos(m_hwnd,
                 NULL,
                 m_ClientCoordinates.left,
                 m_ClientCoordinates.top,
                 m_ClientCoordinates.right,
                 m_ClientCoordinates.bottom,
                 SWP_NOZORDER | SWP_SHOWWINDOW);

    // Get the client area we can draw on. The position we set above includes
    // a scrollbar which we obviously can't draw on. GetClientRect gives us
    // the size without the scroll, and it's more efficient than getting the
    // scroll metrics and calculating the size from that
    RECT ClientRect;
    GetClientRect(m_hwnd, &ClientRect);
    m_CellSize.cx = ClientRect.right / m_xNumCells;
    m_CellSize.cy = ClientRect.bottom / m_yNumCells;

    // Let all the cells know about their new coords
    UpdateCellCoordinates();

    // We scale the font size up or down depending on the cell size
    if (m_CurrentFont.hFont)
    {
        // Delete the existing font
        DeleteObject(m_CurrentFont.hFont);

        HDC hdc;
        hdc = GetDC(m_hwnd);
        if (hdc)
        {
            // Update the font size with respect to the cell size
            m_CurrentFont.Font.lfHeight = (m_CellSize.cy - 5);
            m_CurrentFont.hFont = CreateFontIndirectW(&m_CurrentFont.Font);
            ReleaseDC(m_hwnd, hdc);
        }
    }

    // Redraw the whole grid
    InvalidateRect(m_hwnd, &ClientRect, TRUE);

    return 0;
}

VOID
CGridView::OnVScroll(_In_ INT Value,
                     _In_ INT Pos)
{
    INT PrevScrollPosition = m_ScrollPosition;

    switch (Value)
    {
    case SB_LINEUP:
        m_ScrollPosition -= 1;
        break;

    case SB_LINEDOWN:
        m_ScrollPosition += 1;
        break;

    case SB_PAGEUP:
        m_ScrollPosition -= m_yNumCells;
        break;

    case SB_PAGEDOWN:
        m_ScrollPosition += m_yNumCells;
        break;

    case SB_THUMBTRACK:
        m_ScrollPosition = Pos;
        break;

    default:
        break;
    }

    // Make sure we don't scroll past row 0 or max rows
    m_ScrollPosition = max(0, m_ScrollPosition);
    m_ScrollPosition = min(m_ScrollPosition, m_NumRows);

    // Check if there's a difference from the previous position
    INT ScrollDiff;
    ScrollDiff = PrevScrollPosition - m_ScrollPosition;
    if (ScrollDiff)
    {
        // Set the new scrollbar position in the scroll box
        SetScrollPos(m_hwnd,
                     SB_VERT,
                     m_ScrollPosition,
                     TRUE);

        // Check if the scrollbar has moved more than the
        // number of visible rows (draged or paged)
        if (abs(ScrollDiff) < m_yNumCells)
        {
            RECT rect;
            GetClientRect(m_hwnd, &rect);

            // Scroll the visible cells which remain within the grid
            // and invalidate any new ones which appear from the top / bottom
            ScrollWindowEx(m_hwnd,
                           0,
                           ScrollDiff * m_CellSize.cy,
                           &rect,
                           &rect,
                           NULL,
                           NULL,
                           SW_INVALIDATE);
        }
        else
        {
            // All the cells need to be redrawn
            InvalidateRect(m_hwnd,
                           NULL,
                           TRUE);
        }
    }
}

LRESULT
CGridView::OnPaint(
    _In_opt_ HDC hdc
    )
{
    PAINTSTRUCT PaintStruct = { 0 };
    HDC LocalHdc = NULL;
    BOOL bSuccess = FALSE;

    // Check if we were passed a DC
    if (hdc == NULL)
    {
        // We weren't, let's get one
        LocalHdc = BeginPaint(m_hwnd, &PaintStruct);
        if (LocalHdc) bSuccess = TRUE;
    }
    else
    {
        // Use the existing DC and just get the region to paint
        bSuccess = GetUpdateRect(m_hwnd,
                                 &PaintStruct.rcPaint,
                                 TRUE);
        if (bSuccess)
        {
            // Update the struct with the DC we were passed
            PaintStruct.hdc = (HDC)hdc;
        }
    }

    // Make sure we have a valid DC
    if (bSuccess)
    {
        // Paint the grid and chars
        DrawGrid(&PaintStruct);

        if (LocalHdc)
        {
            EndPaint(m_hwnd, &PaintStruct);
        }
    }

    return 0;
}

LRESULT
CALLBACK
CGridView::MapWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CGridView *This;
    LRESULT RetCode = 0;

    // Get the object pointer from window context
    This = (CGridView *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (This == NULL)
    {
        // Check that this isn't a create message
        if (uMsg != WM_CREATE)
        {
            // Don't handle null info pointer
            goto HandleDefaultMessage;
        }
    }

    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Get the object pointer from the create param
        This = (CGridView *)((LPCREATESTRUCT)lParam)->lpCreateParams;

        // Store the pointer in the window's global user data
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);

        This->OnCreate(hwnd, ((LPCREATESTRUCTW)lParam)->hwndParent);
        break;
    }

    case WM_SIZE:
    {
        INT Width, Height;
        Width = LOWORD(lParam);
        Height = HIWORD(lParam);

        This->OnSize(Width, Height);
        break;
    }

    case WM_VSCROLL:
    {
        INT Value, Pos;
        Value = LOWORD(wParam);
        Pos = HIWORD(wParam);

        This->OnVScroll(Value, Pos);
        break;
    }

    case WM_PAINT:
    {
        This->OnPaint((HDC)wParam);
        break;
    }

    case WM_DESTROY:
    {
        This->DeleteCells();
        break;
    }

    default:
    {
HandleDefaultMessage:
        RetCode = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        break;
    }
    }

    return RetCode;
}


void
CGridView::DrawGrid(
    _In_ LPPAINTSTRUCT PaintStruct
    )
{
    // Calculate which glyph to start at based on scroll position
    int i;
    i = m_xNumCells * m_ScrollPosition;

    // Make sure we have the correct font on the DC
    HFONT hOldFont;
    hOldFont = (HFONT)SelectFont(PaintStruct->hdc,
                                 m_CurrentFont.hFont);

    // Traverse all the cells
    for (int y = 0; y < m_yNumCells; y++)
    for (int x = 0; x < m_xNumCells; x++)
    {
        // Update the glyph for this cell
        WCHAR ch = (WCHAR)m_CurrentFont.ValidGlyphs[i];
        m_Cells[y][x]->SetChar(ch);

        // Tell it to paint itself
        m_Cells[y][x]->OnPaint(*PaintStruct);
        i++;
    }

    SelectObject(PaintStruct->hdc, hOldFont);

}

void
CGridView::DeleteCells()
{
    if (m_Cells == nullptr)
        return;

    // Free cells withing the 2d array
    for (int i = 0; i < m_yNumCells; i++)
        delete[] m_Cells[i];
    delete[] m_Cells;

    m_Cells = nullptr;
}

void
CGridView::SetCellFocus(
    _In_ CCell* NewActiveCell
    )
{
    if (m_ActiveCell)
    {
        // Remove focus from any existing cell
        m_ActiveCell->SetFocus(false);
        InvalidateRect(m_hwnd, m_ActiveCell->GetCellCoordinates(), TRUE);
    }

    // Set the new active cell and give it focus
    m_ActiveCell = NewActiveCell;
    m_ActiveCell->SetFocus(true);
}
