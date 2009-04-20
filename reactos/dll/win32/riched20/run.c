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
 * Returns 1 if two runs can be safely merged into one, 0 otherwise.
 */ 
int ME_CanJoinRuns(const ME_Run *run1, const ME_Run *run2)
{
  if ((run1->nFlags | run2->nFlags) & MERF_NOJOIN)
    return 0;
  if (run1->style != run2->style)
    return 0;
  if ((run1->nFlags & MERF_STYLEFLAGS) != (run2->nFlags & MERF_STYLEFLAGS))
    return 0;
  return 1;
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
    TRACE("PropagateCharOffset(%s, %d)\n", debugstr_w(p->member.run.strText->szData), shift);
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
        TRACE_(richedit_check)("run, real ofs = %d (+ofsp = %d), counted = %d, len = %d, txt = \"%s\", flags=%08x, fx&mask = %08x\n",
          p->member.run.nCharOfs, p->member.run.nCharOfs+ofsp, ofsp+ofs,
          p->member.run.strText->nLen, debugstr_w(p->member.run.strText->szData),
          p->member.run.nFlags,
          p->member.run.style->fmt.dwMask & p->member.run.style->fmt.dwEffects);
        assert(ofs == p->member.run.nCharOfs);
        assert(p->member.run.strText->nLen);
        ofs += p->member.run.strText->nLen;
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

  nCharOfs = max(nCharOfs, 0);
  nCharOfs = min(nCharOfs, ME_GetTextLength(editor));

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
  if (pOfs) *pOfs = nCharOfs;
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
      editor->pCursors[i].nOffset += p->member.run.strText->nLen;
    }
  }

  ME_AppendString(p->member.run.strText, pNext->member.run.strText);
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
 * ME_SplitRun
 *
 * Splits a run into two in a given place. It also updates the screen position
 * and size (extent) of the newly generated runs.
 */
ME_DisplayItem *ME_SplitRun(ME_WrapContext *wc, ME_DisplayItem *item, int nVChar)
{
  ME_TextEditor *editor = wc->context->editor;
  ME_DisplayItem *item2 = NULL;
  ME_Run *run, *run2;
  ME_Paragraph *para = &ME_GetParagraph(item)->member.para;

  assert(item->member.run.nCharOfs != -1);
  if(TRACE_ON(richedit))
  {
    TRACE("Before check before split\n");
    ME_CheckCharOffsets(editor);
    TRACE("After check before split\n");
  }

  run = &item->member.run;

  TRACE("Before split: %s(%d, %d)\n", debugstr_w(run->strText->szData),
        run->pt.x, run->pt.y);

  item2 = ME_SplitRunSimple(editor, item, nVChar);

  run2 = &item2->member.run;

  ME_CalcRunExtent(wc->context, para, wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, run);
  ME_CalcRunExtent(wc->context, para, wc->nRow ? wc->nLeftMargin : wc->nFirstMargin, run2);

  run2->pt.x = run->pt.x+run->nWidth;
  run2->pt.y = run->pt.y;

  if(TRACE_ON(richedit))
  {
    TRACE("Before check after split\n");
    ME_CheckCharOffsets(editor);
    TRACE("After check after split\n");
    TRACE("After split: %s(%d, %d), %s(%d, %d)\n",
      debugstr_w(run->strText->szData), run->pt.x, run->pt.y,
      debugstr_w(run2->strText->szData), run2->pt.x, run2->pt.y);
  }

  return item2;
}

/******************************************************************************
 * ME_SplitRunSimple
 * 
 * Does the most basic job of splitting a run into two - it does not
 * update the positions and extents.    
 */    
ME_DisplayItem *ME_SplitRunSimple(ME_TextEditor *editor, ME_DisplayItem *item, int nVChar)
{
  ME_Run *run = &item->member.run;
  ME_DisplayItem *item2;
  ME_Run *run2;
  int i;
  assert(nVChar > 0 && nVChar < run->strText->nLen);
  assert(item->type == diRun);
  assert(!(item->member.run.nFlags & MERF_NONTEXT));
  assert(item->member.run.nCharOfs != -1);

  item2 = ME_MakeRun(run->style,
      ME_VSplitString(run->strText, nVChar), run->nFlags&MERF_SPLITMASK);

  item2->member.run.nCharOfs = item->member.run.nCharOfs + nVChar;

  run2 = &item2->member.run;
  ME_InsertBefore(item->next, item2);

  ME_UpdateRunFlags(editor, run);
  ME_UpdateRunFlags(editor, run2);
  for (i=0; i<editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == item &&
        editor->pCursors[i].nOffset >= nVChar) {
      assert(item2->type == diRun);
      editor->pCursors[i].pRun = item2;
      editor->pCursors[i].nOffset -= nVChar;
    }
  }
  ME_GetParagraph(item)->member.para.nFlags |= MEPF_REWRAP;
  return item2;
}

