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
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>
#include <setjmp.h>

#ifndef VERSION
#define VERSION "Beta Version 0.3"
#endif

void *DFcalloc(size_t, size_t);
void *DFmalloc(size_t);
void *DFrealloc(void *, size_t);


#define MAXMESSAGES 50
#define DELAYTICKS 1
#define FIRSTDELAY 7
#define DOUBLETICKS 5

#define MAXTEXTLEN 65000U /* maximum text buffer            */
#define EDITLEN     1024  /* starting length for multiliner */
#define ENTRYLEN     256  /* starting length for one-liner  */
#define GROWLENGTH    64  /* buffers grow by this much      */

#include "system.h"
#include "config.h"
#include "rect.h"
#include "menu.h"
#include "keys.h"
#include "commands.h"
#include "dialbox.h"

/* ------ integer type for message parameters ----- */
typedef long PARAM;

enum Condition
{
    ISRESTORED, ISMINIMIZED, ISMAXIMIZED, ISCLOSING
};

typedef struct window
{
	DFCLASS class;		/* window class                  */
	char *title;		/* window title                  */
	int (*wndproc)(struct window *, enum messages, PARAM, PARAM);

	/* ----------------- window colors -------------------- */
	char WindowColors[4][2];

	/* ---------------- window dimensions ----------------- */
	DFRECT rc;		/* window coordinates (0/0 to 79/24) */
	int ht, wd;		/* window height and width       */
	DFRECT RestoredRC;	/* restored condition rect       */

	/* -------------- linked list pointers ---------------- */
	struct window *parent;      /* parent window            */
	struct window *firstchild;  /* first child this parent  */
	struct window *lastchild;   /* last child this parent   */
	struct window *nextsibling; /* next sibling             */
	struct window *prevsibling; /* previous sibling         */
	struct window *childfocus;	/* child that ha(s/d) focus */

	int attrib;                 /* Window attributes        */
	PCHAR_INFO videosave;       /* video save buffer        */
	enum Condition condition;   /* Restored, Maximized,
	                               Minimized, Closing       */
	enum Condition oldcondition;/* previous condition       */
	int restored_attrib;        /* attributes when restored */
	void *extension;      /* menus, dialogs, documents, etc */
	struct window *PrevMouse;
	struct window *PrevKeyboard;
	struct window *MenuBarWnd;/* menu bar                   */
	struct window *StatusBar; /* status bar                 */
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
	CTLWINDOW *ct;		/* control structure             */
	struct window *dfocus;	/* control window that has focus */
	/* -------------- popdownmenu fields ------------------ */
	MENU *mnu;		/* points to menu structure             */
	MBAR *holdmenu;		/* previous active menu                 */
	struct window *oldFocus;

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

void LogMessages (DFWINDOW, DFMESSAGE, PARAM, PARAM);
void MessageLog(DFWINDOW);
/* ------- window methods ----------- */
#define ICONHEIGHT 3
#define ICONWIDTH  10
#define WindowHeight(w)      ((w)->ht)
#define WindowWidth(w)       ((w)->wd)
#define BorderAdj(w)         (TestAttribute(w,HASBORDER)?1:0)
#define BottomBorderAdj(w)   (TestAttribute(w,HASSTATUSBAR)?1:BorderAdj(w))
#define TopBorderAdj(w)      ((TestAttribute(w,HASTITLEBAR) &&   \
                              TestAttribute(w,HASMENUBAR)) ?  \
                              2 : (TestAttribute(w,HASTITLEBAR | \
                              HASMENUBAR | HASBORDER) ? 1 : 0))
#define ClientWidth(w)       (WindowWidth(w)-BorderAdj(w)*2)
#define ClientHeight(w)      (WindowHeight(w)-TopBorderAdj(w)-\
                              BottomBorderAdj(w))
