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

extern HANDLE me_heap;

static inline void *richedit_alloc( size_t len )
{
    return HeapAlloc( me_heap, 0, len );
}

static inline BOOL richedit_free( void *ptr )
{
    return HeapFree( me_heap, 0, ptr );
}

static inline void *richedit_realloc( void *ptr, size_t len )
{
    return HeapReAlloc( me_heap, 0, ptr, len );
}

#define ALLOC_OBJ(type) richedit_alloc(sizeof(type))
#define ALLOC_N_OBJ(type, count) richedit_alloc((count)*sizeof(type))
#define FREE_OBJ(ptr) richedit_free(ptr)

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
HFONT ME_SelectStyleFont(ME_TextEditor *editor, HDC hDC, ME_Style *s);
void ME_UnselectStyleFont(ME_TextEditor *editor, HDC hDC, ME_Style *s, HFONT hOldFont);
void ME_InitCharFormat2W(CHARFORMAT2W *pFmt);
void ME_SaveTempStyle(ME_TextEditor *editor);
void ME_ClearTempStyle(ME_TextEditor *editor);
void ME_DumpStyleToBuf(CHARFORMAT2W *pFmt, char buf[2048]);
void ME_DumpStyle(ME_Style *s);
CHARFORMAT2W *ME_ToCF2W(CHARFORMAT2W *to, CHARFORMAT2W *from);
void ME_CopyToCF2W(CHARFORMAT2W *to, CHARFORMAT2W *from);
CHARFORMAT2W *ME_ToCFAny(CHARFORMAT2W *to, CHARFORMAT2W *from);
void ME_CopyToCFAny(CHARFORMAT2W *to, CHARFORMAT2W *from);
void ME_CopyCharFormat(CHARFORMAT2W *pDest, CHARFORMAT2W *pSrc); /* only works with 2W structs */
void ME_CharFormatFromLogFont(HDC hDC, LOGFONTW *lf, CHARFORMAT2W *fmt); /* ditto */

/* list.c */
void ME_InsertBefore(ME_DisplayItem *diWhere, ME_DisplayItem *diWhat);
void ME_Remove(ME_DisplayItem *diWhere);
ME_DisplayItem *ME_FindItemBack(ME_DisplayItem *di, ME_DIType nTypeOrClass);
ME_DisplayItem *ME_FindItemFwd(ME_DisplayItem *di, ME_DIType nTypeOrClass);
ME_DisplayItem *ME_FindItemBackOrHere(ME_DisplayItem *di, ME_DIType nTypeOrClass);
ME_DisplayItem *ME_FindItemFwdOrHere(ME_DisplayItem *di, ME_DIType nTypeOrClass);
BOOL ME_DITypesEqual(ME_DIType type, ME_DIType nTypeOrClass);
ME_DisplayItem *ME_MakeDI(ME_DIType type);
void ME_DestroyDisplayItem(ME_DisplayItem *item);
void ME_DestroyTableCellList(ME_DisplayItem *item);
void ME_DumpDocument(ME_TextBuffer *buffer);
const char *ME_GetDITypeName(ME_DIType type);

/* string.c */
int ME_GetOptimalBuffer(int nLen);
ME_String *ME_MakeString(LPCWSTR szText);
ME_String *ME_MakeStringN(LPCWSTR szText, int nMaxChars);
ME_String *ME_MakeStringR(WCHAR cRepeat, int nMaxChars);
ME_String *ME_MakeStringB(int nMaxChars);
ME_String *ME_StrDup(ME_String *s);
void ME_DestroyString(ME_String *s);
void ME_AppendString(ME_String *s1, ME_String *s2);
ME_String *ME_ConcatString(ME_String *s1, ME_String *s2);
ME_String *ME_VSplitString(ME_String *orig, int nVPos);
int ME_IsWhitespaces(ME_String *s);
int ME_IsSplitable(ME_String *s);
/* int ME_CalcSkipChars(ME_String *s); */
int ME_StrLen(ME_String *s);
int ME_StrVLen(ME_String *s);
int ME_FindNonWhitespaceV(ME_String *s, int nVChar);
int ME_FindWhitespaceV(ME_String *s, int nVChar);
int ME_CallWordBreakProc(ME_TextEditor *editor, ME_String *str, INT start, INT code);
int ME_GetCharFwd(ME_String *s, int nPos); /* get char starting from start */
int ME_GetCharBack(ME_String *s, int nPos); /* get char starting from \0  */
int ME_StrRelPos(ME_String *s, int nVChar, int *pRelChars);
int ME_StrRelPos2(ME_String *s, int nVChar, int nRelChars);
int ME_VPosToPos(ME_String *s, int nVPos);
int ME_PosToVPos(ME_String *s, int nPos);
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
int ME_ReverseFindNonWhitespaceV(ME_String *s, int nVChar);
int ME_ReverseFindWhitespaceV(ME_String *s, int nVChar);

