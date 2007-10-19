/*
 * RichEdit - functions dealing with editor object
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

void ME_EmptyUndoStack(ME_TextEditor *editor)
{
  ME_DisplayItem *p, *pNext;

  if (editor->nUndoMode == umIgnore)
    return;

  TRACE("Emptying undo stack\n");

  p = editor->pUndoStack;
  editor->pUndoStack = editor->pUndoStackBottom = NULL;
  editor->nUndoStackSize = 0;
  while(p) {
    pNext = p->next;
    ME_DestroyDisplayItem(p);
    p = pNext;
  }
  p = editor->pRedoStack;
  editor->pRedoStack = NULL;
  while(p) {
    pNext = p->next;
    ME_DestroyDisplayItem(p);
    p = pNext;
  }
}

ME_UndoItem *ME_AddUndoItem(ME_TextEditor *editor, ME_DIType type, ME_DisplayItem *pdi) {
  if (editor->nUndoMode == umIgnore)
    return NULL;
  else if (editor->nUndoLimit == 0)
    return NULL;
  else
  {
    ME_DisplayItem *pItem = (ME_DisplayItem *)ALLOC_OBJ(ME_UndoItem);
    switch(type)
    {
    case diUndoEndTransaction:
      break;
    case diUndoSetParagraphFormat:
      assert(pdi);
      CopyMemory(&pItem->member.para, &pdi->member.para, sizeof(ME_Paragraph));
      pItem->member.para.pFmt = ALLOC_OBJ(PARAFORMAT2);
      CopyMemory(pItem->member.para.pFmt, pdi->member.para.pFmt, sizeof(PARAFORMAT2));
      break;
    case diUndoInsertRun:
      assert(pdi);
      CopyMemory(&pItem->member.run, &pdi->member.run, sizeof(ME_Run));
      pItem->member.run.strText = ME_StrDup(pItem->member.run.strText);
      ME_AddRefStyle(pItem->member.run.style);
      break;
    case diUndoSetCharFormat:
    case diUndoSetDefaultCharFormat:
      break;
    case diUndoDeleteRun:
    case diUndoJoinParagraphs:
      break;
    case diUndoSplitParagraph:
      pItem->member.para.pFmt = ALLOC_OBJ(PARAFORMAT2);
      pItem->member.para.pFmt->cbSize = sizeof(PARAFORMAT2);
      pItem->member.para.pFmt->dwMask = 0;

      break;
    default:
      assert(0 == "AddUndoItem, unsupported item type");
      return NULL;
    }
    pItem->type = type;
    pItem->prev = NULL;
    if (editor->nUndoMode == umAddToUndo || editor->nUndoMode == umAddBackToUndo)
    {
      if (editor->nUndoMode == umAddToUndo)
        TRACE("Pushing id=%s to undo stack, deleting redo stack\n", ME_GetDITypeName(type));
      else
        TRACE("Pushing id=%s to undo stack\n", ME_GetDITypeName(type));

      pItem->next = editor->pUndoStack;
      if (type == diUndoEndTransaction)
        editor->nUndoStackSize++;
      if (editor->pUndoStack)
        editor->pUndoStack->prev = pItem;
      else
        editor->pUndoStackBottom = pItem;
      editor->pUndoStack = pItem;

      if (editor->nUndoStackSize > editor->nUndoLimit)
      { /* remove oldest undo from stack */
        ME_DisplayItem *p = editor->pUndoStackBottom;
        while (p->type !=diUndoEndTransaction)
          p = p->prev; /*find new stack bottom */
        editor->pUndoStackBottom = p->prev;
          editor->pUndoStackBottom->next = NULL;
        do
        {
          ME_DisplayItem *pp = p->next;
          ME_DestroyDisplayItem(p);
          p = pp;
        } while (p);
        editor->nUndoStackSize--;
      }
      /* any new operation (not redo) clears the redo stack */
      if (editor->nUndoMode == umAddToUndo) {
        ME_DisplayItem *p = editor->pRedoStack;
        while(p)
        {
          ME_DisplayItem *pp = p->next;
          ME_DestroyDisplayItem(p);
          p = pp;
        }
        editor->pRedoStack = NULL;
      }
    }
    else if (editor->nUndoMode == umAddToRedo)
    {
      TRACE("Pushing id=%s to redo stack\n", ME_GetDITypeName(type));
      pItem->next = editor->pRedoStack;
      if (editor->pRedoStack)
        editor->pRedoStack->prev = pItem;
      editor->pRedoStack = pItem;
    }
    else
      assert(0);
    return (ME_UndoItem *)pItem;
  }
}

void ME_CommitUndo(ME_TextEditor *editor) {
  if (editor->nUndoMode == umIgnore)
    return;

  assert(editor->nUndoMode == umAddToUndo);

  /* no transactions, no need to commit */
  if (!editor->pUndoStack)
    return;

  /* no need to commit empty transactions */
  if (editor->pUndoStack->type == diUndoEndTransaction)
    return;

  ME_AddUndoItem(editor, diUndoEndTransaction, NULL);
  ME_SendSelChange(editor);
}

