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

void ME_SetCursorToStart(ME_TextEditor *editor, ME_Cursor *cursor)
{
  cursor->pPara = editor->pBuffer->pFirst->member.para.next_para;
  cursor->pRun = ME_FindItemFwd(cursor->pPara, diRun);
  cursor->nOffset = 0;
}

static void ME_SetCursorToEnd(ME_TextEditor *editor, ME_Cursor *cursor)
{
  cursor->pPara = editor->pBuffer->pLast->member.para.prev_para;
  cursor->pRun = ME_FindItemBack(editor->pBuffer->pLast, diRun);
  cursor->nOffset = 0;
}


int ME_GetSelectionOfs(ME_TextEditor *editor, int *from, int *to)
{
  *from = ME_GetCursorOfs(&editor->pCursors[0]);
  *to =   ME_GetCursorOfs(&editor->pCursors[1]);

  if (*from > *to)
  {
    int tmp = *from;
    *from = *to;
    *to = tmp;
    return 1;
  }
  return 0;
}

int ME_GetSelection(ME_TextEditor *editor, ME_Cursor **from, ME_Cursor **to)
{
  int from_ofs = ME_GetCursorOfs( &editor->pCursors[0] );
  int to_ofs   = ME_GetCursorOfs( &editor->pCursors[1] );
  BOOL swap = (from_ofs > to_ofs);

  if (from_ofs == to_ofs)
  {
      /* If cursor[0] is at the beginning of a run and cursor[1] at the end
         of the prev run then we need to swap. */
      if (editor->pCursors[0].nOffset < editor->pCursors[1].nOffset)
          swap = TRUE;
  }

  if (!swap)
  {
    *from = &editor->pCursors[0];
    *to = &editor->pCursors[1];
    return 0;
  } else {
    *from = &editor->pCursors[1];
    *to = &editor->pCursors[0];
    return 1;
  }
}

int ME_GetTextLength(ME_TextEditor *editor)
{
  ME_Cursor cursor;
  ME_SetCursorToEnd(editor, &cursor);
  return ME_GetCursorOfs(&cursor);
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

  if (how->flags & GTL_NUMBYTES ||
      (how->flags & GTL_PRECISE &&     /* GTL_PRECISE seems to imply GTL_NUMBYTES */
       !(how->flags & GTL_NUMCHARS)))  /* unless GTL_NUMCHARS is given */
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
    ME_SetCursorToStart(editor, &editor->pCursors[1]);
    ME_SetCursorToEnd(editor, &editor->pCursors[0]);
    editor->pCursors[0].nOffset = editor->pCursors[0].pRun->member.run.len;
    ME_InvalidateSelection(editor);
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
      ME_GetSelectionOfs(editor, &start, &end);
      if (start != end)
      {
          if (end > len)
          {
              editor->pCursors[0].nOffset = 0;
              end --;
          }
          editor->pCursors[1] = editor->pCursors[0];
          ME_Repaint(editor);
      }
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
    ME_SetCursorToEnd(editor, &editor->pCursors[0]);
    editor->pCursors[1] = editor->pCursors[0];
    ME_InvalidateSelection(editor);
    return len;
  }

  ME_CursorFromCharOfs(editor, from, &editor->pCursors[1]);
  editor->pCursors[0] = editor->pCursors[1];
  ME_MoveCursorChars(editor, &editor->pCursors[0], to - from);
  /* Selection is not allowed in the middle of an end paragraph run. */
  if (editor->pCursors[1].pRun->member.run.nFlags & MERF_ENDPARA)
    editor->pCursors[1].nOffset = 0;
  if (editor->pCursors[0].pRun->member.run.nFlags & MERF_ENDPARA)
  {
    if (to > len)
      editor->pCursors[0].nOffset = editor->pCursors[0].pRun->member.run.len;
    else
      editor->pCursors[0].nOffset = 0;
  }
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
  int run_x;

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
    }
  }
  run_x = ME_PointFromCharContext( &c, &run->member.run, pCursor->nOffset, TRUE );

  *height = pSizeRun->member.run.nAscent + pSizeRun->member.run.nDescent;
  *x = c.rcView.left + run->member.run.pt.x + run_x - editor->horz_si.nPos;
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

