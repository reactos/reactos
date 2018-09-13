// common stuff for the toolbar control

typedef struct {		/* info for recreating the bitmaps */
    int nButtons;
    HINSTANCE hInst;
    UINT wID;
    HBITMAP hbm;
} TBBMINFO, NEAR *PTBBMINFO;

typedef struct {		/* instance data for toolbar window */
    HWND hwnd;
    DWORD style;
    PTBBUTTON pCaptureButton;
    HWND hwndToolTips;
    HWND hdlgCust;
    HWND hwndCommand;
    HFONT hfontIcon;
    int nBitmaps;
    int nSelectedBM;		// currently selected pBitmaps index
    PTBBMINFO pBitmaps;
    HBITMAP hbmCache;
    PSTR *pStrings;
    int nStrings;
    UINT uStructSize;
    int iDxBitmap;
    int iDyBitmap;
    int iButWidth;
    int iButHeight;
    int iYPos;
    int iNumButtons;
    int dyIconFont;
    TBBUTTON Buttons[1];	// BUGBUG: make this a ptr to avoid lots
				// of bogus code that has to deal with reallocing
				// the ptb data
} TBSTATE, NEAR *PTBSTATE;

typedef struct {
/*REVIEW: index, command, flag words, resource ids should be UINT */
    int iBitmap;	/* index into bitmap of this button's picture */
    int idCommand;	/* WM_COMMAND menu ID that this button sends */
    BYTE fsState;	/* button's state */
    BYTE fsStyle;	/* button's style */
    int idsHelp;	/* string ID for button's status bar help */
} OLDTBBUTTON, FAR* LPOLDTBBUTTON;

HBITMAP FAR PASCAL SelectBM(HDC hDC, PTBSTATE pTBState, int nButton);
void FAR PASCAL DrawButton(HDC hdc, int x, int y, int dx, int dy,
      PTBSTATE pTBState, LPTBBUTTON ptButton, BOOL bCache);
int  FAR PASCAL TBHitTest(PTBSTATE pTBState, int xPos, int yPos);
int  FAR PASCAL PositionFromID(PTBSTATE pTBState, int id);
void FAR PASCAL BuildButtonTemplates(void);
void FAR PASCAL TBInputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt);

BOOL FAR PASCAL SaveRestore(PTBSTATE pTBState, BOOL bWrite, LPSTR FAR *lpNames);
BOOL FAR PASCAL SaveRestoreFromReg(PTBSTATE ptb, BOOL bWrite, HKEY hkr, LPCSTR pszSubKey, LPCSTR pszValueName);

void FAR PASCAL CustomizeTB(PTBSTATE pTBState, int iPos);
void FAR PASCAL MoveButton(PTBSTATE pTBState, int nSource);
BOOL FAR PASCAL DeleteButton(PTBSTATE ptb, UINT uIndex);
BOOL FAR PASCAL InsertButtons(PTBSTATE ptb, UINT uWhere, UINT uButtons, LPTBBUTTON lpButtons);

BOOL FAR PASCAL SendItemNotify(PTBSTATE ptb, int iItem, int code);
void FAR PASCAL FlushToolTipsMgr(PTBSTATE ptb);

