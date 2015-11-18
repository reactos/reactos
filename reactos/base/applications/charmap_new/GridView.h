#pragma once
#include "Cell.h"

//typedef struct _CELL
//{
//    RECT CellExt;
//    RECT CellInt;
//    BOOL bActive;
//    BOOL bLarge;
//    WCHAR ch;
//
//} CELL, *PCELL;

#define MAX_GLYPHS 0xFFFF


class CGridView
{
private:
    CAtlStringW m_szMapWndClass;

    HWND m_hwnd;
    HWND m_hParent;

    int m_xNumCells;
    int m_yNumCells;

    RECT m_ClientCoordinates;
    //SIZE ClientSize;
    SIZE m_CellSize;
    CCell*** m_Cells; // m_Cells[][];
    CCell *m_ActiveCell;

    HFONT hFont;
    LOGFONTW CurrentFont;
    INT iYStart;

    USHORT ValidGlyphs[MAX_GLYPHS];
    USHORT NumValidGlyphs;

public:
    CGridView();
    ~CGridView();

    bool Create(
        _In_ HWND hParent
        );

private:
    static LRESULT
        CALLBACK
        MapWndProc(HWND hwnd,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam);

    LRESULT OnCreate(
        _In_ HWND hwnd,
        _In_ HWND hParent
        );


    LRESULT OnSize(
        _In_ INT Width,
        _In_ INT Height
        );

    LRESULT OnPaint(
        _In_opt_ HDC hdc
        );

    bool UpdateGridLayout(
        );

    void DrawGrid(
        _In_ LPPAINTSTRUCT PaintStruct
        );

    void DeleteCells();

    void SetCellFocus(
        _In_ CCell* NewActiveCell
        );
};

