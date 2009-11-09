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

void ME_MakeFirstParagraph(ME_TextEditor *editor)
{
  ME_Context c;
  CHARFORMAT2W cf;
  LOGFONTW lf;
  HFONT hf;
  ME_TextBuffer *text = editor->pBuffer;
  ME_DisplayItem *para = ME_MakeDI(diParagraph);
  ME_DisplayItem *run;
  ME_Style *style;
  ME_String *eol_str;
  WCHAR cr_lf[] = {'\r','\n',0};

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));

  hf = GetStockObject(SYSTEM_FONT);
  assert(hf);
  GetObjectW(hf, sizeof(LOGFONTW), &lf);
  ZeroMemory(&cf, sizeof(cf));
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_BACKCOLOR|CFM_COLOR|CFM_FACE|CFM_SIZE|CFM_CHARSET;
  cf.dwMask |= CFM_ALLCAPS|CFM_BOLD|CFM_DISABLED|CFM_EMBOSS|CFM_HIDDEN;
  cf.dwMask |= CFM_IMPRINT|CFM_ITALIC|CFM_LINK|CFM_OUTLINE|CFM_PROTECTED;
  cf.dwMask |= CFM_REVISED|CFM_SHADOW|CFM_SMALLCAPS|CFM_STRIKEOUT;
  cf.dwMask |= CFM_SUBSCRIPT|CFM_UNDERLINETYPE|CFM_WEIGHT;
  
  cf.dwEffects = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;
  lstrcpyW(cf.szFaceName, lf.lfFaceName);
  /* Convert system font height from logical units to twips for cf.yHeight */
  cf.yHeight = (lf.lfHeight * 72 * 1440) / (c.dpi.cy * c.dpi.cy);
  if (lf.lfWeight > FW_NORMAL) cf.dwEffects |= CFE_BOLD;
  cf.wWeight = lf.lfWeight;
  if (lf.lfItalic) cf.dwEffects |= CFE_ITALIC;
  cf.bUnderlineType = (lf.lfUnderline) ? CFU_CF1UNDERLINE : CFU_UNDERLINENONE;
  if (lf.lfStrikeOut) cf.dwEffects |= CFE_STRIKEOUT;
  cf.bPitchAndFamily = lf.lfPitchAndFamily;
  cf.bCharSet = lf.lfCharSet;

  style = ME_MakeStyle(&cf);
  text->pDefaultStyle = style;

  eol_str = ME_MakeStringN(cr_lf, editor->bEmulateVersion10 ? 2 : 1);
  run = ME_MakeRun(style, eol_str, MERF_ENDPARA);
  run->member.run.nCharOfs = 0;

  ME_InsertBefore(text->pLast, para);
  ME_InsertBefore(text->pLast, run);
  para->member.para.prev_para = text->pFirst;
  para->member.para.next_para = text->pLast;
  text->pFirst->member.para.next_para = para;
  text->pLast->member.para.prev_para = para;

  text->pLast->member.para.nCharOfs = editor->bEmulateVersion10 ? 2 : 1;

  ME_DestroyContext(&c);
}

static void ME_MarkForWrapping(ME_TextEditor *editor, ME_DisplayItem *first, const ME_DisplayItem *last)
{
  while(first != last)
  {
    first->member.para.nFlags |= MEPF_REWRAP;
    first = first->member.para.next_para;
  }
}

void ME_MarkAllForWrapping(ME_TextEditor *editor)
{
  ME_MarkForWrapping(editor, editor->pBuffer->pFirst->member.para.next_para, editor->pBuffer->pLast);
}

void ME_MarkForPainting(ME_TextEditor *editor, ME_DisplayItem *first, const ME_DisplayItem *last)
{
  while(first != last && first)
  {
    first->member.para.nFlags |= MEPF_REPAINT;
    first = first->member.para.next_para;
  }
}

