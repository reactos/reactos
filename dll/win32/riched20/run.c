/*
 * RichEdit - operations on runs (diRun, rectangular pieces of paragraphs).
 * Splitting/joining runs. Adjusting offsets after deleting/adding content.
 * Character/pixel conversions.
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2006 by Phil Krylov
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
WINE_DECLARE_DEBUG_CHANNEL(richedit_check);
WINE_DECLARE_DEBUG_CHANNEL(richedit_lists);

BOOL cursor_next_run( ME_Cursor *cursor, BOOL all_para )
{
    ME_DisplayItem *p = run_get_di( cursor->run )->next;

    while (p->type != diTextEnd)
    {
        if (p->type == diParagraph && !all_para) return FALSE;
        else if (p->type == diRun)
        {
            cursor->run = &p->member.run;
            cursor->para = cursor->run->para;
            cursor->nOffset = 0;
            return TRUE;
        }
        p = p->next;
    }
    return FALSE;
}

BOOL cursor_prev_run( ME_Cursor *cursor, BOOL all_para )
{
    ME_DisplayItem *p = run_get_di( cursor->run )->prev;

    while (p->type != diTextStart)
    {
        if (p->type == diParagraph && !all_para) return FALSE;
        else if (p->type == diRun)
        {
            cursor->run = &p->member.run;
            cursor->para = cursor->run->para;
            cursor->nOffset = 0;
            return TRUE;
        }
        p = p->prev;
    }
    return FALSE;
}

ME_Run *run_next( ME_Run *run )
{
    ME_Cursor cursor;

    cursor.run = run;
    cursor.para = run->para;
    cursor.nOffset = 0;

    if (cursor_next_run( &cursor, FALSE ))
        return cursor.run;

    return NULL;
}

ME_Run *run_prev( ME_Run *run )
{
    ME_Cursor cursor;

    cursor.run = run;
    cursor.para = run->para;
    cursor.nOffset = 0;

    if (cursor_prev_run( &cursor, FALSE ))
        return cursor.run;

    return NULL;
}

ME_Run *run_next_all_paras( ME_Run *run )
{
    ME_Cursor cursor;

    cursor.run = run;
    cursor.para = run->para;
    cursor.nOffset = 0;

    if (cursor_next_run( &cursor, TRUE ))
        return cursor.run;

    return NULL;
}

ME_Run *run_prev_all_paras( ME_Run *run )
{
    ME_Cursor cursor;

    cursor.run = run;
    cursor.para = run->para;
    cursor.nOffset = 0;

    if (cursor_prev_run( &cursor, TRUE ))
        return cursor.run;

    return NULL;
}

/******************************************************************************
 * ME_CanJoinRuns
 *
 * Returns TRUE if two runs can be safely merged into one, FALSE otherwise.
 */
BOOL ME_CanJoinRuns(const ME_Run *run1, const ME_Run *run2)
{
  if ((run1->nFlags | run2->nFlags) & MERF_NOJOIN)
    return FALSE;
  if (run1->style != run2->style)
    return FALSE;
  if ((run1->nFlags & MERF_STYLEFLAGS) != (run2->nFlags & MERF_STYLEFLAGS))
    return FALSE;
  return TRUE;
}

/******************************************************************************
 * editor_propagate_char_ofs
 *
 * Shifts (increases or decreases) character offset (relative to beginning of 
 * the document) of the part of the text starting from given place.
 * Call with only one of para or run non-NULL.
 */ 
void editor_propagate_char_ofs( ME_TextEditor *editor, ME_Paragraph *para, ME_Run *run, int shift )
{
    assert( !para ^ !run );

    if (run)
    {
        para = para_next( run->para );
        do
        {
            run->nCharOfs += shift;
            run = run_next( run );
        } while (run);
    }

    do
    {
        /* update position in marked tree, if added */
        if (para->nFlags & MEPF_REWRAP)
            para_mark_remove( editor, para );
        para->nCharOfs += shift;
        if (para->nFlags & MEPF_REWRAP)
            para_mark_add( editor, para );
        para = para_next( para );
    } while (para);
}

/******************************************************************************
 * ME_CheckCharOffsets
 * 
 * Checks if editor lists' validity and optionally dumps the document structure
 */      
