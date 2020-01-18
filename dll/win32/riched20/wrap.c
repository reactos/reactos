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

typedef struct tagME_WrapContext
{
  ME_Style *style;
  ME_Context *context;
  int nLeftMargin, nRightMargin;
  int nFirstMargin;   /* Offset to first line's text, always to the text itself even if a para number is present */
  int nParaNumOffset; /* Offset to the para number */
  int nAvailWidth;    /* Width avail for text to wrap into.  Does not include any para number text */
  int nRow;
  POINT pt;
  BOOL bOverflown, bWordWrap;
  ME_DisplayItem *pPara;
  ME_DisplayItem *pRowStart;

  ME_DisplayItem *pLastSplittableRun;
} ME_WrapContext;

static BOOL get_run_glyph_buffers( ME_Run *run )
{
    heap_free( run->glyphs );
    run->glyphs = heap_alloc( run->max_glyphs * (sizeof(WORD) + sizeof(SCRIPT_VISATTR) + sizeof(int) + sizeof(GOFFSET)) );
    if (!run->glyphs) return FALSE;

    run->vis_attrs = (SCRIPT_VISATTR*)((char*)run->glyphs + run->max_glyphs * sizeof(WORD));
    run->advances = (int*)((char*)run->glyphs + run->max_glyphs * (sizeof(WORD) + sizeof(SCRIPT_VISATTR)));
    run->offsets = (GOFFSET*)((char*)run->glyphs + run->max_glyphs * (sizeof(WORD) + sizeof(SCRIPT_VISATTR) + sizeof(int)));

    return TRUE;
}

static HRESULT shape_run( ME_Context *c, ME_Run *run )
{
    HRESULT hr;
    int i;

    if (!run->glyphs)
    {
        run->max_glyphs = 1.5 * run->len + 16; /* This is suggested in the uniscribe documentation */
        run->max_glyphs = (run->max_glyphs + 7) & ~7; /* Keep alignment simple */
        get_run_glyph_buffers( run );
    }

    if (run->max_clusters < run->len)
    {
        heap_free( run->clusters );
        run->max_clusters = run->len * 2;
        run->clusters = heap_alloc( run->max_clusters * sizeof(WORD) );
    }

    select_style( c, run->style );
    while (1)
    {
        hr = ScriptShape( c->hDC, &run->style->script_cache, get_text( run, 0 ), run->len, run->max_glyphs,
                          &run->script_analysis, run->glyphs, run->clusters, run->vis_attrs, &run->num_glyphs );
        if (hr != E_OUTOFMEMORY) break;
        if (run->max_glyphs > 10 * run->len) break; /* something has clearly gone wrong */
        run->max_glyphs *= 2;
        get_run_glyph_buffers( run );
    }

    if (SUCCEEDED(hr))
        hr = ScriptPlace( c->hDC, &run->style->script_cache, run->glyphs, run->num_glyphs, run->vis_attrs,
                          &run->script_analysis, run->advances, run->offsets, NULL );

    if (SUCCEEDED(hr))
    {
        for (i = 0, run->nWidth = 0; i < run->num_glyphs; i++)
            run->nWidth += run->advances[i];
    }

    return hr;
}

/******************************************************************************
 * calc_run_extent
 *
 * Updates the size of the run (fills width, ascent and descent). The height
 * is calculated based on whole row's ascent and descent anyway, so no need
 * to use it here.
 */
static void calc_run_extent(ME_Context *c, const ME_Paragraph *para, int startx, ME_Run *run)
{
    if (run->nFlags & MERF_HIDDEN) run->nWidth = 0;
    else
    {
        SIZE size = ME_GetRunSizeCommon( c, para, run, run->len, startx, &run->nAscent, &run->nDescent );
        run->nWidth = size.cx;
    }
}

/******************************************************************************
 * split_run_extents
 *
 * Splits a run into two in a given place. It also updates the screen position
 * and size (extent) of the newly generated runs.
 */