#define WindowRect(w)        ((w)->rc)
#define GetTop(w)            (RectTop(WindowRect(w)))
#define GetBottom(w)         (RectBottom(WindowRect(w)))
#define GetLeft(w)           (RectLeft(WindowRect(w)))
#define GetRight(w)          (RectRight(WindowRect(w)))
#define GetClientTop(w)      (GetTop(w)+TopBorderAdj(w))
#define GetClientBottom(w)   (GetBottom(w)-BottomBorderAdj(w))
#define GetClientLeft(w)     (GetLeft(w)+BorderAdj(w))
#define GetClientRight(w)    (GetRight(w)-BorderAdj(w))
#define GetTitle(w)          ((w)->title)
#define GetParent(w)         ((w)->parent)
#define FirstWindow(w)       ((w)->firstchild)
#define LastWindow(w)        ((w)->lastchild)
#define NextWindow(w)        ((w)->nextsibling)
#define PrevWindow(w)        ((w)->prevsibling)
#define GetClass(w)          ((w)->class)
#define GetAttribute(w)      ((w)->attrib)
#define AddAttribute(w,a)    (GetAttribute(w) |= a)
#define ClearAttribute(w,a)  (GetAttribute(w) &= ~(a))
#define TestAttribute(w,a)   (GetAttribute(w) & (a))
#define isHidden(w)          (!(GetAttribute(w) & VISIBLE))
#define SetVisible(w)        (GetAttribute(w) |= VISIBLE)
#define ClearVisible(w)      (GetAttribute(w) &= ~VISIBLE)
#define gotoxy(w,x,y) cursor(w->rc.lf+(x)+1,w->rc.tp+(y)+1)
BOOL isVisible(DFWINDOW);
DFWINDOW DfCreateWindow(DFCLASS,char *,int,int,int,int,void*,DFWINDOW,
       int (*)(struct window *,enum messages,PARAM,PARAM),int);
void AddTitle(DFWINDOW, char *);
void InsertTitle(DFWINDOW, char *);
void DisplayTitle(DFWINDOW, DFRECT *);
void RepaintBorder(DFWINDOW, DFRECT *);
void PaintShadow(DFWINDOW);
void ClearWindow(DFWINDOW, DFRECT *, int);
void writeline(DFWINDOW, char *, int, int, BOOL);
void InitWindowColors(DFWINDOW);

void SetNextFocus(void);
void SetPrevFocus(void);
void RemoveWindow(DFWINDOW);
void AppendWindow(DFWINDOW);
void ReFocus(DFWINDOW);
void SkipApplicationControls(void);

BOOL CharInView(DFWINDOW, int, int);
void CreatePath(char *, char *, int, int);
#define SwapVideoBuffer(wnd, ish, fh) swapvideo(wnd, wnd->videosave, ish, fh)
int LineLength(char *);
DFRECT AdjustRectangle(DFWINDOW, DFRECT);
BOOL isDerivedFrom(DFWINDOW, DFCLASS);
DFWINDOW GetAncestor(DFWINDOW);
void PutWindowChar(DFWINDOW,char,int,int);
void PutWindowLine(DFWINDOW, void *,int,int);
#define BaseWndProc(class,wnd,msg,p1,p2)    \
    (*classdefs[(classdefs[class].base)].wndproc)(wnd,msg,p1,p2)
#define DefaultWndProc(wnd,msg,p1,p2)         \
	(classdefs[wnd->class].wndproc == NULL) ? \
	BaseWndProc(wnd->class,wnd,msg,p1,p2) :	  \
    (*classdefs[wnd->class].wndproc)(wnd,msg,p1,p2)
