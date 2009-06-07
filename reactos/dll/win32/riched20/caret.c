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
  ME_DisplayItem *pLast = editor->pBuffer->pLast;
  return ME_CharOfsFromRunOfs(editor, pLast->member.para.prev_para,
                              ME_FindItemBack(pLast, diRun), 0);
}


int ME_GetTextLengthEx(ME_TextEditor *editor, const GETTEXTLENGTHEX *how)
{
  int length;

  if (how->flags & GTL_PRECISE && how->flags & GTL_CLOSE)
    return E_INVALIDARG;
  if (how->flags & GTL_NUMCHARS && how->flags & GTL_NUMBYTES)
    return E_INVALIDARG;
  
  length = ME_GetTextLength(editor);

  if ((editor->styleFlags & ES_MULTILINE)
        && (how->flags & GTL_USECRLF)
        && !editor->bEmulateVersion10) /* Ignore GTL_USECRLF flag in 1.0 emulation */
    length += editor->nParagraphs - 1;
  
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
    editor->pCursors[1].pPara = editor->pBuffer->pFirst->member.para.next_para;
    editor->pCursors[1].pRun = ME_FindItemFwd(editor->pCursors[1].pPara, diRun);
    editor->pCursors[1].nOffset = 0;
    editor->pCursors[0].pPara = editor->pBuffer->pLast->member.para.prev_para;
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
    editor->pCursors[0].pPara = editor->pBuffer->pLast->member.para.prev_para;
    editor->pCursors[0].pRun = ME_FindItemBack(editor->pBuffer->pLast, diRun);
    editor->pCursors[0].nOffset = 0;
    editor->pCursors[1] = editor->pCursors[0];
    ME_InvalidateSelection(editor);
    ME_ClearTempStyle(editor);
    return len;
  }

  ME_CursorFromCharOfs(editor, from, &editor->pCursors[1]);
  ME_CursorFromCharOfs(editor, to, &editor->pCursors[0]);
  /* Selection is not allowed in the middle of an end paragraph run. */
  if (editor->pCursors[1].pRun->member.run.nFlags & MERF_ENDPARA)
    editor->pCursors[1].nOffset = 0;
  if (editor->pCursors[0].pRun->member.run.nFlags & MERF_ENDPARA)
    editor->pCursors[0].nOffset = 0;
  return to;
}


static void
ME_GetCursorCoordinates(ME_TextEditor *editor, ME_Cursor *pCursor,
                        int *x, int *y, int *height)
{
  ME_DisplayItem *row;
  ME_DisplayItem *run = pCursor->pRun;
  ME_DisplayItem *para = pCursor->pPara;
  ME_DisplayItem *pSizeRun = run;
  ME_Context c;
  SIZE sz = {0, 0};

  assert(height && x && y);
  assert(~para->member.para.nFlags & MEPF_REWRAP);
  assert(run && run->type == diRun);
  assert(para && para->type == diParagraph);

  row = ME_FindItemBack(run, diStartRowOrParagraph);
  assert(row && row->type == diStartRow);

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));

  if (!pCursor->nOffset)
  {
    ME_DisplayItem *prev = ME_FindItemBack(run, diRunOrParagraph);
    assert(prev);
    if (prev->type == diRun)
      pSizeRun = prev;
  }
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
      sz = ME_GetRunSize(&c, &para->member.para,
                         &run->member.run, run->member.run.strText->nLen,
                         row->member.row.nLMargin);
    }
  }
  if (pCursor->nOffset) {
    sz = ME_GetRunSize(&c, &para->member.para, &run->member.run,
                       pCursor->nOffset, row->member.row.nLMargin);
  }

  *height = pSizeRun->member.run.nAscent + pSizeRun->member.run.nDescent;
  *x = c.rcView.left + run->member.run.pt.x + sz.cx - editor->horz_si.nPos;
  *y = c.rcView.top + para->member.para.pt.y + row->member.row.nBaseline
       + run->member.run.pt.y - pSizeRun->member.run.nAscent
       - editor->vert_si.nPos;
  ME_DestroyContext(&c);
  return;
}


void
ME_MoveCaret(ME_TextEditor *editor)
{
  int x, y, height;

  ME_GetCursorCoordinates(editor, &editor->pCursors[0], &x, &y, &height);
  if(editor->bHaveFocus && !ME_IsSelection(editor))
  {
    x = min(x, editor->rcFormat.right-1);
    ITextHost_TxCreateCaret(editor->texthost, NULL, 0, height);
    ITextHost_TxSetCaretPos(editor->texthost, x, y);
  }
}


void ME_ShowCaret(ME_TextEditor *ed)
{
  ME_MoveCaret(ed);
  if(ed->bHaveFocus && !ME_IsSelection(ed))
    ITextHost_TxShowCaret(ed->texthost, TRUE);
}

void ME_HideCaret(ME_TextEditor *ed)
{
  if(!ed->bHaveFocus || ME_IsSelection(ed))
  {
    ITextHost_TxShowCaret(ed->texthost, FALSE);
    DestroyCaret();
  }
}

