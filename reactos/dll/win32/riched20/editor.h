/*
 * RichEdit - prototypes for functions and macro definitions
 *
 * Copyright 2004 by Krzysztof Foltman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "editstr.h"
#include "wine/unicode.h"

struct _RTF_Info;

extern HANDLE me_heap;

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc( size_t len )
{
    return HeapAlloc( me_heap, 0, len );
}

static inline BOOL heap_free( void *ptr )
{
    return HeapFree( me_heap, 0, ptr );
}

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc( void *ptr, size_t len )
{
    return HeapReAlloc( me_heap, 0, ptr, len );
}

#define ALLOC_OBJ(type) heap_alloc(sizeof(type))
#define ALLOC_N_OBJ(type, count) heap_alloc((count)*sizeof(type))
#define FREE_OBJ(ptr) heap_free(ptr)

#define RUN_IS_HIDDEN(run) ((run)->style->fmt.dwMask & CFM_HIDDEN \
                             && (run)->style->fmt.dwEffects & CFE_HIDDEN)

#define InitFormatEtc(fe, cf, med) \
        {\
        (fe).cfFormat=cf;\
        (fe).dwAspect=DVASPECT_CONTENT;\
        (fe).ptd=NULL;\
        (fe).tymed=med;\
        (fe).lindex=-1;\
        };

/* style.c */
ME_Style *ME_MakeStyle(CHARFORMAT2W *style);
void ME_AddRefStyle(ME_Style *item);
void ME_ReleaseStyle(ME_Style *item);
ME_Style *ME_GetInsertStyle(ME_TextEditor *editor, int nCursor);
ME_Style *ME_ApplyStyle(ME_Style *sSrc, CHARFORMAT2W *style);
HFONT ME_SelectStyleFont(ME_Context *c, ME_Style *s);
void ME_UnselectStyleFont(ME_Context *c, ME_Style *s, HFONT hOldFont);
void ME_InitCharFormat2W(CHARFORMAT2W *pFmt);
void ME_SaveTempStyle(ME_TextEditor *editor);
void ME_ClearTempStyle(ME_TextEditor *editor);
void ME_DumpStyleToBuf(CHARFORMAT2W *pFmt, char buf[2048]);
void ME_DumpStyle(ME_Style *s);
CHARFORMAT2W *ME_ToCF2W(CHARFORMAT2W *to, CHARFORMAT2W *from);
void ME_CopyToCFAny(CHARFORMAT2W *to, CHARFORMAT2W *from);
void ME_CopyCharFormat(CHARFORMAT2W *pDest, const CHARFORMAT2W *pSrc); /* only works with 2W structs */
void ME_CharFormatFromLogFont(HDC hDC, const LOGFONTW *lf, CHARFORMAT2W *fmt); /* ditto */

/* list.c */
void ME_InsertBefore(ME_DisplayItem *diWhere, ME_DisplayItem *diWhat);
void ME_Remove(ME_DisplayItem *diWhere);
ME_DisplayItem *ME_FindItemBack(ME_DisplayItem *di, ME_DIType nTypeOrClass);
ME_DisplayItem *ME_FindItemFwd(ME_DisplayItem *di, ME_DIType nTypeOrClass);
ME_DisplayItem *ME_FindItemBackOrHere(ME_DisplayItem *di, ME_DIType nTypeOrClass);
ME_DisplayItem *ME_MakeDI(ME_DIType type);
void ME_DestroyDisplayItem(ME_DisplayItem *item);
void ME_DumpDocument(ME_TextBuffer *buffer);
const char *ME_GetDITypeName(ME_DIType type);