BOOL ME_InternalDeleteText(ME_TextEditor *editor, ME_Cursor *start,
                           int nChars, BOOL bForce)
{
  ME_Cursor c = *start;
  int nOfs = ME_GetCursorOfs(start), text_len = ME_GetTextLength( editor );
  int shift = 0;
  int totalChars = nChars;
  ME_DisplayItem *start_para;
  BOOL delete_all = FALSE;

  /* Prevent deletion past last end of paragraph run. */
  nChars = min(nChars, text_len - nOfs);
  if (nChars == text_len) delete_all = TRUE;
  start_para = c.pPara;

  if (!bForce)
  {
    ME_ProtectPartialTableDeletion(editor, &c, &nChars);
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
      ME_PrevRun(&c.pPara, &c.pRun);
      c.nOffset = c.pRun->member.run.len;
    }
    run = &c.pRun->member.run;
    if (run->nFlags & MERF_ENDPARA) {
      int eollen = c.pRun->member.run.len;
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
      TRACE("Deleting %d (remaning %d) chars at %d in %s (%d)\n",
        nCharsToDelete, nChars, c.nOffset,
        debugstr_run( run ), run->len);

      /* nOfs is a character offset (from the start of the document
         to the current (deleted) run */
      add_undo_insert_run( editor, nOfs + nChars, get_text( run, c.nOffset ), nCharsToDelete, run->nFlags, run->style );

      ME_StrDeleteV(run->para->text, run->nCharOfs + c.nOffset, nCharsToDelete);
      run->len -= nCharsToDelete;
      TRACE("Post deletion string: %s (%d)\n", debugstr_run( run ), run->len);
      TRACE("Shift value: %d\n", shift);

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
            assert(pThisCur->nOffset <= run->len);
          }
          if (pThisCur->nOffset == run->len)
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

      if (!cursor.pRun->member.run.len)
      {
        TRACE("Removing empty run\n");
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
  if (delete_all) ME_SetDefaultParaFormat( editor, start_para->member.para.pFmt );
  return TRUE;
}

BOOL ME_DeleteTextAtCursor(ME_TextEditor *editor, int nCursor, int nChars)
{
  assert(nCursor>=0 && nCursor<editor->nCursors);
  /* text operations set modified state */
  editor->nModifyStep = 1;
  return ME_InternalDeleteText(editor, &editor->pCursors[nCursor],
                               nChars, FALSE);
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
  ME_ReleaseStyle(pStyle);
}


void ME_InsertEndRowFromCursor(ME_TextEditor *editor, int nCursor)
{
  ME_Style              *pStyle = ME_GetInsertStyle(editor, nCursor);
  WCHAR                 space = ' ';

  /* FIXME no no no */
  if (ME_IsSelection(editor))
    ME_DeleteSelection(editor);

  ME_InternalInsertTextFromCursor(editor, nCursor, &space, 1, pStyle,
                                  MERF_ENDROW);
  ME_ReleaseStyle(pStyle);
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
      ME_DisplayItem *tp, *end_run, *run, *prev;
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
        const WCHAR cr = '\r', *eol_str = str;

        if (!editor->bEmulateVersion10)
        {
          eol_str = &cr;
          eol_len = 1;
        }

        p = &editor->pCursors[nCursor];

        if (p->nOffset == p->pRun->member.run.len)
        {
           run = ME_FindItemFwd( p->pRun, diRun );
           if (!run) run = p->pRun;
        }
        else
        {
          if (p->nOffset) ME_SplitRunSimple(editor, p);
          run = p->pRun;
        }

        tmp_style = ME_GetInsertStyle(editor, nCursor);
        /* ME_SplitParagraph increases style refcount */
        tp = ME_SplitParagraph(editor, run, run->member.run.style, eol_str, eol_len, 0);

        end_run = ME_FindItemBack(tp, diRun);
        ME_ReleaseStyle(end_run->member.run.style);
        end_run->member.run.style = tmp_style;

        /* Move any cursors that were at the end of the previous run to the beginning of the new para */
        prev = ME_FindItemBack( end_run, diRun );
        if (prev)
        {
          int i;
          for (i = 0; i < editor->nCursors; i++)
          {
            if (editor->pCursors[i].pRun == prev &&
                editor->pCursors[i].nOffset == prev->member.run.len)
            {
              editor->pCursors[i].pPara = tp;
              editor->pCursors[i].pRun = run;
              editor->pCursors[i].nOffset = 0;
            }
          }
        }

      }
    }
    len -= pos - str;
    str = pos;
  }
}

