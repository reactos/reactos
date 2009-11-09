/*
 * RichEdit - Paragraph wrapping. Don't try to understand it. You've been
 * warned !
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


#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

/*
 * Unsolved problems:
 *
 * - center and right align in WordPad omits all spaces at the start, we don't
 * - objects/images are not handled yet
 * - no tabs
 */

static ME_DisplayItem *ME_MakeRow(int height, int baseline, int width)
{
  ME_DisplayItem *item = ME_MakeDI(diStartRow);

  item->member.row.nHeight = height;
  item->member.row.nBaseline = baseline;
  item->member.row.nWidth = width;
  return item;
}

static void ME_BeginRow(ME_WrapContext *wc)
{
  PARAFORMAT2 *pFmt;
  ME_DisplayItem *para = wc->pPara;

  pFmt = para->member.para.pFmt;
  wc->pRowStart = NULL;
  wc->bOverflown = FALSE;
  wc->pLastSplittableRun = NULL;
  wc->bWordWrap = wc->context->editor->bWordWrap;
  if (para->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND)) {
    wc->nAvailWidth = 0;
    wc->bWordWrap = FALSE;
    if (para->member.para.nFlags & MEPF_ROWEND)
    {
      ME_Cell *cell = &ME_FindItemBack(para, diCell)->member.cell;
      cell->nWidth = 0;
    }
  } else if (para->member.para.pCell) {
    ME_Cell *cell = &para->member.para.pCell->member.cell;
    int width;

    width = cell->nRightBoundary;
    if (cell->prev_cell)
      width -= cell->prev_cell->member.cell.nRightBoundary;
    if (!cell->prev_cell)
    {
      int rowIndent = ME_GetTableRowEnd(para)->member.para.pFmt->dxStartIndent;
      width -= rowIndent;
    }
    cell->nWidth = max(ME_twips2pointsX(wc->context, width), 0);

    wc->nAvailWidth = cell->nWidth
        - (wc->nRow ? wc->nLeftMargin : wc->nFirstMargin) - wc->nRightMargin;
    wc->bWordWrap = TRUE;
  } else {
    wc->nAvailWidth = wc->context->nAvailWidth
        - (wc->nRow ? wc->nLeftMargin : wc->nFirstMargin) - wc->nRightMargin;
  }
  wc->pt.x = wc->context->pt.x;
  if (wc->context->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE)
    /* Shift the text down because of the border. */
    wc->pt.y++;
}

static void ME_InsertRowStart(ME_WrapContext *wc, const ME_DisplayItem *pEnd)
{
  ME_DisplayItem *p, *row, *para;
  BOOL bSkippingSpaces = TRUE;
  int ascent = 0, descent = 0, width=0, shift = 0, align = 0;
  PARAFORMAT2 *pFmt;
  /* wrap text */
  para = wc->pPara;
  pFmt = para->member.para.pFmt;

  for (p = pEnd->prev; p!=wc->pRowStart->prev; p = p->prev)
  {
      /* ENDPARA run shouldn't affect row height, except if it's the only run in the paragraph */
      if (p->type==diRun && ((p==wc->pRowStart) || !(p->member.run.nFlags & MERF_ENDPARA))) { /* FIXME add more run types */
        if (p->member.run.nAscent>ascent)
          ascent = p->member.run.nAscent;
        if (p->member.run.nDescent>descent)
          descent = p->member.run.nDescent;
        if (bSkippingSpaces)
        {
          /* Exclude space characters from run width.
           * Other whitespace or delimiters are not treated this way. */
          SIZE sz;
          int len = p->member.run.strText->nLen;
          WCHAR *text = p->member.run.strText->szData + len - 1;

          assert (len);
          if (~p->member.run.nFlags & MERF_GRAPHICS)
            while (len && *(text--) == ' ')
              len--;
          if (len)
          {
              if (len == p->member.run.strText->nLen)
              {
                  width += p->member.run.nWidth;
              } else {
                  sz = ME_GetRunSize(wc->context, &para->member.para,
                                     &p->member.run, len, p->member.run.pt.x);
                  width += sz.cx;
              }
          }
          bSkippingSpaces = !len;
        } else if (!(p->member.run.nFlags & MERF_ENDPARA))
          width += p->member.run.nWidth;
      }
  }

  para->member.para.nWidth = max(para->member.para.nWidth, width);
  row = ME_MakeRow(ascent+descent, ascent, width);
  if (wc->context->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE)
  {
    /* The text was shifted down in ME_BeginRow so move the wrap context
     * back to where it should be. */
    wc->pt.y--;
    /* The height of the row is increased by the borders. */
    row->member.row.nHeight += 2;
  }
  row->member.row.pt = wc->pt;
  row->member.row.nLMargin = (!wc->nRow ? wc->nFirstMargin : wc->nLeftMargin);
  row->member.row.nRMargin = wc->nRightMargin;
  assert(para->member.para.pFmt->dwMask & PFM_ALIGNMENT);
  align = para->member.para.pFmt->wAlignment;
  if (align == PFA_CENTER)
    shift = max((wc->nAvailWidth-width)/2, 0);
  if (align == PFA_RIGHT)
    shift = max(wc->nAvailWidth-width, 0);
  for (p = wc->pRowStart; p!=pEnd; p = p->next)
  {
    if (p->type==diRun) { /* FIXME add more run types */
      p->member.run.pt.x += row->member.row.nLMargin+shift;
    }
  }
  ME_InsertBefore(wc->pRowStart, row);
  wc->nRow++;
  wc->pt.y += row->member.row.nHeight;
  ME_BeginRow(wc);
}

