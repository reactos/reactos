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

void ME_SkipAndPropagateCharOffset(ME_DisplayItem *p, int shift)
{
  p = ME_FindItemFwd(p, diRunOrParagraphOrEnd);
  assert(p);
  ME_PropagateCharOffset(p, shift);
}

/******************************************************************************
 * ME_PropagateCharOffsets
 *
 * Shifts (increases or decreases) character offset (relative to beginning of 
 * the document) of the part of the text starting from given place.  
 */ 
void ME_PropagateCharOffset(ME_DisplayItem *p, int shift)
{
	/* Runs in one paragraph contain character offset relative to their owning
	 * paragraph. If we start the shifting from the run, we need to shift
	 * all the relative offsets until the end of the paragraph
	 */	 	    
  if (p->type == diRun) /* propagate in all runs in this para */
  {
    TRACE("PropagateCharOffset(%s, %d)\n", debugstr_run( &p->member.run ), shift);
    do {
      p->member.run.nCharOfs += shift;
      assert(p->member.run.nCharOfs >= 0);
      p = ME_FindItemFwd(p, diRunOrParagraphOrEnd);
    } while(p->type == diRun);
  }
	/* Runs in next paragraphs don't need their offsets updated, because they, 
	 * again, those offsets are relative to their respective paragraphs.
	 * Instead of that, we're updating paragraphs' character offsets.	  
	 */	 	    
  if (p->type == diParagraph) /* propagate in all next paras */
  {
    do {
      p->member.para.nCharOfs += shift;
      assert(p->member.para.nCharOfs >= 0);
      p = p->member.para.next_para;
    } while(p->type == diParagraph);
  }
  /* diTextEnd also has character offset in it, which makes finding text length
   * easier. But it needs to be up to date first.
   */
  if (p->type == diTextEnd)
  {
    p->member.para.nCharOfs += shift;
    assert(p->member.para.nCharOfs >= 0);
  }
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
        return;
      case diParagraph:
        TRACE_(richedit_check)("para, real ofsp = %d, counted = %d\n", p->member.para.nCharOfs, ofsp+ofs);
        assert(ofsp+ofs == p->member.para.nCharOfs);
        ofsp = p->member.para.nCharOfs;
        ofs = 0;
        break;
      case diRun:
        TRACE_(richedit_check)("run, real ofs = %d (+ofsp = %d), counted = %d, len = %d, txt = %s, flags=%08x, fx&mask = %08x\n",
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
}

/******************************************************************************
 * ME_CharOfsFromRunOfs
 *
 * Converts a character position relative to the start of the run, to a
 * character position relative to the start of the document.
 * Kind of a "local to global" offset conversion.
 */
int ME_CharOfsFromRunOfs(ME_TextEditor *editor, const ME_DisplayItem *pPara,
                         const ME_DisplayItem *pRun, int nOfs)
{
  assert(pRun && pRun->type == diRun);
  assert(pPara && pPara->type == diParagraph);
  return pPara->member.para.nCharOfs + pRun->member.run.nCharOfs + nOfs;
}

/******************************************************************************
 * ME_CursorFromCharOfs
 *
 * Converts a character offset (relative to the start of the document) to
 * a cursor structure (which contains a run and a position relative to that
 * run).
 */
void ME_CursorFromCharOfs(ME_TextEditor *editor, int nCharOfs, ME_Cursor *pCursor)
{
  ME_RunOfsFromCharOfs(editor, nCharOfs, &pCursor->pPara,
                       &pCursor->pRun, &pCursor->nOffset);
}

/******************************************************************************
 * ME_RunOfsFromCharOfs
 *
 * Find a run and relative character offset given an absolute character offset
 * (absolute offset being an offset relative to the start of the document).
 * Kind of a "global to local" offset conversion.
 */
void ME_RunOfsFromCharOfs(ME_TextEditor *editor,
                          int nCharOfs,
                          ME_DisplayItem **ppPara,
                          ME_DisplayItem **ppRun,
                          int *pOfs)
{
  ME_DisplayItem *item, *next_item;
  int endOfs = nCharOfs, len = ME_GetTextLength(editor);

  nCharOfs = max(nCharOfs, 0);
  nCharOfs = min(nCharOfs, len);

  /* Find the paragraph at the offset. */
  next_item = editor->pBuffer->pFirst->member.para.next_para;
  do {
    item = next_item;
    next_item = item->member.para.next_para;
  } while (next_item->member.para.nCharOfs <= nCharOfs);
  assert(item->type == diParagraph);
  nCharOfs -= item->member.para.nCharOfs;
  if (ppPara) *ppPara = item;

  /* Find the run at the offset. */
  next_item = ME_FindItemFwd(item, diRun);
  do {
    item = next_item;
    next_item = ME_FindItemFwd(item, diRunOrParagraphOrEnd);
  } while (next_item->type == diRun &&
           next_item->member.run.nCharOfs <= nCharOfs);
  assert(item->type == diRun);
  nCharOfs -= item->member.run.nCharOfs;

  if (ppRun) *ppRun = item;
  if (pOfs) {
    if (((*ppRun)->member.run.nFlags & MERF_ENDPARA) && endOfs > len)
      *pOfs = (*ppRun)->member.run.len;
    else *pOfs = nCharOfs;
  }
}

/******************************************************************************
 * ME_JoinRuns
 * 
 * Merges two adjacent runs, the one given as a parameter and the next one.
 */    
void ME_JoinRuns(ME_TextEditor *editor, ME_DisplayItem *p)
{
  ME_DisplayItem *pNext = p->next;
  int i;
  assert(p->type == diRun && pNext->type == diRun);
  assert(p->member.run.nCharOfs != -1);
  ME_GetParagraph(p)->member.para.nFlags |= MEPF_REWRAP;

  /* Update all cursors so that they don't contain the soon deleted run */
  for (i=0; i<editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == pNext) {
      editor->pCursors[i].pRun = p;
      editor->pCursors[i].nOffset += p->member.run.len;
    }
  }

  p->member.run.len += pNext->member.run.len;
  ME_Remove(pNext);
  ME_DestroyDisplayItem(pNext);
  ME_UpdateRunFlags(editor, &p->member.run);
  if(TRACE_ON(richedit))
  {
    TRACE("Before check after join\n");
    ME_CheckCharOffsets(editor);
    TRACE("After check after join\n");
  }
}

