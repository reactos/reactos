/* ------------- dflat.h ----------- */
#ifndef DFLAT_H
#define DFLAT_H

//#ifdef BUILD_FULL_DFLAT
#define INCLUDE_MULTI_WINDOWS
#define INCLUDE_LOGGING
#define INCLUDE_SHELLDOS
#define INCLUDE_WINDOWOPTIONS
#define INCLUDE_PICTUREBOX
#define INCLUDE_MINIMIZE
#define INCLUDE_MAXIMIZE
#define INCLUDE_RESTORE
#define INCLUDE_EXTENDEDSELECTIONS
//#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <process.h>
#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <setjmp.h>

#ifndef DF_VERSION
#define DF_VERSION "Beta Version 0.3"
#endif

void *DfCalloc(size_t, size_t);
void *DfMalloc(size_t);
void *DfRealloc(void *, size_t);


#define DF_MAXMESSAGES 50
#define DF_DELAYTICKS 1
#define DF_FIRSTDELAY 7
#define DF_DOUBLETICKS 5

#define DF_MAXTEXTLEN 65000U /* maximum text buffer            */
#define DF_EDITLEN     1024  /* starting length for multiliner */
#define DF_ENTRYLEN     256  /* starting length for one-liner  */
#define DF_GROWLENGTH    64  /* buffers grow by this much      */

#include "system.h"
#include "config.h"
#include "rect.h"
#include "menu.h"
#include "keys.h"
#include "commands.h"
#include "dialbox.h"

/* ------ integer type for message parameters ----- */
typedef long DF_PARAM;

enum DfCondition
{
    DF_SRESTORED, DF_ISMINIMIZED, DF_ISMAXIMIZED, DF_ISCLOSING
};

typedef struct DfWindow
{
	DFCLASS class;		/* window class                  */
	char *title;		/* window title                  */
	int (*wndproc)(struct DfWindow *, enum DfMessages, DF_PARAM, DF_PARAM);

	/* ----------------- window colors -------------------- */
	char WindowColors[4][2];

	/* ---------------- window dimensions ----------------- */
	DFRECT rc;		/* window coordinates (0/0 to 79/24) */
	int ht, wd;		/* window height and width       */
	DFRECT RestoredRC;	/* restored condition rect       */

	/* -------------- linked list pointers ---------------- */
	struct DfWindow *parent;      /* parent window            */
	struct DfWindow *firstchild;  /* first child this parent  */
	struct DfWindow *lastchild;   /* last child this parent   */
	struct DfWindow *nextsibling; /* next sibling             */
	struct DfWindow *prevsibling; /* previous sibling         */
	struct DfWindow *childfocus;	/* child that ha(s/d) focus */

	int attrib;                 /* Window attributes        */
	PCHAR_INFO videosave;       /* video save buffer        */
	enum DfCondition condition;   /* Restored, Maximized,
	                               Minimized, Closing       */
	enum DfCondition oldcondition;/* previous condition       */
	int restored_attrib;        /* attributes when restored */
	void *extension;      /* menus, dialogs, documents, etc */
	struct DfWindow *PrevMouse;
	struct DfWindow *PrevKeyboard;
	struct DfWindow *MenuBarWnd;/* menu bar                   */
	struct DfWindow *StatusBar; /* status bar                 */
	int isHelping;		/* > 0 when help is being displayed */

	/* ----------------- text box fields ------------------ */
	int wlines;     /* number of lines of text              */
	int wtop;       /* text line that is on the top display */
	unsigned char *text; /* window text                     */
	unsigned int textlen;  /* text length                   */
	int wleft;      /* left position in window viewport     */
	int textwidth;  /* width of longest line in textbox     */
	int BlkBegLine; /* beginning line of marked block       */
	int BlkBegCol;  /* beginning column of marked block     */
	int BlkEndLine; /* ending line of marked block          */
	int BlkEndCol;  /* ending column of marked block        */
	int HScrollBox; /* position of horizontal scroll box    */
	int VScrollBox; /* position of vertical scroll box      */
	unsigned int *TextPointers; /* -> list of line offsets	*/

	/* ----------------- list box fields ------------------ */
	int selection;  /* current selection                    */
	BOOL AddMode;   /* adding extended selections mode      */
	int AnchorPoint;/* anchor point for extended selections */
	int SelectCount;/* count of selected items              */

	/* ----------------- edit box fields ------------------ */
	int CurrCol;      /* Current column                     */
	int CurrLine;     /* Current line                       */
	int WndRow;       /* Current window row                 */
	BOOL TextChanged; /* TRUE if text has changed           */
	unsigned char *DeletedText; /* for undo                 */
	unsigned DeletedLength; /* Length of deleted field      */
	BOOL InsertMode;   /* TRUE or FALSE for text insert     */
	BOOL WordWrapMode; /* TRUE or FALSE for word wrap       */
	unsigned int MaxTextLength; /* maximum text length      */

	/* ---------------- dialog box fields ----------------- */
	int ReturnCode;		/* return code from a dialog box */
	BOOL Modal;		/* True if a modeless dialog box */
	DF_CTLWINDOW *ct;		/* control structure             */
	struct DfWindow *dfocus;	/* control window that has focus */
	/* -------------- popdownmenu fields ------------------ */
	DF_MENU *mnu;		/* points to menu structure             */
	DF_MBAR *holdmenu;		/* previous active menu                 */
	struct DfWindow *oldFocus;

	/* --------------- help box fields -------------------- */
	void *firstword; /* -> first in list of key words       */
	void *lastword;  /* -> last in list of key words        */
	void *thisword;  /* -> current in list of key words     */
	/* -------------- status bar fields ------------------- */
	BOOL TimePosted; /* True if time has been posted        */
#ifdef INCLUDE_PICTUREBOX
	/* ------------- picture box fields ------------------- */
	int VectorCount;  /* number of vectors in vector list   */
	void *VectorList; /* list of picture box vectors        */
#endif
} * DFWINDOW;

