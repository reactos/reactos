/*
 *	Edit control
 *
 *	Copyright  David W. Metcalfe, 1994
 *	Copyright  William Magro, 1995, 1996
 *	Copyright  Frans van Dorsselaer, 1996, 1997
 *
 */

/*
 *	please read EDIT.TODO (and update it when you change things)
 */

#include <windows.h>
#include <user32/win.h>
#include <user32/class.h>
#include <user32/nc.h>
#include <user32/resource.h>
#include <user32/combo.h>
#include <user32/debug.h>

#define MAX(x,y) x > y ? x : y 
#define MIN(x,y) x < y ? x : y 

#define BUFLIMIT_MULTI		65534	/* maximum buffer size (not including '\0')
					   FIXME: BTW, new specs say 65535 (do you dare ???) */
#define BUFLIMIT_SINGLE		766	/* maximum buffer size (not including '\0') */
#define BUFSTART_MULTI		1024	/* starting size */
#define BUFSTART_SINGLE		256	/* starting size */
#define GROWLENGTH		64	/* buffers grow by this much */
#define HSCROLL_FRACTION	3	/* scroll window by 1/3 width */

/*
 *	extra flags for EDITSTATE.flags field
 */
#define EF_MODIFIED		0x0001	/* text has been modified */
#define EF_FOCUSED		0x0002	/* we have input focus */
#define EF_UPDATE		0x0004	/* notify parent of changed state on next WM_PAINT */
#define EF_VSCROLL_TRACK	0x0008	/* don't SetScrollPos() since we are tracking the thumb */
#define EF_HSCROLL_TRACK	0x0010	/* don't SetScrollPos() since we are tracking the thumb */
#define EF_VSCROLL_HACK		0x0020	/* we already have informed the user of the hacked handler */
#define EF_HSCROLL_HACK		0x0040	/* we already have informed the user of the hacked handler */
#define EF_AFTER_WRAP		0x0080	/* the caret is displayed after the last character of a
					   wrapped line, instead of in front of the next character */
#define EF_USE_SOFTBRK		0x0100	/* Enable soft breaks in text. */

typedef enum
{
	END_0 = 0,	/* line ends with terminating '\0' character */
	END_WRAP,	/* line is wrapped */
	END_HARD,	/* line ends with a hard return '\r\n' */
	END_SOFT	/* line ends with a soft return '\r\r\n' */
} LINE_END;

typedef struct tagLINEDEF {
	INT length;		/* bruto length of a line in bytes */
	INT net_length;	/* netto length of a line in visible characters */
	LINE_END ending;
	INT width;		/* width of the line in pixels */
	struct tagLINEDEF *next;
} LINEDEF;

typedef INT   (CALLBACK *EDITWORDBREAKPROCA)(LPSTR,INT,INT,INT);
typedef INT   (CALLBACK *EDITWORDBREAKPROCW)(LPWSTR,INT,INT,INT);

typedef struct
{
	HANDLE heap;			/* our own heap */
	LPSTR text;			/* the actual contents of the control */
	INT buffer_size;		/* the size of the buffer */
	INT buffer_limit;		/* the maximum size to which the buffer may grow */
	HFONT font;			/* NULL means standard system font */
	INT x_offset;			/* scroll offset	for multi lines this is in pixels
								for single lines it's in characters */
	INT line_height;		/* height of a screen line in pixels */
	INT char_width;		/* average character width in pixels */
	DWORD style;			/* sane version of wnd->dwStyle */
	WORD flags;			/* flags that are not in es->style or wnd->flags (EF_XXX) */
	INT undo_insert_count;	/* number of characters inserted in sequence */
	INT undo_position;		/* character index of the insertion and deletion */
	LPSTR undo_text;		/* deleted text */
	INT undo_buffer_size;		/* size of the deleted text buffer */
	INT selection_start;		/* == selection_end if no selection */
	INT selection_end;		/* == current caret position */
	CHAR password_char;		/* == 0 if no password char, and for multi line controls */
	INT left_margin;		/* in pixels */
	INT right_margin;		/* in pixels */
	RECT format_rect;
	INT region_posx;		/* Position of cursor relative to region: */
	INT region_posy;		/* -1: to left, 0: within, 1: to right */
	EDITWORDBREAKPROCA word_break_procA;
	EDITWORDBREAKPROCW word_break_procW;
	INT line_count;		/* number of lines */
	INT y_offset;			/* scroll offset in number of lines */
	/*
	 *	only for multi line controls
	 */
	INT lock_count;		/* amount of re-entries in the EditWndProc */
	INT tabs_count;
	LPINT tabs;
	INT text_width;		/* width of the widest line in pixels */
	LINEDEF *first_line_def;	/* linked list of (soft) linebreaks */
	HLOCAL hloc;		/* for controls receiving EM_GETHANDLE */
} EDITSTATE;

#define SLOWORD(l)             ((INT)(LONG)(l))
#define SHIWORD(l)             ((INT)((LONG)(l) >> 16))


#define SWAP_INT(x,y) do { INT temp = (INT)(x); (x) = (INT)(y); (y) = temp; } while(0)
#define ORDER_INT(x,y) do { if ((INT)(y) < (INT)(x)) SWAP_INT((x),(y)); } while(0)

#define SWAP_UINT(x,y) do { UINT temp = (UINT)(x); (x) = (UINT)(y); (y) = temp; } while(0)
#define ORDER_UINT(x,y) do { if ((UINT)(y) < (UINT)(x)) SWAP_UINT((x),(y)); } while(0)

#define DPRINTF_EDIT_NOTIFY(hwnd, str) \
	({DPRINT( "notification " str " sent to hwnd=%08x\n", \
		       (UINT)(hwnd));})

#define EDIT_SEND_CTLCOLOR(wnd,hdc) \
	(SendMessageA((wnd)->parent->hwndSelf, WM_CTLCOLOREDIT, \
			(WPARAM)(hdc), (LPARAM)(wnd)->hwndSelf))
#define EDIT_NOTIFY_PARENT(wnd, wNotifyCode, str) \
	(DPRINTF_EDIT_NOTIFY((wnd)->parent->hwndSelf, str), \
	SendMessageA((wnd)->parent->hwndSelf, WM_COMMAND, \
			MAKEWPARAM((wnd)->wIDmenu, wNotifyCode), \
			(LPARAM)(wnd)->hwndSelf))

#define DPRINTF_EDIT_MSG(str) \
	DPRINT( \
		     " bit : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n", \
		     (UINT)hwnd, (UINT)wParam, (UINT)lParam)

/*********************************************************************
 *
 *	Declarations
 *
 */

/*
 *	These functions have trivial implementations
 *	We still like to call them internally
 *	"static __inline__" makes them more like macro's
 */
static __inline__ BOOL	EDIT_EM_CanUndo(WND *wnd, EDITSTATE *es);
static __inline__ void		EDIT_EM_EmptyUndoBuffer(WND *wnd, EDITSTATE *es);
static __inline__ void		EDIT_WM_Clear(WND *wnd, EDITSTATE *es);
static __inline__ void		EDIT_WM_Cut(WND *wnd, EDITSTATE *es);
/*
 *	This is the only exported function
 */
LRESULT WINAPI EditWndProc( HWND hwnd, UINT msg,
                            WPARAM wParam, LPARAM lParam );
/*
 *	Helper functions only valid for one type of control
 */
