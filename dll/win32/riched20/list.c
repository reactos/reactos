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
  switch (nTypeOrClass)
  {
    case diRunOrParagraph:
      return type == diRun || type == diParagraph;
    case diRunOrStartRow:
      return type == diRun || type == diStartRow;
    case diParagraphOrEnd:
      return type == diTextEnd || type == diParagraph;
    case diStartRowOrParagraph:
      return type == diStartRow || type == diParagraph;
    case diStartRowOrParagraphOrEnd:
      return type == diStartRow || type == diParagraph || type == diTextEnd;
    case diRunOrParagraphOrEnd:
      return type == diRun || type == diParagraph || type == diTextEnd;
    default:
      return type == nTypeOrClass;
  }
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

static const char *ME_GetDITypeName(ME_DIType type)
{
  switch(type)
  {
    case diParagraph: return "diParagraph";
    case diRun: return "diRun";
    case diCell: return "diCell";
    case diTextStart: return "diTextStart";
    case diTextEnd: return "diTextEnd";
    case diStartRow: return "diStartRow";
    default: return "?";
  }
}

void ME_DestroyDisplayItem(ME_DisplayItem *item)
{
  if (0)
    TRACE("type=%s\n", ME_GetDITypeName(item->type));
  if (item->type==diRun)
  {
    if (item->member.run.reobj)
    {
      list_remove(&item->member.run.reobj->entry);
      ME_DeleteReObject(item->member.run.reobj);
    }
    free(item->member.run.glyphs);
    free(item->member.run.clusters);
    ME_ReleaseStyle(item->member.run.style);
  }
  free(item);
}

ME_DisplayItem *ME_MakeDI(ME_DIType type)
{
  ME_DisplayItem *item = calloc(1, sizeof(*item));

  item->type = type;
  item->prev = item->next = NULL;
  return item;
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
        TRACE(" - Run(%s, %d, flags=%x)\n", debugstr_run( &pItem->member.run ),
          pItem->member.run.nCharOfs, pItem->member.run.nFlags);
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