static void ME_WrapEndParagraph(ME_WrapContext *wc, ME_DisplayItem *p)
{
  ME_DisplayItem *para = wc->pPara;
  PARAFORMAT2 *pFmt = para->member.para.pFmt;
  if (wc->pRowStart)
    ME_InsertRowStart(wc, p);
  if (wc->context->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE)
  {
    /* ME_BeginRow was called an extra time for the paragraph, and it shifts the
     * text down by one pixel for the border, so fix up the wrap context. */
    wc->pt.y--;
  }

  /*
  p = para->next;
  while(p) {
    if (p->type == diParagraph || p->type == diTextEnd)
      return;
    if (p->type == diRun)
    {
      ME_Run *run = &p->member.run;
      TRACE("%s - (%d, %d)\n", debugstr_w(run->strText->szData), run->pt.x, run->pt.y);
    }
    p = p->next;
  }
  */
}

static void ME_WrapSizeRun(ME_WrapContext *wc, ME_DisplayItem *p)
{
  /* FIXME compose style (out of character and paragraph styles) here */

  ME_UpdateRunFlags(wc->context->editor, &p->member.run);

  ME_CalcRunExtent(wc->context, &wc->pPara->member.para,
                   wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, &p->member.run);
}

static ME_DisplayItem *ME_MaximizeSplit(ME_WrapContext *wc, ME_DisplayItem *p, int i)
{
  ME_DisplayItem *pp, *piter = p;
  int j;
  if (!i)
    return NULL;
  j = ME_ReverseFindNonWhitespaceV(p->member.run.strText, i);
  if (j>0) {
    pp = ME_SplitRun(wc, piter, j);
    wc->pt.x += piter->member.run.nWidth;
    return pp;
  }
  else
  {
    pp = piter;
    /* omit all spaces before split point */
    while(piter != wc->pRowStart)
    {
      piter = ME_FindItemBack(piter, diRun);
      if (piter->member.run.nFlags & MERF_WHITESPACE)
      {
        pp = piter;
        continue;
      }
      if (piter->member.run.nFlags & MERF_ENDWHITE)
      {
        j = ME_ReverseFindNonWhitespaceV(piter->member.run.strText, i);
        pp = ME_SplitRun(wc, piter, i);
        wc->pt = pp->member.run.pt;
        return pp;
      }
      /* this run is the end of spaces, so the run edge is a good point to split */
      wc->pt = pp->member.run.pt;
      wc->bOverflown = TRUE;
      TRACE("Split point is: %s|%s\n", debugstr_w(piter->member.run.strText->szData), debugstr_w(pp->member.run.strText->szData));
      return pp;
    }
    wc->pt = piter->member.run.pt;
    return piter;
  }
}