static ME_DisplayItem *split_run_extents(ME_WrapContext *wc, ME_DisplayItem *item, int nVChar)
{
  ME_TextEditor *editor = wc->context->editor;
  ME_Run *run, *run2;
  ME_Paragraph *para = &wc->pPara->member.para;
  ME_Cursor cursor = {wc->pPara, item, nVChar};

  assert(item->member.run.nCharOfs != -1);
  ME_CheckCharOffsets(editor);

  run = &item->member.run;

  TRACE("Before split: %s(%d, %d)\n", debugstr_run( run ),
        run->pt.x, run->pt.y);

  ME_SplitRunSimple(editor, &cursor);

  run2 = &cursor.pRun->member.run;
  run2->script_analysis = run->script_analysis;

  shape_run( wc->context, run );
  shape_run( wc->context, run2 );
  calc_run_extent(wc->context, para, wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, run);

  run2->pt.x = run->pt.x+run->nWidth;
  run2->pt.y = run->pt.y;

  ME_CheckCharOffsets(editor);

  TRACE("After split: %s(%d, %d), %s(%d, %d)\n",
        debugstr_run( run ), run->pt.x, run->pt.y,
        debugstr_run( run2 ), run2->pt.x, run2->pt.y);

  return cursor.pRun;
}

/******************************************************************************
 * find_split_point
 *
 * Returns a character position to split inside the run given a run-relative
 * pixel horizontal position. This version rounds left (ie. if the second
 * character is at pixel position 8, then for cx=0..7 it returns 0).
 */
static int find_split_point( ME_Context *c, int cx, ME_Run *run )
{
    if (!run->len || cx <= 0) return 0;
    return ME_CharFromPointContext( c, cx, run, FALSE, FALSE );
}

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

  pFmt = &para->member.para.fmt;
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
      int rowIndent = ME_GetTableRowEnd(para)->member.para.fmt.dxStartIndent;
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

static void layout_row( ME_DisplayItem *start, const ME_DisplayItem *end )
{
    ME_DisplayItem *p;
    int i, num_runs = 0;
    int buf[16 * 5]; /* 5 arrays - 4 of int & 1 of BYTE, alloc space for 5 of ints */
    int *vis_to_log = buf, *log_to_vis, *widths, *pos;
    BYTE *levels;
    BOOL found_black = FALSE;

    for (p = end->prev; p != start->prev; p = p->prev)
    {
        if (p->type == diRun)
        {
            if (!found_black) found_black = !(p->member.run.nFlags & (MERF_WHITESPACE | MERF_ENDPARA));
            if (found_black) num_runs++;
        }
    }

    TRACE("%d runs\n", num_runs);
    if (!num_runs) return;

    if (num_runs > ARRAY_SIZE( buf ) / 5)
        vis_to_log = heap_alloc( num_runs * sizeof(int) * 5 );

    log_to_vis = vis_to_log + num_runs;
    widths = vis_to_log + 2 * num_runs;
    pos = vis_to_log + 3 * num_runs;
    levels = (BYTE*)(vis_to_log + 4 * num_runs);

    for (i = 0, p = start; i < num_runs; p = p->next)
    {
        if (p->type == diRun)
        {
            levels[i] = p->member.run.script_analysis.s.uBidiLevel;
            widths[i] = p->member.run.nWidth;
            TRACE( "%d: level %d width %d\n", i, levels[i], widths[i] );
            i++;
        }
    }

    ScriptLayout( num_runs, levels, vis_to_log, log_to_vis );

    pos[0] = start->member.run.para->pt.x;
    for (i = 1; i < num_runs; i++)
        pos[i] = pos[i - 1] + widths[ vis_to_log[ i - 1 ] ];

    for (i = 0, p = start; i < num_runs; p = p->next)
    {
        if (p->type == diRun)
        {
            p->member.run.pt.x = pos[ log_to_vis[ i ] ];
            TRACE( "%d: x = %d\n", i, p->member.run.pt.x );
            i++;
        }
    }

    if (vis_to_log != buf) heap_free( vis_to_log );
}

