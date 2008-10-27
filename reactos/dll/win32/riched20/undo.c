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

ME_UndoItem *ME_AddUndoItem(ME_TextEditor *editor, ME_DIType type, const ME_DisplayItem *pdi) {
  if (editor->nUndoMode == umIgnore)
    return NULL;
  else if (editor->nUndoLimit == 0)
    return NULL;
  else
  {
    ME_DisplayItem *pItem = (ME_DisplayItem *)ALLOC_OBJ(ME_UndoItem);
    ((ME_UndoItem *)pItem)->nCR = ((ME_UndoItem *)pItem)->nLF = -1;
    switch(type)
    {
    case diUndoPotentialEndTransaction:
        /* only should be added for manually typed chars, not undos or redos */
        assert(editor->nUndoMode == umAddToUndo);
        /* intentional fall-through to next case */
    case diUndoEndTransaction:
      break;
    case diUndoSetParagraphFormat:
      assert(pdi);
      pItem->member.para = pdi->member.para;
      pItem->member.para.pFmt = ALLOC_OBJ(PARAFORMAT2);
      *pItem->member.para.pFmt = *pdi->member.para.pFmt;
      break;
    case diUndoInsertRun:
      assert(pdi);
      pItem->member.run = pdi->member.run;
      pItem->member.run.strText = ME_StrDup(pItem->member.run.strText);
      ME_AddRefStyle(pItem->member.run.style);
      if (pdi->member.run.ole_obj)
      {
        pItem->member.run.ole_obj = ALLOC_OBJ(*pItem->member.run.ole_obj);
        ME_CopyReObject(pItem->member.run.ole_obj, pdi->member.run.ole_obj);
      }
      else pItem->member.run.ole_obj = NULL;
      break;
    case diUndoSetCharFormat:
      break;
    case diUndoDeleteRun:
    case diUndoJoinParagraphs:
      break;
    case diUndoSplitParagraph:
    {
      ME_DisplayItem *prev_para = pdi->member.para.prev_para;
      assert(pdi->member.para.pFmt->cbSize == sizeof(PARAFORMAT2));
      pItem->member.para.pFmt = ALLOC_OBJ(PARAFORMAT2);
      pItem->member.para.pFmt->cbSize = sizeof(PARAFORMAT2);
      pItem->member.para.pFmt->dwMask = 0;
      *pItem->member.para.pFmt = *pdi->member.para.pFmt;
      pItem->member.para.border = pdi->member.para.border;
      pItem->member.para.nFlags = prev_para->member.para.nFlags & ~MEPF_CELL;
      pItem->member.para.pCell = NULL;
      break;
    }
    default:
      assert(0 == "AddUndoItem, unsupported item type");
      return NULL;
    }
    pItem->type = type;
    pItem->prev = NULL;
    if (editor->nUndoMode == umAddToUndo || editor->nUndoMode == umAddBackToUndo)
    {
      if (editor->pUndoStack
          && editor->pUndoStack->type == diUndoPotentialEndTransaction)
      {
          editor->pUndoStack->type = diUndoEndTransaction;
      }
      if (editor->nUndoMode == umAddToUndo)
        TRACE("Pushing id=%s to undo stack, deleting redo stack\n", ME_GetDITypeName(type));
      else
        TRACE("Pushing id=%s to undo stack\n", ME_GetDITypeName(type));

      pItem->next = editor->pUndoStack;
      if (type == diUndoEndTransaction || type == diUndoPotentialEndTransaction)
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

/**
 * Commits preceding changes into a transaction that can be undone together.
 *
 * This should be called after all the changes occur associated with an event
 * so that the group of changes can be undone atomically as a transaction.
 *
 * This will have no effect the undo mode is set to ignore changes, or if no
 * changes preceded calling this function before the last time it was called.
 *
 * This can also be used to conclude a coalescing transaction (used for grouping
 * typed characters).
 */
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
    
  if (editor->pUndoStack->type == diUndoPotentialEndTransaction)
  {
      /* Previous transaction was as a result of characters typed,
       * so the end of this transaction is confirmed. */
      editor->pUndoStack->type = diUndoEndTransaction;
      return;
  }

  ME_AddUndoItem(editor, diUndoEndTransaction, NULL);
  ME_SendSelChange(editor);
}

/**
 * Groups supsequent changes with previous ones for an undo if coalescing.
 *
 * Has no effect if the previous changes were followed by a ME_CommitUndo. This
 * function will only have an affect if the previous changes were followed by
 * a call to ME_CommitCoalescingUndo, which allows the transaction to be
 * continued.
 *
 * This allows multiple consecutively typed characters to be grouped together
 * to be undone by a single undo operation.
 */
void ME_ContinueCoalescingTransaction(ME_TextEditor *editor)
{
  ME_DisplayItem* p;

  if (editor->nUndoMode == umIgnore)
    return;

  assert(editor->nUndoMode == umAddToUndo);

  p = editor->pUndoStack;

  if (p && p->type == diUndoPotentialEndTransaction) {
    assert(p->next); /* EndTransactions shouldn't be at bottom of undo stack */
    editor->pUndoStack = p->next;
    editor->pUndoStack->prev = NULL;
    editor->nUndoStackSize--;
    ME_DestroyDisplayItem(p);
  }
}

/**
 * Commits preceding changes into a undo transaction that can be expanded.
 *
 * This function allows the transaction to be reopened with
 * ME_ContinueCoalescingTransaction in order to continue the transaction.  If an
 * undo item is added to the undo stack as a result of a change without the
 * transaction being reopened, then the transaction will be ended, and the
 * changes will become a part of the next transaction.
 *
 * This is used to allow typed characters to be grouped together since each
 * typed character results in a single event, and each event adding undo items
 * must be committed.  Using this function as opposed to ME_CommitUndo allows
 * multiple events to be grouped, and undone together.
 */
void ME_CommitCoalescingUndo(ME_TextEditor *editor)
{
  if (editor->nUndoMode == umIgnore)
    return;

  assert(editor->nUndoMode == umAddToUndo);

  /* no transactions, no need to commit */
  if (!editor->pUndoStack)
    return;

  /* no need to commit empty transactions */
  if (editor->pUndoStack->type == diUndoEndTransaction)
    return;
  if (editor->pUndoStack->type == diUndoPotentialEndTransaction)
    return;

  ME_AddUndoItem(editor, diUndoPotentialEndTransaction, NULL);
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
  case diUndoPotentialEndTransaction:
  case diUndoEndTransaction:
    assert(0);
  case diUndoSetParagraphFormat:
  {
    ME_Cursor tmp;
    ME_DisplayItem *para;
    ME_CursorFromCharOfs(editor, pItem->member.para.nCharOfs, &tmp);
    para = ME_FindItemBack(tmp.pRun, diParagraph);
    ME_AddUndoItem(editor, diUndoSetParagraphFormat, para);
    *para->member.para.pFmt = *pItem->member.para.pFmt;
    para->member.para.border = pItem->member.para.border;
    break;
  }
  case diUndoSetCharFormat:
  {
    ME_SetCharFormat(editor, pUItem->nStart, pUItem->nLen, &pItem->member.ustyle->fmt);
    break;
  }
  case diUndoInsertRun:
  {
    ME_InsertRun(editor, pItem->member.run.nCharOfs, pItem);
    break;
  }
  case diUndoDeleteRun:
  {
    ME_InternalDeleteText(editor, pUItem->nStart, pUItem->nLen, TRUE);
    break;
  }
  case diUndoJoinParagraphs:
  {
    ME_Cursor tmp;
    ME_CursorFromCharOfs(editor, pUItem->nStart, &tmp);
    /* the only thing that's needed is paragraph offset, so no need to split runs */
    ME_JoinParagraphs(editor, ME_GetParagraph(tmp.pRun), TRUE);
    break;
  }
  case diUndoSplitParagraph:
  {
    ME_Cursor tmp;
    ME_DisplayItem *this_para, *new_para;
    BOOL bFixRowStart;
    int paraFlags = pItem->member.para.nFlags & (MEPF_ROWSTART|MEPF_CELL|MEPF_ROWEND);
    ME_CursorFromCharOfs(editor, pUItem->nStart, &tmp);
    if (tmp.nOffset)
      tmp.pRun = ME_SplitRunSimple(editor, tmp.pRun, tmp.nOffset);
    assert(pUItem->nCR >= 0);
    assert(pUItem->nLF >= 0);
    this_para = ME_GetParagraph(tmp.pRun);
    bFixRowStart = this_para->member.para.nFlags & MEPF_ROWSTART;
    if (bFixRowStart)
    {
      /* Re-insert the paragraph before the table, making sure the nFlag value
       * is correct. */
      this_para->member.para.nFlags &= ~MEPF_ROWSTART;
    }
    new_para = ME_SplitParagraph(editor, tmp.pRun, tmp.pRun->member.run.style,
                                 pUItem->nCR, pUItem->nLF, paraFlags);
    if (bFixRowStart)
      new_para->member.para.nFlags |= MEPF_ROWSTART;
    assert(pItem->member.para.pFmt->cbSize == sizeof(PARAFORMAT2));
    *new_para->member.para.pFmt = *pItem->member.para.pFmt;
    new_para->member.para.border = pItem->member.para.border;
    if (pItem->member.para.pCell)
    {
      ME_DisplayItem *pItemCell, *pCell;
      pItemCell = pItem->member.para.pCell;
      pCell = new_para->member.para.pCell;
      pCell->member.cell.nRightBoundary = pItemCell->member.cell.nRightBoundary;
      pCell->member.cell.border = pItemCell->member.cell.border;
    }
    break;
  }
  default:
    assert(0 == "PlayUndoItem, unexpected type");
  }
}

BOOL ME_Undo(ME_TextEditor *editor) {
  ME_DisplayItem *p;
  ME_UndoMode nMode = editor->nUndoMode;
  
  if (editor->nUndoMode == umIgnore)
    return FALSE;
  assert(nMode == umAddToUndo || nMode == umIgnore);
  
  /* no undo items ? */
  if (!editor->pUndoStack)
    return FALSE;
    
  /* watch out for uncommitted transactions ! */
  assert(editor->pUndoStack->type == diUndoEndTransaction
        || editor->pUndoStack->type == diUndoPotentialEndTransaction);
  
  editor->nUndoMode = umAddToRedo;
  p = editor->pUndoStack->next;
  ME_DestroyDisplayItem(editor->pUndoStack);
  editor->pUndoStack = p;
  do {
    p->prev = NULL;
    ME_PlayUndoItem(editor, p);
    editor->pUndoStack = p->next;
    ME_DestroyDisplayItem(p);
    p = editor->pUndoStack;
  } while(p && p->type != diUndoEndTransaction);
  if (p)
    p->prev = NULL;
  ME_MoveCursorFromTableRowStartParagraph(editor);
  ME_AddUndoItem(editor, diUndoEndTransaction, NULL);
  ME_CheckTablesForCorruption(editor);
  editor->nUndoStackSize--;
  editor->nUndoMode = nMode;
  ME_UpdateRepaint(editor);
  return TRUE;
}

BOOL ME_Redo(ME_TextEditor *editor) {
  ME_DisplayItem *p;
  ME_UndoMode nMode = editor->nUndoMode;
  
  assert(nMode == umAddToUndo || nMode == umIgnore);
  
  if (editor->nUndoMode == umIgnore)
    return FALSE;
  /* no redo items ? */
  if (!editor->pRedoStack)
    return FALSE;
    
  /* watch out for uncommitted transactions ! */
  assert(editor->pRedoStack->type == diUndoEndTransaction);
  
  editor->nUndoMode = umAddBackToUndo;
  p = editor->pRedoStack->next;
  ME_DestroyDisplayItem(editor->pRedoStack);
  editor->pRedoStack = p;
  do {
    p->prev = NULL;
    ME_PlayUndoItem(editor, p);
    editor->pRedoStack = p->next;
    ME_DestroyDisplayItem(p);
    p = editor->pRedoStack;
  } while(p && p->type != diUndoEndTransaction);
  if (p)
    p->prev = NULL;
  ME_MoveCursorFromTableRowStartParagraph(editor);
  ME_AddUndoItem(editor, diUndoEndTransaction, NULL);
  ME_CheckTablesForCorruption(editor);
  editor->nUndoMode = nMode;
  ME_UpdateRepaint(editor);
  return TRUE;
}