BOOL ME_InternalDeleteText(ME_TextEditor *editor, int nOfs, int nChars,
                           BOOL bForce)
{
  ME_Cursor c;
  int shift = 0;
  int totalChars = nChars;
  ME_DisplayItem *start_para;

  /* Prevent deletion past last end of paragraph run. */
  nChars = min(nChars, ME_GetTextLength(editor) - nOfs);

  ME_CursorFromCharOfs(editor, nOfs, &c);
  start_para = c.pPara;

  if (!bForce)
  {
    ME_ProtectPartialTableDeletion(editor, nOfs, &nChars);
    if (nChars == 0)
      return FALSE;
  }

  while(nChars > 0)
  {
    ME_Run *run;
    ME_CursorFromCharOfs(editor, nOfs+nChars, &c);
    if (!c.nOffset &&
        nOfs+nChars == (c.pRun->member.run.nCharOfs
                        + c.pPara->member.para.nCharOfs))
    {
      /* We aren't deleting anything in this run, so we will go back to the
       * last run we are deleting text in. */
      c.pRun = ME_FindItemBack(c.pRun, diRun);
      c.pPara = ME_GetParagraph(c.pRun);
      c.nOffset = c.pRun->member.run.strText->nLen;
    }
    run = &c.pRun->member.run;
    if (run->nFlags & MERF_ENDPARA) {
      int eollen = c.pRun->member.run.strText->nLen;
      BOOL keepFirstParaFormat;

      if (!ME_FindItemFwd(c.pRun, diParagraph))
      {
        return TRUE;
      }
      keepFirstParaFormat = (totalChars == nChars && nChars <= eollen &&
                             run->nCharOfs);
      if (!editor->bEmulateVersion10) /* v4.1 */
      {
        ME_DisplayItem *next_para = ME_FindItemFwd(c.pRun, diParagraphOrEnd);
        ME_DisplayItem *this_para = next_para->member.para.prev_para;

        /* The end of paragraph before a table row is only deleted if there
         * is nothing else on the line before it. */
        if (this_para == start_para &&
            next_para->member.para.nFlags & MEPF_ROWSTART)
        {
          /* If the paragraph will be empty, then it should be deleted, however
           * it still might have text right now which would inherit the
           * MEPF_STARTROW property if we joined it right now.
           * Instead we will delete it after the preceding text is deleted. */
          if (nOfs > this_para->member.para.nCharOfs) {
            /* Skip this end of line. */
            nChars -= (eollen < nChars) ? eollen : nChars;
            continue;
          }
          keepFirstParaFormat = TRUE;
        }
      }
      ME_JoinParagraphs(editor, c.pPara, keepFirstParaFormat);
      /* ME_SkipAndPropagateCharOffset(p->pRun, shift); */
      ME_CheckCharOffsets(editor);
      nChars -= (eollen < nChars) ? eollen : nChars;
      continue;
    }
    else
    {
      ME_Cursor cursor;
      int nCharsToDelete = min(nChars, c.nOffset);
      int i;

      c.nOffset -= nCharsToDelete;

      ME_FindItemBack(c.pRun, diParagraph)->member.para.nFlags |= MEPF_REWRAP;

      cursor = c;
      /* nChars is the number of characters that should be deleted from the
         PRECEDING runs (these BEFORE cursor.pRun)
         nCharsToDelete is a number of chars to delete from THIS run */
      nChars -= nCharsToDelete;
      shift -= nCharsToDelete;
      TRACE("Deleting %d (remaning %d) chars at %d in '%s' (%d)\n",
        nCharsToDelete, nChars, c.nOffset,
        debugstr_w(run->strText->szData), run->strText->nLen);

      if (!c.nOffset && run->strText->nLen == nCharsToDelete)
      {
        /* undo = reinsert whole run */
        /* nOfs is a character offset (from the start of the document
           to the current (deleted) run */
        ME_UndoItem *pUndo = ME_AddUndoItem(editor, diUndoInsertRun, c.pRun);
        if (pUndo)
          pUndo->di.member.run.nCharOfs = nOfs+nChars;
      }
      else
      {
        /* undo = reinsert partial run */
        ME_UndoItem *pUndo = ME_AddUndoItem(editor, diUndoInsertRun, c.pRun);
        if (pUndo) {
          ME_DestroyString(pUndo->di.member.run.strText);
          pUndo->di.member.run.nCharOfs = nOfs+nChars;
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
            assert(pThisCur->nOffset <= run->strText->nLen);
          }
          if (pThisCur->nOffset == run->strText->nLen)
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

      if (!cursor.pRun->member.run.strText->nLen)
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
  return TRUE;
}

BOOL ME_DeleteTextAtCursor(ME_TextEditor *editor, int nCursor, int nChars)
{  
  assert(nCursor>=0 && nCursor<editor->nCursors);
  /* text operations set modified state */
  editor->nModifyStep = 1;
  return ME_InternalDeleteText(editor, ME_GetCursorOfs(editor, nCursor), nChars,
                               FALSE);
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


void ME_InsertOLEFromCursor(ME_TextEditor *editor, const REOBJECT* reo, int nCursor)
{
  ME_Style              *pStyle = ME_GetInsertStyle(editor, nCursor);
  ME_DisplayItem        *di;
  WCHAR                 space = ' ';
  
  /* FIXME no no no */
  if (ME_IsSelection(editor))
    ME_DeleteSelection(editor);

  di = ME_InternalInsertTextFromCursor(editor, nCursor, &space, 1, pStyle,
                                       MERF_GRAPHICS);
  di->member.run.ole_obj = ALLOC_OBJ(*reo);
  ME_CopyReObject(di->member.run.ole_obj, reo);
  ME_SendSelChange(editor);
}


void ME_InsertEndRowFromCursor(ME_TextEditor *editor, int nCursor)
{
  ME_Style              *pStyle = ME_GetInsertStyle(editor, nCursor);
  ME_DisplayItem        *di;
  WCHAR                 space = ' ';

  /* FIXME no no no */
  if (ME_IsSelection(editor))
    ME_DeleteSelection(editor);

  di = ME_InternalInsertTextFromCursor(editor, nCursor, &space, 1, pStyle,
                                       MERF_ENDROW);
  ME_SendSelChange(editor);
}


void ME_InsertTextFromCursor(ME_TextEditor *editor, int nCursor, 
  const WCHAR *str, int len, ME_Style *style)
{
  const WCHAR *pos;
  ME_Cursor *p = NULL;
  int oldLen;

  /* FIXME really HERE ? */
  if (ME_IsSelection(editor))
    ME_DeleteSelection(editor);

  /* FIXME: is this too slow? */
  /* Didn't affect performance for WM_SETTEXT (around 50sec/30K) */
  oldLen = ME_GetTextLength(editor);

  /* text operations set modified state */
  editor->nModifyStep = 1;

  assert(style);

  assert(nCursor>=0 && nCursor<editor->nCursors);
  if (len == -1)
    len = lstrlenW(str);

  /* grow the text limit to fit our text */
  if(editor->nTextLimit < oldLen +len)
    editor->nTextLimit = oldLen + len;

  pos = str;

  while (len)
  {
    /* FIXME this sucks - no respect for unicode (what else can be a line separator in unicode?) */
    while(pos - str < len && *pos != '\r' && *pos != '\n' && *pos != '\t')
      pos++;

    if (pos != str) { /* handle text */
      ME_InternalInsertTextFromCursor(editor, nCursor, str, pos-str, style, 0);
    } else if (*pos == '\t') { /* handle tabs */
      WCHAR tab = '\t';
      ME_InternalInsertTextFromCursor(editor, nCursor, &tab, 1, style, MERF_TAB);
      pos++;
    } else { /* handle EOLs */
      ME_DisplayItem *tp, *end_run;
      ME_Style *tmp_style;
      int eol_len = 0;

      /* Find number of CR and LF in end of paragraph run */
      if (*pos =='\r')
      {
        if (len > 1 && pos[1] == '\n')
          eol_len = 2;
        else if (len > 2 && pos[1] == '\r' && pos[2] == '\n')
          eol_len = 3;
        else
          eol_len = 1;
      } else {
        assert(*pos == '\n');
        eol_len = 1;
      }
      pos += eol_len;

      if (!editor->bEmulateVersion10 && eol_len == 3)
      {
        /* handle special \r\r\n sequence (richedit 2.x and higher only) */
        WCHAR space = ' ';
        ME_InternalInsertTextFromCursor(editor, nCursor, &space, 1, style, 0);
      } else {
        ME_String *eol_str;

        if (!editor->bEmulateVersion10) {
          WCHAR cr = '\r';
          eol_str = ME_MakeStringN(&cr, 1);
        } else {
          eol_str = ME_MakeStringN(str, eol_len);
        }

        p = &editor->pCursors[nCursor];
        if (p->nOffset) {
          ME_SplitRunSimple(editor, p->pRun, p->nOffset);
          p = &editor->pCursors[nCursor];
        }
        tmp_style = ME_GetInsertStyle(editor, nCursor);
        /* ME_SplitParagraph increases style refcount */
        tp = ME_SplitParagraph(editor, p->pRun, p->pRun->member.run.style, eol_str, 0);
        p->pRun = ME_FindItemFwd(tp, diRun);
        p->pPara = ME_GetParagraph(p->pRun);
        end_run = ME_FindItemBack(tp, diRun);
        ME_ReleaseStyle(end_run->member.run.style);
        end_run->member.run.style = tmp_style;
        p->nOffset = 0;
      }
    }
    len -= pos - str;
    str = pos;
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
      ME_DisplayItem *pPara = pCursor->pPara;
      do {
        pRun = ME_FindItemBack(pRun, diRunOrParagraph);
        assert(pRun);
        switch (pRun->type)
        {
          case diRun:
            break;
          case diParagraph:
            pPara = pRun;
            if (pPara->member.para.prev_para->type == diTextStart)
              return FALSE;
            pRun = ME_FindItemBack(pPara, diRunOrParagraph);
            pPara = pPara->member.para.prev_para;
            /* every paragraph ought to have at least one run */
            assert(pRun && pRun->type == diRun);
            assert(pRun->member.run.nFlags & MERF_ENDPARA);
            break;
          default:
            assert(pRun->type != diRun && pRun->type != diParagraph);
            return FALSE;
        }
      } while (RUN_IS_HIDDEN(&pRun->member.run) ||
               pRun->member.run.nFlags & MERF_HIDDEN);
      pCursor->pPara = pPara;
      pCursor->pRun = pRun;
      if (pRun->member.run.nFlags & MERF_ENDPARA)
        pCursor->nOffset = 0;
      else
        pCursor->nOffset = pRun->member.run.strText->nLen;
    }

    if (pCursor->nOffset)
      pCursor->nOffset = pCursor->nOffset + nRelOfs;
    return TRUE;
  }
  else
  {
    if (!(pRun->member.run.nFlags & MERF_ENDPARA))
    {
      int new_ofs = pCursor->nOffset + nRelOfs;

      if (new_ofs < pRun->member.run.strText->nLen)
      {
        pCursor->nOffset = new_ofs;
        return TRUE;
      }
    }
    do {
      pRun = ME_FindItemFwd(pRun, diRun);
    } while (pRun && (RUN_IS_HIDDEN(&pRun->member.run) ||
                      pRun->member.run.nFlags & MERF_HIDDEN));
    if (pRun)
    {
      pCursor->pPara = ME_GetParagraph(pRun);
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
          /* Skip empty start of table row paragraph */
          if (pOtherRun->member.para.prev_para->member.para.nFlags & MEPF_ROWSTART)
            pOtherRun = pOtherRun->member.para.prev_para;
          /* Paragraph breaks are treated as separate words */
          if (pOtherRun->member.para.prev_para->type == diTextStart)
            return FALSE;

          pRun = ME_FindItemBack(pOtherRun, diRun);
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
        if (pOtherRun->member.para.nFlags & MEPF_ROWSTART)
            pOtherRun = pOtherRun->member.para.next_para;
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
  cursor->pPara = ME_GetParagraph(pRun);
  cursor->pRun = pRun;
  cursor->nOffset = nOffset;
  return TRUE;
}


static void
ME_SelectByType(ME_TextEditor *editor, ME_SelectionType selectionType)
{
  /* pCursor[0] is the end of the selection
   * pCursor[1] is the start of the selection (or the position selection anchor)
   * pCursor[2] and [3] are the selection anchors that are backed up
   * so they are kept when the selection changes for drag selection.
   */

  editor->nSelectionType = selectionType;
  switch(selectionType)
  {
    case stPosition:
      break;
    case stWord:
      ME_MoveCursorWords(editor, &editor->pCursors[0], +1);
      editor->pCursors[1] = editor->pCursors[0];
      ME_MoveCursorWords(editor, &editor->pCursors[1], -1);
      break;
    case stLine:
    case stParagraph:
    {
      ME_DisplayItem *pItem;
      ME_DIType fwdSearchType, backSearchType;
      if (selectionType == stParagraph) {
          backSearchType = diParagraph;
          fwdSearchType = diParagraphOrEnd;
      } else {
          backSearchType = diStartRow;
          fwdSearchType = diStartRowOrParagraphOrEnd;
      }
      pItem = ME_FindItemFwd(editor->pCursors[0].pRun, fwdSearchType);
      assert(pItem);
      if (pItem->type == diTextEnd)
          editor->pCursors[0].pRun = ME_FindItemBack(pItem, diRun);
      else
          editor->pCursors[0].pRun = ME_FindItemFwd(pItem, diRun);
      editor->pCursors[0].pPara = ME_GetParagraph(editor->pCursors[0].pRun);
      editor->pCursors[0].nOffset = 0;

      pItem = ME_FindItemBack(pItem, backSearchType);
      editor->pCursors[1].pRun = ME_FindItemFwd(pItem, diRun);
      editor->pCursors[1].pPara = ME_GetParagraph(editor->pCursors[1].pRun);
      editor->pCursors[1].nOffset = 0;
      break;
    }
    case stDocument:
      /* Select everything with cursor anchored from the start of the text */
      editor->nSelectionType = stDocument;
      editor->pCursors[1].pPara = editor->pBuffer->pFirst->member.para.next_para;
      editor->pCursors[1].pRun = ME_FindItemFwd(editor->pCursors[1].pPara, diRun);
      editor->pCursors[1].nOffset = 0;
      editor->pCursors[0].pPara = editor->pBuffer->pLast->member.para.prev_para;
      editor->pCursors[0].pRun = ME_FindItemBack(editor->pBuffer->pLast, diRun);
      editor->pCursors[0].nOffset = 0;
      break;
    default: assert(0);
  }
  /* Store the anchor positions for extending the selection. */
  editor->pCursors[2] = editor->pCursors[0];
  editor->pCursors[3] = editor->pCursors[1];
}

int ME_GetCursorOfs(ME_TextEditor *editor, int nCursor)
{
  ME_Cursor *pCursor = &editor->pCursors[nCursor];
  return pCursor->pPara->member.para.nCharOfs
         + pCursor->pRun->member.run.nCharOfs + pCursor->nOffset;
}

/* Helper function for ME_FindPixelPos to find paragraph within tables */
static ME_DisplayItem* ME_FindPixelPosInTableRow(int x, int y,
                                                 ME_DisplayItem *para)
{
  ME_DisplayItem *cell, *next_cell;
  assert(para->member.para.nFlags & MEPF_ROWSTART);
  cell = para->member.para.next_para->member.para.pCell;
  assert(cell);

  /* find the cell we are in */
  while ((next_cell = cell->member.cell.next_cell) != NULL) {
    if (x < next_cell->member.cell.pt.x)
    {
      para = ME_FindItemFwd(cell, diParagraph);
      /* Found the cell, but there might be multiple paragraphs in
       * the cell, so need to search down the cell for the paragraph. */
      while (cell == para->member.para.pCell) {
        if (y < para->member.para.pt.y + para->member.para.nHeight)
        {
          if (para->member.para.nFlags & MEPF_ROWSTART)
            return ME_FindPixelPosInTableRow(x, y, para);
          else
            return para;
        }
        para = para->member.para.next_para;
      }
      /* Past the end of the cell, so go back to the last cell paragraph */
      return para->member.para.prev_para;
    }
    cell = next_cell;
  }
  /* Return table row delimiter */
  para = ME_FindItemFwd(cell, diParagraph);
  assert(para->member.para.nFlags & MEPF_ROWEND);
  assert(para->member.para.pFmt->dwMask & PFM_TABLEROWDELIMITER);
  assert(para->member.para.pFmt->wEffects & PFE_TABLEROWDELIMITER);
  return para;
}

static BOOL ME_ReturnFoundPos(ME_TextEditor *editor, ME_DisplayItem *found,
                               ME_Cursor *result, int rx, BOOL isExact)
{
  assert(found);
  assert(found->type == diRun);
  if ((found->member.run.nFlags & MERF_ENDPARA) || rx < 0)
    rx = 0;
  result->pRun = found;
  result->nOffset = ME_CharFromPointCursor(editor, rx, &found->member.run);
  if (editor->pCursors[0].nOffset == found->member.run.strText->nLen && rx)
  {
    result->pRun = ME_FindItemFwd(editor->pCursors[0].pRun, diRun);
    result->nOffset = 0;
  }
  result->pPara = ME_GetParagraph(result->pRun);
  return isExact;
}

/* Finds the run and offset from the pixel position.
 *
 * x & y are pixel positions in virtual coordinates into the rich edit control,
 * so client coordinates must first be adjusted by the scroll position.
 *
 * returns TRUE if the result was exactly under the cursor, otherwise returns
 * FALSE, and result is set to the closest position to the coordinates.
 */
static BOOL ME_FindPixelPos(ME_TextEditor *editor, int x, int y,
                            ME_Cursor *result, BOOL *is_eol)
{
  ME_DisplayItem *p = editor->pBuffer->pFirst->member.para.next_para;
  ME_DisplayItem *last = NULL;
  int rx = 0;
  BOOL isExact = TRUE;

  x -= editor->rcFormat.left;
  y -= editor->rcFormat.top;

  if (is_eol)
    *is_eol = 0;

  /* find paragraph */
  for (; p != editor->pBuffer->pLast; p = p->member.para.next_para)
  {
    assert(p->type == diParagraph);
    if (y < p->member.para.pt.y + p->member.para.nHeight)
    {
      if (p->member.para.nFlags & MEPF_ROWSTART)
        p = ME_FindPixelPosInTableRow(x, y, p);
      y -= p->member.para.pt.y;
      p = ME_FindItemFwd(p, diStartRow);
      break;
    } else if (p->member.para.nFlags & MEPF_ROWSTART) {
      p = ME_GetTableRowEnd(p);
    }
  }
  /* find row */
  for (; p != editor->pBuffer->pLast; )
  {
    ME_DisplayItem *pp;
    assert(p->type == diStartRow);
    if (y < p->member.row.pt.y + p->member.row.nHeight)
    {
        p = ME_FindItemFwd(p, diRun);
        break;
    }
    pp = ME_FindItemFwd(p, diStartRowOrParagraphOrEnd);
    if (pp->type != diStartRow)
    {
        p = ME_FindItemFwd(p, diRun);
        break;
    }
    p = pp;
  }
  if (p == editor->pBuffer->pLast)
  {
    /* The position is below the last paragraph, so the last row will be used
     * rather than the end of the text, so the x position will be used to
     * determine the offset closest to the pixel position. */
    isExact = FALSE;
    p = ME_FindItemBack(p, diStartRow);
    if (p != NULL){
      p = ME_FindItemFwd(p, diRun);
    }
    else
    {
      p = editor->pBuffer->pLast;
    }
  }
  for (; p != editor->pBuffer->pLast; p = p->next)
  {
    switch (p->type)
    {
    case diRun:
      rx = x - p->member.run.pt.x;
      if (rx < p->member.run.nWidth)
        return ME_ReturnFoundPos(editor, p, result, rx, isExact);
      break;
    case diStartRow:
      isExact = FALSE;
      p = ME_FindItemFwd(p, diRun);
      if (is_eol) *is_eol = 1;
      rx = 0; /* FIXME not sure */
      return ME_ReturnFoundPos(editor, p, result, rx, isExact);
    case diCell:
    case diParagraph:
    case diTextEnd:
      isExact = FALSE;
      rx = 0; /* FIXME not sure */
      p = last;
      return ME_ReturnFoundPos(editor, p, result, rx, isExact);
    default: assert(0);
    }
    last = p;
  }
  result->pRun = ME_FindItemBack(p, diRun);
  result->pPara = ME_GetParagraph(result->pRun);
  result->nOffset = 0;
  assert(result->pRun->member.run.nFlags & MERF_ENDPARA);
  return FALSE;
}


/* Returns the character offset closest to the pixel position
 *
 * x & y are pixel positions in client coordinates.
 *
 * isExact will be set to TRUE if the run is directly under the pixel
 * position, FALSE if it not, unless isExact is set to NULL.
 */
int ME_CharFromPos(ME_TextEditor *editor, int x, int y, BOOL *isExact)
{
  ME_Cursor cursor;
  RECT rc;
  BOOL bResult;

  ITextHost_TxGetClientRect(editor->texthost, &rc);
  if (x < 0 || y < 0 || x >= rc.right || y >= rc.bottom) {
    if (isExact) *isExact = FALSE;
    return -1;
  }
  x += editor->horz_si.nPos;
  y += editor->vert_si.nPos;
  bResult = ME_FindPixelPos(editor, x, y, &cursor, NULL);
  if (isExact) *isExact = bResult;
  return cursor.pPara->member.para.nCharOfs
         + cursor.pRun->member.run.nCharOfs + cursor.nOffset;
}



/* Extends the selection with a word, line, or paragraph selection type.
 *
 * The selection is anchored by editor->pCursors[2-3] such that the text
 * between the anchors will remain selected, and one end will be extended.
 *
 * editor->pCursors[0] should have the position to extend the selection to
 * before this function is called.
 *
 * Nothing will be done if editor->nSelectionType equals stPosition.
 */
static void ME_ExtendAnchorSelection(ME_TextEditor *editor)
{
  ME_Cursor tmp_cursor;
  int curOfs, anchorStartOfs, anchorEndOfs;
  if (editor->nSelectionType == stPosition || editor->nSelectionType == stDocument)
      return;
  curOfs = ME_GetCursorOfs(editor, 0);
  anchorStartOfs = ME_GetCursorOfs(editor, 3);
  anchorEndOfs = ME_GetCursorOfs(editor, 2);

  tmp_cursor = editor->pCursors[0];
  editor->pCursors[0] = editor->pCursors[2];
  editor->pCursors[1] = editor->pCursors[3];
  if (curOfs < anchorStartOfs)
  {
      /* Extend the left side of selection */
      editor->pCursors[1] = tmp_cursor;
      if (editor->nSelectionType == stWord)
          ME_MoveCursorWords(editor, &editor->pCursors[1], -1);
      else
      {
          ME_DisplayItem *pItem;
          ME_DIType searchType = ((editor->nSelectionType == stLine) ?
                                  diStartRowOrParagraph:diParagraph);
          pItem = ME_FindItemBack(editor->pCursors[1].pRun, searchType);
          editor->pCursors[1].pRun = ME_FindItemFwd(pItem, diRun);
          editor->pCursors[1].pPara = ME_GetParagraph(editor->pCursors[1].pRun);
          editor->pCursors[1].nOffset = 0;
      }
  }
  else if (curOfs >= anchorEndOfs)
  {
      /* Extend the right side of selection */
      editor->pCursors[0] = tmp_cursor;
      if (editor->nSelectionType == stWord)
          ME_MoveCursorWords(editor, &editor->pCursors[0], +1);
      else
      {
          ME_DisplayItem *pItem;
          ME_DIType searchType = ((editor->nSelectionType == stLine) ?
                                  diStartRowOrParagraphOrEnd:diParagraphOrEnd);
          pItem = ME_FindItemFwd(editor->pCursors[0].pRun, searchType);
          if (pItem->type == diTextEnd)
              editor->pCursors[0].pRun = ME_FindItemBack(pItem, diRun);
          else
              editor->pCursors[0].pRun = ME_FindItemFwd(pItem, diRun);
          editor->pCursors[0].pPara = ME_GetParagraph(editor->pCursors[0].pRun);
          editor->pCursors[0].nOffset = 0;
      }
  }
}

void ME_LButtonDown(ME_TextEditor *editor, int x, int y, int clickNum)
{
  ME_Cursor tmp_cursor;
  int is_selection = 0;
  BOOL is_shift;

  editor->nUDArrowX = -1;

  x += editor->horz_si.nPos;
  y += editor->vert_si.nPos;

  tmp_cursor = editor->pCursors[0];
  is_selection = ME_IsSelection(editor);
  is_shift = GetKeyState(VK_SHIFT) < 0;

  ME_FindPixelPos(editor, x, y, &editor->pCursors[0], &editor->bCaretAtEnd);

  if (x >= editor->rcFormat.left || is_shift)
  {
    if (clickNum > 1)
    {
      editor->pCursors[1] = editor->pCursors[0];
      if (is_shift) {
          if (x >= editor->rcFormat.left)
              ME_SelectByType(editor, stWord);
          else
              ME_SelectByType(editor, stParagraph);
      } else if (clickNum % 2 == 0) {
          ME_SelectByType(editor, stWord);
      } else {
          ME_SelectByType(editor, stParagraph);
      }
    }
    else if (!is_shift)
    {
      editor->nSelectionType = stPosition;
      editor->pCursors[1] = editor->pCursors[0];
    }
    else if (!is_selection)
    {
      editor->nSelectionType = stPosition;
      editor->pCursors[1] = tmp_cursor;
    }
    else if (editor->nSelectionType != stPosition)
    {
      ME_ExtendAnchorSelection(editor);
    }
  }
  else
  {
    if (clickNum < 2) {
        ME_SelectByType(editor, stLine);
    } else if (clickNum % 2 == 0 || is_shift) {
        ME_SelectByType(editor, stParagraph);
    } else {
        ME_SelectByType(editor, stDocument);
    }
  }
  ME_InvalidateSelection(editor);
  ITextHost_TxShowCaret(editor->texthost, FALSE);
  ME_ShowCaret(editor);
  ME_ClearTempStyle(editor);
  ME_SendSelChange(editor);
}

void ME_MouseMove(ME_TextEditor *editor, int x, int y)
{
  ME_Cursor tmp_cursor;

  if (editor->nSelectionType == stDocument)
      return;
  x += editor->horz_si.nPos;
  y += editor->vert_si.nPos;

  tmp_cursor = editor->pCursors[0];
  /* FIXME: do something with the return value of ME_FindPixelPos */
  ME_FindPixelPos(editor, x, y, &tmp_cursor, &editor->bCaretAtEnd);

  ME_InvalidateSelection(editor);
  editor->pCursors[0] = tmp_cursor;
  ME_ExtendAnchorSelection(editor);

  if (editor->nSelectionType != stPosition &&
      memcmp(&editor->pCursors[1], &editor->pCursors[3], sizeof(ME_Cursor)))
  {
      /* The scroll the cursor towards the other end, since it was the one
       * extended by ME_ExtendAnchorSelection */
      ME_EnsureVisible(editor, &editor->pCursors[1]);
  } else {
      ME_EnsureVisible(editor, &editor->pCursors[0]);
  }

  ME_InvalidateSelection(editor);
  ITextHost_TxShowCaret(editor->texthost, FALSE);
  ME_ShowCaret(editor);
  ME_SendSelChange(editor);
}

static ME_DisplayItem *ME_FindRunInRow(ME_TextEditor *editor, ME_DisplayItem *pRow, 
                                int x, int *pOffset, int *pbCaretAtEnd)
{
  ME_DisplayItem *pNext, *pLastRun;
  pNext = ME_FindItemFwd(pRow, diRunOrStartRow);
  assert(pNext->type == diRun);
  pLastRun = pNext;
  if (pbCaretAtEnd) *pbCaretAtEnd = FALSE;
  if (pOffset) *pOffset = 0;
  do {
    int run_x = pNext->member.run.pt.x;
    int width = pNext->member.run.nWidth;
    if (x < run_x)
    {
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
    if (pbCaretAtEnd) *pbCaretAtEnd = TRUE;
    return pNext;
  } else {
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
  ME_DisplayItem *pItem, *pOldPara, *pNewPara;
  int x = ME_GetXForArrow(editor, pCursor);

  if (editor->bCaretAtEnd && !pCursor->nOffset)
    pRun = ME_FindItemBack(pRun, diRun);
  if (!pRun)
    return;
  pOldPara = ME_GetParagraph(pRun);
  if (nRelOfs == -1)
  {
    /* start of this row */
    pItem = ME_FindItemBack(pRun, diStartRow);
    assert(pItem);
    /* start of the previous row */
    pItem = ME_FindItemBack(pItem, diStartRow);
    if (!pItem)
      return; /* row not found - ignore */
    pNewPara = ME_GetParagraph(pItem);
    if (pOldPara->member.para.nFlags & MEPF_ROWEND ||
        (pOldPara->member.para.pCell &&
         pOldPara->member.para.pCell != pNewPara->member.para.pCell))
    {
      /* Brought out of a cell */
      pNewPara = ME_GetTableRowStart(pOldPara)->member.para.prev_para;
      if (pNewPara->type == diTextStart)
        return; /* At the top, so don't go anywhere. */
      pItem = ME_FindItemFwd(pNewPara, diStartRow);
    }
    if (pNewPara->member.para.nFlags & MEPF_ROWEND)
    {
      /* Brought into a table row */
      ME_Cell *cell = &ME_FindItemBack(pNewPara, diCell)->member.cell;
      while (x < cell->pt.x && cell->prev_cell)
        cell = &cell->prev_cell->member.cell;
      if (cell->next_cell) /* else - we are still at the end of the row */
        pItem = ME_FindItemBack(cell->next_cell, diStartRow);
    }
  }
  else
  {
    /* start of the next row */
    pItem = ME_FindItemFwd(pRun, diStartRow);
    if (!pItem)
      return; /* row not found - ignore */
    pNewPara = ME_GetParagraph(pItem);
    if (pOldPara->member.para.nFlags & MEPF_ROWSTART ||
        (pOldPara->member.para.pCell &&
         pOldPara->member.para.pCell != pNewPara->member.para.pCell))
    {
      /* Brought out of a cell */
      pNewPara = ME_GetTableRowEnd(pOldPara)->member.para.next_para;
      if (pNewPara->type == diTextEnd)
        return; /* At the bottom, so don't go anywhere. */
      pItem = ME_FindItemFwd(pNewPara, diStartRow);
    }
    if (pNewPara->member.para.nFlags & MEPF_ROWSTART)
    {
      /* Brought into a table row */
      ME_DisplayItem *cell = ME_FindItemFwd(pNewPara, diCell);
      while (cell->member.cell.next_cell &&
             x >= cell->member.cell.next_cell->member.cell.pt.x)
        cell = cell->member.cell.next_cell;
      pItem = ME_FindItemFwd(cell, diStartRow);
    }
  }
  if (!pItem)
  {
    /* row not found - ignore */
    return;
  }
  pCursor->pRun = ME_FindRunInRow(editor, pItem, x, &pCursor->nOffset, &editor->bCaretAtEnd);
  pCursor->pPara = ME_GetParagraph(pCursor->pRun);
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
}

static void ME_ArrowPageUp(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *p = ME_FindItemFwd(editor->pBuffer->pFirst, diStartRow);

  if (editor->vert_si.nPos < p->member.row.nHeight)
  {
    pCursor->pPara = editor->pBuffer->pFirst->member.para.next_para;
    pCursor->pRun = ME_FindItemFwd(pCursor->pPara, diRun);
    pCursor->nOffset = 0;
    editor->bCaretAtEnd = FALSE;
    /* Native clears seems to clear this x value on page up at the top
     * of the text, but not on page down at the end of the text.
     * Doesn't make sense, but we try to be bug for bug compatible. */
    editor->nUDArrowX = -1;
  } else {
    ME_DisplayItem *pRun = pCursor->pRun;
    ME_DisplayItem *pLast;
    int x, y, ys, yd, yp, yprev;
    int yOldScrollPos = editor->vert_si.nPos;

    x = ME_GetXForArrow(editor, pCursor);
    if (!pCursor->nOffset && editor->bCaretAtEnd)
      pRun = ME_FindItemBack(pRun, diRun);

    p = ME_FindItemBack(pRun, diStartRowOrParagraph);
    assert(p->type == diStartRow);
    yp = ME_FindItemBack(p, diParagraph)->member.para.pt.y;
    yprev = ys = y = yp + p->member.row.pt.y;

    ME_ScrollUp(editor, editor->sizeWindow.cy);
    /* Only move the cursor by the amount scrolled. */
    yd = y + editor->vert_si.nPos - yOldScrollPos;
    pLast = p;

    do {
      p = ME_FindItemBack(p, diStartRowOrParagraph);
      if (!p)
        break;
      if (p->type == diParagraph) { /* crossing paragraphs */
        if (p->member.para.prev_para == NULL)
          break;
        yp = p->member.para.prev_para->member.para.pt.y;
        continue;
      }
      y = yp + p->member.row.pt.y;
      if (y < yd)
        break;
      pLast = p;
      yprev = y;
    } while(1);

    pCursor->pRun = ME_FindRunInRow(editor, pLast, x, &pCursor->nOffset,
                                    &editor->bCaretAtEnd);
    pCursor->pPara = ME_GetParagraph(pCursor->pRun);
  }
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
}

static void ME_ArrowPageDown(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pLast;
  int x, y;

  /* Find y position of the last row */
  pLast = editor->pBuffer->pLast;
  y = pLast->member.para.prev_para->member.para.pt.y
      + ME_FindItemBack(pLast, diStartRow)->member.row.pt.y;

  x = ME_GetXForArrow(editor, pCursor);

  if (editor->vert_si.nPos >= y - editor->sizeWindow.cy)
  {
    pCursor->pPara = editor->pBuffer->pLast->member.para.prev_para;
    pCursor->pRun = ME_FindItemBack(editor->pBuffer->pLast, diRun);
    pCursor->nOffset = 0;
    editor->bCaretAtEnd = FALSE;
  } else {
    ME_DisplayItem *pRun = pCursor->pRun;
    ME_DisplayItem *p;
    int ys, yd, yp, yprev;
    int yOldScrollPos = editor->vert_si.nPos;

    if (!pCursor->nOffset && editor->bCaretAtEnd)
      pRun = ME_FindItemBack(pRun, diRun);

    p = ME_FindItemBack(pRun, diStartRowOrParagraph);
    assert(p->type == diStartRow);
    yp = ME_FindItemBack(p, diParagraph)->member.para.pt.y;
    yprev = ys = y = yp + p->member.row.pt.y;

    /* For native richedit controls:
     * v1.0 - v3.1 can only scroll down as far as the scrollbar lets us
     * v4.1 can scroll past this position here. */
    ME_ScrollDown(editor, editor->sizeWindow.cy);
    /* Only move the cursor by the amount scrolled. */
    yd = y + editor->vert_si.nPos - yOldScrollPos;
    pLast = p;

    do {
      p = ME_FindItemFwd(p, diStartRowOrParagraph);
      if (!p)
        break;
      if (p->type == diParagraph) {
        yp = p->member.para.pt.y;
        continue;
      }
      y = yp + p->member.row.pt.y;
      if (y >= yd)
        break;
      pLast = p;
      yprev = y;
    } while(1);

    pCursor->pRun = ME_FindRunInRow(editor, pLast, x, &pCursor->nOffset,
                                    &editor->bCaretAtEnd);
    pCursor->pPara = ME_GetParagraph(pCursor->pRun);
  }
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
}

static void ME_ArrowHome(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRow = ME_FindItemBack(pCursor->pRun, diStartRow);
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
      assert(pCursor->pPara == ME_GetParagraph(pRun));
      pCursor->nOffset = 0;
    }
  }
  editor->bCaretAtEnd = FALSE;
}

static void ME_ArrowCtrlHome(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  pCursor->pPara = editor->pBuffer->pFirst->member.para.next_para;
  pCursor->pRun = ME_FindItemFwd(pCursor->pPara, diRun);
  pCursor->nOffset = 0;
  editor->bCaretAtEnd = FALSE;
}

static void ME_ArrowEnd(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *pRow;

  if (editor->bCaretAtEnd && !pCursor->nOffset)
    return;

  pRow = ME_FindItemFwd(pCursor->pRun, diStartRowOrParagraphOrEnd);
  assert(pRow);
  if (pRow->type == diStartRow) {
    ME_DisplayItem *pRun = ME_FindItemFwd(pRow, diRun);
    assert(pRun);
    pCursor->pRun = pRun;
    assert(pCursor->pPara == ME_GetParagraph(pCursor->pRun));
    pCursor->nOffset = 0;
    editor->bCaretAtEnd = TRUE;
    return;
  }
  pCursor->pRun = ME_FindItemBack(pRow, diRun);
  assert(pCursor->pRun && pCursor->pRun->member.run.nFlags & MERF_ENDPARA);
  assert(pCursor->pPara == ME_GetParagraph(pCursor->pRun));
  pCursor->nOffset = 0;
  editor->bCaretAtEnd = FALSE;
}

static void ME_ArrowCtrlEnd(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  pCursor->pPara = editor->pBuffer->pLast->member.para.prev_para;
  pCursor->pRun = ME_FindItemBack(editor->pBuffer->pLast, diRun);
  assert(pCursor->pRun->member.run.nFlags & MERF_ENDPARA);
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

void ME_DeleteSelection(ME_TextEditor *editor)
{
  int from, to;
  ME_GetSelection(editor, &from, &to);
  ME_DeleteTextAtCursor(editor, ME_GetSelCursor(editor,-1), to-from);
}

ME_Style *ME_GetSelectionInsertStyle(ME_TextEditor *editor)
{
  return ME_GetInsertStyle(editor, 0);
}

void ME_SendSelChange(ME_TextEditor *editor)
{
  SELCHANGE sc;

  if (!(editor->nEventMask & ENM_SELCHANGE))
    return;

  sc.nmhdr.code = EN_SELCHANGE;
  ME_GetSelection(editor, &sc.chrg.cpMin, &sc.chrg.cpMax);
  sc.seltyp = SEL_EMPTY;
  if (sc.chrg.cpMin != sc.chrg.cpMax)
    sc.seltyp |= SEL_TEXT;
  if (sc.chrg.cpMin < sc.chrg.cpMax+1) /* wth were RICHEDIT authors thinking ? */
    sc.seltyp |= SEL_MULTICHAR;
  TRACE("cpMin=%d cpMax=%d seltyp=%d (%s %s)\n",
    sc.chrg.cpMin, sc.chrg.cpMax, sc.seltyp,
    (sc.seltyp & SEL_TEXT) ? "SEL_TEXT" : "",
    (sc.seltyp & SEL_MULTICHAR) ? "SEL_MULTICHAR" : "");
  if (sc.chrg.cpMin != editor->notified_cr.cpMin || sc.chrg.cpMax != editor->notified_cr.cpMax)
  {
    ME_ClearTempStyle(editor);

    editor->notified_cr = sc.chrg;
    ITextHost_TxNotify(editor->texthost, sc.nmhdr.code, &sc);
  }
}

BOOL
ME_ArrowKey(ME_TextEditor *editor, int nVKey, BOOL extend, BOOL ctrl)
{
  int nCursor = 0;
  ME_Cursor *p = &editor->pCursors[nCursor];
  ME_Cursor tmp_curs = *p;
  BOOL success = FALSE;

  ME_CheckCharOffsets(editor);
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
  ITextHost_TxShowCaret(editor->texthost, FALSE);
  ME_EnsureVisible(editor, &tmp_curs);
  ME_ShowCaret(editor);
  ME_SendSelChange(editor);
  return success;
}
