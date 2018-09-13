//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      view.h
//
//  Contents:  CPropView view object class definition
//
//  History:   22-Jan-97 EricB      Created.
//
//-----------------------------------------------------------------------------

#ifndef __PROP_VIEW__
#define __PROP_VIEW__

class CHtmlPropPage;
class CSite;
 
typedef CSite * LPSITE;

class CPropView : public IOleInPlaceFrame, public IOleCommandTarget
{
friend CHtmlPropPage;

public:

#ifdef _DEBUG
    char szClass[16];
#endif

    // Construtor/Destructor
    CPropView();
    ~CPropView();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IOleWindow methods    
    STDMETHODIMP GetWindow(HWND * phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL bExtend);

    // IOleInPlaceUIWindow Methods
    STDMETHODIMP GetBorder(LPRECT);
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS);
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS);
    STDMETHODIMP SetActiveObject(LPOLEINPLACEACTIVEOBJECT, LPCOLESTR);

    // IOleInPlaceFrame Methods
    STDMETHODIMP InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS);
    STDMETHODIMP SetMenu(HMENU, HOLEMENU, HWND);
    STDMETHODIMP RemoveMenus(HMENU);
    STDMETHODIMP SetStatusText(LPCOLESTR);
    STDMETHODIMP EnableModeless(BOOL);
    STDMETHODIMP TranslateAccelerator(LPMSG, WORD);

    //IOleCommandTarget
    STDMETHODIMP QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds,
                             OLECMD prgCmds[], OLECMDTEXT * pCmdText);

    STDMETHODIMP Exec(const GUID * pguidCmdGroup, DWORD nCmdID,
                      DWORD nCmdexecopt, VARIANTARG * pvaIn,
                      VARIANTARG * pvaOut);

    //	Helper functions
    //STDMETHODIMP RestoreContainerMenu();

    //
    //  Static WndProc to be passed to CreateWindow
    //

    static long CALLBACK StaticWndProc(HWND hWnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);

    //
    //  Instance specific wind proc
    //

    LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    //
    // Window/object creation member.
    //

    static HRESULT Create(HWND hWndOwner, HINSTANCE hInstance,
                          LPCTSTR pszUrl, CPropView ** ppObj);

    HWND GetWnd() {return m_hWnd;};

protected:
    void        OnApply(void);

private:
    ULONG       m_cRef;
    HWND        m_hWnd;
    LPSITE      m_pSite;    // Site holding object        
    BOOL        m_fCreated;
    LPCTSTR     m_pszUrl;
    HWND        m_hWndObj;  // The object's window
    HINSTANCE   m_hInst;
    RECT        m_rect;

    IOleInPlaceActiveObject   * m_pIOleIPActiveObject;

    BOOL    CreateDocObject(LPCTSTR pchPath);
    DWORD   GetCommandStatus(ULONG);
    void    ExecCommand(ULONG);
    int     OnCreate(HWND hWnd);
    LRESULT OnDoInit(void);
    void    OnDraw(void);
    void    OnPaint(void);
    void    OnSize(void);
    void    OnSetFocus(void);
    void    OnDestroy(void);
};

typedef CPropView * LPPROPVIEW;

#endif