/* row.c */
ME_DisplayItem *ME_FindRowStart(ME_Context *c, ME_DisplayItem *run, int nRelPos);
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
void ME_GetGraphicsSize(ME_TextEditor *editor, ME_Run *run, SIZE *pSize);
int ME_CharFromPoint(ME_TextEditor *editor, int cx, ME_Run *run);
/* this one accounts for 1/2 char tolerance */
int ME_CharFromPointCursor(ME_TextEditor *editor, int cx, ME_Run *run);
int ME_PointFromChar(ME_TextEditor *editor, ME_Run *pRun, int nOffset);
int ME_GetLastSplittablePlace(ME_Context *c, ME_Run *run);
int ME_CanJoinRuns(ME_Run *run1, ME_Run *run2);
void ME_JoinRuns(ME_TextEditor *editor, ME_DisplayItem *p);
ME_DisplayItem *ME_SplitRun(ME_Context *c, ME_DisplayItem *item, int nChar);
ME_DisplayItem *ME_SplitRunSimple(ME_TextEditor *editor, ME_DisplayItem *item, int nChar);
int ME_FindSplitPoint(ME_Context *c, POINT *pt, ME_Run *run, int desperate);
void ME_UpdateRunFlags(ME_TextEditor *editor, ME_Run *run);
ME_DisplayItem *ME_SplitFurther(ME_TextEditor *editor, ME_DisplayItem *run);
void ME_CalcRunExtent(ME_Context *c, ME_Paragraph *para, ME_Run *run);
SIZE ME_GetRunSize(ME_Context *c, ME_Paragraph *para, ME_Run *run, int nLen);
void ME_CursorFromCharOfs(ME_TextEditor *editor, int nCharOfs, ME_Cursor *pCursor);
void ME_RunOfsFromCharOfs(ME_TextEditor *editor, int nCharOfs, ME_DisplayItem **ppRun, int *pOfs);
int ME_CharOfsFromRunOfs(ME_TextEditor *editor, ME_DisplayItem *pRun, int nOfs);
void ME_SkipAndPropagateCharOffset(ME_DisplayItem *p, int shift);
void ME_SetCharFormat(ME_TextEditor *editor, int nFrom, int nLen, CHARFORMAT2W *pFmt);
void ME_SetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt);
void ME_GetCharFormat(ME_TextEditor *editor, int nFrom, int nLen, CHARFORMAT2W *pFmt);
void ME_GetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt);
void ME_GetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt);
void ME_SetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *mod);

/* caret.c */
int ME_SetSelection(ME_TextEditor *editor, int from, int to);
void ME_SelectWord(ME_TextEditor *editor);
void ME_HideCaret(ME_TextEditor *ed);
void ME_ShowCaret(ME_TextEditor *ed);
void ME_MoveCaret(ME_TextEditor *ed);
int ME_FindPixelPos(ME_TextEditor *editor, int x, int y, ME_Cursor *result, BOOL *is_eol);
int ME_CharFromPos(ME_TextEditor *editor, int x, int y);
void ME_LButtonDown(ME_TextEditor *editor, int x, int y);
void ME_MouseMove(ME_TextEditor *editor, int x, int y);
void ME_DeleteTextAtCursor(ME_TextEditor *editor, int nCursor, int nChars);
void ME_InsertTextFromCursor(ME_TextEditor *editor, int nCursor, 
                             const WCHAR *str, int len, ME_Style *style);
BOOL ME_ArrowKey(ME_TextEditor *ed, int nVKey, BOOL extend, BOOL ctrl);

void ME_InitContext(ME_Context *c, ME_TextEditor *editor, HDC hDC);
void ME_DestroyContext(ME_Context *c);
ME_Style *GetInsertStyle(ME_TextEditor *editor, int nCursor);
void ME_MustBeWrapped(ME_Context *c, ME_DisplayItem *para);
void ME_GetCursorCoordinates(ME_TextEditor *editor, ME_Cursor *pCursor,
                             int *x, int *y, int *height);
int ME_GetCursorOfs(ME_TextEditor *editor, int nCursor);
void ME_GetSelection(ME_TextEditor *editor, int *from, int *to);
int ME_CountParagraphsBetween(ME_TextEditor *editor, int from, int to);
BOOL ME_IsSelection(ME_TextEditor *editor);
void ME_DeleteSelection(ME_TextEditor *editor);
void ME_SendSelChange(ME_TextEditor *editor);
void ME_InsertGraphicsFromCursor(ME_TextEditor *editor, int nCursor);
void ME_InsertTableCellFromCursor(ME_TextEditor *editor, int nCursor);
void ME_InternalDeleteText(ME_TextEditor *editor, int nOfs, int nChars);
int ME_GetTextLength(ME_TextEditor *editor);
int ME_GetTextLengthEx(ME_TextEditor *editor, GETTEXTLENGTHEX *how);
ME_Style *ME_GetSelectionInsertStyle(ME_TextEditor *editor);
BOOL ME_UpdateSelection(ME_TextEditor *editor, ME_Cursor *pTempCursor);