/* string.c */
ME_String *ME_MakeStringN(LPCWSTR szText, int nMaxChars);
ME_String *ME_MakeStringR(WCHAR cRepeat, int nMaxChars);
ME_String *ME_StrDup(const ME_String *s);
void ME_DestroyString(ME_String *s);
void ME_AppendString(ME_String *s1, const ME_String *s2);
ME_String *ME_VSplitString(ME_String *orig, int nVPos);
int ME_IsWhitespaces(const ME_String *s);
int ME_IsSplitable(const ME_String *s);
int ME_FindNonWhitespaceV(const ME_String *s, int nVChar);
int ME_FindWhitespaceV(ME_String *s, int nVChar);
int ME_CallWordBreakProc(ME_TextEditor *editor, ME_String *str, INT start, INT code);
void ME_StrDeleteV(ME_String *s, int nVChar, int nChars);
/* smart helpers for A<->W conversions, they reserve/free memory and call MultiByte<->WideChar functions */
LPWSTR ME_ToUnicode(BOOL unicode, LPVOID psz);
void ME_EndToUnicode(BOOL unicode, LPVOID psz);

static inline int ME_IsWSpace(WCHAR ch)
{
  return ch > '\0' && ch <= ' ';
}

static inline int ME_CharCompare(WCHAR a, WCHAR b, int caseSensitive)
{
  return caseSensitive ? (a == b) : (toupperW(a) == toupperW(b));
}

/* note: those two really return the first matching offset (starting from EOS)+1 
 * in other words, an offset of the first trailing white/black */
int ME_ReverseFindNonWhitespaceV(const ME_String *s, int nVChar);
int ME_ReverseFindWhitespaceV(const ME_String *s, int nVChar);

/* row.c */
ME_DisplayItem *ME_RowStart(ME_DisplayItem *item);
/* ME_DisplayItem *ME_RowEnd(ME_DisplayItem *item); */
void ME_RenumberParagraphs(ME_DisplayItem *item); /* TODO */
ME_DisplayItem *ME_FindRowWithNumber(ME_TextEditor *editor, int nRow);
int ME_RowNumberFromCharOfs(ME_TextEditor *editor, int nOfs);

/* run.c */
ME_DisplayItem *ME_MakeRun(ME_Style *s, ME_String *strData, int nFlags);
/* note: ME_InsertRun inserts a copy of the specified run - so you need to destroy the original */
ME_DisplayItem *ME_InsertRun(ME_TextEditor *editor, int nCharOfs, ME_DisplayItem *pItem);
ME_DisplayItem *ME_InsertRunAtCursor(ME_TextEditor *editor, ME_Cursor *cursor,
                                     ME_Style *style, const WCHAR *str, int len, int flags);
void ME_CheckCharOffsets(ME_TextEditor *editor);
void ME_PropagateCharOffset(ME_DisplayItem *p, int shift);
int ME_CharFromPoint(ME_Context *c, int cx, ME_Run *run);
/* this one accounts for 1/2 char tolerance */
int ME_CharFromPointCursor(ME_TextEditor *editor, int cx, ME_Run *run);
int ME_PointFromChar(ME_TextEditor *editor, ME_Run *pRun, int nOffset);
int ME_GetLastSplittablePlace(ME_Context *c, ME_Run *run);
int ME_CanJoinRuns(const ME_Run *run1, const ME_Run *run2);
void ME_JoinRuns(ME_TextEditor *editor, ME_DisplayItem *p);
ME_DisplayItem *ME_SplitRun(ME_WrapContext *wc, ME_DisplayItem *item, int nChar);
ME_DisplayItem *ME_SplitRunSimple(ME_TextEditor *editor, ME_DisplayItem *item, int nChar);
int ME_FindSplitPoint(ME_Context *c, POINT *pt, ME_Run *run, int desperate);
void ME_UpdateRunFlags(ME_TextEditor *editor, ME_Run *run);
ME_DisplayItem *ME_SplitFurther(ME_TextEditor *editor, ME_DisplayItem *run);
void ME_CalcRunExtent(ME_Context *c, const ME_Paragraph *para, int startx, ME_Run *run);
SIZE ME_GetRunSize(ME_Context *c, const ME_Paragraph *para, ME_Run *run, int nLen, int startx);
void ME_CursorFromCharOfs(ME_TextEditor *editor, int nCharOfs, ME_Cursor *pCursor);
void ME_RunOfsFromCharOfs(ME_TextEditor *editor, int nCharOfs, ME_DisplayItem **ppPara, ME_DisplayItem **ppRun, int *pOfs);
int ME_CharOfsFromRunOfs(ME_TextEditor *editor, const ME_DisplayItem *pPara, const ME_DisplayItem *pRun, int nOfs);
void ME_SkipAndPropagateCharOffset(ME_DisplayItem *p, int shift);
void ME_SetCharFormat(ME_TextEditor *editor, int nFrom, int nLen, CHARFORMAT2W *pFmt);
void ME_SetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt);
void ME_GetCharFormat(ME_TextEditor *editor, int nFrom, int nLen, CHARFORMAT2W *pFmt);
void ME_GetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt);
void ME_GetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt);
void ME_SetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *mod);

