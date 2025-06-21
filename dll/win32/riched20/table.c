/*
 * RichEdit functions dealing with on tables
 *
 * Copyright 2008 by Dylan Smith
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

/*
 * The implementation of tables differs greatly between version 3.0
 * (in riched20.dll) and version 4.1 (in msftedit.dll) of richedit controls.
 * Currently Wine is not distinguishing between version 3.0 and version 4.1,
 * so v4.1 is assumed unless v1.0 is being emulated (i.e. riched32.dll is used).
 * If this lack of distinction causes a bug in a Windows application, then Wine
 * will need to start making this distinction.
 *
 * Richedit version 1.0 - 3.0:
 *   Tables are implemented in these versions using tabs at the end of cells,
 *   and tab stops to position the cells.  The paragraph format flag PFE_TABLE
 *   will indicate that the paragraph is a table row.  Note that in this
 *   implementation there is one paragraph per table row.
 *
 * Richedit version 4.1:
 *   Tables are implemented such that cells can contain multiple paragraphs,
 *   each with its own paragraph format, and cells may even contain tables
 *   nested within the cell.
 *
 *   There is also a paragraph at the start of each table row that contains
 *   the rows paragraph format (e.g. to change the row alignment to row), and a
 *   paragraph at the end of the table row with the PFE_TABLEROWDELIMITER flag
 *   set. The paragraphs at the start and end of the table row should always be
 *   empty, but should have a length of 2.
 *
 *   Wine implements this using display items (ME_DisplayItem) with a type of
 *   diCell.  These cell display items store the cell properties, and are
 *   inserted into the editors linked list before each cell, and at the end of
 *   the last cell. The cell display item for a cell comes before the paragraphs
 *   for the cell, but the last cell display item refers to no cell, so it is
 *   just a delimiter.
 */

#include "editor.h"
#include "rtf.h"

static ME_Paragraph* table_insert_end_para( ME_TextEditor *editor, ME_Cursor *cursor,
                                            const WCHAR *eol_str, int eol_len, int para_flags )
{
    ME_Style *style = style_get_insert_style( editor, cursor );
    ME_Paragraph *para;

    if (cursor->nOffset) run_split( editor, cursor );

    para = para_split( editor, cursor->run, style, eol_str, eol_len, para_flags );
    ME_ReleaseStyle( style );
    cursor->para = para;
    cursor->run = para_first_run( para );
    return para;
}

ME_Paragraph* table_insert_row_start( ME_TextEditor *editor, ME_Cursor *cursor )
{
    ME_Paragraph *para;

    para = table_insert_end_para( editor, cursor, L"\r\n", 2, MEPF_ROWSTART );
    return para_prev( para );
}

ME_Paragraph* table_insert_row_start_at_para( ME_TextEditor *editor, ME_Paragraph *para )
{
    ME_Paragraph *prev_para, *end_para, *start_row;
    ME_Cursor cursor;

    cursor.para = para;
    cursor.run = para_first_run( para );
    cursor.nOffset = 0;

    start_row = table_insert_row_start( editor, &cursor );

    end_para = para_next( editor->pCursors[0].para );
    prev_para = para_next( start_row );
    para = para_next( prev_para );

    while (para != end_para)
    {
        para->cell = para_cell( prev_para );
        para->nFlags |= MEPF_CELL;
        para->nFlags &= ~(MEPF_ROWSTART | MEPF_ROWEND);
        para->fmt.dwMask |= PFM_TABLE | PFM_TABLEROWDELIMITER;
        para->fmt.wEffects |= PFE_TABLE;
        para->fmt.wEffects &= ~PFE_TABLEROWDELIMITER;
        prev_para = para;
        para = para_next( para );
    }
    return start_row;
}

/* Inserts a diCell and starts a new paragraph for the next cell.
 *
 * Returns the first paragraph of the new cell. */
