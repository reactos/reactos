#pragma once
class CCell
{
private:
    HWND m_hParent;
    RECT m_CellCoordinates;

    bool m_bHasFocus;
    bool m_bIsLarge;
    WCHAR ch;

public:
    CCell(
        _In_ HWND hParent
        );

    CCell(
        _In_ HWND hParent,
        _In_ RECT& CellLocation
        );

    ~CCell();

    LPRECT GetCellCoordinates() { return &m_CellCoordinates; }
    void SetFocus(_In_ bool HasFocus) { m_bHasFocus = HasFocus; }

    bool OnPaint(
        _In_ PAINTSTRUCT &PaintStruct
        );

    void SetCellCoordinates(
        _In_ RECT& Coordinates
        );
};