#include "classdef.h"
#include "video.h"

void DfLogMessages (DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
void DfMessageLog(DFWINDOW);
/* ------- window methods ----------- */
#define DF_ICONHEIGHT 3
#define DF_ICONWIDTH  10
#define DfWindowHeight(w)      ((w)->ht)
#define DfWindowWidth(w)       ((w)->wd)
#define DfBorderAdj(w)         (DfTestAttribute(w,DF_HASBORDER)?1:0)
#define DfBottomBorderAdj(w)   (DfTestAttribute(w,DF_HASSTATUSBAR)?1:DfBorderAdj(w))
#define DfTopBorderAdj(w)      ((DfTestAttribute(w,DF_HASTITLEBAR) &&   \
                              DfTestAttribute(w,DF_HASMENUBAR)) ?  \
                              2 : (DfTestAttribute(w,DF_HASTITLEBAR | \
                              DF_HASMENUBAR | DF_HASBORDER) ? 1 : 0))
#define DfClientWidth(w)       (DfWindowWidth(w)-DfBorderAdj(w)*2)
#define DfClientHeight(w)      (DfWindowHeight(w)-DfTopBorderAdj(w)-\
                              DfBottomBorderAdj(w))
#define DfWindowRect(w)        ((w)->rc)
#define DfGetTop(w)            (DfRectTop(DfWindowRect(w)))
#define DfGetBottom(w)         (DfRectBottom(DfWindowRect(w)))
#define DfGetLeft(w)           (DfRectLeft(DfWindowRect(w)))
#define DfGetRight(w)          (DfRectRight(DfWindowRect(w)))
#define DfGetClientTop(w)      (DfGetTop(w)+DfTopBorderAdj(w))
#define DfGetClientBottom(w)   (DfGetBottom(w)-DfBottomBorderAdj(w))
#define DfGetClientLeft(w)     (DfGetLeft(w)+DfBorderAdj(w))
#define DfGetClientRight(w)    (DfGetRight(w)-DfBorderAdj(w))
#define DfGetTitle(w)          ((w)->title)
#define DfGetParent(w)         ((w)->parent)
#define DfFirstWindow(w)       ((w)->firstchild)
#define DfLastWindow(w)        ((w)->lastchild)
#define DfNextWindow(w)        ((w)->nextsibling)
#define DfPrevWindow(w)        ((w)->prevsibling)
#define DfGetClass(w)          ((w)->class)
#define DfGetAttribute(w)      ((w)->attrib)
#define DfAddAttribute(w,a)    (DfGetAttribute(w) |= a)
#define DfClearAttribute(w,a)  (DfGetAttribute(w) &= ~(a))
#define DfTestAttribute(w,a)   (DfGetAttribute(w) & (a))
#define isHidden(w)          (!(DfGetAttribute(w) & DF_VISIBLE))
#define DfSetVisible(w)        (DfGetAttribute(w) |= DF_VISIBLE)
#define DfClearVisible(w)      (DfGetAttribute(w) &= ~DF_VISIBLE)
#define DfGotoXY(w,x,y) DfCursor(w->rc.lf+(x)+1,w->rc.tp+(y)+1)
BOOL DfIsVisible(DFWINDOW);
DFWINDOW DfDfCreateWindow(DFCLASS,char *,int,int,int,int,void*,DFWINDOW,
       int (*)(struct DfWindow *,enum DfMessages,DF_PARAM,DF_PARAM),int);