ME_Paragraph* table_insert_cell( ME_TextEditor *editor, ME_Cursor *cursor )
{
    WCHAR tab = '\t';

    return table_insert_end_para( editor, editor->pCursors, &tab, 1, MEPF_CELL );
}

ME_Paragraph* table_insert_row_end( ME_TextEditor *editor, ME_Cursor *cursor )
{
    ME_Paragraph *para;

    para = table_insert_end_para( editor, cursor, L"\r\n", 2, MEPF_ROWEND );
    return para_prev( para );
}

ME_Paragraph* table_row_end( ME_Paragraph *para )
{
  ME_Cell *cell;

  if (para->nFlags & MEPF_ROWEND) return para;
  if (para->nFlags & MEPF_ROWSTART) para = para_next( para );
  cell = para_cell( para );
  while (cell_next( cell ))
    cell = cell_next( cell );

  para = &ME_FindItemFwd( cell_get_di( cell ), diParagraph )->member.para;
  assert( para && para->nFlags & MEPF_ROWEND );
  return para;
}

ME_Paragraph* table_row_start( ME_Paragraph *para )
{
  ME_Cell *cell;

  if (para->nFlags & MEPF_ROWSTART) return para;
  if (para->nFlags & MEPF_ROWEND) para = para_prev( para );
  cell = para_cell( para );

  while (cell_prev( cell ))
    cell = cell_prev( cell );

  para = &ME_FindItemBack( cell_get_di( cell ), diParagraph )->member.para;
  assert( para && para->nFlags & MEPF_ROWSTART );
  return para;
}

ME_Paragraph* table_outer_para( ME_Paragraph *para )
{
  if (para->nFlags & MEPF_ROWEND) para = para_prev( para );
  while (para_cell( para ))
  {
    para = table_row_start( para );
    if (!para_cell( para )) break;
    para = &ME_FindItemBack( cell_get_di( para_cell( para ) ), diParagraph )->member.para;
  }
  return para;
}

ME_Cell *table_row_first_cell( ME_Paragraph *para )
{
    if (!para_in_table( para )) return NULL;

    para = para_next( table_row_start( para ) );
    return para_cell( para );
}

ME_Cell *table_row_end_cell( ME_Paragraph *para )
{
    if (!para_in_table( para )) return NULL;

    para = para_prev( table_row_end( para ));
    return cell_next( para_cell( para ) );
}

ME_Cell *cell_create( void )
{
    ME_DisplayItem *item = ME_MakeDI( diCell );
    return &item->member.cell;
}

ME_Cell *cell_next( ME_Cell *cell )
{
    return cell->next_cell;
}

ME_Cell *cell_prev( ME_Cell *cell )
{
    return cell->prev_cell;
}

ME_Paragraph *cell_first_para( ME_Cell *cell )
{
    return &ME_FindItemFwd( cell_get_di( cell ), diParagraph )->member.para;
}

ME_Paragraph *cell_end_para( ME_Cell *cell )
{
    ME_Cell *next = cell_next( cell );

    if (!next) return cell_first_para( cell ); /* End of row */

    return &ME_FindItemBack( cell_get_di( next ), diParagraph )->member.para;
}

