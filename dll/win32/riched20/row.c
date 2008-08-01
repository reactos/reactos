/*
 * RichEdit - Operations on rows of text (rows are recreated during
 * wrapping and are used for displaying the document, they don't keep any
 * true document content; delete all rows, rewrap all paragraphs and 
 * you get them back).
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

ME_DisplayItem *ME_FindRowStart(ME_Context *c, ME_DisplayItem *item, 
                                int nRelPos) {
  ME_DisplayItem *para = ME_GetParagraph(item);
  ME_MustBeWrapped(c, para);
  if(nRelPos<=0) { /* if this or preceding row */
    do {
      ME_DisplayItem *item2 = ME_FindItemBack(item, diStartRowOrParagraph);
      if (item2->type == diParagraph)
      {
        if (item2->member.para.prev_para == NULL)
          return item;
        /* if skipping to the preceding paragraph, ensure it's wrapped */
        ME_MustBeWrapped(c, item2->member.para.prev_para);
        item = item2;
        continue;
      }
      else if (item2->type == diStartRow)
      {
        nRelPos++;
        if (nRelPos>0)
          return item;
        item = item2;
        continue;
      }
      assert(0 == "bug in FindItemBack(item, diStartRowOrParagraph)");
      item = item2;
    } while(1);
  }
  while(nRelPos>0) { /* if one of the next rows */
    ME_DisplayItem *item2 = ME_FindItemFwd(item, diStartRowOrParagraph);
    if (!item2)
      return item;
    if (item2->type == diParagraph)
    {
      if (item2->member.para.next_para == NULL)
        return item;
      continue;
    }
    item = item2;
    nRelPos--;
  }
  return item;
}

/* I'm sure these functions would simplify some code in caret ops etc,
 * I just didn't remember them when I wrote that code
 */ 

ME_DisplayItem *ME_RowStart(ME_DisplayItem *item) {
  return ME_FindItemBackOrHere(item, diStartRow);
}

/*
ME_DisplayItem *ME_RowEnd(ME_DisplayItem *item) {
  ME_DisplayItem *item2 = ME_FindItemFwd(item, diStartRowOrParagraphOrEnd);
  if (!item2) return NULL;
  return ME_FindItemBack(item, diRun);
}
*/

ME_DisplayItem *
ME_FindRowWithNumber(ME_TextEditor *editor, int nRow)
{
  ME_DisplayItem *item = ME_FindItemFwd(editor->pBuffer->pFirst, diParagraph);
  int nCount = 0;
  
  while (item && nCount + item->member.para.nRows <= nRow)
  {
    nCount += item->member.para.nRows;
    item = ME_FindItemFwd(item, diParagraph);
  }
  if (!item)
    return item;
  for (item = ME_FindItemFwd(item, diStartRow); item && nCount < nRow; nCount++)
    item = ME_FindItemFwd(item, diStartRow);
  return item;
}


int
ME_RowNumberFromCharOfs(ME_TextEditor *editor, int nOfs)
{
  ME_DisplayItem *item = editor->pBuffer->pFirst->next;
  int nRow = 0;

  while (item && item->member.para.next_para->member.para.nCharOfs <= nOfs)
  {
    nRow += item->member.para.nRows;
    item = ME_FindItemFwd(item, diParagraph);
  }
  if (item)
  {
    ME_DisplayItem *next_para = item->member.para.next_para;
    
    nOfs -= item->member.para.nCharOfs;
    item = ME_FindItemFwd(item, diRun);
    while ((item = ME_FindItemFwd(item, diStartRowOrParagraph)) != NULL)
    {
      if (item == next_para)
        break;
      item = ME_FindItemFwd(item, diRun);
      if (item->member.run.nCharOfs > nOfs)
        break;
      nRow++;
    }
  }
  return nRow;
}