void ME_CheckCharOffsets(ME_TextEditor *editor)
{
  ME_DisplayItem *p = editor->pBuffer->pFirst;
  int ofs = 0, ofsp = 0;

  if (!TRACE_ON(richedit_check))
    return;

  TRACE_(richedit_check)("Checking begin\n");
  if(TRACE_ON(richedit_lists))
  {
    TRACE_(richedit_lists)("---\n");
    ME_DumpDocument(editor->pBuffer);
  }
  do {
    p = ME_FindItemFwd(p, diRunOrParagraphOrEnd);
    switch(p->type) {
      case diTextEnd:
        TRACE_(richedit_check)("tend, real ofsp = %d, counted = %d\n", p->member.para.nCharOfs, ofsp+ofs);
        assert(ofsp+ofs == p->member.para.nCharOfs);
        TRACE_(richedit_check)("Checking finished\n");
        return;
      case diParagraph:
        TRACE_(richedit_check)("para, real ofsp = %d, counted = %d\n", p->member.para.nCharOfs, ofsp+ofs);
        assert(ofsp+ofs == p->member.para.nCharOfs);
        ofsp = p->member.para.nCharOfs;
        ofs = 0;
        break;
      case diRun:
        TRACE_(richedit_check)("run, real ofs = %d (+ofsp = %d), counted = %d, len = %d, txt = %s, flags=%08x, fx&mask = %08lx\n",
          p->member.run.nCharOfs, p->member.run.nCharOfs+ofsp, ofsp+ofs,
          p->member.run.len, debugstr_run( &p->member.run ),
          p->member.run.nFlags,
          p->member.run.style->fmt.dwMask & p->member.run.style->fmt.dwEffects);
        assert(ofs == p->member.run.nCharOfs);
        assert(p->member.run.len);
        ofs += p->member.run.len;
        break;
      case diCell:
        TRACE_(richedit_check)("cell\n");
        break;
      default:
        assert(0);
    }
  } while(1);
  TRACE_(richedit_check)("Checking finished\n");
}

/******************************************************************************
 * run_char_ofs
 *
 * Converts a character position relative to the start of the run to a
 * character position relative to the start of the document.
 */

int run_char_ofs( ME_Run *run, int ofs )
{
    return run->para->nCharOfs + run->nCharOfs + ofs;
}

/******************************************************************************
 * cursor_from_char_ofs
 *
 * Converts a character offset (relative to the start of the document) to
 * a cursor structure (which contains a run and a position relative to that
 * run).
 */
void cursor_from_char_ofs( ME_TextEditor *editor, int char_ofs, ME_Cursor *cursor )
{
    ME_Paragraph *para;
    ME_Run *run;

    char_ofs = min( max( char_ofs, 0 ), ME_GetTextLength( editor ) );

    /* Find the paragraph at the offset. */
    for (para = editor_first_para( editor );
         para_next( para )->nCharOfs <= char_ofs;
         para = para_next( para ))
        ;

    char_ofs -= para->nCharOfs;

    /* Find the run at the offset. */
    for (run = para_first_run( para );
         run_next( run ) && run_next( run )->nCharOfs <= char_ofs;
         run = run_next( run ))
        ;

    char_ofs -= run->nCharOfs;

    cursor->para = para;
    cursor->run = run;
    cursor->nOffset = char_ofs;
}

/******************************************************************************
 * run_join
 * 
 * Merges two adjacent runs, the one given as a parameter and the next one.
 */    
void run_join( ME_TextEditor *editor, ME_Run *run )
{
  ME_Run *next = run_next( run );
  int i;

  assert( run );
  assert( run->nCharOfs != -1 );
  para_mark_rewrap( editor, run->para );

  /* Update all cursors so that they don't contain the soon deleted run */
  for (i = 0; i < editor->nCursors; i++)
  {
    if (editor->pCursors[i].run == next)
    {
      editor->pCursors[i].run = run;
      editor->pCursors[i].nOffset += run->len;
    }
  }

  run->len += next->len;
  ME_Remove( run_get_di( next ) );
  ME_DestroyDisplayItem( run_get_di( next ) );
  ME_UpdateRunFlags( editor, run );
  ME_CheckCharOffsets( editor );
}