/* Table rows should either be deleted completely or not at all. */
void table_protect_partial_deletion( ME_TextEditor *editor, ME_Cursor *c, int *num_chars )
{
  int start_ofs = ME_GetCursorOfs( c );
  ME_Cursor c2 = *c;
  ME_Paragraph *this_para = c->para, *end_para;

  ME_MoveCursorChars( editor, &c2, *num_chars, FALSE );
  end_para = c2.para;
  if (c2.run->nFlags & MERF_ENDPARA)
  {
    /* End offset might be in the middle of the end paragraph run.
     * If this is the case, then we need to use the next paragraph as the last
     * paragraphs.
     */
    int remaining = start_ofs + *num_chars - c2.run->nCharOfs - end_para->nCharOfs;
    if (remaining)
    {
      assert( remaining < c2.run->len );
      end_para = para_next( end_para );
    }
  }
  if (!editor->bEmulateVersion10) /* v4.1 */
  {
    if (para_cell( this_para ) != para_cell( end_para ) ||
        ((this_para->nFlags | end_para->nFlags) & (MEPF_ROWSTART | MEPF_ROWEND)))
    {
      while (this_para != end_para)
      {
        ME_Paragraph *next_para = para_next( this_para );
        BOOL truancate_del = FALSE;
        if (this_para->nFlags & MEPF_ROWSTART)
        {
          /* The following while loop assumes that next_para is MEPF_ROWSTART,
           * so moving back one paragraph lets it be processed as the start
           * of the row. */
          next_para = this_para;
          this_para = para_prev( this_para );
        }
        else if (para_cell( next_para) != para_cell( this_para ) || this_para->nFlags & MEPF_ROWEND)
        {
          /* Start of the deletion from after the start of the table row. */
          truancate_del = TRUE;
        }
        while (!truancate_del && next_para->nFlags & MEPF_ROWSTART)
        {
          next_para = para_next( table_row_end( next_para ) );
          if (next_para->nCharOfs > start_ofs + *num_chars)
          {
            /* End of deletion is not past the end of the table row. */
            next_para = para_next( this_para );
            /* Delete the end paragraph preceding the table row if the
             * preceding table row will be empty. */
            if (this_para->nCharOfs >= start_ofs) next_para = para_next( next_para );
            truancate_del = TRUE;
          }
          else this_para = para_prev( next_para );
        }
        if (truancate_del)
        {
          ME_Run *end_run = para_end_run( para_prev( next_para ) );
          int new_chars = next_para->nCharOfs - start_ofs - end_run->len;
          new_chars = max( new_chars, 0 );
          assert( new_chars <= *num_chars);
          *num_chars = new_chars;
          break;
        }
        this_para = next_para;
      }
    }
  }
  else /* v1.0 - 3.0 */
  {
    ME_Run *run;
    int chars_to_boundary;

    if ((this_para->nCharOfs != start_ofs || this_para == end_para) && para_in_table( this_para ))
    {
      run = c->run;
      /* Find the next tab or end paragraph to use as a delete boundary */
      while (!(run->nFlags & (MERF_TAB | MERF_ENDPARA)))
        run = run_next( run );
      chars_to_boundary = run->nCharOfs - c->run->nCharOfs - c->nOffset;
      *num_chars = min( *num_chars, chars_to_boundary );
    }
    else if (para_in_table( end_para ))
    {
      /* The deletion starts from before the row, so don't join it with
       * previous non-empty paragraphs. */
      ME_Paragraph *cur_para;
      run = NULL;
      if (start_ofs > this_para->nCharOfs)
      {
          cur_para = para_prev( end_para );
          run = para_end_run( cur_para );
      }
      if (!run)
      {
        cur_para = end_para;
        run = para_first_run( end_para );
      }
      if (run)
      {
        chars_to_boundary = cur_para->nCharOfs + run->nCharOfs - start_ofs;
        if (chars_to_boundary >= 0) *num_chars = min( *num_chars, chars_to_boundary );
      }
    }
    if (*num_chars < 0) *num_chars = 0;
  }
}