void DfAddTitle(DFWINDOW, char *);
void DfInsertTitle(DFWINDOW, char *);
void DfDisplayTitle(DFWINDOW, DFRECT *);
void DfRepaintBorder(DFWINDOW, DFRECT *);
void DfPaintShadow(DFWINDOW);
void DfClearWindow(DFWINDOW, DFRECT *, int);
void DfWriteLine(DFWINDOW, char *, int, int, BOOL);
void DfInitWindowColors(DFWINDOW);

void DfSetNextFocus(void);
void DfSetPrevFocus(void);
void DfRemoveWindow(DFWINDOW);
void DfAppendWindow(DFWINDOW);
void DfReFocus(DFWINDOW);
void DfSkipApplicationControls(void);

BOOL DfCharInView(DFWINDOW, int, int);
void DfCreatePath(char *, char *, int, int);
#define DfSwapVideoBuffer(wnd, ish, fh) swapvideo(wnd, wnd->videosave, ish, fh)
int DfLineLength(char *);
DFRECT DfAdjustRectangle(DFWINDOW, DFRECT);
BOOL DfIsDerivedFrom(DFWINDOW, DFCLASS);
DFWINDOW DfGetAncestor(DFWINDOW);
void DfPutWindowChar(DFWINDOW,char,int,int);
void DfPutWindowLine(DFWINDOW, void *,int,int);
#define DfBaseWndProc(class,wnd,msg,p1,p2)    \
    (*DfClassDefs[(DfClassDefs[class].base)].wndproc)(wnd,msg,p1,p2)
#define DfDefaultWndProc(wnd,msg,p1,p2)         \
	(DfClassDefs[wnd->class].wndproc == NULL) ? \
	DfBaseWndProc(wnd->class,wnd,msg,p1,p2) :	  \
    (*DfClassDefs[wnd->class].wndproc)(wnd,msg,p1,p2)
struct DfLinkedList    {
    DFWINDOW DfFirstWindow;
    DFWINDOW DfLastWindow;
};
extern DFWINDOW DfApplicationWindow;
extern DFWINDOW DfInFocus;
extern DFWINDOW DfCaptureMouse;
extern DFWINDOW DfCaptureKeyboard;
extern int DfForeground, DfBackground;
extern BOOL DfWindowMoving;
extern BOOL DfWindowSizing;
extern BOOL DfVSliding;
extern BOOL DfHSliding;
extern char DFlatApplication[];
extern char *DfClipboard;
extern unsigned DfClipboardLength;
extern BOOL DfClipString;
/* --------- space between menubar labels --------- */
#define DF_MSPACE 2
/* --------------- border characters ------------- */
#define DF_FOCUS_NW      (unsigned char) '\xc9'
#define DF_FOCUS_NE      (unsigned char) '\xbb'
#define DF_FOCUS_SE      (unsigned char) '\xbc'
#define DF_FOCUS_SW      (unsigned char) '\xc8'
#define DF_FOCUS_SIDE    (unsigned char) '\xba'
#define DF_FOCUS_LINE    (unsigned char) '\xcd'
#define DF_NW            (unsigned char) '\xda'
#define DF_NE            (unsigned char) '\xbf'
#define DF_SE            (unsigned char) '\xd9'
#define DF_SW            (unsigned char) '\xc0'
#define DF_SIDE          (unsigned char) '\xb3'
#define DF_LINE          (unsigned char) '\xc4'
#define DF_LEDGE         (unsigned char) '\xc3'
#define DF_REDGE         (unsigned char) '\xb4'
/* ------------- scroll bar characters ------------ */
#define DF_UPSCROLLBOX    (unsigned char) '\x1e'
#define DF_DOWNSCROLLBOX  (unsigned char) '\x1f'
#define DF_LEFTSCROLLBOX  (unsigned char) '\x11'
#define DF_RIGHTSCROLLBOX (unsigned char) '\x10'
#define DF_SCROLLBARCHAR  (unsigned char) 176
#define DF_SCROLLBOXCHAR  (unsigned char) 178
/* ------------------ menu characters --------------------- */
#define DF_CHECKMARK      (unsigned char) '\x04' //(DF_SCREENHEIGHT==25?251:4)
#define DF_CASCADEPOINTER (unsigned char) '\x10'
/* ----------------- title bar characters ----------------- */
#define DF_CONTROLBOXCHAR (unsigned char) '\xf0'
#define DF_MAXPOINTER     24      /* maximize token            */
#define DF_MINPOINTER     25      /* minimize token            */
#define DF_RESTOREPOINTER 18      /* restore token             */
/* --------------- text control characters ---------------- */
#define DF_APPLCHAR     (unsigned char) 176 /* fills application window */
#define DF_SHORTCUTCHAR '~'    /* prefix: shortcut key display */
#define DF_CHANGECOLOR  (unsigned char) 174 /* prefix to change colors  */
#define DF_RESETCOLOR   (unsigned char) 175 /* reset colors to default  */
#define DF_LISTSELECTOR   4    /* selected list box entry      */