/******************************************************************************
 * ME_SplitRunSimple
 *
 * Does the most basic job of splitting a run into two - it does not
 * update the positions and extents.
 */
ME_DisplayItem *ME_SplitRunSimple(ME_TextEditor *editor, ME_Cursor *cursor)
{
  ME_DisplayItem *run = cursor->pRun;
  ME_DisplayItem *new_run;
  int i;
  int nOffset = cursor->nOffset;

  assert(!(run->member.run.nFlags & MERF_NONTEXT));

  new_run = ME_MakeRun(run->member.run.style,
                       run->member.run.nFlags & MERF_SPLITMASK);
  new_run->member.run.nCharOfs = run->member.run.nCharOfs + nOffset;
  new_run->member.run.len = run->member.run.len - nOffset;
  new_run->member.run.para = run->member.run.para;
  run->member.run.len = nOffset;
  cursor->pRun = new_run;
  cursor->nOffset = 0;

  ME_InsertBefore(run->next, new_run);

  ME_UpdateRunFlags(editor, &run->member.run);
  ME_UpdateRunFlags(editor, &new_run->member.run);
  for (i = 0; i < editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == run &&
        editor->pCursors[i].nOffset >= nOffset) {
      editor->pCursors[i].pRun = new_run;
      editor->pCursors[i].nOffset -= nOffset;
    }
  }
  cursor->pPara->member.para.nFlags |= MEPF_REWRAP;
  return run;
}

