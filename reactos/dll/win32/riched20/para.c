/*
 * RichEdit - functions working on paragraphs of text (diParagraph).
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

static const WCHAR wszParagraphSign[] = {0xB6, 0};

void ME_MakeFirstParagraph(HDC hDC, ME_TextBuffer *text)
{
  PARAFORMAT2 fmt;
  CHARFORMAT2W cf;
  LOGFONTW lf;
  HFONT hf;
  ME_DisplayItem *para = ME_MakeDI(diParagraph);
  ME_DisplayItem *run;
  ME_Style *style;

  hf = (HFONT)GetStockObject(SYSTEM_FONT);
  assert(hf);
  GetObjectW(hf, sizeof(LOGFONTW), &lf);
  ZeroMemory(&cf, sizeof(cf));
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_BACKCOLOR|CFM_COLOR|CFM_FACE|CFM_SIZE|CFM_CHARSET;
  cf.dwMask |= CFM_ALLCAPS|CFM_BOLD|CFM_DISABLED|CFM_EMBOSS|CFM_HIDDEN;
  cf.dwMask |= CFM_IMPRINT|CFM_ITALIC|CFM_LINK|CFM_OUTLINE|CFM_PROTECTED;
  cf.dwMask |= CFM_REVISED|CFM_SHADOW|CFM_SMALLCAPS|CFM_STRIKEOUT;
  cf.dwMask |= CFM_SUBSCRIPT|CFM_UNDERLINE;
  
  cf.dwEffects = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;
  lstrcpyW(cf.szFaceName, lf.lfFaceName);
  cf.yHeight=lf.lfHeight*1440/GetDeviceCaps(hDC, LOGPIXELSY);
  if (lf.lfWeight>=700) /* FIXME correct weight ? */
    cf.dwEffects |= CFE_BOLD;
  cf.wWeight = lf.lfWeight;
  if (lf.lfItalic) cf.dwEffects |= CFE_ITALIC;
  if (lf.lfUnderline) cf.dwEffects |= CFE_UNDERLINE;
  if (lf.lfStrikeOut) cf.dwEffects |= CFE_STRIKEOUT;
  
  ZeroMemory(&fmt, sizeof(fmt));
  fmt.cbSize = sizeof(fmt);
  fmt.dwMask = PFM_ALIGNMENT | PFM_OFFSET | PFM_STARTINDENT | PFM_RIGHTINDENT | PFM_TABSTOPS;

  CopyMemory(para->member.para.pFmt, &fmt, sizeof(PARAFORMAT2));
  
  style = ME_MakeStyle(&cf);
  text->pDefaultStyle = style;
  
  run = ME_MakeRun(style, ME_MakeString(wszParagraphSign), MERF_ENDPARA);
  run->member.run.nCharOfs = 0;

  ME_InsertBefore(text->pLast, para);
  ME_InsertBefore(text->pLast, run);
  para->member.para.prev_para = text->pFirst;
  para->member.para.next_para = text->pLast;
  text->pFirst->member.para.next_para = para;
  text->pLast->member.para.prev_para = para;

  text->pLast->member.para.nCharOfs = 1;
}
 
void ME_MarkAllForWrapping(ME_TextEditor *editor)
{
  ME_MarkForWrapping(editor, editor->pBuffer->pFirst->member.para.next_para, editor->pBuffer->pLast);
}

void ME_MarkForWrapping(ME_TextEditor *editor, ME_DisplayItem *first, ME_DisplayItem *last)
{
  while(first != last)
  {
    first->member.para.nFlags |= MEPF_REWRAP;
    first = first->member.para.next_para;
  }
}

void ME_MarkForPainting(ME_TextEditor *editor, ME_DisplayItem *first, ME_DisplayItem *last)
{
  while(first != last)
  {
    first->member.para.nFlags |= MEPF_REPAINT;
    first = first->member.para.next_para;
  }
}