static void ME_InsertRowStart(ME_WrapContext *wc, const ME_DisplayItem *pEnd)
{
  ME_DisplayItem *p, *row;
  ME_Paragraph *para = &wc->pPara->member.para;
  BOOL bSkippingSpaces = TRUE;
  int ascent = 0, descent = 0, width=0, shift = 0, align = 0;

  /* Include height of para numbering label */
  if (wc->nRow == 0 && para->fmt.wNumbering)
  {
      ascent = para->para_num.style->tm.tmAscent;
      descent = para->para_num.style->tm.tmDescent;
  }

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
          int len = p->member.run.len;
          WCHAR *text = get_text( &p->member.run, len - 1 );

          assert (len);
          if (~p->member.run.nFlags & MERF_GRAPHICS)
            while (len && *(text--) == ' ')
              len--;
          if (len)
          {
              if (len == p->member.run.len)
                  width += p->member.run.nWidth;
              else
                  width += ME_PointFromCharContext( wc->context, &p->member.run, len, FALSE );
          }
          bSkippingSpaces = !len;
        } else if (!(p->member.run.nFlags & MERF_ENDPARA))
          width += p->member.run.nWidth;
      }
  }

  para->nWidth = max(para->nWidth, width);
  row = ME_MakeRow(ascent+descent, ascent, width);
  if (wc->context->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      (para->fmt.dwMask & PFM_TABLE) && (para->fmt.wEffects & PFE_TABLE))
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
  assert(para->fmt.dwMask & PFM_ALIGNMENT);
  align = para->fmt.wAlignment;
  if (align == PFA_CENTER)
    shift = max((wc->nAvailWidth-width)/2, 0);
  if (align == PFA_RIGHT)
    shift = max(wc->nAvailWidth-width, 0);

  if (para->nFlags & MEPF_COMPLEX) layout_row( wc->pRowStart, pEnd );

  row->member.row.pt.x = row->member.row.nLMargin + shift;
  for (p = wc->pRowStart; p!=pEnd; p = p->next)
  {
    if (p->type==diRun) { /* FIXME add more run types */
      p->member.run.pt.x += row->member.row.nLMargin+shift;
    }
  }

  if (wc->nRow == 0 && para->fmt.wNumbering)
  {
    para->para_num.pt.x = wc->nParaNumOffset + shift;
    para->para_num.pt.y = wc->pt.y + row->member.row.nBaseline;
  }

  ME_InsertBefore(wc->pRowStart, row);
  wc->nRow++;
  wc->pt.y += row->member.row.nHeight;
  ME_BeginRow(wc);
}

static void ME_WrapEndParagraph(ME_WrapContext *wc, ME_DisplayItem *p)
{
  ME_DisplayItem *para = wc->pPara;
  PARAFORMAT2 *pFmt = &para->member.para.fmt;
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
      TRACE("%s - (%d, %d)\n", debugstr_run(run), run->pt.x, run->pt.y);
    }
    p = p->next;
  }
  */
}

static void ME_WrapSizeRun(ME_WrapContext *wc, ME_DisplayItem *p)
{
  /* FIXME compose style (out of character and paragraph styles) here */

  ME_UpdateRunFlags(wc->context->editor, &p->member.run);

  calc_run_extent(wc->context, &wc->pPara->member.para,
                  wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, &p->member.run);
}


static int find_non_whitespace(const WCHAR *s, int len, int start)
{
  int i;
  for (i = start; i < len && ME_IsWSpace( s[i] ); i++)
    ;

  return i;
}

/* note: these two really return the first matching offset (starting from EOS)+1
 * in other words, an offset of the first trailing white/black */

/* note: returns offset of the first trailing whitespace */
static int reverse_find_non_whitespace(const WCHAR *s, int start)
{
  int i;
  for (i = start; i > 0 && ME_IsWSpace( s[i - 1] ); i--)
    ;

  return i;
}

/* note: returns offset of the first trailing nonwhitespace */
static int reverse_find_whitespace(const WCHAR *s, int start)
{
  int i;
  for (i = start; i > 0 && !ME_IsWSpace( s[i - 1] ); i--)
    ;

  return i;
}

