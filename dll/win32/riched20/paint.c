/*
 * RichEdit - painting functions
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2005 by Phil Krylov
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

static void draw_paragraph( ME_Context *c, ME_Paragraph *para );

static inline BOOL editor_opaque( ME_TextEditor *editor )
{
    return editor->back_style != TXTBACK_TRANSPARENT;
}

void editor_draw( ME_TextEditor *editor, HDC hDC, const RECT *update )
{
  ME_Paragraph *para;
  ME_Context c;
  ME_Cell *cell;
  int ys, ye;
  HRGN oldRgn;
  RECT rc, client;
  HBRUSH brush = CreateSolidBrush( ITextHost_TxGetSysColor( editor->texthost, COLOR_WINDOW ) );

  ME_InitContext( &c, editor, hDC );
  if (!update)
  {
      client = c.rcView;
      client.left -= editor->selofs;
      update = &client;
  }

  oldRgn = CreateRectRgn(0, 0, 0, 0);
  if (!GetClipRgn(hDC, oldRgn))
  {
    DeleteObject(oldRgn);
    oldRgn = NULL;
  }
  IntersectClipRect( hDC, update->left, update->top, update->right, update->bottom );

  brush = SelectObject( hDC, brush );
  SetBkMode(hDC, TRANSPARENT);

  para = editor_first_para( editor );
  /* This context point is an offset for the paragraph positions stored
   * during wrapping. It shouldn't be modified during painting. */
  c.pt.x = c.rcView.left - editor->horz_si.nPos;
  c.pt.y = c.rcView.top - editor->vert_si.nPos;
  while (para_next( para ))
  {
    ys = c.pt.y + para->pt.y;
    cell = para_cell( para );
    if (cell && para == cell_end_para( cell ))
      ye = c.pt.y + cell->pt.y + cell->nHeight;
    else ye = ys + para->nHeight;

    if (cell && !(para->nFlags & MEPF_ROWEND) && para == cell_first_para( cell ))
    {
      /* the border shifts the text down */
      ys -= para_cell( para )->yTextOffset;
    }

    /* Draw the paragraph if any of the paragraph is in the update region. */
    if (ys < update->bottom && ye > update->top)
      draw_paragraph( &c, para );
    para = para_next( para );
  }
  if (editor_opaque( editor ))
  {
    if (c.pt.y + editor->nTotalLength < c.rcView.bottom)
    { /* space after the end of the text */
      rc.top = c.pt.y + editor->nTotalLength;
      rc.left = c.rcView.left;
      rc.bottom = c.rcView.bottom;
      rc.right = c.rcView.right;
      if (IntersectRect( &rc, &rc, update ))
        PatBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
    }
    if (editor->selofs)
    { /* selection bar */
      rc.left = c.rcView.left - editor->selofs;
      rc.top = c.rcView.top;
      rc.right = c.rcView.left;
      rc.bottom = c.rcView.bottom;
      if (IntersectRect( &rc, &rc, update ))
        PatBlt( hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
    }
  }

  DeleteObject( SelectObject( hDC, brush ) );
  SelectClipRgn( hDC, oldRgn );
  if (oldRgn) DeleteObject( oldRgn );
  ME_DestroyContext( &c );

  if (editor->in_place_active) {
    if (editor->nTotalLength != editor->nLastTotalLength || editor->nTotalWidth != editor->nLastTotalWidth)
      ME_SendRequestResize(editor, FALSE);
    editor->nLastTotalLength = editor->nTotalLength;
    editor->nLastTotalWidth = editor->nTotalWidth;
  }
}

void ME_Repaint(ME_TextEditor *editor)
{
  if (ME_WrapMarkedParagraphs(editor))
  {
    ME_UpdateScrollBar(editor);
    FIXME("ME_Repaint had to call ME_WrapMarkedParagraphs\n");
  }
  if (!editor->freeze_count)
    ITextHost_TxViewChange(editor->texthost, TRUE);
}

void ME_UpdateRepaint(ME_TextEditor *editor, BOOL update_now)
{
  /* Should be called whenever the contents of the control have changed */
  BOOL wrappedParagraphs;

  wrappedParagraphs = ME_WrapMarkedParagraphs(editor);
  if (wrappedParagraphs)
    ME_UpdateScrollBar(editor);

  /* Ensure that the cursor is visible */
  editor_ensure_visible( editor, &editor->pCursors[0] );

  update_caret( editor );

  if (!editor->freeze_count)
    ITextHost_TxViewChange(editor->texthost, update_now);

  ME_SendSelChange(editor);

  if(editor->nEventMask & ENM_CHANGE)
  {
    editor->nEventMask &= ~ENM_CHANGE;
    ITextHost_TxNotify( editor->texthost, EN_CHANGE, NULL );
    editor->nEventMask |= ENM_CHANGE;
  }
}

void
ME_RewrapRepaint(ME_TextEditor *editor)
{
  /* RewrapRepaint should be called whenever the control has changed in
   * looks, but not content. Like resizing. */
  
  editor_mark_rewrap_all( editor );
  ME_WrapMarkedParagraphs(editor);
  ME_UpdateScrollBar(editor);
  ME_Repaint(editor);
}

int ME_twips2pointsX(const ME_Context *c, int x)
{
  if (c->editor->nZoomNumerator == 0)
    return x * c->dpi.cx / 1440;
  else
    return x * c->dpi.cx * c->editor->nZoomNumerator / 1440 / c->editor->nZoomDenominator;
}

int ME_twips2pointsY(const ME_Context *c, int y)
{
  if (c->editor->nZoomNumerator == 0)
    return y * c->dpi.cy / 1440;
  else
    return y * c->dpi.cy * c->editor->nZoomNumerator / 1440 / c->editor->nZoomDenominator;
}


static int calc_y_offset( const ME_Context *c, ME_Style *style )
{
    int offs = 0, twips = 0;

    if ((style->fmt.dwMask & style->fmt.dwEffects) & CFM_OFFSET)
        twips = style->fmt.yOffset;

    if ((style->fmt.dwMask & style->fmt.dwEffects) & (CFM_SUPERSCRIPT | CFM_SUBSCRIPT))
    {
        if (style->fmt.dwEffects & CFE_SUPERSCRIPT) twips = style->fmt.yHeight/3;
        if (style->fmt.dwEffects & CFE_SUBSCRIPT) twips = -style->fmt.yHeight/12;
    }

    if (twips) offs = ME_twips2pointsY( c, twips );

    return offs;
}