/* wrap.c */
void ME_PrepareParagraphForWrapping(ME_Context *c, ME_DisplayItem *tp);
ME_DisplayItem *ME_MakeRow(int height, int baseline, int width);
void ME_InsertRowStart(ME_WrapContext *wc, ME_DisplayItem *pEnd);
void ME_WrapTextParagraph(ME_Context *c, ME_DisplayItem *tp);
BOOL ME_WrapMarkedParagraphs(ME_TextEditor *editor);
void ME_InvalidateMarkedParagraphs(ME_TextEditor *editor);
void ME_SendRequestResize(ME_TextEditor *editor, BOOL force);

/* para.c */
ME_DisplayItem *ME_GetParagraph(ME_DisplayItem *run); 
void ME_GetSelectionParas(ME_TextEditor *editor, ME_DisplayItem **para, ME_DisplayItem **para_end);
void ME_MakeFirstParagraph(HDC hDC, ME_TextBuffer *editor);
ME_DisplayItem *ME_SplitParagraph(ME_TextEditor *editor, ME_DisplayItem *rp, ME_Style *style);
ME_DisplayItem *ME_JoinParagraphs(ME_TextEditor *editor, ME_DisplayItem *tp);
void ME_DumpParaStyle(ME_Paragraph *s);
void ME_DumpParaStyleToBuf(PARAFORMAT2 *pFmt, char buf[2048]);
void ME_SetParaFormat(ME_TextEditor *editor, ME_DisplayItem *para, PARAFORMAT2 *pFmt);
void ME_SetSelectionParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt);
void ME_GetParaFormat(ME_TextEditor *editor, ME_DisplayItem *para, PARAFORMAT2 *pFmt);
void ME_GetSelectionParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt);
/* marks from first up to (but not including) last */
void ME_MarkForWrapping(ME_TextEditor *editor, ME_DisplayItem *first, ME_DisplayItem *last);
void ME_MarkForPainting(ME_TextEditor *editor, ME_DisplayItem *first, ME_DisplayItem *last);
void ME_MarkAllForWrapping(ME_TextEditor *editor);

/* paint.c */
void ME_PaintContent(ME_TextEditor *editor, HDC hDC, BOOL bOnlyNew, RECT *rcUpdate);
void ME_Repaint(ME_TextEditor *editor);
void ME_RewrapRepaint(ME_TextEditor *editor);
void ME_UpdateRepaint(ME_TextEditor *editor);
void ME_DrawParagraph(ME_Context *c, ME_DisplayItem *paragraph);
void ME_EnsureVisible(ME_TextEditor *editor, ME_DisplayItem *pRun);
COLORREF ME_GetBackColor(ME_TextEditor *editor);
void ME_InvalidateSelection(ME_TextEditor *editor);
void ME_QueueInvalidateFromCursor(ME_TextEditor *editor, int nCursor);
BOOL ME_SetZoom(ME_TextEditor *editor, int numerator, int denominator);

/* scroll functions in paint.c */

void ME_ScrollAbs(ME_TextEditor *editor, int absY);
void ME_ScrollUp(ME_TextEditor *editor, int cy);
void ME_ScrollDown(ME_TextEditor *editor, int cy);
void ME_Scroll(ME_TextEditor *editor, int value, int type);
void ME_UpdateScrollBar(ME_TextEditor *editor);
int ME_GetYScrollPos(ME_TextEditor *editor);
BOOL ME_GetYScrollVisible(ME_TextEditor *editor);

/* richole.c */
extern LRESULT CreateIRichEditOle(ME_TextEditor *editor, LPVOID *);

/* wintest.c */

/* editor.c */
ME_TextEditor *ME_MakeEditor(HWND hWnd);
void ME_DestroyEditor(ME_TextEditor *editor);
void ME_SendOldNotify(ME_TextEditor *editor, int nCode);
void ME_LinkNotify(ME_TextEditor *editor, UINT msg, WPARAM wParam, LPARAM lParam);
ME_UndoItem *ME_AddUndoItem(ME_TextEditor *editor, ME_DIType type, ME_DisplayItem *di);
void ME_CommitUndo(ME_TextEditor *editor);
void ME_Undo(ME_TextEditor *editor);
void ME_Redo(ME_TextEditor *editor);
void ME_EmptyUndoStack(ME_TextEditor *editor);
int ME_GetTextW(ME_TextEditor *editor, WCHAR *buffer, int nStart, int nChars, BOOL bCRLF);
ME_DisplayItem *ME_FindItemAtOffset(ME_TextEditor *editor, ME_DIType nItemType, int nOffset, int *nItemOffset);
void ME_StreamInFill(ME_InStream *stream);
int ME_AutoURLDetect(ME_TextEditor *editor, WCHAR curChar);
extern int me_debug;
extern void DoWrap(ME_TextEditor *editor);

/* writer.c */
LRESULT ME_StreamOutRange(ME_TextEditor *editor, DWORD dwFormat, int nStart, int nTo, EDITSTREAM *stream);
LRESULT ME_StreamOut(ME_TextEditor *editor, DWORD dwFormat, EDITSTREAM *stream);

/* clipboard.c */
HRESULT ME_GetDataObject(ME_TextEditor *editor, CHARRANGE *lpchrg, LPDATAOBJECT *lplpdataobj);