static ME_DisplayItem *ME_MaximizeSplit(ME_WrapContext *wc, ME_DisplayItem *p, int i)
{
  ME_DisplayItem *pp, *piter = p;
  int j;
  if (!i)
    return NULL;
  j = reverse_find_non_whitespace( get_text( &p->member.run, 0 ), i);
  if (j>0) {
    pp = split_run_extents(wc, piter, j);
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
        i = reverse_find_non_whitespace( get_text( &piter->member.run, 0 ),
                                         piter->member.run.len );
        pp = split_run_extents(wc, piter, i);
        wc->pt = pp->member.run.pt;
        return pp;
      }
      /* this run is the end of spaces, so the run edge is a good point to split */
      wc->pt = pp->member.run.pt;
      wc->bOverflown = TRUE;
      TRACE("Split point is: %s|%s\n", debugstr_run( &piter->member.run ), debugstr_run( &pp->member.run ));
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

  idesp = i = find_split_point( wc->context, loc, run );
  len = run->len;
  assert(len>0);
  assert(i<len);
  if (i) {
    /* don't split words */
    i = reverse_find_whitespace( get_text( run, 0 ), i );
    pp = ME_MaximizeSplit(wc, p, i);
    if (pp)
      return pp;
  }
  TRACE("Must backtrack to split at: %s\n", debugstr_run( &p->member.run ));
  if (wc->pLastSplittableRun)
  {
    if (wc->pLastSplittableRun->member.run.nFlags & (MERF_GRAPHICS|MERF_TAB))
    {
      wc->pt = wc->pLastSplittableRun->member.run.pt;
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
      len = run->len;
      /* don't split words */
      i = reverse_find_whitespace( get_text( run, 0 ), len );
      if (i == len)
        i = reverse_find_non_whitespace( get_text( run, 0 ), len );
      if (i) {
        ME_DisplayItem *piter2 = split_run_extents(wc, piter, i);
        wc->pt = piter2->member.run.pt;
        return piter2;
      }
      /* splittable = must have whitespaces */
      assert(0 == "Splittable, but no whitespaces");
    }
    else
    {
      /* restart from the first run beginning with spaces */
      wc->pt = wc->pLastSplittableRun->member.run.pt;
      return wc->pLastSplittableRun;
    }
  }
  TRACE("Backtracking failed, trying desperate: %s\n", debugstr_run( &p->member.run ));
  /* OK, no better idea, so assume we MAY split words if we can split at all*/
  if (idesp)
    return split_run_extents(wc, piter, idesp);
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
      return split_run_extents(wc, piter, 1);
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
  len = run->len;

  if (wc->bOverflown) /* just skipping final whitespaces */
  {
    /* End paragraph run can't overflow to the next line by itself. */
    if (run->nFlags & MERF_ENDPARA)
      return p->next;

    if (run->nFlags & MERF_WHITESPACE) {
      wc->pt.x += run->nWidth;
      /* skip runs consisting of only whitespaces */
      return p->next;
    }

    if (run->nFlags & MERF_STARTWHITE) {
      /* try to split the run at the first non-white char */
      int black;
      black = find_non_whitespace( get_text( run, 0 ), run->len, 0 );
      if (black) {
        wc->bOverflown = FALSE;
        pp = split_run_extents(wc, p, black);
        calc_run_extent(wc->context, &wc->pPara->member.para,
                        wc->nRow ? wc->nLeftMargin : wc->nFirstMargin,
                        &pp->member.run);
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
    /* total white run or end para */
    if (run->nFlags & (MERF_WHITESPACE | MERF_ENDPARA)) {
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
      int black = reverse_find_non_whitespace( get_text( run, 0 ), len );
      split_run_extents(wc, p, black);
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
  }
  wc->pt.x += run->nWidth;
  return p->next;
}

static int ME_GetParaLineSpace(ME_Context* c, ME_Paragraph* para)
{
  int   sp = 0, ls = 0;
  if (!(para->fmt.dwMask & PFM_LINESPACING)) return 0;

  /* FIXME: how to compute simply the line space in ls ??? */
  /* FIXME: does line spacing include the line itself ??? */
  switch (para->fmt.bLineSpacingRule)
  {
  case 0:       sp = ls; break;
  case 1:       sp = (3 * ls) / 2; break;
  case 2:       sp = 2 * ls; break;
  case 3:       sp = ME_twips2pointsY(c, para->fmt.dyLineSpacing); if (sp < ls) sp = ls; break;
  case 4:       sp = ME_twips2pointsY(c, para->fmt.dyLineSpacing); break;
  case 5:       sp = para->fmt.dyLineSpacing / 20; break;
  default: FIXME("Unsupported spacing rule value %d\n", para->fmt.bLineSpacingRule);
  }
  if (c->editor->nZoomNumerator == 0)
    return sp;
  else
    return sp * c->editor->nZoomNumerator / c->editor->nZoomDenominator;
}

static void ME_PrepareParagraphForWrapping(ME_TextEditor *editor, ME_Context *c, ME_DisplayItem *tp) {
  ME_DisplayItem *p;

  tp->member.para.nWidth = 0;
  /* remove row start items as they will be reinserted by the
   * paragraph wrapper anyway */
  editor->total_rows -= tp->member.para.nRows;
  tp->member.para.nRows = 0;
  for (p = tp->next; p != tp->member.para.next_para; p = p->next) {
    if (p->type == diStartRow) {
      ME_DisplayItem *pRow = p;
      p = p->prev;
      ME_Remove(pRow);
      ME_DestroyDisplayItem(pRow);
    }
  }
  /* join runs that can be joined */
  for (p = tp->next; p != tp->member.para.next_para; p = p->next) {
    assert(p->type != diStartRow); /* should have been deleted above */
    if (p->type == diRun) {
      while (p->next->type == diRun && /* FIXME */
             ME_CanJoinRuns(&p->member.run, &p->next->member.run)) {
        ME_JoinRuns(c->editor, p);
      }
    }
  }
}

static HRESULT itemize_para( ME_Context *c, ME_DisplayItem *p )
{
    ME_Paragraph *para = &p->member.para;
    ME_Run *run;
    ME_DisplayItem *di;
    SCRIPT_ITEM buf[16], *items = buf;
    int items_passed = ARRAY_SIZE( buf ), num_items, cur_item;
    SCRIPT_CONTROL control = { LANG_USER_DEFAULT, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
                               FALSE, FALSE, 0 };
    SCRIPT_STATE state = { 0, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, 0, 0 };
    HRESULT hr;

    assert( p->type == diParagraph );

    if (para->fmt.dwMask & PFM_RTLPARA && para->fmt.wEffects & PFE_RTLPARA)
        state.uBidiLevel = 1;

    TRACE( "Base embedding level %d\n", state.uBidiLevel );

    while (1)
    {
        hr = ScriptItemize( para->text->szData, para->text->nLen, items_passed, &control,
                            &state, items, &num_items );
        if (hr != E_OUTOFMEMORY) break; /* may not be enough items if hr == E_OUTOFMEMORY */
        if (items_passed > para->text->nLen + 1) break; /* something else has gone wrong */
        items_passed *= 2;
        if (items == buf)
            items = heap_alloc( items_passed * sizeof( *items ) );
        else
            items = heap_realloc( items, items_passed * sizeof( *items ) );
        if (!items) break;
    }
    if (FAILED( hr )) goto end;

    if (TRACE_ON( richedit ))
    {
        TRACE( "got items:\n" );
        for (cur_item = 0; cur_item < num_items; cur_item++)
        {
            TRACE( "\t%d - %d RTL %d bidi level %d\n", items[cur_item].iCharPos, items[cur_item+1].iCharPos - 1,
                   items[cur_item].a.fRTL, items[cur_item].a.s.uBidiLevel );
        }

        TRACE( "before splitting runs into ranges\n" );
        for (di = p->next; di != p->member.para.next_para; di = di->next)
        {
            if (di->type != diRun) continue;
            TRACE( "\t%d: %s\n", di->member.run.nCharOfs, debugstr_run( &di->member.run ) );
        }
    }

    /* split runs into ranges at item boundaries */
    for (di = p->next, cur_item = 0; di != p->member.para.next_para; di = di->next)
    {
        if (di->type != diRun) continue;
        run = &di->member.run;

        if (run->nCharOfs == items[cur_item+1].iCharPos) cur_item++;

        items[cur_item].a.fLogicalOrder = TRUE;
        run->script_analysis = items[cur_item].a;

        if (run->nFlags & MERF_ENDPARA) break; /* don't split eop runs */

        if (run->nCharOfs + run->len > items[cur_item+1].iCharPos)
        {
            ME_Cursor cursor = {p, di, items[cur_item+1].iCharPos - run->nCharOfs};
            ME_SplitRunSimple( c->editor, &cursor );
        }
    }

    if (TRACE_ON( richedit ))
    {
        TRACE( "after splitting into ranges\n" );
        for (di = p->next; di != p->member.para.next_para; di = di->next)
        {
            if (di->type != diRun) continue;
            TRACE( "\t%d: %s\n", di->member.run.nCharOfs, debugstr_run( &di->member.run ) );
        }
    }

    para->nFlags |= MEPF_COMPLEX;

end:
    if (items != buf) heap_free( items );
    return hr;
}


static HRESULT shape_para( ME_Context *c, ME_DisplayItem *p )
{
    ME_DisplayItem *di;
    ME_Run *run;
    HRESULT hr;

    for (di = p->next; di != p->member.para.next_para; di = di->next)
    {
        if (di->type != diRun) continue;
        run = &di->member.run;

        hr = shape_run( c, run );
        if (FAILED( hr ))
        {
            run->para->nFlags &= ~MEPF_COMPLEX;
            return hr;
        }
    }
    return hr;
}

static void ME_WrapTextParagraph(ME_TextEditor *editor, ME_Context *c, ME_DisplayItem *tp) {
  ME_DisplayItem *p;
  ME_WrapContext wc;
  int border = 0;
  int linespace = 0;
  PARAFORMAT2 *pFmt;

  assert(tp->type == diParagraph);
  if (!(tp->member.para.nFlags & MEPF_REWRAP)) {
    return;
  }
  ME_PrepareParagraphForWrapping(editor, c, tp);

  /* Calculate paragraph numbering label */
  para_num_init( c, &tp->member.para );

  /* For now treating all non-password text as complex for better testing */
  if (!c->editor->cPasswordMask /* &&
      ScriptIsComplex( tp->member.para.text->szData, tp->member.para.text->nLen, SIC_COMPLEX ) == S_OK */)
  {
      if (SUCCEEDED( itemize_para( c, tp ) ))
          shape_para( c, tp );
  }

  pFmt = &tp->member.para.fmt;

  wc.context = c;
  wc.pPara = tp;
/*   wc.para_style = tp->member.para.style; */
  wc.style = NULL;
  wc.nParaNumOffset = 0;
  if (tp->member.para.nFlags & MEPF_ROWEND) {
    wc.nFirstMargin = wc.nLeftMargin = wc.nRightMargin = 0;
  } else {
    int dxStartIndent = pFmt->dxStartIndent;
    if (tp->member.para.pCell) {
      dxStartIndent += ME_GetTableRowEnd(tp)->member.para.fmt.dxOffset;
    }
    wc.nLeftMargin = ME_twips2pointsX(c, dxStartIndent + pFmt->dxOffset);
    wc.nFirstMargin = ME_twips2pointsX(c, dxStartIndent);
    if (pFmt->wNumbering)
    {
        wc.nParaNumOffset = wc.nFirstMargin;
        dxStartIndent = max( ME_twips2pointsX(c, pFmt->wNumberingTab),
                             tp->member.para.para_num.width );
        wc.nFirstMargin += dxStartIndent;
    }
    wc.nRightMargin = ME_twips2pointsX(c, pFmt->dxRightIndent);

    if (wc.nFirstMargin < 0)
        wc.nFirstMargin = 0;
    if (wc.nLeftMargin < 0)
        wc.nLeftMargin = 0;
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
    border = ME_GetParaBorderWidth(c, tp->member.para.fmt.wBorders);
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
  if (tp->member.para.fmt.dwMask & PFM_SPACEAFTER)
    wc.pt.y += ME_twips2pointsY(c, pFmt->dySpaceAfter);

  tp->member.para.nFlags &= ~MEPF_REWRAP;
  tp->member.para.nHeight = wc.pt.y;
  tp->member.para.nRows = wc.nRow;
  editor->total_rows += wc.nRow;
}

static void ME_MarkRepaintEnd(ME_DisplayItem *para,
                              ME_DisplayItem **repaint_start,
                              ME_DisplayItem **repaint_end)
{
    if (!*repaint_start)
      *repaint_start = para;
    *repaint_end = para;
}

static void adjust_para_y(ME_DisplayItem *item, ME_Context *c, ME_DisplayItem *repaint_start, ME_DisplayItem *repaint_end)
{
    if (item->member.para.nFlags & MEPF_ROWSTART)
    {
        ME_DisplayItem *cell = ME_FindItemFwd(item, diCell);
        ME_DisplayItem *endRowPara;
        int borderWidth = 0;
        cell->member.cell.pt = c->pt;
        /* Offset the text by the largest top border width. */
        while (cell->member.cell.next_cell)
        {
            borderWidth = max(borderWidth, cell->member.cell.border.top.width);
            cell = cell->member.cell.next_cell;
        }
        endRowPara = ME_FindItemFwd(cell, diParagraph);
        assert(endRowPara->member.para.nFlags & MEPF_ROWEND);
        if (borderWidth > 0)
        {
            borderWidth = max(ME_twips2pointsY(c, borderWidth), 1);
            while (cell)
            {
                cell->member.cell.yTextOffset = borderWidth;
                cell = cell->member.cell.prev_cell;
            }
            c->pt.y += borderWidth;
        }
        if (endRowPara->member.para.fmt.dxStartIndent > 0)
        {
            int dxStartIndent = endRowPara->member.para.fmt.dxStartIndent;
            cell = ME_FindItemFwd(item, diCell);
            cell->member.cell.pt.x += ME_twips2pointsX(c, dxStartIndent);
            c->pt.x = cell->member.cell.pt.x;
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
            bottomBorder = ME_twips2pointsY(c, bottomBorder);
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
        c->pt.x = startRowPara->member.para.pt.x;
        c->pt.y = cell->member.cell.pt.y + nHeight;
        if (prevHeight < nHeight)
        {
            /* The height of the cells has grown, so invalidate the bottom of
             * the cells. */
            ME_MarkRepaintEnd(item, &repaint_start, &repaint_end);
            cell = ME_FindItemBack(item, diCell);
            while (cell)
            {
                ME_MarkRepaintEnd(ME_FindItemBack(cell, diParagraph), &repaint_start, &repaint_end);
                cell = cell->member.cell.prev_cell;
            }
        }
    }
    else if (item->member.para.pCell &&
             item->member.para.pCell != item->member.para.next_para->member.para.pCell)
    {
        /* The next paragraph is in the next cell in the table row. */
        ME_Cell *cell = &item->member.para.pCell->member.cell;
        cell->nHeight = c->pt.y + item->member.para.nHeight - cell->pt.y;

        /* Propagate the largest height to the end so that it can be easily
         * sent back to all the cells at the end of the row. */
        if (cell->prev_cell)
            cell->nHeight = max(cell->nHeight, cell->prev_cell->member.cell.nHeight);

        c->pt.x = cell->pt.x + cell->nWidth;
        c->pt.y = cell->pt.y;
        cell->next_cell->member.cell.pt = c->pt;
        if (!(item->member.para.next_para->member.para.nFlags & MEPF_ROWEND))
            c->pt.y += cell->yTextOffset;
    }
    else
    {
        if (item->member.para.pCell)
        {
            /* Next paragraph in the same cell. */
            c->pt.x = item->member.para.pCell->member.cell.pt.x;
        }
        else
            /* Normal paragraph */
            c->pt.x = 0;
        c->pt.y += item->member.para.nHeight;
    }
}

BOOL ME_WrapMarkedParagraphs(ME_TextEditor *editor)
{
  ME_DisplayItem *item;
  ME_Context c;
  int totalWidth = editor->nTotalWidth, diff = 0, prev_width;
  ME_DisplayItem *repaint_start = NULL, *repaint_end = NULL;
  ME_Paragraph *para;

  if (!editor->first_marked_para)
    return FALSE;

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));

  item = editor->first_marked_para;
  c.pt = item->member.para.pt;
  while (item != editor->pBuffer->pLast)
  {
    assert(item->type == diParagraph);

    prev_width = item->member.para.nWidth;
    ME_WrapTextParagraph(editor, &c, item);
    if (prev_width == totalWidth && item->member.para.nWidth < totalWidth)
      totalWidth = get_total_width(editor);
    else
      totalWidth = max(totalWidth, item->member.para.nWidth);

    if (!item->member.para.nCharOfs)
      ME_MarkRepaintEnd(item->member.para.prev_para, &repaint_start, &repaint_end);
    ME_MarkRepaintEnd(item, &repaint_start, &repaint_end);
    adjust_para_y(item, &c, repaint_start, repaint_end);

    if (item->member.para.next_para)
    {
      diff = c.pt.y - item->member.para.next_para->member.para.pt.y;
      if (diff)
      {
        para = &item->member.para;
        while (para->next_para && para != &item->member.para.next_marked->member.para &&
               para != &editor->pBuffer->pLast->member.para)
        {
          ME_MarkRepaintEnd(para->next_para, &repaint_start, &repaint_end);
          para->next_para->member.para.pt.y = c.pt.y;
          adjust_para_y(para->next_para, &c, repaint_start, repaint_end);
          para = &para->next_para->member.para;
        }
      }
    }
    if (item->member.para.next_marked)
    {
      ME_DisplayItem *rem = item;
      item = item->member.para.next_marked;
      remove_marked_para(editor, rem);
    }
    else
    {
      remove_marked_para(editor, item);
      item = editor->pBuffer->pLast;
    }
    c.pt.y = item->member.para.pt.y;
  }
  editor->sizeWindow.cx = c.rcView.right-c.rcView.left;
  editor->sizeWindow.cy = c.rcView.bottom-c.rcView.top;

  editor->nTotalLength = c.pt.y;
  editor->nTotalWidth = totalWidth;
  editor->pBuffer->pLast->member.para.pt.x = 0;
  editor->pBuffer->pLast->member.para.pt.y = c.pt.y;

  ME_DestroyContext(&c);

  if (repaint_start || editor->nTotalLength < editor->nLastTotalLength)
    ME_InvalidateParagraphRange(editor, repaint_start, repaint_end);
  return !!repaint_start;
}

void ME_InvalidateParagraphRange(ME_TextEditor *editor,
                                 ME_DisplayItem *start_para,
                                 ME_DisplayItem *last_para)
{
  ME_Context c;
  RECT rc;
  int ofs;

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));
  rc = c.rcView;
  ofs = editor->vert_si.nPos;

  if (start_para) {
    start_para = ME_GetOuterParagraph(start_para);
    last_para = ME_GetOuterParagraph(last_para);
    rc.top = c.rcView.top + start_para->member.para.pt.y - ofs;
  } else {
    rc.top = c.rcView.top + editor->nTotalLength - ofs;
  }
  if (editor->nTotalLength < editor->nLastTotalLength)
    rc.bottom = c.rcView.top + editor->nLastTotalLength - ofs;
  else
    rc.bottom = c.rcView.top + last_para->member.para.pt.y + last_para->member.para.nHeight - ofs;
  ITextHost_TxInvalidateRect(editor->texthost, &rc, TRUE);

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

      info.nmhdr.hwndFrom = NULL;
      info.nmhdr.idFrom = 0;
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
