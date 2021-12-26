#pragma once
#include <ui/rosctrls.h>

class CRichEdit :
    public CWindow
{
    HMODULE m_LoadedLibrary;

    VOID GenericInsertText(LPCWSTR lpszText, SIZE_T InsertedTextLen, DWORD dwEffects)
    {
        SETTEXTEX SetText;
        SIZE_T Len = GetTextLen();

        if (InsertedTextLen)
        {
            /* Insert new text */
            SetText.flags = ST_SELECTION;
            SetText.codepage = 1200;

            SendMessageW(EM_SETTEXTEX, (WPARAM) &SetText, (LPARAM) lpszText);

            if ((dwEffects == CFM_LINK) && !PathIsURLW(lpszText))
            {
                // if text is not an URL, no styling is used
                SetRangeFormatting(Len, Len + InsertedTextLen, 0);
            }
            else
            {
                SetRangeFormatting(Len, Len + InsertedTextLen, dwEffects);
            }
        }
    }

public:
    CRichEdit() : CWindow(), m_LoadedLibrary(NULL) {}

    VOID SetRangeFormatting(SIZE_T Start, SIZE_T End, DWORD dwEffects)
    {
        CHARFORMAT2W CharFormat;

        SendMessageW(EM_SETSEL, Start, End);

        ZeroMemory(&CharFormat, sizeof(CharFormat));

        CharFormat.cbSize = sizeof(CharFormat);
        CharFormat.dwMask = dwEffects;
        CharFormat.dwEffects = dwEffects;

        SendMessageW(EM_SETCHARFORMAT, SCF_WORD | SCF_SELECTION, (LPARAM) &CharFormat);

        SendMessageW(EM_SETSEL, End, End + 1);
    }

    LONG GetTextLen()
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
        GenericInsertText(lpszText, wcslen(lpszText), dwEffects);
    }

    VOID InsertText(const ATL::CStringW& szText, DWORD dwEffects)
    {
        GenericInsertText(szText.GetString(), szText.GetLength(), dwEffects);
    }
    /*
    * Clear old text and add new
    */
    VOID SetText(LPCWSTR lpszText, DWORD dwEffects)
    {
        SetWindowTextW(L"");
        InsertText(lpszText, dwEffects);
    }

    VOID SetText(const ATL::CStringW& szText, DWORD dwEffects)
    {
        SetText(szText.GetString(), dwEffects);
    }

    HWND Create(HWND hwndParent)
    {
        m_LoadedLibrary = LoadLibraryW(L"riched20.dll");

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

    virtual VOID OnLink(ENLINK *Link)
    {
    }

    ~CRichEdit()
    {
        if (m_LoadedLibrary)
        {
            FreeLibrary(m_LoadedLibrary);
        }
    }

};