static void	EDIT_BuildLineDefs_ML(WND *wnd, EDITSTATE *es);
static LPSTR	EDIT_GetPasswordPointer_SL(WND *wnd, EDITSTATE *es);
static void	EDIT_MoveDown_ML(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MovePageDown_ML(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MovePageUp_ML(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveUp_ML(WND *wnd, EDITSTATE *es, BOOL extend);
/*
 *	Helper functions valid for both single line _and_ multi line controls
 */
static INT	EDIT_CallWordBreakProc(WND *wnd, EDITSTATE *es, INT start, INT index, INT count, INT action);
static INT	EDIT_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y, LPBOOL after_wrap);
static void	EDIT_ConfinePoint(WND *wnd, EDITSTATE *es, LPINT x, LPINT y);
static void	EDIT_GetLineRect(WND *wnd, EDITSTATE *es, INT line, INT scol, INT ecol, LPRECT rc);
static void	EDIT_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end);
static void	EDIT_LockBuffer(WND *wnd, EDITSTATE *es);
static BOOL	EDIT_MakeFit(WND *wnd, EDITSTATE *es, INT size);
static BOOL	EDIT_MakeUndoFit(WND *wnd, EDITSTATE *es, INT size);
static void	EDIT_MoveBackward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveEnd(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveForward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveHome(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveWordBackward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveWordForward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_PaintLine(WND *wnd, EDITSTATE *es, HDC hdc, INT line, BOOL rev);
static INT	EDIT_PaintText(WND *wnd, EDITSTATE *es, HDC hdc, INT x, INT y, INT line, INT col, INT count, BOOL rev);
static void	EDIT_SetCaretPos(WND *wnd, EDITSTATE *es, INT pos, BOOL after_wrap); 
static void	EDIT_SetRectNP(WND *wnd, EDITSTATE *es, LPRECT lprc);
static void	EDIT_UnlockBuffer(WND *wnd, EDITSTATE *es, BOOL force);
static INT	EDIT_WordBreakProc(LPSTR s, INT index, INT count, INT action);
/*
 *	EM_XXX message handlers
 */
static LRESULT	EDIT_EM_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y);
static BOOL	EDIT_EM_FmtLines(WND *wnd, EDITSTATE *es, BOOL add_eol);
static HLOCAL	EDIT_EM_GetHandle(WND *wnd, EDITSTATE *es);
static INT	EDIT_EM_GetLine(WND *wnd, EDITSTATE *es, INT line, LPSTR lpch);
static LRESULT	EDIT_EM_GetSel(WND *wnd, EDITSTATE *es, UINT *start, UINT *end);
static LRESULT	EDIT_EM_GetThumb(WND *wnd, EDITSTATE *es);
static INT	EDIT_EM_LineFromChar(WND *wnd, EDITSTATE *es, INT index);
static INT	EDIT_EM_LineIndex(WND *wnd, EDITSTATE *es, INT line);
static INT	EDIT_EM_LineLength(WND *wnd, EDITSTATE *es, INT index);
static BOOL	EDIT_EM_LineScroll(WND *wnd, EDITSTATE *es, INT dx, INT dy);
static LRESULT	EDIT_EM_PosFromChar(WND *wnd, EDITSTATE *es, INT index, BOOL after_wrap);
static void	EDIT_EM_ReplaceSel(WND *wnd, EDITSTATE *es, BOOL can_undo, LPCSTR lpsz_replace);
static LRESULT	EDIT_EM_Scroll(WND *wnd, EDITSTATE *es, INT action);
static void	EDIT_EM_ScrollCaret(WND *wnd, EDITSTATE *es);
static void	EDIT_EM_SetHandle(WND *wnd, EDITSTATE *es, HLOCAL hloc);
static void	EDIT_EM_SetLimitText(WND *wnd, EDITSTATE *es, INT limit);
static void	EDIT_EM_SetMargins(WND *wnd, EDITSTATE *es, INT action, INT left, INT right);
static void	EDIT_EM_SetPasswordChar(WND *wnd, EDITSTATE *es, CHAR c);
static void	EDIT_EM_SetSel(WND *wnd, EDITSTATE *es, UINT start, UINT end, BOOL after_wrap);
static BOOL	EDIT_EM_SetTabStops(WND *wnd, EDITSTATE *es, INT count, LPINT tabs);
static void	EDIT_EM_SetWordBreakProc(WND *wnd, EDITSTATE *es, EDITWORDBREAKPROCA wbp);
static BOOL	EDIT_EM_Undo(WND *wnd, EDITSTATE *es);
/*
 *	WM_XXX message handlers
 */
static void	EDIT_WM_Char(WND *wnd, EDITSTATE *es, CHAR c, DWORD key_data);
static void	EDIT_WM_Command(WND *wnd, EDITSTATE *es, INT code, INT id, HWND conrtol);
static void	EDIT_WM_ContextMenu(WND *wnd, EDITSTATE *es, HWND hwnd, INT x, INT y);
static void	EDIT_WM_Copy(WND *wnd, EDITSTATE *es);
static LRESULT	EDIT_WM_Create(WND *wnd, EDITSTATE *es, LPCREATESTRUCTA cs);
static void	EDIT_WM_Destroy(WND *wnd, EDITSTATE *es);
static LRESULT	EDIT_WM_EraseBkGnd(WND *wnd, EDITSTATE *es, HDC dc);
static INT	EDIT_WM_GetText(WND *wnd, EDITSTATE *es, INT count, LPSTR text);
static LRESULT	EDIT_WM_HScroll(WND *wnd, EDITSTATE *es, INT action, INT pos, HWND scroll_bar);
static LRESULT	EDIT_WM_KeyDown(WND *wnd, EDITSTATE *es, INT key, DWORD key_data);
static LRESULT	EDIT_WM_KillFocus(WND *wnd, EDITSTATE *es, HWND window_getting_focus);
static LRESULT	EDIT_WM_LButtonDblClk(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y);
static LRESULT	EDIT_WM_LButtonDown(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y);
static LRESULT	EDIT_WM_LButtonUp(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y);
static LRESULT	EDIT_WM_MouseMove(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y);
static LRESULT	EDIT_WM_NCCreate(WND *wnd, LPCREATESTRUCTA cs);
static void	EDIT_WM_Paint(WND *wnd, EDITSTATE *es);
static void	EDIT_WM_Paste(WND *wnd, EDITSTATE *es);
static void	EDIT_WM_SetFocus(WND *wnd, EDITSTATE *es, HWND window_losing_focus);
static void	EDIT_WM_SetFont(WND *wnd, EDITSTATE *es, HFONT font, BOOL redraw);
static void	EDIT_WM_SetText(WND *wnd, EDITSTATE *es, LPCSTR text);
static void	EDIT_WM_Size(WND *wnd, EDITSTATE *es, UINT action, INT width, INT height);
static LRESULT	EDIT_WM_SysKeyDown(WND *wnd, EDITSTATE *es, INT key, DWORD key_data);
static void	EDIT_WM_Timer(WND *wnd, EDITSTATE *es, INT id, TIMERPROC timer_proc);
static LRESULT	EDIT_WM_VScroll(WND *wnd, EDITSTATE *es, INT action, INT pos, HWND scroll_bar);


/*********************************************************************
 *
 *	EM_CANUNDO
 *
 */
static __inline__ BOOL EDIT_EM_CanUndo(WND *wnd, EDITSTATE *es)
{
	return (es->undo_insert_count || lstrlenA(es->undo_text));
}


/*********************************************************************
 *
 *	EM_EMPTYUNDOBUFFER
 *
 */
static __inline__ void EDIT_EM_EmptyUndoBuffer(WND *wnd, EDITSTATE *es)
{
	es->undo_insert_count = 0;
	*es->undo_text = '\0';
}


/*********************************************************************
 *
 *	WM_CLEAR
 *
 */
static __inline__ void EDIT_WM_Clear(WND *wnd, EDITSTATE *es)
{
	EDIT_EM_ReplaceSel(wnd, es, TRUE, "");
}


/*********************************************************************
 *
 *	WM_CUT
 *
 */
static __inline__ void EDIT_WM_Cut(WND *wnd, EDITSTATE *es)
{
	EDIT_WM_Copy(wnd, es);
	EDIT_WM_Clear(wnd, es);
}


/*********************************************************************
 *
 *	EditWndProc()
 *
 *	The messages are in the order of the actual integer values
 *	(which can be found in include/windows.h)
 *	Whereever possible the 16 bit versions are converted to
 *	the  bit ones, so that we can 'fall through' to the
 *	helper functions.  These are mostly  bit (with a few
 *	exceptions, clearly indicated by a '16' extension to their
 *	names).
 *
 */
LRESULT WINAPI EditWndProc( HWND hwnd, UINT msg,
                            WPARAM wParam, LPARAM lParam )
{
	WND *wnd = WIN_FindWndPtr(hwnd);
	EDITSTATE *es = *(EDITSTATE **)((wnd)->wExtra);
	LRESULT result = 0;

	switch (msg) {
	case WM_DESTROY:
		DPRINTF_EDIT_MSG("WM_DESTROY");
		EDIT_WM_Destroy(wnd, es);
		return 0;

	case WM_NCCREATE:
		DPRINTF_EDIT_MSG("WM_NCCREATE");
		return EDIT_WM_NCCreate(wnd, (LPCREATESTRUCTA)lParam);
	}

	if (!es)
		return DefWindowProcA(hwnd, msg, wParam, lParam);

	EDIT_LockBuffer(wnd, es);
	switch (msg) {
	case EM_GETSEL:
		DPRINTF_EDIT_MSG("EM_GETSEL");
		result = EDIT_EM_GetSel(wnd, es, (UINT *)wParam, (UINT *)lParam);
		break;

	
	case EM_SETSEL:
		DPRINTF_EDIT_MSG("EM_SETSEL");
		EDIT_EM_SetSel(wnd, es, wParam, lParam, FALSE);
		result = 1;
		break;

	
	case EM_GETRECT:
		DPRINTF_EDIT_MSG("EM_GETRECT");
		if (lParam)
			CopyRect((LPRECT)lParam, &es->format_rect);
		break;


	case EM_SETRECT:
		DPRINTF_EDIT_MSG("EM_SETRECT");
		if ((es->style & ES_MULTILINE) && lParam) {
			EDIT_SetRectNP(wnd, es, (LPRECT)lParam);
			InvalidateRect(wnd->hwndSelf, NULL, TRUE);
		}
		break;


	case EM_SETRECTNP:
		DPRINTF_EDIT_MSG("EM_SETRECTNP");
		if ((es->style & ES_MULTILINE) && lParam)
			EDIT_SetRectNP(wnd, es, (LPRECT)lParam);
		break;


	case EM_SCROLL:
		DPRINTF_EDIT_MSG("EM_SCROLL");
		result = EDIT_EM_Scroll(wnd, es, (INT)wParam);
 		break;

	
	case EM_LINESCROLL:
		DPRINTF_EDIT_MSG("EM_LINESCROLL");
		result = (LRESULT)EDIT_EM_LineScroll(wnd, es, (INT)wParam, (INT)lParam);
		break;

	
	case EM_SCROLLCARET:
		DPRINTF_EDIT_MSG("EM_SCROLLCARET");
		EDIT_EM_ScrollCaret(wnd, es);
		result = 1;
		break;

	case EM_GETMODIFY:
		DPRINTF_EDIT_MSG("EM_GETMODIFY");
		return ((es->flags & EF_MODIFIED) != 0);
		break;

	case EM_SETMODIFY:
		DPRINTF_EDIT_MSG("EM_SETMODIFY");
		if (wParam)
			es->flags |= EF_MODIFIED;
		else
			es->flags &= ~EF_MODIFIED;
		break;

	case EM_GETLINECOUNT:
		DPRINTF_EDIT_MSG("EM_GETLINECOUNT");
		result = (es->style & ES_MULTILINE) ? es->line_count : 1;
		break;

	case EM_LINEINDEX:
		DPRINTF_EDIT_MSG("EM_LINEINDEX");
		result = (LRESULT)EDIT_EM_LineIndex(wnd, es, (INT)wParam);
		break;

	case EM_SETHANDLE:
		DPRINTF_EDIT_MSG("EM_SETHANDLE");
		EDIT_EM_SetHandle(wnd, es, (HLOCAL)wParam);
		break;

	case EM_GETHANDLE:
		DPRINTF_EDIT_MSG("EM_GETHANDLE");
		result = (LRESULT)EDIT_EM_GetHandle(wnd, es);
		break;

	case EM_GETTHUMB:
		DPRINTF_EDIT_MSG("EM_GETTHUMB");
		result = EDIT_EM_GetThumb(wnd, es);
		break;

	/* messages 0x00bf and 0x00c0 missing from specs */

	case 0x00bf:
		DPRINTF_EDIT_MSG("undocumented 0x00bf, please report");
		result = DefWindowProcA(hwnd, msg, wParam, lParam);
		break;

	case 0x00c0:
		DPRINTF_EDIT_MSG("undocumented 0x00c0, please report");
		result = DefWindowProcA(hwnd, msg, wParam, lParam);
		break;

	case EM_LINELENGTH:
		DPRINTF_EDIT_MSG("EM_LINELENGTH");
		result = (LRESULT)EDIT_EM_LineLength(wnd, es, (INT)wParam);
		break;


	case EM_REPLACESEL:
		DPRINTF_EDIT_MSG("EM_REPLACESEL");
		EDIT_EM_ReplaceSel(wnd, es, (BOOL)wParam, (LPCSTR)lParam);
		result = 1;
		break;

	/* message 0x00c3 missing from specs */

	case 0x00c3:
		DPRINTF_EDIT_MSG("undocumented 0x00c3, please report");
		result = DefWindowProcA(hwnd, msg, wParam, lParam);
		break;

	case EM_GETLINE:
		DPRINTF_EDIT_MSG("EM_GETLINE");
		result = (LRESULT)EDIT_EM_GetLine(wnd, es, (INT)wParam, (LPSTR)lParam);
		break;

	case EM_SETLIMITTEXT:
		DPRINTF_EDIT_MSG("EM_SETLIMITTEXT");
		EDIT_EM_SetLimitText(wnd, es, (INT)wParam);
		break;

	case EM_CANUNDO:
		DPRINTF_EDIT_MSG("EM_CANUNDO");
		result = (LRESULT)EDIT_EM_CanUndo(wnd, es);
		break;

	case EM_UNDO:
		/* fall through */
	case WM_UNDO:
		DPRINTF_EDIT_MSG("EM_UNDO / WM_UNDO");
		result = (LRESULT)EDIT_EM_Undo(wnd, es);
		break;

	case EM_FMTLINES:
		DPRINTF_EDIT_MSG("EM_FMTLINES");
		result = (LRESULT)EDIT_EM_FmtLines(wnd, es, (BOOL)wParam);
		break;

	case EM_LINEFROMCHAR:
		DPRINTF_EDIT_MSG("EM_LINEFROMCHAR");
		result = (LRESULT)EDIT_EM_LineFromChar(wnd, es, (INT)wParam);
		break;

	/* message 0x00ca missing from specs */

	case 0x00ca:
		DPRINTF_EDIT_MSG("undocumented 0x00ca, please report");
		result = DefWindowProcA(hwnd, msg, wParam, lParam);
		break;

	case EM_SETTABSTOPS:
		DPRINTF_EDIT_MSG("EM_SETTABSTOPS");
		result = (LRESULT)EDIT_EM_SetTabStops(wnd, es, (INT)wParam, (LPINT)lParam);
		break;

	case EM_SETPASSWORDCHAR:
		DPRINTF_EDIT_MSG("EM_SETPASSWORDCHAR");
		EDIT_EM_SetPasswordChar(wnd, es, (CHAR)wParam);
		break;

	case EM_EMPTYUNDOBUFFER:
		DPRINTF_EDIT_MSG("EM_EMPTYUNDOBUFFER");
		EDIT_EM_EmptyUndoBuffer(wnd, es);
		break;

	case EM_GETFIRSTVISIBLELINE:
		DPRINTF_EDIT_MSG("EM_GETFIRSTVISIBLELINE");
		result = (es->style & ES_MULTILINE) ? es->y_offset : es->x_offset;
		break;

	case EM_SETREADONLY:
		DPRINTF_EDIT_MSG("EM_SETREADONLY");
		if (wParam) {
			wnd->dwStyle |= ES_READONLY;
			es->style |= ES_READONLY;
		} else {
			wnd->dwStyle &= ~ES_READONLY;
			es->style &= ~ES_READONLY;
		}
		return 1;
 		break;

	case EM_SETWORDBREAKPROC:
		DPRINTF_EDIT_MSG("EM_SETWORDBREAKPROC");
		EDIT_EM_SetWordBreakProc(wnd, es, (EDITWORDBREAKPROCA)lParam);
		break;

	case EM_GETWORDBREAKPROC:
		DPRINTF_EDIT_MSG("EM_GETWORDBREAKPROC");
		result = (LRESULT)es->word_break_procA;
		break;

	case EM_GETPASSWORDCHAR:
		DPRINTF_EDIT_MSG("EM_GETPASSWORDCHAR");
		result = es->password_char;
		break;

	/* The following EM_xxx are new to win95 and don't exist for 16 bit */

	case EM_SETMARGINS:
		DPRINTF_EDIT_MSG("EM_SETMARGINS");
		EDIT_EM_SetMargins(wnd, es, (INT)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case EM_GETMARGINS:
		DPRINTF_EDIT_MSG("EM_GETMARGINS");
		result = MAKELONG(es->left_margin, es->right_margin);
		break;

	case EM_GETLIMITTEXT:
		DPRINTF_EDIT_MSG("EM_GETLIMITTEXT");
		result = es->buffer_limit;
		break;

	case EM_POSFROMCHAR:
		DPRINTF_EDIT_MSG("EM_POSFROMCHAR");
		result = EDIT_EM_PosFromChar(wnd, es, (INT)wParam, FALSE);
		break;

	case EM_CHARFROMPOS:
		DPRINTF_EDIT_MSG("EM_CHARFROMPOS");
		result = EDIT_EM_CharFromPos(wnd, es, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_GETDLGCODE:
		DPRINTF_EDIT_MSG("WM_GETDLGCODE");
		result = (es->style & ES_MULTILINE) ?
				DLGC_WANTALLKEYS | DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS :
				DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;
		break;

	case WM_CHAR:
		DPRINTF_EDIT_MSG("WM_CHAR");
		EDIT_WM_Char(wnd, es, (CHAR)wParam, (DWORD)lParam);
		break;

	case WM_CLEAR:
		DPRINTF_EDIT_MSG("WM_CLEAR");
		EDIT_WM_Clear(wnd, es);
		break;

	case WM_COMMAND:
		DPRINTF_EDIT_MSG("WM_COMMAND");
		EDIT_WM_Command(wnd, es, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
		break;

 	case WM_CONTEXTMENU:
		DPRINTF_EDIT_MSG("WM_CONTEXTMENU");
		EDIT_WM_ContextMenu(wnd, es, (HWND)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_COPY:
		DPRINTF_EDIT_MSG("WM_COPY");
		EDIT_WM_Copy(wnd, es);
		break;

	case WM_CREATE:
		DPRINTF_EDIT_MSG("WM_CREATE");
		result = EDIT_WM_Create(wnd, es, (LPCREATESTRUCTA)lParam);
		break;

	case WM_CUT:
		DPRINTF_EDIT_MSG("WM_CUT");
		EDIT_WM_Cut(wnd, es);
		break;

	case WM_ENABLE:
		DPRINTF_EDIT_MSG("WM_ENABLE");
		InvalidateRect(hwnd, NULL, TRUE);
		break;

	case WM_ERASEBKGND:
		DPRINTF_EDIT_MSG("WM_ERASEBKGND");
		result = EDIT_WM_EraseBkGnd(wnd, es, (HDC)wParam);
		break;

	case WM_GETFONT:
		DPRINTF_EDIT_MSG("WM_GETFONT");
		result = (LRESULT)es->font;
		break;

	case WM_GETTEXT:
		DPRINTF_EDIT_MSG("WM_GETTEXT");
		result = (LRESULT)EDIT_WM_GetText(wnd, es, (INT)wParam, (LPSTR)lParam);
		break;

	case WM_GETTEXTLENGTH:
		DPRINTF_EDIT_MSG("WM_GETTEXTLENGTH");
		result = lstrlenA(es->text);
		break;

	case WM_HSCROLL:
		DPRINTF_EDIT_MSG("WM_HSCROLL");
		result = EDIT_WM_HScroll(wnd, es, LOWORD(wParam), SHIWORD(wParam), (HWND)lParam);
		break;

	case WM_KEYDOWN:
		DPRINTF_EDIT_MSG("WM_KEYDOWN");
		result = EDIT_WM_KeyDown(wnd, es, (INT)wParam, (DWORD)lParam);
		break;

	case WM_KILLFOCUS:
		DPRINTF_EDIT_MSG("WM_KILLFOCUS");
		result = EDIT_WM_KillFocus(wnd, es, (HWND)wParam);
		break;

	case WM_LBUTTONDBLCLK:
		DPRINTF_EDIT_MSG("WM_LBUTTONDBLCLK");
		result = EDIT_WM_LButtonDblClk(wnd, es, (DWORD)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_LBUTTONDOWN:
		DPRINTF_EDIT_MSG("WM_LBUTTONDOWN");
		result = EDIT_WM_LButtonDown(wnd, es, (DWORD)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_LBUTTONUP:
		DPRINTF_EDIT_MSG("WM_LBUTTONUP");
		result = EDIT_WM_LButtonUp(wnd, es, (DWORD)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_MOUSEACTIVATE:
		/*
		 *	FIXME: maybe DefWindowProc() screws up, but it seems that
		 *		modalless dialog boxes need this.  If we don't do this, the focus
		 *		will _not_ be set by DefWindowProc() for edit controls in a
		 *		modalless dialog box ???
		 */
		DPRINTF_EDIT_MSG("WM_MOUSEACTIVATE");
		SetFocus(wnd->hwndSelf);
		result = MA_ACTIVATE;
		break;

	case WM_MOUSEMOVE:
		/*
		 *	DPRINTF_EDIT_MSG("WM_MOUSEMOVE");
		 */
		result = EDIT_WM_MouseMove(wnd, es, (DWORD)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_PAINT:
		DPRINTF_EDIT_MSG("WM_PAINT");
		EDIT_WM_Paint(wnd, es);
		break;

	case WM_PASTE:
		DPRINTF_EDIT_MSG("WM_PASTE");
		EDIT_WM_Paste(wnd, es);
		break;

	case WM_SETFOCUS:
		DPRINTF_EDIT_MSG("WM_SETFOCUS");
		EDIT_WM_SetFocus(wnd, es, (HWND)wParam);
		break;

	case WM_SETFONT:
		DPRINTF_EDIT_MSG("WM_SETFONT");
		EDIT_WM_SetFont(wnd, es, (HFONT)wParam, LOWORD(lParam) != 0);
		break;

	case WM_SETTEXT:
		DPRINTF_EDIT_MSG("WM_SETTEXT");
		EDIT_WM_SetText(wnd, es, (LPCSTR)lParam);
		result = TRUE;
		break;

	case WM_SIZE:
		DPRINTF_EDIT_MSG("WM_SIZE");
		EDIT_WM_Size(wnd, es, (UINT)wParam, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_SYSKEYDOWN:
		DPRINTF_EDIT_MSG("WM_SYSKEYDOWN");
		result = EDIT_WM_SysKeyDown(wnd, es, (INT)wParam, (DWORD)lParam);
		break;

	case WM_TIMER:
		DPRINTF_EDIT_MSG("WM_TIMER");
		EDIT_WM_Timer(wnd, es, (INT)wParam, (TIMERPROC)lParam);
		break;

	case WM_VSCROLL:
		DPRINTF_EDIT_MSG("WM_VSCROLL");
		result = EDIT_WM_VScroll(wnd, es, LOWORD(wParam), SHIWORD(wParam), (HWND)(lParam));
		break;

	default:
		result = DefWindowProcA(hwnd, msg, wParam, lParam);
		break;
	}
	EDIT_UnlockBuffer(wnd, es, FALSE);
	return result;
}


/*********************************************************************
 *
 *	EDIT_BuildLineDefs_ML
 *
 *	Build linked list of text lines.
 *	Lines can end with '\0' (last line), a character (if it is wrapped),
 *	a soft return '\r\r\n' or a hard return '\r\n'
 *
 */
static void EDIT_BuildLineDefs_ML(WND *wnd, EDITSTATE *es)
{
	HDC dc;
	HFONT old_font = 0;
	LPSTR start, cp;
	INT fw;
	LINEDEF *current_def;
	LINEDEF **previous_next;

	current_def = es->first_line_def;
	do {
		LINEDEF *next_def = current_def->next;
		HeapFree(es->heap, 0, current_def);
		current_def = next_def;
	} while (current_def);
	es->line_count = 0;
	es->text_width = 0;

	dc = GetDC(wnd->hwndSelf);
	if (es->font)
		old_font = SelectObject(dc, es->font);

	fw = es->format_rect.right - es->format_rect.left;
	start = es->text;
	previous_next = &es->first_line_def;
	do {
		current_def = HeapAlloc(es->heap, 0, sizeof(LINEDEF));
		current_def->next = NULL;
		cp = start;
		while (*cp) {
			if ((*cp == '\r') && (*(cp + 1) == '\n'))
				break;
			cp++;
		}
		if (!(*cp)) {
			current_def->ending = END_0;
			current_def->net_length = lstrlenA(start);
		} else if ((cp > start) && (*(cp - 1) == '\r')) {
			current_def->ending = END_SOFT;
			current_def->net_length = cp - start - 1;
		} else {
			current_def->ending = END_HARD;
			current_def->net_length = cp - start;
		}
		current_def->width = (INT)LOWORD(GetTabbedTextExtentA(dc,
					start, current_def->net_length,
					es->tabs_count, es->tabs));
		/* FIXME: check here for lines that are too wide even in AUTOHSCROLL (> 767 ???) */
		if ((!(es->style & ES_AUTOHSCROLL)) && (current_def->width > fw)) {
			INT next = 0;
			INT prev;
			do {
				prev = next;
				next = EDIT_CallWordBreakProc(wnd, es, start - es->text,
						prev + 1, current_def->net_length, WB_RIGHT);
				current_def->width = (INT)LOWORD(GetTabbedTextExtentA(dc,
							start, next, es->tabs_count, es->tabs));
			} while (current_def->width <= fw);
			if (!prev) {
				next = 0;
				do {
					prev = next;
					next++;
					current_def->width = (INT)LOWORD(GetTabbedTextExtentA(dc,
								start, next, es->tabs_count, es->tabs));
				} while (current_def->width <= fw);
				if (!prev)
					prev = 1;
			}
			current_def->net_length = prev;
			current_def->ending = END_WRAP;
			current_def->width = (INT)LOWORD(GetTabbedTextExtentA(dc, start,
						current_def->net_length, es->tabs_count, es->tabs));
		}
		switch (current_def->ending) {
		case END_SOFT:
			current_def->length = current_def->net_length + 3;
			break;
		case END_HARD:
			current_def->length = current_def->net_length + 2;
			break;
		case END_WRAP:
		case END_0:
			current_def->length = current_def->net_length;
			break;
		}
		es->text_width = MAX(es->text_width, current_def->width);
		start += current_def->length;
		*previous_next = current_def;
		previous_next = &current_def->next;
		es->line_count++;
	} while (current_def->ending != END_0);
	if (es->font)
		SelectObject(dc, old_font);
	ReleaseDC(wnd->hwndSelf, dc);
}


/*********************************************************************
 *
 *	EDIT_CallWordBreakProc
 *
 *	Call appropriate WordBreakProc (internal or external).
 *
 *	Note: The "start" argument should always be an index refering
 *		to es->text.  The actual wordbreak proc might be
 *		16 bit, so we can't always pass any  bit LPSTR.
 *		Hence we assume that es->text is the buffer that holds
 *		the string under examination (we can decide this for ourselves).
 *
 */
static INT EDIT_CallWordBreakProc(WND *wnd, EDITSTATE *es, INT start, INT index, INT count, INT action)
{
	if (es->word_break_procA)
        {
            DPRINT("(wordbrk=%p,str='%s',idx=%d,cnt=%d,act=%d)\n",
                           es->word_break_procA, es->text + start, index,
                           count, action );
            return (INT)es->word_break_procA( es->text + start, index,
                                                  count, action );
        }
	else
            return EDIT_WordBreakProc(es->text + start, index, count, action);
}


/*********************************************************************
 *
 *	EDIT_CharFromPos
 *
 *	Beware: This is not the function called on EM_CHARFROMPOS
 *		The position _can_ be outside the formatting / client
 *		rectangle
 *		The return value is only the character index
 *
 */
static INT EDIT_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y, LPBOOL after_wrap)
{
	INT index;
	HDC dc;
	HFONT old_font = 0;

	if (es->style & ES_MULTILINE) {
		INT line = (y - es->format_rect.top) / es->line_height + es->y_offset;
		INT line_index = 0;
		LINEDEF *line_def = es->first_line_def;
		INT low, high;
		while ((line > 0) && line_def->next) {
			line_index += line_def->length;
			line_def = line_def->next;
			line--;
		}
		x += es->x_offset - es->format_rect.left;
		if (x >= line_def->width) {
			if (after_wrap)
				*after_wrap = (line_def->ending == END_WRAP);
			return line_index + line_def->net_length;
		}
		if (x <= 0) {
			if (after_wrap)
				*after_wrap = FALSE;
			return line_index;
		}
		dc = GetDC(wnd->hwndSelf);
		if (es->font)
			old_font = SelectObject(dc, es->font);
                    low = line_index + 1;
                    high = line_index + line_def->net_length + 1;
                    while (low < high - 1)
                    {
                        INT mid = (low + high) / 2;
			if (LOWORD(GetTabbedTextExtentA(dc, es->text + line_index,mid - line_index, es->tabs_count, es->tabs)) > x) high = mid;
                        else low = mid;
                    }
                    index = low;

		if (after_wrap)
			*after_wrap = ((index == line_index + line_def->net_length) &&
							(line_def->ending == END_WRAP));
	} else {
		LPSTR text;
		SIZE size;
		if (after_wrap)
			*after_wrap = FALSE;
		x -= es->format_rect.left;
		if (!x)
			return es->x_offset;
		text = EDIT_GetPasswordPointer_SL(wnd, es);
		dc = GetDC(wnd->hwndSelf);
		if (es->font)
			old_font = SelectObject(dc, es->font);
		if (x < 0)
                {
                    INT low = 0;
                    INT high = es->x_offset;
                    while (low < high - 1)
                    {
                        INT mid = (low + high) / 2;
                        GetTextExtentPointA( dc, text + mid,
                                               es->x_offset - mid, &size );
                        if (size.cx > -x) low = mid;
                        else high = mid;
                    }
                    index = low;
		}
                else
                {
                    INT low = es->x_offset;
                    INT high = lstrlenA(es->text) + 1;
                    while (low < high - 1)
                    {
                        INT mid = (low + high) / 2;
                        GetTextExtentPointA( dc, text + es->x_offset,
                                               mid - es->x_offset, &size );
                        if (size.cx > x) high = mid;
                        else low = mid;
                    }
                    index = low;
		}
		if (es->style & ES_PASSWORD)
			HeapFree(es->heap, 0 ,text);
	}
	if (es->font)
		SelectObject(dc, old_font);
	ReleaseDC(wnd->hwndSelf, dc);
	return index;
}


/*********************************************************************
 *
 *	EDIT_ConfinePoint
 *
 *	adjusts the point to be within the formatting rectangle
 *	(so CharFromPos returns the nearest _visible_ character)
 *
 */
static void EDIT_ConfinePoint(WND *wnd, EDITSTATE *es, LPINT x, LPINT y)
{
	*x = MIN(MAX(*x, es->format_rect.left), es->format_rect.right - 1);
	*y = MIN(MAX(*y, es->format_rect.top), es->format_rect.bottom - 1);
}


/*********************************************************************
 *
 *	EDIT_GetLineRect
 *
 *	Calculates the bounding rectangle for a line from a starting
 *	column to an ending column.
 *
 */
static void EDIT_GetLineRect(WND *wnd, EDITSTATE *es, INT line, INT scol, INT ecol, LPRECT rc)
{
	INT line_index =  EDIT_EM_LineIndex(wnd, es, line);

	if (es->style & ES_MULTILINE)
		rc->top = es->format_rect.top + (line - es->y_offset) * es->line_height;
	else
		rc->top = es->format_rect.top;
	rc->bottom = rc->top + es->line_height;
	rc->left = (scol == 0) ? es->format_rect.left : SLOWORD(EDIT_EM_PosFromChar(wnd, es, line_index + scol, TRUE));
	rc->right = (ecol == -1) ? es->format_rect.right : SLOWORD(EDIT_EM_PosFromChar(wnd, es, line_index + ecol, TRUE));
}


/*********************************************************************
 *
 *	EDIT_GetPasswordPointer_SL
 *
 *	note: caller should free the (optionally) allocated buffer
 *
 */
static LPSTR EDIT_GetPasswordPointer_SL(WND *wnd, EDITSTATE *es)
{
	if (es->style & ES_PASSWORD) {
		INT len = lstrlenA(es->text);
		LPSTR text = HeapAlloc(es->heap, 0, len + 1);
		HEAP_memset(text, len, es->password_char);
		text[len] = '\0';
		return text;
	} else
		return es->text;
}


/*********************************************************************
 *
 *	EDIT_LockBuffer
 *
 *	This acts as a LOCAL_Lock(), but it locks only once.  This way
 *	you can call it whenever you like, without unlocking.
 *
 */
static void EDIT_LockBuffer(WND *wnd, EDITSTATE *es)
{
	if (!es) {
		DPRINT( "no EDITSTATE ... please report\n");
		return;
	}
	if (!(es->style & ES_MULTILINE))
		return;
	if (!es->text) {
		if (es->hloc)
			es->text = LocalLock(es->hloc);
		else {
			DPRINT( "no buffer ... please report\n");
			return;
		}
	}
	es->lock_count++;
}


/*********************************************************************
 *
 *	EDIT_SL_InvalidateText
 *
 *	Called from EDIT_InvalidateText().
 *	Does the job for single-line controls only.
 *
 */
static void EDIT_SL_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end)
{
	RECT line_rect;
	RECT rc;

	EDIT_GetLineRect(wnd, es, 0, start, end, &line_rect);
	if (IntersectRect(&rc, &line_rect, &es->format_rect))
		InvalidateRect(wnd->hwndSelf, &rc, FALSE);
}


/*********************************************************************
 *
 *	EDIT_ML_InvalidateText
 *
 *	Called from EDIT_InvalidateText().
 *	Does the job for multi-line controls only.
 *
 */
static void EDIT_ML_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end)
{
	INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
	INT sl = EDIT_EM_LineFromChar(wnd, es, start);
	INT el = EDIT_EM_LineFromChar(wnd, es, end);
	INT sc;
	INT ec;
	RECT rc1;
	RECT rcWnd;
	RECT rcLine;
	RECT rcUpdate;
	INT l;

	if ((el < es->y_offset) || (sl > es->y_offset + vlc))
		return;

	sc = start - EDIT_EM_LineIndex(wnd, es, sl);
	ec = end - EDIT_EM_LineIndex(wnd, es, el);
	if (sl < es->y_offset) {
		sl = es->y_offset;
		sc = 0;
	}
	if (el > es->y_offset + vlc) {
		el = es->y_offset + vlc;
		ec = EDIT_EM_LineLength(wnd, es, EDIT_EM_LineIndex(wnd, es, el));
	}
	GetClientRect(wnd->hwndSelf, &rc1);
	IntersectRect(&rcWnd, &rc1, &es->format_rect);
	if (sl == el) {
		EDIT_GetLineRect(wnd, es, sl, sc, ec, &rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect(wnd->hwndSelf, &rcUpdate, FALSE);
	} else {
		EDIT_GetLineRect(wnd, es, sl, sc,
				EDIT_EM_LineLength(wnd, es,
					EDIT_EM_LineIndex(wnd, es, sl)),
				&rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect(wnd->hwndSelf, &rcUpdate, FALSE);
		for (l = sl + 1 ; l < el ; l++) {
			EDIT_GetLineRect(wnd, es, l, 0,
				EDIT_EM_LineLength(wnd, es,
					EDIT_EM_LineIndex(wnd, es, l)),
				&rcLine);
			if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
				InvalidateRect(wnd->hwndSelf, &rcUpdate, FALSE);
		}
		EDIT_GetLineRect(wnd, es, el, 0, ec, &rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect(wnd->hwndSelf, &rcUpdate, FALSE);
	}
}


/*********************************************************************
 *
 *	EDIT_InvalidateText
 *
 *	Invalidate the text from offset start upto, but not including,
 *	offset end.  Useful for (re)painting the selection.
 *	Regions outside the linewidth are not invalidated.
 *	end == -1 means end == TextLength.
 *	start and end need not be ordered.
 *
 */
static void EDIT_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end)
{
	if (end == start)
		return;

	if (end == -1)
		end = lstrlenA(es->text);

	ORDER_INT(start, end);

	if (es->style & ES_MULTILINE)
		EDIT_ML_InvalidateText(wnd, es, start, end);
	else
		EDIT_SL_InvalidateText(wnd, es, start, end);
}


/*********************************************************************
 *
 *	EDIT_MakeFit
 *
 *	Try to fit size + 1 bytes in the buffer.  Constrain to limits.
 *
 */
static BOOL EDIT_MakeFit(WND *wnd, EDITSTATE *es, INT size)
{
	HLOCAL hNew;

	if (size <= es->buffer_size)
		return TRUE;
	if (size > es->buffer_limit) {
		EDIT_NOTIFY_PARENT(wnd, EN_MAXTEXT, "EN_MAXTEXT");
		return FALSE;
	}
	size = ((size / GROWLENGTH) + 1) * GROWLENGTH;
	if (size > es->buffer_limit)
		size = es->buffer_limit;

	DPRINT( "trying to ReAlloc to %d+1\n", size);

	EDIT_UnlockBuffer(wnd, es, TRUE);
	if (es->text) {
		if ((es->text = HeapReAlloc(es->heap, 0, es->text, size + 1)))
			es->buffer_size = MIN(HeapSize(es->heap, 0, es->text) - 1, es->buffer_limit);
		else
			es->buffer_size = 0;
	} else if (es->hloc) {
		if ((hNew = LocalReAlloc(es->hloc, size + 1, 0))) {
			DPRINT( "Old  bit handle %08x, new handle %08x\n", es->hloc, hNew);
			es->hloc = hNew;
			es->buffer_size = MIN(LocalSize(es->hloc) - 1, es->buffer_limit);
		}
	}
	if (es->buffer_size < size) {
		EDIT_LockBuffer(wnd, es);
		DPRINT( "FAILED !  We now have %d+1\n", es->buffer_size);
		EDIT_NOTIFY_PARENT(wnd, EN_ERRSPACE, "EN_ERRSPACE");
		return FALSE;
	} else {
		EDIT_LockBuffer(wnd, es);
		DPRINT( "We now have %d+1\n", es->buffer_size);
		return TRUE;
	}
}


/*********************************************************************
 *
 *	EDIT_MakeUndoFit
 *
 *	Try to fit size + 1 bytes in the undo buffer.
 *
 */
static BOOL EDIT_MakeUndoFit(WND *wnd, EDITSTATE *es, INT size)
{
	if (size <= es->undo_buffer_size)
		return TRUE;
	size = ((size / GROWLENGTH) + 1) * GROWLENGTH;

	DPRINT( "trying to ReAlloc to %d+1\n", size);

	if ((es->undo_text = HeapReAlloc(es->heap, 0, es->undo_text, size + 1))) {
		es->undo_buffer_size = HeapSize(es->heap, 0, es->undo_text) - 1;
		if (es->undo_buffer_size < size) {
			DPRINT( "FAILED !  We now have %d+1\n", es->undo_buffer_size);
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}


/*********************************************************************
 *
 *	EDIT_MoveBackward
 *
 */
static void EDIT_MoveBackward(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT e = es->selection_end;

	if (e) {
		e--;
		if ((es->style & ES_MULTILINE) && e &&
				(es->text[e - 1] == '\r') && (es->text[e] == '\n')) {
			e--;
			if (e && (es->text[e - 1] == '\r'))
				e--;
		}
	}
	EDIT_EM_SetSel(wnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveDown_ML
 *
 *	Only for multi line controls
 *	Move the caret one line down, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */
static void EDIT_MoveDown_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x, y + es->line_height, &after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveEnd
 *
 */
static void EDIT_MoveEnd(WND *wnd, EDITSTATE *es, BOOL extend)
{
	BOOL after_wrap = FALSE;
	INT e;

	if (es->style & ES_MULTILINE)
		e = EDIT_CharFromPos(wnd, es, 0x7fffffff,
			HIWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, es->flags & EF_AFTER_WRAP)), &after_wrap);
	else
		e = lstrlenA(es->text);
	EDIT_EM_SetSel(wnd, es, extend ? es->selection_start : e, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveForward
 *
 */
static void EDIT_MoveForward(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT e = es->selection_end;

	if (es->text[e]) {
		e++;
		if ((es->style & ES_MULTILINE) && (es->text[e - 1] == '\r')) {
			if (es->text[e] == '\n')
				e++;
			else if ((es->text[e] == '\r') && (es->text[e + 1] == '\n'))
				e += 2;
		}
	}
	EDIT_EM_SetSel(wnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveHome
 *
 *	Home key: move to beginning of line.
 *
 */
static void EDIT_MoveHome(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT e;

	if (es->style & ES_MULTILINE)
		e = EDIT_CharFromPos(wnd, es, 0x80000000,
			HIWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, es->flags & EF_AFTER_WRAP)), NULL);
	else
		e = 0;
	EDIT_EM_SetSel(wnd, es, e, extend ? es->selection_start : e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MovePageDown_ML
 *
 *	Only for multi line controls
 *	Move the caret one page down, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */
static void EDIT_MovePageDown_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x,
		y + (es->format_rect.bottom - es->format_rect.top),
		&after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MovePageUp_ML
 *
 *	Only for multi line controls
 *	Move the caret one page up, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */
static void EDIT_MovePageUp_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x,
		y - (es->format_rect.bottom - es->format_rect.top),
		&after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveUp_ML
 *
 *	Only for multi line controls
 *	Move the caret one line up, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */ 
static void EDIT_MoveUp_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x, y - es->line_height, &after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveWordBackward
 *
 */
static void EDIT_MoveWordBackward(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	INT l;
	INT ll;
	INT li;

	l = EDIT_EM_LineFromChar(wnd, es, e);
	ll = EDIT_EM_LineLength(wnd, es, e);
	li = EDIT_EM_LineIndex(wnd, es, l);
	if (e - li == 0) {
		if (l) {
			li = EDIT_EM_LineIndex(wnd, es, l - 1);
			e = li + EDIT_EM_LineLength(wnd, es, li);
		}
	} else {
		e = li + (INT)EDIT_CallWordBreakProc(wnd, es,
				li, e - li, ll, WB_LEFT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveWordForward
 *
 */
static void EDIT_MoveWordForward(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	INT l;
	INT ll;
	INT li;

	l = EDIT_EM_LineFromChar(wnd, es, e);
	ll = EDIT_EM_LineLength(wnd, es, e);
	li = EDIT_EM_LineIndex(wnd, es, l);
	if (e - li == ll) {
		if ((es->style & ES_MULTILINE) && (l != es->line_count - 1))
			e = EDIT_EM_LineIndex(wnd, es, l + 1);
	} else {
		e = li + EDIT_CallWordBreakProc(wnd, es,
				li, e - li + 1, ll, WB_RIGHT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_PaintLine
 *
 */
static void EDIT_PaintLine(WND *wnd, EDITSTATE *es, HDC dc, INT line, BOOL rev)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	INT li;
	INT ll;
	INT x;
	INT y;
	LRESULT pos;

	if (es->style & ES_MULTILINE) {
		INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		if ((line < es->y_offset) || (line > es->y_offset + vlc) || (line >= es->line_count))
			return;
	} else if (line)
		return;

	DPRINT( "line=%d\n", line);

	pos = EDIT_EM_PosFromChar(wnd, es, EDIT_EM_LineIndex(wnd, es, line), FALSE);
	x = SLOWORD(pos);
	y = SHIWORD(pos);
	li = EDIT_EM_LineIndex(wnd, es, line);
	ll = EDIT_EM_LineLength(wnd, es, li);
	s = es->selection_start;
	e = es->selection_end;
	ORDER_INT(s, e);
	s = MIN(li + ll, MAX(li, s));
	e = MIN(li + ll, MAX(li, e));
	if (rev && (s != e) &&
			((es->flags & EF_FOCUSED) || (es->style & ES_NOHIDESEL))) {
		x += EDIT_PaintText(wnd, es, dc, x, y, line, 0, s - li, FALSE);
		x += EDIT_PaintText(wnd, es, dc, x, y, line, s - li, e - s, TRUE);
		x += EDIT_PaintText(wnd, es, dc, x, y, line, e - li, li + ll - e, FALSE);
	} else
		x += EDIT_PaintText(wnd, es, dc, x, y, line, 0, ll, FALSE);
}


/*********************************************************************
 *
 *	EDIT_PaintText
 *
 */
static INT EDIT_PaintText(WND *wnd, EDITSTATE *es, HDC dc, INT x, INT y, INT line, INT col, INT count, BOOL rev)
{
	COLORREF BkColor;
	COLORREF TextColor;
	INT ret;
	INT li;
	SIZE size;

	if (!count)
		return 0;
	BkColor = GetBkColor(dc);
	TextColor = GetTextColor(dc);
	if (rev) {
		SetBkColor(dc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	li = EDIT_EM_LineIndex(wnd, es, line);
	if (es->style & ES_MULTILINE) {
		ret = (INT)LOWORD(TabbedTextOutA(dc, x, y, es->text + li + col, count,
					es->tabs_count, es->tabs, es->format_rect.left - es->x_offset));
	} else {
		LPSTR text = EDIT_GetPasswordPointer_SL(wnd, es);
		TextOutA(dc, x, y, text + li + col, count);
		GetTextExtentPointA(dc, text + li + col, count, &size);
		ret = size.cx;
		if (es->style & ES_PASSWORD)
			HeapFree(es->heap, 0, text);
	}
	if (rev) {
		SetBkColor(dc, BkColor);
		SetTextColor(dc, TextColor);
	}
	return ret;
}


/*********************************************************************
 *
 *	EDIT_SetCaretPos
 *
 */
static void EDIT_SetCaretPos(WND *wnd, EDITSTATE *es, INT pos,
			     BOOL after_wrap)
{
	LRESULT res = EDIT_EM_PosFromChar(wnd, es, pos, after_wrap);
	INT x = SLOWORD(res);
	INT y = SHIWORD(res);

	if(x < es->format_rect.left)
		x = es->format_rect.left;
	if(x > es->format_rect.right - 2)
		x = es->format_rect.right - 2;
	if(y > es->format_rect.bottom)
		y = es->format_rect.bottom;
	if(y < es->format_rect.top)
		y = es->format_rect.top;
	SetCaretPos(x, y);
	return;
}


/*********************************************************************
 *
 *	EDIT_SetRectNP
 *
 *	note:	this is not (exactly) the handler called on EM_SETRECTNP
 *		it is also used to set the rect of a single line control
 *
 */
static void EDIT_SetRectNP(WND *wnd, EDITSTATE *es, LPRECT rc)
{
	CopyRect(&es->format_rect, rc);
	if (es->style & WS_BORDER) {
		INT bw = GetSystemMetrics(SM_CXBORDER) + 1;
		if(TWEAK_WineLook == WIN31_LOOK)
			bw += 2;
		es->format_rect.left += bw;
		es->format_rect.top += bw;
		es->format_rect.right -= bw;
		es->format_rect.bottom -= bw;
	}
	es->format_rect.left += es->left_margin;
	es->format_rect.right -= es->right_margin;
	es->format_rect.right = MAX(es->format_rect.right, es->format_rect.left + es->char_width);
	if (es->style & ES_MULTILINE)
		es->format_rect.bottom = es->format_rect.top +
			MAX(1, (es->format_rect.bottom - es->format_rect.top) / es->line_height) * es->line_height;
	else
		es->format_rect.bottom = es->format_rect.top + es->line_height;
	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL))
		EDIT_BuildLineDefs_ML(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_UnlockBuffer
 *
 */
static void EDIT_UnlockBuffer(WND *wnd, EDITSTATE *es, BOOL force)
{
	if (!es) {
		DPRINT( "no EDITSTATE ... please report\n");
		return;
	}
	if (!(es->style & ES_MULTILINE))
		return;
	if (!es->lock_count) {
		DPRINT( "lock_count == 0 ... please report\n");
		return;
	}
	if (!es->text) {
		DPRINT( "es->text == 0 ... please report\n");
		return;
	}
	if (force || (es->lock_count == 1)) {
		if (es->hloc) {
			LocalUnlock(es->hloc);
			es->text = NULL;
		}
	}
	es->lock_count--;
}


/*********************************************************************
 *
 *	EDIT_WordBreakProc
 *
 *	Find the beginning of words.
 *	Note:	unlike the specs for a WordBreakProc, this function only
 *		allows to be called without linebreaks between s[0] upto
 *		s[count - 1].  Remember it is only called
 *		internally, so we can decide this for ourselves.
 *
 */
static INT EDIT_WordBreakProc(LPSTR s, INT index, INT count, INT action)
{
	INT ret = 0;

	DPRINT( "s=%p, index=%u, count=%u, action=%d\n", 
		     s, index, count, action);

	switch (action) {
	case WB_LEFT:
		if (!count)
			break;
		if (index)
			index--;
		if (s[index] == ' ') {
			while (index && (s[index] == ' '))
				index--;
			if (index) {
				while (index && (s[index] != ' '))
					index--;
				if (s[index] == ' ')
					index++;
			}
		} else {
			while (index && (s[index] != ' '))
				index--;
			if (s[index] == ' ')
				index++;
		}
		ret = index;
		break;
	case WB_RIGHT:
		if (!count)
			break;
		if (index)
			index--;
		if (s[index] == ' ')
			while ((index < count) && (s[index] == ' ')) index++;
		else {
			while (s[index] && (s[index] != ' ') && (index < count))
				index++;
			while ((s[index] == ' ') && (index < count)) index++;
		}
		ret = index;
		break;
	case WB_ISDELIMITER:
		ret = (s[index] == ' ');
		break;
	default:
		DPRINT( "unknown action code, please report !\n");
		break;
	}
	return ret;
}


/*********************************************************************
 *
 *	EM_CHARFROMPOS
 *
 *	FIXME: do the specs mean to return LineIndex or LineNumber ???
 *		Let's assume LineIndex is meant
 *	FIXME: do the specs mean to return -1 if outside client area or
 *		if outside formatting rectangle ???
 *
 */
static LRESULT EDIT_EM_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y)
{
	POINT pt;
	RECT rc;
	INT index;

	pt.x = x;
	pt.y = y;
	GetClientRect(wnd->hwndSelf, &rc);
	if (!PtInRect(&rc, pt))
		return -1;

	index = EDIT_CharFromPos(wnd, es, x, y, NULL);
	return MAKELONG(index, EDIT_EM_LineIndex(wnd, es,
			EDIT_EM_LineFromChar(wnd, es, index)));
}


/*********************************************************************
 *
 *	EM_FMTLINES
 *
 * Enable or disable soft breaks.
 */
static BOOL EDIT_EM_FmtLines(WND *wnd, EDITSTATE *es, BOOL add_eol)
{
	es->flags &= ~EF_USE_SOFTBRK;
	if (add_eol) {
		es->flags |= EF_USE_SOFTBRK;
		DPRINT( "soft break enabled, not implemented\n");
	}
	return add_eol;
}


/*********************************************************************
 *
 *	EM_GETHANDLE
 *
 *	Hopefully this won't fire back at us.
 *	We always start with a fixed buffer in our own heap.
 *	However, with this message a  bit application requests
 *	a handle to  bit moveable local heap memory, where it expects
 *	to find the text.
 *	It's a pity that from this moment on we have to use this
 *	local heap, because applications may rely on the handle
 *	in the future.
 *
 *	In this function we'll try to switch to local heap.
 *
 */
static HLOCAL EDIT_EM_GetHandle(WND *wnd, EDITSTATE *es)
{
	HLOCAL newBuf;
	LPSTR newText;
	INT newSize;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if (es->hloc)
		return es->hloc;
	

	if (!(newBuf = LocalAlloc(LMEM_MOVEABLE, lstrlenA(es->text) + 1))) {
		DPRINT( "could not allocate new  bit buffer\n");
		return 0;
	}
	newSize = MIN(LocalSize(newBuf) - 1, es->buffer_limit);
	if (!(newText = LocalLock(newBuf))) {
		DPRINT( "could not lock new  bit buffer\n");
		LocalFree(newBuf);
		return 0;
	}
	lstrcpyA(newText, es->text);
	EDIT_UnlockBuffer(wnd, es, TRUE);
	if (es->text)
		HeapFree(es->heap, 0, es->text);
	es->hloc = newBuf;
	es->buffer_size = newSize;
	es->text = newText;
	EDIT_LockBuffer(wnd, es);
	DPRINT( "switched to  bit local heap\n");

	return es->hloc;
}




/*********************************************************************
 *
 *	EM_GETLINE
 *
 */
static INT EDIT_EM_GetLine(WND *wnd, EDITSTATE *es, INT line, LPSTR lpch)
{
	LPSTR src;
	INT len;
	INT i;

	if (es->style & ES_MULTILINE) {
		if (line >= es->line_count)
			return 0;
	} else
		line = 0;
	i = EDIT_EM_LineIndex(wnd, es, line);
	src = es->text + i;
	len = MIN(*(WORD *)lpch, EDIT_EM_LineLength(wnd, es, i));
	for (i = 0 ; i < len ; i++) {
		*lpch = *src;
		src++;
		lpch++;
	}
	return (LRESULT)len;
}


/*********************************************************************
 *
 *	EM_GETSEL
 *
 */
static LRESULT EDIT_EM_GetSel(WND *wnd, EDITSTATE *es, UINT * start, UINT * end)
{
	UINT s = es->selection_start;
	UINT e = es->selection_end;

	ORDER_UINT(s, e);
	if (start)
		*start = s;
	if (end)
		*end = e;
	return MAKELONG(s, e);
}


/*********************************************************************
 *
 *	EM_GETTHUMB
 *
 *	FIXME: is this right ?  (or should it be only VSCROLL)
 *	(and maybe only for edit controls that really have their
 *	own scrollbars) (and maybe only for multiline controls ?)
 *	All in all: very poorly documented
 *
 *	FIXME: now it's also broken, because of the new WM_HSCROLL /
 *		WM_VSCROLL handlers
 *
 */
static LRESULT EDIT_EM_GetThumb(WND *wnd, EDITSTATE *es)
{
	return MAKELONG(EDIT_WM_VScroll(wnd, es, EM_GETTHUMB, 0, 0),
		EDIT_WM_HScroll(wnd, es, EM_GETTHUMB, 0, 0));
}


/*********************************************************************
 *
 *	EM_LINEFROMCHAR
 *
 */
static INT EDIT_EM_LineFromChar(WND *wnd, EDITSTATE *es, INT index)
{
	INT line;
	LINEDEF *line_def;

	if (!(es->style & ES_MULTILINE))
		return 0;
	if (index > lstrlenA(es->text))
		return es->line_count - 1;
	if (index == -1)
		index = MIN(es->selection_start, es->selection_end);

	line = 0;
	line_def = es->first_line_def;
	index -= line_def->length;
	while ((index >= 0) && line_def->next) {
		line++;
		line_def = line_def->next;
		index -= line_def->length;
	}
	return line;
}


/*********************************************************************
 *
 *	EM_LINEINDEX
 *
 */
static INT EDIT_EM_LineIndex(WND *wnd, EDITSTATE *es, INT line)
{
	INT line_index;
	LINEDEF *line_def;

	if (!(es->style & ES_MULTILINE))
		return 0;
	if (line >= es->line_count)
		return -1;

	line_index = 0;
	line_def = es->first_line_def;
	if (line == -1) {
		INT index = es->selection_end - line_def->length;
		while ((index >= 0) && line_def->next) {
			line_index += line_def->length;
			line_def = line_def->next;
			index -= line_def->length;
		}
	} else {
		while (line > 0) {
			line_index += line_def->length;
			line_def = line_def->next;
			line--;
		}
	}
	return line_index;
}


/*********************************************************************
 *
 *	EM_LINELENGTH
 *
 */
static INT EDIT_EM_LineLength(WND *wnd, EDITSTATE *es, INT index)
{
	LINEDEF *line_def;

	if (!(es->style & ES_MULTILINE))
		return lstrlenA(es->text);

	if (index == -1) {
		/* FIXME: broken
		INT sl = EDIT_EM_LineFromChar(wnd, es, es->selection_start);
		INT el = EDIT_EM_LineFromChar(wnd, es, es->selection_end);
		return es->selection_start - es->line_defs[sl].offset +
				es->line_defs[el].offset +
				es->line_defs[el].length - es->selection_end;
		*/
		return 0;
	}
	line_def = es->first_line_def;
	index -= line_def->length;
	while ((index >= 0) && line_def->next) {
		line_def = line_def->next;
		index -= line_def->length;
	}
	return line_def->net_length;
}


/*********************************************************************
 *
 *	EM_LINESCROLL
 *
 *	FIXME: dx is in average character widths
 *		However, we assume it is in pixels when we use this
 *		function internally
 *
 */
static BOOL EDIT_EM_LineScroll(WND *wnd, EDITSTATE *es, INT dx, INT dy)
{
	INT nyoff;

	if (!(es->style & ES_MULTILINE))
		return FALSE;

	if (-dx > es->x_offset)
		dx = -es->x_offset;
	if (dx > es->text_width - es->x_offset)
		dx = es->text_width - es->x_offset;
	nyoff = MAX(0, es->y_offset + dy);
	if (nyoff >= es->line_count)
		nyoff = es->line_count - 1;
	dy = (es->y_offset - nyoff) * es->line_height;
	if (dx || dy) {
		RECT rc1;
		RECT rc;
		GetClientRect(wnd->hwndSelf, &rc1);
		IntersectRect(&rc, &rc1, &es->format_rect);
		ScrollWindowEx(wnd->hwndSelf, -dx, dy,
				NULL, &rc, (HRGN)NULL, NULL, SW_INVALIDATE);
		es->y_offset = nyoff;
		es->x_offset += dx;
	}
	if (dx && !(es->flags & EF_HSCROLL_TRACK))
		EDIT_NOTIFY_PARENT(wnd, EN_HSCROLL, "EN_HSCROLL");
	if (dy && !(es->flags & EF_VSCROLL_TRACK))
		EDIT_NOTIFY_PARENT(wnd, EN_VSCROLL, "EN_VSCROLL");
	return TRUE;
}


/*********************************************************************
 *
 *	EM_POSFROMCHAR
 *
 */
static LRESULT EDIT_EM_PosFromChar(WND *wnd, EDITSTATE *es, INT index, BOOL after_wrap)
{
	INT len = lstrlenA(es->text);
	INT l;
	INT li;
	INT x;
	INT y = 0;
	HDC dc;
	HFONT old_font = 0;
	SIZE size;

	index = MIN(index, len);
	dc = GetDC(wnd->hwndSelf);
	if (es->font)
		old_font = SelectObject(dc, es->font);
	if (es->style & ES_MULTILINE) {
		l = EDIT_EM_LineFromChar(wnd, es, index);
		y = (l - es->y_offset) * es->line_height;
		li = EDIT_EM_LineIndex(wnd, es, l);
		if (after_wrap && (li == index) && l) {
			INT l2 = l - 1;
			LINEDEF *line_def = es->first_line_def;
			while (l2) {
				line_def = line_def->next;
				l2--;
			}
			if (line_def->ending == END_WRAP) {
				l--;
				y -= es->line_height;
				li = EDIT_EM_LineIndex(wnd, es, l);
			}
		}
		x = LOWORD(GetTabbedTextExtentA(dc, es->text + li, index - li,
				es->tabs_count, es->tabs)) - es->x_offset;
	} else {
		LPSTR text = EDIT_GetPasswordPointer_SL(wnd, es);
		if (index < es->x_offset) {
			GetTextExtentPointA(dc, text + index,
					es->x_offset - index, &size);
			x = -size.cx;
		} else {
			GetTextExtentPointA(dc, text + es->x_offset,
					index - es->x_offset, &size);
			 x = size.cx;
		}
		y = 0;
		if (es->style & ES_PASSWORD)
			HeapFree(es->heap, 0 ,text);
	}
	x += es->format_rect.left;
	y += es->format_rect.top;
	if (es->font)
		SelectObject(dc, old_font);
	ReleaseDC(wnd->hwndSelf, dc);
	return MAKELONG((INT)x, (INT)y);
}


/*********************************************************************
 *
 *	EM_REPLACESEL
 *
 *	FIXME: handle ES_NUMBER and ES_OEMCONVERT here
 *
 */
static void EDIT_EM_ReplaceSel(WND *wnd, EDITSTATE *es, BOOL can_undo, LPCSTR lpsz_replace)
{
	INT strl = lstrlenA(lpsz_replace);
	INT tl = lstrlenA(es->text);
	INT utl;
	UINT s;
	UINT e;
	INT i;
	LPSTR p;

	s = es->selection_start;
	e = es->selection_end;

	if ((s == e) && !strl)
		return;

	ORDER_UINT(s, e);

	if (!EDIT_MakeFit(wnd, es, tl - (e - s) + strl))
		return;

	if (e != s) {
		/* there is something to be deleted */
		if (can_undo) {
			utl = lstrlenA(es->undo_text);
			if (!es->undo_insert_count && (*es->undo_text && (s == es->undo_position))) {
				/* undo-buffer is extended to the right */
				EDIT_MakeUndoFit(wnd, es, utl + e - s);
				lstrcpynA(es->undo_text + utl, es->text + s, e - s + 1);
			} else if (!es->undo_insert_count && (*es->undo_text && (e == es->undo_position))) {
				/* undo-buffer is extended to the left */
				EDIT_MakeUndoFit(wnd, es, utl + e - s);
				for (p = es->undo_text + utl ; p >= es->undo_text ; p--)
					p[e - s] = p[0];
				for (i = 0 , p = es->undo_text ; i < e - s ; i++)
					p[i] = (es->text + s)[i];
				es->undo_position = s;
			} else {
				/* new undo-buffer */
				EDIT_MakeUndoFit(wnd, es, e - s);
				lstrcpynA(es->undo_text, es->text + s, e - s + 1);
				es->undo_position = s;
			}
			/* any deletion makes the old insertion-undo invalid */
			es->undo_insert_count = 0;
		} else
			EDIT_EM_EmptyUndoBuffer(wnd, es);

		/* now delete */
		lstrcpyA(es->text + s, es->text + e);
	}
	if (strl) {
		/* there is an insertion */
		if (can_undo) {
			if ((s == es->undo_position) ||
					((es->undo_insert_count) &&
					(s == es->undo_position + es->undo_insert_count)))
				/*
				 * insertion is new and at delete position or
				 * an extension to either left or right
				 */
				es->undo_insert_count += strl;
			else {
				/* new insertion undo */
				es->undo_position = s;
				es->undo_insert_count = strl;
				/* new insertion makes old delete-buffer invalid */
				*es->undo_text = '\0';
			}
		} else
			EDIT_EM_EmptyUndoBuffer(wnd, es);

		/* now insert */
		tl = lstrlenA(es->text);
		for (p = es->text + tl ; p >= es->text + s ; p--)
			p[strl] = p[0];
		for (i = 0 , p = es->text + s ; i < strl ; i++)
			p[i] = lpsz_replace[i];
		if(es->style & ES_UPPERCASE)
			CharUpperBuffA(p, strl);
		else if(es->style & ES_LOWERCASE)
			CharLowerBuffA(p, strl);
		s += strl;
	}
	/* FIXME: really inefficient */
	if (es->style & ES_MULTILINE)
		EDIT_BuildLineDefs_ML(wnd, es);

	EDIT_EM_SetSel(wnd, es, s, s, FALSE);
	es->flags |= EF_MODIFIED;
	es->flags |= EF_UPDATE;
	EDIT_EM_ScrollCaret(wnd, es);

	/* FIXME: really inefficient */
	InvalidateRect(wnd->hwndSelf, NULL, TRUE);
}


/*********************************************************************
 *
 *	EM_SCROLL
 *
 */
static LRESULT EDIT_EM_Scroll(WND *wnd, EDITSTATE *es, INT action)
{
	INT dy;

	if (!(es->style & ES_MULTILINE))
		return (LRESULT)FALSE;

	dy = 0;

	switch (action) {
	case SB_LINEUP:
		if (es->y_offset)
			dy = -1;
		break;
	case SB_LINEDOWN:
		if (es->y_offset < es->line_count - 1)
			dy = 1;
		break;
	case SB_PAGEUP:
		if (es->y_offset)
			dy = -(es->format_rect.bottom - es->format_rect.top) / es->line_height;
		break;
	case SB_PAGEDOWN:
		if (es->y_offset < es->line_count - 1)
			dy = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		break;
	default:
		return (LRESULT)FALSE;
	}
	if (dy) {
		EDIT_EM_LineScroll(wnd, es, 0, dy);
		EDIT_NOTIFY_PARENT(wnd, EN_VSCROLL, "EN_VSCROLL");
	}
	return MAKELONG((INT)dy, (BOOL)TRUE);
}


/*********************************************************************
 *
 *	EM_SCROLLCARET
 *
 */
static void EDIT_EM_ScrollCaret(WND *wnd, EDITSTATE *es)
{
	if (es->style & ES_MULTILINE) {
		INT l;
		INT li;
		INT vlc;
		INT ww;
		INT cw = es->char_width;
		INT x;
		INT dy = 0;
		INT dx = 0;

		l = EDIT_EM_LineFromChar(wnd, es, es->selection_end);
		li = EDIT_EM_LineIndex(wnd, es, l);
		x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, es->flags & EF_AFTER_WRAP));
		vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		if (l >= es->y_offset + vlc)
			dy = l - vlc + 1 - es->y_offset;
		if (l < es->y_offset)
			dy = l - es->y_offset;
		ww = es->format_rect.right - es->format_rect.left;
		if (x < es->format_rect.left)
			dx = x - es->format_rect.left - ww / HSCROLL_FRACTION / cw * cw;
		if (x > es->format_rect.right)
			dx = x - es->format_rect.left - (HSCROLL_FRACTION - 1) * ww / HSCROLL_FRACTION / cw * cw;
		if (dy || dx)
			EDIT_EM_LineScroll(wnd, es, dx, dy);
	} else {
		INT x;
		INT goal;
		INT format_width;

		if (!(es->style & ES_AUTOHSCROLL))
			return;

		x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, FALSE));
		format_width = es->format_rect.right - es->format_rect.left;
		if (x < es->format_rect.left) {
			goal = es->format_rect.left + format_width / HSCROLL_FRACTION;
			do {
				es->x_offset--;
				x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, FALSE));
			} while ((x < goal) && es->x_offset);
			/* FIXME: use ScrollWindow() somehow to improve performance */
			InvalidateRect(wnd->hwndSelf, NULL, TRUE);
		} else if (x > es->format_rect.right) {
			INT x_last;
			INT len = lstrlenA(es->text);
			goal = es->format_rect.right - format_width / HSCROLL_FRACTION;
			do {
				es->x_offset++;
				x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, FALSE));
				x_last = SLOWORD(EDIT_EM_PosFromChar(wnd, es, len, FALSE));
			} while ((x > goal) && (x_last > es->format_rect.right));
			/* FIXME: use ScrollWindow() somehow to improve performance */
			InvalidateRect(wnd->hwndSelf, NULL, TRUE);
		}
	}
}


/*********************************************************************
 *
 *	EM_SETHANDLE
 *
 *	FIXME:	ES_LOWERCASE, ES_UPPERCASE, ES_OEMCONVERT, ES_NUMBER ???
 *
 */
static void EDIT_EM_SetHandle(WND *wnd, EDITSTATE *es, HLOCAL hloc)
{
	if (!(es->style & ES_MULTILINE))
		return;

	if (!hloc) {
		DPRINT( "called with NULL handle\n");
		return;
	}

	EDIT_UnlockBuffer(wnd, es, TRUE);
	/*
	 *	old buffer is freed by caller, unless
	 *	it is still in our own heap.  (in that case
	 *	we free it, correcting the buggy caller.)
	 */
	if (es->text)
		HeapFree(es->heap, 0, es->text);

	es->hloc = hloc;
	es->text = NULL;
	es->buffer_size = LocalSize(es->hloc) - 1;
	EDIT_LockBuffer(wnd, es);

	es->x_offset = es->y_offset = 0;
	es->selection_start = es->selection_end = 0;
	EDIT_EM_EmptyUndoBuffer(wnd, es);
	es->flags &= ~EF_MODIFIED;
	es->flags &= ~EF_UPDATE;
	EDIT_BuildLineDefs_ML(wnd, es);
	InvalidateRect(wnd->hwndSelf, NULL, TRUE);
	EDIT_EM_ScrollCaret(wnd, es);
}





/*********************************************************************
 *
 *	EM_SETLIMITTEXT
 *
 *	FIXME: in WinNT maxsize is 0x7FFFFFFF / 0xFFFFFFFF
 *	However, the windows version is not complied to yet in all of edit.c
 *
 */
static void EDIT_EM_SetLimitText(WND *wnd, EDITSTATE *es, INT limit)
{
	if (es->style & ES_MULTILINE) {
		if (limit)
			es->buffer_limit = MIN(limit, BUFLIMIT_MULTI);
		else
			es->buffer_limit = BUFLIMIT_MULTI;
	} else {
		if (limit)
			es->buffer_limit = MIN(limit, BUFLIMIT_SINGLE);
		else
			es->buffer_limit = BUFLIMIT_SINGLE;
	}
}


/*********************************************************************
 *
 *	EM_SETMARGINS
 * 
 * EC_USEFONTINFO is used as a left or right value i.e. lParam and not as an
 * action wParam despite what the docs say. It also appears not to affect
 * multiline controls??
 *
 */
static void EDIT_EM_SetMargins(WND *wnd, EDITSTATE *es, INT action,
			       INT left, INT right)
{
	if (action & EC_LEFTMARGIN) {
		if (left != EC_USEFONTINFO)
			es->left_margin = left;
		else
			if (es->style & ES_MULTILINE)
				es->left_margin = 0; /* ?? */
			else
			  es->left_margin = es->char_width;
	}

	if (action & EC_RIGHTMARGIN) {
		if (right != EC_USEFONTINFO)
 			es->right_margin = right;
		else
			if (es->style & ES_MULTILINE)
				es->right_margin = 0; /* ?? */
			else
				es->right_margin = es->char_width;
	}
	DPRINT( "left=%d, right=%d\n", es->left_margin, es->right_margin);
}


/*********************************************************************
 *
 *	EM_SETPASSWORDCHAR
 *
 */
static void EDIT_EM_SetPasswordChar(WND *wnd, EDITSTATE *es, CHAR c)
{
	if (es->style & ES_MULTILINE)
		return;

	if (es->password_char == c)
		return;

	es->password_char = c;
	if (c) {
		wnd->dwStyle |= ES_PASSWORD;
		es->style |= ES_PASSWORD;
	} else {
		wnd->dwStyle &= ~ES_PASSWORD;
		es->style &= ~ES_PASSWORD;
	}
	InvalidateRect(wnd->hwndSelf, NULL, TRUE);
}


/*********************************************************************
 *
 *	EDIT_EM_SetSel
 *
 *	note:	unlike the specs say: the order of start and end
 *		_is_ preserved in Windows.  (i.e. start can be > end)
 *		In other words: this handler is OK
 *
 */
static void EDIT_EM_SetSel(WND *wnd, EDITSTATE *es, UINT start, UINT end, BOOL after_wrap)
{
	UINT old_start = es->selection_start;
	UINT old_end = es->selection_end;
	UINT len = lstrlenA(es->text);

	if (start == -1) {
		start = es->selection_end;
		end = es->selection_end;
	} else {
		start = MIN(start, len);
		end = MIN(end, len);
	}
	es->selection_start = start;
	es->selection_end = end;
	if (after_wrap)
		es->flags |= EF_AFTER_WRAP;
	else
		es->flags &= ~EF_AFTER_WRAP;
	if (es->flags & EF_FOCUSED)
		EDIT_SetCaretPos(wnd, es, end, after_wrap);
/* This is little  bit more efficient than before, not sure if it can be improved. FIXME? */
        ORDER_UINT(start, end);
        ORDER_UINT(end, old_end);
        ORDER_UINT(start, old_start);
        ORDER_UINT(old_start, old_end);
	if (end != old_start)
        {
/*
 * One can also do 
 *          ORDER_UINT(end, old_start);
 *          EDIT_InvalidateText(wnd, es, start, end);
 *          EDIT_InvalidateText(wnd, es, old_start, old_end);
 * in place of the following if statement.                          
 */
            if (old_start > end )
            {
                EDIT_InvalidateText(wnd, es, start, end);
                EDIT_InvalidateText(wnd, es, old_start, old_end);
            }
            else
            {
                EDIT_InvalidateText(wnd, es, start, old_start);
                EDIT_InvalidateText(wnd, es, end, old_end);
            }
	}
        else EDIT_InvalidateText(wnd, es, start, old_end);
}


/*********************************************************************
 *
 *	EM_SETTABSTOPS
 *
 */
static BOOL EDIT_EM_SetTabStops(WND *wnd, EDITSTATE *es, INT count, LPINT tabs)
{
	if (!(es->style & ES_MULTILINE))
		return FALSE;
	if (es->tabs)
		HeapFree(es->heap, 0, es->tabs);
	es->tabs_count = count;
	if (!count)
		es->tabs = NULL;
	else {
		es->tabs = HeapAlloc(es->heap, 0, count * sizeof(INT));
		memcpy(es->tabs, tabs, count * sizeof(INT));
	}
	return TRUE;
}




/*********************************************************************
 *
 *	EM_SETWORDBREAKPROC
 *
 */
static void EDIT_EM_SetWordBreakProc(WND *wnd, EDITSTATE *es, EDITWORDBREAKPROCA wbp)
{
	if (es->word_break_procA == wbp)
		return;

	es->word_break_procA = wbp;
	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL)) {
		EDIT_BuildLineDefs_ML(wnd, es);
		InvalidateRect(wnd->hwndSelf, NULL, TRUE);
	}
}





/*********************************************************************
 *
 *	EM_UNDO / WM_UNDO
 *
 */
static BOOL EDIT_EM_Undo(WND *wnd, EDITSTATE *es)
{
	INT ulength = lstrlenA(es->undo_text);
	LPSTR utext = HeapAlloc(es->heap, 0, ulength + 1);

	lstrcpyA(utext, es->undo_text);

	DPRINT( "before UNDO:insertion length = %d, deletion buffer = %s\n",
		     es->undo_insert_count, utext);

	EDIT_EM_SetSel(wnd, es, es->undo_position, es->undo_position + es->undo_insert_count, FALSE);
	EDIT_EM_EmptyUndoBuffer(wnd, es);
	EDIT_EM_ReplaceSel(wnd, es, TRUE, utext);
	EDIT_EM_SetSel(wnd, es, es->undo_position, es->undo_position + es->undo_insert_count, FALSE);
	HeapFree(es->heap, 0, utext);

	DPRINT( "after UNDO:insertion length = %d, deletion buffer = %s\n",
			es->undo_insert_count, es->undo_text);

	return TRUE;
}


/*********************************************************************
 *
 *	WM_CHAR
 *
 */
static void EDIT_WM_Char(WND *wnd, EDITSTATE *es, CHAR c, DWORD key_data)
{
	switch (c) {
	case '\r':
	case '\n':
		if (es->style & ES_MULTILINE) {
			if (es->style & ES_READONLY) {
				EDIT_MoveHome(wnd, es, FALSE);
				EDIT_MoveDown_ML(wnd, es, FALSE);
			} else
				EDIT_EM_ReplaceSel(wnd, es, TRUE, "\r\n");
		}
		break;
	case '\t':
		if ((es->style & ES_MULTILINE) && !(es->style & ES_READONLY))
			EDIT_EM_ReplaceSel(wnd, es, TRUE, "\t");
		break;
	default:
		if (!(es->style & ES_READONLY) && ((BYTE)c >= ' ') && (c != 127)) {
			char str[2];
 			str[0] = c;
 			str[1] = '\0';
 			EDIT_EM_ReplaceSel(wnd, es, TRUE, str);
 		}
		break;
	}
}


/*********************************************************************
 *
 *	WM_COMMAND
 *
 */
static void EDIT_WM_Command(WND *wnd, EDITSTATE *es, INT code, INT id, HWND control)
{
	if (code || control)
		return;

	switch (id) {
		case EM_UNDO:
			EDIT_EM_Undo(wnd, es);
			break;
		case WM_CUT:
			EDIT_WM_Cut(wnd, es);
			break;
		case WM_COPY:
			EDIT_WM_Copy(wnd, es);
			break;
		case WM_PASTE:
			EDIT_WM_Paste(wnd, es);
			break;
		case WM_CLEAR:
			EDIT_WM_Clear(wnd, es);
			break;
		case EM_SETSEL:
			EDIT_EM_SetSel(wnd, es, 0, -1, FALSE);
			EDIT_EM_ScrollCaret(wnd, es);
			break;
		default:
			DPRINT( "unknown menu item, please report\n");
			break;
	}
}


/*********************************************************************
 *
 *	WM_CONTEXTMENU
 *
 *	Note: the resource files resource/sysres_??.rc cannot define a
 *		single popup menu.  Hence we use a (dummy) menubar
 *		containing the single popup menu as its first item.
 *
 *	FIXME: the message identifiers have been chosen arbitrarily,
 *		hence we use MF_BYPOSITION.
 *		We might as well use the "real" values (anybody knows ?)
 *		The menu definition is in resources/sysres_??.rc.
 *		Once these are OK, we better use MF_BYCOMMAND here
 *		(as we do in EDIT_WM_Command()).
 *
 */
static void EDIT_WM_ContextMenu(WND *wnd, EDITSTATE *es, HWND hwnd, INT x, INT y)
{
	HMENU menu = LoadMenuIndirectA(SYSRES_GetResPtr(SYSRES_MENU_EDITMENU));
	HMENU popup = GetSubMenu(menu, 0);
	UINT start = es->selection_start;
	UINT end = es->selection_end;

	ORDER_UINT(start, end);

	/* undo */
	EnableMenuItem(popup, 0, MF_BYPOSITION | (EDIT_EM_CanUndo(wnd, es) ? MF_ENABLED : MF_GRAYED));
	/* cut */
	EnableMenuItem(popup, 2, MF_BYPOSITION | ((end - start) && !(es->style & ES_PASSWORD) ? MF_ENABLED : MF_GRAYED));
	/* copy */
	EnableMenuItem(popup, 3, MF_BYPOSITION | ((end - start) && !(es->style & ES_PASSWORD) ? MF_ENABLED : MF_GRAYED));
	/* paste */
	EnableMenuItem(popup, 4, MF_BYPOSITION | (IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED));
	/* delete */
	EnableMenuItem(popup, 5, MF_BYPOSITION | ((end - start) ? MF_ENABLED : MF_GRAYED));
	/* select all */
	EnableMenuItem(popup, 7, MF_BYPOSITION | (start || (end != lstrlenA(es->text)) ? MF_ENABLED : MF_GRAYED));

	TrackPopupMenu(popup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, wnd->hwndSelf, NULL);
	DestroyMenu(menu);
}


/*********************************************************************
 *
 *	WM_COPY
 *
 */
static void EDIT_WM_Copy(WND *wnd, EDITSTATE *es)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	HGLOBAL hdst;
	LPSTR dst;

	if (e == s)
		return;
	ORDER_INT(s, e);
	hdst = GlobalAlloc(GMEM_MOVEABLE, (DWORD)(e - s + 1));
	dst = GlobalLock(hdst);
	lstrcpynA(dst, es->text + s, e - s + 1);
	GlobalUnlock(hdst);
	OpenClipboard(wnd->hwndSelf);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hdst);
	CloseClipboard();
}


/*********************************************************************
 *
 *	WM_CREATE
 *
 */
static LRESULT EDIT_WM_Create(WND *wnd, EDITSTATE *es, LPCREATESTRUCTA cs)
{
	/*
	 *	To initialize some final structure members, we call some helper
	 *	functions.  However, since the EDITSTATE is not consistent (i.e.
	 *	not fully initialized), we should be very careful which
	 *	functions can be called, and in what order.
	 */
	EDIT_WM_SetFont(wnd, es, 0, FALSE);
	if (cs->lpszName && *(cs->lpszName) != '\0') {
		EDIT_EM_ReplaceSel(wnd, es, FALSE, cs->lpszName);
                /* if we insert text to the editline, the text scrolls out of the window, as the caret is placed after the insert pos normally; thus we reset es->selection... to 0 and update caret */
                es->selection_start = es->selection_end = 0;
                EDIT_EM_ScrollCaret(wnd, es);
        }
	return 0;
}


/*********************************************************************
 *
 *	WM_DESTROY
 *
 */
static void EDIT_WM_Destroy(WND *wnd, EDITSTATE *es)
{
	if (es->hloc) {
		while (LocalUnlock(es->hloc)) ;
		LocalFree(es->hloc);
	}
	
	HeapDestroy(es->heap);
	HeapFree(GetProcessHeap(), 0, es);
	*(EDITSTATE **)wnd->wExtra = NULL;
}


/*********************************************************************
 *
 *	WM_ERASEBKGND
 *
 */
static LRESULT EDIT_WM_EraseBkGnd(WND *wnd, EDITSTATE *es, HDC dc)
{
	HBRUSH brush;
	RECT rc;

	if (!(brush = (HBRUSH)EDIT_SEND_CTLCOLOR(wnd, dc)))
		brush = (HBRUSH)GetStockObject(WHITE_BRUSH);

	GetClientRect(wnd->hwndSelf, &rc);
	IntersectClipRect(dc, rc.left, rc.top, rc.right, rc.bottom);
	GetClipBox(dc, &rc);
	/*
	 *	FIXME:	specs say that we should UnrealizeObject() the brush,
	 *		but the specs of UnrealizeObject() say that we shouldn't
	 *		unrealize a stock object.  The default brush that
	 *		DefWndProc() returns is ... a stock object.
	 */
	FillRect(dc, &rc, brush);
	return -1;
}


/*********************************************************************
 *
 *	WM_GETTEXT
 *
 */
static INT EDIT_WM_GetText(WND *wnd, EDITSTATE *es, INT count, LPSTR text)
{
	INT len = lstrlenA(es->text);

	if (count > len) {
		lstrcpyA(text, es->text);
		return len;
	} else
		return -1;
}


/*********************************************************************
 *
 *	EDIT_HScroll_Hack
 *
 *	16 bit notepad needs this.  Actually it is not _our_ hack,
 *	it is notepad's.  Notepad is sending us scrollbar messages with
 *	undocumented parameters without us even having a scrollbar ... !?!?
 *
 */
static LRESULT EDIT_HScroll_Hack(WND *wnd, EDITSTATE *es, INT action, INT pos, HWND scroll_bar)
{
	INT dx = 0;
	INT fw = es->format_rect.right - es->format_rect.left;
	LRESULT ret = 0;

	if (!(es->flags & EF_HSCROLL_HACK)) {
		DPRINT( "hacked WM_HSCROLL handler invoked\n");
		DPRINT( "      if you are _not_ running 16 bit notepad, please report\n");
		DPRINT( "      (this message is only displayed once per edit control)\n");
		es->flags |= EF_HSCROLL_HACK;
	}

	switch (action) {
	case SB_LINELEFT:
		if (es->x_offset)
			dx = -es->char_width;
		break;
	case SB_LINERIGHT:
		if (es->x_offset < es->text_width)
			dx = es->char_width;
		break;
	case SB_PAGELEFT:
		if (es->x_offset)
			dx = -fw / HSCROLL_FRACTION / es->char_width * es->char_width;
		break;
	case SB_PAGERIGHT:
		if (es->x_offset < es->text_width)
			dx = fw / HSCROLL_FRACTION / es->char_width * es->char_width;
		break;
	case SB_LEFT:
		if (es->x_offset)
			dx = -es->x_offset;
		break;
	case SB_RIGHT:
		if (es->x_offset < es->text_width)
			dx = es->text_width - es->x_offset;
		break;
	case SB_THUMBTRACK:
		es->flags |= EF_HSCROLL_TRACK;
		dx = pos * es->text_width / 100 - es->x_offset;
		break;
	case SB_THUMBPOSITION:
		es->flags &= ~EF_HSCROLL_TRACK;
		if (!(dx = pos * es->text_width / 100 - es->x_offset))
			EDIT_NOTIFY_PARENT(wnd, EN_HSCROLL, "EN_HSCROLL");
		break;
	case SB_ENDSCROLL:
		break;

	/*
	 *	FIXME : the next two are undocumented !
	 *	Are we doing the right thing ?
	 *	At least Win 3.1 Notepad makes use of EM_GETTHUMB this way,
	 *	although it's also a regular control message.
	 */
	case EM_GETTHUMB:
		ret = es->text_width ? es->x_offset * 100 / es->text_width : 0;
		break;
	case EM_LINESCROLL:
		dx = pos;
		break;

	default:
		DPRINT( "undocumented (hacked) WM_HSCROLL parameter, please report\n");
		return 0;
	}
	if (dx)
		EDIT_EM_LineScroll(wnd, es, dx, 0);
	return ret;
}


/*********************************************************************
 *
 *	WM_HSCROLL
 *
 */
static LRESULT EDIT_WM_HScroll(WND *wnd, EDITSTATE *es, INT action, INT pos, HWND scroll_bar)
{
	INT dx;
	INT fw;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if (!(es->style & ES_AUTOHSCROLL))
		return 0;

	if (!(es->style & WS_HSCROLL))
		return EDIT_HScroll_Hack(wnd, es, action, pos, scroll_bar);

	dx = 0;
	fw = es->format_rect.right - es->format_rect.left;
	switch (action) {
	case SB_LINELEFT:
		if (es->x_offset)
			dx = -es->char_width;
		break;
	case SB_LINERIGHT:
		if (es->x_offset < es->text_width)
			dx = es->char_width;
		break;
	case SB_PAGELEFT:
		if (es->x_offset)
			dx = -fw / HSCROLL_FRACTION / es->char_width * es->char_width;
		break;
	case SB_PAGERIGHT:
		if (es->x_offset < es->text_width)
			dx = fw / HSCROLL_FRACTION / es->char_width * es->char_width;
		break;
	case SB_LEFT:
		if (es->x_offset)
			dx = -es->x_offset;
		break;
	case SB_RIGHT:
		if (es->x_offset < es->text_width)
			dx = es->text_width - es->x_offset;
		break;
	case SB_THUMBTRACK:
		es->flags |= EF_HSCROLL_TRACK;
		dx = pos - es->x_offset;
		break;
	case SB_THUMBPOSITION:
		es->flags &= ~EF_HSCROLL_TRACK;
		if (!(dx = pos - es->x_offset)) {
			SetScrollPos(wnd->hwndSelf, SB_HORZ, pos, TRUE);
			EDIT_NOTIFY_PARENT(wnd, EN_HSCROLL, "EN_HSCROLL");
		}
		break;
	case SB_ENDSCROLL:
		break;

	default:
		DPRINT( "undocumented WM_HSCROLL parameter, please report\n");
		return 0;
	}
	if (dx)
		EDIT_EM_LineScroll(wnd, es, dx, 0);
	return 0;
}


/*********************************************************************
 *
 *	EDIT_CheckCombo
 *
 */
static BOOL EDIT_CheckCombo(WND *wnd, UINT msg, INT key, DWORD key_data)
{
	HWND hLBox;

	if (WIDGETS_IsControl(wnd->parent, BIC_COMBO) &&
			(hLBox = COMBO_GetLBWindow(wnd->parent))) {
		HWND hCombo = wnd->parent->hwndSelf;
		BOOL bUIFlip = TRUE;

		DPRINT( "[%04x]: handling msg %04x (%04x)\n",
			     wnd->hwndSelf, (UINT)msg, (UINT)key);

		switch (msg) {
		case WM_KEYDOWN: /* Handle F4 and arrow keys */
			if (key != VK_F4) {
				bUIFlip = (BOOL)SendMessageA(hCombo, CB_GETEXTENDEDUI, 0, 0);
				if (SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0))
					bUIFlip = FALSE;
			}
			if (!bUIFlip)
				SendMessageA(hLBox, WM_KEYDOWN, (WPARAM)key, 0);
			else {
				/* make sure ComboLBox pops up */
				SendMessageA(hCombo, CB_SETEXTENDEDUI, 0, 0);
				SendMessageA(hLBox, WM_KEYDOWN, VK_F4, 0);
				SendMessageA(hCombo, CB_SETEXTENDEDUI, 1, 0);
			}
			break;
		case WM_SYSKEYDOWN: /* Handle Alt+up/down arrows */
			bUIFlip = (BOOL)SendMessageA(hCombo, CB_GETEXTENDEDUI, 0, 0);
			if (bUIFlip) {
				bUIFlip = (BOOL)SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0);
				SendMessageA(hCombo, CB_SHOWDROPDOWN, (bUIFlip) ? FALSE : TRUE, 0);
			} else
				SendMessageA(hLBox, WM_KEYDOWN, VK_F4, 0);
			break;
		}
		return TRUE;
	}
	return FALSE;
}


/*********************************************************************
 *
 *	WM_KEYDOWN
 *
 *	Handling of special keys that don't produce a WM_CHAR
 *	(i.e. non-printable keys) & Backspace & Delete
 *
 */
static LRESULT EDIT_WM_KeyDown(WND *wnd, EDITSTATE *es, INT key, DWORD key_data)
{
	BOOL shift;
	BOOL control;

	if (GetKeyState(VK_MENU) & 0x8000)
		return 0;

	shift = GetKeyState(VK_SHIFT) & 0x8000;
	control = GetKeyState(VK_CONTROL) & 0x8000;

	switch (key) {
	case VK_F4:
	case VK_UP:
		if (EDIT_CheckCombo(wnd, WM_KEYDOWN, key, key_data))
			break;
		if (key == VK_F4)
			break;
		/* fall through */
	case VK_LEFT:
		if ((es->style & ES_MULTILINE) && (key == VK_UP))
			EDIT_MoveUp_ML(wnd, es, shift);
		else
			if (control)
				EDIT_MoveWordBackward(wnd, es, shift);
			else
				EDIT_MoveBackward(wnd, es, shift);
		break;
	case VK_DOWN:
		if (EDIT_CheckCombo(wnd, WM_KEYDOWN, key, key_data))
			break;
		/* fall through */
	case VK_RIGHT:
		if ((es->style & ES_MULTILINE) && (key == VK_DOWN))
			EDIT_MoveDown_ML(wnd, es, shift);
		else if (control)
			EDIT_MoveWordForward(wnd, es, shift);
		else
			EDIT_MoveForward(wnd, es, shift);
		break;
	case VK_HOME:
		EDIT_MoveHome(wnd, es, shift);
		break;
	case VK_END:
		EDIT_MoveEnd(wnd, es, shift);
		break;
	case VK_PRIOR:
		if (es->style & ES_MULTILINE)
			EDIT_MovePageUp_ML(wnd, es, shift);
		break;
	case VK_NEXT:
		if (es->style & ES_MULTILINE)
			EDIT_MovePageDown_ML(wnd, es, shift);
		break;
	case VK_BACK:
		if (!(es->style & ES_READONLY) && !control) {
			if (es->selection_start != es->selection_end)
				EDIT_WM_Clear(wnd, es);
			else {
				/* delete character left of caret */
				EDIT_EM_SetSel(wnd, es, -1, 0, FALSE);
				EDIT_MoveBackward(wnd, es, TRUE);
				EDIT_WM_Clear(wnd, es);
			}
		}
		break;
	case VK_DELETE:
		if (!(es->style & ES_READONLY) && !(shift && control)) {
			if (es->selection_start != es->selection_end) {
				if (shift)
					EDIT_WM_Cut(wnd, es);
				else
					EDIT_WM_Clear(wnd, es);
			} else {
				if (shift) {
					/* delete character left of caret */
					EDIT_EM_SetSel(wnd, es, -1, 0, FALSE);
					EDIT_MoveBackward(wnd, es, TRUE);
					EDIT_WM_Clear(wnd, es);
				} else if (control) {
					/* delete to end of line */
					EDIT_EM_SetSel(wnd, es, -1, 0, FALSE);
					EDIT_MoveEnd(wnd, es, TRUE);
					EDIT_WM_Clear(wnd, es);
				} else {
					/* delete character right of caret */
					EDIT_EM_SetSel(wnd, es, -1, 0, FALSE);
					EDIT_MoveForward(wnd, es, TRUE);
					EDIT_WM_Clear(wnd, es);
				}
			}
		}
		break;
	case VK_INSERT:
		if (shift) {
			if (!(es->style & ES_READONLY))
				EDIT_WM_Paste(wnd, es);
		} else if (control)
			EDIT_WM_Copy(wnd, es);
		break;
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_KILLFOCUS
 *
 */
static LRESULT EDIT_WM_KillFocus(WND *wnd, EDITSTATE *es, HWND window_getting_focus)
{
	es->flags &= ~EF_FOCUSED;
	DestroyCaret();
	if(!(es->style & ES_NOHIDESEL))
		EDIT_InvalidateText(wnd, es, es->selection_start, es->selection_end);
	EDIT_NOTIFY_PARENT(wnd, EN_KILLFOCUS, "EN_KILLFOCUS");
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONDBLCLK
 *
 *	The caret position has been set on the WM_LBUTTONDOWN message
 *
 */
static LRESULT EDIT_WM_LButtonDblClk(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y)
{
	INT s;
	INT e = es->selection_end;
	INT l;
	INT li;
	INT ll;

	if (!(es->flags & EF_FOCUSED))
		return 0;

	l = EDIT_EM_LineFromChar(wnd, es, e);
	li = EDIT_EM_LineIndex(wnd, es, l);
	ll = EDIT_EM_LineLength(wnd, es, e);
	s = li + EDIT_CallWordBreakProc (wnd, es, li, e - li, ll, WB_LEFT);
	e = li + EDIT_CallWordBreakProc(wnd, es, li, e - li, ll, WB_RIGHT);
	EDIT_EM_SetSel(wnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONDOWN
 *
 */
static LRESULT EDIT_WM_LButtonDown(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y)
{
	INT e;
	BOOL after_wrap;

	if (!(es->flags & EF_FOCUSED))
		return 0;

	SetCapture(wnd->hwndSelf);
	EDIT_ConfinePoint(wnd, es, &x, &y);
	e = EDIT_CharFromPos(wnd, es, x, y, &after_wrap);
	EDIT_EM_SetSel(wnd, es, (keys & MK_SHIFT) ? es->selection_start : e, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
	es->region_posx = es->region_posy = 0;
	SetTimer(wnd->hwndSelf, 0, 100, NULL);
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONUP
 *
 */
static LRESULT EDIT_WM_LButtonUp(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y)
{
	if (GetCapture() == wnd->hwndSelf) {
		KillTimer(wnd->hwndSelf, 0);
		ReleaseCapture();
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_MOUSEMOVE
 *
 */
static LRESULT EDIT_WM_MouseMove(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y)
{
	INT e;
	BOOL after_wrap;
	INT prex, prey;

	if (GetCapture() != wnd->hwndSelf)
		return 0;

	/*
	 *	FIXME: gotta do some scrolling if outside client
	 *		area.  Maybe reset the timer ?
	 */
	prex = x; prey = y;
	EDIT_ConfinePoint(wnd, es, &x, &y);
	es->region_posx = (prex < x) ? -1 : ((prex > x) ? 1 : 0);
	es->region_posy = (prey < y) ? -1 : ((prey > y) ? 1 : 0);
	e = EDIT_CharFromPos(wnd, es, x, y, &after_wrap);
	EDIT_EM_SetSel(wnd, es, es->selection_start, e, after_wrap);
	return 0;
}


/*********************************************************************
 *
 *	WM_NCCREATE
 *
 */
static LRESULT EDIT_WM_NCCreate(WND *wnd, LPCREATESTRUCTA cs)
{
	EDITSTATE *es;

	if (!(es = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*es))))
		return FALSE;
	*(EDITSTATE **)wnd->wExtra = es;

       /*
        *      Note: since the EDITSTATE has not been fully initialized yet,
        *            we can't use any API calls that may send
        *            WM_XXX messages before WM_NCCREATE is completed.
        */

 	if (!(es->heap = HeapCreate(0, 0x10000, 0)))
 		return FALSE;
 	es->style = cs->style;
 
	if ((es->style & WS_BORDER) && !(es->style & WS_DLGFRAME))
		wnd->dwStyle &= ~WS_BORDER;

	if (es->style & ES_MULTILINE) {
		es->buffer_size = BUFSTART_MULTI;
		es->buffer_limit = BUFLIMIT_MULTI;
		if (es->style & WS_VSCROLL)
			es->style |= ES_AUTOVSCROLL;
		if (es->style & WS_HSCROLL)
			es->style |= ES_AUTOHSCROLL;
		es->style &= ~ES_PASSWORD;
		if ((es->style & ES_CENTER) || (es->style & ES_RIGHT)) {
			if (es->style & ES_RIGHT)
				es->style &= ~ES_CENTER;
			es->style &= ~WS_HSCROLL;
			es->style &= ~ES_AUTOHSCROLL;
		}

		/* FIXME: for now, all multi line controls are AUTOVSCROLL */
		es->style |= ES_AUTOVSCROLL;
	} else {
		es->buffer_size = BUFSTART_SINGLE;
		es->buffer_limit = BUFLIMIT_SINGLE;
		es->style &= ~ES_CENTER;
		es->style &= ~ES_RIGHT;
		es->style &= ~WS_HSCROLL;
		es->style &= ~WS_VSCROLL;
		es->style &= ~ES_AUTOVSCROLL;
		es->style &= ~ES_WANTRETURN;
		if (es->style & ES_UPPERCASE) {
			es->style &= ~ES_LOWERCASE;
			es->style &= ~ES_NUMBER;
		} else if (es->style & ES_LOWERCASE)
			es->style &= ~ES_NUMBER;
		if (es->style & ES_PASSWORD)
			es->password_char = '*';

		/* FIXME: for now, all single line controls are AUTOHSCROLL */
		es->style |= ES_AUTOHSCROLL;
	}
	if (!(es->text = HeapAlloc(es->heap, 0, es->buffer_size + 1)))
		return FALSE;
	es->buffer_size = HeapSize(es->heap, 0, es->text) - 1;
	if (!(es->undo_text = HeapAlloc(es->heap, 0, es->buffer_size + 1)))
		return FALSE;
	es->undo_buffer_size = HeapSize(es->heap, 0, es->undo_text) - 1;
	*es->text = '\0';
	if (es->style & ES_MULTILINE)
		if (!(es->first_line_def = HeapAlloc(es->heap, HEAP_ZERO_MEMORY, sizeof(LINEDEF))))
			return FALSE;
	es->line_count = 1;

	return TRUE;
}

/*********************************************************************
 *
 *	WM_PAINT
 *
 */
static void EDIT_WM_Paint(WND *wnd, EDITSTATE *es)
{
	PAINTSTRUCT ps;
	INT i;
	HDC dc;
	HFONT old_font = 0;
	RECT rc;
	RECT rcLine;
	RECT rcRgn;
	BOOL rev = IsWindowEnabled(wnd->hwndSelf) &&
				((es->flags & EF_FOCUSED) ||
					(es->style & ES_NOHIDESEL));

	if (es->flags & EF_UPDATE)
		EDIT_NOTIFY_PARENT(wnd, EN_UPDATE, "EN_UPDATE");

	dc = BeginPaint(wnd->hwndSelf, &ps);
	if(es->style & WS_BORDER) {
		GetClientRect(wnd->hwndSelf, &rc);
		if(es->style & ES_MULTILINE) {
			if(es->style & WS_HSCROLL) rc.bottom++;
			if(es->style & WS_VSCROLL) rc.right++;
		}
		Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
	}
	IntersectClipRect(dc, es->format_rect.left,
				es->format_rect.top,
				es->format_rect.right,
				es->format_rect.bottom);
	if (es->style & ES_MULTILINE) {
		GetClientRect(wnd->hwndSelf, &rc);
		IntersectClipRect(dc, rc.left, rc.top, rc.right, rc.bottom);
	}
	if (es->font)
		old_font = SelectObject(dc, es->font);
	EDIT_SEND_CTLCOLOR(wnd, dc);
	if (!IsWindowEnabled(wnd->hwndSelf))
		SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
	GetClipBox(dc, &rcRgn);
	if (es->style & ES_MULTILINE) {
		INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		for (i = es->y_offset ; i <= MIN(es->y_offset + vlc, es->y_offset + es->line_count - 1) ; i++) {
			EDIT_GetLineRect(wnd, es, i, 0, -1, &rcLine);
			if (IntersectRect(&rc, &rcRgn, &rcLine))
				EDIT_PaintLine(wnd, es, dc, i, rev);
		}
	} else {
		EDIT_GetLineRect(wnd, es, 0, 0, -1, &rcLine);
		if (IntersectRect(&rc, &rcRgn, &rcLine))
			EDIT_PaintLine(wnd, es, dc, 0, rev);
	}
	if (es->font)
		SelectObject(dc, old_font);
	if (es->flags & EF_FOCUSED)
		EDIT_SetCaretPos(wnd, es, es->selection_end,
				 es->flags & EF_AFTER_WRAP);
	EndPaint(wnd->hwndSelf, &ps);
	if ((es->style & WS_VSCROLL) && !(es->flags & EF_VSCROLL_TRACK)) {
		INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		SCROLLINFO si;
		si.cbSize	= sizeof(SCROLLINFO);
		si.fMask	= SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
		si.nMin		= 0;
		si.nMax		= es->line_count + vlc - 2;
		si.nPage	= vlc;
		si.nPos		= es->y_offset;
		SetScrollInfo(wnd->hwndSelf, SB_VERT, &si, TRUE);
	}
	if ((es->style & WS_HSCROLL) && !(es->flags & EF_HSCROLL_TRACK)) {
		SCROLLINFO si;
		INT fw = es->format_rect.right - es->format_rect.left;
		si.cbSize	= sizeof(SCROLLINFO);
		si.fMask	= SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
		si.nMin		= 0;
		si.nMax		= es->text_width + fw - 1;
		si.nPage	= fw;
		si.nPos		= es->x_offset;
		SetScrollInfo(wnd->hwndSelf, SB_HORZ, &si, TRUE);
	}

	if (es->flags & EF_UPDATE) {
		es->flags &= ~EF_UPDATE;
		EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
	}
}


/*********************************************************************
 *
 *	WM_PASTE
 *
 */
static void EDIT_WM_Paste(WND *wnd, EDITSTATE *es)
{
	HGLOBAL hsrc;
	LPSTR src;

	OpenClipboard(wnd->hwndSelf);
	if ((hsrc = GetClipboardData(CF_TEXT))) {
		src = (LPSTR)GlobalLock(hsrc);
		EDIT_EM_ReplaceSel(wnd, es, TRUE, src);
		GlobalUnlock(hsrc);
	}
	CloseClipboard();
}


/*********************************************************************
 *
 *	WM_SETFOCUS
 *
 */
static void EDIT_WM_SetFocus(WND *wnd, EDITSTATE *es, HWND window_losing_focus)
{
	es->flags |= EF_FOCUSED;
	CreateCaret(wnd->hwndSelf, 0, 2, es->line_height);
	EDIT_SetCaretPos(wnd, es, es->selection_end,
			 es->flags & EF_AFTER_WRAP);
	if(!(es->style & ES_NOHIDESEL))
		EDIT_InvalidateText(wnd, es, es->selection_start, es->selection_end);
	ShowCaret(wnd->hwndSelf);
	EDIT_NOTIFY_PARENT(wnd, EN_SETFOCUS, "EN_SETFOCUS");
}


/*********************************************************************
 *
 *	WM_SETFONT
 *
 * With Win95 look the margins are set to default font value unless 
 * the system font (font == 0) is being set, in which case they are left
 * unchanged.
 *
 */
static void EDIT_WM_SetFont(WND *wnd, EDITSTATE *es, HFONT font, BOOL redraw)
{
	TEXTMETRIC tm;
	HDC dc;
	HFONT old_font = 0;

	es->font = font;
	dc = GetDC(wnd->hwndSelf);
	if (font)
		old_font = SelectObject(dc, font);
	GetTextMetricsA(dc, &tm);
	es->line_height = tm.tmHeight;
	es->char_width = tm.tmAveCharWidth;
	if (font)
		SelectObject(dc, old_font);
	ReleaseDC(wnd->hwndSelf, dc);
	if (font && (TWEAK_WineLook > WIN31_LOOK))
		EDIT_EM_SetMargins(wnd, es, EC_LEFTMARGIN | EC_RIGHTMARGIN,
				   EC_USEFONTINFO, EC_USEFONTINFO);
	if (es->style & ES_MULTILINE)
		EDIT_BuildLineDefs_ML(wnd, es);
	else {
		RECT r;
		GetClientRect(wnd->hwndSelf, &r);
		EDIT_SetRectNP(wnd, es, &r);
	}
	if (redraw)
		InvalidateRect(wnd->hwndSelf, NULL, TRUE);
	if (es->flags & EF_FOCUSED) {
		DestroyCaret();
		CreateCaret(wnd->hwndSelf, 0, 2, es->line_height);
		EDIT_SetCaretPos(wnd, es, es->selection_end,
				 es->flags & EF_AFTER_WRAP);
		ShowCaret(wnd->hwndSelf);
	}
}


/*********************************************************************
 *
 *	WM_SETTEXT
 *
 * NOTES
 *  For multiline controls (ES_MULTILINE), reception of WM_SETTEXT triggers:
 *  The modified flag is reset. No notifications are sent.
 *
 *  For single-line controls, reception of WM_SETTEXT triggers:
 *  The modified flag is reset. EN_UPDATE and EN_CHANGE notifications are sent.
 *
 */
static void EDIT_WM_SetText(WND *wnd, EDITSTATE *es, LPCSTR text)
{
	EDIT_EM_SetSel(wnd, es, 0, -1, FALSE);
	if (text) {
		DPRINT( "\t'%s'\n", text);
		EDIT_EM_ReplaceSel(wnd, es, FALSE, text);
	} else {
		DPRINT( "\t<NULL>\n");
		EDIT_EM_ReplaceSel(wnd, es, FALSE, "");
	}
	es->x_offset = 0;
	if (es->style & ES_MULTILINE) {
		es->flags &= ~EF_UPDATE;
	} else {
		es->flags |= EF_UPDATE;
	}
	es->flags &= ~EF_MODIFIED;
	EDIT_EM_SetSel(wnd, es, 0, 0, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	WM_SIZE
 *
 */
static void EDIT_WM_Size(WND *wnd, EDITSTATE *es, UINT action, INT width, INT height)
{
	if ((action == SIZE_MAXIMIZED) || (action == SIZE_RESTORED)) {
		RECT rc;
		SetRect(&rc, 0, 0, width, height);
		EDIT_SetRectNP(wnd, es, &rc);
		InvalidateRect(wnd->hwndSelf, NULL, TRUE);
	}
}


/*********************************************************************
 *
 *	WM_SYSKEYDOWN
 *
 */
static LRESULT EDIT_WM_SysKeyDown(WND *wnd, EDITSTATE *es, INT key, DWORD key_data)
{
	if ((key == VK_BACK) && (key_data & 0x2000)) {
		if (EDIT_EM_CanUndo(wnd, es))
			EDIT_EM_Undo(wnd, es);
		return 0;
	} else if (key == VK_UP || key == VK_DOWN)
		if (EDIT_CheckCombo(wnd, WM_SYSKEYDOWN, key, key_data))
			return 0;
	return DefWindowProcA(wnd->hwndSelf, WM_SYSKEYDOWN, (WPARAM)key, (LPARAM)key_data);
}


/*********************************************************************
 *
 *	WM_TIMER
 *
 */
static void EDIT_WM_Timer(WND *wnd, EDITSTATE *es, INT id, TIMERPROC timer_proc)
{
	if (es->region_posx < 0) {
		EDIT_MoveBackward(wnd, es, TRUE);
	} else if (es->region_posx > 0) {
		EDIT_MoveForward(wnd, es, TRUE);
	}
/*
 *	FIXME: gotta do some vertical scrolling here, like
 *		EDIT_EM_LineScroll(wnd, 0, 1);
 */
}


/*********************************************************************
 *
 *	EDIT_VScroll_Hack
 *
 *	16 bit notepad needs this.  Actually it is not _our_ hack,
 *	it is notepad's.  Notepad is sending us scrollbar messages with
 *	undocumented parameters without us even having a scrollbar ... !?!?
 *
 */
static LRESULT EDIT_VScroll_Hack(WND *wnd, EDITSTATE *es, INT action, INT pos, HWND scroll_bar)
{
	INT dy = 0;
	LRESULT ret = 0;

	if (!(es->flags & EF_VSCROLL_HACK)) {
		DPRINT( "hacked WM_VSCROLL handler invoked\n");
		DPRINT( "      if you are _not_ running 16 bit notepad, please report\n");
		DPRINT( "      (this message is only displayed once per edit control)\n");
		es->flags |= EF_VSCROLL_HACK;
	}

	switch (action) {
	case SB_LINEUP:
	case SB_LINEDOWN:
	case SB_PAGEUP:
	case SB_PAGEDOWN:
		EDIT_EM_Scroll(wnd, es, action);
		return 0;
	case SB_TOP:
		dy = -es->y_offset;
		break;
	case SB_BOTTOM:
		dy = es->line_count - 1 - es->y_offset;
		break;
	case SB_THUMBTRACK:
		es->flags |= EF_VSCROLL_TRACK;
		dy = (pos * (es->line_count - 1) + 50) / 100 - es->y_offset;
		break;
	case SB_THUMBPOSITION:
		es->flags &= ~EF_VSCROLL_TRACK;
		if (!(dy = (pos * (es->line_count - 1) + 50) / 100 - es->y_offset))
			EDIT_NOTIFY_PARENT(wnd, EN_VSCROLL, "EN_VSCROLL");
		break;
	case SB_ENDSCROLL:
		break;

	/*
	 *	FIXME : the next two are undocumented !
	 *	Are we doing the right thing ?
	 *	At least Win 3.1 Notepad makes use of EM_GETTHUMB this way,
	 *	although it's also a regular control message.
	 */
	case EM_GETTHUMB:
		ret = (es->line_count > 1) ? es->y_offset * 100 / (es->line_count - 1) : 0;
		break;
	case EM_LINESCROLL:
		dy = pos;
		break;

	default:
		DPRINT( "undocumented (hacked) WM_VSCROLL parameter, please report\n");
		return 0;
	}
	if (dy)
		EDIT_EM_LineScroll(wnd, es, 0, dy);
	return ret;
}


/*********************************************************************
 *
 *	WM_VSCROLL
 *
 */
static LRESULT EDIT_WM_VScroll(WND *wnd, EDITSTATE *es, INT action, INT pos, HWND scroll_bar)
{
	INT dy;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if (!(es->style & ES_AUTOVSCROLL))
		return 0;

	if (!(es->style & WS_VSCROLL))
		return EDIT_VScroll_Hack(wnd, es, action, pos, scroll_bar);

	dy = 0;
	switch (action) {
	case SB_LINEUP:
	case SB_LINEDOWN:
	case SB_PAGEUP:
	case SB_PAGEDOWN:
		EDIT_EM_Scroll(wnd, es, action);
		return 0;

	case SB_TOP:
		dy = -es->y_offset;
		break;
	case SB_BOTTOM:
		dy = es->line_count - 1 - es->y_offset;
		break;
	case SB_THUMBTRACK:
		es->flags |= EF_VSCROLL_TRACK;
		dy = pos - es->y_offset;
		break;
	case SB_THUMBPOSITION:
		es->flags &= ~EF_VSCROLL_TRACK;
		if (!(dy = pos - es->y_offset)) {
			SetScrollPos(wnd->hwndSelf, SB_VERT, pos, TRUE);
			EDIT_NOTIFY_PARENT(wnd, EN_VSCROLL, "EN_VSCROLL");
		}
		break;
	case SB_ENDSCROLL:
		break;

	default:
		DPRINT( "undocumented WM_VSCROLL action %d, please report\n",
			action);
		return 0;
	}
	if (dy)
		EDIT_EM_LineScroll(wnd, es, 0, dy);
	return 0;
}

// temporary hack

WINBOOL STDCALL IsClipboardFormatAvailable(
    UINT  format 
   )
{
	return FALSE;
}

WINBOOL
STDCALL
OpenClipboard(
	      HWND hWndNewOwner)
{
	return FALSE;
}

WINBOOL
STDCALL
CloseClipboard(
	       VOID)
{
	return TRUE;
}

WINBOOL
STDCALL
EmptyClipboard(
	       VOID)
{
	return TRUE;
}

HANDLE
STDCALL
SetClipboardData(
		 UINT uFormat,
		 HANDLE hMem)
{
	return NULL;
}

 
HANDLE
STDCALL
GetClipboardData(
		 UINT uFormat)
{
	return NULL;
}