static COLORREF get_text_color( ME_Context *c, ME_Style *style, BOOL highlight )
{
    COLORREF color;

    if (highlight)
        color = ITextHost_TxGetSysColor( c->editor->texthost, COLOR_HIGHLIGHTTEXT );
    else if ((style->fmt.dwMask & CFM_LINK) && (style->fmt.dwEffects & CFE_LINK))
        color = RGB(0,0,255);
    else if ((style->fmt.dwMask & CFM_COLOR) && (style->fmt.dwEffects & CFE_AUTOCOLOR))
        color = ITextHost_TxGetSysColor( c->editor->texthost, COLOR_WINDOWTEXT );
    else
        color = style->fmt.crTextColor;

    return color;
}

static COLORREF get_back_color( ME_Context *c, ME_Style *style, BOOL highlight )
{
    COLORREF color;

    if (highlight)
        color = ITextHost_TxGetSysColor( c->editor->texthost, COLOR_HIGHLIGHT );
    else if ( (style->fmt.dwMask & CFM_BACKCOLOR)
            && !(style->fmt.dwEffects & CFE_AUTOBACKCOLOR) )
        color = style->fmt.crBackColor;
    else
        color = ITextHost_TxGetSysColor( c->editor->texthost, COLOR_WINDOW );

    return color;
}

static HPEN get_underline_pen( ME_Style *style, COLORREF color )
{
    if (style->fmt.dwEffects & CFE_LINK)
        return CreatePen( PS_SOLID, 1, color );

    /* Choose the pen type for underlining the text. */
    if (style->fmt.dwEffects & CFE_UNDERLINE)
    {
        switch (style->fmt.bUnderlineType)
        {
        case CFU_UNDERLINE:
        case CFU_UNDERLINEWORD: /* native seems to map it to simple underline (MSDN) */
        case CFU_UNDERLINEDOUBLE: /* native seems to map it to simple underline (MSDN) */
            return CreatePen( PS_SOLID, 1, color );
        case CFU_UNDERLINEDOTTED:
            return CreatePen( PS_DOT, 1, color );
        default:
            FIXME( "Unknown underline type (%u)\n", style->fmt.bUnderlineType );
            /* fall through */
        case CFU_CF1UNDERLINE: /* this type is supported in the font, do nothing */
        case CFU_UNDERLINENONE:
            break;
        }
    }
    return NULL;
}

static void draw_underline( ME_Context *c, ME_Run *run, int x, int y, COLORREF color )
{
    HPEN pen;

    pen = get_underline_pen( run->style, color );
    if (pen)
    {
        HPEN old_pen = SelectObject( c->hDC, pen );
        MoveToEx( c->hDC, x, y + 1, NULL );
        LineTo( c->hDC, x + run->nWidth, y + 1 );
        SelectObject( c->hDC, old_pen );
        DeleteObject( pen );
    }
    return;
}

/*********************************************************************
 *  draw_space
 *
 * Draw the end-of-paragraph or tab space.
 *
 * If actually_draw is TRUE then ensure any underline is drawn.
 */
static void draw_space( ME_Context *c, ME_Run *run, int x, int y,
                        BOOL selected, BOOL actually_draw, int ymin, int cy )
{
    HDC hdc = c->hDC;
    BOOL old_style_selected = FALSE;
    RECT rect;
    COLORREF back_color = 0;

    SetRect( &rect, x, ymin, x + run->nWidth, ymin + cy );

    if (c->editor->bHideSelection || (!c->editor->bHaveFocus && (c->editor->props & TXTBIT_HIDESELECTION)))
        selected = FALSE;
    if (c->editor->bEmulateVersion10)
    {
        old_style_selected = selected;
        selected = FALSE;
    }

    if (selected)
        back_color = ITextHost_TxGetSysColor( c->editor->texthost, COLOR_HIGHLIGHT );

    if (actually_draw)
    {
        COLORREF text_color = get_text_color( c, run->style, selected );
        COLORREF old_text, old_back;
        int y_offset = calc_y_offset( c, run->style );
        static const WCHAR space[1] = {' '};

        select_style( c, run->style );
        old_text = SetTextColor( hdc, text_color );
        if (selected) old_back = SetBkColor( hdc, back_color );

        ExtTextOutW( hdc, x, y - y_offset, selected ? ETO_OPAQUE : 0, &rect, space, 1, &run->nWidth );

        if (selected) SetBkColor( hdc, old_back );
        SetTextColor( hdc, old_text );

        draw_underline( c, run, x, y - y_offset, text_color );
    }
    else if (selected)
    {
        HBRUSH brush = CreateSolidBrush( back_color );
        FillRect( hdc, &rect, brush );
        DeleteObject( brush );
    }

    if (old_style_selected)
        PatBlt( hdc, x, ymin, run->nWidth, cy, DSTINVERT );
}

static void get_selection_rect( ME_Context *c, ME_Run *run, int from, int to, int cy, RECT *r )
{
    from = max( 0, from );
    to = min( run->len, to );
    r->left = ME_PointFromCharContext( c, run, from, TRUE );
    r->top = 0;
    r->right = ME_PointFromCharContext( c, run, to, TRUE );
    r->bottom = cy;
    return;
}

static void draw_text( ME_Context *c, ME_Run *run, int x, int y, BOOL selected, RECT *sel_rect )
{
    COLORREF text_color = get_text_color( c, run->style, selected );
    COLORREF back_color = get_back_color( c, run->style, selected );
    COLORREF old_text, old_back = 0;
    const WCHAR *text = get_text( run, 0 );
    ME_String *masked = NULL;
    const BOOL paint_bg = ( selected
        || ( ( run->style->fmt.dwMask & CFM_BACKCOLOR )
            && !(CFE_AUTOBACKCOLOR & run->style->fmt.dwEffects) )
        );

    if (c->editor->password_char)
    {
        masked = ME_MakeStringR( c->editor->password_char, run->len );
        text = masked->szData;
    }

    old_text = SetTextColor( c->hDC, text_color );
    if (paint_bg) old_back = SetBkColor( c->hDC, back_color );

    if (run->para->nFlags & MEPF_COMPLEX)
        ScriptTextOut( c->hDC, &run->style->script_cache, x, y, paint_bg ? ETO_OPAQUE : 0, sel_rect,
                       &run->script_analysis, NULL, 0, run->glyphs, run->num_glyphs, run->advances,
                       NULL, run->offsets );
    else
        ExtTextOutW( c->hDC, x, y, paint_bg ? ETO_OPAQUE : 0, sel_rect, text, run->len, NULL );

    if (paint_bg) SetBkColor( c->hDC, old_back );
    SetTextColor( c->hDC, old_text );

    draw_underline( c, run, x, y, text_color );

    ME_DestroyString( masked );
    return;
}