/* split paragraph at the beginning of the run */
ME_DisplayItem *ME_SplitParagraph(ME_TextEditor *editor, ME_DisplayItem *run, ME_Style *style)
{
  ME_DisplayItem *next_para = NULL;
  ME_DisplayItem *run_para = NULL;
  ME_DisplayItem *new_para = ME_MakeDI(diParagraph);
  ME_DisplayItem *end_run = ME_MakeRun(style,ME_MakeString(wszParagraphSign), MERF_ENDPARA);
  ME_UndoItem *undo = NULL;
  int ofs;
  ME_DisplayItem *pp;
  int end_len = (editor->bEmulateVersion10 ? 2 : 1);
  
  assert(run->type == diRun);  

  run_para = ME_GetParagraph(run);
  assert(run_para->member.para.pFmt->cbSize == sizeof(PARAFORMAT2));

  ofs = end_run->member.run.nCharOfs = run->member.run.nCharOfs;
  next_para = run_para->member.para.next_para;
  assert(next_para == ME_FindItemFwd(run_para, diParagraphOrEnd));
  
  undo = ME_AddUndoItem(editor, diUndoJoinParagraphs, NULL);
  if (undo)
    undo->nStart = run_para->member.para.nCharOfs + ofs;
  
  /* the new paragraph will have a different starting offset, so let's update its runs */
  pp = run;
  while(pp->type == diRun) {
    pp->member.run.nCharOfs -= ofs;
    pp = ME_FindItemFwd(pp, diRunOrParagraphOrEnd);
  }
  new_para->member.para.nCharOfs = ME_GetParagraph(run)->member.para.nCharOfs+ofs;
  new_para->member.para.nCharOfs += end_len;
  
  new_para->member.para.nFlags = MEPF_REWRAP; /* FIXME copy flags (if applicable) */
  /* FIXME initialize format style and call ME_SetParaFormat blah blah */
  CopyMemory(new_para->member.para.pFmt, run_para->member.para.pFmt, sizeof(PARAFORMAT2));
  
  /* FIXME remove this as soon as nLeftMargin etc are replaced with proper fields of PARAFORMAT2 */
  new_para->member.para.nLeftMargin = run_para->member.para.nLeftMargin;
  new_para->member.para.nRightMargin = run_para->member.para.nRightMargin;
  new_para->member.para.nFirstMargin = run_para->member.para.nFirstMargin;

  new_para->member.para.bTable = run_para->member.para.bTable;
  
  /* Inherit previous cell definitions if any */
  new_para->member.para.pCells = NULL;
  if (run_para->member.para.pCells)
  {
    ME_TableCell *pCell, *pNewCell;

    for (pCell = run_para->member.para.pCells; pCell; pCell = pCell->next)
    {
      pNewCell = ALLOC_OBJ(ME_TableCell);
      pNewCell->nRightBoundary = pCell->nRightBoundary;
      pNewCell->next = NULL;
      if (new_para->member.para.pCells)
        new_para->member.para.pLastCell->next = pNewCell;
      else
        new_para->member.para.pCells = pNewCell;
      new_para->member.para.pLastCell = pNewCell;
    }
  }
    
  /* fix paragraph properties. FIXME only needed when called from RTF reader */
  if (run_para->member.para.pCells && !run_para->member.para.bTable)
  {
    /* Paragraph does not have an \intbl keyword, so any table definition
     * stored is invalid */
    ME_DestroyTableCellList(run_para);
  }
  
  /* insert paragraph into paragraph double linked list */
  new_para->member.para.prev_para = run_para;
  new_para->member.para.next_para = next_para;
  run_para->member.para.next_para = new_para;
  next_para->member.para.prev_para = new_para;

  /* insert end run of the old paragraph, and new paragraph, into DI double linked list */
  ME_InsertBefore(run, new_para);
  ME_InsertBefore(new_para, end_run);

  /* force rewrap of the */
  run_para->member.para.prev_para->member.para.nFlags |= MEPF_REWRAP;
  new_para->member.para.prev_para->member.para.nFlags |= MEPF_REWRAP;
  
  /* we've added the end run, so we need to modify nCharOfs in the next paragraphs */
  ME_PropagateCharOffset(next_para, end_len);
  editor->nParagraphs++;
  
  return new_para;
}

/* join tp with tp->member.para.next_para, keeping tp's style; this
 * is consistent with the original */