static ME_DisplayItem *ME_SplitByBacktracking(ME_WrapContext *wc, ME_DisplayItem *p, int loc)
{
  ME_DisplayItem *piter = p, *pp;
  int i, idesp, len;
  ME_Run *run = &p->member.run;

  idesp = i = ME_CharFromPoint(wc->context, loc, run);
  len = run->strText->nLen;
  assert(len>0);
  assert(i<len);
  if (i) {
    /* don't split words */
    i = ME_ReverseFindWhitespaceV(run->strText, i);
    pp = ME_MaximizeSplit(wc, p, i);
    if (pp)
      return pp;
  }
  TRACE("Must backtrack to split at: %s\n", debugstr_w(p->member.run.strText->szData));
  if (wc->pLastSplittableRun)
  {
    if (wc->pLastSplittableRun->member.run.nFlags & (MERF_GRAPHICS|MERF_TAB))
    {
      wc->pt = wc->ptLastSplittableRun;
      return wc->pLastSplittableRun;
    }
    else if (wc->pLastSplittableRun->member.run.nFlags & MERF_SPLITTABLE)
    {
      /* the following two lines are just to check if we forgot to call UpdateRunFlags earlier,
         they serve no other purpose */
      ME_UpdateRunFlags(wc->context->editor, run);
      assert((wc->pLastSplittableRun->member.run.nFlags & MERF_SPLITTABLE));

      piter = wc->pLastSplittableRun;
      run = &piter->member.run;
      len = run->strText->nLen;
      /* don't split words */
      i = ME_ReverseFindWhitespaceV(run->strText, len);
      if (i == len)
        i = ME_ReverseFindNonWhitespaceV(run->strText, len);
      if (i) {
        ME_DisplayItem *piter2 = ME_SplitRun(wc, piter, i);
        wc->pt = piter2->member.run.pt;
        return piter2;
      }
      /* splittable = must have whitespaces */
      assert(0 == "Splittable, but no whitespaces");
    }
    else
    {
      /* restart from the first run beginning with spaces */
      wc->pt = wc->ptLastSplittableRun;
      return wc->pLastSplittableRun;
    }
  }
  TRACE("Backtracking failed, trying desperate: %s\n", debugstr_w(p->member.run.strText->szData));
  /* OK, no better idea, so assume we MAY split words if we can split at all*/
  if (idesp)
    return ME_SplitRun(wc, piter, idesp);
  else
  if (wc->pRowStart && piter != wc->pRowStart)
  {
    /* don't need to break current run, because it's possible to split
       before this run */
    wc->bOverflown = TRUE;
    return piter;
  }
  else
  {
    /* split point inside first character - no choice but split after that char */
    if (len != 1) {
      /* the run is more than 1 char, so we may split */
      return ME_SplitRun(wc, piter, 1);
    }
    /* the run is one char, can't split it */
    return piter;
  }
}