static void draw_text_with_style( ME_Context *c, ME_Run *run, int x, int y,
                                  int nSelFrom, int nSelTo, int ymin, int cy )
{
  HDC hDC = c->hDC;
  int yOffset = 0;
  BOOL selected = (nSelFrom < run->len && nSelTo >= 0
                   && nSelFrom < nSelTo && !c->editor->bHideSelection &&
                   (c->editor->bHaveFocus || !(c->editor->props & TXTBIT_HIDESELECTION)));
  BOOL old_style_selected = FALSE;
  RECT sel_rect;
  HRGN clip = NULL, sel_rgn = NULL;

  yOffset = calc_y_offset( c, run->style );

  if (selected)
  {
    get_selection_rect( c, run, nSelFrom, nSelTo, cy, &sel_rect );
    OffsetRect( &sel_rect, x, ymin );

    if (c->editor->bEmulateVersion10)
    {
      old_style_selected = TRUE;
      selected = FALSE;
    }
    else
    {
      sel_rgn = CreateRectRgnIndirect( &sel_rect );
      clip = CreateRectRgn( 0, 0, 0, 0 );
      if (GetClipRgn( hDC, clip ) != 1)
      {
        DeleteObject( clip );
        clip = NULL;
      }
    }
  }

  select_style( c, run->style );

  if (sel_rgn) ExtSelectClipRgn( hDC, sel_rgn, RGN_DIFF );

  if (!(run->style->fmt.dwEffects & CFE_AUTOBACKCOLOR)
      && (run->style->fmt.dwMask & CFM_BACKCOLOR) )
  {
    RECT tmp_rect;
    get_selection_rect( c, run, 0, run->len, cy, &tmp_rect );
    OffsetRect( &tmp_rect, x, ymin );
    draw_text( c, run, x, y - yOffset, FALSE, &tmp_rect );
  }
  else
    draw_text( c, run, x, y - yOffset, FALSE, NULL );

  if (sel_rgn)
  {
    ExtSelectClipRgn( hDC, clip, RGN_COPY );
    ExtSelectClipRgn( hDC, sel_rgn, RGN_AND );
    draw_text( c, run, x, y - yOffset, TRUE, &sel_rect );
    ExtSelectClipRgn( hDC, clip, RGN_COPY );
    if (clip) DeleteObject( clip );
    DeleteObject( sel_rgn );
  }

  if (old_style_selected)
    PatBlt( hDC, sel_rect.left, ymin, sel_rect.right - sel_rect.left, cy, DSTINVERT );
}

static void ME_DebugWrite(HDC hDC, const POINT *pt, LPCWSTR szText) {
  int align = SetTextAlign(hDC, TA_LEFT|TA_TOP);
  HGDIOBJ hFont = SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
  COLORREF color = SetTextColor(hDC, RGB(128,128,128));
  TextOutW(hDC, pt->x, pt->y, szText, lstrlenW(szText));
  SelectObject(hDC, hFont);
  SetTextAlign(hDC, align);
  SetTextColor(hDC, color);
}

static void draw_run( ME_Context *c, int x, int y, ME_Cursor *cursor )
{
  ME_Row *row;
  ME_Run *run = cursor->run;
  int runofs = run_char_ofs( run, cursor->nOffset );
  LONG nSelFrom, nSelTo;

  if (run->nFlags & MERF_HIDDEN) return;

  row = row_from_cursor( cursor );
  ME_GetSelectionOfs(c->editor, &nSelFrom, &nSelTo);

  /* Draw selected end-of-paragraph mark */
  if (run->nFlags & MERF_ENDPARA)
  {
    if (runofs >= nSelFrom && runofs < nSelTo)
    {
      draw_space( c, run, x, y, TRUE, FALSE,
                  c->pt.y + run->para->pt.y + row->pt.y, row->nHeight );
    }
    return;
  }

  if (run->nFlags & (MERF_TAB | MERF_ENDCELL))
  {
    BOOL selected = runofs >= nSelFrom && runofs < nSelTo;

    draw_space( c, run, x, y, selected, TRUE,
                c->pt.y + run->para->pt.y + row->pt.y, row->nHeight );
    return;
  }

  if (run->nFlags & MERF_GRAPHICS)
    draw_ole( c, x, y, run, (runofs >= nSelFrom) && (runofs < nSelTo) );
  else
    draw_text_with_style( c, run, x, y, nSelFrom - runofs, nSelTo - runofs,
                          c->pt.y + run->para->pt.y + row->pt.y, row->nHeight );
}

/* The documented widths are in points (72 dpi), but converting them to
 * 96 dpi (standard display resolution) avoids dealing with fractions. */
static const struct {unsigned width : 8, pen_style : 4, dble : 1;} border_details[] = {
  /* none */            {0, PS_SOLID, FALSE},
  /* 3/4 */             {1, PS_SOLID, FALSE},
  /* 1 1/2 */           {2, PS_SOLID, FALSE},
  /* 2 1/4 */           {3, PS_SOLID, FALSE},
  /* 3 */               {4, PS_SOLID, FALSE},
  /* 4 1/2 */           {6, PS_SOLID, FALSE},
  /* 6 */               {8, PS_SOLID, FALSE},
  /* 3/4 double */      {1, PS_SOLID, TRUE},
  /* 1 1/2 double */    {2, PS_SOLID, TRUE},
  /* 2 1/4 double */    {3, PS_SOLID, TRUE},
  /* 3/4 gray */        {1, PS_DOT /* FIXME */, FALSE},
  /* 1 1/2 dashed */    {2, PS_DASH, FALSE},
};