/******************************************************************************
 * ME_MakeRun
 * 
 * A helper function to create run structures quickly.
 */   
ME_DisplayItem *ME_MakeRun(ME_Style *s, ME_String *strData, int nFlags)
{
  ME_DisplayItem *item = ME_MakeDI(diRun);
  item->member.run.style = s;
  item->member.run.ole_obj = NULL;
  item->member.run.strText = strData;
  item->member.run.nFlags = nFlags;
  item->member.run.nCharOfs = -1;
  ME_AddRefStyle(s);
  return item;
}

/******************************************************************************
 * ME_InsertRun
 * 
 * Inserts a run at a given character position (offset).
 */   
ME_DisplayItem *ME_InsertRun(ME_TextEditor *editor, int nCharOfs, ME_DisplayItem *pItem)
{
  ME_Cursor tmp;
  ME_DisplayItem *pDI;

  assert(pItem->type == diRun || pItem->type == diUndoInsertRun);

  ME_CursorFromCharOfs(editor, nCharOfs, &tmp);
  pDI = ME_InsertRunAtCursor(editor, &tmp, pItem->member.run.style,
                             pItem->member.run.strText->szData,
                             pItem->member.run.strText->nLen,
                             pItem->member.run.nFlags);
  
  return pDI;
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
  ME_DisplayItem *pDI;
  ME_UndoItem *pUI;

  if (cursor->nOffset) {
  	/* We're inserting at the middle of the existing run, which means that
		 * that run must be split. It isn't always necessary, but */
    cursor->pRun = ME_SplitRunSimple(editor, cursor->pRun, cursor->nOffset);
    cursor->nOffset = 0;
  }

  pUI = ME_AddUndoItem(editor, diUndoDeleteRun, NULL);
  if (pUI) {
    pUI->nStart = cursor->pPara->member.para.nCharOfs
                  + cursor->pRun->member.run.nCharOfs;
    pUI->nLen = len;
  }

  pDI = ME_MakeRun(style, ME_MakeStringN(str, len), flags);
  pDI->member.run.nCharOfs = cursor->pRun->member.run.nCharOfs;
  ME_InsertBefore(cursor->pRun, pDI);
  TRACE("Shift length:%d\n", len);
  ME_PropagateCharOffset(cursor->pRun, len);
  cursor->pPara->member.para.nFlags |= MEPF_REWRAP;
  return pDI;
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
  ME_String *strText = run->strText;
  assert(run->nCharOfs >= 0);

  if (RUN_IS_HIDDEN(run) || run->nFlags & MERF_TABLESTART)
    run->nFlags |= MERF_HIDDEN;
  else
    run->nFlags &= ~MERF_HIDDEN;

  if (ME_IsSplitable(strText))
    run->nFlags |= MERF_SPLITTABLE;
  else
    run->nFlags &= ~MERF_SPLITTABLE;

  if (!(run->nFlags & MERF_NOTEXT)) {
    if (ME_IsWhitespaces(strText))
      run->nFlags |= MERF_WHITESPACE | MERF_STARTWHITE | MERF_ENDWHITE;
    else
    {
      run->nFlags &= ~MERF_WHITESPACE;

      if (ME_IsWSpace(strText->szData[0]))
        run->nFlags |= MERF_STARTWHITE;
      else
        run->nFlags &= ~MERF_STARTWHITE;

      if (ME_IsWSpace(strText->szData[strText->nLen - 1]))
        run->nFlags |= MERF_ENDWHITE;
      else
        run->nFlags &= ~MERF_ENDWHITE;
    }
  }
  else
    run->nFlags &= ~(MERF_WHITESPACE | MERF_STARTWHITE | MERF_ENDWHITE);
}

