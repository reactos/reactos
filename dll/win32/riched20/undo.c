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

static void destroy_undo_item( struct undo_item *undo )
{
    switch( undo->type )
    {
    case undo_insert_run:
        free( undo->u.insert_run.str );
        ME_ReleaseStyle( undo->u.insert_run.style );
        break;
    case undo_split_para:
        ME_DestroyString( undo->u.split_para.eol_str );
        break;
    default:
        break;
    }

    free( undo );
}

static void empty_redo_stack(ME_TextEditor *editor)
{
    struct undo_item *cursor, *cursor2;
    LIST_FOR_EACH_ENTRY_SAFE( cursor, cursor2, &editor->redo_stack, struct undo_item, entry )
    {
        list_remove( &cursor->entry );
        destroy_undo_item( cursor );
    }
}

void ME_EmptyUndoStack(ME_TextEditor *editor)
{
  struct undo_item *cursor, *cursor2;
  if (editor->nUndoMode == umIgnore)  /* NOTE don't use editor_undo_ignored() here! */
    return;
  
  TRACE("Emptying undo stack\n");

  editor->nUndoStackSize = 0;

  LIST_FOR_EACH_ENTRY_SAFE( cursor, cursor2, &editor->undo_stack, struct undo_item, entry )
  {
      list_remove( &cursor->entry );
      destroy_undo_item( cursor );
  }

  empty_redo_stack( editor );
}

static struct undo_item *add_undo( ME_TextEditor *editor, enum undo_type type )
{
    struct undo_item *undo, *item;
    struct list *head;

    if (editor_undo_ignored(editor)) return NULL;
    if (editor->nUndoLimit == 0) return NULL;

    undo = malloc( sizeof(*undo) );
    if (!undo) return NULL;
    undo->type = type;

    if (editor->nUndoMode == umAddToUndo || editor->nUndoMode == umAddBackToUndo)
    {

        head = list_head( &editor->undo_stack );
        if (head)
        {
            item = LIST_ENTRY( head, struct undo_item, entry );
            if (item->type == undo_potential_end_transaction)
                item->type = undo_end_transaction;
        }

        if (editor->nUndoMode == umAddToUndo)
            TRACE("Pushing id=%d to undo stack, deleting redo stack\n", type);
        else
            TRACE("Pushing id=%d to undo stack\n", type);

        list_add_head( &editor->undo_stack, &undo->entry );

        if (type == undo_end_transaction || type == undo_potential_end_transaction)
            editor->nUndoStackSize++;

        if (editor->nUndoStackSize > editor->nUndoLimit)
        {
            struct undo_item *cursor2;
            /* remove oldest undo from stack */
            LIST_FOR_EACH_ENTRY_SAFE_REV( item, cursor2, &editor->undo_stack, struct undo_item, entry )
            {
                BOOL done = (item->type == undo_end_transaction);
                list_remove( &item->entry );
                destroy_undo_item( item );
                if (done) break;
            }
            editor->nUndoStackSize--;
        }

        /* any new operation (not redo) clears the redo stack */
        if (editor->nUndoMode == umAddToUndo) empty_redo_stack( editor );
    }
    else if (editor->nUndoMode == umAddToRedo)
    {
        TRACE("Pushing id=%d to redo stack\n", type);
        list_add_head( &editor->redo_stack, &undo->entry );
    }

    return undo;
}

BOOL add_undo_insert_run( ME_TextEditor *editor, int pos, const WCHAR *str, int len, int flags, ME_Style *style )
{
    struct undo_item *undo = add_undo( editor, undo_insert_run );
    if (!undo) return FALSE;

    undo->u.insert_run.str = malloc( (len + 1) * sizeof(WCHAR) );
    if (!undo->u.insert_run.str)
    {
        ME_EmptyUndoStack( editor );
        return FALSE;
    }
    memcpy( undo->u.insert_run.str, str, len * sizeof(WCHAR) );
    undo->u.insert_run.str[len] = 0;
    undo->u.insert_run.pos = pos;
    undo->u.insert_run.len = len;
    undo->u.insert_run.flags = flags;
    undo->u.insert_run.style = style;
    ME_AddRefStyle( style );
    return TRUE;
}

BOOL add_undo_set_para_fmt( ME_TextEditor *editor, const ME_Paragraph *para )
{
    struct undo_item *undo = add_undo( editor, undo_set_para_fmt );
    if (!undo) return FALSE;

    undo->u.set_para_fmt.pos = para->nCharOfs;
    undo->u.set_para_fmt.fmt = para->fmt;
    undo->u.set_para_fmt.border = para->border;

    return TRUE;
}