ME_Paragraph* table_append_row( ME_TextEditor *editor, ME_Paragraph *table_row )
{
  WCHAR endl = '\r', tab = '\t';
  ME_Run *run;
  int i;

  if (!editor->bEmulateVersion10) /* v4.1 */
  {
    ME_Cell *new_cell, *cell;
    ME_Paragraph *para, *prev_table_end, *new_row_start;

    cell = table_row_first_cell( table_row );
    prev_table_end = table_row_end( table_row );
    para = para_next( prev_table_end );
    run = para_first_run( para );
    editor->pCursors[0].para = para;
    editor->pCursors[0].run = run;
    editor->pCursors[0].nOffset = 0;
    editor->pCursors[1] = editor->pCursors[0];
    new_row_start = table_insert_row_start( editor, editor->pCursors );
    new_cell = table_row_first_cell( new_row_start );
    /* Copy cell properties */
    new_cell->nRightBoundary = cell->nRightBoundary;
    new_cell->border = cell->border;
    while (cell_next( cell ))
    {
      cell = cell_next( cell );
      para = table_insert_cell( editor, editor->pCursors );
      new_cell = para_cell( para );
      /* Copy cell properties */
      new_cell->nRightBoundary = cell->nRightBoundary;
      new_cell->border = cell->border;
    };
    para = table_insert_row_end( editor, editor->pCursors );
    para->fmt = prev_table_end->fmt;
    /* return the table row start for the inserted paragraph */
    return new_row_start;
  }
  else /* v1.0 - 3.0 */
  {
    run = para_end_run( table_row );
    assert( para_in_table( table_row ) );
    editor->pCursors[0].para = table_row;
    editor->pCursors[0].run = run;
    editor->pCursors[0].nOffset = 0;
    editor->pCursors[1] = editor->pCursors[0];
    ME_InsertTextFromCursor( editor, 0, &endl, 1, run->style );
    run = editor->pCursors[0].run;
    for (i = 0; i < table_row->fmt.cTabCount; i++)
      ME_InsertTextFromCursor( editor, 0, &tab, 1, run->style );

    return para_next( table_row );
  }
}

/* Selects the next table cell or appends a new table row if at end of table */
static void table_select_next_cell_or_append( ME_TextEditor *editor, ME_Run *run )
{
  ME_Paragraph *para = run->para;
  ME_Cell *cell;
  int i;

  assert( para_in_table( para ) );
  if (!editor->bEmulateVersion10) /* v4.1 */
  {
    /* Get the initial cell */
    if (para->nFlags & MEPF_ROWSTART) cell = para_cell( para_next( para ) );
    else if (para->nFlags & MEPF_ROWEND) cell = para_cell( para_prev( para ) );
    else cell = para_cell( para );

    /* Get the next cell. */
    if (cell_next( cell ) && cell_next( cell_next( cell ) ))
        cell = cell_next( cell );
    else
    {
      para = para_next( table_row_end( para ) );
      if (para->nFlags & MEPF_ROWSTART) cell = para_cell( para_next( para ) );
      else
      {
        /* Insert row */
        para = para_prev( para );
        para = table_append_row( editor, table_row_start( para ) );
        /* Put cursor at the start of the new table row */
        para = para_next( para );
        editor->pCursors[0].para = para;
        editor->pCursors[0].run = para_first_run( para );
        editor->pCursors[0].nOffset = 0;
        editor->pCursors[1] = editor->pCursors[0];
        ME_WrapMarkedParagraphs(editor);
        return;
      }
    }
    /* Select cell */
    editor->pCursors[1].para = cell_first_para( cell );
    editor->pCursors[1].run = para_first_run( editor->pCursors[1].para );
    editor->pCursors[1].nOffset = 0;
    editor->pCursors[0].para = cell_end_para( cell );
    editor->pCursors[0].run = para_end_run( editor->pCursors[0].para );
    editor->pCursors[0].nOffset = 0;
  }
  else /* v1.0 - 3.0 */
  {
    if (run->nFlags & MERF_ENDPARA && para_in_table( para_next( para ) ))
    {
      run = run_next_all_paras( run );
      assert(run);
    }
    for (i = 0; i < 2; i++)
    {
      while (!(run->nFlags & MERF_TAB))
      {
        if (!run_next( run ))
        {
          para = para_next( run->para );
          if (para_in_table( para ))
          {
            run = para_first_run( para );
            editor->pCursors[0].para = para;
            editor->pCursors[0].run = run;
            editor->pCursors[0].nOffset = 0;
            i = 1;
          }
          else
          {
            /* Insert table row */
            para = table_append_row( editor, para_prev( para ) );
            /* Put cursor at the start of the new table row */
            editor->pCursors[0].para = para;
            editor->pCursors[0].run = para_first_run( para );
            editor->pCursors[0].nOffset = 0;
            editor->pCursors[1] = editor->pCursors[0];
            ME_WrapMarkedParagraphs(editor);
            return;
          }
        }
        else run = run_next( run );
      }
      if (i == 0) run = run_next_all_paras( run );
      editor->pCursors[i].run = run;
      editor->pCursors[i].para = run->para;
      editor->pCursors[i].nOffset = 0;
    }
  }
}


