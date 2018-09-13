
/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// CLIPDSP.H - ClipBook viewer display routines adapted from 3.1 clipboard
// viewer.

// 11-91 clausgi created

// added for winball - clausgi
extern UINT cf_link;
extern UINT cf_objectlink;
// end additions

/* Header text string ids */
#define IDS_ERROR       202      /* as string ids.  Be sure to keep these    */
#define IDS_BINARY      203      /* different.                               */
#define IDS_CLEAR           204
#define IDS_CANTDISPLAY 207      /* "Can't display data in this format" */
#define IDS_NOTRENDERED 208      /* "Application Couldn't render data"  */
#define IDS_ALREADYOPEN 209      /* OpenClipboard() fails */
#define IDS_MEMERROR    210
// clausgi addition.. planes/bitsperpixel don't match...
#define IDS_BADBMPFMT   211
#define IDS_CLEARTITLE  212
#define IDS_CONFIRMCLEAR 213

/* Other constants */
#define CDEFFMTS        8       /* Count of predifined clipboard formats    */
#define VPOSLAST        100     /* Highest vert scroll bar value */
#define HPOSLAST        100     /* Highest horiz scroll bar value */
#define CCHFMTNAMEMAX   79      /* Longest clipboard data fmt name, including
                                   terminator */
#define cLineAlwaysShow 3       /* # of "standard text height" lines to show
                                   when maximally scrolled down */
#define BUFFERLEN       160      /* String buffer length */
#define SMALLBUFFERLEN  90

#define CBMENU          1       /* Number for the Clipboard main menu  */

#define         CBM_AUTO        400

/*  Last parameter to SetDIBits() and GetDIBits() calls */

#define  DIB_RGB_COLORS   0
#define  DIB_PAL_COLORS   1

#define  IDCLEAR        IDOK

BOOL PASCAL ClearClipboard(HWND);
void PASCAL GetClipboardName(int fmt, LPTSTR szName, int iSize);

/* Far low mem situations. */
void FAR PASCAL MemErrorMessage(void);

extern BOOL fOwnerDisplay;
extern HMENU    hDispMenu;

extern void PASCAL ClipbrdVScroll(HWND  hwnd,WORD wParam,WORD   wThumb);
extern void PASCAL ClipbrdHScroll(HWND  hwnd,WORD wParam,WORD   wThumb);
extern void PASCAL DrawStuff(HWND       hwnd,register PAINTSTRUCT *pps, HWND hwndMDI );
extern void PASCAL ChangeCharDimensions(HWND, UINT, UINT);
extern void PASCAL SetCharDimensions(HWND, HFONT);
extern void PASCAL SendOwnerMessage(UINT message,WPARAM wParam,LPARAM lParam);
extern UINT PASCAL GetBestFormat( HWND hwnd, UINT wFormat);
extern void PASCAL InitOwnerScrollInfo( VOID );
extern void PASCAL RestoreOwnerScrollInfo(register HWND hwnd);
extern void PASCAL SaveOwnerScrollInfo(register HWND hwnd);
extern void PASCAL UpdateCBMenu(HWND hwnd, HWND hwndMDI );
extern BOOL PASCAL ClearClipboard(register HWND hwnd);