/******************************************************************************
 * ME_MakeRun
 * 
 * A helper function to create run structures quickly.
 */   
ME_DisplayItem *ME_MakeRun(ME_Style *s, int nFlags)
{
  ME_DisplayItem *item = ME_MakeDI(diRun);
  item->member.run.style = s;
  item->member.run.ole_obj = NULL;
  item->member.run.nFlags = nFlags;
  item->member.run.nCharOfs = -1;
  item->member.run.len = 0;
  item->member.run.para = NULL;
  item->member.run.num_glyphs = 0;
  item->member.run.max_glyphs = 0;
  item->member.run.glyphs = NULL;
  item->member.run.vis_attrs = NULL;
  item->member.run.advances = NULL;
  item->member.run.offsets = NULL;
  item->member.run.max_clusters = 0;
  item->member.run.clusters = NULL;
  ME_AddRefStyle(s);
  return item;
}

/******************************************************************************
 * ME_InsertRunAtCursor
 *
 * Inserts a new run with given style, flags and content at a given position,
 * which is passed as a cursor structure (which consists of a run and 
 * a run-relative character offset).
 */
ME_DisplayItem *
ME_InsertRunAtCursor(ME_TextEditor *editor, ME_Cursor *cursor, ME_Style *style,
                     const WCHAR *str, int len, int flags)
{
  ME_DisplayItem *pDI, *insert_before = cursor->pRun, *prev;

  if (cursor->nOffset)
  {
    if (cursor->nOffset == cursor->pRun->member.run.len)
    {
      insert_before = ME_FindItemFwd( cursor->pRun, diRun );
      if (!insert_before) insert_before = cursor->pRun; /* Always insert before the final eop run */
    }
    else
    {
      ME_SplitRunSimple( editor, cursor );
      insert_before = cursor->pRun;
    }
  }

  add_undo_delete_run( editor, insert_before->member.run.para->nCharOfs +
                       insert_before->member.run.nCharOfs, len );

  pDI = ME_MakeRun(style, flags);
  pDI->member.run.nCharOfs = insert_before->member.run.nCharOfs;
  pDI->member.run.len = len;
  pDI->member.run.para = insert_before->member.run.para;
  ME_InsertString( pDI->member.run.para->text, pDI->member.run.nCharOfs, str, len );
  ME_InsertBefore( insert_before, pDI );
  TRACE("Shift length:%d\n", len);
  ME_PropagateCharOffset( insert_before, len );
  insert_before->member.run.para->nFlags |= MEPF_REWRAP;

  /* Move any cursors that were at the end of the previous run to the end of the inserted run */
  prev = ME_FindItemBack( pDI, diRun );
  if (prev)
  {
    int i;

    for (i = 0; i < editor->nCursors; i++)
    {
      if (editor->pCursors[i].pRun == prev &&
          editor->pCursors[i].nOffset == prev->member.run.len)
      {
        editor->pCursors[i].pRun = pDI;
        editor->pCursors[i].nOffset = len;
      }
    }
  }

  return pDI;
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
  HGDIOBJ hOldFont;
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

  if (c->editor->cPasswordMask)
  {
    mask_text = ME_MakeStringR( c->editor->cPasswordMask, run->len );
    str = mask_text->szData;
  }
  else
    str = get_text( run, 0 );

  hOldFont = ME_SelectStyleFont(c, run->style);
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

  ME_UnselectStyleFont(c, run->style, hOldFont);
  return fit;
}

int ME_CharFromPoint(ME_TextEditor *editor, int cx, ME_Run *run, BOOL closest, BOOL visual_order)
{
    ME_Context c;
    int ret;

    ME_InitContext( &c, editor, ITextHost_TxGetDC( editor->texthost ) );
    ret = ME_CharFromPointContext( &c, cx, run, closest, visual_order );
    ME_DestroyContext(&c);
    return ret;
}

/******************************************************************************
 * ME_GetTextExtent
 *
 * Finds a width and a height of the text using a specified style
 */
