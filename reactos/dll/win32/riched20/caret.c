/*
 * RichEdit - Caret and selection functions.
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2005 by Phil Krylov
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


#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

static BOOL
ME_MoveCursorChars(ME_TextEditor *editor, ME_Cursor *pCursor, int nRelOfs);


void ME_GetSelection(ME_TextEditor *editor, int *from, int *to)
{
  *from = ME_GetCursorOfs(editor, 0);
  *to =   ME_GetCursorOfs(editor, 1);
  
  if (*from > *to)
  {
    int tmp = *from;
    *from = *to;
    *to = tmp;    
  }
}

int ME_GetTextLength(ME_TextEditor *editor)
{
  return ME_CharOfsFromRunOfs(editor, ME_FindItemBack(editor->pBuffer->pLast, diRun), 0);   
}


int ME_GetTextLengthEx(ME_TextEditor *editor, GETTEXTLENGTHEX *how)
{
  int length;
  
  if (how->flags & GTL_PRECISE && how->flags & GTL_CLOSE)
    return E_INVALIDARG;
  if (how->flags & GTL_NUMCHARS && how->flags & GTL_NUMBYTES)
    return E_INVALIDARG;
  
  length = ME_GetTextLength(editor);
  
  if (how->flags & GTL_USECRLF)
    length += editor->nParagraphs;
  
  if (how->flags & GTL_NUMBYTES)
  {
    CPINFO cpinfo;
    
    if (how->codepage == 1200)
      return length * 2;
    if (how->flags & GTL_PRECISE)
      FIXME("GTL_PRECISE flag unsupported. Using GTL_CLOSE\n");
    if (GetCPInfo(how->codepage, &cpinfo))
      return length * cpinfo.MaxCharSize;
    ERR("Invalid codepage %u\n", how->codepage);
    return E_INVALIDARG;
  }
  return length; 
}


int ME_SetSelection(ME_TextEditor *editor, int from, int to)
{
  int selectionEnd = 0;
  const int len = ME_GetTextLength(editor);

  /* all negative values are effectively the same */
  if (from < 0)
    from = -1;
  if (to < 0)
    to = -1;

  /* select all */
  if (from == 0 && to == -1)
  {
    editor->pCursors[1].pRun = ME_FindItemFwd(editor->pBuffer->pFirst, diRun);
    editor->pCursors[1].nOffset = 0; 
    editor->pCursors[0].pRun = ME_FindItemBack(editor->pBuffer->pLast, diRun); 
    editor->pCursors[0].nOffset = 0;
    ME_InvalidateSelection(editor);
    ME_ClearTempStyle(editor);
    return len + 1;
  }

  /* if both values are equal and also out of bound, that means to */
  /* put the selection at the end of the text */
  if ((from == to) && (to < 0 || to > len))
  {
    selectionEnd = 1;
  }
  else
  {
    /* if from is negative and to is positive then selection is */
    /* deselected and caret moved to end of the current selection */
    if (from < 0)
    {
      int start, end;
      ME_GetSelection(editor, &start, &end);
      editor->pCursors[1] = editor->pCursors[0];
      ME_Repaint(editor);
      ME_ClearTempStyle(editor);
      return end;
    }

    /* adjust to if it's a negative value */
    if (to < 0)
      to = len + 1;

    /* flip from and to if they are reversed */
    if (from>to)
    {
      int tmp = from;
      from = to;
      to = tmp;
    }

    /* after fiddling with the values, we find from > len && to > len */
    if (from > len)
      selectionEnd = 1;
    /* special case with to too big */
    else if (to > len)
      to = len + 1;
  }

  if (selectionEnd)
  {
    editor->pCursors[1].pRun = editor->pCursors[0].pRun = ME_FindItemBack(editor->pBuffer->pLast, diRun);
    editor->pCursors[1].nOffset = editor->pCursors[0].nOffset = 0;
    ME_InvalidateSelection(editor);
    ME_ClearTempStyle(editor);
    return len;
  }

  ME_RunOfsFromCharOfs(editor, from, &editor->pCursors[1].pRun, &editor->pCursors[1].nOffset);
  ME_RunOfsFromCharOfs(editor, to, &editor->pCursors[0].pRun, &editor->pCursors[0].nOffset);
  return to;
}