static ME_DisplayItem *ME_WrapHandleRun(ME_WrapContext *wc, ME_DisplayItem *p)
{
  ME_DisplayItem *pp;
  ME_Run *run;
  int len;

  assert(p->type == diRun);
  if (!wc->pRowStart)
    wc->pRowStart = p;
  run = &p->member.run;
  run->pt.x = wc->pt.x;
  run->pt.y = wc->pt.y;
  ME_WrapSizeRun(wc, p);
  len = run->strText->nLen;

  if (wc->bOverflown) /* just skipping final whitespaces */
  {
    /* End paragraph run can't overflow to the next line by itself. */
    if (run->nFlags & MERF_ENDPARA)
      return p->next;

    if (run->nFlags & MERF_WHITESPACE) {
      p->member.run.nFlags |= MERF_SKIPPED;
      wc->pt.x += run->nWidth;
      /* skip runs consisting of only whitespaces */
      return p->next;
    }

    if (run->nFlags & MERF_STARTWHITE) {
      /* try to split the run at the first non-white char */
      int black;
      black = ME_FindNonWhitespaceV(run->strText, 0);
      if (black) {
        wc->bOverflown = FALSE;
        pp = ME_SplitRun(wc, p, black);
        p->member.run.nFlags |= MERF_SKIPPED;
        ME_InsertRowStart(wc, pp);
        return pp;
      }
    }
    /* black run: the row goes from pRowStart to the previous run */
    ME_InsertRowStart(wc, p);
    return p;
  }
  /* simply end the current row and move on to next one */
  if (run->nFlags & MERF_ENDROW)
  {
    p = p->next;
    ME_InsertRowStart(wc, p);
    return p;
  }

  /* will current run fit? */
  if (wc->bWordWrap &&
      wc->pt.x + run->nWidth - wc->context->pt.x > wc->nAvailWidth)
  {
    int loc = wc->context->pt.x + wc->nAvailWidth - wc->pt.x;
    /* total white run ? */
    if (run->nFlags & MERF_WHITESPACE) {
      /* let the overflow logic handle it */
      wc->bOverflown = TRUE;
      return p;
    }
    /* TAB: we can split before */
    if (run->nFlags & MERF_TAB) {
      wc->bOverflown = TRUE;
      if (wc->pRowStart == p)
        /* Don't split before the start of the run, or we will get an
         * endless loop. */
        return p->next;
      else
        return p;
    }
    /* graphics: we can split before, if run's width is smaller than row's width */
    if ((run->nFlags & MERF_GRAPHICS) && run->nWidth <= wc->nAvailWidth) {
      wc->bOverflown = TRUE;
      return p;
    }
    /* can we separate out the last spaces ? (to use overflow logic later) */
    if (run->nFlags & MERF_ENDWHITE)
    {
      /* we aren't sure if it's *really* necessary, it's a good start however */
      int black = ME_ReverseFindNonWhitespaceV(run->strText, len);
      ME_SplitRun(wc, p, black);
      /* handle both parts again */
      return p;
    }
    /* determine the split point by backtracking */
    pp = ME_SplitByBacktracking(wc, p, loc);
    if (pp == wc->pRowStart)
    {
      if (run->nFlags & MERF_STARTWHITE)
      {
          /* We had only spaces so far, so we must be on the first line of the
           * paragraph (or the first line after MERF_ENDROW forced the line
           * break within the paragraph), since no other lines of the paragraph
           * start with spaces. */

          /* The lines will only contain spaces, and the rest of the run will
           * overflow onto the next line. */
          wc->bOverflown = TRUE;
          return p;
      }
      /* Couldn't split the first run, possible because we have a large font
       * with a single character that caused an overflow.
       */
      wc->pt.x += run->nWidth;
      return p->next;
    }
    if (p != pp) /* found a suitable split point */
    {
      wc->bOverflown = TRUE;
      return pp;
    }
    /* we detected that it's best to split on start of this run */
    if (wc->bOverflown)
      return pp;
    ERR("failure!\n");
    /* not found anything - writing over margins is the only option left */
  }
  if ((run->nFlags & (MERF_SPLITTABLE | MERF_STARTWHITE))
    || ((run->nFlags & (MERF_GRAPHICS|MERF_TAB)) && (p != wc->pRowStart)))
  {
    wc->pLastSplittableRun = p;
    wc->ptLastSplittableRun = wc->pt;
  }
  wc->pt.x += run->nWidth;
  return p->next;
}

static void ME_PrepareParagraphForWrapping(ME_Context *c, ME_DisplayItem *tp);