BOOL add_undo_set_char_fmt( ME_TextEditor *editor, int pos, int len, const CHARFORMAT2W *fmt )
{
    struct undo_item *undo = add_undo( editor, undo_set_char_fmt );
    if (!undo) return FALSE;

    undo->u.set_char_fmt.pos = pos;
    undo->u.set_char_fmt.len = len;
    undo->u.set_char_fmt.fmt = *fmt;

    return TRUE;
}

BOOL add_undo_join_paras( ME_TextEditor *editor, int pos )
{
    struct undo_item *undo = add_undo( editor, undo_join_paras );
    if (!undo) return FALSE;

    undo->u.join_paras.pos = pos;
    return TRUE;
}

BOOL add_undo_split_para( ME_TextEditor *editor, const ME_Paragraph *para, ME_String *eol_str, const ME_Cell *cell )
{
    struct undo_item *undo = add_undo( editor, undo_split_para );
    if (!undo) return FALSE;

    undo->u.split_para.pos = para->nCharOfs - eol_str->nLen;
    undo->u.split_para.eol_str = eol_str;
    undo->u.split_para.fmt = para->fmt;
    undo->u.split_para.border = para->border;
    undo->u.split_para.flags = para->prev_para->member.para.nFlags & ~MEPF_CELL;

    if (cell)
    {
        undo->u.split_para.cell_border = cell->border;
        undo->u.split_para.cell_right_boundary = cell->nRightBoundary;
    }
    return TRUE;
}

BOOL add_undo_delete_run( ME_TextEditor *editor, int pos, int len )
{
    struct undo_item *undo = add_undo( editor, undo_delete_run );
    if (!undo) return FALSE;

    undo->u.delete_run.pos = pos;
    undo->u.delete_run.len = len;

    return TRUE;
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
void ME_CommitUndo(ME_TextEditor *editor)
{
  struct undo_item *item;
  struct list *head;

  if (editor_undo_ignored(editor))
    return;
  
  assert(editor->nUndoMode == umAddToUndo);
  
  /* no transactions, no need to commit */
  head = list_head( &editor->undo_stack );
  if (!head) return;

  /* no need to commit empty transactions */
  item = LIST_ENTRY( head, struct undo_item, entry );
  if (item->type == undo_end_transaction) return;

  if (item->type == undo_potential_end_transaction)
  {
      item->type = undo_end_transaction;
      return;
  }

  add_undo( editor, undo_end_transaction );
}

/**
 * Groups subsequent changes with previous ones for an undo if coalescing.
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
  struct undo_item *item;
  struct list *head;

  if (editor_undo_ignored(editor))
    return;

  assert(editor->nUndoMode == umAddToUndo);

  head = list_head( &editor->undo_stack );
  if (!head) return;

  item = LIST_ENTRY( head, struct undo_item, entry );
  if (item->type == undo_potential_end_transaction)
  {
    list_remove( &item->entry );
    editor->nUndoStackSize--;
    destroy_undo_item( item );
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
  struct undo_item *item;
  struct list *head;

  if (editor_undo_ignored(editor))
    return;

  assert(editor->nUndoMode == umAddToUndo);

  head = list_head( &editor->undo_stack );
  if (!head) return;

  /* no need to commit empty transactions */
  item = LIST_ENTRY( head, struct undo_item, entry );
  if (item->type == undo_end_transaction ||
      item->type == undo_potential_end_transaction)
      return;

  add_undo( editor, undo_potential_end_transaction );
}