void
ME_GetCursorCoordinates(ME_TextEditor *editor, ME_Cursor *pCursor,
                        int *x, int *y, int *height)
{
  ME_DisplayItem *pCursorRun = pCursor->pRun;
  ME_DisplayItem *pSizeRun = pCursor->pRun;

  assert(!pCursor->nOffset || !editor->bCaretAtEnd);
  assert(height && x && y);
  assert(!(ME_GetParagraph(pCursorRun)->member.para.nFlags & MEPF_REWRAP));
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
  
  if (pCursorRun->type == diRun) {
    ME_DisplayItem *row = ME_FindItemBack(pCursorRun, diStartRowOrParagraph);

    if (row) {
      HDC hDC = GetDC(editor->hWnd);
      ME_Context c;
      ME_DisplayItem *run = pCursorRun;
      ME_DisplayItem *para = NULL;
      SIZE sz = {0, 0};
    
      ME_InitContext(&c, editor, hDC);
      
      if (!pCursor->nOffset && !editor->bCaretAtEnd)
      {
        ME_DisplayItem *prev = ME_FindItemBack(pCursorRun, diRunOrStartRow);
        assert(prev);
        if (prev->type == diRun)
          pSizeRun = prev;
      }
      assert(row->type == diStartRow); /* paragraph -> run without start row ?*/
      para = ME_FindItemBack(row, diParagraph);
      assert(para);
      assert(para->type == diParagraph);
      if (editor->bCaretAtEnd && !pCursor->nOffset && 
          run == ME_FindItemFwd(row, diRun))
      {
        ME_DisplayItem *tmp = ME_FindItemBack(row, diRunOrParagraph);
        assert(tmp);
        if (tmp->type == diRun)
        {
          row = ME_FindItemBack(tmp, diStartRow);
          pSizeRun = run = tmp;
          assert(run);
          assert(run->type == diRun);
          sz = ME_GetRunSize(&c, &para->member.para, &run->member.run, ME_StrLen(run->member.run.strText));
        }
      }
      if (pCursor->nOffset && !(run->member.run.nFlags & MERF_SKIPPED)) {
        sz = ME_GetRunSize(&c, &para->member.para, &run->member.run, pCursor->nOffset);
      }

      *height = pSizeRun->member.run.nAscent + pSizeRun->member.run.nDescent;
      *x = run->member.run.pt.x + sz.cx;
      *y = para->member.para.nYPos + row->member.row.nBaseline + pSizeRun->member.run.pt.y - pSizeRun->member.run.nAscent - ME_GetYScrollPos(editor);
      
      ME_DestroyContext(&c);
      ReleaseDC(editor->hWnd, hDC);
      return;
    }
  }
  *height = 10; /* FIXME use global font */
  *x = 0;
  *y = 0;
}


void
ME_MoveCaret(ME_TextEditor *editor)
{
  int x, y, height;

  ME_WrapMarkedParagraphs(editor);
  ME_GetCursorCoordinates(editor, &editor->pCursors[0], &x, &y, &height);
  if(editor->bHaveFocus)
  {
    CreateCaret(editor->hWnd, NULL, 0, height);
    SetCaretPos(x, y);
  }
}


void ME_ShowCaret(ME_TextEditor *ed)
{
  ME_MoveCaret(ed);
  if(ed->bHaveFocus)
    ShowCaret(ed->hWnd);
}

void ME_HideCaret(ME_TextEditor *ed)
{
  if(ed->bHaveFocus)
  {
    HideCaret(ed->hWnd);
    DestroyCaret();
  }
}

void ME_InternalDeleteText(ME_TextEditor *editor, int nOfs, 
  int nChars)
{
  ME_Cursor c;
  int shift = 0;
  
  while(nChars > 0)
  {
    ME_Run *run;
    ME_CursorFromCharOfs(editor, nOfs, &c);
    run = &c.pRun->member.run;
    if (run->nFlags & MERF_ENDPARA) {
      if (!ME_FindItemFwd(c.pRun, diParagraph))
      {
        return;
      }
      ME_JoinParagraphs(editor, ME_GetParagraph(c.pRun));
      /* ME_SkipAndPropagateCharOffset(p->pRun, shift); */
      ME_CheckCharOffsets(editor);
      nChars--;
      if (editor->bEmulateVersion10 && nChars)
        nChars--;
      continue;
    }
    else
    {
      ME_Cursor cursor;
      int nIntendedChars = nChars;
      int nCharsToDelete = nChars;
      int i;
      int loc = c.nOffset;
      
      ME_FindItemBack(c.pRun, diParagraph)->member.para.nFlags |= MEPF_REWRAP;
      
      cursor = c;
      ME_StrRelPos(run->strText, loc, &nChars);
      /* nChars is the number of characters that should be deleted from the
         FOLLOWING runs (these AFTER cursor.pRun)
         nCharsToDelete is a number of chars to delete from THIS run */
      nCharsToDelete -= nChars;
      shift -= nCharsToDelete;
      TRACE("Deleting %d (intended %d-remaning %d) chars at %d in '%s' (%d)\n", 
        nCharsToDelete, nIntendedChars, nChars, c.nOffset, 
        debugstr_w(run->strText->szData), run->strText->nLen);

      if (!c.nOffset && ME_StrVLen(run->strText) == nCharsToDelete)
      {
        /* undo = reinsert whole run */
        /* nOfs is a character offset (from the start of the document
           to the current (deleted) run */
        ME_UndoItem *pUndo = ME_AddUndoItem(editor, diUndoInsertRun, c.pRun);
        if (pUndo)
          pUndo->di.member.run.nCharOfs = nOfs;
      }
      else
      {
        /* undo = reinsert partial run */
        ME_UndoItem *pUndo = ME_AddUndoItem(editor, diUndoInsertRun, c.pRun);
        if (pUndo) {
          ME_DestroyString(pUndo->di.member.run.strText);
          pUndo->di.member.run.nCharOfs = nOfs;
          pUndo->di.member.run.strText = ME_MakeStringN(run->strText->szData+c.nOffset, nCharsToDelete);
        }
      }
      TRACE("Post deletion string: %s (%d)\n", debugstr_w(run->strText->szData), run->strText->nLen);
      TRACE("Shift value: %d\n", shift);
      ME_StrDeleteV(run->strText, c.nOffset, nCharsToDelete);
      
      /* update cursors (including c) */
      for (i=-1; i<editor->nCursors; i++) {
        ME_Cursor *pThisCur = editor->pCursors + i; 
        if (i == -1) pThisCur = &c;
        if (pThisCur->pRun == cursor.pRun) {
          if (pThisCur->nOffset > cursor.nOffset) {
            if (pThisCur->nOffset-cursor.nOffset < nCharsToDelete)
              pThisCur->nOffset = cursor.nOffset;
            else
              pThisCur->nOffset -= nCharsToDelete;
            assert(pThisCur->nOffset >= 0);
            assert(pThisCur->nOffset <= ME_StrVLen(run->strText));
          }
          if (pThisCur->nOffset == ME_StrVLen(run->strText))
          {
            pThisCur->pRun = ME_FindItemFwd(pThisCur->pRun, diRunOrParagraphOrEnd);
            assert(pThisCur->pRun->type == diRun);
            pThisCur->nOffset = 0;
          }
        }
      }
      
      /* c = updated data now */
      
      if (c.pRun == cursor.pRun)
        ME_SkipAndPropagateCharOffset(c.pRun, shift);
      else
        ME_PropagateCharOffset(c.pRun, shift);

      if (!ME_StrVLen(cursor.pRun->member.run.strText))
      {
        TRACE("Removing useless run\n");
        ME_Remove(cursor.pRun);
        ME_DestroyDisplayItem(cursor.pRun);
      }
      
      shift = 0;
      /*
      ME_CheckCharOffsets(editor);
      */
      continue;
    }
  }
}