/* caret.c */
int ME_SetSelection(ME_TextEditor *editor, int from, int to);
void ME_HideCaret(ME_TextEditor *ed);
void ME_ShowCaret(ME_TextEditor *ed);
void ME_MoveCaret(ME_TextEditor *ed);
int ME_CharFromPos(ME_TextEditor *editor, int x, int y, BOOL *isExact);
void ME_LButtonDown(ME_TextEditor *editor, int x, int y, int clickNum);
void ME_MouseMove(ME_TextEditor *editor, int x, int y);
BOOL ME_DeleteTextAtCursor(ME_TextEditor *editor, int nCursor, int nChars);
void ME_InsertTextFromCursor(ME_TextEditor *editor, int nCursor, 
                             const WCHAR *str, int len, ME_Style *style);
void ME_InsertEndRowFromCursor(ME_TextEditor *editor, int nCursor);
BOOL ME_ArrowKey(ME_TextEditor *ed, int nVKey, BOOL extend, BOOL ctrl);

void ME_MustBeWrapped(ME_Context *c, ME_DisplayItem *para);
void ME_GetCursorCoordinates(ME_TextEditor *editor, ME_Cursor *pCursor,
                             int *x, int *y, int *height);
int ME_GetCursorOfs(ME_TextEditor *editor, int nCursor);
void ME_GetSelection(ME_TextEditor *editor, int *from, int *to);
int ME_CountParagraphsBetween(ME_TextEditor *editor, int from, int to);
BOOL ME_IsSelection(ME_TextEditor *editor);
void ME_DeleteSelection(ME_TextEditor *editor);
void ME_SendSelChange(ME_TextEditor *editor);
void ME_InsertOLEFromCursor(ME_TextEditor *editor, const REOBJECT* reo, int nCursor);
BOOL ME_InternalDeleteText(ME_TextEditor *editor, int nOfs, int nChars, BOOL bForce);
int ME_GetTextLength(ME_TextEditor *editor);
int ME_GetTextLengthEx(ME_TextEditor *editor, const GETTEXTLENGTHEX *how);
ME_Style *ME_GetSelectionInsertStyle(ME_TextEditor *editor);

/* context.c */
void ME_InitContext(ME_Context *c, ME_TextEditor *editor, HDC hDC);
void ME_DestroyContext(ME_Context *c);

/* wrap.c */
BOOL ME_WrapMarkedParagraphs(ME_TextEditor *editor);
void ME_InvalidateMarkedParagraphs(ME_TextEditor *editor);
void ME_SendRequestResize(ME_TextEditor *editor, BOOL force);

/* para.c */
ME_DisplayItem *ME_GetParagraph(ME_DisplayItem *run);
void ME_GetSelectionParas(ME_TextEditor *editor, ME_DisplayItem **para, ME_DisplayItem **para_end);
void ME_MakeFirstParagraph(ME_TextEditor *editor);
ME_DisplayItem *ME_SplitParagraph(ME_TextEditor *editor, ME_DisplayItem *rp, ME_Style *style, ME_String *eol_str, int paraFlags);
ME_DisplayItem *ME_JoinParagraphs(ME_TextEditor *editor, ME_DisplayItem *tp,
                                  BOOL keepFirstParaFormat);
