//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       acuictl.h
//
//  Contents:   UI Control class definitions
//
//  History:    12-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__ACUICTL_H__)
#define __ACUICTL_H__

//
// Forward class declaration
//

class CInvokeInfoHelper;

//
// Link subclass definitions
//

typedef struct _TUI_LINK_SUBCLASS_DATA {

    HWND    hwndParent;
    WNDPROC wpPrev;
    DWORD_PTR uToolTipText;
    DWORD   uId;
    HWND    hwndTip;
    LPVOID  pvData;
    BOOL    fMouseCaptured;

} TUI_LINK_SUBCLASS_DATA, *PTUI_LINK_SUBCLASS_DATA;

//
// IACUIControl abstract base class interface.  This is used by the
// invoke UI entry point to put up the appropriate UI.  There are different
// implementations of this interface based on the invoke reason code
//

class IACUIControl
{
public:

    //
    // Constructor
    //

    IACUIControl (CInvokeInfoHelper& riih);

    //
    // Virtual destructor
    //

    virtual ~IACUIControl ();

    //
    // UI Message processing
    //

    virtual BOOL OnUIMessage (
                     HWND   hwnd,
                     UINT   uMsg,
                     WPARAM wParam,
                     LPARAM lParam
                     );

    void LoadActionText(WCHAR **ppszRet, WCHAR *pwszIn, DWORD dwDefId);
    void SetupButtons(HWND hwnd);

    //
    // Pure virtual methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay) = 0;

    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam) = 0;

    virtual BOOL OnYes (HWND hwnd) = 0;

    virtual BOOL OnNo (HWND hwnd) = 0;

    virtual BOOL OnMore (HWND hwnd) = 0;

protected:

    //
    // Invoke Info Helper reference
    //

    CInvokeInfoHelper& m_riih;

    //
    // Invoke result
    //

    HRESULT            m_hrInvokeResult;

    WCHAR               *m_pszCopyActionText;
    WCHAR               *m_pszCopyActionTextNoTS;
    WCHAR               *m_pszCopyActionTextNotSigned;
};

//
// CVerifiedTrustUI class is used to invoke authenticode UI where the
// trust hierarchy for the signer has been successfully verified and the
// user has to make an override decision
//

class CVerifiedTrustUI : public IACUIControl
{
public:

    //
    // Initialization
    //

    CVerifiedTrustUI (CInvokeInfoHelper& riih, HRESULT& rhr);

    ~CVerifiedTrustUI ();

    //
    // IACUIControl methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay);

    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam);

    virtual BOOL OnYes (HWND hwnd);

    virtual BOOL OnNo (HWND hwnd);

    virtual BOOL OnMore (HWND hwnd);

private:

    //
    // Formatted strings for display
    //

    LPWSTR             m_pszInstallAndRun;
    LPWSTR             m_pszAuthenticity;
    LPWSTR             m_pszCaution;
    LPWSTR             m_pszPersonalTrust;

    //
    // links
    //

    TUI_LINK_SUBCLASS_DATA m_lsdPublisher;
    TUI_LINK_SUBCLASS_DATA m_lsdOpusInfo;
    TUI_LINK_SUBCLASS_DATA m_lsdCA;
    TUI_LINK_SUBCLASS_DATA m_lsdAdvanced;
};

//
// CUnverifiedTrustUI class is used to invoke authenticode UI where the
// trust hierarchy for the signer has been NOT been successfully verified and
// the user has to make an override decision
//

class CUnverifiedTrustUI : public IACUIControl
{
public:

    //
    // Initialization
    //

    CUnverifiedTrustUI (CInvokeInfoHelper& riih, HRESULT& rhr);

    ~CUnverifiedTrustUI ();

    //
    // IACUIControl methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay);

    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam);

    virtual BOOL OnYes (HWND hwnd);

    virtual BOOL OnNo (HWND hwnd);

    virtual BOOL OnMore (HWND hwnd);