void ME_DeleteTextAtCursor(ME_TextEditor *editor, int nCursor, 
  int nChars)
{  
  assert(nCursor>=0 && nCursor<editor->nCursors);
  /* text operations set modified state */
  editor->nModifyStep = 1;
  ME_InternalDeleteText(editor, ME_GetCursorOfs(editor, nCursor), nChars);
}

static ME_DisplayItem *
ME_InternalInsertTextFromCursor(ME_TextEditor *editor, int nCursor,
                                const WCHAR *str, int len, ME_Style *style,
                                int flags)
{
  ME_Cursor *p = &editor->pCursors[nCursor];

  editor->bCaretAtEnd = FALSE;
  
  assert(p->pRun->type == diRun);
  
  return ME_InsertRunAtCursor(editor, p, style, str, len, flags);
}


/* FIXME this is temporary, just to have something to test how bad graphics handler is */
void ME_InsertGraphicsFromCursor(ME_TextEditor *editor, int nCursor)
{
  ME_Style *pStyle = ME_GetInsertStyle(editor, nCursor);
  WCHAR space = ' ';
  
  /* FIXME no no no */
  if (ME_IsSelection(editor))
    ME_DeleteSelection(editor);

  ME_InternalInsertTextFromCursor(editor, nCursor, &space, 1, pStyle,
                                  MERF_GRAPHICS);
  ME_SendSelChange(editor);
}


void
ME_InsertTableCellFromCursor(ME_TextEditor *editor, int nCursor)
{
  WCHAR tab = '\t';
  ME_DisplayItem *p, *run;
  ME_Style *pStyle = ME_GetInsertStyle(editor, nCursor);
  
  p = ME_InternalInsertTextFromCursor(editor, nCursor, &tab, 1, pStyle,
                                      MERF_CELL);
  run = p;
  while ((run = ME_FindItemBack(run, diRunOrParagraph))->type == diRun)
  {
    if (run->member.run.nFlags & MERF_CELL)
    {
      assert(run->member.run.pCell->next);
      p->member.run.pCell = run->member.run.pCell->next;
      return;
    }
  }
  assert(run->type == diParagraph);
  assert(run->member.para.bTable);
  assert(run->member.para.pCells);
  p->member.run.pCell = run->member.para.pCells;
}


void ME_InsertTextFromCursor(ME_TextEditor *editor, int nCursor, 
  const WCHAR *str, int len, ME_Style *style)
{
  const WCHAR *pos;
  ME_Cursor *p = NULL;
  int freeSpace;

  /* FIXME really HERE ? */
  if (ME_IsSelection(editor))
    ME_DeleteSelection(editor);

  /* FIXME: is this too slow? */
  /* Didn't affect performance for WM_SETTEXT (around 50sec/30K) */
  freeSpace = editor->nTextLimit - ME_GetTextLength(editor);

  /* text operations set modified state */
  editor->nModifyStep = 1;

  assert(style);

  assert(nCursor>=0 && nCursor<editor->nCursors);
  if (len == -1)
    len = lstrlenW(str);
  len = min(len, freeSpace);
  while (len)
  {
    pos = str;
    /* FIXME this sucks - no respect for unicode (what else can be a line separator in unicode?) */
    while(pos-str < len && *pos != '\r' && *pos != '\n' && *pos != '\t')
      pos++;
    if (pos-str < len && *pos == '\t') { /* handle tabs */
      WCHAR tab = '\t';

      if (pos!=str)
        ME_InternalInsertTextFromCursor(editor, nCursor, str, pos-str, style, 0);
    
      ME_InternalInsertTextFromCursor(editor, nCursor, &tab, 1, style, MERF_TAB);
 
      pos++;
      if(pos-str <= len) {
        len -= pos - str;
        str = pos;
        continue;
      }
    }
    if (pos-str < len) {   /* handle EOLs */
      ME_DisplayItem *tp, *end_run;
      ME_Style *tmp_style;
      if (pos!=str)
        ME_InternalInsertTextFromCursor(editor, nCursor, str, pos-str, style, 0);
      p = &editor->pCursors[nCursor];
      if (p->nOffset) {
        ME_SplitRunSimple(editor, p->pRun, p->nOffset);
        p = &editor->pCursors[nCursor];
      }
      tmp_style = ME_GetInsertStyle(editor, nCursor);
      /* ME_SplitParagraph increases style refcount */
      tp = ME_SplitParagraph(editor, p->pRun, p->pRun->member.run.style);
      p->pRun = ME_FindItemFwd(tp, diRun);
      end_run = ME_FindItemBack(tp, diRun);
      ME_ReleaseStyle(end_run->member.run.style);
      end_run->member.run.style = tmp_style;
      p->nOffset = 0;
      if(pos-str < len && *pos =='\r')
        pos++;
      if(pos-str < len && *pos =='\n')
        pos++;
      if(pos-str <= len) {
        len -= pos - str;
        str = pos;
        continue;
      }
    }
    ME_InternalInsertTextFromCursor(editor, nCursor, str, len, style, 0);
    len = 0;
  }
}


