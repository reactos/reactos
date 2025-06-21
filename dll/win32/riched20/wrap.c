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
  ME_Paragraph *para;
  ME_Run *pRowStart;
  ME_Run *pLastSplittableRun;
} ME_WrapContext;

static BOOL get_run_glyph_buffers( ME_Run *run )
{
    free( run->glyphs );
    run->glyphs = malloc( run->max_glyphs * (sizeof(WORD) + sizeof(SCRIPT_VISATTR) + sizeof(int) + sizeof(GOFFSET)) );
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
        free( run->clusters );
        run->max_clusters = run->len * 2;
        run->clusters = malloc( run->max_clusters * sizeof(WORD) );
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
static ME_Run *split_run_extents( ME_WrapContext *wc, ME_Run *run, int nVChar )
{
  ME_TextEditor *editor = wc->context->editor;
  ME_Run *run2;
  ME_Cursor cursor = { wc->para, run, nVChar };

  assert( run->nCharOfs != -1 );
  ME_CheckCharOffsets(editor);

  TRACE("Before split: %s(%ld, %ld)\n", debugstr_run( run ),
        run->pt.x, run->pt.y);

  run_split( editor, &cursor );

  run2 = cursor.run;
  run2->script_analysis = run->script_analysis;

  shape_run( wc->context, run );
  shape_run( wc->context, run2 );
  calc_run_extent(wc->context, wc->para, wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, run);

  run2->pt.x = run->pt.x + run->nWidth;
  run2->pt.y = run->pt.y;

  ME_CheckCharOffsets(editor);

  TRACE("After split: %s(%ld, %ld), %s(%ld, %ld)\n",
        debugstr_run( run ), run->pt.x, run->pt.y,
        debugstr_run( run2 ), run2->pt.x, run2->pt.y);

  return cursor.run;
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

static ME_Row *row_create( int height, int baseline, int width )
{
  ME_DisplayItem *item = ME_MakeDI(diStartRow);

  item->member.row.nHeight = height;
  item->member.row.nBaseline = baseline;
  item->member.row.nWidth = width;
  return &item->member.row;
}

static void ME_BeginRow(ME_WrapContext *wc)
{
  ME_Cell *cell;

  wc->pRowStart = NULL;
  wc->bOverflown = FALSE;
  wc->pLastSplittableRun = NULL;
  wc->bWordWrap = wc->context->editor->bWordWrap;
  if (wc->para->nFlags & (MEPF_ROWSTART | MEPF_ROWEND))
  {
    wc->nAvailWidth = 0;
    wc->bWordWrap = FALSE;
    if (wc->para->nFlags & MEPF_ROWEND)
    {
      cell = table_row_end_cell( wc->para );
      cell->nWidth = 0;
    }
  }
  else if (para_cell( wc->para ))
  {
    int width;

    cell = para_cell( wc->para );
    width = cell->nRightBoundary;
    if (cell_prev( cell )) width -= cell_prev( cell )->nRightBoundary;
    else
    {
      int rowIndent = table_row_end( wc->para )->fmt.dxStartIndent;
      width -= rowIndent;
    }
    cell->nWidth = max(ME_twips2pointsX(wc->context, width), 0);

    wc->nAvailWidth = cell->nWidth
        - (wc->nRow ? wc->nLeftMargin : wc->nFirstMargin) - wc->nRightMargin;
    wc->bWordWrap = TRUE;
  }
  else
    wc->nAvailWidth = wc->context->nAvailWidth
        - (wc->nRow ? wc->nLeftMargin : wc->nFirstMargin) - wc->nRightMargin;

  wc->pt.x = wc->context->pt.x;
  if (wc->context->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      para_in_table( wc->para ))
    /* Shift the text down because of the border. */
    wc->pt.y++;
}

static void layout_row( ME_Run *start, ME_Run *last )
{
    ME_Run *run;
    int i, num_runs = 0;
    int buf[16 * 5]; /* 5 arrays - 4 of int & 1 of BYTE, alloc space for 5 of ints */
    int *vis_to_log = buf, *log_to_vis, *widths, *pos;
    BYTE *levels;
    BOOL found_black = FALSE;

    for (run = last; run; run = run_prev( run ))
    {
        if (!found_black) found_black = !(run->nFlags & (MERF_WHITESPACE | MERF_ENDPARA));
        if (found_black) num_runs++;
        if (run == start) break;
    }

    TRACE("%d runs\n", num_runs);
    if (!num_runs) return;

    if (num_runs > ARRAY_SIZE( buf ) / 5)
        vis_to_log = malloc( num_runs * sizeof(int) * 5 );

    log_to_vis = vis_to_log + num_runs;
    widths = vis_to_log + 2 * num_runs;
    pos = vis_to_log + 3 * num_runs;
    levels = (BYTE*)(vis_to_log + 4 * num_runs);

    for (i = 0, run = start; i < num_runs; run = run_next( run ))
    {
        levels[i] = run->script_analysis.s.uBidiLevel;
        widths[i] = run->nWidth;
        TRACE( "%d: level %d width %d\n", i, levels[i], widths[i] );
        i++;
    }

    ScriptLayout( num_runs, levels, vis_to_log, log_to_vis );

    pos[0] = run->para->pt.x;
    for (i = 1; i < num_runs; i++)
        pos[i] = pos[i - 1] + widths[ vis_to_log[ i - 1 ] ];

    for (i = 0, run = start; i < num_runs; run = run_next( run ))
    {
        run->pt.x = pos[ log_to_vis[ i ] ];
        TRACE( "%d: x = %ld\n", i, run->pt.x );
        i++;
    }

    if (vis_to_log != buf) free( vis_to_log );
}

static void ME_InsertRowStart( ME_WrapContext *wc, ME_Run *last )
{
    ME_Run *run;
    ME_Row *row;
    BOOL bSkippingSpaces = TRUE;
    int ascent = 0, descent = 0, width = 0, shift = 0, align = 0;

    /* Include height of para numbering label */
    if (wc->nRow == 0 && wc->para->fmt.wNumbering)
    {
        ascent = wc->para->para_num.style->tm.tmAscent;
        descent = wc->para->para_num.style->tm.tmDescent;
    }

    for (run = last; run; run = run_prev( run ))
    {
        /* ENDPARA run shouldn't affect row height, except if it's the only run in the paragraph */
        if (run == wc->pRowStart || !(run->nFlags & MERF_ENDPARA))
        {
            if (run->nAscent > ascent) ascent = run->nAscent;
            if (run->nDescent > descent) descent = run->nDescent;
            if (bSkippingSpaces)
            {
                /* Exclude space characters from run width.
                 * Other whitespace or delimiters are not treated this way. */
                int len = run->len;
                WCHAR *text = get_text( run, len - 1 );

                assert(len);
                if (~run->nFlags & MERF_GRAPHICS)
                    while (len && *(text--) == ' ') len--;
                if (len)
                {
                    if (len == run->len)
                        width += run->nWidth;
                    else
                        width += ME_PointFromCharContext( wc->context, run, len, FALSE );
                }
                bSkippingSpaces = !len;
            }
            else if (!(run->nFlags & MERF_ENDPARA))
                width += run->nWidth;
        }
        if (run == wc->pRowStart) break;
    }

    wc->para->nWidth = max( wc->para->nWidth, width );
    row = row_create( ascent + descent, ascent, width );
    if (wc->context->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
        (wc->para->fmt.dwMask & PFM_TABLE) && (wc->para->fmt.wEffects & PFE_TABLE))
    {
        /* The text was shifted down in ME_BeginRow so move the wrap context
         * back to where it should be. */
        wc->pt.y--;
        /* The height of the row is increased by the borders. */
        row->nHeight += 2;
    }
    row->pt = wc->pt;
    row->nLMargin = (!wc->nRow ? wc->nFirstMargin : wc->nLeftMargin);
    row->nRMargin = wc->nRightMargin;
    assert(wc->para->fmt.dwMask & PFM_ALIGNMENT);
    align = wc->para->fmt.wAlignment;
    if (align == PFA_CENTER) shift = max((wc->nAvailWidth-width)/2, 0);
    if (align == PFA_RIGHT) shift = max(wc->nAvailWidth-width, 0);

    if (wc->para->nFlags & MEPF_COMPLEX) layout_row( wc->pRowStart, last );

    row->pt.x = row->nLMargin + shift;

    for (run = wc->pRowStart; run; run = run_next( run ))
    {
        run->pt.x += row->nLMargin + shift;
        if (run == last) break;
    }

    if (wc->nRow == 0 && wc->para->fmt.wNumbering)
    {
        wc->para->para_num.pt.x = wc->nParaNumOffset + shift;
        wc->para->para_num.pt.y = wc->pt.y + row->nBaseline;
    }

    ME_InsertBefore( run_get_di( wc->pRowStart ), row_get_di( row ) );
    wc->nRow++;
    wc->pt.y += row->nHeight;
    ME_BeginRow( wc );
}

static void ME_WrapEndParagraph( ME_WrapContext *wc )
{
  if (wc->pRowStart) ME_InsertRowStart( wc, wc->para->eop_run );

  if (wc->context->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      wc->para->fmt.dwMask & PFM_TABLE && wc->para->fmt.wEffects & PFE_TABLE)
  {
    /* ME_BeginRow was called an extra time for the paragraph, and it shifts the
     * text down by one pixel for the border, so fix up the wrap context. */
    wc->pt.y--;
  }
}

static void ME_WrapSizeRun( ME_WrapContext *wc, ME_Run *run )
{
  /* FIXME compose style (out of character and paragraph styles) here */

  ME_UpdateRunFlags( wc->context->editor, run );

  calc_run_extent( wc->context, wc->para,
                   wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, run );
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

static ME_Run *ME_MaximizeSplit( ME_WrapContext *wc, ME_Run *run, int i )
{
  ME_Run *new_run, *iter = run;
  int j;
  if (!i)
    return NULL;
  j = reverse_find_non_whitespace( get_text( run, 0 ), i );
  if (j > 0)
  {
    new_run = split_run_extents( wc, iter, j );
    wc->pt.x += iter->nWidth;
    return new_run;
  }
  else
  {
    new_run = iter;
    /* omit all spaces before split point */
    while (iter != wc->pRowStart)
    {
      iter = run_prev( iter );
      if (iter->nFlags & MERF_WHITESPACE)
      {
        new_run = iter;
        continue;
      }
      if (iter->nFlags & MERF_ENDWHITE)
      {
        i = reverse_find_non_whitespace( get_text( iter, 0 ), iter->len );
        new_run = split_run_extents( wc, iter, i );
        wc->pt = new_run->pt;
        return new_run;
      }
      /* this run is the end of spaces, so the run edge is a good point to split */
      wc->pt = new_run->pt;
      wc->bOverflown = TRUE;
      TRACE( "Split point is: %s|%s\n", debugstr_run( iter ), debugstr_run( new_run ) );
      return new_run;
    }
    wc->pt = iter->pt;
    return iter;
  }
}

static ME_Run *ME_SplitByBacktracking( ME_WrapContext *wc, ME_Run *run, int loc )
{
  ME_Run *new_run;
  int i, idesp, len;

  idesp = i = find_split_point( wc->context, loc, run );
  len = run->len;
  assert( len > 0 );
  assert( i < len );
  if (i)
  {
    /* don't split words */
    i = reverse_find_whitespace( get_text( run, 0 ), i );
    new_run = ME_MaximizeSplit(wc, run, i);
    if (new_run) return new_run;
  }
  TRACE("Must backtrack to split at: %s\n", debugstr_run( run ));
  if (wc->pLastSplittableRun)
  {
    if (wc->pLastSplittableRun->nFlags & (MERF_GRAPHICS|MERF_TAB))
    {
      wc->pt = wc->pLastSplittableRun->pt;
      return wc->pLastSplittableRun;
    }
    else if (wc->pLastSplittableRun->nFlags & MERF_SPLITTABLE)
    {
      /* the following two lines are just to check if we forgot to call UpdateRunFlags earlier,
         they serve no other purpose */
      ME_UpdateRunFlags(wc->context->editor, run);
      assert((wc->pLastSplittableRun->nFlags & MERF_SPLITTABLE));

      run = wc->pLastSplittableRun;
      len = run->len;
      /* don't split words */
      i = reverse_find_whitespace( get_text( run, 0 ), len );
      if (i == len)
        i = reverse_find_non_whitespace( get_text( run, 0 ), len );
      new_run = split_run_extents(wc, run, i);
      wc->pt = new_run->pt;
      return new_run;
    }
    else
    {
      /* restart from the first run beginning with spaces */
      wc->pt = wc->pLastSplittableRun->pt;
      return wc->pLastSplittableRun;
    }
  }
  TRACE("Backtracking failed, trying desperate: %s\n", debugstr_run( run ));
  /* OK, no better idea, so assume we MAY split words if we can split at all*/
  if (idesp)
    return split_run_extents(wc, run, idesp);
  else
  if (wc->pRowStart && run != wc->pRowStart)
  {
    /* don't need to break current run, because it's possible to split
       before this run */
    wc->bOverflown = TRUE;
    return run;
  }
  else
  {
    /* split point inside first character - no choice but split after that char */
    if (len != 1)
      /* the run is more than 1 char, so we may split */
      return split_run_extents( wc, run, 1 );

    /* the run is one char, can't split it */
    return run;
  }
}

static ME_Run *ME_WrapHandleRun( ME_WrapContext *wc, ME_Run *run )
{
  ME_Run *new_run;
  int len;

  if (!wc->pRowStart) wc->pRowStart = run;
  run->pt.x = wc->pt.x;
  run->pt.y = wc->pt.y;
  ME_WrapSizeRun( wc, run );
  len = run->len;

  if (wc->bOverflown) /* just skipping final whitespaces */
  {
    /* End paragraph run can't overflow to the next line by itself. */
    if (run->nFlags & MERF_ENDPARA) return run_next( run );

    if (run->nFlags & MERF_WHITESPACE)
    {
      wc->pt.x += run->nWidth;
      /* skip runs consisting of only whitespaces */
      return run_next( run );
    }

    if (run->nFlags & MERF_STARTWHITE)
    {
      /* try to split the run at the first non-white char */
      int black;
      black = find_non_whitespace( get_text( run, 0 ), run->len, 0 );
      if (black)
      {
        ME_Run *new_run;
        wc->bOverflown = FALSE;
        new_run = split_run_extents( wc, run, black );
        calc_run_extent( wc->context, wc->para,
                         wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, run );
        ME_InsertRowStart( wc, run );
        return new_run;
      }
    }
    /* black run: the row goes from pRowStart to the previous run */
    ME_InsertRowStart( wc, run_prev( run ) );
    return run;
  }
  /* simply end the current row and move on to next one */
  if (run->nFlags & MERF_ENDROW)
  {
    ME_InsertRowStart( wc, run );
    return run_next( run );
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
      return run;
    }
    /* TAB: we can split before */
    if (run->nFlags & MERF_TAB) {
      wc->bOverflown = TRUE;
      if (wc->pRowStart == run)
        /* Don't split before the start of the run, or we will get an
         * endless loop. */
        return run_next( run );
      else
        return run;
    }
    /* graphics: we can split before, if run's width is smaller than row's width */
    if ((run->nFlags & MERF_GRAPHICS) && run->nWidth <= wc->nAvailWidth) {
      wc->bOverflown = TRUE;
      return run;
    }
    /* can we separate out the last spaces ? (to use overflow logic later) */
    if (run->nFlags & MERF_ENDWHITE)
    {
      /* we aren't sure if it's *really* necessary, it's a good start however */
      int black = reverse_find_non_whitespace( get_text( run, 0 ), len );
      split_run_extents( wc, run, black );
      /* handle both parts again */
      return run;
    }
    /* determine the split point by backtracking */
    new_run = ME_SplitByBacktracking( wc, run, loc );
    if (new_run == wc->pRowStart)
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
          return run;
      }
      /* Couldn't split the first run, possible because we have a large font
       * with a single character that caused an overflow.
       */
      wc->pt.x += run->nWidth;
      return run_next( run );
    }
    if (run != new_run) /* found a suitable split point */
    {
      wc->bOverflown = TRUE;
      return new_run;
    }
    /* we detected that it's best to split on start of this run */
    if (wc->bOverflown)
      return new_run;
    ERR("failure!\n");
    /* not found anything - writing over margins is the only option left */
  }
  if ((run->nFlags & (MERF_SPLITTABLE | MERF_STARTWHITE))
    || ((run->nFlags & (MERF_GRAPHICS|MERF_TAB)) && (run != wc->pRowStart)))
  {
    wc->pLastSplittableRun = run;
  }
  wc->pt.x += run->nWidth;
  return run_next( run );
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

static void ME_PrepareParagraphForWrapping( ME_TextEditor *editor, ME_Context *c, ME_Paragraph *para )
{
    ME_DisplayItem *p;

    para->nWidth = 0;
    /* remove row start items as they will be reinserted by the
     * paragraph wrapper anyway */
    editor->total_rows -= para->nRows;
    para->nRows = 0;
    for (p = para_get_di( para ); p != para->next_para; p = p->next)
    {
        if (p->type == diStartRow)
        {
            ME_DisplayItem *pRow = p;
            p = p->prev;
            ME_Remove( pRow );
            ME_DestroyDisplayItem( pRow );
        }
    }

    /* join runs that can be joined */
    for (p = para_get_di( para )->next; p != para->next_para; p = p->next)
    {
        assert(p->type != diStartRow); /* should have been deleted above */
        if (p->type == diRun)
        {
            while (p->next->type == diRun && ME_CanJoinRuns( &p->member.run, &p->next->member.run ))
                run_join( c->editor, &p->member.run );
        }
    }
}

static HRESULT itemize_para( ME_Context *c, ME_Paragraph *para )
{
    ME_Run *run;
    SCRIPT_ITEM buf[16], *items = buf;
    int items_passed = ARRAY_SIZE( buf ), num_items, cur_item;
    SCRIPT_CONTROL control = { LANG_USER_DEFAULT, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
                               FALSE, FALSE, 0 };
    SCRIPT_STATE state = { 0, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, 0, 0 };
    HRESULT hr;

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
            items = malloc( items_passed * sizeof( *items ) );
        else
            items = realloc( items, items_passed * sizeof( *items ) );
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
        for (run = para_first_run( para ); run; run = run_next( run ))
            TRACE( "\t%d: %s\n", run->nCharOfs, debugstr_run( run ) );
    }

    /* split runs into ranges at item boundaries */
    for (run = para_first_run( para ), cur_item = 0; run; run = run_next( run ))
    {
        if (run->nCharOfs == items[cur_item+1].iCharPos) cur_item++;

        items[cur_item].a.fLogicalOrder = TRUE;
        run->script_analysis = items[cur_item].a;

        if (run->nFlags & MERF_ENDPARA) break; /* don't split eop runs */

        if (run->nCharOfs + run->len > items[cur_item+1].iCharPos)
        {
            ME_Cursor cursor = { para, run, items[cur_item + 1].iCharPos - run->nCharOfs };
            run_split( c->editor, &cursor );
        }
    }

    if (TRACE_ON( richedit ))
    {
        TRACE( "after splitting into ranges\n" );
        for (run = para_first_run( para ); run; run = run_next( run ))
            TRACE( "\t%d: %s\n", run->nCharOfs, debugstr_run( run ) );
    }

    para->nFlags |= MEPF_COMPLEX;

end:
    if (items != buf) free( items );
    return hr;
}


