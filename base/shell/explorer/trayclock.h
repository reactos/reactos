#pragma once

const struct
{
    BOOL IsTime;
    DWORD dwFormatFlags;
    LPCWSTR lpFormat;
} ClockWndFormats[] = {
    { TRUE, 0, NULL },
    { FALSE, 0, L"dddd" },
    { FALSE, DATE_SHORTDATE, NULL }
};
const UINT ClockWndFormatsCount = _ARRAYSIZE(ClockWndFormats);

#define CLOCKWND_FORMAT_COUNT ClockWndFormatsCount

extern const WCHAR szTrayClockWndClass[];
class CTrayClockWnd :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayClockWnd, CWindow, CControlWinTraits >
{
    HWND hWndNotify;
    HFONT hFont;
    COLORREF textColor;
    RECT rcText;
    SYSTEMTIME LocalTime;

    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD IsTimerEnabled : 1;
            DWORD IsInitTimerEnabled : 1;
            DWORD LinesMeasured : 1;
            DWORD IsHorizontal : 1;
        };
    };
    DWORD LineSpacing;
    SIZE CurrentSize;
    WORD VisibleLines;
    SIZE LineSizes[CLOCKWND_FORMAT_COUNT];
    WCHAR szLines[CLOCKWND_FORMAT_COUNT][48];

public:
    CTrayClockWnd();
    virtual ~CTrayClockWnd();

private:
    LRESULT OnThemeChanged();
    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL MeasureLines();
    WORD GetMinimumSize(IN BOOL Horizontal, IN OUT PSIZE pSize);
    VOID UpdateWnd();
    VOID Update();
    UINT CalculateDueTime();
    BOOL ResetTime();
    VOID CalibrateTimer();
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    VOID SetFont(IN HFONT hNewFont, IN BOOL bRedraw);
    LRESULT DrawBackground(HDC hdc);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNcLButtonDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:
    DECLARE_WND_CLASS_EX(szTrayClockWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayClockWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
        MESSAGE_HANDLER(TCWM_GETMINIMUMSIZE, OnGetMinimumSize)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
        MESSAGE_HANDLER(WM_NCLBUTTONDBLCLK, OnNcLButtonDblClick)
    END_MSG_MAP()

    HWND _Init(IN HWND hWndParent);
};