struct LinkedList    {
    DFWINDOW FirstWindow;
    DFWINDOW LastWindow;
};
extern DFWINDOW ApplicationWindow;
extern DFWINDOW inFocus;
extern DFWINDOW CaptureMouse;
extern DFWINDOW CaptureKeyboard;
extern int foreground, background;
extern BOOL WindowMoving;
extern BOOL WindowSizing;
extern BOOL VSliding;
extern BOOL HSliding;
extern char DFlatApplication[];
extern char *Clipboard;
extern unsigned ClipboardLength;
extern BOOL ClipString;
/* --------- space between menubar labels --------- */
#define MSPACE 2
/* --------------- border characters ------------- */
#define FOCUS_NW      (unsigned char) '\xc9'
#define FOCUS_NE      (unsigned char) '\xbb'
#define FOCUS_SE      (unsigned char) '\xbc'
#define FOCUS_SW      (unsigned char) '\xc8'
#define FOCUS_SIDE    (unsigned char) '\xba'
#define FOCUS_LINE    (unsigned char) '\xcd'
#define NW            (unsigned char) '\xda'
#define NE            (unsigned char) '\xbf'
#define SE            (unsigned char) '\xd9'
#define SW            (unsigned char) '\xc0'
#define SIDE          (unsigned char) '\xb3'
#define LINE          (unsigned char) '\xc4'
#define LEDGE         (unsigned char) '\xc3'
#define REDGE         (unsigned char) '\xb4'
/* ------------- scroll bar characters ------------ */
#define UPSCROLLBOX    (unsigned char) '\x1e'
#define DOWNSCROLLBOX  (unsigned char) '\x1f'
#define LEFTSCROLLBOX  (unsigned char) '\x11'
#define RIGHTSCROLLBOX (unsigned char) '\x10'
#define SCROLLBARCHAR  (unsigned char) 176
#define SCROLLBOXCHAR  (unsigned char) 178
/* ------------------ menu characters --------------------- */
#define CHECKMARK      (unsigned char) '\x04' //(SCREENHEIGHT==25?251:4)
#define CASCADEPOINTER (unsigned char) '\x10'
/* ----------------- title bar characters ----------------- */
#define CONTROLBOXCHAR (unsigned char) '\xf0'
#define MAXPOINTER     24      /* maximize token            */
#define MINPOINTER     25      /* minimize token            */
#define RESTOREPOINTER 18      /* restore token             */
/* --------------- text control characters ---------------- */
#define APPLCHAR     (unsigned char) 176 /* fills application window */
#define SHORTCUTCHAR '~'    /* prefix: shortcut key display */
#define CHANGECOLOR  (unsigned char) 174 /* prefix to change colors  */
#define RESETCOLOR   (unsigned char) 175 /* reset colors to default  */
#define LISTSELECTOR   4    /* selected list box entry      */

/* --------- message prototypes ----------- */
BOOL DfInitialize (void);
void DfTerminate (void);
void DfPostMessage (DFWINDOW, DFMESSAGE, PARAM, PARAM);
int DfSendMessage (DFWINDOW, DFMESSAGE, PARAM, PARAM);
BOOL DfDispatchMessage (void);
void handshake(void);
SHORT DfGetScreenHeight (void);
SHORT DfGetScreenWidth (void);

/* ---- standard window message processing prototypes ----- */
int ApplicationProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int NormalProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int TextBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int ListBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int EditBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int PictureProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int MenuBarProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int PopDownProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int ButtonProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int ComboProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int TextProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int RadioButtonProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int CheckBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int SpinButtonProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int BoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int DialogProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int SystemMenuProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int HelpBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int MessageBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int CancelBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int ErrorBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int YesNoBoxProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int StatusBarProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
int WatchIconProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);

/* ------------- normal box prototypes ------------- */
void SetStandardColor(DFWINDOW);
void SetReverseColor(DFWINDOW);
BOOL isAncestor(DFWINDOW, DFWINDOW);
#define HitControlBox(wnd, p1, p2)     \
     (TestAttribute(wnd, CONTROLBOX) && \
     p1 == 2 && p2 == 0)
#define WndForeground(wnd) 		\
	(wnd->WindowColors [STD_COLOR] [FG])
#define WndBackground(wnd) 		\
	(wnd->WindowColors [STD_COLOR] [BG])
#define FrameForeground(wnd) 	\
	(wnd->WindowColors [FRAME_COLOR] [FG])
#define FrameBackground(wnd) 	\
	(wnd->WindowColors [FRAME_COLOR] [BG])
#define SelectForeground(wnd) 	\
	(wnd->WindowColors [SELECT_COLOR] [FG])
#define SelectBackground(wnd) 	\
	(wnd->WindowColors [SELECT_COLOR] [BG])
#define HighlightForeground(wnd) 	\
	(wnd->WindowColors [HILITE_COLOR] [FG])
#define HighlightBackground(wnd) 	\
	(wnd->WindowColors [HILITE_COLOR] [BG])