static HRESULT shape_para( ME_Context *c, ME_Paragraph *para )
{
    ME_Run *run;
    HRESULT hr;

    for (run = para_first_run( para ); run; run = run_next( run ))
    {
        hr = shape_run( c, run );
        if (FAILED( hr ))
        {
            para->nFlags &= ~MEPF_COMPLEX;
            return hr;
        }
    }
    return hr;
}

static void ME_WrapTextParagraph( ME_TextEditor *editor, ME_Context *c, ME_Paragraph *para )
{
  ME_Run *run;
  ME_WrapContext wc;
  int border = 0;
  int linespace = 0;

  if (!(para->nFlags & MEPF_REWRAP)) return;

  ME_PrepareParagraphForWrapping( editor, c, para );

  /* Calculate paragraph numbering label */
  para_num_init( c, para );

  /* For now treating all non-password text as complex for better testing */
  if (!c->editor->password_char /* &&
      ScriptIsComplex( tp->member.para.text->szData, tp->member.para.text->nLen, SIC_COMPLEX ) == S_OK */)
  {
      if (SUCCEEDED( itemize_para( c, para ) ))
          shape_para( c, para );
  }
  else
  {
      /* If the user has just converted a normal rich editor with already
       * existing text into a password input, the text may contain paragraphs
       * with MEPF_COMPLEX set.  Since we don't really shape any paragraphs
       * here, we need to ensure that the MEPF_COMPLEX flag is unset.
       */
      para->nFlags &= ~MEPF_COMPLEX;
  }

  wc.context = c;
  wc.para = para;
  wc.style = NULL;
  wc.nParaNumOffset = 0;
  if (para->nFlags & MEPF_ROWEND)
    wc.nFirstMargin = wc.nLeftMargin = wc.nRightMargin = 0;
  else
  {
    int dxStartIndent = para->fmt.dxStartIndent;
    if (para_cell( wc.para )) dxStartIndent += table_row_end( para )->fmt.dxOffset;

    wc.nLeftMargin = ME_twips2pointsX( c, dxStartIndent + para->fmt.dxOffset );
    wc.nFirstMargin = ME_twips2pointsX( c, dxStartIndent );
    if (para->fmt.wNumbering)
    {
        wc.nParaNumOffset = wc.nFirstMargin;
        dxStartIndent = max( ME_twips2pointsX(c, para->fmt.wNumberingTab),
                             para->para_num.width );
        wc.nFirstMargin += dxStartIndent;
    }
    wc.nRightMargin = ME_twips2pointsX( c, para->fmt.dxRightIndent );

    if (wc.nFirstMargin < 0) wc.nFirstMargin = 0;
    if (wc.nLeftMargin < 0) wc.nLeftMargin = 0;
  }
  if (c->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
      para->fmt.dwMask & PFM_TABLE && para->fmt.wEffects & PFE_TABLE)
  {
    wc.nFirstMargin += ME_twips2pointsX( c, para->fmt.dxOffset * 2 );
  }
  wc.nRow = 0;
  wc.pt.y = 0;
  if (para->fmt.dwMask & PFM_SPACEBEFORE)
    wc.pt.y += ME_twips2pointsY( c, para->fmt.dySpaceBefore );
  if (!(para->fmt.dwMask & PFM_TABLE && para->fmt.wEffects & PFE_TABLE) &&
      para->fmt.dwMask & PFM_BORDER)
  {
    border = ME_GetParaBorderWidth( c, para->fmt.wBorders );
    if (para->fmt.wBorders & 1)
    {
      wc.nFirstMargin += border;
      wc.nLeftMargin += border;
    }
    if (para->fmt.wBorders & 2) wc.nRightMargin -= border;
    if (para->fmt.wBorders & 4) wc.pt.y += border;
  }

  linespace = ME_GetParaLineSpace( c, para );

  ME_BeginRow( &wc );
  run = para_first_run( para );
  while (run)
  {
    run = ME_WrapHandleRun( &wc, run );
    if (wc.nRow && run == wc.pRowStart) wc.pt.y += linespace;
  }
  ME_WrapEndParagraph( &wc );
  if (!(para->fmt.dwMask & PFM_TABLE && para->fmt.wEffects & PFE_TABLE) &&
      (para->fmt.dwMask & PFM_BORDER) && (para->fmt.wBorders & 8))
    wc.pt.y += border;
  if (para->fmt.dwMask & PFM_SPACEAFTER)
      wc.pt.y += ME_twips2pointsY( c, para->fmt.dySpaceAfter );

  para->nFlags &= ~MEPF_REWRAP;
  para->nHeight = wc.pt.y;
  para->nRows = wc.nRow;
  editor->total_rows += wc.nRow;
}