static const COLORREF pen_colors[16] = {
  /* Black */           RGB(0x00, 0x00, 0x00),  /* Blue */            RGB(0x00, 0x00, 0xFF),
  /* Cyan */            RGB(0x00, 0xFF, 0xFF),  /* Green */           RGB(0x00, 0xFF, 0x00),
  /* Magenta */         RGB(0xFF, 0x00, 0xFF),  /* Red */             RGB(0xFF, 0x00, 0x00),
  /* Yellow */          RGB(0xFF, 0xFF, 0x00),  /* White */           RGB(0xFF, 0xFF, 0xFF),
  /* Dark blue */       RGB(0x00, 0x00, 0x80),  /* Dark cyan */       RGB(0x00, 0x80, 0x80),
  /* Dark green */      RGB(0x00, 0x80, 0x80),  /* Dark magenta */    RGB(0x80, 0x00, 0x80),
  /* Dark red */        RGB(0x80, 0x00, 0x00),  /* Dark yellow */     RGB(0x80, 0x80, 0x00),
  /* Dark gray */       RGB(0x80, 0x80, 0x80),  /* Light gray */      RGB(0xc0, 0xc0, 0xc0),
};

static int ME_GetBorderPenWidth(const ME_Context* c, int idx)
{
  int width = border_details[idx].width;

  if (c->dpi.cx != 96)
    width = MulDiv(width, c->dpi.cx, 96);

  if (c->editor->nZoomNumerator != 0)
    width = MulDiv(width, c->editor->nZoomNumerator, c->editor->nZoomDenominator);

  return width;
}

int ME_GetParaBorderWidth(const ME_Context* c, int flags)
{
  int idx = (flags >> 8) & 0xF;
  int width;

  if (idx >= ARRAY_SIZE(border_details))
  {
      FIXME("Unsupported border value %d\n", idx);
      return 0;
  }
  width = ME_GetBorderPenWidth(c, idx);
  if (border_details[idx].dble) width = width * 2 + 1;
  return width;
}