static void ME_UpdateTableFlags(ME_DisplayItem *para)
{
  para->member.para.pFmt->dwMask |= PFM_TABLE|PFM_TABLEROWDELIMITER;
  if (para->member.para.pCell) {
    para->member.para.nFlags |= MEPF_CELL;
  } else {
    para->member.para.nFlags &= ~MEPF_CELL;
  }
  if (para->member.para.nFlags & MEPF_ROWEND) {
    para->member.para.pFmt->wEffects |= PFE_TABLEROWDELIMITER;
  } else {
    para->member.para.pFmt->wEffects &= ~PFE_TABLEROWDELIMITER;
  }
  if (para->member.para.nFlags & (MEPF_ROWSTART|MEPF_CELL|MEPF_ROWEND))
    para->member.para.pFmt->wEffects |= PFE_TABLE;
  else
    para->member.para.pFmt->wEffects &= ~PFE_TABLE;
}

static BOOL ME_SetParaFormat(ME_TextEditor *editor, ME_DisplayItem *para, const PARAFORMAT2 *pFmt)
{
  PARAFORMAT2 copy;
  DWORD dwMask;

  assert(para->member.para.pFmt->cbSize == sizeof(PARAFORMAT2));
  dwMask = pFmt->dwMask;
  if (pFmt->cbSize < sizeof(PARAFORMAT))
    return FALSE;
  else if (pFmt->cbSize < sizeof(PARAFORMAT2))
    dwMask &= PFM_ALL;
  else
    dwMask &= PFM_ALL2;

  ME_AddUndoItem(editor, diUndoSetParagraphFormat, para);

  copy = *para->member.para.pFmt;

#define COPY_FIELD(m, f) \
  if (dwMask & (m)) {                           \
    para->member.para.pFmt->dwMask |= m;        \
    para->member.para.pFmt->f = pFmt->f;        \
  }

  COPY_FIELD(PFM_NUMBERING, wNumbering);
  COPY_FIELD(PFM_STARTINDENT, dxStartIndent);
  if (dwMask & PFM_OFFSETINDENT)
    para->member.para.pFmt->dxStartIndent += pFmt->dxStartIndent;
  COPY_FIELD(PFM_RIGHTINDENT, dxRightIndent);
  COPY_FIELD(PFM_OFFSET, dxOffset);
  COPY_FIELD(PFM_ALIGNMENT, wAlignment);
  if (dwMask & PFM_TABSTOPS)
  {
    para->member.para.pFmt->cTabCount = pFmt->cTabCount;
    memcpy(para->member.para.pFmt->rgxTabs, pFmt->rgxTabs, pFmt->cTabCount*sizeof(LONG));
  }

  if (dwMask & (PFM_ALL2 & ~PFM_ALL))
  {
    /* PARAFORMAT2 fields */

#define EFFECTS_MASK (PFM_RTLPARA|PFM_KEEP|PFM_KEEPNEXT|PFM_PAGEBREAKBEFORE| \
                      PFM_NOLINENUMBER|PFM_NOWIDOWCONTROL|PFM_DONOTHYPHEN|PFM_SIDEBYSIDE| \
                      PFM_TABLE)
    /* we take for granted that PFE_xxx is the hiword of the corresponding PFM_xxx */
    if (dwMask & EFFECTS_MASK) {
      para->member.para.pFmt->dwMask |= dwMask & EFFECTS_MASK;
      para->member.para.pFmt->wEffects &= ~HIWORD(dwMask);
      para->member.para.pFmt->wEffects |= pFmt->wEffects & HIWORD(dwMask);
    }
#undef EFFECTS_MASK

    COPY_FIELD(PFM_SPACEBEFORE, dySpaceBefore);
    COPY_FIELD(PFM_SPACEAFTER, dySpaceAfter);
    COPY_FIELD(PFM_LINESPACING, dyLineSpacing);
    COPY_FIELD(PFM_STYLE, sStyle);
    COPY_FIELD(PFM_LINESPACING, bLineSpacingRule);
    COPY_FIELD(PFM_SHADING, wShadingWeight);
    COPY_FIELD(PFM_SHADING, wShadingStyle);
    COPY_FIELD(PFM_NUMBERINGSTART, wNumberingStart);
    COPY_FIELD(PFM_NUMBERINGSTYLE, wNumberingStyle);
    COPY_FIELD(PFM_NUMBERINGTAB, wNumberingTab);
    COPY_FIELD(PFM_BORDER, wBorderSpace);
    COPY_FIELD(PFM_BORDER, wBorderWidth);
    COPY_FIELD(PFM_BORDER, wBorders);
  }

  para->member.para.pFmt->dwMask |= dwMask;
