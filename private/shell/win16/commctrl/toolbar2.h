// common stuff for the toolbar control

#ifndef _TOOLBAR_H
#define _TOOLBAR_H

#define TBHIGHLIGHT_BACK
#define TBHIGHLIGHT_GLYPH

DECLARE_HANDLE(HTBDROPTARGET);


typedef struct {		/* info for recreating the bitmaps */
    int nButtons;
    HINSTANCE hInst;
    UINT wID;
} TBBMINFO, NEAR *PTBBMINFO;

typedef struct {		/* instance data for toolbar window */
    CONTROLINFO ci;

    HDC hdcMono;
    HBITMAP hbmMono;
    PTBBUTTON pCaptureButton;
    HWND hwndToolTips;
    HWND hdlgCust;
    HFONT hfontIcon;
    int nBitmaps;
#ifdef GLYPHCACHE
    int nSelectedBM;		// currently selected pBitmaps index
#endif
    PTBBMINFO pBitmaps;
#ifdef FACECACHE
    HBITMAP hbmCache;
#endif
    PTSTR *pStrings;
    int nStrings;
    int nTextRows;              // # Rows of text per button
    UINT uStructSize;
    int iDxBitmap;
    int iDyBitmap;
    int iButWidth;
    int iButHeight;
    int iButMinWidth;           // The min and max width of the button. If the app does not
    int iButMaxWidth;           // have an opinion on what the min and max should be, these will be 0
    int iYPos;
    int iNumButtons;
    int dyIconFont;
    int xFirstButton;
    BOOL fHimlValid : 1;
    BOOL fHimlNative : 1;
                                    // Stuff for drag and drop
    HTBDROPTARGET htbdroptarget;    // Drop target if TBSTYLE_DROPPABLE

    HIMAGELIST himl;
    HIMAGELIST himlHot;         // Image list for the hot-tracked image
    HIMAGELIST himlDisabled;    // Image list for the hot-tracked image
    int nCurHTButton;           // Index of the currently Hot Tracked Button
    BOOL fMouseTrack;           // Are we currently tracking Mouse over this toolbar ?
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


#ifdef __cplusplus
extern "C" {
#endif

HTBDROPTARGET CreateTBDropTarget(PTBSTATE ptb);
void DestroyTBDropTarget(HTBDROPTARGET htbdtgt);
void ReattachTBDropTarget (HTBDROPTARGET htbdtgt, PTBSTATE ptb);

HBITMAP FAR PASCAL SelectBM(HDC hDC, PTBSTATE pTBState, int nButton);
void FAR PASCAL DrawButton(HDC hdc, int x, int y, PTBSTATE pTBState, LPTBBUTTON ptButton);
void NEAR PASCAL DrawFace(PTBSTATE ptb, LPTBBUTTON ptButton, HDC hdc, int x, int y,
                            int offx, int offy, int dx, int dy, UINT state);
int  FAR PASCAL TBHitTest(PTBSTATE pTBState, int xPos, int yPos);
int  FAR PASCAL PositionFromID(PTBSTATE pTBState, int id);
void FAR PASCAL BuildButtonTemplates(void);
void FAR PASCAL TBInputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt);

BOOL FAR PASCAL SaveRestore(PTBSTATE pTBState, BOOL bWrite, LPTSTR FAR *lpNames);
BOOL FAR PASCAL SaveRestoreFromReg(PTBSTATE ptb, BOOL bWrite, HKEY hkr, LPCTSTR pszSubKey, LPCTSTR pszValueName);

void FAR PASCAL CustomizeTB(PTBSTATE pTBState, int iPos);
void FAR PASCAL MoveButton(PTBSTATE pTBState, int nSource);
BOOL FAR PASCAL DeleteButton(PTBSTATE ptb, UINT uIndex);
BOOL FAR PASCAL InsertButtons(PTBSTATE ptb, UINT uWhere, UINT uButtons, LPTBBUTTON lpButtons);

BOOL FAR PASCAL SendItemNotify(PTBSTATE ptb, int iItem, int code);
void FAR PASCAL FlushToolTipsMgr(PTBSTATE ptb);
void PASCAL ReleaseMonoDC(PTBSTATE ptb);

extern const int g_dxButtonSep;

#ifdef __cplusplus
}
#endif


#endif _TOOLBAR_H
