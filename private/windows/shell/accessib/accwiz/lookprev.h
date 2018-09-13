
#define CCH_MAX_STRING    256

class CLookPreviewGlobals
{
public:
	CLookPreviewGlobals()
	{
		// We don't initialize stuff here because we rely
		// on some other global variables that are not yet initialized
		m_bInitialized = FALSE;
	}
	BOOL Initialize();

	TCHAR m_szActive[CCH_MAX_STRING];
	TCHAR m_szInactive[CCH_MAX_STRING];
	TCHAR m_szMinimized[CCH_MAX_STRING];
	TCHAR m_szIconTitle[CCH_MAX_STRING];
	TCHAR m_szNormal[CCH_MAX_STRING];
	TCHAR m_szDisabled[CCH_MAX_STRING];
	TCHAR m_szSelected[CCH_MAX_STRING];
	TCHAR m_szMsgBox[CCH_MAX_STRING];
	TCHAR m_szButton[CCH_MAX_STRING];
//	TCHAR m_szSmallCaption[40];
	TCHAR m_szWindowText[CCH_MAX_STRING];
	TCHAR m_szMsgBoxText[CCH_MAX_STRING];

protected:
	static BOOL sm_bOneInstanceCreated; // This variable insures that only one instance of CLookPreviewGlobals is created
	BOOL m_bInitialized;
};

class CLookPrev
{
public:
	CLookPrev()
	{
		m_hwnd = NULL;
		m_hmenuSample = NULL;
		m_hbmLook = NULL;       // bitmap for the appearance preview
	}
	
	HWND m_hwnd;

	// Static window proc
	static LRESULT CALLBACK LookPreviewWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static CLookPreviewGlobals sm_Globals;

protected:
	HMENU m_hmenuSample;
	HBITMAP m_hbmLook;       // bitmap for the appearance preview

	void ShowBitmap(HDC hdc);
	void Draw(HDC hdc);

protected: // Message handlers
	void OnCreate();
	void OnDestroy();
	void OnRepaint();
	void OnRecalc();
	void OnPaint(HDC hdc);
};


// Messages for the Look Preview window
#define LPM_REPAINT		WM_USER + 1
#define LPM_RECALC		WM_USER + 2