static void ME_WrapTextParagraph(ME_Context *c, ME_DisplayItem *tp) {
  ME_DisplayItem *p;
  ME_WrapContext wc;
  int border = 0;
  int linespace = 0;
  PARAFORMAT2 *pFmt;

  assert(tp->type == diParagraph);
  if (!(tp->member.para.nFlags & MEPF_REWRAP)) {
    return;
  }
  ME_PrepareParagraphForWrapping(c, tp);
  pFmt = tp->member.para.pFmt;

  wc.context = c;
  wc.pPara = tp;
/*   wc.para_style = tp->member.para.style; */
  wc.style = NULL;
  if (tp->member.para.nFlags & MEPF_ROWEND) {
    wc.nFirstMargin = wc.nLeftMargin = wc.nRightMargin = 0;
  } else {
    int dxStartIndent = pFmt->dxStartIndent;
    if (tp->member.para.pCell) {
      dxStartIndent += ME_GetTableRowEnd(tp)->member.para.pFmt->dxOffset;
    }
    wc.nFirstMargin = ME_twips2pointsX(c, dxStartIndent);
    wc.nLeftMargin = wc.nFirstMargin + ME_twips2pointsX(c, pFmt->dxOffset);
    wc.nRightMargin = ME_twips2pointsX(c, pFmt->dxRightIndent);
  }
  if (c->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE)
  {
    wc.nFirstMargin += ME_twips2pointsX(c, pFmt->dxOffset * 2);
  }
  wc.nRow = 0;
  wc.pt.y = 0;
  if (pFmt->dwMask & PFM_SPACEBEFORE)
    wc.pt.y += ME_twips2pointsY(c, pFmt->dySpaceBefore);
  if (!(pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE) &&
      pFmt->dwMask & PFM_BORDER)
  {
    border = ME_GetParaBorderWidth(c->editor, tp->member.para.pFmt->wBorders);
    if (pFmt->wBorders & 1) {
      wc.nFirstMargin += border;
      wc.nLeftMargin += border;
    }
    if (pFmt->wBorders & 2)
      wc.nRightMargin -= border;
    if (pFmt->wBorders & 4)
      wc.pt.y += border;
  }

  linespace = ME_GetParaLineSpace(c, &tp->member.para);

  ME_BeginRow(&wc);
  for (p = tp->next; p!=tp->member.para.next_para; ) {
    assert(p->type != diStartRow);
    if (p->type == diRun) {
      p = ME_WrapHandleRun(&wc, p);
    }
    else p = p->next;
    if (wc.nRow && p == wc.pRowStart)
      wc.pt.y += linespace;
  }
  ME_WrapEndParagraph(&wc, p);
  if (!(pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE) &&
      (pFmt->dwMask & PFM_BORDER) && (pFmt->wBorders & 8))
    wc.pt.y += border;
  if (tp->member.para.pFmt->dwMask & PFM_SPACEAFTER)
    wc.pt.y += ME_twips2pointsY(c, pFmt->dySpaceAfter);

  tp->member.para.nFlags &= ~MEPF_REWRAP;
  tp->member.para.nHeight = wc.pt.y;
  tp->member.para.nRows = wc.nRow;
}


static void ME_PrepareParagraphForWrapping(ME_Context *c, ME_DisplayItem *tp) {
  ME_DisplayItem *p, *pRow;

  tp->member.para.nWidth = 0;
  /* remove all items that will be reinserted by paragraph wrapper anyway */
  tp->member.para.nRows = 0;
  for (p = tp->next; p!=tp->member.para.next_para; p = p->next) {
    switch(p->type) {
      case diStartRow:
        pRow = p;
        p = p->prev;
        ME_Remove(pRow);
        ME_DestroyDisplayItem(pRow);
        break;
      default:
        break;
    }
  }
  /* join runs that can be joined, set up flags */
  for (p = tp->next; p!=tp->member.para.next_para; p = p->next) {
    int changed = 0;
    switch(p->type) {
      case diStartRow: assert(0); break; /* should have deleted it */
      case diRun:
        while (p->next->type == diRun) { /* FIXME */
          if (ME_CanJoinRuns(&p->member.run, &p->next->member.run)) {
            ME_JoinRuns(c->editor, p);
            changed = 1;
          }
          else
            break;
        }
        p->member.run.nFlags &= ~MERF_CALCBYWRAP;
        break;
      default:
        break;
    }
  }
}