/* --------- message prototypes ----------- */
BOOL DfInitialize (void);
void DfTerminate (void);
void DfPostMessage (DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfSendMessage (DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
BOOL DfDispatchMessage (void);
void DfHandshake(void);
SHORT DfGetScreenHeight (void);
SHORT DfGetScreenWidth (void);

/* ---- standard window message processing prototypes ----- */
int DfApplicationProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfNormalProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfTextBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfListBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfEditBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfPictureProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfMenuBarProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfPopDownProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfButtonProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfComboProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfTextProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfRadioButtonProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfCheckBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfSpinButtonProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfDialogProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfSystemMenuProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfHelpBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfMessageBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfCancelBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfErrorBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfYesNoBoxProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfStatusBarProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
int DfWatchIconProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);

/* ------------- normal box prototypes ------------- */
void DfSetStandardColor(DFWINDOW);
void DfSetReverseColor(DFWINDOW);
BOOL DfIsAncestor(DFWINDOW, DFWINDOW);
#define DfHitControlBox(wnd, p1, p2)     \
     (DfTestAttribute(wnd, DF_CONTROLBOX) && \
     p1 == 2 && p2 == 0)
#define DfWndForeground(wnd) 		\
	(wnd->WindowColors [DF_STD_COLOR] [DF_FG])
#define DfWndBackground(wnd) 		\
	(wnd->WindowColors [DF_STD_COLOR] [DF_BG])
#define DfFrameForeground(wnd) 	\
	(wnd->WindowColors [DF_FRAME_COLOR] [DF_FG])
#define DfFrameBackground(wnd) 	\
	(wnd->WindowColors [DF_FRAME_COLOR] [DF_BG])
#define DfSelectForeground(wnd) 	\
	(wnd->WindowColors [DF_SELECT_COLOR] [DF_FG])
#define DfSelectBackground(wnd) 	\
	(wnd->WindowColors [DF_SELECT_COLOR] [DF_BG])
#define DfHighlightForeground(wnd) 	\
	(wnd->WindowColors [DF_HILITE_COLOR] [DF_FG])
#define DfHighlightBackground(wnd) 	\
	(wnd->WindowColors [DF_HILITE_COLOR] [DF_BG])
#define DfWindowClientColor(wnd, fg, bg) 	\
		DfWndForeground(wnd) = fg, DfWndBackground(wnd) = bg
#define DfWindowReverseColor(wnd, fg, bg) \
		DfSelectForeground(wnd) = fg, DfSelectBackground(wnd) = bg
#define DfWindowFrameColor(wnd, fg, bg) \
		DfFrameForeground(wnd) = fg, DfFrameBackground(wnd) = bg
#define DfWindowHighlightColor(wnd, fg, bg) \
		DfHighlightForeground(wnd) = fg, DfHighlightBackground(wnd) = bg
/* -------- text box prototypes ---------- */
#define DfTextLine(wnd, sel) \
      (wnd->text + *((wnd->TextPointers) + sel))
void DfWriteTextLine(DFWINDOW, DFRECT *, int, BOOL);
#define DfTextBlockMarked(wnd) (  wnd->BlkBegLine ||    \
                                wnd->BlkEndLine ||    \
                                wnd->BlkBegCol  ||    \
                                wnd->BlkEndCol)
