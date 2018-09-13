/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    

Abstract:

    

Author:

    Carlos Klapp (a-caklap) 15-Sep-1997

Environment:

    Win32, User Mode

--*/


#if defined( NEW_WINDOWING_CODE )


//
// Data common to all window structures
//
typedef class _COMMONWIN_DATA {
public:
    static LPCSTR       lpcszName_CommonWinData;
    DWORD               m_dwSize;
    LPCSTR              m_lpcszName;
    WIN_TYPES           m_enumType;
    HFONT               m_hfont; //Current font

    _COMMONWIN_DATA()
    {
        m_dwSize = sizeof(*this);
        m_lpcszName = lpcszName_CommonWinData;
        m_enumType = MINVAL_WINDOW;
    }

    virtual
    void
    Validate()
    {
        Assert(sizeof(*this) == m_dwSize);
        Assert(!strcmp(lpcszName_CommonWinData, m_lpcszName));
        Assert(MINVAL_WINDOW < m_enumType);
        Assert(m_enumType < MAXVAL_WINDOW);
    }

    virtual 
    void 
    SetFont(
        HFONT hfont
        )
    {
        if (m_hfont) {
            DeleteObject(m_hfont);
        }
        m_hfont = hfont;
    }

    virtual 
    BOOL
    CanCopy()
    {
        return FALSE;
    }

    virtual 
    BOOL
    CanCut()
    {
        return FALSE;
    }

    virtual 
    BOOL
    CanPaste()
    {
        return FALSE;
    }


} COMMONWIN_DATA, * PCOMMONWIN_DATA;


//
// Data used by the command window
// 
typedef class _CMDWIN_DATA : COMMONWIN_DATA {
public:
    //
    // Name of data structure
    //
    static LPCSTR       lpcszName_CmdWinData;

    //
    // Used for debugging
    //
    DWORD               m_dwSize;
    LPCSTR              m_lpcszName;

    //
    // Used to resize the divided windows.
    //
    BOOL                bTrackingMouse;
    int                 nDividerPosition;

    //
    // Handle to the two main cmd windows.
    //
    HWND                hwndHistory;
    HWND                hwndEdit;
    BOOL                bHistoryActive;

    _CMDWIN_DATA()
    {
        m_enumType = CMD_WINDOW;
        m_dwSize = sizeof(*this);
        m_lpcszName = lpcszName_CmdWinData;
        bTrackingMouse = FALSE;
        nDividerPosition = 0;
        hwndHistory = NULL;
        hwndEdit = NULL;
        bHistoryActive = FALSE;
    }

    virtual
    void
    Validate()
    {
        _COMMONWIN_DATA::Validate();

        Assert(sizeof(*this) == m_dwSize);
        Assert(!strcmp(lpcszName_CmdWinData, m_lpcszName));

        Assert(CMD_WINDOW == m_enumType);

        Assert(hwndHistory);
        Assert(hwndEdit);
    }

    virtual
    void
    SetFont(
        HFONT hfont
        )
    {
        COMMONWIN_DATA::SetFont(hfont);

        SendMessage( hwndHistory, WM_SETFONT, (WPARAM)m_hfont, (LPARAM)FALSE );
        SendMessage( hwndEdit, WM_SETFONT, (WPARAM)m_hfont, (LPARAM)FALSE );
    }

} CMDWIN_DATA, * PCMDWIN_DATA;


//
// Data used by the command window
// 
typedef class _CALLSWIN_DATA : COMMONWIN_DATA {
public:
    //
    // Name of data structure
    //
    static LPCSTR       lpcszName_ListWinData;

    //
    // Used for debugging
    //
    DWORD               m_dwSize;
    LPCSTR              m_lpcszName;

    //
    // Handle to the two main cmd windows.
    //
    HWND                hwndList;


    _CALLSWIN_DATA()
    {
        //m_enumType = CALSS_WINDOW;
        m_dwSize = sizeof(*this);
        m_lpcszName = lpcszName_ListWinData;
        hwndList = 0;
    }

    virtual
    void
    Validate()
    {
        _COMMONWIN_DATA::Validate();

        Assert(sizeof(*this) == m_dwSize);
        Assert(!strcmp(lpcszName_ListWinData, m_lpcszName));

        //Assert(CALLS_WINDOW == m_enumType);

        Assert(hwndList);
    }

    virtual
    void
    SetFont(
        HFONT hfont
        )
    {
        COMMONWIN_DATA::SetFont(hfont);

        SendMessage( hwndList, WM_SETFONT, (WPARAM)m_hfont, (LPARAM)FALSE );
    }

} CALLSWIN_DATA, * PCALLSWIN_DATA;



PCOMMONWIN_DATA
GetCommonWinData(
    HWND hwnd
    );

PCOMMONWIN_DATA
SetCommonWinData(
    HWND hwnd, 
    PCOMMONWIN_DATA pWinData_New
    );

PCMDWIN_DATA
GetCmdWinData(
    HWND hwnd
    );

PCMDWIN_DATA
SetCmdWinData(
    HWND hwnd, 
    PCMDWIN_DATA pCmdWinData_New
    );

PCALLSWIN_DATA
GetCallsWinData(
    HWND hwnd
    );

PCALLSWIN_DATA
SetCallsWinData(
    HWND hwnd, 
    PCALLSWIN_DATA pCallsWinData_New
    );

HWND NewCmd_CreateWindow(HWND hwndParent);
LRESULT CALLBACK NewCmd_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif //NEW_WINDOWING_CODE 