/******************************************************************************
 * run_split
 *
 * Does the most basic job of splitting a run into two - it does not
 * update the positions and extents.
 */
ME_Run *run_split( ME_TextEditor *editor, ME_Cursor *cursor )
{
    ME_Run *run = cursor->run, *new_run;
    int i;
    int nOffset = cursor->nOffset;

    assert( !(run->nFlags & MERF_NONTEXT) );

    new_run = run_create( run->style, run->nFlags & MERF_SPLITMASK );
    new_run->nCharOfs = run->nCharOfs + nOffset;
    new_run->len = run->len - nOffset;
    new_run->para = run->para;
    run->len = nOffset;
    cursor->run = new_run;
    cursor->nOffset = 0;

    ME_InsertBefore( run_get_di( run )->next, run_get_di( new_run ) );

    ME_UpdateRunFlags( editor, run );
    ME_UpdateRunFlags( editor, new_run );
    for (i = 0; i < editor->nCursors; i++)
    {
        if (editor->pCursors[i].run == run &&
            editor->pCursors[i].nOffset >= nOffset)
        {
            editor->pCursors[i].run = new_run;
            editor->pCursors[i].nOffset -= nOffset;
        }
    }
    para_mark_rewrap( editor, run->para );
    return run;
}

/******************************************************************************
 * run_create
 * 
 * A helper function to create run structures quickly.
 */   
ME_Run *run_create( ME_Style *s, int flags )
{
    ME_DisplayItem *item = ME_MakeDI( diRun );
    ME_Run *run = &item->member.run;

    if (!item) return NULL;

    ME_AddRefStyle( s );
    run->style = s;
    run->reobj = NULL;
    run->nFlags = flags;
    run->nCharOfs = -1;
    run->len = 0;
    run->para = NULL;
    run->num_glyphs = 0;
    run->max_glyphs = 0;
    run->glyphs = NULL;
    run->vis_attrs = NULL;
    run->advances = NULL;
    run->offsets = NULL;
    run->max_clusters = 0;
    run->clusters = NULL;
    return run;
}

/******************************************************************************
 * run_insert
 *
 * Inserts a new run with given style, flags and content at a given position,
 * which is passed as a cursor structure (which consists of a run and 
 * a run-relative character offset).
 */
ME_Run *run_insert( ME_TextEditor *editor, ME_Cursor *cursor, ME_Style *style,
                    const WCHAR *str, int len, int flags )
{
  ME_Run *insert_before = cursor->run, *run, *prev;

  if (cursor->nOffset)
  {
    if (cursor->nOffset == insert_before->len)
    {
      insert_before = run_next_all_paras( insert_before );
      if (!insert_before) insert_before = cursor->run; /* Always insert before the final eop run */
    }
    else
    {
      run_split( editor, cursor );
      insert_before = cursor->run;
    }
  }

  add_undo_delete_run( editor, insert_before->para->nCharOfs + insert_before->nCharOfs, len );

  run = run_create( style, flags );
  run->nCharOfs = insert_before->nCharOfs;
  run->len = len;
  run->para = insert_before->para;
  ME_InsertString( run->para->text, run->nCharOfs, str, len );
  ME_InsertBefore( run_get_di( insert_before ), run_get_di( run ) );
  TRACE("Shift length:%d\n", len);
  editor_propagate_char_ofs( editor, NULL, insert_before, len );
  para_mark_rewrap( editor, insert_before->para );

  /* Move any cursors that were at the end of the previous run to the end of the inserted run */
  prev = run_prev_all_paras( run );
  if (prev)
  {
    int i;

    for (i = 0; i < editor->nCursors; i++)
    {
      if (editor->pCursors[i].run == prev &&
          editor->pCursors[i].nOffset == prev->len)
      {
        editor->pCursors[i].run = run;
        editor->pCursors[i].nOffset = len;
      }
    }
  }

  return run;
}

static BOOL run_is_splittable( const ME_Run *run )
{
    WCHAR *str = get_text( run, 0 ), *p;
    int i;
    BOOL found_ink = FALSE;

    for (i = 0, p = str; i < run->len; i++, p++)
    {
        if (ME_IsWSpace( *p ))
        {
            if (found_ink) return TRUE;
        }
        else
            found_ink = TRUE;
    }
    return FALSE;
}