static void ME_DrawParaDecoration(ME_Context* c, ME_Paragraph* para, int y, RECT* bounds)
{
  int           idx, border_width, top_border, bottom_border;
  RECT          rc;
  BOOL          hasParaBorder;

  SetRectEmpty(bounds);
  if (!(para->fmt.dwMask & (PFM_BORDER | PFM_SPACEBEFORE | PFM_SPACEAFTER))) return;

  border_width = top_border = bottom_border = 0;
  idx = (para->fmt.wBorders >> 8) & 0xF;
  hasParaBorder = (!(c->editor->bEmulateVersion10 &&
                     para->fmt.dwMask & PFM_TABLE &&
                     para->fmt.wEffects & PFE_TABLE) &&
                   (para->fmt.dwMask & PFM_BORDER) &&
                    idx != 0 &&
                    (para->fmt.wBorders & 0xF));
  if (hasParaBorder)
  {
    /* FIXME: wBorders is not stored as MSDN says in v1.0 - 4.1 of richedit
     * controls. It actually stores the paragraph or row border style. Although
     * the value isn't used for drawing, it is used for streaming out rich text.
     *
     * wBorders stores the border style for each side (top, left, bottom, right)
     * using nibble (4 bits) to store each border style.  The rich text format
     * control words, and their associated value are the following:
     *   \brdrdash       0
     *   \brdrdashsm     1
     *   \brdrdb         2
     *   \brdrdot        3
     *   \brdrhair       4
     *   \brdrs          5
     *   \brdrth         6
     *   \brdrtriple     7
     *
     * The order of the sides stored actually differs from v1.0 to 3.0 and v4.1.
     * The mask corresponding to each side for the version are the following:
     *     mask       v1.0-3.0    v4.1
     *     0x000F     top         left
     *     0x00F0     left        top
     *     0x0F00     bottom      right
     *     0xF000     right       bottom
     */
    if (para->fmt.wBorders & 0x00B0)
      FIXME("Unsupported border flags %x\n", para->fmt.wBorders);
    border_width = ME_GetParaBorderWidth(c, para->fmt.wBorders);
    if (para->fmt.wBorders & 4)       top_border = border_width;
    if (para->fmt.wBorders & 8)       bottom_border = border_width;
  }

  if (para->fmt.dwMask & PFM_SPACEBEFORE)
  {
    bounds->top = ME_twips2pointsY(c, para->fmt.dySpaceBefore);
    if (editor_opaque( c->editor ))
    {
      rc.left = c->rcView.left;
      rc.right = c->rcView.right;
      rc.top = y;
      rc.bottom = y + bounds->top + top_border;
      PatBlt(c->hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
    }
  }

  if (para->fmt.dwMask & PFM_SPACEAFTER)
  {
    bounds->bottom = ME_twips2pointsY(c, para->fmt.dySpaceAfter);
    if (editor_opaque( c->editor ))
    {
      rc.left = c->rcView.left;
      rc.right = c->rcView.right;
      rc.bottom = y + para->nHeight;
      rc.top = rc.bottom - bounds->bottom - bottom_border;
      PatBlt(c->hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
    }
  }

  /* Native richedit doesn't support paragraph borders in v1.0 - 4.1,
   * but might support it in later versions. */
  if (hasParaBorder) {
    int         pen_width, rightEdge;
    COLORREF    pencr;
    HPEN        pen = NULL, oldpen = NULL;
    POINT       pt;

    if (para->fmt.wBorders & 64) /* autocolor */
      pencr = ITextHost_TxGetSysColor(c->editor->texthost,
                                      COLOR_WINDOWTEXT);
    else
      pencr = pen_colors[(para->fmt.wBorders >> 12) & 0xF];

    rightEdge = c->pt.x + max(c->editor->sizeWindow.cx,
                              c->editor->nTotalWidth);

    pen_width = ME_GetBorderPenWidth(c, idx);
    pen = CreatePen(border_details[idx].pen_style, pen_width, pencr);
    oldpen = SelectObject(c->hDC, pen);
    MoveToEx(c->hDC, 0, 0, &pt);

    /* before & after spaces are not included in border */

    /* helper to draw the double lines in case of corner */
#define DD(x)   ((para->fmt.wBorders & (x)) ? (pen_width + 1) : 0)

    if (para->fmt.wBorders & 1)
    {
      MoveToEx(c->hDC, c->pt.x, y + bounds->top, NULL);
      LineTo(c->hDC, c->pt.x, y + para->nHeight - bounds->bottom);
      if (border_details[idx].dble) {
        rc.left = c->pt.x + 1;
        rc.right = rc.left + border_width;
        rc.top = y + bounds->top;
        rc.bottom = y + para->nHeight - bounds->bottom;
        PatBlt(c->hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
        MoveToEx(c->hDC, c->pt.x + pen_width + 1, y + bounds->top + DD(4), NULL);
        LineTo(c->hDC, c->pt.x + pen_width + 1, y + para->nHeight - bounds->bottom - DD(8));
      }
      bounds->left += border_width;
    }
    if (para->fmt.wBorders & 2)
    {
      MoveToEx(c->hDC, rightEdge - 1, y + bounds->top, NULL);
      LineTo(c->hDC, rightEdge - 1, y + para->nHeight - bounds->bottom);
      if (border_details[idx].dble) {
        rc.left = rightEdge - pen_width - 1;
        rc.right = rc.left + pen_width;
        rc.top = y + bounds->top;
        rc.bottom = y + para->nHeight - bounds->bottom;
        PatBlt(c->hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
        MoveToEx(c->hDC, rightEdge - 1 - pen_width - 1, y + bounds->top + DD(4), NULL);
        LineTo(c->hDC, rightEdge - 1 - pen_width - 1, y + para->nHeight - bounds->bottom - DD(8));
      }
      bounds->right += border_width;
    }
    if (para->fmt.wBorders & 4)
    {
      MoveToEx(c->hDC, c->pt.x, y + bounds->top, NULL);
      LineTo(c->hDC, rightEdge, y + bounds->top);
      if (border_details[idx].dble) {
        MoveToEx(c->hDC, c->pt.x + DD(1), y + bounds->top + pen_width + 1, NULL);
        LineTo(c->hDC, rightEdge - DD(2), y + bounds->top + pen_width + 1);
      }
      bounds->top += border_width;
    }
    if (para->fmt.wBorders & 8)
    {
      MoveToEx(c->hDC, c->pt.x, y + para->nHeight - bounds->bottom - 1, NULL);
      LineTo(c->hDC, rightEdge, y + para->nHeight - bounds->bottom - 1);
      if (border_details[idx].dble) {
        MoveToEx(c->hDC, c->pt.x + DD(1), y + para->nHeight - bounds->bottom - 1 - pen_width - 1, NULL);
        LineTo(c->hDC, rightEdge - DD(2), y + para->nHeight - bounds->bottom - 1 - pen_width - 1);
      }
      bounds->bottom += border_width;
    }
#undef DD

    MoveToEx(c->hDC, pt.x, pt.y, NULL);
    SelectObject(c->hDC, oldpen);
    DeleteObject(pen);
  }
}

static void draw_table_borders( ME_Context *c, ME_Paragraph *para )
{
  if (!c->editor->bEmulateVersion10) /* v4.1 */
  {
    if (para_cell( para ))
    {
      RECT rc;
      ME_Cell *cell = para_cell( para );
      ME_Paragraph *after_row;
      HPEN pen, oldPen;
      LOGBRUSH logBrush;
      HBRUSH brush;
      COLORREF color;
      POINT oldPt;
      int width;
      BOOL atTop = (para == cell_first_para( cell ));
      BOOL atBottom = (para == cell_end_para( cell ));
      int top = c->pt.y + (atTop ? cell->pt.y : para->pt.y);
      int bottom = (atBottom ?
                    c->pt.y + cell->pt.y + cell->nHeight :
                    top + para->nHeight + (atTop ? cell->yTextOffset : 0));
      rc.left = c->pt.x + cell->pt.x;
      rc.right = rc.left + cell->nWidth;
      if (atTop)
      {
        /* Erase gap before text if not all borders are the same height. */
        width = max(ME_twips2pointsY(c, cell->border.top.width), 1);
        rc.top = top + width;
        width = cell->yTextOffset - width;
        rc.bottom = rc.top + width;
        if (width && editor_opaque( c->editor ))
          PatBlt(c->hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
      }
      /* Draw cell borders.
       * The order borders are draw in is left, top, bottom, right in order
       * to be consistent with native richedit.  This is noticeable from the
       * overlap of borders of different colours. */
      if (!(para->nFlags & MEPF_ROWEND))
      {
        rc.top = top;
        rc.bottom = bottom;
        if (cell->border.left.width > 0)
        {
          color = cell->border.left.colorRef;
          width = max(ME_twips2pointsX(c, cell->border.left.width), 1);
        }
        else
        {
          color = RGB(192,192,192);
          width = 1;
        }
        logBrush.lbStyle = BS_SOLID;
        logBrush.lbColor = color;
        logBrush.lbHatch = 0;
        pen = ExtCreatePen(PS_GEOMETRIC|PS_SOLID|PS_ENDCAP_FLAT|PS_JOIN_MITER,
                           width, &logBrush, 0, NULL);
        oldPen = SelectObject(c->hDC, pen);
        MoveToEx(c->hDC, rc.left, rc.top, &oldPt);
        LineTo(c->hDC, rc.left, rc.bottom);
        SelectObject(c->hDC, oldPen);
        DeleteObject(pen);
        MoveToEx(c->hDC, oldPt.x, oldPt.y, NULL);
      }

      if (atTop)
      {
        if (cell->border.top.width > 0)
        {
          brush = CreateSolidBrush(cell->border.top.colorRef);
          width = max(ME_twips2pointsY(c, cell->border.top.width), 1);
        }
        else
        {
          brush = GetStockObject(LTGRAY_BRUSH);
          width = 1;
        }
        rc.top = top;
        rc.bottom = rc.top + width;
        FillRect(c->hDC, &rc, brush);
        if (cell->border.top.width > 0)
          DeleteObject(brush);
      }

      /* Draw the bottom border if at the last paragraph in the cell, and when
       * in the last row of the table. */
      if (atBottom)
      {
        int oldLeft = rc.left;
        width = max(ME_twips2pointsY(c, cell->border.bottom.width), 1);
        after_row = para_next( table_row_end( para ) );
        if (after_row->nFlags & MEPF_ROWSTART)
        {
          ME_Cell *next_end;
          next_end = table_row_end_cell( after_row );
          assert( next_end && !cell_next( next_end ) );
          rc.left = c->pt.x + next_end->pt.x;
        }
        if (rc.left < rc.right)
        {
          if (cell->border.bottom.width > 0)
            brush = CreateSolidBrush(cell->border.bottom.colorRef);
          else
            brush = GetStockObject(LTGRAY_BRUSH);
          rc.bottom = bottom;
          rc.top = rc.bottom - width;
          FillRect(c->hDC, &rc, brush);
          if (cell->border.bottom.width > 0)
            DeleteObject(brush);
        }
        rc.left = oldLeft;
      }

      /* Right border only drawn if at the end of the table row. */
      if (!cell_next( cell_next( cell ) ) && !(para->nFlags & MEPF_ROWSTART))
      {
        rc.top = top;
        rc.bottom = bottom;
        if (cell->border.right.width > 0)
        {
          color = cell->border.right.colorRef;
          width = max(ME_twips2pointsX(c, cell->border.right.width), 1);
        }
        else
        {
          color = RGB(192,192,192);
          width = 1;
        }
        logBrush.lbStyle = BS_SOLID;
        logBrush.lbColor = color;
        logBrush.lbHatch = 0;
        pen = ExtCreatePen(PS_GEOMETRIC|PS_SOLID|PS_ENDCAP_FLAT|PS_JOIN_MITER,
                           width, &logBrush, 0, NULL);
        oldPen = SelectObject(c->hDC, pen);
        MoveToEx(c->hDC, rc.right - 1, rc.top, &oldPt);
        LineTo(c->hDC, rc.right - 1, rc.bottom);
        SelectObject(c->hDC, oldPen);
        DeleteObject(pen);
        MoveToEx(c->hDC, oldPt.x, oldPt.y, NULL);
      }
    }
  }
  else /* v1.0 - 3.0 */
  {
    /* Draw simple table border */
    if (para_in_table( para ))
    {
      HPEN pen = NULL, oldpen = NULL;
      int i, firstX, startX, endX, rowY, rowBottom, nHeight;
      POINT oldPt;

      pen = CreatePen(PS_SOLID, 0, para->border.top.colorRef);
      oldpen = SelectObject(c->hDC, pen);

      /* Find the start relative to the text */
      firstX = c->pt.x + para_first_run( para )->pt.x;
      /* Go back by the horizontal gap, which is stored in dxOffset */
      firstX -= ME_twips2pointsX(c, para->fmt.dxOffset);
      /* The left edge, stored in dxStartIndent affected just the first edge */
      startX = firstX - ME_twips2pointsX(c, para->fmt.dxStartIndent);
      rowY = c->pt.y + para->pt.y;
      if (para->fmt.dwMask & PFM_SPACEBEFORE)
        rowY += ME_twips2pointsY(c, para->fmt.dySpaceBefore);
      nHeight = para_first_row( para )->nHeight;
      rowBottom = rowY + nHeight;

      /* Draw horizontal lines */
      MoveToEx(c->hDC, firstX, rowY, &oldPt);
      i = para->fmt.cTabCount - 1;
      endX = startX + ME_twips2pointsX(c, para->fmt.rgxTabs[i] & 0x00ffffff) + 1;
      LineTo(c->hDC, endX, rowY);
      /* The bottom of the row only needs to be drawn if the next row is
       * not a table. */
      if (!(para_next( para ) && para_in_table( para_next( para ) ) && para->nRows == 1))
      {
        /* Decrement rowBottom to draw the bottom line within the row, and
         * to not draw over this line when drawing the vertical lines. */
        rowBottom--;
        MoveToEx(c->hDC, firstX, rowBottom, NULL);
        LineTo(c->hDC, endX, rowBottom);
      }

      /* Draw vertical lines */
      MoveToEx(c->hDC, firstX, rowY, NULL);
      LineTo(c->hDC, firstX, rowBottom);
      for (i = 0; i < para->fmt.cTabCount; i++)
      {
        int rightBoundary = para->fmt.rgxTabs[i] & 0x00ffffff;
        endX = startX + ME_twips2pointsX(c, rightBoundary);
        MoveToEx(c->hDC, endX, rowY, NULL);
        LineTo(c->hDC, endX, rowBottom);
      }

      MoveToEx(c->hDC, oldPt.x, oldPt.y, NULL);
      SelectObject(c->hDC, oldpen);
      DeleteObject(pen);
    }
  }
}

static void draw_para_number( ME_Context *c, ME_Paragraph *para )
{
    int x, y;
    COLORREF old_text;

    if (para->fmt.wNumbering)
    {
        select_style( c, para->para_num.style );
        old_text = SetTextColor( c->hDC, get_text_color( c, para->para_num.style, FALSE ) );

        x = c->pt.x + para->para_num.pt.x;
        y = c->pt.y + para->pt.y + para->para_num.pt.y;

        ExtTextOutW( c->hDC, x, y, 0, NULL, para->para_num.text->szData, para->para_num.text->nLen, NULL );

        SetTextColor( c->hDC, old_text );
    }
}

static void draw_paragraph( ME_Context *c, ME_Paragraph *para )
{
  int align = SetTextAlign(c->hDC, TA_BASELINE);
  ME_DisplayItem *p;
  ME_Cell *cell;
  ME_Run *run;
  RECT rc, bounds;
  int y;
  int height = 0, baseline = 0, no=0;
  BOOL visible = FALSE;

  rc.left = c->pt.x;
  rc.right = c->rcView.right;

  y = c->pt.y + para->pt.y;
  if ((cell = para_cell( para )))
  {
    rc.left = c->pt.x + cell->pt.x;
    rc.right = rc.left + cell->nWidth;
  }
  if (para->nFlags & MEPF_ROWSTART)
  {
    cell = table_row_first_cell( para );
    rc.right = c->pt.x + cell->pt.x;
  }
  else if (para->nFlags & MEPF_ROWEND)
  {
    cell = table_row_end_cell( para );
    rc.left = c->pt.x + cell->pt.x + cell->nWidth;
  }
  ME_DrawParaDecoration(c, para, y, &bounds);
  y += bounds.top;
  if (bounds.left || bounds.right)
  {
    rc.left = max(rc.left, c->pt.x + bounds.left);
    rc.right = min(rc.right, c->pt.x - bounds.right
                             + max(c->editor->sizeWindow.cx,
                                   c->editor->nTotalWidth));
  }

  for (p = para_get_di( para )->next; p != para_get_di( para_next( para ) ); p = p->next)
  {
    switch(p->type) {
      case diParagraph:
        assert(FALSE);
        break;
      case diStartRow:
        y += height;
        rc.top = y;
        if (para->nFlags & (MEPF_ROWSTART|MEPF_ROWEND)) {
          rc.bottom = y + para->nHeight;
        } else {
          rc.bottom = y + p->member.row.nHeight;
        }
        visible = RectVisible(c->hDC, &rc);
        if (editor_opaque( c->editor ) && visible)
          PatBlt(c->hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
        if (bounds.right)
        {
          /* If scrolled to the right past the end of the text, then
           * there may be space to the right of the paragraph border. */
          RECT after_bdr = rc;
          after_bdr.left = rc.right + bounds.right;
          after_bdr.right = c->rcView.right;
          if (editor_opaque( c->editor ) && RectVisible( c->hDC, &after_bdr ))
            PatBlt(c->hDC, after_bdr.left, after_bdr.top, after_bdr.right - after_bdr.left,
                   after_bdr.bottom - after_bdr.top, PATCOPY);
        }
        if (me_debug)
        {
          WCHAR buf[128];
          POINT pt = c->pt;
          wsprintfW( buf, L"row[%d]", no );
          pt.y = 12+y;
          ME_DebugWrite(c->hDC, &pt, buf);
        }

        height = p->member.row.nHeight;
        baseline = p->member.row.nBaseline;
        break;
      case diRun:
        assert(para);
        run = &p->member.run;
        if (visible && me_debug) {
          RECT rc;
          rc.left = c->pt.x + run->pt.x;
          rc.right = rc.left + run->nWidth;
          rc.top = c->pt.y + para->pt.y + run->pt.y;
          rc.bottom = rc.top + height;
          TRACE("rc = %s\n", wine_dbgstr_rect(&rc));
          FrameRect(c->hDC, &rc, GetSysColorBrush(COLOR_GRAYTEXT));
        }
        if (visible)
        {
          ME_Cursor cursor;

          cursor.run = run;
          cursor.para = para;
          cursor.nOffset = 0;
          draw_run( c, c->pt.x + run->pt.x, c->pt.y + para->pt.y + run->pt.y + baseline, &cursor );
        }
        if (me_debug)
        {
          WCHAR buf[2560];
          POINT pt;
          pt.x = c->pt.x + run->pt.x;
          pt.y = c->pt.y + para->pt.y + run->pt.y;
          wsprintfW( buf, L"[%d:%x] %ls", no, p->member.run.nFlags, get_text( &p->member.run, 0 ));
          ME_DebugWrite(c->hDC, &pt, buf);
        }
        break;
      default:
        break;
    }
    no++;
  }

    if (editor_opaque( c->editor ) && para_cell( para ))
    {
        /* Clear any space at the bottom of the cell after the text. */
        rc.top = c->pt.y + para->pt.y + para->nHeight;
        rc.bottom = c->pt.y + cell->pt.y + cell->nHeight;
        PatBlt( c->hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
    }

    draw_table_borders( c, para );
    draw_para_number( c, para );

    SetTextAlign( c->hDC, align );
}

static void enable_show_scrollbar( ME_TextEditor *editor, INT bar, BOOL enable )
{
    if (enable || editor->scrollbars & ES_DISABLENOSCROLL)
        ITextHost_TxEnableScrollBar( editor->texthost, bar, enable ? 0 : ESB_DISABLE_BOTH );
    if (!(editor->scrollbars & ES_DISABLENOSCROLL))
        ITextHost_TxShowScrollBar( editor->texthost, bar, enable );
}

static void set_scroll_range_pos( ME_TextEditor *editor, INT bar, SCROLLINFO *info, BOOL set_range, BOOL notify )
{
    LONG max_pos = info->nMax, pos = info->nPos;

    /* Scale the scrollbar info to 16-bit values. */
    if (max_pos > 0xffff)
    {
        pos = MulDiv( pos, 0xffff, max_pos );
        max_pos = 0xffff;
    }
    if (set_range) ITextHost_TxSetScrollRange( editor->texthost, bar, 0, max_pos, FALSE );
    ITextHost_TxSetScrollPos( editor->texthost, bar, pos, TRUE );

    if (notify && editor->nEventMask & ENM_SCROLL)
        ITextHost_TxNotify( editor->texthost, bar == SB_VERT ? EN_VSCROLL : EN_HSCROLL, NULL );
}

void scroll_abs( ME_TextEditor *editor, int x, int y, BOOL notify )
{
  int scrollX = 0, scrollY = 0;

  if (editor->horz_si.nPos != x) {
    x = min(x, editor->horz_si.nMax);
    x = max(x, editor->horz_si.nMin);
    scrollX = editor->horz_si.nPos - x;
    editor->horz_si.nPos = x;
    set_scroll_range_pos( editor, SB_HORZ, &editor->horz_si, FALSE, notify );
  }

  if (editor->vert_si.nPos != y) {
    y = min(y, editor->vert_si.nMax - (int)editor->vert_si.nPage);
    y = max(y, editor->vert_si.nMin);
    scrollY = editor->vert_si.nPos - y;
    editor->vert_si.nPos = y;
    set_scroll_range_pos( editor, SB_VERT, &editor->vert_si, FALSE, notify );
  }

  if (abs(scrollX) > editor->sizeWindow.cx || abs(scrollY) > editor->sizeWindow.cy)
    ITextHost_TxInvalidateRect(editor->texthost, NULL, TRUE);
  else
    ITextHost_TxScrollWindowEx(editor->texthost, scrollX, scrollY,
                               &editor->rcFormat, &editor->rcFormat,
                               NULL, NULL, SW_INVALIDATE);
  ME_UpdateScrollBar(editor);
  ME_Repaint(editor);
}

void scroll_h_abs( ME_TextEditor *editor, int x, BOOL notify )
{
    scroll_abs( editor, x, editor->vert_si.nPos, notify );
}

void scroll_v_abs( ME_TextEditor *editor, int y, BOOL notify )
{
    scroll_abs( editor, editor->horz_si.nPos, y, notify );
}

void ME_ScrollUp(ME_TextEditor *editor, int cy)
{
    scroll_v_abs( editor, editor->vert_si.nPos - cy, TRUE );
}

void ME_ScrollDown(ME_TextEditor *editor, int cy)
{
    scroll_v_abs( editor, editor->vert_si.nPos + cy, TRUE );
}

void ME_ScrollLeft(ME_TextEditor *editor, int cx)
{
    scroll_h_abs( editor, editor->horz_si.nPos - cx, TRUE );
}

void ME_ScrollRight(ME_TextEditor *editor, int cx)
{
    scroll_h_abs( editor, editor->horz_si.nPos + cx, TRUE );
}

void ME_UpdateScrollBar(ME_TextEditor *editor)
{
  /* Note that this is the only function that should ever call
   * SetScrollInfo with SIF_PAGE or SIF_RANGE. */
  BOOL enable;

  if (ME_WrapMarkedParagraphs(editor))
    FIXME("ME_UpdateScrollBar had to call ME_WrapMarkedParagraphs\n");

  /* Update horizontal scrollbar */
  enable = editor->nTotalWidth > editor->sizeWindow.cx;
  if (editor->horz_si.nPos && !enable)
  {
    scroll_h_abs( editor, 0, TRUE );
    /* ME_HScrollAbs will call this function, so nothing else needs to be done here. */
    return;
  }

  if (editor->scrollbars & WS_HSCROLL && !enable ^ !editor->horz_sb_enabled)
  {
    editor->horz_sb_enabled = enable;
    enable_show_scrollbar( editor, SB_HORZ, enable );
  }

  if (editor->horz_si.nMax != editor->nTotalWidth || editor->horz_si.nPage != editor->sizeWindow.cx)
  {
    editor->horz_si.nMax = editor->nTotalWidth;
    editor->horz_si.nPage = editor->sizeWindow.cx;
    TRACE( "min = %d max = %d page = %d\n", editor->horz_si.nMin, editor->horz_si.nMax, editor->horz_si.nPage );
    if ((enable || editor->horz_sb_enabled) && editor->scrollbars & WS_HSCROLL)
      set_scroll_range_pos( editor, SB_HORZ, &editor->horz_si, TRUE, TRUE );
  }

  /* Update vertical scrollbar */
  enable = editor->nTotalLength > editor->sizeWindow.cy && (editor->props & TXTBIT_MULTILINE);

  if (editor->vert_si.nPos && !enable)
  {
    scroll_v_abs( editor, 0, TRUE );
    /* ME_VScrollAbs will call this function, so nothing else needs to be done here. */
    return;
  }

  if (editor->scrollbars & WS_VSCROLL && !enable ^ !editor->vert_sb_enabled)
  {
    editor->vert_sb_enabled = enable;
    enable_show_scrollbar( editor, SB_VERT, enable );
  }

  if (editor->vert_si.nMax != editor->nTotalLength || editor->vert_si.nPage != editor->sizeWindow.cy)
  {
    editor->vert_si.nMax = editor->nTotalLength;
    editor->vert_si.nPage = editor->sizeWindow.cy;
    TRACE( "min = %d max = %d page = %d\n", editor->vert_si.nMin, editor->vert_si.nMax, editor->vert_si.nPage );
    if ((enable || editor->vert_sb_enabled) && editor->scrollbars & WS_VSCROLL)
      set_scroll_range_pos( editor, SB_VERT, &editor->vert_si, TRUE, TRUE );
  }
}

void editor_ensure_visible( ME_TextEditor *editor, ME_Cursor *cursor )
{
  ME_Run *run = cursor->run;
  ME_Row *row = row_from_cursor( cursor );
  ME_Paragraph *para = cursor->para;
  int x, y, yheight;

  if (!editor->in_place_active)
    return;

  if (editor->scrollbars & ES_AUTOHSCROLL)
  {
    x = run->pt.x + ME_PointFromChar( editor, run, cursor->nOffset, TRUE );
    if (x > editor->horz_si.nPos + editor->sizeWindow.cx)
      x = x + 1 - editor->sizeWindow.cx;
    else if (x > editor->horz_si.nPos)
      x = editor->horz_si.nPos;

    if (~editor->scrollbars & ES_AUTOVSCROLL)
    {
      scroll_h_abs( editor, x, TRUE );
      return;
    }
  }
  else
  {
    if (~editor->scrollbars & ES_AUTOVSCROLL) return;
    x = editor->horz_si.nPos;
  }

  y = para->pt.y + row->pt.y;
  yheight = row->nHeight;

  if (y < editor->vert_si.nPos)
    scroll_abs( editor, x, y, TRUE );
  else if (y + yheight > editor->vert_si.nPos + editor->sizeWindow.cy)
    scroll_abs( editor, x, y + yheight - editor->sizeWindow.cy, TRUE );
  else if (x != editor->horz_si.nPos)
    scroll_abs( editor, x, editor->vert_si.nPos, TRUE );
}


void
ME_InvalidateSelection(ME_TextEditor *editor)
{
  ME_Paragraph *sel_start, *sel_end;
  ME_Paragraph *repaint_start = NULL, *repaint_end = NULL;
  LONG nStart, nEnd;
  int len = ME_GetTextLength(editor);

  ME_GetSelectionOfs(editor, &nStart, &nEnd);
  /* if both old and new selection are 0-char (= caret only), then
  there's no (inverted) area to be repainted, neither old nor new */
  if (nStart == nEnd && editor->nLastSelStart == editor->nLastSelEnd)
    return;
  ME_WrapMarkedParagraphs(editor);
  editor_get_selection_paras( editor, &sel_start, &sel_end );

  /* last selection markers aren't always updated, which means
   * they can point past the end of the document */
  if (editor->nLastSelStart > len || editor->nLastSelEnd > len)
  {
    repaint_start = editor_first_para( editor );
    repaint_end = para_prev( editor_end_para( editor ) );
  }
  else
  {
    /* if the start part of selection is being expanded or contracted... */
    if (nStart < editor->nLastSelStart)
    {
      repaint_start = sel_start;
      repaint_end = editor->last_sel_start_para;
    }
    else if (nStart > editor->nLastSelStart)
    {
      repaint_start = editor->last_sel_start_para;
      repaint_end = sel_start;
    }

    /* if the end part of selection is being contracted or expanded... */
    if (nEnd < editor->nLastSelEnd)
    {
      if (!repaint_start) repaint_start = sel_end;
      repaint_end = editor->last_sel_end_para;
    }
    else if (nEnd > editor->nLastSelEnd)
    {
      if (!repaint_start) repaint_start = editor->last_sel_end_para;
      repaint_end = sel_end;
    }
  }

  if (repaint_start)
    para_range_invalidate( editor, repaint_start, repaint_end );
  /* remember the last invalidated position */
  ME_GetSelectionOfs(editor, &editor->nLastSelStart, &editor->nLastSelEnd);
  editor_get_selection_paras( editor, &editor->last_sel_start_para, &editor->last_sel_end_para );
}

BOOL
ME_SetZoom(ME_TextEditor *editor, int numerator, int denominator)
{
  /* TODO: Zoom images and objects */

  if (numerator == 0 && denominator == 0)
  {
    editor->nZoomNumerator = editor->nZoomDenominator = 0;
    return TRUE;
  }
  if (numerator <= 0 || denominator <= 0)
    return FALSE;
  if (numerator * 64 <= denominator || numerator / denominator >= 64)
    return FALSE;

  editor->nZoomNumerator = numerator;
  editor->nZoomDenominator = denominator;

  ME_RewrapRepaint(editor);
  return TRUE;
}