#undef COPY_FIELD

  if (memcmp(&copy, para->member.para.pFmt, sizeof(PARAFORMAT2)))
    para->member.para.nFlags |= MEPF_REWRAP;

  return TRUE;
}

/* split paragraph at the beginning of the run */
ME_DisplayItem *ME_SplitParagraph(ME_TextEditor *editor, ME_DisplayItem *run,
                                  ME_Style *style, ME_String *eol_str,
                                  int paraFlags)
{
  ME_DisplayItem *next_para = NULL;
  ME_DisplayItem *run_para = NULL;
  ME_DisplayItem *new_para = ME_MakeDI(diParagraph);
  ME_DisplayItem *end_run;
  ME_UndoItem *undo = NULL;
  int ofs, i;
  ME_DisplayItem *pp;
  int run_flags = MERF_ENDPARA;

  if (!editor->bEmulateVersion10) { /* v4.1 */
    /* At most 1 of MEPF_CELL, MEPF_ROWSTART, or MEPF_ROWEND should be set. */
    assert(!(paraFlags & ~(MEPF_CELL|MEPF_ROWSTART|MEPF_ROWEND)));
    assert(!(paraFlags & (paraFlags-1)));
    if (paraFlags == MEPF_CELL)
      run_flags |= MERF_ENDCELL;
    else if (paraFlags == MEPF_ROWSTART)
      run_flags |= MERF_TABLESTART|MERF_HIDDEN;
  } else { /* v1.0 - v3.0 */
    assert(!(paraFlags & (MEPF_CELL|MEPF_ROWSTART|MEPF_ROWEND)));
  }
  end_run = ME_MakeRun(style, eol_str, run_flags);

  assert(run->type == diRun);
  run_para = ME_GetParagraph(run);
  assert(run_para->member.para.pFmt->cbSize == sizeof(PARAFORMAT2));

  ofs = end_run->member.run.nCharOfs = run->member.run.nCharOfs;
  next_para = run_para->member.para.next_para;
  assert(next_para == ME_FindItemFwd(run_para, diParagraphOrEnd));
  
  undo = ME_AddUndoItem(editor, diUndoJoinParagraphs, NULL);
  if (undo)
    undo->nStart = run_para->member.para.nCharOfs + ofs;

  /* Update selection cursors to point to the correct paragraph. */
  for (i = 0; i < editor->nCursors; i++) {
    if (editor->pCursors[i].pPara == run_para &&
        run->member.run.nCharOfs <= editor->pCursors[i].pRun->member.run.nCharOfs)
    {
      editor->pCursors[i].pPara = new_para;
    }
  }

  /* the new paragraph will have a different starting offset, so let's update its runs */
  pp = run;
  while(pp->type == diRun) {
    pp->member.run.nCharOfs -= ofs;
    pp = ME_FindItemFwd(pp, diRunOrParagraphOrEnd);
  }
  new_para->member.para.nCharOfs = run_para->member.para.nCharOfs + ofs;
  new_para->member.para.nCharOfs += eol_str->nLen;
  new_para->member.para.nFlags = MEPF_REWRAP;

  /* FIXME initialize format style and call ME_SetParaFormat blah blah */
  *new_para->member.para.pFmt = *run_para->member.para.pFmt;
  new_para->member.para.border = run_para->member.para.border;

  /* insert paragraph into paragraph double linked list */
  new_para->member.para.prev_para = run_para;
  new_para->member.para.next_para = next_para;
  run_para->member.para.next_para = new_para;
  next_para->member.para.prev_para = new_para;

  /* insert end run of the old paragraph, and new paragraph, into DI double linked list */
  ME_InsertBefore(run, new_para);
  ME_InsertBefore(new_para, end_run);

  if (!editor->bEmulateVersion10) { /* v4.1 */
    if (paraFlags & (MEPF_ROWSTART|MEPF_CELL))
    {
      ME_DisplayItem *cell = ME_MakeDI(diCell);
      ME_InsertBefore(new_para, cell);
      new_para->member.para.pCell = cell;
      cell->member.cell.next_cell = NULL;
      if (paraFlags & MEPF_ROWSTART)
      {
        run_para->member.para.nFlags |= MEPF_ROWSTART;
        cell->member.cell.prev_cell = NULL;
        cell->member.cell.parent_cell = run_para->member.para.pCell;
        if (run_para->member.para.pCell)
          cell->member.cell.nNestingLevel = run_para->member.para.pCell->member.cell.nNestingLevel + 1;
        else
          cell->member.cell.nNestingLevel = 1;
      } else {
        cell->member.cell.prev_cell = run_para->member.para.pCell;
        assert(cell->member.cell.prev_cell);
        cell->member.cell.prev_cell->member.cell.next_cell = cell;
        assert(run_para->member.para.nFlags & MEPF_CELL);
        assert(!(run_para->member.para.nFlags & MEPF_ROWSTART));
        cell->member.cell.nNestingLevel = cell->member.cell.prev_cell->member.cell.nNestingLevel;
        cell->member.cell.parent_cell = cell->member.cell.prev_cell->member.cell.parent_cell;
      }
    } else if (paraFlags & MEPF_ROWEND) {
      run_para->member.para.nFlags |= MEPF_ROWEND;
      run_para->member.para.pCell = run_para->member.para.pCell->member.cell.parent_cell;
      new_para->member.para.pCell = run_para->member.para.pCell;
      assert(run_para->member.para.prev_para->member.para.nFlags & MEPF_CELL);
      assert(!(run_para->member.para.prev_para->member.para.nFlags & MEPF_ROWSTART));
      if (new_para->member.para.pCell != new_para->member.para.next_para->member.para.pCell
          && new_para->member.para.next_para->member.para.pCell
          && !new_para->member.para.next_para->member.para.pCell->member.cell.prev_cell)
      {
        /* Row starts just after the row that was ended. */
        new_para->member.para.nFlags |= MEPF_ROWSTART;
      }
    } else {
      new_para->member.para.pCell = run_para->member.para.pCell;
    }
    ME_UpdateTableFlags(run_para);
    ME_UpdateTableFlags(new_para);
  }

  /* force rewrap of the */
  run_para->member.para.prev_para->member.para.nFlags |= MEPF_REWRAP;
  new_para->member.para.prev_para->member.para.nFlags |= MEPF_REWRAP;

  /* we've added the end run, so we need to modify nCharOfs in the next paragraphs */
  ME_PropagateCharOffset(next_para, eol_str->nLen);
  editor->nParagraphs++;

  return new_para;
}