ME_DisplayItem *ME_JoinParagraphs(ME_TextEditor *editor, ME_DisplayItem *tp)
{
  ME_DisplayItem *pNext, *pFirstRunInNext, *pRun, *pTmp;
  int i, shift;
  ME_UndoItem *undo = NULL;
  int end_len = (editor->bEmulateVersion10 ? 2 : 1);

  assert(tp->type == diParagraph);
  assert(tp->member.para.next_para);
  assert(tp->member.para.next_para->type == diParagraph);
  
  pNext = tp->member.para.next_para;
  
  {
    /* null char format operation to store the original char format for the ENDPARA run */
    CHARFORMAT2W fmt;
    ME_InitCharFormat2W(&fmt);
    ME_SetCharFormat(editor, pNext->member.para.nCharOfs - end_len, end_len, &fmt);
  }
  undo = ME_AddUndoItem(editor, diUndoSplitParagraph, NULL);
  if (undo)
  {
    undo->nStart = pNext->member.para.nCharOfs - end_len;
    assert(pNext->member.para.pFmt->cbSize == sizeof(PARAFORMAT2));
    CopyMemory(undo->di.member.para.pFmt, pNext->member.para.pFmt, sizeof(PARAFORMAT2));
  }
  
  shift = pNext->member.para.nCharOfs - tp->member.para.nCharOfs - end_len;
  
  pRun = ME_FindItemBack(pNext, diRunOrParagraph);
  pFirstRunInNext = ME_FindItemFwd(pNext, diRunOrParagraph);
  
  assert(pRun);
  assert(pRun->type == diRun);
  assert(pRun->member.run.nFlags & MERF_ENDPARA);
  assert(pFirstRunInNext->type == diRun);
  
  /* if some cursor points at end of paragraph, make it point to the first
     run of the next joined paragraph */
  for (i=0; i<editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == pRun) {
      editor->pCursors[i].pRun = pFirstRunInNext;
      editor->pCursors[i].nOffset = 0;
    }
  }

  pTmp = pNext;
  do {
    pTmp = ME_FindItemFwd(pTmp, diRunOrParagraphOrEnd);
    if (pTmp->type != diRun)
      break;
    TRACE("shifting \"%s\" by %d (previous %d)\n", debugstr_w(pTmp->member.run.strText->szData), shift, pTmp->member.run.nCharOfs);
    pTmp->member.run.nCharOfs += shift;
  } while(1);
  
  ME_Remove(pRun);
  ME_DestroyDisplayItem(pRun);

  if (editor->pLastSelStartPara == pNext)
    editor->pLastSelStartPara = tp;
  if (editor->pLastSelEndPara == pNext)
    editor->pLastSelEndPara = tp;
    
  tp->member.para.next_para = pNext->member.para.next_para;
  pNext->member.para.next_para->member.para.prev_para = tp;
  ME_Remove(pNext);
  ME_DestroyDisplayItem(pNext);

  ME_PropagateCharOffset(tp->member.para.next_para, -end_len);
  
  ME_CheckCharOffsets(editor);
  
  editor->nParagraphs--;
  tp->member.para.nFlags |= MEPF_REWRAP;
  return tp;
}

ME_DisplayItem *ME_GetParagraph(ME_DisplayItem *item) {
  return ME_FindItemBackOrHere(item, diParagraph);
}

static void ME_DumpStyleEffect(char **p, const char *name, PARAFORMAT2 *fmt, int mask)
{
  *p += sprintf(*p, "%-22s%s\n", name, (fmt->dwMask & mask) ? ((fmt->wEffects & mask) ? "yes" : "no") : "N/A");
}

void ME_DumpParaStyleToBuf(PARAFORMAT2 *pFmt, char buf[2048])
{
  /* FIXME only PARAFORMAT styles implemented */
  char *p;
  p = buf;
  p += sprintf(p, "Alignment:            %s\n",
    !(pFmt->dwMask & PFM_ALIGNMENT) ? "N/A" :
      ((pFmt->wAlignment == PFA_LEFT) ? "left" :
        ((pFmt->wAlignment == PFA_RIGHT) ? "right" :
          ((pFmt->wAlignment == PFA_CENTER) ? "center" :
            /*((pFmt->wAlignment == PFA_JUSTIFY) ? "justify" : "incorrect")*/
            "incorrect"))));

  if (pFmt->dwMask & PFM_OFFSET)
    p += sprintf(p, "Offset:               %d\n", (int)pFmt->dxOffset);
  else
    p += sprintf(p, "Offset:               N/A\n");
    
  if (pFmt->dwMask & PFM_OFFSETINDENT)
    p += sprintf(p, "Offset indent:        %d\n", (int)pFmt->dxStartIndent);
  else
    p += sprintf(p, "Offset indent:        N/A\n");
    
  if (pFmt->dwMask & PFM_STARTINDENT)
    p += sprintf(p, "Start indent:         %d\n", (int)pFmt->dxStartIndent);
  else
    p += sprintf(p, "Start indent:         N/A\n");
    
  if (pFmt->dwMask & PFM_RIGHTINDENT)
    p += sprintf(p, "Right indent:         %d\n", (int)pFmt->dxRightIndent);
  else
    p += sprintf(p, "Right indent:         N/A\n");
    
  ME_DumpStyleEffect(&p, "Page break before:", pFmt, PFM_PAGEBREAKBEFORE);
}