BOOL ME_WrapMarkedParagraphs(ME_TextEditor *editor)
{
  ME_DisplayItem *item;
  ME_Context c;
  BOOL bModified = FALSE;
  int yStart = -1;
  int totalWidth = 0;

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));
  c.pt.x = 0;
  item = editor->pBuffer->pFirst->next;
  while(item != editor->pBuffer->pLast) {
    BOOL bRedraw = FALSE;

    assert(item->type == diParagraph);
    if ((item->member.para.nFlags & MEPF_REWRAP)
     || (item->member.para.pt.y != c.pt.y))
      bRedraw = TRUE;
    item->member.para.pt = c.pt;

    ME_WrapTextParagraph(&c, item);

    if (bRedraw)
    {
      item->member.para.nFlags |= MEPF_REPAINT;
      if (yStart == -1)
        yStart = c.pt.y;
    }

    bModified = bModified | bRedraw;

    if (item->member.para.nFlags & MEPF_ROWSTART)
    {
      ME_DisplayItem *cell = ME_FindItemFwd(item, diCell);
      ME_DisplayItem *endRowPara;
      int borderWidth = 0;
      cell->member.cell.pt = c.pt;
      /* Offset the text by the largest top border width. */
      while (cell->member.cell.next_cell) {
        borderWidth = max(borderWidth, cell->member.cell.border.top.width);
        cell = cell->member.cell.next_cell;
      }
      endRowPara = ME_FindItemFwd(cell, diParagraph);
      assert(endRowPara->member.para.nFlags & MEPF_ROWEND);
      if (borderWidth > 0)
      {
        borderWidth = max(ME_twips2pointsY(&c, borderWidth), 1);
        while (cell) {
          cell->member.cell.yTextOffset = borderWidth;
          cell = cell->member.cell.prev_cell;
        }
        c.pt.y += borderWidth;
      }
      if (endRowPara->member.para.pFmt->dxStartIndent > 0)
      {
        int dxStartIndent = endRowPara->member.para.pFmt->dxStartIndent;
        cell = ME_FindItemFwd(item, diCell);
        cell->member.cell.pt.x += ME_twips2pointsX(&c, dxStartIndent);
        c.pt.x = cell->member.cell.pt.x;
      }
    }
    else if (item->member.para.nFlags & MEPF_ROWEND)
    {
      /* Set all the cells to the height of the largest cell */
      ME_DisplayItem *startRowPara;
      int prevHeight, nHeight, bottomBorder = 0;
      ME_DisplayItem *cell = ME_FindItemBack(item, diCell);
      item->member.para.nWidth = cell->member.cell.pt.x + cell->member.cell.nWidth;
      if (!(item->member.para.next_para->member.para.nFlags & MEPF_ROWSTART))
      {
        /* Last row, the bottom border is added to the height. */
        cell = cell->member.cell.prev_cell;
        while (cell)
        {
          bottomBorder = max(bottomBorder, cell->member.cell.border.bottom.width);
          cell = cell->member.cell.prev_cell;
        }
        bottomBorder = ME_twips2pointsY(&c, bottomBorder);
        cell = ME_FindItemBack(item, diCell);
      }
      prevHeight = cell->member.cell.nHeight;
      nHeight = cell->member.cell.prev_cell->member.cell.nHeight + bottomBorder;
      cell->member.cell.nHeight = nHeight;
      item->member.para.nHeight = nHeight;
      cell = cell->member.cell.prev_cell;
      cell->member.cell.nHeight = nHeight;
      while (cell->member.cell.prev_cell)
      {
        cell = cell->member.cell.prev_cell;
        cell->member.cell.nHeight = nHeight;
      }
      /* Also set the height of the start row paragraph */
      startRowPara = ME_FindItemBack(cell, diParagraph);
      startRowPara->member.para.nHeight = nHeight;
      c.pt.x = startRowPara->member.para.pt.x;
      c.pt.y = cell->member.cell.pt.y + nHeight;
      if (prevHeight < nHeight)
      {
        /* The height of the cells has grown, so invalidate the bottom of
         * the cells. */
        item->member.para.nFlags |= MEPF_REPAINT;
        cell = ME_FindItemBack(item, diCell);
        while (cell) {
          ME_FindItemBack(cell, diParagraph)->member.para.nFlags |= MEPF_REPAINT;
          cell = cell->member.cell.prev_cell;
        }
      }
    }
    else if (item->member.para.pCell &&
             item->member.para.pCell != item->member.para.next_para->member.para.pCell)
    {
      /* The next paragraph is in the next cell in the table row. */
      ME_Cell *cell = &item->member.para.pCell->member.cell;
      cell->nHeight = c.pt.y + item->member.para.nHeight - cell->pt.y;

      /* Propagate the largest height to the end so that it can be easily
       * sent back to all the cells at the end of the row. */
      if (cell->prev_cell)
        cell->nHeight = max(cell->nHeight, cell->prev_cell->member.cell.nHeight);

      c.pt.x = cell->pt.x + cell->nWidth;
      c.pt.y = cell->pt.y;
      cell->next_cell->member.cell.pt = c.pt;
      if (!(item->member.para.next_para->member.para.nFlags & MEPF_ROWEND))
        c.pt.y += cell->yTextOffset;
    }
    else
    {
      if (item->member.para.pCell) {
        /* Next paragraph in the same cell. */
        c.pt.x = item->member.para.pCell->member.cell.pt.x;
      } else {
        /* Normal paragraph */
        c.pt.x = 0;
      }
      c.pt.y += item->member.para.nHeight;
    }

    totalWidth = max(totalWidth, item->member.para.nWidth);
    item = item->member.para.next_para;
  }
  editor->sizeWindow.cx = c.rcView.right-c.rcView.left;
  editor->sizeWindow.cy = c.rcView.bottom-c.rcView.top;

  editor->nTotalLength = c.pt.y;
  editor->nTotalWidth = totalWidth;
  editor->pBuffer->pLast->member.para.pt.x = 0;
  editor->pBuffer->pLast->member.para.pt.y = c.pt.y;

  ME_DestroyContext(&c);

  if (bModified || editor->nTotalLength < editor->nLastTotalLength)
    ME_InvalidateMarkedParagraphs(editor);
  return bModified;
}