static BOOL
ME_MoveCursorChars(ME_TextEditor *editor, ME_Cursor *pCursor, int nRelOfs)
{
  ME_DisplayItem *pRun = pCursor->pRun;
  
  if (nRelOfs == -1)
  {
    if (!pCursor->nOffset)
    {
      do {
        pRun = ME_FindItemBack(pRun, diRunOrParagraph);
        assert(pRun);
        switch (pRun->type)
        {
          case diRun:
            break;
          case diParagraph:
            if (pRun->member.para.prev_para->type == diTextStart)
              return FALSE;
            pRun = ME_FindItemBack(pRun, diRunOrParagraph);
            /* every paragraph ought to have at least one run */
            assert(pRun && pRun->type == diRun);
            assert(pRun->member.run.nFlags & MERF_ENDPARA);
            break;
          default:
            assert(pRun->type != diRun && pRun->type != diParagraph);
            return FALSE;
        }
      } while (RUN_IS_HIDDEN(&pRun->member.run));
      pCursor->pRun = pRun;
      if (pRun->member.run.nFlags & MERF_ENDPARA)
        pCursor->nOffset = 0;
      else
        pCursor->nOffset = pRun->member.run.strText->nLen;
    }
    
    if (pCursor->nOffset)
      pCursor->nOffset = ME_StrRelPos2(pCursor->pRun->member.run.strText, pCursor->nOffset, nRelOfs);
    return TRUE;
  }
  else
  {
    if (!(pRun->member.run.nFlags & MERF_ENDPARA))
    {
      int new_ofs = ME_StrRelPos2(pRun->member.run.strText, pCursor->nOffset, nRelOfs);
    
      if (new_ofs < pRun->member.run.strText->nLen)
      {
        pCursor->nOffset = new_ofs;
        return TRUE;
      }
    }
    do {
      pRun = ME_FindItemFwd(pRun, diRun);
    } while (pRun && RUN_IS_HIDDEN(&pRun->member.run));
    if (pRun)
    {
      pCursor->pRun = pRun;
      pCursor->nOffset = 0;
      return TRUE;
    }
  }
  return FALSE;
}


static BOOL
ME_MoveCursorWords(ME_TextEditor *editor, ME_Cursor *cursor, int nRelOfs)
{
  ME_DisplayItem *pRun = cursor->pRun, *pOtherRun;
  int nOffset = cursor->nOffset;
  
  if (nRelOfs == -1)
  {
    /* Backward movement */
    while (TRUE)
    {
      nOffset = ME_CallWordBreakProc(editor, pRun->member.run.strText,
                                     nOffset, WB_MOVEWORDLEFT);
       if (nOffset)
        break;
      pOtherRun = ME_FindItemBack(pRun, diRunOrParagraph);
      if (pOtherRun->type == diRun)
      {
        if (ME_CallWordBreakProc(editor, pOtherRun->member.run.strText,
                                 pOtherRun->member.run.strText->nLen - 1,
                                 WB_ISDELIMITER)
            && !(pRun->member.run.nFlags & MERF_ENDPARA)
            && !(cursor->pRun == pRun && cursor->nOffset == 0)
            && !ME_CallWordBreakProc(editor, pRun->member.run.strText, 0,
                                     WB_ISDELIMITER))
          break;
        pRun = pOtherRun;
        nOffset = pOtherRun->member.run.strText->nLen;
      }
      else if (pOtherRun->type == diParagraph)
      {
        if (cursor->pRun == pRun && cursor->nOffset == 0)
        {
          /* Paragraph breaks are treated as separate words */
          if (pOtherRun->member.para.prev_para->type == diTextStart)
            return FALSE;
          pRun = ME_FindItemBack(pOtherRun, diRunOrParagraph);
        }
        break;
      }
    }
  }
  else
  {
    /* Forward movement */
    BOOL last_delim = FALSE;
    
    while (TRUE)
    {
      if (last_delim && !ME_CallWordBreakProc(editor, pRun->member.run.strText,
                                              nOffset, WB_ISDELIMITER))
        break;
      nOffset = ME_CallWordBreakProc(editor, pRun->member.run.strText,
                                     nOffset, WB_MOVEWORDRIGHT);
      if (nOffset < pRun->member.run.strText->nLen)
        break;
      pOtherRun = ME_FindItemFwd(pRun, diRunOrParagraphOrEnd);
      if (pOtherRun->type == diRun)
      {
        last_delim = ME_CallWordBreakProc(editor, pRun->member.run.strText,
                                          nOffset - 1, WB_ISDELIMITER);
        pRun = pOtherRun;
        nOffset = 0;
      }
      else if (pOtherRun->type == diParagraph)
      {
        if (cursor->pRun == pRun)
          pRun = ME_FindItemFwd(pOtherRun, diRun);
        nOffset = 0;
        break;
      }
      else /* diTextEnd */
      {
        if (cursor->pRun == pRun)
          return FALSE;
        nOffset = 0;
        break;
      }
    }
  }
  cursor->pRun = pRun;
  cursor->nOffset = nOffset;
  return TRUE;
}