#define WindowClientColor(wnd, fg, bg) 	\
		WndForeground(wnd) = fg, WndBackground(wnd) = bg
#define WindowReverseColor(wnd, fg, bg) \
		SelectForeground(wnd) = fg, SelectBackground(wnd) = bg
#define WindowFrameColor(wnd, fg, bg) \
		FrameForeground(wnd) = fg, FrameBackground(wnd) = bg
#define WindowHighlightColor(wnd, fg, bg) \
		HighlightForeground(wnd) = fg, HighlightBackground(wnd) = bg
/* -------- text box prototypes ---------- */
#define TextLine(wnd, sel) \
      (wnd->text + *((wnd->TextPointers) + sel))
void WriteTextLine(DFWINDOW, DFRECT *, int, BOOL);
#define TextBlockMarked(wnd) (  wnd->BlkBegLine ||    \
                                wnd->BlkEndLine ||    \
                                wnd->BlkBegCol  ||    \
                                wnd->BlkEndCol)
void MarkTextBlock(DFWINDOW, int, int, int, int);
#define ClearTextBlock(wnd) wnd->BlkBegLine = wnd->BlkEndLine =  \
                        wnd->BlkBegCol  = wnd->BlkEndCol = 0;
#define GetText(w)        ((w)->text)
#define GetTextLines(w)   ((w)->wlines)
void ClearTextPointers(DFWINDOW);
void BuildTextPointers(DFWINDOW);
int TextLineNumber(DFWINDOW, char *);
/* ------------ Clipboard prototypes ------------- */
void CopyTextToClipboard(char *);
void CopyToClipboard(DFWINDOW);
#define PasteFromClipboard(wnd) PasteText(wnd,Clipboard,ClipboardLength)
BOOL PasteText(DFWINDOW, char *, unsigned);
void ClearClipboard(void);
/* --------- menu prototypes ---------- */
int CopyCommand(unsigned char *, unsigned char *, int, int);
void PrepFileMenu(void *, struct Menu *);
void PrepEditMenu(void *, struct Menu *);
void PrepSearchMenu(void *, struct Menu *);
void PrepWindowMenu(void *, struct Menu *);
void BuildSystemMenu(DFWINDOW);
BOOL isActive(MBAR *, int);
char *GetCommandText(MBAR *, int);
BOOL isCascadedCommand(MBAR *,int);
void ActivateCommand(MBAR *,int);
void DeactivateCommand(MBAR *,int);
BOOL GetCommandToggle(MBAR *,int);
void SetCommandToggle(MBAR *,int);
void ClearCommandToggle(MBAR *,int);
void InvertCommandToggle(MBAR *,int);
int BarSelection(int);
/* ------------- list box prototypes -------------- */
BOOL ItemSelected(DFWINDOW, int);
/* ------------- edit box prototypes ----------- */
#define CurrChar (TextLine(wnd, wnd->CurrLine)+wnd->CurrCol)
#define WndCol   (wnd->CurrCol-wnd->wleft)
#define isMultiLine(wnd) TestAttribute(wnd, MULTILINE)
void DfSearchText(DFWINDOW);
void DfReplaceText(DFWINDOW);
void DfSearchNext(DFWINDOW);
/* --------- message box prototypes -------- */
DFWINDOW SliderBox(int, char *, char *);
BOOL InputBox(DFWINDOW, char *, char *, char *, int);
BOOL GenericMessage(DFWINDOW, char *, char *, int,
	int (*)(struct window *, enum messages, PARAM, PARAM),
	char *, char *, int, int, int);
#define DfTestErrorMessage(msg)	\
	GenericMessage(NULL, "Error", msg, 2, ErrorBoxProc,	  \
		Ok, Cancel, ID_OK, ID_CANCEL, TRUE)
#define DfErrorMessage(msg) \
	GenericMessage(NULL, "Error", msg, 1, ErrorBoxProc,   \
		Ok, NULL, ID_OK, 0, TRUE)
#define DfMessageBox(ttl, msg) \
	GenericMessage(NULL, ttl, msg, 1, MessageBoxProc, \
		Ok, NULL, ID_OK, 0, TRUE)
