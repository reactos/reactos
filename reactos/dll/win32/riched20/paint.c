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

void ME_PaintContent(ME_TextEditor *editor, HDC hDC, BOOL bOnlyNew, const RECT *rcUpdate)
{
  ME_DisplayItem *item;
  ME_Context c;
  int yoffset;
  int ys, ye;

  editor->nSequence++;
  yoffset = ME_GetYScrollPos(editor);
  ME_InitContext(&c, editor, hDC);
  SetBkMode(hDC, TRANSPARENT);
  ME_MoveCaret(editor); /* Calls ME_WrapMarkedParagraphs */
  item = editor->pBuffer->pFirst->next;
  /* This context point is an offset for the paragraph positions stored
   * during wrapping. It shouldn't be modified during painting. */
  c.pt.x = c.rcView.left;
  c.pt.y = c.rcView.top - yoffset;
  while(item != editor->pBuffer->pLast)
  {
    assert(item->type == diParagraph);

    ys = c.pt.y + item->member.para.pt.y;
    if (item->member.para.pCell
        != item->member.para.next_para->member.para.pCell)
    {
      ME_Cell *cell = NULL;
      cell = &ME_FindItemBack(item->member.para.next_para, diCell)->member.cell;
      ye = c.pt.y + cell->pt.y + cell->nHeight;
    } else {
      ye = ys + item->member.para.nHeight;
    }
    if (item->member.para.pCell && !(item->member.para.nFlags & MEPF_ROWEND) &&
        item->member.para.pCell != item->member.para.prev_para->member.para.pCell)
    {
      /* the border shifts the text down */
      ys -= item->member.para.pCell->member.cell.yTextOffset;
    }

    if (!bOnlyNew || (item->member.para.nFlags & MEPF_REPAINT))
    {
      /* Draw the pargraph if any of the paragraph is in the update region. */
      BOOL bPaint = (rcUpdate == NULL);
      if (rcUpdate)
        bPaint = ys < rcUpdate->bottom && ye > rcUpdate->top;
      if (bPaint)
      {
        ME_DrawParagraph(&c, item);
        /* Clear the repaint flag if the whole paragraph is in the
         * update region. */
        if (!rcUpdate || (rcUpdate->top <= ys && rcUpdate->bottom >= ye))
          item->member.para.nFlags &= ~MEPF_REPAINT;
      }
    }
    item = item->member.para.next_para;
  }
  if (c.pt.y + editor->nTotalLength < c.rcView.bottom)
  {
    /* Fill space after the end of the text. */
    RECT rc;
    rc.top = c.pt.y + editor->nTotalLength;
    rc.left = c.rcView.left;
    rc.bottom = c.rcView.bottom;
    rc.right = c.rcView.right;

    if (bOnlyNew)
    {
      /* Only erase region drawn from previous call to ME_PaintContent */
      if (editor->nTotalLength < editor->nLastTotalLength)
        rc.bottom = c.pt.y + editor->nLastTotalLength;
      else
        SetRectEmpty(&rc);
    }

    IntersectRect(&rc, &rc, rcUpdate);

    if (!IsRectEmpty(&rc))
      FillRect(hDC, &rc, c.editor->hbrBackground);
  }
  if (editor->nTotalLength != editor->nLastTotalLength)
    ME_SendRequestResize(editor, FALSE);
  editor->nLastTotalLength = editor->nTotalLength;
  ME_DestroyContext(&c, NULL);
}

void ME_Repaint(ME_TextEditor *editor)
{
  if (ME_WrapMarkedParagraphs(editor))
  {
    ME_UpdateScrollBar(editor);
    FIXME("ME_Repaint had to call ME_WrapMarkedParagraphs\n");
  }
  if (!IsWindowVisible(editor->hWnd))
    return;
  if (!editor->bEmulateVersion10 || (editor->nEventMask & ENM_UPDATE))
    ME_SendOldNotify(editor, EN_UPDATE);
  UpdateWindow(editor->hWnd);
}

void ME_UpdateRepaint(ME_TextEditor *editor)
{
  /* Should be called whenever the contents of the control have changed */
  ME_Cursor *pCursor;
  BOOL wrappedParagraphs;

  wrappedParagraphs = ME_WrapMarkedParagraphs(editor);
  if (wrappedParagraphs)
    ME_UpdateScrollBar(editor);
  
  /* Ensure that the cursor is visible */
  pCursor = &editor->pCursors[0];
  ME_EnsureVisible(editor, pCursor->pRun);
  
  /* send EN_CHANGE if the event mask asks for it */
  if(editor->nEventMask & ENM_CHANGE)
  {
    editor->nEventMask &= ~ENM_CHANGE;
    ME_SendOldNotify(editor, EN_CHANGE);
    editor->nEventMask |= ENM_CHANGE;
  }
  ME_Repaint(editor);
  ME_SendSelChange(editor);
}

void
ME_RewrapRepaint(ME_TextEditor *editor)
{ 
  /* RewrapRepaint should be called whenever the control has changed in
   * looks, but not content. Like resizing. */
  
  ME_MarkAllForWrapping(editor);
  ME_WrapMarkedParagraphs(editor);
  ME_UpdateScrollBar(editor);
  ME_Repaint(editor);
}

int ME_twips2pointsX(ME_Context *c, int x)
{
  if (c->editor->nZoomNumerator == 0)
    return x * c->dpi.cx / 1440;
  else
    return x * c->dpi.cx * c->editor->nZoomNumerator / 1440 / c->editor->nZoomDenominator;
}