void
ME_SelectWord(ME_TextEditor *editor)
{
  if (!(editor->pCursors[0].pRun->member.run.nFlags & MERF_ENDPARA))
    ME_MoveCursorWords(editor, &editor->pCursors[0], -1);
  ME_MoveCursorWords(editor, &editor->pCursors[1], +1);
  ME_InvalidateSelection(editor);
  ME_SendSelChange(editor);
}


int ME_GetCursorOfs(ME_TextEditor *editor, int nCursor)
{
  ME_Cursor *pCursor = &editor->pCursors[nCursor];
  
  return ME_GetParagraph(pCursor->pRun)->member.para.nCharOfs
    + pCursor->pRun->member.run.nCharOfs + pCursor->nOffset;
}

int ME_FindPixelPos(ME_TextEditor *editor, int x, int y, ME_Cursor *result, BOOL *is_eol)
{
  ME_DisplayItem *p = editor->pBuffer->pFirst->member.para.next_para;
  int rx = 0;
  
  if (is_eol)
    *is_eol = 0;

  while(p != editor->pBuffer->pLast)
  {
    if (p->type == diParagraph)
    {
      int ry = y - p->member.para.nYPos;
      if (ry < 0)
      {
        result->pRun = ME_FindItemFwd(p, diRun);
        result->nOffset = 0;
        return 0;
      }
      if (ry >= p->member.para.nHeight)
      {
        p = p->member.para.next_para;
        continue;
      }
      p = ME_FindItemFwd(p, diStartRow);
      y = ry;
      continue;
    }
    if (p->type == diStartRow)
    {
      int ry = y - p->member.row.nYPos;
      if (ry < 0)
        return 0;
      if (ry >= p->member.row.nHeight)
      {
        p = ME_FindItemFwd(p, diStartRowOrParagraphOrEnd);
        if (p->type != diStartRow)
          return 0;
        continue;
      }
      p = ME_FindItemFwd(p, diRun);
      continue;
    }
    if (p->type == diRun)
    {
      ME_DisplayItem *pp;
      rx = x - p->member.run.pt.x;
      if (rx < 0)
        rx = 0;
      if (rx >= p->member.run.nWidth) /* not this run yet... find next item */
      {
        pp = p;
        do {
          p = p->next;
          if (p->type == diRun)
          {
            rx = x - p->member.run.pt.x;
            goto continue_search;
          }
          if (p->type == diStartRow)
          {
            p = ME_FindItemFwd(p, diRun);
            if (is_eol)
              *is_eol = 1;
            rx = 0; /* FIXME not sure */
            goto found_here;
          }
          if (p->type == diParagraph || p->type == diTextEnd)
          {
            rx = 0; /* FIXME not sure */
            p = pp;
            goto found_here;
          }
        } while(1);
        continue;
      }
    found_here:
      if (p->member.run.nFlags & MERF_ENDPARA)
        rx = 0;
      result->pRun = p;
      result->nOffset = ME_CharFromPointCursor(editor, rx, &p->member.run);
      if (editor->pCursors[0].nOffset == p->member.run.strText->nLen && rx)
      {
        result->pRun = ME_FindItemFwd(editor->pCursors[0].pRun, diRun);
        result->nOffset = 0;
      }
      return 1;
    }
    assert(0);
  continue_search:
    ;
  }
  result->pRun = ME_FindItemBack(p, diRun);
  result->nOffset = 0;
  assert(result->pRun->member.run.nFlags & MERF_ENDPARA);
  return 0;
}


int
ME_CharFromPos(ME_TextEditor *editor, int x, int y)
{
  ME_Cursor cursor;
  RECT rc;

  GetClientRect(editor->hWnd, &rc);
  if (x < 0 || y < 0 || x >= rc.right || y >= rc.bottom)
    return -1;
  y += ME_GetYScrollPos(editor);
  ME_FindPixelPos(editor, x, y, &cursor, NULL);
  return (ME_GetParagraph(cursor.pRun)->member.para.nCharOfs
          + cursor.pRun->member.run.nCharOfs + cursor.nOffset);
}