static void ME_PlayUndoItem(ME_TextEditor *editor, ME_DisplayItem *pItem)
{
  ME_UndoItem *pUItem = (ME_UndoItem *)pItem;

  if (editor->nUndoMode == umIgnore)
    return;
  TRACE("Playing undo/redo item, id=%s\n", ME_GetDITypeName(pItem->type));

  switch(pItem->type)
  {
  case diUndoEndTransaction:
    assert(0);
  case diUndoSetParagraphFormat:
  {
    ME_Cursor tmp;
    ME_CursorFromCharOfs(editor, pItem->member.para.nCharOfs, &tmp);
    ME_SetParaFormat(editor, ME_FindItemBack(tmp.pRun, diParagraph), pItem->member.para.pFmt);
    break;
  }
  case diUndoSetCharFormat:
  {
    ME_SetCharFormat(editor, pUItem->nStart, pUItem->nLen, &pItem->member.ustyle->fmt);
    break;
  }
  case diUndoSetDefaultCharFormat:
  {
    ME_SetDefaultCharFormat(editor, &pItem->member.ustyle->fmt);
    break;
  }
  case diUndoInsertRun:
  {
    ME_InsertRun(editor, pItem->member.run.nCharOfs, pItem);
    break;
  }
  case diUndoDeleteRun:
  {
    ME_InternalDeleteText(editor, pUItem->nStart, pUItem->nLen);
    break;
  }
  case diUndoJoinParagraphs:
  {
    ME_Cursor tmp;
    ME_CursorFromCharOfs(editor, pUItem->nStart, &tmp);
    /* the only thing that's needed is paragraph offset, so no need to split runs */
    ME_JoinParagraphs(editor, ME_GetParagraph(tmp.pRun));
    break;
  }
  case diUndoSplitParagraph:
  {
    ME_Cursor tmp;
    ME_DisplayItem *new_para;
    ME_CursorFromCharOfs(editor, pUItem->nStart, &tmp);
    if (tmp.nOffset)
      tmp.pRun = ME_SplitRunSimple(editor, tmp.pRun, tmp.nOffset);
    new_para = ME_SplitParagraph(editor, tmp.pRun, tmp.pRun->member.run.style);
    assert(pItem->member.para.pFmt->cbSize == sizeof(PARAFORMAT2));
    CopyMemory(new_para->member.para.pFmt, pItem->member.para.pFmt, sizeof(PARAFORMAT2));
    break;
  }
  default:
    assert(0 == "PlayUndoItem, unexpected type");
  }
}

void ME_Undo(ME_TextEditor *editor) {
  ME_DisplayItem *p;
  ME_UndoMode nMode = editor->nUndoMode;

  if (editor->nUndoMode == umIgnore)
    return;
  assert(nMode == umAddToUndo || nMode == umIgnore);

  /* no undo items ? */
  if (!editor->pUndoStack)
    return;

  /* watch out for uncommited transactions ! */
  assert(editor->pUndoStack->type == diUndoEndTransaction);

  editor->nUndoMode = umAddToRedo;
  p = editor->pUndoStack->next;
  ME_DestroyDisplayItem(editor->pUndoStack);
  do {
    ME_DisplayItem *pp = p;
    ME_PlayUndoItem(editor, p);
    p = p->next;
    ME_DestroyDisplayItem(pp);
  } while(p && p->type != diUndoEndTransaction);
  ME_AddUndoItem(editor, diUndoEndTransaction, NULL);
  editor->pUndoStack = p;
  editor->nUndoStackSize--;
  if (p)
    p->prev = NULL;
  editor->nUndoMode = nMode;
  ME_UpdateRepaint(editor);
}

void ME_Redo(ME_TextEditor *editor) {
  ME_DisplayItem *p;
  ME_UndoMode nMode = editor->nUndoMode;

  assert(nMode == umAddToUndo || nMode == umIgnore);

  if (editor->nUndoMode == umIgnore)
    return;
  /* no redo items ? */
  if (!editor->pRedoStack)
    return;

  /* watch out for uncommited transactions ! */
  assert(editor->pRedoStack->type == diUndoEndTransaction);

  editor->nUndoMode = umAddBackToUndo;
  p = editor->pRedoStack->next;
  ME_DestroyDisplayItem(editor->pRedoStack);
  do {
    ME_DisplayItem *pp = p;
    ME_PlayUndoItem(editor, p);
    p = p->next;
    ME_DestroyDisplayItem(pp);
  } while(p && p->type != diUndoEndTransaction);
  ME_AddUndoItem(editor, diUndoEndTransaction, NULL);
  editor->pRedoStack = p;
  if (p)
    p->prev = NULL;
  editor->nUndoMode = nMode;
  ME_UpdateRepaint(editor);
}