int ME_twips2pointsY(ME_Context *c, int y)
{
  if (c->editor->nZoomNumerator == 0)
    return y * c->dpi.cy / 1440;
  else
    return y * c->dpi.cy * c->editor->nZoomNumerator / 1440 / c->editor->nZoomDenominator;
}

static void ME_HighlightSpace(ME_Context *c, int x, int y, LPCWSTR szText,
                              int nChars, ME_Style *s, int width,
                              int nSelFrom, int nSelTo, int ymin, int cy)
{
  HDC hDC = c->hDC;
  HGDIOBJ hOldFont = NULL;
  SIZE sz;
  int selWidth;
  /* Only highlight if there is a selection in the run and when
   * EM_HIDESELECTION is not being used to hide the selection. */
  if (nSelFrom >= nChars || nSelTo < 0 || nSelFrom >= nSelTo
      || c->editor->bHideSelection)
    return;
  hOldFont = ME_SelectStyleFont(c, s);
  if (width <= 0)
  {
    GetTextExtentPoint32W(hDC, szText, nChars, &sz);
    width = sz.cx;
  }
  if (nSelFrom < 0) nSelFrom = 0;
  if (nSelTo > nChars) nSelTo = nChars;
  GetTextExtentPoint32W(hDC, szText, nSelFrom, &sz);
  x += sz.cx;
  if (nSelTo != nChars)
  {
    GetTextExtentPoint32W(hDC, szText+nSelFrom, nSelTo-nSelFrom, &sz);
    selWidth = sz.cx;
  } else {
    selWidth = width - sz.cx;
  }
  ME_UnselectStyleFont(c, s, hOldFont);

  if (c->editor->bEmulateVersion10)
    PatBlt(hDC, x, ymin, selWidth, cy, DSTINVERT);
  else
  {
    RECT rect;
    HBRUSH hBrush;
    rect.left = x;
    rect.top = ymin;
    rect.right = x + selWidth;
    rect.bottom = ymin + cy;
    hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
    FillRect(hDC, &rect, hBrush);
    DeleteObject(hBrush);
  }
}