struct repaint_range
{
    ME_Paragraph *start, *end;
};

static void update_repaint( ME_Paragraph *para, struct repaint_range *repaint )
{
    if (!repaint->start) repaint->start = para;
    repaint->end = para;
}

static void adjust_para_y( ME_Paragraph *para, ME_Context *c, struct repaint_range *repaint )
{
    ME_Cell *cell;

    if (para->nFlags & MEPF_ROWSTART)
    {
        ME_Paragraph *end_row_para = table_row_end( para );
        int borderWidth = 0;

        cell = table_row_first_cell( para );
        cell->pt = c->pt;
        /* Offset the text by the largest top border width. */
        while (cell_next( cell ))
        {
            borderWidth = max( borderWidth, cell->border.top.width );
            cell = cell_next( cell );
        }
        if (borderWidth > 0)
        {
            borderWidth = max(ME_twips2pointsY(c, borderWidth), 1);
            while (cell)
            {
                cell->yTextOffset = borderWidth;
                cell = cell_prev( cell );
            }
            c->pt.y += borderWidth;
        }
        if (end_row_para->fmt.dxStartIndent > 0)
        {
            cell = table_row_first_cell( para );
            cell->pt.x += ME_twips2pointsX( c, end_row_para->fmt.dxStartIndent );
            c->pt.x = cell->pt.x;
        }
    }
    else if (para->nFlags & MEPF_ROWEND)
    {
        /* Set all the cells to the height of the largest cell */
        ME_Paragraph *start_row_para = table_row_start( para );
        int prevHeight, nHeight, bottomBorder = 0;

        cell = table_row_end_cell( para );
        para->nWidth = cell->pt.x + cell->nWidth;
        if (!(para_next( para )->nFlags & MEPF_ROWSTART))
        {
            /* Last row, the bottom border is added to the height. */
            while ((cell = cell_prev( cell )))
                bottomBorder = max( bottomBorder, cell->border.bottom.width );

            bottomBorder = ME_twips2pointsY(c, bottomBorder);
            cell = table_row_end_cell( para );
        }
        prevHeight = cell->nHeight;
        nHeight = cell_prev( cell )->nHeight + bottomBorder;
        cell->nHeight = nHeight;
        para->nHeight = nHeight;
        while (cell_prev( cell ))
        {
            cell = cell_prev( cell );
            cell->nHeight = nHeight;
        }

        /* Also set the height of the start row paragraph */
        start_row_para->nHeight = nHeight;
        c->pt.x = start_row_para->pt.x;
        c->pt.y = cell->pt.y + nHeight;
        if (prevHeight < nHeight)
        {
            /* The height of the cells has grown, so invalidate the bottom of
             * the cells. */
            update_repaint( para, repaint );
            cell = cell_prev( table_row_end_cell( para ) );
            while (cell)
            {
                update_repaint( cell_end_para( cell ), repaint );
                cell = cell_prev( cell );
            }
        }
    }
    else if ((cell = para_cell( para )) && para == cell_end_para( cell ))
    {
        /* The next paragraph is in the next cell in the table row. */
        cell->nHeight = c->pt.y + para->nHeight - cell->pt.y;

        /* Propagate the largest height to the end so that it can be easily
         * sent back to all the cells at the end of the row. */
        if (cell_prev( cell ))
            cell->nHeight = max( cell->nHeight, cell_prev( cell )->nHeight );

        c->pt.x = cell->pt.x + cell->nWidth;
        c->pt.y = cell->pt.y;
        cell_next( cell )->pt = c->pt;
        if (!(para_next( para )->nFlags & MEPF_ROWEND))
            c->pt.y += cell->yTextOffset;
    }
    else
    {
        if ((cell = para_cell( para ))) /* Next paragraph in the same cell. */
            c->pt.x = cell->pt.x;
        else /* Normal paragraph */
            c->pt.x = 0;
        c->pt.y += para->nHeight;
    }
}

