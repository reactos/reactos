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
    m_yNumCells(10)
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
CGridView::UpdateGridLayout(
    )
{
    // Go through all the cells and calculate
    // their coordinates within the grid 
    for (int y = 0; y < m_yNumCells; y++)
    for (int x = 0; x < m_xNumCells; x++)
    {
        RECT CellCoordinates;
        CellCoordinates.left = x * m_CellSize.cx + 1;
        CellCoordinates.top = y * m_CellSize.cy + 1;
        CellCoordinates.right = (x + 1) * m_CellSize.cx + 2;
        CellCoordinates.bottom = (y + 1) * m_CellSize.cy + 2;

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

    SetWindowPos(m_hwnd,
                 NULL,
                 m_ClientCoordinates.left,
                 m_ClientCoordinates.top,
                 m_ClientCoordinates.right,
                 m_ClientCoordinates.bottom,
                 SWP_NOZORDER | SWP_SHOWWINDOW);

    // Get the client area we can draw on. The position we set above
    // includes a scrollbar. GetClientRect gives us the size without
    // the scroll, and it more efficient than getting the scroll
    // metrics and calculating the size
    RECT ClientRect;
    GetClientRect(m_hwnd, &ClientRect);
    m_CellSize.cx = ClientRect.right / m_xNumCells;
    m_CellSize.cy = ClientRect.bottom / m_yNumCells;

    UpdateGridLayout();

    return 0;
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

    if (bSuccess)
    {
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
    // Traverse all the cells and tell them to paint themselves
    for (int y = 0; y < m_yNumCells; y++)
    for (int x = 0; x < m_xNumCells; x++)
    {
        m_Cells[y][x]->OnPaint(*PaintStruct);
    }
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
    }

    // Set the new active cell and give it focus
    m_ActiveCell = NewActiveCell;
    m_ActiveCell->SetFocus(true);
}