static void ME_PlayUndoItem(ME_TextEditor *editor, struct undo_item *undo)
{

  if (editor_undo_ignored(editor))
    return;
  TRACE("Playing undo/redo item, id=%d\n", undo->type);

  switch(undo->type)
  {
  case undo_potential_end_transaction:
  case undo_end_transaction:
    assert(0);
  case undo_set_para_fmt:
  {
    ME_Cursor tmp;
    cursor_from_char_ofs( editor, undo->u.set_para_fmt.pos, &tmp );
    add_undo_set_para_fmt( editor, tmp.para );
    tmp.para->fmt = undo->u.set_para_fmt.fmt;
    tmp.para->border = undo->u.set_para_fmt.border;
    para_mark_rewrap( editor, tmp.para );
    break;
  }
  case undo_set_char_fmt:
  {
    ME_Cursor start, end;
    cursor_from_char_ofs( editor, undo->u.set_char_fmt.pos, &start );
    end = start;
    ME_MoveCursorChars(editor, &end, undo->u.set_char_fmt.len, FALSE);
    ME_SetCharFormat(editor, &start, &end, &undo->u.set_char_fmt.fmt);
    break;
  }
  case undo_insert_run:
  {
    ME_Cursor tmp;
    cursor_from_char_ofs( editor, undo->u.insert_run.pos, &tmp );
    run_insert( editor, &tmp, undo->u.insert_run.style,
                undo->u.insert_run.str, undo->u.insert_run.len,
                undo->u.insert_run.flags );
    break;
  }
  case undo_delete_run:
  {
    ME_Cursor tmp;
    cursor_from_char_ofs( editor, undo->u.delete_run.pos, &tmp );
    ME_InternalDeleteText(editor, &tmp, undo->u.delete_run.len, TRUE);
    break;
  }
  case undo_join_paras:
  {
    ME_Cursor tmp;
    cursor_from_char_ofs( editor, undo->u.join_paras.pos, &tmp );
    para_join( editor, tmp.para, TRUE );
    break;
  }
  case undo_split_para:
  {
    ME_Cursor tmp;
    ME_Paragraph *this_para, *new_para;
    BOOL bFixRowStart;
    int paraFlags = undo->u.split_para.flags & (MEPF_ROWSTART|MEPF_CELL|MEPF_ROWEND);

    cursor_from_char_ofs( editor, undo->u.split_para.pos, &tmp );
    if (tmp.nOffset) run_split( editor, &tmp );
    this_para = tmp.para;
    bFixRowStart = this_para->nFlags & MEPF_ROWSTART;
    if (bFixRowStart)
    {
      /* Re-insert the paragraph before the table, making sure the nFlag value
       * is correct. */
      this_para->nFlags &= ~MEPF_ROWSTART;
    }
    new_para = para_split( editor, tmp.run, tmp.run->style,
                           undo->u.split_para.eol_str->szData, undo->u.split_para.eol_str->nLen, paraFlags );
    if (bFixRowStart)
      new_para->nFlags |= MEPF_ROWSTART;
    new_para->fmt = undo->u.split_para.fmt;
    new_para->border = undo->u.split_para.border;
    if (paraFlags)
    {
      para_cell( new_para )->nRightBoundary = undo->u.split_para.cell_right_boundary;
      para_cell( new_para )->border = undo->u.split_para.cell_border;
    }
    break;
  }
  }
}

BOOL ME_Undo(ME_TextEditor *editor)
{
  ME_UndoMode nMode = editor->nUndoMode;
  struct list *head;
  struct undo_item *undo, *cursor2;

  if (editor_undo_ignored(editor)) return FALSE;
  assert(nMode == umAddToUndo || nMode == umIgnore);

  head = list_head( &editor->undo_stack );
  if (!head) return FALSE;

  /* watch out for uncommitted transactions ! */
  undo = LIST_ENTRY( head, struct undo_item, entry );
  assert(undo->type == undo_end_transaction
        || undo->type == undo_potential_end_transaction);

  editor->nUndoMode = umAddToRedo;

  list_remove( &undo->entry );
  destroy_undo_item( undo );

  LIST_FOR_EACH_ENTRY_SAFE( undo, cursor2, &editor->undo_stack, struct undo_item, entry )
  {
      if (undo->type == undo_end_transaction) break;
      ME_PlayUndoItem( editor, undo );
      list_remove( &undo->entry );
      destroy_undo_item( undo );
  }

  table_move_from_row_start( editor );
  add_undo( editor, undo_end_transaction );
  editor->nUndoStackSize--;
  editor->nUndoMode = nMode;
  ME_UpdateRepaint(editor, FALSE);
  return TRUE;
}

BOOL ME_Redo(ME_TextEditor *editor)
{
  ME_UndoMode nMode = editor->nUndoMode;
  struct list *head;
  struct undo_item *undo, *cursor2;

  assert(nMode == umAddToUndo || nMode == umIgnore);
  
  if (editor_undo_ignored(editor)) return FALSE;

  head = list_head( &editor->redo_stack );
  if (!head) return FALSE;

  /* watch out for uncommitted transactions ! */
  undo = LIST_ENTRY( head, struct undo_item, entry );
  assert( undo->type == undo_end_transaction );

  editor->nUndoMode = umAddBackToUndo;
  list_remove( &undo->entry );
  destroy_undo_item( undo );

  LIST_FOR_EACH_ENTRY_SAFE( undo, cursor2, &editor->redo_stack, struct undo_item, entry )
  {
      if (undo->type == undo_end_transaction) break;
      ME_PlayUndoItem( editor, undo );
      list_remove( &undo->entry );
      destroy_undo_item( undo );
  }
  table_move_from_row_start( editor );
  add_undo( editor, undo_end_transaction );
  editor->nUndoMode = nMode;
  ME_UpdateRepaint(editor, FALSE);
  return TRUE;
}

void editor_disable_undo(ME_TextEditor *editor)
{
    ME_EmptyUndoStack(editor);
    editor->undo_ctl_state = undoDisabled;
}

void editor_enable_undo(ME_TextEditor *editor)
{
    if (editor->undo_ctl_state == undoDisabled)
    {
        editor->undo_ctl_state = undoActive;
    }
}
