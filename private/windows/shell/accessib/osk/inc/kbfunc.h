
/****************************************************************************/
/* FUNCTIONS IN THIS FILE */
/****************************************************************************/
BOOL InitProc(void);
BOOL RegisterWndClass(HINSTANCE hInst);
HWND CreateMainWindow(BOOL re_size);
void mlGetSystemParam(void);
BOOL rSetWindowPos(void);
void FinishProcess(void);
void udfDraw3D(HDC bhdc, RECT brect);
void udfDraw3Dpush(HDC bhdc, RECT brect, BOOL wcolor);      // button down

void vPrintCenter(HWND Childhwnd, HDC bhdc, RECT brect, int index, int bpush);
void vPrintCenter_Block(HWND Childhwnd, HDC bhdc, RECT brect, int index, int bpush);

BOOL ChooseNewFont(HWND hWnd);
void ChangeTextKeyColor(void);
BOOL RDrawIcon(HDC hDC, TCHAR *pIconName, int bpush, RECT rect);
BOOL RDrawBitMap(HDC hDC, TCHAR *pIconName, int bpush, RECT rect, BOOL transform);
BOOL SavePreferences(void);
BOOL OpenPreferences(void);
void DeleteChildBackground(void);
BOOL Change_Reset_DeadKeyColor(HWND Childhwnd, int index, int result);
HFONT	ReSizeFont(int index, LOGFONT *plf, int outsize);
BOOL NumLockLight(void);
void RedrawKeys(void);
void DrawIcon_KeyLight(HDC hDC, int which, RECT rect);
void SetKeyRegion(HWND hwnd, int w, int h);
void CapShift_Redraw(void);
int GetKeyText(UINT vk, UINT sc, BYTE *kbuf, TCHAR *cbuf, HKL hkl);
BOOL RedrawNumLock(void);
BOOL RedrawScrollLock(void);
void ChangeBitmapColorDC (HDC hdcBM, LPBITMAP lpBM, COLORREF rgbOld, COLORREF rgbNew);
void ChangeBitmapColor (HBITMAP hbmSrc, COLORREF rgbOld, COLORREF rgbNew, HPALETTE hPal);