static BOOL run_is_entirely_ws( const ME_Run *run )
{
    WCHAR *str = get_text( run, 0 ), *p;
    int i;

    for (i = 0, p = str; i < run->len; i++, p++)
        if (!ME_IsWSpace( *p )) return FALSE;

    return TRUE;
}

/******************************************************************************
 * ME_UpdateRunFlags
 *
 * Determine some of run attributes given its content (style, text content).
 * Some flags cannot be determined by this function (MERF_GRAPHICS,
 * MERF_ENDPARA)
 */
void ME_UpdateRunFlags(ME_TextEditor *editor, ME_Run *run)
{
  assert(run->nCharOfs >= 0);

  if (RUN_IS_HIDDEN(run) || run->nFlags & MERF_TABLESTART)
    run->nFlags |= MERF_HIDDEN;
  else
    run->nFlags &= ~MERF_HIDDEN;

  if (run_is_splittable( run ))
    run->nFlags |= MERF_SPLITTABLE;
  else
    run->nFlags &= ~MERF_SPLITTABLE;

  if (!(run->nFlags & MERF_NOTEXT))
  {
    if (run_is_entirely_ws( run ))
      run->nFlags |= MERF_WHITESPACE | MERF_STARTWHITE | MERF_ENDWHITE;
    else
    {
      run->nFlags &= ~MERF_WHITESPACE;

      if (ME_IsWSpace( *get_text( run, 0 ) ))
        run->nFlags |= MERF_STARTWHITE;
      else
        run->nFlags &= ~MERF_STARTWHITE;

      if (ME_IsWSpace( *get_text( run, run->len - 1 ) ))
        run->nFlags |= MERF_ENDWHITE;
      else
        run->nFlags &= ~MERF_ENDWHITE;
    }
  }
  else
    run->nFlags &= ~(MERF_WHITESPACE | MERF_STARTWHITE | MERF_ENDWHITE);
}

/******************************************************************************
 * ME_CharFromPointContext
 *
 * Returns a character position inside the run given a run-relative
 * pixel horizontal position.
 *
 * If closest is FALSE return the actual character
 * If closest is TRUE will round to the closest leading edge.
 * ie. if the second character is at pixel position 8 and third at 16 then for:
 * closest = FALSE cx = 0..7 return 0, cx = 8..15 return 1
 * closest = TRUE  cx = 0..3 return 0, cx = 4..11 return 1.
 */
int ME_CharFromPointContext(ME_Context *c, int cx, ME_Run *run, BOOL closest, BOOL visual_order)
{
  ME_String *mask_text = NULL;
  WCHAR *str;
  int fit = 0;
  SIZE sz, sz2, sz3;
  if (!run->len || cx <= 0)
    return 0;

  if (run->nFlags & (MERF_TAB | MERF_ENDCELL))
  {
    if (!closest || cx < run->nWidth / 2) return 0;
    return 1;
  }

  if (run->nFlags & MERF_GRAPHICS)
  {
    SIZE sz;
    ME_GetOLEObjectSize(c, run, &sz);
    if (!closest || cx < sz.cx / 2) return 0;
    return 1;
  }

  if (run->para->nFlags & MEPF_COMPLEX)
  {
      int cp, trailing;
      if (visual_order && run->script_analysis.fRTL) cx = run->nWidth - cx - 1;

      ScriptXtoCP( cx, run->len, run->num_glyphs, run->clusters, run->vis_attrs, run->advances, &run->script_analysis,
                   &cp, &trailing );
      TRACE("x %d cp %d trailing %d (run width %d) rtl %d log order %d\n", cx, cp, trailing, run->nWidth,
            run->script_analysis.fRTL, run->script_analysis.fLogicalOrder);
      return closest ? cp + trailing : cp;
  }

  if (c->editor->password_char)
  {
    mask_text = ME_MakeStringR( c->editor->password_char, run->len );
    str = mask_text->szData;
  }
  else
    str = get_text( run, 0 );

  select_style(c, run->style);
  GetTextExtentExPointW(c->hDC, str, run->len,
                        cx, &fit, NULL, &sz);
  if (closest && fit != run->len)
  {
    GetTextExtentPoint32W(c->hDC, str, fit, &sz2);
    GetTextExtentPoint32W(c->hDC, str, fit + 1, &sz3);
    if (cx >= (sz2.cx+sz3.cx)/2)
      fit = fit + 1;
  }

  ME_DestroyString( mask_text );

  return fit;
}

