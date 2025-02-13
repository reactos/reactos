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

ME_Row *row_next( ME_Row *row )
{
    ME_DisplayItem *item;

    item = ME_FindItemFwd( row_get_di( row ), diStartRowOrParagraphOrEnd );
    if (!item || item->type != diStartRow) return NULL;
    return &item->member.row;
}

ME_Row *row_next_all_paras( ME_Row *row )
{
    ME_DisplayItem *item;

    item = ME_FindItemFwd( row_get_di( row ), diStartRow );
    if (!item) return NULL;
    return &item->member.row;
}

ME_Row *row_prev_all_paras( ME_Row *row )
{
    ME_DisplayItem *item;

    item = ME_FindItemBack( row_get_di( row ), diStartRow );
    if (!item) return NULL;
    return &item->member.row;
}

ME_Run *row_first_run( ME_Row *row )
{
    ME_DisplayItem *item;

    item = ME_FindItemFwd( row_get_di( row ), diRunOrStartRow );
    assert( item->type == diRun );
    return &item->member.run;
}

ME_Run *row_next_run( ME_Row *row, ME_Run *run )
{
    ME_DisplayItem *item;

    assert( row == &ME_FindItemBack( run_get_di( run ), diStartRow )->member.row );

    item = ME_FindItemFwd( run_get_di( run ), diRunOrStartRow );
    if (!item || item->type == diStartRow) return NULL;
    return &item->member.run;
}

ME_Row *row_from_cursor( ME_Cursor *cursor )
{
    ME_DisplayItem *item;

    item = ME_FindItemBack( run_get_di( cursor->run ), diStartRow );
    return &item->member.row;
}

void row_first_cursor( ME_Row *row, ME_Cursor *cursor )
{
    ME_DisplayItem *item;

    item = ME_FindItemFwd( row_get_di( row ), diRun );
    cursor->run = &item->member.run;
    cursor->para = cursor->run->para;
    cursor->nOffset = 0;
}

void row_end_cursor( ME_Row *row, ME_Cursor *cursor, BOOL include_eop )
{
    ME_DisplayItem *item, *run;

    item = ME_FindItemFwd( row_get_di( row ), diStartRowOrParagraphOrEnd );
    run = ME_FindItemBack( item, diRun );
    cursor->run = &run->member.run;
    cursor->para = cursor->run->para;
    cursor->nOffset = (item->type == diStartRow || include_eop) ? cursor->run->len : 0;
}

ME_Paragraph *row_para( ME_Row *row )
{
    ME_Cursor cursor;

    row_first_cursor( row, &cursor );
    return cursor.para;
}

ME_Row *row_from_row_number( ME_TextEditor *editor, int row_num )
{
    ME_Paragraph *para = editor_first_para( editor );
    ME_Row *row;
    int count = 0;

    while (para_next( para ) && count + para->nRows <= row_num)
    {
        count += para->nRows;
        para = para_next( para );
    }
    if (!para_next( para )) return NULL;

    for (row = para_first_row( para ); row && count < row_num; count++)
        row = row_next( row );

    return row;
}


int row_number_from_char_ofs( ME_TextEditor *editor, int ofs )
{
    ME_Paragraph *para = editor_first_para( editor );
    ME_Row *row;
    ME_Cursor cursor;
    int row_num = 0;

    while (para_next( para ) && para_next( para )->nCharOfs <= ofs)
    {
        row_num += para->nRows;
        para = para_next( para );
    }

    if (para_next( para ))
    {
        for (row = para_first_row( para ); row; row = row_next( row ))
        {
            row_end_cursor( row, &cursor, TRUE );
            if (ME_GetCursorOfs( &cursor ) > ofs ) break;
            row_num++;
        }
    }

    return row_num;
}