/******************************************************************************
 * ME_CharFromPoint
 * 
 * Returns a character position inside the run given a run-relative
 * pixel horizontal position. This version rounds left (ie. if the second
 * character is at pixel position 8, then for cx=0..7 it returns 0).  
 */     
int ME_CharFromPoint(ME_Context *c, int cx, ME_Run *run)
{
  int fit = 0;
  HGDIOBJ hOldFont;
  SIZE sz;
  if (!run->strText->nLen)
    return 0;

  if (run->nFlags & MERF_TAB ||
      (run->nFlags & (MERF_ENDCELL|MERF_ENDPARA)) == MERF_ENDCELL)
  {
    if (cx < run->nWidth/2) 
      return 0;
    return 1;
  }
  if (run->nFlags & MERF_GRAPHICS)
  {
    SIZE sz;
    ME_GetOLEObjectSize(c, run, &sz);
    if (cx < sz.cx)
      return 0;
    return 1;
  }
  hOldFont = ME_SelectStyleFont(c, run->style);
  
  if (c->editor->cPasswordMask)
  {
    ME_String *strMasked = ME_MakeStringR(c->editor->cPasswordMask, run->strText->nLen);
    GetTextExtentExPointW(c->hDC, strMasked->szData, run->strText->nLen,
      cx, &fit, NULL, &sz);
    ME_DestroyString(strMasked);
  }
  else
  {
    GetTextExtentExPointW(c->hDC, run->strText->szData, run->strText->nLen,
      cx, &fit, NULL, &sz);
  }
  
  ME_UnselectStyleFont(c, run->style, hOldFont);

  return fit;
}

/******************************************************************************
 * ME_CharFromPointCursor
 *
 * Returns a character position inside the run given a run-relative
 * pixel horizontal position. This version rounds to the nearest character edge
 * (ie. if the second character is at pixel position 8, then for cx=0..3
 * it returns 0, and for cx=4..7 it returns 1).
 *
 * It is used for mouse click handling, for better usability (and compatibility
 * with the native control).
 */
int ME_CharFromPointCursor(ME_TextEditor *editor, int cx, ME_Run *run)
{
  ME_String *strRunText;
  /* This could point to either the run's real text, or it's masked form in a password control */

  int fit = 0;
  ME_Context c;
  HGDIOBJ hOldFont;
  SIZE sz, sz2, sz3;
  if (!run->strText->nLen)
    return 0;

  if (run->nFlags & (MERF_TAB | MERF_ENDCELL))
  {
    if (cx < run->nWidth/2)
      return 0;
    return 1;
  }
  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));
  if (run->nFlags & MERF_GRAPHICS)
  {
    SIZE sz;
    ME_GetOLEObjectSize(&c, run, &sz);
    ME_DestroyContext(&c);
    if (cx < sz.cx/2)
      return 0;
    return 1;
  }

  if (editor->cPasswordMask)
    strRunText = ME_MakeStringR(editor->cPasswordMask, run->strText->nLen);
  else
    strRunText = run->strText;

  hOldFont = ME_SelectStyleFont(&c, run->style);
  GetTextExtentExPointW(c.hDC, strRunText->szData, strRunText->nLen,
                        cx, &fit, NULL, &sz);
  if (fit != strRunText->nLen)
  {
    GetTextExtentPoint32W(c.hDC, strRunText->szData, fit, &sz2);
    GetTextExtentPoint32W(c.hDC, strRunText->szData, fit + 1, &sz3);
    if (cx >= (sz2.cx+sz3.cx)/2)
      fit = fit + 1;
  }

  if (editor->cPasswordMask)
    ME_DestroyString(strRunText);

  ME_UnselectStyleFont(&c, run->style, hOldFont);
  ME_DestroyContext(&c);
  return fit;
}

/******************************************************************************
 * ME_GetTextExtent
 *
 * Finds a width and a height of the text using a specified style
 */
static void ME_GetTextExtent(ME_Context *c, LPCWSTR szText, int nChars, ME_Style *s, SIZE *size)
{
  HGDIOBJ hOldFont;
  hOldFont = ME_SelectStyleFont(c, s);
  GetTextExtentPoint32W(c->hDC, szText, nChars, size);
  ME_UnselectStyleFont(c, s, hOldFont);
}

/******************************************************************************
 * ME_PointFromChar
 *
 * Returns a run-relative pixel position given a run-relative character
 * position (character offset)
 */
