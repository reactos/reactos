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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "editstr.h"

#define ALLOC_OBJ(type) (type *)HeapAlloc(me_heap, 0, sizeof(type))
#define ALLOC_N_OBJ(type, count) (type *)HeapAlloc(me_heap, 0, count*sizeof(type))
#define FREE_OBJ(ptr) HeapFree(me_heap, 0, ptr)

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
void ME_DumpDocument(ME_TextBuffer *buffer);
const char *ME_GetDITypeName(ME_DIType type);

/* string.c */
int ME_GetOptimalBuffer(int nLen);
ME_String *ME_MakeString(LPCWSTR szText);
ME_String *ME_MakeStringN(LPCWSTR szText, int nMaxChars);
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
int ME_GetCharFwd(ME_String *s, int nPos); /* get char starting from start */
int ME_GetCharBack(ME_String *s, int nPos); /* get char starting from \0  */
int ME_StrRelPos(ME_String *s, int nVChar, int *pRelChars);
int ME_StrRelPos2(ME_String *s, int nVChar, int nRelChars);
int ME_VPosToPos(ME_String *s, int nVPos);
int ME_PosToVPos(ME_String *s, int nPos);
void ME_StrDeleteV(ME_String *s, int nVChar, int nChars);
/* smart helpers for A<->W conversions, they reserve/free memory and call MultiByte<->WideChar functions */
LPWSTR ME_ToUnicode(HWND hWnd, LPVOID psz);
void ME_EndToUnicode(HWND hWnd, LPVOID psz);
LPSTR ME_ToAnsi(HWND hWnd, LPVOID psz);
void ME_EndToAnsi(HWND hWnd, LPVOID psz);


/* note: those two really return the first matching offset (starting from EOS)+1 
 * in other words, an offset of the first trailing white/black */
int ME_ReverseFindNonWhitespaceV(ME_String *s, int nVChar);
int ME_ReverseFindWhitespaceV(ME_String *s, int nVChar);

/* row.c */
ME_DisplayItem *ME_FindRowStart(ME_Context *c, ME_DisplayItem *run, int nRelPos);
ME_DisplayItem *ME_RowStart(ME_DisplayItem *item);
ME_DisplayItem *ME_RowEnd(ME_DisplayItem *item);
void ME_RenumberParagraphs(ME_DisplayItem *item); /* TODO */

/* run.c */
ME_DisplayItem *ME_MakeRun(ME_Style *s, ME_String *strData, int nFlags);
/* note: ME_InsertRun inserts a copy of the specified run - so you need to destroy the original */
ME_DisplayItem *ME_InsertRun(ME_TextEditor *editor, int nCharOfs, ME_DisplayItem *pItem);
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
void ME_CalcRunExtent(ME_Context *c, ME_Run *run);
SIZE ME_GetRunSize(ME_Context *c, ME_Run *run, int nLen);
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
void ME_SetSelection(ME_TextEditor *editor, int from, int to);
void ME_HideCaret(ME_TextEditor *ed);
void ME_ShowCaret(ME_TextEditor *ed);
void ME_MoveCaret(ME_TextEditor *ed);
int ME_FindPixelPos(ME_TextEditor *editor, int x, int y, ME_Cursor *result, BOOL *is_eol);
void ME_LButtonDown(ME_TextEditor *editor, int x, int y);
void ME_MouseMove(ME_TextEditor *editor, int x, int y);
void ME_DeleteTextAtCursor(ME_TextEditor *editor, int nCursor, int nChars);
void ME_InsertTextFromCursor(ME_TextEditor *editor, int nCursor, 
                             const WCHAR *str, int len, ME_Style *style);
void ME_SetCharFormat(ME_TextEditor *editor, int nOfs, int nChars, CHARFORMAT2W *pFmt);
BOOL ME_ArrowKey(ME_TextEditor *ed, int nVKey, int nCtrl);

void ME_InitContext(ME_Context *c, ME_TextEditor *editor, HDC hDC);
void ME_DestroyContext(ME_Context *c);
ME_Style *GetInsertStyle(ME_TextEditor *editor, int nCursor);
void ME_MustBeWrapped(ME_Context *c, ME_DisplayItem *para);
int ME_GetCursorOfs(ME_TextEditor *editor, int nCursor);
void ME_GetSelection(ME_TextEditor *editor, int *from, int *to);
int ME_CountParagraphsBetween(ME_TextEditor *editor, int from, int to);
BOOL ME_IsSelection(ME_TextEditor *editor);
void ME_DeleteSelection(ME_TextEditor *editor);
void ME_SendSelChange(ME_TextEditor *editor);
void ME_InsertGraphicsFromCursor(ME_TextEditor *editor, int nCursor);
void ME_InternalDeleteText(ME_TextEditor *editor, int nOfs, int nChars);
int ME_GetTextLength(ME_TextEditor *editor);
ME_Style *ME_GetSelectionInsertStyle(ME_TextEditor *editor);

/* wrap.c */
void ME_PrepareParagraphForWrapping(ME_Context *c, ME_DisplayItem *tp);
ME_DisplayItem *ME_MakeRow(int height, int baseline, int width);
void ME_InsertRowStart(ME_WrapContext *wc, ME_DisplayItem *pEnd);
void ME_WrapTextParagraph(ME_Context *c, ME_DisplayItem *tp);
void ME_WrapMarkedParagraphs(ME_TextEditor *editor);

/* para.c */
ME_DisplayItem *ME_GetParagraph(ME_DisplayItem *run); 
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
void ME_MarkAllForWrapping(ME_TextEditor *editor);

/* paint.c */
void ME_PaintContent(ME_TextEditor *editor, HDC hDC, BOOL bOnlyNew, RECT *rcUpdate);
void ME_Repaint(ME_TextEditor *editor);
void ME_UpdateRepaint(ME_TextEditor *editor);
void ME_DrawParagraph(ME_Context *c, ME_DisplayItem *paragraph);
void ME_UpdateScrollBar(ME_TextEditor *editor, int ypos);
int ME_GetScrollPos(ME_TextEditor *editor);
void ME_EnsureVisible(ME_TextEditor *editor, ME_DisplayItem *pRun);
COLORREF ME_GetBackColor(ME_TextEditor *editor);

/* richole.c */
extern LRESULT CreateIRichEditOle(LPVOID *);

/* wintest.c */

/* editor.c */
void ME_RegisterEditorClass();
ME_TextEditor *ME_MakeEditor(HWND hWnd);
void ME_DestroyEditor(ME_TextEditor *editor);
void ME_SendOldNotify(ME_TextEditor *editor, int nCode);
ME_UndoItem *ME_AddUndoItem(ME_TextEditor *editor, ME_DIType type, ME_DisplayItem *di);
void ME_CommitUndo(ME_TextEditor *editor);
void ME_Undo(ME_TextEditor *editor);
void ME_Redo(ME_TextEditor *editor);
void ME_EmptyUndoStack(ME_TextEditor *editor);
int ME_GetTextW(ME_TextEditor *editor, WCHAR *buffer, int nStart, int nChars, BOOL bCRLF);

extern int me_debug;
extern HANDLE me_heap;
extern void DoWrap(ME_TextEditor *editor);