void ME_SetParaFormat(ME_TextEditor *editor, ME_DisplayItem *para, PARAFORMAT2 *pFmt)
{
  PARAFORMAT2 copy;
  assert(sizeof(*para->member.para.pFmt) == sizeof(PARAFORMAT2));
  ME_AddUndoItem(editor, diUndoSetParagraphFormat, para);
  
  CopyMemory(&copy, para->member.para.pFmt, sizeof(PARAFORMAT2));

  if (pFmt->dwMask & PFM_ALIGNMENT)
    para->member.para.pFmt->wAlignment = pFmt->wAlignment;
  if (pFmt->dwMask & PFM_STARTINDENT)
    para->member.para.pFmt->dxStartIndent = pFmt->dxStartIndent;
  if (pFmt->dwMask & PFM_OFFSET)
    para->member.para.pFmt->dxOffset = pFmt->dxOffset;
  if (pFmt->dwMask & PFM_OFFSETINDENT)
    para->member.para.pFmt->dxStartIndent += pFmt->dxStartIndent;
    
  if (pFmt->dwMask & PFM_TABSTOPS)
  {
    para->member.para.pFmt->cTabCount = pFmt->cTabCount;
    memcpy(para->member.para.pFmt->rgxTabs, pFmt->rgxTabs, pFmt->cTabCount*sizeof(int));
  }
    
  /* FIXME to be continued (indents, bulleting and such) */

  if (memcmp(&copy, para->member.para.pFmt, sizeof(PARAFORMAT2)))
    para->member.para.nFlags |= MEPF_REWRAP;
}


void
ME_GetSelectionParas(ME_TextEditor *editor, ME_DisplayItem **para, ME_DisplayItem **para_end)
{
  ME_Cursor *pEndCursor = &editor->pCursors[1];
  
  *para = ME_GetParagraph(editor->pCursors[0].pRun);
  *para_end = ME_GetParagraph(editor->pCursors[1].pRun);
  if ((*para_end)->member.para.nCharOfs < (*para)->member.para.nCharOfs) {
    ME_DisplayItem *tmp = *para;

    *para = *para_end;
    *para_end = tmp;
    pEndCursor = &editor->pCursors[0];
  }
  
  /* selection consists of chars from nFrom up to nTo-1 */
  if ((*para_end)->member.para.nCharOfs > (*para)->member.para.nCharOfs) {
    if (!pEndCursor->nOffset) {
      *para_end = ME_GetParagraph(ME_FindItemBack(pEndCursor->pRun, diRun));
    }
  }
}


void ME_SetSelectionParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt)
{
  ME_DisplayItem *para, *para_end;
  
  ME_GetSelectionParas(editor, &para, &para_end);
 
  do {
    ME_SetParaFormat(editor, para, pFmt);
    if (para == para_end)
      break;
    para = para->member.para.next_para;
  } while(1);
}

void ME_GetParaFormat(ME_TextEditor *editor, ME_DisplayItem *para, PARAFORMAT2 *pFmt)
{
  if (pFmt->cbSize >= sizeof(PARAFORMAT2))
  {
    CopyMemory(pFmt, para->member.para.pFmt, sizeof(PARAFORMAT2));
    return;
  }
  CopyMemory(pFmt, para->member.para.pFmt, pFmt->cbSize);  
}

void ME_GetSelectionParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt)
{
  ME_DisplayItem *para, *para_end;
  PARAFORMAT2 tmp;
  
  ME_GetSelectionParas(editor, &para, &para_end);
  
  ME_GetParaFormat(editor, para, pFmt);
  if (para == para_end) return;
  
  do {
    ZeroMemory(&tmp, sizeof(tmp));
    tmp.cbSize = sizeof(tmp);
    ME_GetParaFormat(editor, para, &tmp);
    
    assert(tmp.dwMask & PFM_ALIGNMENT);    
    if (pFmt->wAlignment != tmp.wAlignment)
      pFmt->dwMask &= ~PFM_ALIGNMENT;
    
    assert(tmp.dwMask & PFM_STARTINDENT);
    if (pFmt->dxStartIndent != tmp.dxStartIndent)
      pFmt->dwMask &= ~PFM_STARTINDENT;
    
    assert(tmp.dwMask & PFM_OFFSET);
    if (pFmt->dxOffset != tmp.dxOffset)
      pFmt->dwMask &= ~PFM_OFFSET;
    
    assert(tmp.dwMask & PFM_TABSTOPS);    
    if (pFmt->dwMask & PFM_TABSTOPS) {
      if (pFmt->cTabCount != tmp.cTabCount)
        pFmt->dwMask &= ~PFM_TABSTOPS;
      else
      if (memcmp(pFmt->rgxTabs, tmp.rgxTabs, tmp.cTabCount*sizeof(int)))
        pFmt->dwMask &= ~PFM_TABSTOPS;
    }
    
    if (para == para_end)
      return;
    para = para->member.para.next_para;
  } while(1);
}