/* Move the cursor nRelOfs characters (either forwards or backwards)
 *
 * returns the actual number of characters moved.
 **/
int ME_MoveCursorChars(ME_TextEditor *editor, ME_Cursor *cursor, int nRelOfs)
{
  cursor->nOffset += nRelOfs;
  if (cursor->nOffset < 0)
  {
    cursor->nOffset += cursor->pRun->member.run.nCharOfs;
    if (cursor->nOffset >= 0)
    {
      /* new offset in the same paragraph */
      do {
        cursor->pRun = ME_FindItemBack(cursor->pRun, diRun);
      } while (cursor->nOffset < cursor->pRun->member.run.nCharOfs);
      cursor->nOffset -= cursor->pRun->member.run.nCharOfs;
      return nRelOfs;
    }

    cursor->nOffset += cursor->pPara->member.para.nCharOfs;
    if (cursor->nOffset <= 0)
    {
      /* moved to the start of the text */
      nRelOfs -= cursor->nOffset;
      ME_SetCursorToStart(editor, cursor);
      return nRelOfs;
    }

    /* new offset in a previous paragraph */
    do {
      cursor->pPara = cursor->pPara->member.para.prev_para;
    } while (cursor->nOffset < cursor->pPara->member.para.nCharOfs);
    cursor->nOffset -= cursor->pPara->member.para.nCharOfs;

    cursor->pRun = ME_FindItemBack(cursor->pPara->member.para.next_para, diRun);
    while (cursor->nOffset < cursor->pRun->member.run.nCharOfs) {
      cursor->pRun = ME_FindItemBack(cursor->pRun, diRun);
    }
    cursor->nOffset -= cursor->pRun->member.run.nCharOfs;
  } else if (cursor->nOffset >= cursor->pRun->member.run.len) {
    ME_DisplayItem *next_para;
    int new_offset;

    new_offset = ME_GetCursorOfs(cursor);
    next_para = cursor->pPara->member.para.next_para;
    if (new_offset < next_para->member.para.nCharOfs)
    {
      /* new offset in the same paragraph */
      do {
        cursor->nOffset -= cursor->pRun->member.run.len;
        cursor->pRun = ME_FindItemFwd(cursor->pRun, diRun);
      } while (cursor->nOffset >= cursor->pRun->member.run.len);
      return nRelOfs;
    }

    if (new_offset >= ME_GetTextLength(editor))
    {
      /* new offset at the end of the text */
      ME_SetCursorToEnd(editor, cursor);
      nRelOfs -= new_offset - ME_GetTextLength(editor);
      return nRelOfs;
    }

    /* new offset in a following paragraph */
    do {
      cursor->pPara = next_para;
      next_para = next_para->member.para.next_para;
    } while (new_offset >= next_para->member.para.nCharOfs);

    cursor->nOffset = new_offset - cursor->pPara->member.para.nCharOfs;
    cursor->pRun = ME_FindItemFwd(cursor->pPara, diRun);
    while (cursor->nOffset >= cursor->pRun->member.run.len)
    {
      cursor->nOffset -= cursor->pRun->member.run.len;
      cursor->pRun = ME_FindItemFwd(cursor->pRun, diRun);
    }
  } /* else new offset is in the same run */
  return nRelOfs;
}