void ME_DumpParaStyle(ME_Paragraph *s);
void ME_DumpParaStyleToBuf(const PARAFORMAT2 *pFmt, char buf[2048]);
BOOL ME_SetSelectionParaFormat(ME_TextEditor *editor, const PARAFORMAT2 *pFmt);
void ME_GetSelectionParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt);
/* marks from first up to (but not including) last */
void ME_MarkForPainting(ME_TextEditor *editor, ME_DisplayItem *first, const ME_DisplayItem *last);
void ME_MarkAllForWrapping(ME_TextEditor *editor);
void ME_SetDefaultParaFormat(PARAFORMAT2 *pFmt);

/* paint.c */
void ME_PaintContent(ME_TextEditor *editor, HDC hDC, BOOL bOnlyNew, const RECT *rcUpdate);
void ME_Repaint(ME_TextEditor *editor);
void ME_RewrapRepaint(ME_TextEditor *editor);
void ME_UpdateRepaint(ME_TextEditor *editor);
void ME_EnsureVisible(ME_TextEditor *editor, ME_Cursor *pCursor);
void ME_InvalidateSelection(ME_TextEditor *editor);
BOOL ME_SetZoom(ME_TextEditor *editor, int numerator, int denominator);
int  ME_twips2pointsX(ME_Context *c, int x);
int  ME_twips2pointsY(ME_Context *c, int y);

/* scroll functions in paint.c */

void ME_ScrollAbs(ME_TextEditor *editor, int x, int y);
void ME_HScrollAbs(ME_TextEditor *editor, int x);
void ME_VScrollAbs(ME_TextEditor *editor, int y);
void ME_ScrollUp(ME_TextEditor *editor, int cy);
void ME_ScrollDown(ME_TextEditor *editor, int cy);
void ME_ScrollLeft(ME_TextEditor *editor, int cx);
void ME_ScrollRight(ME_TextEditor *editor, int cx);
void ME_UpdateScrollBar(ME_TextEditor *editor);

/* other functions in paint.c */
int  ME_GetParaBorderWidth(ME_TextEditor *editor, int);
int  ME_GetParaLineSpace(ME_Context *c, ME_Paragraph*);

/* richole.c */
LRESULT CreateIRichEditOle(ME_TextEditor *editor, LPVOID *);
void ME_DrawOLE(ME_Context *c, int x, int y, ME_Run* run, ME_Paragraph *para, BOOL selected);
void ME_GetOLEObjectSize(ME_Context *c, ME_Run *run, SIZE *pSize);
void ME_CopyReObject(REOBJECT* dst, const REOBJECT* src);
void ME_DeleteReObject(REOBJECT* reo);

/* editor.c */
ME_TextEditor *ME_MakeEditor(ITextHost *texthost, BOOL bEmulateVersion10);
LRESULT ME_HandleMessage(ME_TextEditor *editor, UINT msg, WPARAM wParam,
                         LPARAM lParam, BOOL unicode, HRESULT* phresult);
void ME_SendOldNotify(ME_TextEditor *editor, int nCode);
int ME_GetTextW(ME_TextEditor *editor, WCHAR *buffer, int nStart, int nChars, BOOL bCRLF);
void ME_RTFCharAttrHook(struct _RTF_Info *info);
void ME_RTFParAttrHook(struct _RTF_Info *info);
void ME_RTFTblAttrHook(struct _RTF_Info *info);
void ME_RTFSpecialCharHook(struct _RTF_Info *info);
void ME_StreamInFill(ME_InStream *stream);
extern int me_debug;
extern void DoWrap(ME_TextEditor *editor);

/* table.c */
BOOL ME_IsInTable(ME_DisplayItem *pItem);
ME_DisplayItem *ME_InsertTableRowStartFromCursor(ME_TextEditor *editor);
ME_DisplayItem *ME_InsertTableRowStartAtParagraph(ME_TextEditor *editor,
                                                  ME_DisplayItem *para);