void ME_LButtonDown(ME_TextEditor *editor, int x, int y)
{
  ME_Cursor tmp_cursor;
  int is_selection = 0;
  
  editor->nUDArrowX = -1;
  
  y += ME_GetYScrollPos(editor);

  tmp_cursor = editor->pCursors[0];
  is_selection = ME_IsSelection(editor);

  ME_FindPixelPos(editor, x, y, &editor->pCursors[0], &editor->bCaretAtEnd);
  
  if (GetKeyState(VK_SHIFT)>=0)
  {
    editor->pCursors[1] = editor->pCursors[0];
  }
  else
  {
    if (!is_selection) {
      editor->pCursors[1] = tmp_cursor;
      is_selection = 1;
    }
  }
  ME_InvalidateSelection(editor);
  HideCaret(editor->hWnd);
  ME_MoveCaret(editor);
  ShowCaret(editor->hWnd);
  ME_ClearTempStyle(editor);
  ME_SendSelChange(editor);
}

void ME_MouseMove(ME_TextEditor *editor, int x, int y)
{
  ME_Cursor tmp_cursor;
  
  y += ME_GetYScrollPos(editor);

  tmp_cursor = editor->pCursors[0];
  /* FIXME: do something with the return value of ME_FindPixelPos */
  ME_FindPixelPos(editor, x, y, &tmp_cursor, &editor->bCaretAtEnd);
  
  if (tmp_cursor.pRun == editor->pCursors[0].pRun && 
      tmp_cursor.nOffset == editor->pCursors[0].nOffset)
    return;
  
  ME_InvalidateSelection(editor);
  editor->pCursors[0] = tmp_cursor;
  HideCaret(editor->hWnd);
  ME_MoveCaret(editor);
  ME_InvalidateSelection(editor);
  ShowCaret(editor->hWnd);
  ME_SendSelChange(editor);
}

static ME_DisplayItem *ME_FindRunInRow(ME_TextEditor *editor, ME_DisplayItem *pRow, 
                                int x, int *pOffset, int *pbCaretAtEnd)
{
  ME_DisplayItem *pNext, *pLastRun;
  pNext = ME_FindItemFwd(pRow, diRunOrStartRow);
  assert(pNext->type == diRun);
  pLastRun = pNext;
  *pbCaretAtEnd = FALSE;
  do {
    int run_x = pNext->member.run.pt.x;
    int width = pNext->member.run.nWidth;
    if (x < run_x)
    {
      if (pOffset) *pOffset = 0;
      return pNext;
    }
    if (x >= run_x && x < run_x+width)
    {
      int ch = ME_CharFromPointCursor(editor, x-run_x, &pNext->member.run);
      ME_String *s = pNext->member.run.strText;
      if (ch < s->nLen) {
        if (pOffset)
          *pOffset = ch;
        return pNext;          
      }
    }
    pLastRun = pNext;
    pNext = ME_FindItemFwd(pNext, diRunOrStartRow);
  } while(pNext && pNext->type == diRun);
  
  if ((pLastRun->member.run.nFlags & MERF_ENDPARA) == 0)
  {
    pNext = ME_FindItemFwd(pNext, diRun);
    if (pbCaretAtEnd) *pbCaretAtEnd = 1;
    if (pOffset) *pOffset = 0;
    return pNext;
  } else {
    if (pbCaretAtEnd) *pbCaretAtEnd = 0;
    if (pOffset) *pOffset = 0;
    return pLastRun;
  }
}

static int ME_GetXForArrow(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRun = pCursor->pRun;
  int x;

  if (editor->nUDArrowX != -1)
    x = editor->nUDArrowX;
  else {
    if (editor->bCaretAtEnd)
    {
      pRun = ME_FindItemBack(pRun, diRun);
      assert(pRun);
      x = pRun->member.run.pt.x + pRun->member.run.nWidth;
    }
    else {
      x = pRun->member.run.pt.x;
      x += ME_PointFromChar(editor, &pRun->member.run, pCursor->nOffset);
    }
    editor->nUDArrowX = x;
  }
  return x;
}


static void
ME_MoveCursorLines(ME_TextEditor *editor, ME_Cursor *pCursor, int nRelOfs)
{
  ME_DisplayItem *pRun = pCursor->pRun;
  ME_DisplayItem *pItem;
  int x = ME_GetXForArrow(editor, pCursor);

  if (editor->bCaretAtEnd && !pCursor->nOffset)
    pRun = ME_FindItemBack(pRun, diRun);
  if (!pRun)
    return;
  if (nRelOfs == -1)
  {
    /* start of this row */
    pItem = ME_FindItemBack(pRun, diStartRow);
    assert(pItem);
    /* start of the previous row */
    pItem = ME_FindItemBack(pItem, diStartRow);
  }
  else
  {
    /* start of the next row */
    pItem = ME_FindItemFwd(pRun, diStartRow);
    /* FIXME If diParagraph is before diStartRow, wrap the next paragraph?
    */
  }
  if (!pItem)
  {
    /* row not found - ignore */
    return;
  }
  pCursor->pRun = ME_FindRunInRow(editor, pItem, x, &pCursor->nOffset, &editor->bCaretAtEnd);
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
}


