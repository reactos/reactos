//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       paddebug.cxx
//
//  Contents:   Implements debug window.
//
//-------------------------------------------------------------------------

#pragma warning(disable: 4244)

class CPadDoc;

MtExtern(CDebugWindow)

//+------------------------------------------------------------------------
//
//  Class:   CDebugWindow
//
//-------------------------------------------------------------------------
class CDebugWindow
{

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDebugWindow))

    CDebugWindow(CPadDoc* pDoc);
    ~CDebugWindow();

    // Sub-object management

    ULONG SubAddRef();
    ULONG SubRelease();

    // Initialization

    HRESULT RegisterDebugWndClass();
    HRESULT Init();
    void Destroy();
        

    // Window procedure

    LRESULT OnCommand(WORD wNotifyCode, WORD idm, HWND hwndCtl);
    LRESULT OnSize(WORD fwSizeType, WORD nWidth, WORD nHeight);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
    LRESULT DebugWindowOnClose();
    static LRESULT CALLBACK DebugTextWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
    LRESULT ExecuteScript ();
    
    // Other
    
    HRESULT Print (BSTR PrintValue);
    HRESULT NextLine ();

    // Member variables

    CPadDoc *                   _pDoc;

    HWND                        _hwnd;
    HWND                        _hwndText;

    ULONG                       _ulRefs;
    ULONG                       _ulAllRefs;
    WNDPROC                     _wpOrigEditProc; 
    
    // Static members

    static BOOL                 s_fDebugWndClassRegistered;
};
