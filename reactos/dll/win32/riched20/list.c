/*
 * RichEdit - Basic operations on double linked lists.
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

WINE_DEFAULT_DEBUG_CHANNEL(richedit_lists);

void ME_InsertBefore(ME_DisplayItem *diWhere, ME_DisplayItem *diWhat)
{
  diWhat->next = diWhere;
  diWhat->prev = diWhere->prev;

  diWhere->prev->next = diWhat;
  diWhat->next->prev = diWhat;
}

void ME_Remove(ME_DisplayItem *diWhere)
{
  ME_DisplayItem *diNext = diWhere->next;
  ME_DisplayItem *diPrev = diWhere->prev;
  assert(diNext);
  assert(diPrev);
  diPrev->next = diNext;
  diNext->prev = diPrev;
}

static BOOL ME_DITypesEqual(ME_DIType type, ME_DIType nTypeOrClass)
{
  if (type==nTypeOrClass)
    return TRUE;
  if (nTypeOrClass==diRunOrParagraph && (type==diRun || type==diParagraph))
    return TRUE;
  if (nTypeOrClass==diRunOrStartRow && (type==diRun || type==diStartRow))
    return TRUE;
  if (nTypeOrClass==diParagraphOrEnd && (type==diTextEnd || type==diParagraph))
    return TRUE;
  if (nTypeOrClass==diStartRowOrParagraph && (type==diStartRow || type==diParagraph))
    return TRUE;
  if (nTypeOrClass==diStartRowOrParagraphOrEnd
    && (type==diStartRow || type==diParagraph || type==diTextEnd))
    return TRUE;
  if (nTypeOrClass==diRunOrParagraphOrEnd
    && (type==diRun || type==diParagraph || type==diTextEnd))
    return TRUE;
  return FALSE;
}

ME_DisplayItem *ME_FindItemBack(ME_DisplayItem *di, ME_DIType nTypeOrClass)
{
  if (!di)
    return NULL;
  di = di->prev;
  while(di!=NULL) {
    if (ME_DITypesEqual(di->type, nTypeOrClass))
      return di;
    di = di->prev;
  }
  return NULL;
}

ME_DisplayItem *ME_FindItemBackOrHere(ME_DisplayItem *di, ME_DIType nTypeOrClass)
{
  while(di!=NULL) {
    if (ME_DITypesEqual(di->type, nTypeOrClass))
      return di;
    di = di->prev;
  }
  return NULL;
}

ME_DisplayItem *ME_FindItemFwd(ME_DisplayItem *di, ME_DIType nTypeOrClass)
{
  if (!di) return NULL;
  di = di->next;
  while(di!=NULL) {
    if (ME_DITypesEqual(di->type, nTypeOrClass))
      return di;
    di = di->next;
  }
  return NULL;
}

ME_DisplayItem *ME_FindItemFwdOrHere(ME_DisplayItem *di, ME_DIType nTypeOrClass)
{
  while(di!=NULL) {
    if (ME_DITypesEqual(di->type, nTypeOrClass))
      return di;
    di = di->next;
  }
  return NULL;
}

void ME_DestroyDisplayItem(ME_DisplayItem *item) {
/*  TRACE("type=%s\n", ME_GetDITypeName(item->type)); */
  if (item->type==diParagraph || item->type == diUndoSetParagraphFormat) {
    FREE_OBJ(item->member.para.pFmt);
  }
  if (item->type==diRun || item->type == diUndoInsertRun) {
    if (item->member.run.ole_obj) ME_DeleteReObject(item->member.run.ole_obj);
    ME_ReleaseStyle(item->member.run.style);
    ME_DestroyString(item->member.run.strText);
  }
  if (item->type==diUndoSetCharFormat) {
    ME_ReleaseStyle(item->member.ustyle);
  }
  if (item->type==diUndoSplitParagraph) {
     FREE_OBJ(item->member.para.pFmt);
     FREE_OBJ(item->member.para.pCell);
  }
  FREE_OBJ(item);
}

ME_DisplayItem *ME_MakeDI(ME_DIType type) {
  ME_DisplayItem *item = ALLOC_OBJ(ME_DisplayItem);
  ZeroMemory(item, sizeof(ME_DisplayItem));
  item->type = type;
  item->prev = item->next = NULL;
  if (type == diParagraph || type == diUndoSplitParagraph) {
    item->member.para.pFmt = ALLOC_OBJ(PARAFORMAT2);
    ME_SetDefaultParaFormat(item->member.para.pFmt);
    item->member.para.nFlags = MEPF_REWRAP;
  }
    
  return item;
}

const char *ME_GetDITypeName(ME_DIType type)
{
  switch(type)
  {
    case diParagraph: return "diParagraph";
    case diRun: return "diRun";
    case diCell: return "diCell";
    case diTextStart: return "diTextStart";
    case diTextEnd: return "diTextEnd";
    case diStartRow: return "diStartRow";
    case diUndoEndTransaction: return "diUndoEndTransaction";
    case diUndoPotentialEndTransaction: return "diUndoPotentialEndTransaction";
    case diUndoSetParagraphFormat: return "diUndoSetParagraphFormat";
    case diUndoSetCharFormat: return "diUndoSetCharFormat";
    case diUndoInsertRun: return "diUndoInsertRun";
    case diUndoDeleteRun: return "diUndoDeleteRun";
    case diUndoJoinParagraphs: return "diJoinParagraphs";
    case diUndoSplitParagraph: return "diSplitParagraph";
    default: return "?";
  }
}

void ME_DumpDocument(ME_TextBuffer *buffer)
{
  /* FIXME this is useless, */
  ME_DisplayItem *pItem = buffer->pFirst;
  TRACE("DOCUMENT DUMP START\n");
  while(pItem) {
    switch(pItem->type)
    {
      case diTextStart:
        TRACE("Start\n");
        break;
      case diCell:
        TRACE("Cell(level=%d%s)\n", pItem->member.cell.nNestingLevel,
              !pItem->member.cell.next_cell ? ", END" :
                (!pItem->member.cell.prev_cell ? ", START" :""));
        break;
      case diParagraph:
        TRACE("Paragraph(ofs=%d)\n", pItem->member.para.nCharOfs);
        if (pItem->member.para.nFlags & MEPF_ROWSTART)
          TRACE(" - (Table Row Start)\n");
        if (pItem->member.para.nFlags & MEPF_ROWEND)
          TRACE(" - (Table Row End)\n");
        break;
      case diStartRow:
        TRACE(" - StartRow\n");
        break;
      case diRun:
        TRACE(" - Run(\"%s\", %d)\n", debugstr_w(pItem->member.run.strText->szData), 
          pItem->member.run.nCharOfs);
        if (pItem->member.run.nFlags & MERF_ENDPARA)
          TRACE(" - Paragraph end: %d CR, %d LF\n", pItem->member.run.nCR, pItem->member.run.nLF);
        break;
      case diTextEnd:
        TRACE("End(ofs=%d)\n", pItem->member.para.nCharOfs);
        break;
      default:
        break;
    }
    pItem = pItem->next;
  }
  TRACE("DOCUMENT DUMP END\n");
}