int ME_CharFromPoint(ME_TextEditor *editor, int cx, ME_Run *run, BOOL closest, BOOL visual_order)
{
    ME_Context c;
    int ret;
    HDC hdc = ITextHost_TxGetDC( editor->texthost );

    ME_InitContext( &c, editor, hdc );
    ret = ME_CharFromPointContext( &c, cx, run, closest, visual_order );
    ME_DestroyContext(&c);
    ITextHost_TxReleaseDC( editor->texthost, hdc );
    return ret;
}

/******************************************************************************
 * ME_GetTextExtent
 *
 * Finds a width and a height of the text using a specified style
 */
static void ME_GetTextExtent(ME_Context *c, LPCWSTR szText, int nChars, ME_Style *s, SIZE *size)
{
    if (c->hDC)
    {
        select_style( c, s );
        GetTextExtentPoint32W( c->hDC, szText, nChars, size );
    }
    else
    {
        size->cx = 0;
        size->cy = 0;
    }
}

/******************************************************************************
 * ME_PointFromCharContext
 *
 * Returns a run-relative pixel position given a run-relative character
 * position (character offset)
 */
int ME_PointFromCharContext(ME_Context *c, ME_Run *pRun, int nOffset, BOOL visual_order)
{
  SIZE size;
  ME_String *mask_text = NULL;
  WCHAR *str;

  if (pRun->nFlags & MERF_GRAPHICS)
  {
    if (nOffset)
      ME_GetOLEObjectSize(c, pRun, &size);
    return nOffset != 0;
  } else if (pRun->nFlags & MERF_ENDPARA) {
    nOffset = 0;
  }

  if (pRun->para->nFlags & MEPF_COMPLEX)
  {
      int x;
      ScriptCPtoX( nOffset, FALSE, pRun->len, pRun->num_glyphs, pRun->clusters,
                   pRun->vis_attrs, pRun->advances, &pRun->script_analysis, &x );
      if (visual_order && pRun->script_analysis.fRTL) x = pRun->nWidth - x - 1;
      return x;
  }
  if (c->editor->password_char)
  {
    mask_text = ME_MakeStringR( c->editor->password_char, pRun->len );
    str = mask_text->szData;
  }
  else
      str = get_text( pRun, 0 );

  ME_GetTextExtent(c, str, nOffset, pRun->style, &size);
  ME_DestroyString( mask_text );
  return size.cx;
}

/******************************************************************************
 * ME_PointFromChar
 *
 * Calls ME_PointFromCharContext after first creating a context.
 */
int ME_PointFromChar(ME_TextEditor *editor, ME_Run *pRun, int nOffset, BOOL visual_order)
{
    ME_Context c;
    int ret;
    HDC hdc = ITextHost_TxGetDC( editor->texthost );

    ME_InitContext( &c, editor, hdc );
    ret = ME_PointFromCharContext( &c, pRun, nOffset, visual_order );
    ME_DestroyContext(&c);
    ITextHost_TxReleaseDC( editor->texthost, hdc );

    return ret;
}

/******************************************************************************
 * ME_GetRunSizeCommon
 * 
 * Finds width, height, ascent and descent of a run, up to given character
 * (nLen).
 */