BOOL wrap_marked_paras_dc( ME_TextEditor *editor, HDC hdc, BOOL invalidate )
{
  ME_Paragraph *para, *next;
  struct wine_rb_entry *entry, *next_entry = NULL;
  ME_Context c;
  int totalWidth = editor->nTotalWidth, prev_width;
  struct repaint_range repaint = { NULL, NULL };

  if (!editor->marked_paras.root) return FALSE;

  ME_InitContext( &c, editor, hdc );

  entry = rb_head( editor->marked_paras.root );
  while (entry)
  {
    para = WINE_RB_ENTRY_VALUE( entry, ME_Paragraph, marked_entry );

    /* If the first entry lies inside a table, go back to the start
       of the table to ensure cell heights are kept in sync. */
    if (!next_entry && para_in_table( para ) && para != table_outer_para( para ))
    {
        para = table_outer_para( para );
        next_entry = entry;
    }
    else
        next_entry = rb_next( entry );

    c.pt = para->pt;
    prev_width = para->nWidth;
    ME_WrapTextParagraph( editor, &c, para );
    if (prev_width == totalWidth && para->nWidth < totalWidth)
      totalWidth = get_total_width(editor);
    else
      totalWidth = max(totalWidth, para->nWidth);

    update_repaint( para, &repaint );
    adjust_para_y( para, &c, &repaint );

    if (para_next( para ))
    {
      if (c.pt.x != para_next( para )->pt.x || c.pt.y != para_next( para )->pt.y ||
          para_in_table( para ))
      {
        next = para;
        while (para_next( next ) && &next->marked_entry != next_entry &&
               next != &editor->pBuffer->pLast->member.para)
        {
          update_repaint( para_next( next ), &repaint );
          para_next( next )->pt = c.pt;
          adjust_para_y( para_next( next ), &c, &repaint );
          next = para_next( next );
        }
      }
    }
    entry = next_entry;
  }
  wine_rb_destroy( &editor->marked_paras, NULL, NULL );

  editor->sizeWindow.cx = c.rcView.right-c.rcView.left;
  editor->sizeWindow.cy = c.rcView.bottom-c.rcView.top;

  editor->nTotalLength = editor->pBuffer->pLast->member.para.pt.y;
  editor->nTotalWidth = totalWidth;

  ME_DestroyContext(&c);

  if (invalidate && (repaint.start || editor->nTotalLength < editor->nLastTotalLength))
    para_range_invalidate( editor, repaint.start, repaint.end);
  return !!repaint.start;
}

BOOL ME_WrapMarkedParagraphs( ME_TextEditor *editor )
{
    HDC hdc = ITextHost_TxGetDC( editor->texthost );
    BOOL ret = wrap_marked_paras_dc( editor, hdc, TRUE );
    ITextHost_TxReleaseDC( editor->texthost, hdc );
    return ret;
}

void para_range_invalidate( ME_TextEditor *editor, ME_Paragraph *start_para,
                            ME_Paragraph *last_para )
{
  RECT rc;
  int ofs;

  rc = editor->rcFormat;
  ofs = editor->vert_si.nPos;

  if (start_para)
  {
    start_para = table_outer_para( start_para );
    last_para = table_outer_para( last_para );
    rc.top += start_para->pt.y - ofs;
  } else {
    rc.top += editor->nTotalLength - ofs;
  }
  if (editor->nTotalLength < editor->nLastTotalLength)
    rc.bottom = editor->rcFormat.top + editor->nLastTotalLength - ofs;
  else
    rc.bottom = editor->rcFormat.top + last_para->pt.y + last_para->nHeight - ofs;
  ITextHost_TxInvalidateRect(editor->texthost, &rc, TRUE);
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