BOOL
ME_MoveCursorWords(ME_TextEditor *editor, ME_Cursor *cursor, int nRelOfs)
{
  ME_DisplayItem *pRun = cursor->pRun, *pOtherRun;
  ME_DisplayItem *pPara = cursor->pPara;
  int nOffset = cursor->nOffset;

  if (nRelOfs == -1)
  {
    /* Backward movement */
    while (TRUE)
    {
      nOffset = ME_CallWordBreakProc(editor, get_text( &pRun->member.run, 0 ),
                                     pRun->member.run.len, nOffset, WB_MOVEWORDLEFT);
      if (nOffset)
        break;
      pOtherRun = ME_FindItemBack(pRun, diRunOrParagraph);
      if (pOtherRun->type == diRun)
      {
        if (ME_CallWordBreakProc(editor, get_text( &pOtherRun->member.run, 0 ),
                                 pOtherRun->member.run.len,
                                 pOtherRun->member.run.len - 1,
                                 WB_ISDELIMITER)
            && !(pRun->member.run.nFlags & MERF_ENDPARA)
            && !(cursor->pRun == pRun && cursor->nOffset == 0)
            && !ME_CallWordBreakProc(editor, get_text( &pRun->member.run, 0 ),
                                     pRun->member.run.len, 0,
                                     WB_ISDELIMITER))
          break;
        pRun = pOtherRun;
        nOffset = pOtherRun->member.run.len;
      }
      else if (pOtherRun->type == diParagraph)
      {
        if (cursor->pRun == pRun && cursor->nOffset == 0)
        {
          pPara = pOtherRun;
          /* Skip empty start of table row paragraph */
          if (pPara->member.para.prev_para->member.para.nFlags & MEPF_ROWSTART)
            pPara = pPara->member.para.prev_para;
          /* Paragraph breaks are treated as separate words */
          if (pPara->member.para.prev_para->type == diTextStart)
            return FALSE;

          pRun = ME_FindItemBack(pPara, diRun);
          pPara = pPara->member.para.prev_para;
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
      if (last_delim && !ME_CallWordBreakProc(editor, get_text( &pRun->member.run, 0 ),
                                              pRun->member.run.len, nOffset, WB_ISDELIMITER))
        break;
      nOffset = ME_CallWordBreakProc(editor, get_text( &pRun->member.run, 0 ),
                                     pRun->member.run.len, nOffset, WB_MOVEWORDRIGHT);
      if (nOffset < pRun->member.run.len)
        break;
      pOtherRun = ME_FindItemFwd(pRun, diRunOrParagraphOrEnd);
      if (pOtherRun->type == diRun)
      {
        last_delim = ME_CallWordBreakProc(editor, get_text( &pRun->member.run, 0 ),
                                          pRun->member.run.len, nOffset - 1, WB_ISDELIMITER);
        pRun = pOtherRun;
        nOffset = 0;
      }
      else if (pOtherRun->type == diParagraph)
      {
        if (pOtherRun->member.para.nFlags & MEPF_ROWSTART)
            pOtherRun = pOtherRun->member.para.next_para;
        if (cursor->pRun == pRun) {
          pPara = pOtherRun;
          pRun = ME_FindItemFwd(pPara, diRun);
        }
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
  cursor->pPara = pPara;
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
      ME_SetCursorToStart(editor, &editor->pCursors[1]);
      ME_SetCursorToEnd(editor, &editor->pCursors[0]);
      break;
    default: assert(0);
  }
  /* Store the anchor positions for extending the selection. */
  editor->pCursors[2] = editor->pCursors[0];
  editor->pCursors[3] = editor->pCursors[1];
}

int ME_GetCursorOfs(const ME_Cursor *cursor)
{
  return cursor->pPara->member.para.nCharOfs
         + cursor->pRun->member.run.nCharOfs + cursor->nOffset;
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

static BOOL ME_FindRunInRow(ME_TextEditor *editor, ME_DisplayItem *pRow,
                            int x, ME_Cursor *cursor, BOOL *pbCaretAtEnd)
{
  ME_DisplayItem *pNext, *pLastRun;
  ME_Row *row = &pRow->member.row;
  BOOL exact = TRUE;

  if (x < row->pt.x)
  {
      x = row->pt.x;
      exact = FALSE;
  }
  pNext = ME_FindItemFwd(pRow, diRunOrStartRow);
  assert(pNext->type == diRun);
  if (pbCaretAtEnd) *pbCaretAtEnd = FALSE;
  cursor->nOffset = 0;
  do {
    int run_x = pNext->member.run.pt.x;
    int width = pNext->member.run.nWidth;

    if (x >= run_x && x < run_x+width)
    {
      cursor->nOffset = ME_CharFromPoint(editor, x-run_x, &pNext->member.run, TRUE, TRUE);
      cursor->pRun = pNext;
      cursor->pPara = ME_GetParagraph( cursor->pRun );
      return exact;
    }
    pLastRun = pNext;
    pNext = ME_FindItemFwd(pNext, diRunOrStartRow);
  } while(pNext && pNext->type == diRun);

  if ((pLastRun->member.run.nFlags & MERF_ENDPARA) == 0)
  {
    cursor->pRun = ME_FindItemFwd(pNext, diRun);
    if (pbCaretAtEnd) *pbCaretAtEnd = TRUE;
  }
  else
    cursor->pRun = pLastRun;

  cursor->pPara = ME_GetParagraph( cursor->pRun );
  return FALSE;
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
  BOOL isExact = TRUE;

  x -= editor->rcFormat.left;
  y -= editor->rcFormat.top;

  if (is_eol)
    *is_eol = FALSE;

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
    if (y < p->member.row.pt.y + p->member.row.nHeight) break;
    pp = ME_FindItemFwd(p, diStartRow);
    if (!pp) break;
    p = pp;
  }
  if (p == editor->pBuffer->pLast)
  {
    /* The position is below the last paragraph, so the last row will be used
     * rather than the end of the text, so the x position will be used to
     * determine the offset closest to the pixel position. */
    isExact = FALSE;
    p = ME_FindItemBack(p, diStartRow);
    if (!p) p = editor->pBuffer->pLast;
  }

  assert( p->type == diStartRow || p == editor->pBuffer->pLast );

  if( p->type == diStartRow )
      return ME_FindRunInRow( editor, p, x, result, is_eol ) && isExact;

  result->pRun = ME_FindItemBack(p, diRun);
  result->pPara = ME_GetParagraph(result->pRun);
  result->nOffset = 0;
  assert(result->pRun->member.run.nFlags & MERF_ENDPARA);
  return FALSE;
}


/* Sets the cursor to the position closest to the pixel position
 *
 * x & y are pixel positions in client coordinates.
 *
 * isExact will be set to TRUE if the run is directly under the pixel
 * position, FALSE if it not, unless isExact is set to NULL.
 *
 * return FALSE if outside client area and the cursor is not set,
 * otherwise TRUE is returned.
 */
BOOL ME_CharFromPos(ME_TextEditor *editor, int x, int y,
                    ME_Cursor *cursor, BOOL *isExact)
{
  RECT rc;
  BOOL bResult;

  ITextHost_TxGetClientRect(editor->texthost, &rc);
  if (x < 0 || y < 0 || x >= rc.right || y >= rc.bottom) {
    if (isExact) *isExact = FALSE;
    return FALSE;
  }
  x += editor->horz_si.nPos;
  y += editor->vert_si.nPos;
  bResult = ME_FindPixelPos(editor, x, y, cursor, NULL);
  if (isExact) *isExact = bResult;
  return TRUE;
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
  curOfs = ME_GetCursorOfs(&editor->pCursors[0]);
  anchorStartOfs = ME_GetCursorOfs(&editor->pCursors[3]);
  anchorEndOfs = ME_GetCursorOfs(&editor->pCursors[2]);

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
  BOOL is_selection = FALSE, is_shift;

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
      x += ME_PointFromChar(editor, &pRun->member.run, pCursor->nOffset, TRUE);
    }
    editor->nUDArrowX = x;
  }
  return x;
}


static void
ME_MoveCursorLines(ME_TextEditor *editor, ME_Cursor *pCursor, int nRelOfs)
{
  ME_DisplayItem *pRun = pCursor->pRun;
  ME_DisplayItem *pOldPara = pCursor->pPara;
  ME_DisplayItem *pItem, *pNewPara;
  int x = ME_GetXForArrow(editor, pCursor);

  if (editor->bCaretAtEnd && !pCursor->nOffset)
    if (!ME_PrevRun(&pOldPara, &pRun))
      return;

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
  ME_FindRunInRow(editor, pItem, x, pCursor, &editor->bCaretAtEnd);
  assert(pCursor->pRun);
  assert(pCursor->pRun->type == diRun);
}

static void ME_ArrowPageUp(ME_TextEditor *editor, ME_Cursor *pCursor)
{
  ME_DisplayItem *p = ME_FindItemFwd(editor->pBuffer->pFirst, diStartRow);

  if (editor->vert_si.nPos < p->member.row.nHeight)
  {
    ME_SetCursorToStart(editor, pCursor);
    editor->bCaretAtEnd = FALSE;
    /* Native clears seems to clear this x value on page up at the top
     * of the text, but not on page down at the end of the text.
     * Doesn't make sense, but we try to be bug for bug compatible. */
    editor->nUDArrowX = -1;
  } else {
    ME_DisplayItem *pRun = pCursor->pRun;
    ME_DisplayItem *pLast;
    int x, y, yd, yp;
    int yOldScrollPos = editor->vert_si.nPos;

    x = ME_GetXForArrow(editor, pCursor);
    if (!pCursor->nOffset && editor->bCaretAtEnd)
      pRun = ME_FindItemBack(pRun, diRun);

    p = ME_FindItemBack(pRun, diStartRowOrParagraph);
    assert(p->type == diStartRow);
    yp = ME_FindItemBack(p, diParagraph)->member.para.pt.y;
    y = yp + p->member.row.pt.y;

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
    } while(1);

    ME_FindRunInRow(editor, pLast, x, pCursor, &editor->bCaretAtEnd);
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
    ME_SetCursorToEnd(editor, pCursor);
    editor->bCaretAtEnd = FALSE;
  } else {
    ME_DisplayItem *pRun = pCursor->pRun;
    ME_DisplayItem *p;
    int yd, yp;
    int yOldScrollPos = editor->vert_si.nPos;

    if (!pCursor->nOffset && editor->bCaretAtEnd)
      pRun = ME_FindItemBack(pRun, diRun);

    p = ME_FindItemBack(pRun, diStartRowOrParagraph);
    assert(p->type == diStartRow);
    yp = ME_FindItemBack(p, diParagraph)->member.para.pt.y;
    y = yp + p->member.row.pt.y;

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
    } while(1);

    ME_FindRunInRow(editor, pLast, x, pCursor, &editor->bCaretAtEnd);
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
  ME_SetCursorToStart(editor, pCursor);
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
  ME_SetCursorToEnd(editor, pCursor);
  editor->bCaretAtEnd = FALSE;
}

BOOL ME_IsSelection(ME_TextEditor *editor)
{
  return editor->pCursors[0].pRun != editor->pCursors[1].pRun ||
         editor->pCursors[0].nOffset != editor->pCursors[1].nOffset;
}

void ME_DeleteSelection(ME_TextEditor *editor)
{
  int from, to;
  int nStartCursor = ME_GetSelectionOfs(editor, &from, &to);
  int nEndCursor = nStartCursor ^ 1;
  ME_DeleteTextAtCursor(editor, nStartCursor, to - from);
  editor->pCursors[nEndCursor] = editor->pCursors[nStartCursor];
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

  sc.nmhdr.hwndFrom = NULL;
  sc.nmhdr.idFrom = 0;
  sc.nmhdr.code = EN_SELCHANGE;
  ME_GetSelectionOfs(editor, &sc.chrg.cpMin, &sc.chrg.cpMax);
  sc.seltyp = SEL_EMPTY;
  if (sc.chrg.cpMin != sc.chrg.cpMax)
    sc.seltyp |= SEL_TEXT;
  if (sc.chrg.cpMin < sc.chrg.cpMax+1) /* what were RICHEDIT authors thinking ? */
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
      editor->bCaretAtEnd = FALSE;
      if (ctrl)
        success = ME_MoveCursorWords(editor, &tmp_curs, -1);
      else
        success = ME_MoveCursorChars(editor, &tmp_curs, -1);
      break;
    case VK_RIGHT:
      editor->bCaretAtEnd = FALSE;
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
      editor->bCaretAtEnd = FALSE;
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