static void ME_ArrowPageUp(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRun = pCursor->pRun;
  ME_DisplayItem *pLast, *p;
  int x, y, ys, yd, yp, yprev;
  ME_Cursor tmp_curs = *pCursor;
  
  x = ME_GetXForArrow(editor, pCursor);
  if (!pCursor->nOffset && editor->bCaretAtEnd)
    pRun = ME_FindItemBack(pRun, diRun);
  
  p = ME_FindItemBack(pRun, diStartRowOrParagraph);
  assert(p->type == diStartRow);
  yp = ME_FindItemBack(p, diParagraph)->member.para.nYPos;
  yprev = ys = y = yp + p->member.row.nYPos;
  yd = y - editor->sizeWindow.cy;
  pLast = p;
  
  do {
    p = ME_FindItemBack(p, diStartRowOrParagraph);
    if (!p)
      break;
    if (p->type == diParagraph) { /* crossing paragraphs */
      if (p->member.para.prev_para == NULL)
        break;
      yp = p->member.para.prev_para->member.para.nYPos;
      continue;
    }
    y = yp + p->member.row.nYPos;
    if (y < yd)
      break;
    pLast = p;
    yprev = y;
  } while(1);
  
  pCursor->pRun = ME_FindRunInRow(editor, pLast, x, &pCursor->nOffset, &editor->bCaretAtEnd);
  ME_UpdateSelection(editor, &tmp_curs);
  if (yprev < editor->sizeWindow.cy)
  {
    ME_EnsureVisible(editor, ME_FindItemFwd(editor->pBuffer->pFirst, diRun));
    ME_Repaint(editor);
  }
  else 
  {
    ME_ScrollUp(editor, ys-yprev);
  }
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
}

/* FIXME: in the original RICHEDIT, PageDown always scrolls by the same amount 
   of pixels, even if it makes the scroll bar position exceed its normal maximum.
   In such a situation, clicking the scrollbar restores its position back to the
   normal range (ie. sets it to (doclength-screenheight)). */

static void ME_ArrowPageDown(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRun = pCursor->pRun;
  ME_DisplayItem *pLast, *p;
  int x, y, ys, yd, yp, yprev;
  ME_Cursor tmp_curs = *pCursor;
  
  x = ME_GetXForArrow(editor, pCursor);
  if (!pCursor->nOffset && editor->bCaretAtEnd)
    pRun = ME_FindItemBack(pRun, diRun);
  
  p = ME_FindItemBack(pRun, diStartRowOrParagraph);
  assert(p->type == diStartRow);
  yp = ME_FindItemBack(p, diParagraph)->member.para.nYPos;
  yprev = ys = y = yp + p->member.row.nYPos;
  yd = y + editor->sizeWindow.cy;
  pLast = p;
  
  do {
    p = ME_FindItemFwd(p, diStartRowOrParagraph);
    if (!p)
      break;
    if (p->type == diParagraph) {
      yp = p->member.para.nYPos;
      continue;
    }
    y = yp + p->member.row.nYPos;
    if (y >= yd)
      break;
    pLast = p;
    yprev = y;
  } while(1);
  
  pCursor->pRun = ME_FindRunInRow(editor, pLast, x, &pCursor->nOffset, &editor->bCaretAtEnd);
  ME_UpdateSelection(editor, &tmp_curs);
  if (yprev >= editor->nTotalLength-editor->sizeWindow.cy)
  {
    ME_EnsureVisible(editor, ME_FindItemBack(editor->pBuffer->pLast, diRun));
    ME_Repaint(editor);
  }
  else 
  {
    ME_ScrollUp(editor,ys-yprev);
  }
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
}

static void ME_ArrowHome(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRow = ME_FindItemBack(pCursor->pRun, diStartRow);
  /* bCaretAtEnd doesn't make sense if the cursor isn't set at the
  first character of the next row */
  assert(!editor->bCaretAtEnd || !pCursor->nOffset);
  ME_WrapMarkedParagraphs(editor);
  if (pRow) {
    ME_DisplayItem *pRun;
    if (editor->bCaretAtEnd && !pCursor->nOffset) {
      pRow = ME_FindItemBack(pRow, diStartRow);
      if (!pRow)
        return;
    }
    pRun = ME_FindItemFwd(pRow, diRun);
    if (pRun) {
      pCursor->pRun = pRun;
      pCursor->nOffset = 0;
    }
  }
  editor->bCaretAtEnd = FALSE;
}

static void ME_ArrowCtrlHome(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRow = ME_FindItemBack(pCursor->pRun, diTextStart);
  if (pRow) {
    ME_DisplayItem *pRun = ME_FindItemFwd(pRow, diRun);
    if (pRun) {
      pCursor->pRun = pRun;
      pCursor->nOffset = 0;
    }
  }
}

static void ME_ArrowEnd(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRow;
  
  if (editor->bCaretAtEnd && !pCursor->nOffset)
    return;
  
  pRow = ME_FindItemFwd(pCursor->pRun, diStartRowOrParagraphOrEnd);
  assert(pRow);
  if (pRow->type == diStartRow) {
    /* FIXME WTF was I thinking about here ? */
    ME_DisplayItem *pRun = ME_FindItemFwd(pRow, diRun);
    assert(pRun);
    pCursor->pRun = pRun;
    pCursor->nOffset = 0;
    editor->bCaretAtEnd = 1;
    return;
  }
  pCursor->pRun = ME_FindItemBack(pRow, diRun);
  assert(pCursor->pRun && pCursor->pRun->member.run.nFlags & MERF_ENDPARA);
  pCursor->nOffset = 0;
  editor->bCaretAtEnd = FALSE;
}
      
static void ME_ArrowCtrlEnd(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *p = ME_FindItemFwd(pCursor->pRun, diTextEnd);
  assert(p);
  p = ME_FindItemBack(p, diRun);
  assert(p);
  assert(p->member.run.nFlags & MERF_ENDPARA);
  pCursor->pRun = p;
  pCursor->nOffset = 0;
  editor->bCaretAtEnd = FALSE;
}