ME_DisplayItem *ME_InsertTableCellFromCursor(ME_TextEditor *editor);
ME_DisplayItem *ME_InsertTableRowEndFromCursor(ME_TextEditor *editor);
ME_DisplayItem *ME_GetTableRowEnd(ME_DisplayItem *para);
ME_DisplayItem *ME_GetTableRowStart(ME_DisplayItem *para);
void ME_CheckTablesForCorruption(ME_TextEditor *editor);
void ME_ProtectPartialTableDeletion(ME_TextEditor *editor, int nOfs,int *nChars);
ME_DisplayItem* ME_AppendTableRow(ME_TextEditor *editor, ME_DisplayItem *table_row);
void ME_TabPressedInTable(ME_TextEditor *editor, BOOL bSelectedRow);
void ME_MoveCursorFromTableRowStartParagraph(ME_TextEditor *editor);
struct RTFTable *ME_MakeTableDef(ME_TextEditor *editor);
void ME_InitTableDef(ME_TextEditor *editor, struct RTFTable *tableDef);

/* txthost.c */
ITextHost *ME_CreateTextHost(HWND hwnd, BOOL bEmulateVersion10);
#ifdef __i386__ /* Use wrappers to perform thiscall on i386 */
#define TXTHOST_VTABLE(This) (&itextHostStdcallVtbl)
#else /* __i386__ */
#define TXTHOST_VTABLE(This) (This)->lpVtbl
#endif /* __i386__ */
 /*** ITextHost methods ***/