void ME_InvalidateMarkedParagraphs(ME_TextEditor *editor)
{
  ME_Context c;
  RECT rc;
  int ofs;
  ME_DisplayItem *item;

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));
  rc = c.rcView;
  ofs = editor->vert_si.nPos;

  item = editor->pBuffer->pFirst;
  while(item != editor->pBuffer->pLast) {
    if (item->member.para.nFlags & MEPF_REPAINT) {
      rc.top = c.rcView.top + item->member.para.pt.y - ofs;
      rc.bottom = max(c.rcView.top + item->member.para.pt.y
                      + item->member.para.nHeight - ofs,
                      c.rcView.bottom);
      ITextHost_TxInvalidateRect(editor->texthost, &rc, TRUE);
    }
    item = item->member.para.next_para;
  }
  if (editor->nTotalLength < editor->nLastTotalLength)
  {
    rc.top = c.rcView.top + editor->nTotalLength - ofs;
    rc.bottom = c.rcView.top + editor->nLastTotalLength - ofs;
    ITextHost_TxInvalidateRect(editor->texthost, &rc, TRUE);
  }
  ME_DestroyContext(&c);
}


void
ME_SendRequestResize(ME_TextEditor *editor, BOOL force)
{
  if (editor->nEventMask & ENM_REQUESTRESIZE)
  {
    RECT rc;

    ITextHost_TxGetClientRect(editor->texthost, &rc);

    if (force || rc.bottom != editor->nTotalLength)
    {
      REQRESIZE info;

      info.nmhdr.code = EN_REQUESTRESIZE;
      info.rc = rc;
      info.rc.right = editor->nTotalWidth;
      info.rc.bottom = editor->nTotalLength;

      editor->nEventMask &= ~ENM_REQUESTRESIZE;
      ITextHost_TxNotify(editor->texthost, info.nmhdr.code, &info);
      editor->nEventMask |= ENM_REQUESTRESIZE;
    }
  }
}