void DfMarkTextBlock(DFWINDOW, int, int, int, int);
#define DfClearTextBlock(wnd) wnd->BlkBegLine = wnd->BlkEndLine =  \
                        wnd->BlkBegCol  = wnd->BlkEndCol = 0;
#define DfGetText(w)        ((w)->text)
#define DfGetTextLines(w)   ((w)->wlines)
void DfClearTextPointers(DFWINDOW);
void DfBuildTextPointers(DFWINDOW);
int DfTextLineNumber(DFWINDOW, char *);
/* ------------ DfClipboard prototypes ------------- */
void DfCopyTextToClipboard(char *);
void DfCopyToClipboard(DFWINDOW);
#define DfPasteFromClipboard(wnd) DfPasteText(wnd,DfClipboard,DfClipboardLength)
BOOL DfPasteText(DFWINDOW, char *, unsigned);
void DfClearClipboard(void);
/* --------- menu prototypes ---------- */
int DfCopyCommand(unsigned char *, unsigned char *, int, int);
void DfPrepFileMenu(void *, struct DfMenu *);
void DfPrepEditMenu(void *, struct DfMenu *);
void DfPrepSearchMenu(void *, struct DfMenu *);
void DfPrepWindowMenu(void *, struct DfMenu *);
void DfBuildSystemMenu(DFWINDOW);
BOOL isActive(DF_MBAR *, int);
char *DfGetCommandText(DF_MBAR *, int);
BOOL DfIsCascadedCommand(DF_MBAR *,int);
void DfActivateCommand(DF_MBAR *,int);
void DfDeactivateCommand(DF_MBAR *,int);
BOOL DfGetCommandToggle(DF_MBAR *,int);
void DfSetCommandToggle(DF_MBAR *,int);
void DfClearCommandToggle(DF_MBAR *,int);
void DfInvertCommandToggle(DF_MBAR *,int);
int DfBarSelection(int);
/* ------------- list box prototypes -------------- */
BOOL DfItemSelected(DFWINDOW, int);
/* ------------- edit box prototypes ----------- */
#define DfCurrChar (DfTextLine(wnd, wnd->CurrLine)+wnd->CurrCol)
#define DfWndCol   (wnd->CurrCol-wnd->wleft)
#define DfIsMultiLine(wnd) DfTestAttribute(wnd, DF_MULTILINE)
void DfSearchText(DFWINDOW);
void DfReplaceText(DFWINDOW);
void DfSearchNext(DFWINDOW);
/* --------- message box prototypes -------- */
DFWINDOW DfSliderBox(int, char *, char *);
BOOL DfInputBox(DFWINDOW, char *, char *, char *, int);
BOOL DfGenericMessage(DFWINDOW, char *, char *, int,
	int (*)(struct DfWindow *, enum DfMessages, DF_PARAM, DF_PARAM),
	char *, char *, int, int, int);
#define DfTestErrorMessage(msg)	\
	DfGenericMessage(NULL, "Error", msg, 2, DfErrorBoxProc,	  \
		DfOk, DfCancel, DF_ID_OK, DF_ID_CANCEL, TRUE)
#define DfErrorMessage(msg) \
	DfGenericMessage(NULL, "Error", msg, 1, DfErrorBoxProc,   \
		DfOk, NULL, DF_ID_OK, 0, TRUE)
#define DfMessageBox(ttl, msg) \
	DfGenericMessage(NULL, ttl, msg, 1, DfMessageBoxProc, \
		DfOk, NULL, DF_ID_OK, 0, TRUE)
#define DfYesNoBox(msg)	\
	DfGenericMessage(NULL, NULL, msg, 2, DfYesNoBoxProc,   \
		DfYes, DfNo, DF_ID_OK, DF_ID_CANCEL, TRUE)
#define DfCancelBox(wnd, msg) \
	DfGenericMessage(wnd, "Wait...", msg, 1, DfCancelBoxProc, \
		DfCancel, NULL, DF_ID_CANCEL, 0, FALSE)