#define DfYesNoBox(msg)	\
	GenericMessage(NULL, NULL, msg, 2, YesNoBoxProc,   \
		Yes, No, ID_OK, ID_CANCEL, TRUE)
#define DfCancelBox(wnd, msg) \
	GenericMessage(wnd, "Wait...", msg, 1, CancelBoxProc, \
		Cancel, NULL, ID_CANCEL, 0, FALSE)
void CloseCancelBox(void);
DFWINDOW MomentaryMessage(char *);
int MsgHeight(char *);
int MsgWidth(char *);

/* ------------- dialog box prototypes -------------- */
BOOL DfDialogBox(DFWINDOW, DBOX *, BOOL,
       int (*)(struct window *, enum messages, PARAM, PARAM));
void ClearDialogBoxes(void);
BOOL OpenFileDialogBox(char *, char *);
BOOL SaveAsDialogBox(char *);
void GetDlgListText(DFWINDOW, char *, enum commands);
BOOL DfDlgDirList(DFWINDOW, char *, enum commands,
                            enum commands, unsigned);
BOOL RadioButtonSetting(DBOX *, enum commands);
void PushRadioButton(DBOX *, enum commands);
void PutItemText(DFWINDOW, enum commands, char *);
void PutComboListText(DFWINDOW, enum commands, char *);
void GetItemText(DFWINDOW, enum commands, char *, int);
char *GetDlgTextString(DBOX *, enum commands, DFCLASS);
void SetDlgTextString(DBOX *, enum commands, char *, DFCLASS);
BOOL CheckBoxSetting(DBOX *, enum commands);
CTLWINDOW *FindCommand(DBOX *, enum commands, int);
DFWINDOW ControlWindow(DBOX *, enum commands);
void SetScrollBars(DFWINDOW);
void SetRadioButton(DBOX *, CTLWINDOW *);
void ControlSetting(DBOX *, enum commands, int, int);
void SetFocusCursor(DFWINDOW);

#define GetControl(wnd)             (wnd->ct)
#define GetDlgText(db, cmd)         GetDlgTextString(db, cmd, TEXT)
#define GetDlgTextBox(db, cmd)      GetDlgTextString(db, cmd, TEXTBOX)
#define GetEditBoxText(db, cmd)     GetDlgTextString(db, cmd, EDITBOX)
#define GetComboBoxText(db, cmd)    GetDlgTextString(db, cmd, COMBOBOX)
#define SetDlgText(db, cmd, s)      SetDlgTextString(db, cmd, s, TEXT)
#define SetDlgTextBox(db, cmd, s)   SetDlgTextString(db, cmd, s, TEXTBOX)
#define SetEditBoxText(db, cmd, s)  SetDlgTextString(db, cmd, s, EDITBOX)
#define SetComboBoxText(db, cmd, s) SetDlgTextString(db, cmd, s, COMBOBOX)
#define SetDlgTitle(db, ttl)        ((db)->dwnd.title = ttl)
#define SetCheckBox(db, cmd)        ControlSetting(db, cmd, CHECKBOX, ON)
#define ClearCheckBox(db, cmd)      ControlSetting(db, cmd, CHECKBOX, OFF)
#define EnableButton(db, cmd)       ControlSetting(db, cmd, BUTTON, ON)
#define DisableButton(db, cmd)      ControlSetting(db, cmd, BUTTON, OFF)

/* ---- types of vectors that can be in a picture box ------- */
enum VectTypes {VECTOR, SOLIDBAR, HEAVYBAR, CROSSBAR, LIGHTBAR};

/* ------------- picture box prototypes ------------- */
void DrawVector(DFWINDOW, int, int, int, int);
void DrawBox(DFWINDOW, int, int, int, int);
void DrawBar(DFWINDOW, enum VectTypes, int, int, int, int);
DFWINDOW WatchIcon(void);

/* ------------- help box prototypes ------------- */
void LoadHelpFile(void);
void UnLoadHelpFile(void);
BOOL DisplayHelp(DFWINDOW, char *);

extern char *ClassNames[];

void BuildFileName(char *, char *);

#endif
