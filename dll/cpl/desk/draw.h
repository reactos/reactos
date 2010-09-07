BOOL
MyDrawFrameControl(HDC hDC, LPRECT rc, UINT uType, UINT uState, THEME *theme);
BOOL
MyDrawEdge(HDC hDC, LPRECT rc, UINT edge, UINT flags, THEME *theme);
VOID
MyDrawCaptionButtons(HDC hdc, LPRECT lpRect, BOOL bMinMax, int x, THEME *theme);
VOID
MyDrawScrollbar(HDC hdc, LPRECT rc, HBRUSH hbrScrollbar, THEME *theme);
BOOL
MyDrawCaptionTemp(HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont, HICON hIcon, LPCWSTR str, UINT uFlags, THEME *theme);
DWORD
MyDrawMenuBarTemp(HWND Wnd, HDC DC, LPRECT Rect, HMENU Menu, HFONT Font, THEME *theme);