void DfCloseCancelBox(void);
DFWINDOW DfMomentaryMessage(char *);
int DfMsgHeight(char *);
int DfMsgWidth(char *);

/* ------------- dialog box prototypes -------------- */
BOOL DfDialogBox(DFWINDOW, DF_DBOX *, BOOL,
       int (*)(struct DfWindow *, enum DfMessages, DF_PARAM, DF_PARAM));
void DfClearDialogBoxes(void);
BOOL DfOpenFileDialogBox(char *, char *);
BOOL DfSaveAsDialogBox(char *);
void DfGetDlgListText(DFWINDOW, char *, enum DfCommands);
BOOL DfDlgDirList(DFWINDOW, char *, enum DfCommands,
                            enum DfCommands, unsigned);
BOOL DfRadioButtonSetting(DF_DBOX *, enum DfCommands);
void DfPushRadioButton(DF_DBOX *, enum DfCommands);
void DfPutItemText(DFWINDOW, enum DfCommands, char *);
void DfPutComboListText(DFWINDOW, enum DfCommands, char *);
void DfGetItemText(DFWINDOW, enum DfCommands, char *, int);
char *DfGetDlgTextString(DF_DBOX *, enum DfCommands, DFCLASS);
void DfSetDlgTextString(DF_DBOX *, enum DfCommands, char *, DFCLASS);
BOOL DfCheckBoxSetting(DF_DBOX *, enum DfCommands);
DF_CTLWINDOW *DfFindCommand(DF_DBOX *, enum DfCommands, int);
DFWINDOW DfControlWindow(DF_DBOX *, enum DfCommands);
void DfSetScrollBars(DFWINDOW);
void DfSetRadioButton(DF_DBOX *, DF_CTLWINDOW *);
void DfControlSetting(DF_DBOX *, enum DfCommands, int, int);
void DfSetFocusCursor(DFWINDOW);

#define DfGetControl(wnd)             (wnd->ct)
#define DfGetDlgText(db, cmd)         DfGetDlgTextString(db, cmd, DF_TEXT)
#define DfGetDlgTextBox(db, cmd)      DfGetDlgTextString(db, cmd, DF_TEXTBOX)
#define DfGetEditBoxText(db, cmd)     DfGetDlgTextString(db, cmd, DF_EDITBOX)
#define DfGetComboBoxText(db, cmd)    DfGetDlgTextString(db, cmd, DF_COMBOBOX)
#define DfSetDlgText(db, cmd, s)      DfSetDlgTextString(db, cmd, s, DF_TEXT)
#define DfSetDlgTextBox(db, cmd, s)   DfSetDlgTextString(db, cmd, s, DF_TEXTBOX)
#define DfSetEditBoxText(db, cmd, s)  DfSetDlgTextString(db, cmd, s, DF_EDITBOX)
#define DfSetComboBoxText(db, cmd, s) DfSetDlgTextString(db, cmd, s, DF_COMBOBOX)
#define DfSetDlgTitle(db, ttl)        ((db)->dwnd.title = ttl)
#define DfSetCheckBox(db, cmd)        DfControlSetting(db, cmd, DF_CHECKBOX, DF_ON)
#define DfClearCheckBox(db, cmd)      DfControlSetting(db, cmd, DF_CHECKBOX, DF_OFF)
#define DfEnableButton(db, cmd)       DfControlSetting(db, cmd, DF_BUTTON, DF_ON)
#define DfDisableButton(db, cmd)      DfControlSetting(db, cmd, DF_BUTTON, DF_OFF)

/* ---- types of vectors that can be in a picture box ------- */
enum DfVectTypes {DF_VECTOR, DF_SOLIDBAR, DF_HEAVYBAR, DF_CROSSBAR, DF_LIGHTBAR};

/* ------------- picture box prototypes ------------- */
void DfDrawVector(DFWINDOW, int, int, int, int);
void DfDrawBox(DFWINDOW, int, int, int, int);
void DfDrawBar(DFWINDOW, enum DfVectTypes, int, int, int, int);
DFWINDOW DfWatchIcon(void);

/* ------------- help box prototypes ------------- */
void DfLoadHelpFile(void);
void DfUnLoadHelpFile(void);
BOOL DfDisplayHelp(DFWINDOW, char *);

extern char *DfClassNames[];

void DfBuildFileName(char *, char *);

#endif