SIZE ME_GetRunSizeCommon(ME_Context *c, const ME_Paragraph *para, ME_Run *run, int nLen,
                         int startx, int *pAscent, int *pDescent)
{
  SIZE size;

  nLen = min( nLen, run->len );

  if (run->nFlags & MERF_ENDPARA)
  {
      nLen = min( nLen, 1 );
      ME_GetTextExtent( c, L" ", nLen, run->style, &size );
  }
  else if (para->nFlags & MEPF_COMPLEX)
  {
      size.cx = run->nWidth;
  }
  else if (c->editor->password_char)
  {
    ME_String *szMasked = ME_MakeStringR( c->editor->password_char, nLen );
    ME_GetTextExtent(c, szMasked->szData, nLen,run->style, &size); 
    ME_DestroyString(szMasked);
  }
  else
  {
    ME_GetTextExtent(c, get_text( run, 0 ), nLen, run->style, &size);
  }
  *pAscent = run->style->tm.tmAscent;
  *pDescent = run->style->tm.tmDescent;
  size.cy = *pAscent + *pDescent;

  if (run->nFlags & MERF_TAB)
  {
    int pos = 0, i = 0, ppos, shift = 0;
    const PARAFORMAT2 *pFmt = &para->fmt;

    if (c->editor->bEmulateVersion10 && /* v1.0 - 3.0 */
        pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE)
      /* The horizontal gap shifts the tab positions to leave the gap. */
      shift = pFmt->dxOffset * 2;
    do {
      if (i < pFmt->cTabCount)
      {
        /* Only one side of the horizontal gap is needed at the end of
         * the table row. */
        if (i == pFmt->cTabCount -1)
          shift = shift >> 1;
        pos = shift + (pFmt->rgxTabs[i]&0x00FFFFFF);
        i++;
      }
      else
      {
        pos += lDefaultTab - (pos % lDefaultTab);
      }
      ppos = ME_twips2pointsX(c, pos);
      if (ppos > startx + run->pt.x) {
        size.cx = ppos - startx - run->pt.x;
        break;
      }
    } while(1);
    size.cy = *pAscent + *pDescent;
    return size;
  }
  if (run->nFlags & MERF_GRAPHICS)
  {
    ME_GetOLEObjectSize(c, run, &size);
    if (size.cy > *pAscent)
      *pAscent = size.cy;
    /* descent is unchanged */
    return size;
  }
  return size;
}

/******************************************************************************
 * ME_SetSelectionCharFormat
 *
 * Applies a style change, either to a current selection, or to insert cursor
 * (ie. the style next typed characters will use).
 */
void ME_SetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt)
{
  if (!ME_IsSelection(editor))
  {
    ME_Style *s;
    if (!editor->pBuffer->pCharStyle)
      editor->pBuffer->pCharStyle = style_get_insert_style( editor, editor->pCursors );
    s = ME_ApplyStyle(editor, editor->pBuffer->pCharStyle, pFmt);
    ME_ReleaseStyle(editor->pBuffer->pCharStyle);
    editor->pBuffer->pCharStyle = s;
  } else {
    ME_Cursor *from, *to;
    ME_GetSelection(editor, &from, &to);
    ME_SetCharFormat(editor, from, to, pFmt);
  }
}

/******************************************************************************
 * ME_SetCharFormat
 *
 * Applies a style change to the specified part of the text
 *
 * The start and end cursors specify the part of the text.  These cursors will
 * be updated to stay valid, but this function may invalidate other
 * non-selection cursors. The end cursor may be NULL to specify all the text
 * following the start cursor.
 *
 * If no text is selected, then nothing is done.
 */
void ME_SetCharFormat( ME_TextEditor *editor, ME_Cursor *start, ME_Cursor *end, CHARFORMAT2W *fmt )
{
  ME_Run *run, *start_run = start->run, *end_run = NULL;

  if (end && start->run == end->run && start->nOffset == end->nOffset)
    return;

  if (start->nOffset == start->run->len)
    start_run = run_next_all_paras( start->run );
  else if (start->nOffset)
  {
    /* run_split() may or may not update the cursors, depending on whether they
     * are selection cursors, but we need to make sure they are valid. */
    int split_offset = start->nOffset;
    ME_Run *split_run = run_split( editor, start );
    start_run = start->run;
    if (end && end->run == split_run)
    {
      end->run = start->run;
      end->nOffset -= split_offset;
    }
  }

  if (end)
  {
    if (end->nOffset == end->run->len)
      end_run = run_next_all_paras( end->run );
    else
    {
      if (end->nOffset) run_split( editor, end );
      end_run = end->run;
    }
  }

  for (run = start_run; run != end_run; run = run_next_all_paras( run ))
  {
    ME_Style *new_style = ME_ApplyStyle( editor, run->style, fmt );
    ME_Paragraph *para = run->para;

    add_undo_set_char_fmt( editor, para->nCharOfs + run->nCharOfs,
                           run->len, &run->style->fmt );
    ME_ReleaseStyle( run->style );
    run->style = new_style;

    /* The para numbering style depends on the eop style */
    if ((run->nFlags & MERF_ENDPARA) && para->para_num.style)
    {
      ME_ReleaseStyle(para->para_num.style);
      para->para_num.style = NULL;
    }
    para_mark_rewrap( editor, para );
  }
}