static void ME_DrawTextWithStyle(ME_Context *c, int x, int y, LPCWSTR szText,
                                 int nChars, ME_Style *s, int width,
                                 int nSelFrom, int nSelTo, int ymin, int cy)
{
  HDC hDC = c->hDC;
  HGDIOBJ hOldFont;
  COLORREF rgbOld;
  int yOffset = 0, yTwipsOffset = 0;
  SIZE          sz;
  COLORREF      rgb;
  HPEN hPen = NULL, hOldPen = NULL;
  BOOL bHighlightedText = (nSelFrom < nChars && nSelTo >= 0
                           && nSelFrom < nSelTo && !c->editor->bHideSelection);
  int xSelStart = x, xSelEnd = x;
  int *lpDx = NULL;
  /* lpDx is only needed for tabs to make sure the underline done automatically
   * by the font extends to the end of the tab. Tabs are always stored as
   * a single character run, so we can handle this case separately, since
   * otherwise lpDx would need to specify the lengths of each character. */
  if (width && nChars == 1)
      lpDx = &width; /* Make sure underline for tab extends across tab space */

  hOldFont = ME_SelectStyleFont(c, s);
  if ((s->fmt.dwMask & s->fmt.dwEffects) & CFM_OFFSET) {
    yTwipsOffset = s->fmt.yOffset;
  }
  if ((s->fmt.dwMask & s->fmt.dwEffects) & (CFM_SUPERSCRIPT | CFM_SUBSCRIPT)) {
    if (s->fmt.dwEffects & CFE_SUPERSCRIPT) yTwipsOffset = s->fmt.yHeight/3;
    if (s->fmt.dwEffects & CFE_SUBSCRIPT) yTwipsOffset = -s->fmt.yHeight/12;
  }
  if (yTwipsOffset)
    yOffset = ME_twips2pointsY(c, yTwipsOffset);

  if ((s->fmt.dwMask & CFM_LINK) && (s->fmt.dwEffects & CFE_LINK))
    rgb = RGB(0,0,255);
  else if ((s->fmt.dwMask & CFM_COLOR) && (s->fmt.dwEffects & CFE_AUTOCOLOR))
    rgb = GetSysColor(COLOR_WINDOWTEXT);
  else
    rgb = s->fmt.crTextColor;

  /* Determine the area that is selected in the run. */
  GetTextExtentPoint32W(hDC, szText, nChars, &sz);
  /* Treat width as an optional parameter.  We can get the width from the
   * text extent of the string if it isn't specified. */
  if (!width) width = sz.cx;
  if (bHighlightedText)
  {
    if (nSelFrom <= 0)
    {
      nSelFrom = 0;
    }
    else
    {
      GetTextExtentPoint32W(hDC, szText, nSelFrom, &sz);
      xSelStart = x + sz.cx;
    }
    if (nSelTo >= nChars)
    {
      nSelTo = nChars;
      xSelEnd = x + width;
    }
    else
    {
      GetTextExtentPoint32W(hDC, szText+nSelFrom, nSelTo-nSelFrom, &sz);
      xSelEnd = xSelStart + sz.cx;
    }
  }

  /* Choose the pen type for underlining the text. */
  if (s->fmt.dwMask & CFM_UNDERLINETYPE)
  {
    switch (s->fmt.bUnderlineType)
    {
    case CFU_UNDERLINE:
    case CFU_UNDERLINEWORD: /* native seems to map it to simple underline (MSDN) */
    case CFU_UNDERLINEDOUBLE: /* native seems to map it to simple underline (MSDN) */
      hPen = CreatePen(PS_SOLID, 1, rgb);
      break;
    case CFU_UNDERLINEDOTTED:
      hPen = CreatePen(PS_DOT, 1, rgb);
      break;
    default:
      WINE_FIXME("Unknown underline type (%u)\n", s->fmt.bUnderlineType);
      /* fall through */
    case CFU_CF1UNDERLINE: /* this type is supported in the font, do nothing */
    case CFU_UNDERLINENONE:
      hPen = NULL;
      break;
    }
    if (hPen)
    {
      hOldPen = SelectObject(hDC, hPen);
    }
  }

  rgbOld = SetTextColor(hDC, rgb);
  if (bHighlightedText && !c->editor->bEmulateVersion10)
  {
    COLORREF rgbBackOld;
    RECT dim;
    /* FIXME: should use textmetrics info for Descent info */
    if (hPen)
      MoveToEx(hDC, x, y - yOffset + 1, NULL);
    if (xSelStart > x)
    {
      ExtTextOutW(hDC, x, y-yOffset, 0, NULL, szText, nSelFrom, NULL);
      if (hPen)
        LineTo(hDC, xSelStart, y - yOffset + 1);
    }
    dim.top = ymin;
    dim.bottom = ymin + cy;
    dim.left = xSelStart;
    dim.right = xSelEnd;
    SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    rgbBackOld = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
    ExtTextOutW(hDC, xSelStart, y-yOffset, ETO_OPAQUE, &dim,
                szText+nSelFrom, nSelTo-nSelFrom, lpDx);
    if (hPen)
      LineTo(hDC, xSelEnd, y - yOffset + 1);
    SetBkColor(hDC, rgbBackOld);
    if (xSelEnd < x + width)
    {
      SetTextColor(hDC, rgb);
      ExtTextOutW(hDC, xSelEnd, y-yOffset, 0, NULL, szText+nSelTo,
                  nChars-nSelTo, NULL);
      if (hPen)
        LineTo(hDC, x + width, y - yOffset + 1);
    }
  }
  else
  {
    ExtTextOutW(hDC, x, y-yOffset, 0, NULL, szText, nChars, lpDx);

    /* FIXME: should use textmetrics info for Descent info */
    if (hPen)
    {
      MoveToEx(hDC, x, y - yOffset + 1, NULL);
      LineTo(hDC, x + width, y - yOffset + 1);
    }

    if (bHighlightedText) /* v1.0 inverts the selection */
    {
      PatBlt(hDC, xSelStart, ymin, xSelEnd-xSelStart, cy, DSTINVERT);
    }
  }

  if (hPen)
  {
    SelectObject(hDC, hOldPen);
    DeleteObject(hPen);
  }
  SetTextColor(hDC, rgbOld);
  ME_UnselectStyleFont(c, s, hOldFont);
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

static void ME_DrawRun(ME_Context *c, int x, int y, ME_DisplayItem *rundi, ME_Paragraph *para) 
{
  ME_Run *run = &rundi->member.run;
  ME_DisplayItem *start;
  int runofs = run->nCharOfs+para->nCharOfs;
  int nSelFrom, nSelTo;
  const WCHAR wszSpace[] = {' ', 0};
  
  if (run->nFlags & MERF_HIDDEN)
    return;

  start = ME_FindItemBack(rundi, diStartRow);
  ME_GetSelection(c->editor, &nSelFrom, &nSelTo);

  /* Draw selected end-of-paragraph mark */
  /* you can always comment it out if you need visible paragraph marks */
  if (run->nFlags & MERF_ENDPARA)
  {
    if (runofs >= nSelFrom && runofs < nSelTo)
    {
      ME_HighlightSpace(c, x, y, wszSpace, 1, run->style, 0, 0, 1,
                        c->pt.y + para->pt.y + start->member.row.pt.y,
                        start->member.row.nHeight);
    }
    return;
  }

  if (run->nFlags & (MERF_TAB | MERF_ENDCELL))
  {
    /* wszSpace is used instead of the tab character because otherwise
     * an unwanted symbol can be inserted instead. */
    ME_DrawTextWithStyle(c, x, y, wszSpace, 1, run->style, run->nWidth,
                         nSelFrom-runofs, nSelTo-runofs,
                         c->pt.y + para->pt.y + start->member.row.pt.y,
                         start->member.row.nHeight);
    return;
  }

  if (run->nFlags & MERF_GRAPHICS)
    ME_DrawOLE(c, x, y, run, para, (runofs >= nSelFrom) && (runofs < nSelTo));
  else
  {
    if (c->editor->cPasswordMask)
    {
      ME_String *szMasked = ME_MakeStringR(c->editor->cPasswordMask,ME_StrVLen(run->strText));
      ME_DrawTextWithStyle(c, x, y,
        szMasked->szData, ME_StrVLen(szMasked), run->style, run->nWidth,
        nSelFrom-runofs,nSelTo-runofs,
        c->pt.y + para->pt.y + start->member.row.pt.y,
        start->member.row.nHeight);
      ME_DestroyString(szMasked);
    }
    else
      ME_DrawTextWithStyle(c, x, y,
        run->strText->szData, ME_StrVLen(run->strText), run->style, run->nWidth,
        nSelFrom-runofs,nSelTo-runofs,
        c->pt.y + para->pt.y + start->member.row.pt.y,
        start->member.row.nHeight);
  }
}

static const struct {unsigned width_num : 4, width_den : 4, pen_style : 4, dble : 1;} border_details[] = {
  /* none */            {0, 1, PS_SOLID, FALSE},
  /* 3/4 */             {3, 4, PS_SOLID, FALSE},
  /* 1 1/2 */           {3, 2, PS_SOLID, FALSE},
  /* 2 1/4 */           {9, 4, PS_SOLID, FALSE},
  /* 3 */               {3, 1, PS_SOLID, FALSE},
  /* 4 1/2 */           {9, 2, PS_SOLID, FALSE},
  /* 6 */               {6, 1, PS_SOLID, FALSE},
  /* 3/4 double */      {3, 4, PS_SOLID, TRUE},
  /* 1 1/2 double */    {3, 2, PS_SOLID, TRUE},
  /* 2 1/4 double */    {9, 4, PS_SOLID, TRUE},
  /* 3/4 gray */        {3, 4, PS_DOT /* FIXME */, FALSE},
  /* 1 1/2 dashed */    {3, 2, PS_DASH, FALSE},
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

static int ME_GetBorderPenWidth(ME_TextEditor* editor, int idx)
{
  int width;

  if (editor->nZoomNumerator == 0)
  {
      width = border_details[idx].width_num + border_details[idx].width_den / 2;
      width /= border_details[idx].width_den;
  }
  else
  {
      width = border_details[idx].width_num * editor->nZoomNumerator;
      width += border_details[idx].width_den * editor->nZoomNumerator / 2;
      width /= border_details[idx].width_den * editor->nZoomDenominator;
  }
  return width;
}

int  ME_GetParaBorderWidth(ME_TextEditor* editor, int flags)
{
  int idx = (flags >> 8) & 0xF;
  int width;

  if (idx >= sizeof(border_details) / sizeof(border_details[0]))
  {
      FIXME("Unsupported border value %d\n", idx);
      return 0;
  }
  width = ME_GetBorderPenWidth(editor, idx);
  if (border_details[idx].dble) width = width * 2 + 1;
  return width;
}

int  ME_GetParaLineSpace(ME_Context* c, ME_Paragraph* para)
{
  int   sp = 0, ls = 0;
  if (!(para->pFmt->dwMask & PFM_LINESPACING)) return 0;

  /* FIXME: how to compute simply the line space in ls ??? */
  /* FIXME: does line spacing include the line itself ??? */
  switch (para->pFmt->bLineSpacingRule)
  {
  case 0:       sp = ls; break;
  case 1:       sp = (3 * ls) / 2; break;
  case 2:       sp = 2 * ls; break;
  case 3:       sp = ME_twips2pointsY(c, para->pFmt->dyLineSpacing); if (sp < ls) sp = ls; break;
  case 4:       sp = ME_twips2pointsY(c, para->pFmt->dyLineSpacing); break;
  case 5:       sp = para->pFmt->dyLineSpacing / 20; break;
  default: FIXME("Unsupported spacing rule value %d\n", para->pFmt->bLineSpacingRule);
  }
  if (c->editor->nZoomNumerator == 0)
    return sp;
  else
    return sp * c->editor->nZoomNumerator / c->editor->nZoomDenominator;
}

static void ME_DrawParaDecoration(ME_Context* c, ME_Paragraph* para, int y, RECT* bounds)
{
  int           idx, border_width, top_border, bottom_border;
  RECT          rc;
  BOOL          hasParaBorder;

  SetRectEmpty(bounds);
  if (!(para->pFmt->dwMask & (PFM_BORDER | PFM_SPACEBEFORE | PFM_SPACEAFTER))) return;

  border_width = top_border = bottom_border = 0;
  idx = (para->pFmt->wBorders >> 8) & 0xF;
  hasParaBorder = (!(c->editor->bEmulateVersion10 &&
                     para->pFmt->dwMask & PFM_TABLE &&
                     para->pFmt->wEffects & PFE_TABLE) &&
                   (para->pFmt->dwMask & PFM_BORDER) &&
                    idx != 0 &&
                    (para->pFmt->wBorders & 0xF));
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
    if (para->pFmt->wBorders & 0x00B0)
      FIXME("Unsupported border flags %x\n", para->pFmt->wBorders);
    border_width = ME_GetParaBorderWidth(c->editor, para->pFmt->wBorders);
    if (para->pFmt->wBorders & 4)       top_border = border_width;
    if (para->pFmt->wBorders & 8)       bottom_border = border_width;
  }

  if (para->pFmt->dwMask & PFM_SPACEBEFORE)
  {
    rc.left = c->rcView.left;
    rc.right = c->rcView.right;
    rc.top = y;
    bounds->top = ME_twips2pointsY(c, para->pFmt->dySpaceBefore);
    rc.bottom = y + bounds->top + top_border;
    FillRect(c->hDC, &rc, c->editor->hbrBackground);
  }

  if (para->pFmt->dwMask & PFM_SPACEAFTER)
  {
    rc.left = c->rcView.left;
    rc.right = c->rcView.right;
    rc.bottom = y + para->nHeight;
    bounds->bottom = ME_twips2pointsY(c, para->pFmt->dySpaceAfter);
    rc.top = rc.bottom - bounds->bottom - bottom_border;
    FillRect(c->hDC, &rc, c->editor->hbrBackground);
  }

  /* Native richedit doesn't support paragraph borders in v1.0 - 4.1,
   * but might support it in later versions. */
  if (hasParaBorder) {
    int         pen_width;
    COLORREF    pencr;
    HPEN        pen = NULL, oldpen = NULL;
    POINT       pt;

    if (para->pFmt->wBorders & 64) /* autocolor */
      pencr = GetSysColor(COLOR_WINDOWTEXT);
    else
      pencr = pen_colors[(para->pFmt->wBorders >> 12) & 0xF];

    pen_width = ME_GetBorderPenWidth(c->editor, idx);
    pen = CreatePen(border_details[idx].pen_style, pen_width, pencr);
    oldpen = SelectObject(c->hDC, pen);
    MoveToEx(c->hDC, 0, 0, &pt);

    /* before & after spaces are not included in border */

    /* helper to draw the double lines in case of corner */
#define DD(x)   ((para->pFmt->wBorders & (x)) ? (pen_width + 1) : 0)

    if (para->pFmt->wBorders & 1)
    {
      MoveToEx(c->hDC, c->rcView.left, y + bounds->top, NULL);
      LineTo(c->hDC, c->rcView.left, y + para->nHeight - bounds->bottom);
      if (border_details[idx].dble) {
        rc.left = c->rcView.left + 1;
        rc.right = rc.left + border_width;
        rc.top = y + bounds->top;
        rc.bottom = y + para->nHeight - bounds->bottom;
        FillRect(c->hDC, &rc, c->editor->hbrBackground);
        MoveToEx(c->hDC, c->rcView.left + pen_width + 1, y + bounds->top + DD(4), NULL);
        LineTo(c->hDC, c->rcView.left + pen_width + 1, y + para->nHeight - bounds->bottom - DD(8));
      }
      bounds->left += border_width;
    }
    if (para->pFmt->wBorders & 2)
    {
      MoveToEx(c->hDC, c->rcView.right - 1, y + bounds->top, NULL);
      LineTo(c->hDC, c->rcView.right - 1, y + para->nHeight - bounds->bottom);
      if (border_details[idx].dble) {
        rc.left = c->rcView.right - pen_width - 1;
        rc.right = c->rcView.right - 1;
        rc.top = y + bounds->top;
        rc.bottom = y + para->nHeight - bounds->bottom;
        FillRect(c->hDC, &rc, c->editor->hbrBackground);
        MoveToEx(c->hDC, c->rcView.right - 1 - pen_width - 1, y + bounds->top + DD(4), NULL);
        LineTo(c->hDC, c->rcView.right - 1 - pen_width - 1, y + para->nHeight - bounds->bottom - DD(8));
      }
      bounds->right += border_width;
    }
    if (para->pFmt->wBorders & 4)
    {
      MoveToEx(c->hDC, c->rcView.left, y + bounds->top, NULL);
      LineTo(c->hDC, c->rcView.right, y + bounds->top);
      if (border_details[idx].dble) {
        MoveToEx(c->hDC, c->rcView.left + DD(1), y + bounds->top + pen_width + 1, NULL);
        LineTo(c->hDC, c->rcView.right - DD(2), y + bounds->top + pen_width + 1);
      }
      bounds->top += border_width;
    }
    if (para->pFmt->wBorders & 8)
    {
      MoveToEx(c->hDC, c->rcView.left, y + para->nHeight - bounds->bottom - 1, NULL);
      LineTo(c->hDC, c->rcView.right, y + para->nHeight - bounds->bottom - 1);
      if (border_details[idx].dble) {
        MoveToEx(c->hDC, c->rcView.left + DD(1), y + para->nHeight - bounds->bottom - 1 - pen_width - 1, NULL);
        LineTo(c->hDC, c->rcView.right - DD(2), y + para->nHeight - bounds->bottom - 1 - pen_width - 1);
      }
      bounds->bottom += border_width;
    }
#undef DD

    MoveToEx(c->hDC, pt.x, pt.y, NULL);
    SelectObject(c->hDC, oldpen);
    DeleteObject(pen);
  }
}

static void ME_DrawTableBorders(ME_Context *c, ME_DisplayItem *paragraph)
{
  ME_Paragraph *para = &paragraph->member.para;
  if (!c->editor->bEmulateVersion10) /* v4.1 */
  {
    if (para->pCell)
    {
      RECT rc;
      ME_Cell *cell = &para->pCell->member.cell;
      ME_DisplayItem *paraAfterRow;
      HPEN pen, oldPen;
      LOGBRUSH logBrush;
      HBRUSH brush;
      COLORREF color;
      POINT oldPt;
      int width;
      BOOL atTop = (para->pCell != para->prev_para->member.para.pCell);
      BOOL atBottom = (para->pCell != para->next_para->member.para.pCell);
      int top = c->pt.y + (atTop ? cell->pt.y : para->pt.y);
      int bottom = (atBottom ?
                    c->pt.y + cell->pt.y + cell->nHeight :
                    top + para->nHeight + (atTop ? cell->yTextOffset : 0));
      rc.left = c->pt.x + cell->pt.x;
      rc.right = rc.left + cell->nWidth;
      if (atTop) {
        /* Erase gap before text if not all borders are the same height. */
        width = max(ME_twips2pointsY(c, cell->border.top.width), 1);
        rc.top = top + width;
        width = cell->yTextOffset - width;
        rc.bottom = rc.top + width;
        if (width) {
          FillRect(c->hDC, &rc, c->editor->hbrBackground);
        }
      }
      /* Draw cell borders.
       * The order borders are draw in is left, top, bottom, right in order
       * to be consistent with native richedit.  This is noticeable from the
       * overlap of borders of different colours. */
      if (!(para->nFlags & MEPF_ROWEND)) {
        rc.top = top;
        rc.bottom = bottom;
        if (cell->border.left.width > 0)
        {
          color = cell->border.left.colorRef;
          width = max(ME_twips2pointsX(c, cell->border.left.width), 1);
        } else {
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

      if (atTop) {
        if (cell->border.top.width > 0)
        {
          brush = CreateSolidBrush(cell->border.top.colorRef);
          width = max(ME_twips2pointsY(c, cell->border.top.width), 1);
        } else {
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
      if (atBottom) {
        int oldLeft = rc.left;
        width = max(ME_twips2pointsY(c, cell->border.bottom.width), 1);
        paraAfterRow = ME_GetTableRowEnd(paragraph)->member.para.next_para;
        if (paraAfterRow->member.para.nFlags & MEPF_ROWSTART) {
          ME_DisplayItem *nextEndCell;
          nextEndCell = ME_FindItemBack(ME_GetTableRowEnd(paraAfterRow), diCell);
          assert(nextEndCell && !nextEndCell->member.cell.next_cell);
          rc.left = c->pt.x + nextEndCell->member.cell.pt.x;
          /* FIXME: Native draws FROM the bottom of the table rather than
           * TO the bottom of the table in this case, but just doing so here
           * will cause the next row to erase the border. */
          /*
          rc.top = bottom;
          rc.bottom = rc.top + width;
           */
        }
        if (rc.left < rc.right) {
          if (cell->border.bottom.width > 0) {
            brush = CreateSolidBrush(cell->border.bottom.colorRef);
          } else {
            brush = GetStockObject(LTGRAY_BRUSH);
          }
          rc.bottom = bottom;
          rc.top = rc.bottom - width;
          FillRect(c->hDC, &rc, brush);
          if (cell->border.bottom.width > 0)
            DeleteObject(brush);
        }
        rc.left = oldLeft;
      }

      /* Right border only drawn if at the end of the table row. */
      if (!cell->next_cell->member.cell.next_cell &&
          !(para->nFlags & MEPF_ROWSTART))
      {
        rc.top = top;
        rc.bottom = bottom;
        if (cell->border.right.width > 0) {
          color = cell->border.right.colorRef;
          width = max(ME_twips2pointsX(c, cell->border.right.width), 1);
        } else {
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
  } else { /* v1.0 - 3.0 */
    /* Draw simple table border */
    if (para->pFmt->dwMask & PFM_TABLE && para->pFmt->wEffects & PFE_TABLE) {
      HPEN pen = NULL, oldpen = NULL;
      int i, firstX, startX, endX, rowY, rowBottom, nHeight;
      POINT oldPt;
      PARAFORMAT2 *pNextFmt;

      pen = CreatePen(PS_SOLID, 0, para->border.top.colorRef);
      oldpen = SelectObject(c->hDC, pen);

      /* Find the start relative to the text */
      firstX = c->pt.x + ME_FindItemFwd(paragraph, diRun)->member.run.pt.x;
      /* Go back by the horizontal gap, which is stored in dxOffset */
      firstX -= ME_twips2pointsX(c, para->pFmt->dxOffset);
      /* The left edge, stored in dxStartIndent affected just the first edge */
      startX = firstX - ME_twips2pointsX(c, para->pFmt->dxStartIndent);
      rowY = c->pt.y + para->pt.y;
      if (para->pFmt->dwMask & PFM_SPACEBEFORE)
        rowY += ME_twips2pointsY(c, para->pFmt->dySpaceBefore);
      nHeight = ME_FindItemFwd(paragraph, diStartRow)->member.row.nHeight;
      rowBottom = rowY + nHeight;

      /* Draw horizontal lines */
      MoveToEx(c->hDC, firstX, rowY, &oldPt);
      i = para->pFmt->cTabCount - 1;
      endX = startX + ME_twips2pointsX(c, para->pFmt->rgxTabs[i] & 0x00ffffff) + 1;
      LineTo(c->hDC, endX, rowY);
      pNextFmt = para->next_para->member.para.pFmt;
      /* The bottom of the row only needs to be drawn if the next row is
       * not a table. */
      if (!(pNextFmt && pNextFmt->dwMask & PFM_TABLE && pNextFmt->wEffects &&
            para->nRows == 1))
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
      for (i = 0; i < para->pFmt->cTabCount; i++)
      {
        int rightBoundary = para->pFmt->rgxTabs[i] & 0x00ffffff;
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

void ME_DrawParagraph(ME_Context *c, ME_DisplayItem *paragraph)
{
  int align = SetTextAlign(c->hDC, TA_BASELINE);
  ME_DisplayItem *p;
  ME_Run *run;
  ME_Paragraph *para = NULL;
  RECT rc, bounds;
  int y;
  int height = 0, baseline = 0, no=0;
  BOOL visible = FALSE;

  rc.left = c->pt.x;
  rc.right = c->rcView.right;

  assert(paragraph);
  para = &paragraph->member.para;
  y = c->pt.y + para->pt.y;
  if (para->pCell)
  {
    ME_Cell *cell = &para->pCell->member.cell;
    rc.left = c->pt.x + cell->pt.x;
    rc.right = rc.left + cell->nWidth;
  }
  if (para->nFlags & MEPF_ROWSTART) {
    ME_Cell *cell = &para->next_para->member.para.pCell->member.cell;
    rc.right = c->pt.x + cell->pt.x;
  } else if (para->nFlags & MEPF_ROWEND) {
    ME_Cell *cell = &para->prev_para->member.para.pCell->member.cell;
    rc.left = c->pt.x + cell->pt.x + cell->nWidth;
  }
  ME_DrawParaDecoration(c, para, y, &bounds);
  y += bounds.top;
  rc.left += bounds.left;
  rc.right -= bounds.right;

  for (p = paragraph->next; p != para->next_para; p = p->next)
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
        if (visible) {
          FillRect(c->hDC, &rc, c->editor->hbrBackground);
        }
        if (me_debug)
        {
          const WCHAR wszRowDebug[] = {'r','o','w','[','%','d',']',0};
          WCHAR buf[128];
          POINT pt = c->pt;
          wsprintfW(buf, wszRowDebug, no);
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
          rc.bottom = rc.bottom + height;
          TRACE("rc = (%d, %d, %d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
          if (run->nFlags & MERF_SKIPPED)
            DrawFocusRect(c->hDC, &rc);
          else
            FrameRect(c->hDC, &rc, GetSysColorBrush(COLOR_GRAYTEXT));
        }
        if (visible)
          ME_DrawRun(c, c->pt.x + run->pt.x,
                     c->pt.y + para->pt.y + run->pt.y + baseline, p, para);
        if (me_debug)
        {
          /* I'm using %ls, hope wsprintfW is not going to use wrong (4-byte) WCHAR version */
          const WCHAR wszRunDebug[] = {'[','%','d',':','%','x',']',' ','%','l','s',0};
          WCHAR buf[2560];
          POINT pt;
          pt.x = c->pt.x + run->pt.x;
          pt.y = c->pt.y + para->pt.y + run->pt.y;
          wsprintfW(buf, wszRunDebug, no, p->member.run.nFlags, p->member.run.strText->szData);
          ME_DebugWrite(c->hDC, &pt, buf);
        }
        break;
      case diCell:
        /* Clear any space at the bottom of the cell after the text. */
        if (para->nFlags & (MEPF_ROWSTART|MEPF_ROWEND))
          break;
        y += height;
        rc.top = c->pt.y + para->pt.y + para->nHeight;
        rc.bottom = c->pt.y + p->member.cell.pt.y + p->member.cell.nHeight;
        if (RectVisible(c->hDC, &rc))
        {
          FillRect(c->hDC, &rc, c->editor->hbrBackground);
        }
        break;
      default:
        break;
    }
    no++;
  }

  ME_DrawTableBorders(c, paragraph);

  SetTextAlign(c->hDC, align);
}

void ME_ScrollAbs(ME_TextEditor *editor, int absY)
{
  ME_Scroll(editor, absY, 1);
}

void ME_ScrollUp(ME_TextEditor *editor, int cy)
{
  ME_Scroll(editor, cy, 2);
}

void ME_ScrollDown(ME_TextEditor *editor, int cy)
{ 
  ME_Scroll(editor, cy, 3);
}

void ME_Scroll(ME_TextEditor *editor, int value, int type)
{
  SCROLLINFO si;
  int nOrigPos, nNewPos, nActualScroll;
  HWND hWnd;
  LONG winStyle;
  BOOL bScrollBarIsVisible, bScrollBarWillBeVisible;

  nOrigPos = ME_GetYScrollPos(editor);
  
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS;
  
  switch (type)
  {
    case 1:
      /*Scroll absolutely*/
      si.nPos = value;
      break;
    case 2:
      /* Scroll up - towards the beginning of the document */
      si.nPos = nOrigPos - value;
      break;
    case 3:
      /* Scroll down - towards the end of the document */
      si.nPos = nOrigPos + value;
      break;
    default:
      FIXME("ME_Scroll called incorrectly\n");
      si.nPos = 0;
  }
  
  nNewPos = SetScrollInfo(editor->hWnd, SB_VERT, &si, TRUE);
  editor->vert_si.nPos = nNewPos;
  nActualScroll = nOrigPos - nNewPos;
  if (abs(nActualScroll) > editor->sizeWindow.cy)
    InvalidateRect(editor->hWnd, NULL, TRUE);
  else
    ScrollWindowEx(editor->hWnd, 0, nActualScroll, NULL, NULL, NULL, NULL, SW_INVALIDATE);
  ME_Repaint(editor);
  
  hWnd = editor->hWnd;
  winStyle = GetWindowLongW(hWnd, GWL_STYLE);
  bScrollBarIsVisible = (winStyle & WS_VSCROLL) != 0;
  bScrollBarWillBeVisible = (editor->nTotalLength > editor->sizeWindow.cy)
                            || (winStyle & ES_DISABLENOSCROLL);
  if (bScrollBarIsVisible != bScrollBarWillBeVisible)
  {
    ShowScrollBar(hWnd, SB_VERT, bScrollBarWillBeVisible);
  }
  ME_UpdateScrollBar(editor);
}

 
void ME_UpdateScrollBar(ME_TextEditor *editor)
{ 
  /* Note that this is the only function that should ever call SetScrolLInfo
   * with SIF_PAGE or SIF_RANGE. SetScrollPos and SetScrollRange should never
   * be used at all. */
  
  HWND hWnd;
  SCROLLINFO si;
  BOOL bScrollBarWasVisible,bScrollBarWillBeVisible;
  
  if (ME_WrapMarkedParagraphs(editor))
    FIXME("ME_UpdateScrollBar had to call ME_WrapMarkedParagraphs\n");
  
  hWnd = editor->hWnd;
  si.cbSize = sizeof(si);
  bScrollBarWasVisible = ME_GetYScrollVisible(editor);
  bScrollBarWillBeVisible = editor->nTotalLength > editor->sizeWindow.cy;
  
  si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
  if (GetWindowLongW(hWnd, GWL_STYLE) & ES_DISABLENOSCROLL)
  {
    si.fMask |= SIF_DISABLENOSCROLL;
    bScrollBarWillBeVisible = TRUE;
  }

  if (bScrollBarWasVisible != bScrollBarWillBeVisible)
  {
    ShowScrollBar(hWnd, SB_VERT, bScrollBarWillBeVisible);
    ME_MarkAllForWrapping(editor);
    ME_WrapMarkedParagraphs(editor);
  }
  
  si.nMin = 0;  
  si.nMax = editor->nTotalLength;
  si.nPos = editor->vert_si.nPos;
  si.nPage = editor->sizeWindow.cy;
     
  if (!(si.nMin == editor->vert_si.nMin && si.nMax == editor->vert_si.nMax && si.nPage == editor->vert_si.nPage))
  {
    TRACE("min=%d max=%d page=%d\n", si.nMin, si.nMax, si.nPage);
    editor->vert_si.nMin = si.nMin;
    editor->vert_si.nMax = si.nMax;
    editor->vert_si.nPage = si.nPage;
    if (bScrollBarWillBeVisible)
    {
      SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    }
    else
    {
      if (bScrollBarWasVisible && !(si.fMask & SIF_DISABLENOSCROLL))
      {
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        ShowScrollBar(hWnd, SB_VERT, FALSE);
        ME_ScrollAbs(editor, 0);
      }
    }
  }
}

int ME_GetYScrollPos(ME_TextEditor *editor)
{
  return editor->vert_si.nPos;
}

BOOL ME_GetYScrollVisible(ME_TextEditor *editor)
{ /* Returns true if the scrollbar is visible */
  return (editor->vert_si.nMax - editor->vert_si.nMin >= max(editor->vert_si.nPage - 1, 0));
}

void ME_EnsureVisible(ME_TextEditor *editor, ME_DisplayItem *pRun)
{
  ME_DisplayItem *pRow = ME_FindItemBack(pRun, diStartRow);
  ME_DisplayItem *pPara = ME_FindItemBack(pRun, diParagraph);
  int y, yrel, yheight, yold;
  
  assert(pRow);
  assert(pPara);
  
  y = pPara->member.para.pt.y+pRow->member.row.pt.y;
  yheight = pRow->member.row.nHeight;
  yold = ME_GetYScrollPos(editor);
  yrel = y - yold;
  
  if (y < yold)
    ME_ScrollAbs(editor,y);
  else if (yrel + yheight > editor->sizeWindow.cy) 
    ME_ScrollAbs(editor,y+yheight-editor->sizeWindow.cy);
}


void
ME_InvalidateFromOfs(ME_TextEditor *editor, int nCharOfs)
{
  RECT rc;
  int x, y, height;
  ME_Cursor tmp;

  ME_RunOfsFromCharOfs(editor, nCharOfs, &tmp.pRun, &tmp.nOffset);
  ME_GetCursorCoordinates(editor, &tmp, &x, &y, &height);

  rc.left = 0;
  rc.top = y;
  rc.bottom = y + height;
  rc.right = editor->rcFormat.right;
  InvalidateRect(editor->hWnd, &rc, FALSE);
}


void
ME_InvalidateSelection(ME_TextEditor *editor)
{
  ME_DisplayItem *para1, *para2;
  int nStart, nEnd;
  int len = ME_GetTextLength(editor);

  ME_GetSelection(editor, &nStart, &nEnd);
  /* if both old and new selection are 0-char (= caret only), then
  there's no (inverted) area to be repainted, neither old nor new */
  if (nStart == nEnd && editor->nLastSelStart == editor->nLastSelEnd)
    return;
  ME_WrapMarkedParagraphs(editor);
  ME_GetSelectionParas(editor, &para1, &para2);
  assert(para1->type == diParagraph);
  assert(para2->type == diParagraph);
  /* last selection markers aren't always updated, which means
  they can point past the end of the document */ 
  if (editor->nLastSelStart > len || editor->nLastSelEnd > len) {
    ME_MarkForPainting(editor,
        ME_FindItemFwd(editor->pBuffer->pFirst, diParagraph),
        ME_FindItemFwd(editor->pBuffer->pFirst, diTextEnd));
  } else {
    /* if the start part of selection is being expanded or contracted... */
    if (nStart < editor->nLastSelStart) {
      ME_MarkForPainting(editor, para1, ME_FindItemFwd(editor->pLastSelStartPara, diParagraphOrEnd));
    } else
    if (nStart > editor->nLastSelStart) {
      ME_MarkForPainting(editor, editor->pLastSelStartPara, ME_FindItemFwd(para1, diParagraphOrEnd));
    }

    /* if the end part of selection is being contracted or expanded... */
    if (nEnd < editor->nLastSelEnd) {
      ME_MarkForPainting(editor, para2, ME_FindItemFwd(editor->pLastSelEndPara, diParagraphOrEnd));
    } else
    if (nEnd > editor->nLastSelEnd) {
      ME_MarkForPainting(editor, editor->pLastSelEndPara, ME_FindItemFwd(para2, diParagraphOrEnd));
    }
  }

  ME_InvalidateMarkedParagraphs(editor);
  /* remember the last invalidated position */
  ME_GetSelection(editor, &editor->nLastSelStart, &editor->nLastSelEnd);
  ME_GetSelectionParas(editor, &editor->pLastSelStartPara, &editor->pLastSelEndPara);
  assert(editor->pLastSelStartPara->type == diParagraph);
  assert(editor->pLastSelEndPara->type == diParagraph);
}

void
ME_QueueInvalidateFromCursor(ME_TextEditor *editor, int nCursor)
{
  editor->nInvalidOfs = ME_GetCursorOfs(editor, nCursor);
}


BOOL
ME_SetZoom(ME_TextEditor *editor, int numerator, int denominator)
{
  /* TODO: Zoom images and objects */

  if (numerator != 0)
  {
    if (denominator == 0)
      return FALSE;
    if (1.0 / 64.0 > (float)numerator / (float)denominator
        || (float)numerator / (float)denominator > 64.0)
      return FALSE;
  }
  
  editor->nZoomNumerator = numerator;
  editor->nZoomDenominator = denominator;
  
  ME_RewrapRepaint(editor);
  return TRUE;
}