void table_handle_tab( ME_TextEditor *editor, BOOL selected_row )
{
  /* FIXME: Shift tab should move to the previous cell. */
  ME_Cursor fromCursor, toCursor;
  ME_InvalidateSelection(editor);
  {
    int from, to;
    from = ME_GetCursorOfs(&editor->pCursors[0]);
    to = ME_GetCursorOfs(&editor->pCursors[1]);
    if (from <= to)
    {
      fromCursor = editor->pCursors[0];
      toCursor = editor->pCursors[1];
    }
    else
    {
      fromCursor = editor->pCursors[1];
      toCursor = editor->pCursors[0];
    }
  }
  if (!editor->bEmulateVersion10) /* v4.1 */
  {
    if (!para_in_table( toCursor.para ))
    {
      editor->pCursors[0] = toCursor;
      editor->pCursors[1] = toCursor;
    }
    else table_select_next_cell_or_append( editor, toCursor.run );
  }
  else /* v1.0 - 3.0 */
  {
    if (!para_in_table( fromCursor.para ))
    {
      editor->pCursors[0] = fromCursor;
      editor->pCursors[1] = fromCursor;
      /* FIXME: For some reason the caret is shown at the start of the
       *        previous paragraph in v1.0 to v3.0 */
    }
    else if ((selected_row || !para_in_table( toCursor.para )))
      table_select_next_cell_or_append( editor, fromCursor.run );
    else
    {
      ME_Run *run = run_prev( toCursor.run );

      if (ME_IsSelection(editor) && !toCursor.nOffset && run && run->nFlags & MERF_TAB)
        table_select_next_cell_or_append( editor, run );
      else
        table_select_next_cell_or_append( editor, toCursor.run );
    }
  }
  ME_InvalidateSelection(editor);
  ME_Repaint(editor);
  update_caret(editor);
  ME_SendSelChange(editor);
}

/* Make sure the cursor is not in the hidden table row start paragraph
 * without a selection. */
void table_move_from_row_start( ME_TextEditor *editor )
{
  ME_Paragraph *para = editor->pCursors[0].para;

  if (para == editor->pCursors[1].para && para->nFlags & MEPF_ROWSTART)
  {
    /* The cursors should not be at the hidden start row paragraph without
     * a selection, so the cursor is moved into the first cell. */
    para = para_next( para );
    editor->pCursors[0].para = para;
    editor->pCursors[0].run = para_first_run( para );
    editor->pCursors[0].nOffset = 0;
    editor->pCursors[1] = editor->pCursors[0];
  }
}

struct RTFTable *ME_MakeTableDef(ME_TextEditor *editor)
{
  RTFTable *tableDef = calloc(1, sizeof(*tableDef));

  if (!editor->bEmulateVersion10) /* v4.1 */
    tableDef->gapH = 10;
  return tableDef;
}

void ME_InitTableDef(ME_TextEditor *editor, struct RTFTable *tableDef)
{
  ZeroMemory(tableDef->cells, sizeof(tableDef->cells));
  ZeroMemory(tableDef->border, sizeof(tableDef->border));
  tableDef->numCellsDefined = 0;
  tableDef->leftEdge = 0;
  if (!editor->bEmulateVersion10) /* v4.1 */
    tableDef->gapH = 10;
  else /* v1.0 - 3.0 */
    tableDef->gapH = 0;
}