private:

    //
    // Formatted strings for display
    //

    LPWSTR              m_pszNoAuthenticity;
    LPWSTR              m_pszProblemsBelow;
    LPWSTR              m_pszInstallAndRun3;

    //
    // links
    //

    TUI_LINK_SUBCLASS_DATA m_lsdPublisher;
    TUI_LINK_SUBCLASS_DATA m_lsdOpusInfo;
    TUI_LINK_SUBCLASS_DATA m_lsdCA;
    TUI_LINK_SUBCLASS_DATA m_lsdAdvanced;
};

//
// CNoSignatureUI class is used to invoke authenticode UI where the
// there is no signature for the subject and the user has to make an
// override decision
//

class CNoSignatureUI : public IACUIControl
{
public:

    //
    // Initialization
    //

    CNoSignatureUI (CInvokeInfoHelper& riih, HRESULT& rhr);

    ~CNoSignatureUI ();

    //
    // IACUIControl methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay);

    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam);

    virtual BOOL OnYes (HWND hwnd);

    virtual BOOL OnNo (HWND hwnd);

    virtual BOOL OnMore (HWND hwnd);

private:

    //
    // Formatted strings for display
    //

    LPWSTR m_pszInstallAndRun2;
    LPWSTR m_pszNoPublisherFound;
};

//
// ACUIMessageProc, this dialog message procedure is used to dispatch
// dialog messages to the control
//

INT_PTR CALLBACK ACUIMessageProc (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  );

//
// Subclassing helper routines and definitions
//

VOID SubclassEditControlForArrowCursor (HWND hwndEdit);

LRESULT CALLBACK ACUISetArrowCursorSubclass (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  );

VOID SubclassEditControlForLink (
                 HWND                       hwndDlg,
                 HWND                       hwndEdit,
                 WNDPROC                    wndproc,
                 PTUI_LINK_SUBCLASS_DATA    plsd
                 );

LRESULT CALLBACK ACUILinkSubclass (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  );

//
// UI control resizing helper functions
//

VOID RebaseControlVertical (
                  HWND  hwndDlg,
                  HWND  hwnd,
                  HWND  hwndNext,
                  BOOL  fResizeForText,
                  int   deltavpos,
                  int   oline,
                  int   minsep,
                  int*  pdeltaheight
                  );

int CalculateControlVerticalDistanceFromDlgBottom (HWND hwnd, UINT Control);

int CalculateControlVerticalDistance (HWND hwnd, UINT Control1, UINT Control2);

VOID ACUICenterWindow (HWND hWndToCenter);

int GetEditControlMaxLineWidth (HWND hwndEdit, HDC hdc, int cline);

void DrawFocusRectangle (HWND hwnd, HDC hdc);

void AdjustEditControlWidthToLineCount(HWND hwnd, int cline, TEXTMETRIC* ptm);

//
// Miscellaneous definitions
//

#define MAX_LOADSTRING_BUFFER 1024

//
// Resource string formatting helper
//

HRESULT FormatACUIResourceString (
                  UINT   StringResourceId,
                  DWORD_PTR* aMessageArgument,
                  LPWSTR* ppszFormatted
                  );

//
// Rendering helper
//

int RenderACUIStringToEditControl (
                  HWND                      hwndDlg,
                  UINT                      ControlId,
                  UINT                      NextControlId,
                  LPCWSTR                   psz,
                  int                       deltavpos,
                  BOOL                      fLink,
                  WNDPROC                   wndproc,
                  PTUI_LINK_SUBCLASS_DATA   plsd,
                  int                       minsep,
                  LPCWSTR                   pszThisTextOnlyInLink
                  );

//
// HTML help viewing helper
//

VOID ACUIViewHTMLHelpTopic (HWND hwnd, LPSTR pszTopic);

//
// Hotkey helpers
//

int GetHotKeyCharPosition (HWND hwnd);

VOID FormatHotKeyOnEditControl (HWND hwnd, int hkcharpos);

#endif