BOOL ME_IsSelection(ME_TextEditor *editor)
{
  return memcmp(&editor->pCursors[0], &editor->pCursors[1], sizeof(ME_Cursor))!=0;
}

static int ME_GetSelCursor(ME_TextEditor *editor, int dir)
{
  int cdir = ME_GetCursorOfs(editor, 0) - ME_GetCursorOfs(editor, 1);
  
  if (cdir*dir>0)
    return 0;
  else
    return 1;
}
      
BOOL ME_UpdateSelection(ME_TextEditor *editor, ME_Cursor *pTempCursor)
{
  ME_Cursor old_anchor = editor->pCursors[1];
  
  if (GetKeyState(VK_SHIFT)>=0) /* cancelling selection */
  {
    /* any selection was present ? if so, it's no more, repaint ! */
    editor->pCursors[1] = editor->pCursors[0];
    if (memcmp(pTempCursor, &old_anchor, sizeof(ME_Cursor))) {
      return TRUE;
    }
    return FALSE;
  }
  else
  {
    if (!memcmp(pTempCursor, &editor->pCursors[1], sizeof(ME_Cursor))) /* starting selection */
    {
      editor->pCursors[1] = *pTempCursor;
      return TRUE;
    }
  }

  ME_Repaint(editor);
  return TRUE;
}

void ME_DeleteSelection(ME_TextEditor *editor)
{
  int from, to;
  ME_GetSelection(editor, &from, &to);
  ME_DeleteTextAtCursor(editor, ME_GetSelCursor(editor,-1), to-from);
}

ME_Style *ME_GetSelectionInsertStyle(ME_TextEditor *editor)
{
  ME_Style *style;
  int from, to;
  ME_Cursor c;
  
  ME_GetSelection(editor, &from, &to);
  ME_CursorFromCharOfs(editor, from, &c);
  if (from != to) {
    style = c.pRun->member.run.style;
    ME_AddRefStyle(style); /* ME_GetInsertStyle has already done that */
  }
  else
    style = ME_GetInsertStyle(editor, 0);
  return style;
}

void ME_SendSelChange(ME_TextEditor *editor)
{
  SELCHANGE sc;

  ME_ClearTempStyle(editor);
  
  if (!(editor->nEventMask & ENM_SELCHANGE))
    return;
  
  sc.nmhdr.hwndFrom = editor->hWnd;
  sc.nmhdr.idFrom = GetWindowLongW(editor->hWnd, GWLP_ID);
  sc.nmhdr.code = EN_SELCHANGE;
  SendMessageW(editor->hWnd, EM_EXGETSEL, 0, (LPARAM)&sc.chrg);
  sc.seltyp = SEL_EMPTY;
  if (sc.chrg.cpMin != sc.chrg.cpMax)
    sc.seltyp |= SEL_TEXT;
  if (sc.chrg.cpMin < sc.chrg.cpMax+1) /* wth were RICHEDIT authors thinking ? */
    sc.seltyp |= SEL_MULTICHAR;
  SendMessageW(GetParent(editor->hWnd), WM_NOTIFY, sc.nmhdr.idFrom, (LPARAM)&sc);
}


BOOL
ME_ArrowKey(ME_TextEditor *editor, int nVKey, BOOL extend, BOOL ctrl)
{
  int nCursor = 0;
  ME_Cursor *p = &editor->pCursors[nCursor];
  ME_Cursor tmp_curs = *p;
  BOOL success = FALSE;
  
  ME_CheckCharOffsets(editor);
  editor->nUDArrowX = -1;
  switch(nVKey) {
    case VK_LEFT:
      editor->bCaretAtEnd = 0;
      if (ctrl)
        success = ME_MoveCursorWords(editor, &tmp_curs, -1);
      else
        success = ME_MoveCursorChars(editor, &tmp_curs, -1);
      break;
    case VK_RIGHT:
      editor->bCaretAtEnd = 0;
      if (ctrl)
        success = ME_MoveCursorWords(editor, &tmp_curs, +1);
      else
        success = ME_MoveCursorChars(editor, &tmp_curs, +1);
      break;
    case VK_UP:
      ME_MoveCursorLines(editor, &tmp_curs, -1);
      break;
    case VK_DOWN:
      ME_MoveCursorLines(editor, &tmp_curs, +1);
      break;
    case VK_PRIOR:
      ME_ArrowPageUp(editor, &tmp_curs);
      break;
    case VK_NEXT:
      ME_ArrowPageDown(editor, &tmp_curs);
      break;
    case VK_HOME: {
      if (ctrl)
        ME_ArrowCtrlHome(editor, &tmp_curs);
      else
        ME_ArrowHome(editor, &tmp_curs);
      editor->bCaretAtEnd = 0;
      break;
    }
    case VK_END: 
      if (ctrl)
        ME_ArrowCtrlEnd(editor, &tmp_curs);
      else
        ME_ArrowEnd(editor, &tmp_curs);
      break;
  }
  
  if (!extend)
    editor->pCursors[1] = tmp_curs;
  *p = tmp_curs;
  
  ME_InvalidateSelection(editor);
  ME_Repaint(editor);
  HideCaret(editor->hWnd);
  ME_EnsureVisible(editor, tmp_curs.pRun); 
  ME_ShowCaret(editor);
  ME_SendSelChange(editor);
  return success;
}