/* join tp with tp->member.para.next_para, keeping tp's style; this
 * is consistent with the original */
ME_DisplayItem *ME_JoinParagraphs(ME_TextEditor *editor, ME_DisplayItem *tp,
                                  BOOL keepFirstParaFormat)
{
  ME_DisplayItem *pNext, *pFirstRunInNext, *pRun, *pTmp;
  int i, shift;
  ME_UndoItem *undo = NULL;
  int end_len;

  assert(tp->type == diParagraph);
  assert(tp->member.para.next_para);
  assert(tp->member.para.next_para->type == diParagraph);

  pNext = tp->member.para.next_para;

  /* Need to locate end-of-paragraph run here, in order to know end_len */
  pRun = ME_FindItemBack(pNext, diRunOrParagraph);

  assert(pRun);
  assert(pRun->type == diRun);
  assert(pRun->member.run.nFlags & MERF_ENDPARA);

  end_len = pRun->member.run.strText->nLen;

  {
    /* null char format operation to store the original char format for the ENDPARA run */
    CHARFORMAT2W fmt;
    ME_InitCharFormat2W(&fmt);
    ME_SetCharFormat(editor, pNext->member.para.nCharOfs - end_len, end_len, &fmt);
  }
  undo = ME_AddUndoItem(editor, diUndoSplitParagraph, pNext);
  if (undo)
  {
    undo->nStart = pNext->member.para.nCharOfs - end_len;
    undo->eol_str = pRun->member.run.strText;
    pRun->member.run.strText = NULL; /* Avoid freeing the string */
  }
  if (!keepFirstParaFormat)
  {
    ME_AddUndoItem(editor, diUndoSetParagraphFormat, tp);
    *tp->member.para.pFmt = *pNext->member.para.pFmt;
    tp->member.para.border = pNext->member.para.border;
  }

  if (!editor->bEmulateVersion10) { /* v4.1 */
    /* Table cell/row properties are always moved over from the removed para. */
    tp->member.para.nFlags = pNext->member.para.nFlags;
    tp->member.para.pCell = pNext->member.para.pCell;

    /* Remove cell boundary if it is between the end paragraph run and the next
     * paragraph display item. */
    pTmp = pRun->next;
    while (pTmp != pNext) {
      if (pTmp->type == diCell)
      {
        ME_Cell *pCell = &pTmp->member.cell;
        if (undo)
        {
          assert(!(undo->di.member.para.nFlags & MEPF_ROWEND));
          if (!(undo->di.member.para.nFlags & MEPF_ROWSTART))
            undo->di.member.para.nFlags |= MEPF_CELL;
          undo->di.member.para.pCell = ALLOC_OBJ(ME_DisplayItem);
          *undo->di.member.para.pCell = *pTmp;
          undo->di.member.para.pCell->next = NULL;
          undo->di.member.para.pCell->prev = NULL;
          undo->di.member.para.pCell->member.cell.next_cell = NULL;
          undo->di.member.para.pCell->member.cell.prev_cell = NULL;
        }
        ME_Remove(pTmp);
        if (pCell->prev_cell)
          pCell->prev_cell->member.cell.next_cell = pCell->next_cell;
        if (pCell->next_cell)
          pCell->next_cell->member.cell.prev_cell = pCell->prev_cell;
        ME_DestroyDisplayItem(pTmp);
        break;
      }
      pTmp = pTmp->next;
    }
  }

  shift = pNext->member.para.nCharOfs - tp->member.para.nCharOfs - end_len;

  pFirstRunInNext = ME_FindItemFwd(pNext, diRunOrParagraph);

  assert(pFirstRunInNext->type == diRun);

  /* Update selection cursors so they don't point to the removed end
   * paragraph run, and point to the correct paragraph. */
  for (i=0; i < editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == pRun) {
      editor->pCursors[i].pRun = pFirstRunInNext;
      editor->pCursors[i].nOffset = 0;
    } else if (editor->pCursors[i].pPara == pNext) {
      editor->pCursors[i].pPara = tp;
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

void ME_DumpParaStyleToBuf(const PARAFORMAT2 *pFmt, char buf[2048])
{
  char *p;
  p = buf;

#define DUMP(mask, name, fmt, field) \
  if (pFmt->dwMask & (mask)) p += sprintf(p, "%-22s" fmt "\n", name, pFmt->field); \
  else p += sprintf(p, "%-22sN/A\n", name);

/* we take for granted that PFE_xxx is the hiword of the corresponding PFM_xxx */
#define DUMP_EFFECT(mask, name) \
  p += sprintf(p, "%-22s%s\n", name, (pFmt->dwMask & (mask)) ? ((pFmt->wEffects & ((mask) >> 8)) ? "yes" : "no") : "N/A");

  DUMP(PFM_NUMBERING,      "Numbering:",         "%u", wNumbering);
  DUMP_EFFECT(PFM_DONOTHYPHEN,     "Disable auto-hyphen:");
  DUMP_EFFECT(PFM_KEEP,            "No page break in para:");
  DUMP_EFFECT(PFM_KEEPNEXT,        "No page break in para & next:");
  DUMP_EFFECT(PFM_NOLINENUMBER,    "No line number:");
  DUMP_EFFECT(PFM_NOWIDOWCONTROL,  "No widow & orphan:");
  DUMP_EFFECT(PFM_PAGEBREAKBEFORE, "Page break before:");
  DUMP_EFFECT(PFM_RTLPARA,         "RTL para:");
  DUMP_EFFECT(PFM_SIDEBYSIDE,      "Side by side:");
  DUMP_EFFECT(PFM_TABLE,           "Table:");
  DUMP(PFM_OFFSETINDENT,   "Offset indent:",     "%d", dxStartIndent);
  DUMP(PFM_STARTINDENT,    "Start indent:",      "%d", dxStartIndent);
  DUMP(PFM_RIGHTINDENT,    "Right indent:",      "%d", dxRightIndent);
  DUMP(PFM_OFFSET,         "Offset:",            "%d", dxOffset);
  if (pFmt->dwMask & PFM_ALIGNMENT) {
    switch (pFmt->wAlignment) {
    case PFA_LEFT   : p += sprintf(p, "Alignment:            left\n"); break;
    case PFA_RIGHT  : p += sprintf(p, "Alignment:            right\n"); break;
    case PFA_CENTER : p += sprintf(p, "Alignment:            center\n"); break;
    case PFA_JUSTIFY: p += sprintf(p, "Alignment:            justify\n"); break;
    default         : p += sprintf(p, "Alignment:            incorrect %d\n", pFmt->wAlignment); break;
    }
  }
  else p += sprintf(p, "Alignment:            N/A\n");
  DUMP(PFM_TABSTOPS,       "Tab Stops:",         "%d", cTabCount);
  if (pFmt->dwMask & PFM_TABSTOPS) {
    int i;
    p += sprintf(p, "\t");
    for (i = 0; i < pFmt->cTabCount; i++) p += sprintf(p, "%x ", pFmt->rgxTabs[i]);
    p += sprintf(p, "\n");
  }
  DUMP(PFM_SPACEBEFORE,    "Space Before:",      "%d", dySpaceBefore);
  DUMP(PFM_SPACEAFTER,     "Space After:",       "%d", dySpaceAfter);
  DUMP(PFM_LINESPACING,    "Line spacing:",      "%d", dyLineSpacing);
  DUMP(PFM_STYLE,          "Text style:",        "%d", sStyle);
  DUMP(PFM_LINESPACING,    "Line spacing rule:", "%u", bLineSpacingRule);
  /* bOutlineLevel should be 0 */
  DUMP(PFM_SHADING,        "Shading Weigth:",    "%u", wShadingWeight);
  DUMP(PFM_SHADING,        "Shading Style:",     "%u", wShadingStyle);
  DUMP(PFM_NUMBERINGSTART, "Numbering Start:",   "%u", wNumberingStart);
  DUMP(PFM_NUMBERINGSTYLE, "Numbering Style:",   "0x%x", wNumberingStyle);
  DUMP(PFM_NUMBERINGTAB,   "Numbering Tab:",     "%u", wNumberingStyle);
  DUMP(PFM_BORDER,         "Border Space:",      "%u", wBorderSpace);
  DUMP(PFM_BORDER,         "Border Width:",      "%u", wBorderWidth);
  DUMP(PFM_BORDER,         "Borders:",           "%u", wBorders);

#undef DUMP
#undef DUMP_EFFECT
}

void
ME_GetSelectionParas(ME_TextEditor *editor, ME_DisplayItem **para, ME_DisplayItem **para_end)
{
  ME_Cursor *pEndCursor = &editor->pCursors[1];

  *para = editor->pCursors[0].pPara;
  *para_end = editor->pCursors[1].pPara;
  if (*para == *para_end)
    return;

  if ((*para_end)->member.para.nCharOfs < (*para)->member.para.nCharOfs) {
    ME_DisplayItem *tmp = *para;

    *para = *para_end;
    *para_end = tmp;
    pEndCursor = &editor->pCursors[0];
  }

  /* The paragraph at the end of a non-empty selection isn't included
   * if the selection ends at the start of the paragraph. */
  if (!pEndCursor->pRun->member.run.nCharOfs && !pEndCursor->nOffset)
    *para_end = (*para_end)->member.para.prev_para;
}


BOOL ME_SetSelectionParaFormat(ME_TextEditor *editor, const PARAFORMAT2 *pFmt)
{
  ME_DisplayItem *para, *para_end;

  ME_GetSelectionParas(editor, &para, &para_end);

  do {
    ME_SetParaFormat(editor, para, pFmt);
    if (para == para_end)
      break;
    para = para->member.para.next_para;
  } while(1);

  return TRUE;
}

static void ME_GetParaFormat(ME_TextEditor *editor,
                             const ME_DisplayItem *para,
                             PARAFORMAT2 *pFmt)
{
  UINT cbSize = pFmt->cbSize;
  if (pFmt->cbSize >= sizeof(PARAFORMAT2)) {
    *pFmt = *para->member.para.pFmt;
  } else {
    CopyMemory(pFmt, para->member.para.pFmt, pFmt->cbSize);
    pFmt->dwMask &= PFM_ALL;
  }
  pFmt->cbSize = cbSize;
}

void ME_GetSelectionParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt)
{
  ME_DisplayItem *para, *para_end;
  PARAFORMAT2 *curFmt;

  if (pFmt->cbSize < sizeof(PARAFORMAT)) {
    pFmt->dwMask = 0;
    return;
  }

  ME_GetSelectionParas(editor, &para, &para_end);

  ME_GetParaFormat(editor, para, pFmt);

  /* Invalidate values that change across the selected paragraphs. */
  while (para != para_end)
  {
    para = para->member.para.next_para;
    curFmt = para->member.para.pFmt;

#define CHECK_FIELD(m, f) \
    if (pFmt->f != curFmt->f) pFmt->dwMask &= ~(m);

    CHECK_FIELD(PFM_NUMBERING, wNumbering);
    CHECK_FIELD(PFM_STARTINDENT, dxStartIndent);
    CHECK_FIELD(PFM_RIGHTINDENT, dxRightIndent);
    CHECK_FIELD(PFM_OFFSET, dxOffset);
    CHECK_FIELD(PFM_ALIGNMENT, wAlignment);
    if (pFmt->dwMask & PFM_TABSTOPS) {
      if (pFmt->cTabCount != para->member.para.pFmt->cTabCount ||
          memcmp(pFmt->rgxTabs, curFmt->rgxTabs, curFmt->cTabCount*sizeof(int)))
        pFmt->dwMask &= ~PFM_TABSTOPS;
    }

    if (pFmt->dwMask >= sizeof(PARAFORMAT2))
    {
      pFmt->dwMask &= ~((pFmt->wEffects ^ curFmt->wEffects) << 16);
      CHECK_FIELD(PFM_SPACEBEFORE, dySpaceBefore);
      CHECK_FIELD(PFM_SPACEAFTER, dySpaceAfter);
      CHECK_FIELD(PFM_LINESPACING, dyLineSpacing);
      CHECK_FIELD(PFM_STYLE, sStyle);
      CHECK_FIELD(PFM_SPACEAFTER, bLineSpacingRule);
      CHECK_FIELD(PFM_SHADING, wShadingWeight);
      CHECK_FIELD(PFM_SHADING, wShadingStyle);
      CHECK_FIELD(PFM_NUMBERINGSTART, wNumberingStart);
      CHECK_FIELD(PFM_NUMBERINGSTYLE, wNumberingStyle);
      CHECK_FIELD(PFM_NUMBERINGTAB, wNumberingTab);
      CHECK_FIELD(PFM_BORDER, wBorderSpace);
      CHECK_FIELD(PFM_BORDER, wBorderWidth);
      CHECK_FIELD(PFM_BORDER, wBorders);
    }
#undef CHECK_FIELD
  }
}

void ME_SetDefaultParaFormat(PARAFORMAT2 *pFmt)
{
    ZeroMemory(pFmt, sizeof(PARAFORMAT2));
    pFmt->cbSize = sizeof(PARAFORMAT2);
    pFmt->dwMask = PFM_ALL2;
    pFmt->wAlignment = PFA_LEFT;
    pFmt->sStyle = -1;
    pFmt->bOutlineLevel = TRUE;
}
