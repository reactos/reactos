#pragma once

class CRichEdit :
    public CWindow
{
public:
    VOID SetRangeFormatting(LONG Start, LONG End, DWORD dwEffects)
    {
        CHARFORMAT2 CharFormat;

        SendMessageW(EM_SETSEL, Start, End);

        ZeroMemory(&CharFormat, sizeof(CharFormat));

        CharFormat.cbSize = sizeof(CharFormat);
        CharFormat.dwMask = dwEffects;
        CharFormat.dwEffects = dwEffects;

        SendMessageW(EM_SETCHARFORMAT, SCF_WORD | SCF_SELECTION, (LPARAM) &CharFormat);

        SendMessageW(EM_SETSEL, End, End + 1);
    }

    LONG GetTextLen(VOID)
    {
        GETTEXTLENGTHEX TxtLenStruct;

        TxtLenStruct.flags = GTL_NUMCHARS;
        TxtLenStruct.codepage = 1200;

        return (LONG) SendMessageW(EM_GETTEXTLENGTHEX, (WPARAM) &TxtLenStruct, 0);
    }

    /*
    * Insert text (without cleaning old text)
    * Supported effects:
    *   - CFM_BOLD
    *   - CFM_ITALIC
    *   - CFM_UNDERLINE
    *   - CFM_LINK
    */
    VOID InsertText(LPCWSTR lpszText, DWORD dwEffects)
    {
        SETTEXTEX SetText;
        LONG Len = GetTextLen();

        /* Insert new text */
        SetText.flags = ST_SELECTION;
        SetText.codepage = 1200;

        SendMessageW(EM_SETTEXTEX, (WPARAM) &SetText, (LPARAM) lpszText);

        SetRangeFormatting(Len, Len + wcslen(lpszText),
            (dwEffects == CFM_LINK) ? (PathIsURLW(lpszText) ? dwEffects : 0) : dwEffects);
    }

    /*
    * Clear old text and add new
    */
    VOID SetText(LPCWSTR lpszText, DWORD dwEffects)
    {
        SetWindowTextW(L"");
        InsertText(lpszText, dwEffects);
    }

    HWND Create(HWND hwndParent)
    {
        // TODO: FreeLibrary when the window is destroyed
        LoadLibraryW(L"riched20.dll");

        m_hWnd = CreateWindowExW(0,
            L"RichEdit20W",
            NULL,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE |
            ES_LEFT | ES_READONLY,
            205, 28, 465, 100,
            hwndParent,
            NULL,
            _AtlBaseModule.GetModuleInstance(),
            NULL);

        if (m_hWnd)
        {
            SendMessageW(EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
            SendMessageW(WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
            SendMessageW(EM_SETEVENTMASK, 0, ENM_LINK | ENM_MOUSEEVENTS);
            SendMessageW(EM_SHOWSCROLLBAR, SB_VERT, TRUE);
        }

        return m_hWnd;
    }

public:
    virtual VOID OnLink(ENLINK *Link)
    {
    }

};