static void ME_GetTextExtent(ME_Context *c, LPCWSTR szText, int nChars, ME_Style *s, SIZE *size)
{
  HGDIOBJ hOldFont;
  if (c->hDC) {
    hOldFont = ME_SelectStyleFont(c, s);
    GetTextExtentPoint32W(c->hDC, szText, nChars, size);
    ME_UnselectStyleFont(c, s, hOldFont);
  } else {
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
  if (c->editor->cPasswordMask)
  {
    mask_text = ME_MakeStringR(c->editor->cPasswordMask, pRun->len);
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

    ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));
    ret = ME_PointFromCharContext( &c, pRun, nOffset, visual_order );
    ME_DestroyContext(&c);

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
  int nMaxLen = run->len;

  if (nLen>nMaxLen)
    nLen = nMaxLen;

  /* FIXME the following call also ensures that TEXTMETRIC structure is filled
   * this is wasteful for MERF_NONTEXT runs, but that shouldn't matter
   * in practice
   */

  if (para->nFlags & MEPF_COMPLEX)
  {
      size.cx = run->nWidth;
  }
  else if (c->editor->cPasswordMask)
  {
    ME_String *szMasked = ME_MakeStringR(c->editor->cPasswordMask,nLen);
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
    PARAFORMAT2 *pFmt = para->pFmt;

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
      editor->pBuffer->pCharStyle = ME_GetInsertStyle(editor, 0);
    s = ME_ApplyStyle(editor->pBuffer->pCharStyle, pFmt);
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
void ME_SetCharFormat(ME_TextEditor *editor, ME_Cursor *start, ME_Cursor *end, CHARFORMAT2W *pFmt)
{
  ME_DisplayItem *run, *start_run = start->pRun, *end_run = NULL;

  if (end && start->pRun == end->pRun && start->nOffset == end->nOffset)
    return;

  if (start->nOffset == start->pRun->member.run.len)
    start_run = ME_FindItemFwd( start->pRun, diRun );
  else if (start->nOffset)
  {
    /* SplitRunSimple may or may not update the cursors, depending on whether they
     * are selection cursors, but we need to make sure they are valid. */
    int split_offset = start->nOffset;
    ME_DisplayItem *split_run = ME_SplitRunSimple(editor, start);
    start_run = start->pRun;
    if (end && end->pRun == split_run)
    {
      end->pRun = start->pRun;
      end->nOffset -= split_offset;
    }
  }

  if (end)
  {
    if (end->nOffset == end->pRun->member.run.len)
      end_run = ME_FindItemFwd( end->pRun, diRun );
    else
    {
      if (end->nOffset) ME_SplitRunSimple(editor, end);
      end_run = end->pRun;
    }
  }

  for (run = start_run; run != end_run; run = ME_FindItemFwd( run, diRun ))
  {
    ME_Style *new_style = ME_ApplyStyle(run->member.run.style, pFmt);

    add_undo_set_char_fmt( editor, run->member.run.para->nCharOfs + run->member.run.nCharOfs,
                           run->member.run.len, &run->member.run.style->fmt );
    ME_ReleaseStyle(run->member.run.style);
    run->member.run.style = new_style;
    run->member.run.para->nFlags |= MEPF_REWRAP;
  }
}

/******************************************************************************
 * ME_SetDefaultCharFormat
 * 
 * Applies a style change to the default character style.
 */     
void ME_SetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *mod)
{
  ME_Style *style;

  assert(mod->cbSize == sizeof(CHARFORMAT2W));
  style = ME_ApplyStyle(editor->pBuffer->pDefaultStyle, mod);
  editor->pBuffer->pDefaultStyle->fmt = style->fmt;
  editor->pBuffer->pDefaultStyle->tm = style->tm;
  ScriptFreeCache( &editor->pBuffer->pDefaultStyle->script_cache );
  ME_ReleaseStyle(style);
  ME_MarkAllForWrapping(editor);
  /*  pcf = editor->pBuffer->pDefaultStyle->fmt; */
}

static void ME_GetRunCharFormat(ME_TextEditor *editor, ME_DisplayItem *run, CHARFORMAT2W *pFmt)
{
  ME_CopyCharFormat(pFmt, &run->member.run.style->fmt);
  if ((pFmt->dwMask & CFM_UNDERLINETYPE) && (pFmt->bUnderlineType == CFU_CF1UNDERLINE))
  {
    pFmt->dwMask |= CFM_UNDERLINE;
    pFmt->dwEffects |= CFE_UNDERLINE;
  }
  if ((pFmt->dwMask & CFM_UNDERLINETYPE) && (pFmt->bUnderlineType == CFU_UNDERLINENONE))
  {
    pFmt->dwMask |= CFM_UNDERLINE;
    pFmt->dwEffects &= ~CFE_UNDERLINE;
  }
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
void ME_GetCharFormat(ME_TextEditor *editor, const ME_Cursor *from,
                      const ME_Cursor *to, CHARFORMAT2W *pFmt)
{
  ME_DisplayItem *run, *run_end;
  CHARFORMAT2W tmp;

  run = from->pRun;
  /* special case - if selection is empty, take previous char's formatting */
  if (from->pRun == to->pRun && from->nOffset == to->nOffset)
  {
    if (!from->nOffset)
    {
      ME_DisplayItem *tmp_run = ME_FindItemBack(run, diRunOrParagraph);
      if (tmp_run->type == diRun) {
        ME_GetRunCharFormat(editor, tmp_run, pFmt);
        return;
      }
    }
    ME_GetRunCharFormat(editor, run, pFmt);
    return;
  }

  run_end = to->pRun;
  if (!to->nOffset)
    run_end = ME_FindItemBack(run_end, diRun);

  ME_GetRunCharFormat(editor, run, pFmt);

  if (run == run_end) return;

  do {
    /* FIXME add more style feature comparisons */
    DWORD dwAttribs = CFM_SIZE | CFM_FACE | CFM_COLOR | CFM_UNDERLINETYPE;
    DWORD dwEffects = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_PROTECTED | CFM_LINK | CFM_SUPERSCRIPT;

    run = ME_FindItemFwd(run, diRun);

    ZeroMemory(&tmp, sizeof(tmp));
    tmp.cbSize = sizeof(tmp);
    ME_GetRunCharFormat(editor, run, &tmp);

    assert((tmp.dwMask & dwAttribs) == dwAttribs);
    /* reset flags that differ */

    if (pFmt->yHeight != tmp.yHeight)
      pFmt->dwMask &= ~CFM_SIZE;
    if (pFmt->dwMask & CFM_FACE)
    {
      if (!(tmp.dwMask & CFM_FACE))
        pFmt->dwMask &= ~CFM_FACE;
      else if (lstrcmpW(pFmt->szFaceName, tmp.szFaceName) ||
          pFmt->bPitchAndFamily != tmp.bPitchAndFamily)
        pFmt->dwMask &= ~CFM_FACE;
    }
    if (pFmt->yHeight != tmp.yHeight)
      pFmt->dwMask &= ~CFM_SIZE;
    if (pFmt->bUnderlineType != tmp.bUnderlineType)
      pFmt->dwMask &= ~CFM_UNDERLINETYPE;
    if (pFmt->dwMask & CFM_COLOR)
    {
      if (!((pFmt->dwEffects&CFE_AUTOCOLOR) & (tmp.dwEffects&CFE_AUTOCOLOR)))
      {
        if (pFmt->crTextColor != tmp.crTextColor)
          pFmt->dwMask &= ~CFM_COLOR;
      }
    }

    pFmt->dwMask &= ~((pFmt->dwEffects ^ tmp.dwEffects) & dwEffects);
    pFmt->dwEffects = tmp.dwEffects;

  } while(run != run_end);
}