static void run_copy_char_fmt( ME_Run *run, CHARFORMAT2W *fmt )
{
    ME_CopyCharFormat( fmt, &run->style->fmt );
}

/******************************************************************************
 * ME_GetDefaultCharFormat
 * 
 * Retrieves the current default character style (the one applied where no
 * other style was applied) .
 */     
void ME_GetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt)
{
  ME_CopyCharFormat(pFmt, &editor->pBuffer->pDefaultStyle->fmt);
}

/******************************************************************************
 * ME_GetSelectionCharFormat
 *
 * If selection exists, it returns all style elements that are set consistently
 * in the whole selection. If not, it just returns the current style.
 */
void ME_GetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt)
{
  ME_Cursor *from, *to;
  if (!ME_IsSelection(editor) && editor->pBuffer->pCharStyle)
  {
    ME_CopyCharFormat(pFmt, &editor->pBuffer->pCharStyle->fmt);
    return;
  }
  ME_GetSelection(editor, &from, &to);
  ME_GetCharFormat(editor, from, to, pFmt);
}

/******************************************************************************
 * ME_GetCharFormat
 *
 * Returns the style consisting of those attributes which are consistently set
 * in the whole character range.
 */
void ME_GetCharFormat( ME_TextEditor *editor, const ME_Cursor *from,
                       const ME_Cursor *to, CHARFORMAT2W *fmt )
{
  ME_Run *run, *run_end, *prev_run;
  CHARFORMAT2W tmp;

  run = from->run;
  /* special case - if selection is empty, take previous char's formatting */
  if (from->run == to->run && from->nOffset == to->nOffset)
  {
    if (!from->nOffset && (prev_run = run_prev( run ))) run = prev_run;
    run_copy_char_fmt( run, fmt );
    return;
  }

  run_end = to->run;
  if (!to->nOffset) run_end = run_prev_all_paras( run_end );

  run_copy_char_fmt( run, fmt );

  if (run == run_end) return;

  do {
    /* FIXME add more style feature comparisons */
    DWORD dwAttribs = CFM_SIZE | CFM_FACE | CFM_COLOR | CFM_UNDERLINETYPE;
    DWORD dwEffects = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_PROTECTED | CFM_LINK | CFM_SUPERSCRIPT;

    run = run_next_all_paras( run );

    memset( &tmp, 0, sizeof(tmp) );
    tmp.cbSize = sizeof(tmp);
    run_copy_char_fmt( run, &tmp );

    assert((tmp.dwMask & dwAttribs) == dwAttribs);

    /* reset flags that differ */
    if (fmt->dwMask & CFM_FACE)
    {
      if (!(tmp.dwMask & CFM_FACE))
        fmt->dwMask &= ~CFM_FACE;
      else if (wcscmp( fmt->szFaceName, tmp.szFaceName ) ||
               fmt->bPitchAndFamily != tmp.bPitchAndFamily)
        fmt->dwMask &= ~CFM_FACE;
    }
    if (fmt->yHeight != tmp.yHeight) fmt->dwMask &= ~CFM_SIZE;
    if (fmt->bUnderlineType != tmp.bUnderlineType) fmt->dwMask &= ~CFM_UNDERLINETYPE;
    if (fmt->dwMask & CFM_COLOR)
    {
      if (!((fmt->dwEffects&CFE_AUTOCOLOR) & (tmp.dwEffects&CFE_AUTOCOLOR)))
      {
        if (fmt->crTextColor != tmp.crTextColor)
          fmt->dwMask &= ~CFM_COLOR;
      }
    }

    fmt->dwMask &= ~((fmt->dwEffects ^ tmp.dwEffects) & dwEffects);
    fmt->dwEffects = tmp.dwEffects;

  } while(run != run_end);
}
