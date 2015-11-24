#pragma once
#include "Cell.h"

#define MAX_GLYPHS 0xFFFF

struct CurrentFont
{
    CAtlStringW FontName;
    LOGFONTW Font;
    HFONT hFont;
    USHORT ValidGlyphs[MAX_GLYPHS];
    USHORT NumValidGlyphs;
};


class CGridView
{
private:
    CAtlStringW m_szMapWndClass;

    HWND m_hwnd;
    HWND m_hParent;

    int m_xNumCells;
    int m_yNumCells;

    RECT m_ClientCoordinates;
    SIZE m_CellSize;
    CCell*** m_Cells; // *m_Cells[][];
    CCell *m_ActiveCell;

    HFONT hFont;
    INT ScrollPosition;

    CurrentFont m_CurrentFont;

public:
    CGridView();
    ~CGridView();

    bool Create(
        _In_ HWND hParent
        );

    bool SetFont(
        _In_ CAtlString& FontName
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

    VOID OnVScroll(
        _In_ INT Value,
        _In_ INT Pos
        );

    LRESULT OnPaint(
        _In_opt_ HDC hdc
        );

    bool UpdateCellCoordinates(
        );

    void DrawGrid(
        _In_ LPPAINTSTRUCT PaintStruct
        );

    void DeleteCells();

    void SetCellFocus(
        _In_ CCell* NewActiveCell
        );
};