int ME_PointFromChar(ME_TextEditor *editor, ME_Run *pRun, int nOffset)
{
  SIZE size;
  ME_Context c;
  ME_String *strRunText;
  /* This could point to either the run's real text, or it's masked form in a password control */

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));
  if (pRun->nFlags & MERF_GRAPHICS)
  {
    if (nOffset)
      ME_GetOLEObjectSize(&c, pRun, &size);
    ME_DestroyContext(&c);
    return nOffset != 0;
  } else if (pRun->nFlags & MERF_ENDPARA) {
    nOffset = 0;
  }

  if (editor->cPasswordMask)
    strRunText = ME_MakeStringR(editor->cPasswordMask, pRun->strText->nLen);
  else
    strRunText = pRun->strText;

  ME_GetTextExtent(&c,  strRunText->szData, nOffset, pRun->style, &size);
  ME_DestroyContext(&c);
  if (editor->cPasswordMask)
    ME_DestroyString(strRunText);
  return size.cx;
}

/******************************************************************************
 * ME_GetRunSizeCommon
 * 
 * Finds width, height, ascent and descent of a run, up to given character
 * (nLen).
 */
static SIZE ME_GetRunSizeCommon(ME_Context *c, const ME_Paragraph *para, ME_Run *run, int nLen,
                                int startx, int *pAscent, int *pDescent)
{
  SIZE size;
  int nMaxLen = run->strText->nLen;

  if (nLen>nMaxLen)
    nLen = nMaxLen;

  /* FIXME the following call also ensures that TEXTMETRIC structure is filled
   * this is wasteful for MERF_NONTEXT runs, but that shouldn't matter
   * in practice
   */
  
  if (c->editor->cPasswordMask)
  {
    ME_String *szMasked = ME_MakeStringR(c->editor->cPasswordMask,nLen);
    ME_GetTextExtent(c, szMasked->szData, nLen,run->style, &size); 
    ME_DestroyString(szMasked);
  }
  else
  {
    ME_GetTextExtent(c, run->strText->szData, nLen, run->style, &size);
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
 * ME_GetRunSize
 * 
 * Finds width and height (but not ascent and descent) of a part of the run
 * up to given character.    
 */     
SIZE ME_GetRunSize(ME_Context *c, const ME_Paragraph *para,
                   ME_Run *run, int nLen, int startx)
{
  int asc, desc;
  return ME_GetRunSizeCommon(c, para, run, nLen, startx, &asc, &desc);
}

/******************************************************************************
 * ME_CalcRunExtent
 * 
 * Updates the size of the run (fills width, ascent and descent). The height
 * is calculated based on whole row's ascent and descent anyway, so no need
 * to use it here.        
 */     
void ME_CalcRunExtent(ME_Context *c, const ME_Paragraph *para, int startx, ME_Run *run)
{
  if (run->nFlags & MERF_HIDDEN)
    run->nWidth = 0;
  else
  {
    int nEnd = run->strText->nLen;
    SIZE size = ME_GetRunSizeCommon(c, para, run, nEnd, startx,
                                    &run->nAscent, &run->nDescent);
    run->nWidth = size.cx;
    if (!size.cx)
      WARN("size.cx == 0\n");
  }
}

/******************************************************************************
 * ME_MustBeWrapped
 * 
 * This should ensure that the given paragraph is wrapped so that its screen
 * row structure may be used. But it doesn't, yet. 
 */     
void ME_MustBeWrapped(ME_Context *c, ME_DisplayItem *para)
{
  assert(para->type == diParagraph);
  /* FIXME */
}

/******************************************************************************
 * ME_SetSelectionCharFormat
 * 
 * Applies a style change, either to a current selection, or to insert cursor
 * (ie. the style next typed characters will use).
 */     
void ME_SetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt)
{
  int nFrom, nTo;
  ME_GetSelection(editor, &nFrom, &nTo);
  if (nFrom == nTo)
  {
    ME_Style *s;
    if (!editor->pBuffer->pCharStyle)
      editor->pBuffer->pCharStyle = ME_GetInsertStyle(editor, 0);
    s = ME_ApplyStyle(editor->pBuffer->pCharStyle, pFmt);
    ME_ReleaseStyle(editor->pBuffer->pCharStyle);
    editor->pBuffer->pCharStyle = s;
  }
  else
    ME_SetCharFormat(editor, nFrom, nTo-nFrom, pFmt);
}

/******************************************************************************
 * ME_SetCharFormat
 * 
 * Applies a style change to the specified part of the text
 */     
void ME_SetCharFormat(ME_TextEditor *editor, int nOfs, int nChars, CHARFORMAT2W *pFmt)
{
  ME_Cursor tmp, tmp2;
  ME_DisplayItem *para;

  ME_CursorFromCharOfs(editor, nOfs, &tmp);
  if (tmp.nOffset)
    tmp.pRun = ME_SplitRunSimple(editor, tmp.pRun, tmp.nOffset);

  ME_CursorFromCharOfs(editor, nOfs+nChars, &tmp2);
  if (tmp2.nOffset)
    tmp2.pRun = ME_SplitRunSimple(editor, tmp2.pRun, tmp2.nOffset);

  para = tmp.pPara;
  para->member.para.nFlags |= MEPF_REWRAP;

  while(tmp.pRun != tmp2.pRun)
  {
    ME_UndoItem *undo = NULL;
    ME_Style *new_style = ME_ApplyStyle(tmp.pRun->member.run.style, pFmt);
    /* ME_DumpStyle(new_style); */
    undo = ME_AddUndoItem(editor, diUndoSetCharFormat, NULL);
    if (undo) {
      undo->nStart = tmp.pRun->member.run.nCharOfs+para->member.para.nCharOfs;
      undo->nLen = tmp.pRun->member.run.strText->nLen;
      undo->di.member.ustyle = tmp.pRun->member.run.style;
      /* we'd have to addref undo...ustyle and release tmp...style
         but they'd cancel each other out so we can do nothing instead */
    }
    else
      ME_ReleaseStyle(tmp.pRun->member.run.style);
    tmp.pRun->member.run.style = new_style;
    tmp.pRun = ME_FindItemFwd(tmp.pRun, diRunOrParagraph);
    if (tmp.pRun->type == diParagraph)
    {
      para = tmp.pRun;
      tmp.pRun = ME_FindItemFwd(tmp.pRun, diRun);
      if (tmp.pRun != tmp2.pRun)
        para->member.para.nFlags |= MEPF_REWRAP;
    }
    assert(tmp.pRun);
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
  int nFrom, nTo;
  ME_GetSelection(editor, &nFrom, &nTo);
  if (nFrom == nTo && editor->pBuffer->pCharStyle)
  {
    ME_CopyCharFormat(pFmt, &editor->pBuffer->pCharStyle->fmt);
    return;
  }
  ME_GetCharFormat(editor, nFrom, nTo, pFmt);
}

/******************************************************************************
 * ME_GetCharFormat
 *
 * Returns the style consisting of those attributes which are consistently set
 * in the whole character range.
 */
void ME_GetCharFormat(ME_TextEditor *editor, int nFrom, int nTo, CHARFORMAT2W *pFmt)
{
  ME_DisplayItem *run, *run_end;
  int nOffset, nOffset2;
  CHARFORMAT2W tmp;

  ME_RunOfsFromCharOfs(editor, nFrom, NULL, &run, &nOffset);
  if (nFrom == nTo) /* special case - if selection is empty, take previous char's formatting */
  {
    if (!nOffset)
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
  
  if (nTo>nFrom) /* selection consists of chars from nFrom up to nTo-1 */
    nTo--;
  ME_RunOfsFromCharOfs(editor, nTo, NULL, &run_end, &nOffset2);

  ME_GetRunCharFormat(editor, run, pFmt);

  if (run == run_end) return;

  do {
    /* FIXME add more style feature comparisons */
    int nAttribs = CFM_SIZE | CFM_FACE | CFM_COLOR | CFM_UNDERLINETYPE;
    int nEffects = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_PROTECTED | CFM_LINK | CFM_SUPERSCRIPT;

    run = ME_FindItemFwd(run, diRun);

    ZeroMemory(&tmp, sizeof(tmp));
    tmp.cbSize = sizeof(tmp);
    ME_GetRunCharFormat(editor, run, &tmp);

    assert((tmp.dwMask & nAttribs) == nAttribs);
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

    pFmt->dwMask &= ~((pFmt->dwEffects ^ tmp.dwEffects) & nEffects);
    pFmt->dwEffects = tmp.dwEffects;

  } while(run != run_end);
}