#define ITextHost_TxGetDC(This) TXTHOST_VTABLE(This)->TxGetDC(This)
#define ITextHost_TxReleaseDC(This,a) TXTHOST_VTABLE(This)->TxReleaseDC(This,a)
#define ITextHost_TxShowScrollBar(This,a,b) TXTHOST_VTABLE(This)->TxShowScrollBar(This,a,b)
#define ITextHost_TxEnableScrollBar(This,a,b) TXTHOST_VTABLE(This)->TxEnableScrollBar(This,a,b)
#define ITextHost_TxSetScrollRange(This,a,b,c,d) TXTHOST_VTABLE(This)->TxSetScrollRange(This,a,b,c,d)
#define ITextHost_TxSetScrollPos(This,a,b,c) TXTHOST_VTABLE(This)->TxSetScrollPos(This,a,b,c)
#define ITextHost_TxInvalidateRect(This,a,b) TXTHOST_VTABLE(This)->TxInvalidateRect(This,a,b)
#define ITextHost_TxViewChange(This,a) TXTHOST_VTABLE(This)->TxViewChange(This,a)
#define ITextHost_TxCreateCaret(This,a,b,c) TXTHOST_VTABLE(This)->TxCreateCaret(This,a,b,c)
#define ITextHost_TxShowCaret(This,a) TXTHOST_VTABLE(This)->TxShowCaret(This,a)
#define ITextHost_TxSetCaretPos(This,a,b) TXTHOST_VTABLE(This)->TxSetCaretPos(This,a,b)
#define ITextHost_TxSetTimer(This,a,b) TXTHOST_VTABLE(This)->TxSetTimer(This,a,b)
#define ITextHost_TxKillTimer(This,a) TXTHOST_VTABLE(This)->TxKillTimer(This,a)
#define ITextHost_TxScrollWindowEx(This,a,b,c,d,e,f,g) TXTHOST_VTABLE(This)->TxScrollWindowEx(This,a,b,c,d,e,f,g)
#define ITextHost_TxSetCapture(This,a) TXTHOST_VTABLE(This)->TxSetCapture(This,a)
#define ITextHost_TxSetFocus(This) TXTHOST_VTABLE(This)->TxSetFocus(This)
#define ITextHost_TxSetCursor(This,a,b) TXTHOST_VTABLE(This)->TxSetCursor(This,a,b)
#define ITextHost_TxScreenToClient(This,a) TXTHOST_VTABLE(This)->TxScreenToClient(This,a)
#define ITextHost_TxClientToScreen(This,a) TXTHOST_VTABLE(This)->TxClientToScreen(This,a)
#define ITextHost_TxActivate(This,a) TXTHOST_VTABLE(This)->TxActivate(This,a)
#define ITextHost_TxDeactivate(This,a) TXTHOST_VTABLE(This)->TxDeactivate(This,a)
#define ITextHost_TxGetClientRect(This,a) TXTHOST_VTABLE(This)->TxGetClientRect(This,a)
#define ITextHost_TxGetViewInset(This,a) TXTHOST_VTABLE(This)->TxGetViewInset(This,a)
#define ITextHost_TxGetCharFormat(This,a) TXTHOST_VTABLE(This)->TxGetCharFormat(This,a)
#define ITextHost_TxGetParaFormat(This,a) TXTHOST_VTABLE(This)->TxGetParaFormat(This,a)
#define ITextHost_TxGetSysColor(This,a) TXTHOST_VTABLE(This)->TxGetSysColor(This,a)
#define ITextHost_TxGetBackStyle(This,a) TXTHOST_VTABLE(This)->TxGetBackStyle(This,a)
#define ITextHost_TxGetMaxLength(This,a) TXTHOST_VTABLE(This)->TxGetMaxLength(This,a)
#define ITextHost_TxGetScrollBars(This,a) TXTHOST_VTABLE(This)->TxGetScrollBars(This,a)
#define ITextHost_TxGetPasswordChar(This,a) TXTHOST_VTABLE(This)->TxGetPasswordChar(This,a)
#define ITextHost_TxGetAcceleratorPos(This,a) TXTHOST_VTABLE(This)->TxGetAcceleratorPos(This,a)
#define ITextHost_TxGetExtent(This,a) TXTHOST_VTABLE(This)->TxGetExtent(This,a)
#define ITextHost_OnTxCharFormatChange(This,a) TXTHOST_VTABLE(This)->OnTxCharFormatChange(This,a)
#define ITextHost_OnTxParaFormatChange(This,a) TXTHOST_VTABLE(This)->OnTxParaFormatChange(This,a)
#define ITextHost_TxGetPropertyBits(This,a,b) TXTHOST_VTABLE(This)->TxGetPropertyBits(This,a,b)
#define ITextHost_TxNotify(This,a,b) TXTHOST_VTABLE(This)->TxNotify(This,a,b)
#define ITextHost_TxImmGetContext(This) TXTHOST_VTABLE(This)->TxImmGetContext(This)
#define ITextHost_TxImmReleaseContext(This,a) TXTHOST_VTABLE(This)->TxImmReleaseContext(This,a)
#define ITextHost_TxGetSelectionBarWidth(This,a) TXTHOST_VTABLE(This)->TxGetSelectionBarWidth(This,a)

/* undo.c */
ME_UndoItem *ME_AddUndoItem(ME_TextEditor *editor, ME_DIType type, const ME_DisplayItem *pdi);
void ME_CommitUndo(ME_TextEditor *editor);
void ME_ContinueCoalescingTransaction(ME_TextEditor *editor);
void ME_CommitCoalescingUndo(ME_TextEditor *editor);
BOOL ME_Undo(ME_TextEditor *editor);
BOOL ME_Redo(ME_TextEditor *editor);
void ME_EmptyUndoStack(ME_TextEditor *editor);

/* writer.c */
LRESULT ME_StreamOutRange(ME_TextEditor *editor, DWORD dwFormat, int nStart, int nTo, EDITSTREAM *stream);
LRESULT ME_StreamOut(ME_TextEditor *editor, DWORD dwFormat, EDITSTREAM *stream);

/* clipboard.c */
HRESULT ME_GetDataObject(ME_TextEditor *editor, const CHARRANGE *lpchrg, LPDATAOBJECT *lplpdataobj);
