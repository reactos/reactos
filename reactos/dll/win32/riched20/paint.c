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

void ME_PaintContent(ME_TextEditor *editor, HDC hDC, BOOL bOnlyNew, const RECT *rcUpdate) {
  ME_DisplayItem *item;
  ME_Context c;
  int yoffset;

  editor->nSequence++;
  yoffset = ME_GetYScrollPos(editor);
  ME_InitContext(&c, editor, hDC);
  SetBkMode(hDC, TRANSPARENT);
  ME_MoveCaret(editor);
  item = editor->pBuffer->pFirst->next;
  c.pt.y -= yoffset;
  while(item != editor->pBuffer->pLast) {
    int ye;
    assert(item->type == diParagraph);
    ye = c.pt.y + item->member.para.nHeight;
    if (!bOnlyNew || (item->member.para.nFlags & MEPF_REPAINT))
    {
      BOOL bPaint = (rcUpdate == NULL);
      if (rcUpdate)
        bPaint = c.pt.y<rcUpdate->bottom && ye>rcUpdate->top;
      if (bPaint)
      {
        ME_DrawParagraph(&c, item);
        if (!rcUpdate || (rcUpdate->top<=c.pt.y && rcUpdate->bottom>=ye))
          item->member.para.nFlags &= ~MEPF_REPAINT;
      }
    }
    c.pt.y = ye;
    item = item->member.para.next_para;
  }
  if (c.pt.y<c.rcView.bottom) {
    RECT rc;
    int xs = c.rcView.left, xe = c.rcView.right;
    int ys = c.pt.y, ye = c.rcView.bottom;
    
    if (bOnlyNew)
    {
      int y1 = editor->nTotalLength-yoffset, y2 = editor->nLastTotalLength-yoffset;
      if (y1<y2)
        ys = y1, ye = y2+1;
      else
        ys = ye;
    }
    
    if (rcUpdate && ys!=ye)
    {
      xs = rcUpdate->left, xe = rcUpdate->right;
      if (rcUpdate->top > ys)
        ys = rcUpdate->top;
      if (rcUpdate->bottom < ye)
        ye = rcUpdate->bottom;
    }

    if (ye>ys) {
      rc.left = xs;
      rc.top = ys;
      rc.right = xe;
      rc.bottom = ye;
      FillRect(hDC, &rc, c.editor->hbrBackground);
    }
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
  ME_SendOldNotify(editor, EN_UPDATE);
  UpdateWindow(editor->hWnd);
}

void ME_UpdateRepaint(ME_TextEditor *editor)
{
  /* Should be called whenever the contents of the control have changed */
  ME_Cursor *pCursor;
  BOOL wrappedParagraphs;

  wrappedParagraphs = ME_WrapMarkedParagraphs(editor);
  if (!editor->bRedraw) return;
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
  if (editor->bRedraw)
  {
    ME_WrapMarkedParagraphs(editor);
    ME_UpdateScrollBar(editor);
    ME_Repaint(editor);
  }
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

static void ME_DrawTextWithStyle(ME_Context *c, int x, int y, LPCWSTR szText, int nChars, 
  ME_Style *s, int *width, int nSelFrom, int nSelTo, int ymin, int cy) {
  HDC hDC = c->hDC;
  HGDIOBJ hOldFont;
  COLORREF rgbOld;
  int yOffset = 0, yTwipsOffset = 0;
  SIZE          sz;
  COLORREF      rgb;

  hOldFont = ME_SelectStyleFont(c, s);
  if ((s->fmt.dwMask & CFM_LINK) && (s->fmt.dwEffects & CFE_LINK))
    rgb = RGB(0,0,255);
  else if ((s->fmt.dwMask & CFM_COLOR) && (s->fmt.dwEffects & CFE_AUTOCOLOR))
    rgb = GetSysColor(COLOR_WINDOWTEXT);
  else
    rgb = s->fmt.crTextColor;
  rgbOld = SetTextColor(hDC, rgb);
  if ((s->fmt.dwMask & s->fmt.dwEffects) & CFM_OFFSET) {
    yTwipsOffset = s->fmt.yOffset;
  }
  if ((s->fmt.dwMask & s->fmt.dwEffects) & (CFM_SUPERSCRIPT | CFM_SUBSCRIPT)) {
    if (s->fmt.dwEffects & CFE_SUPERSCRIPT) yTwipsOffset = s->fmt.yHeight/3;
    if (s->fmt.dwEffects & CFE_SUBSCRIPT) yTwipsOffset = -s->fmt.yHeight/12;
  }
  if (yTwipsOffset)
    yOffset = ME_twips2pointsY(c, yTwipsOffset);
  ExtTextOutW(hDC, x, y-yOffset, 0, NULL, szText, nChars, NULL);
  GetTextExtentPoint32W(hDC, szText, nChars, &sz);
  if (width) *width = sz.cx;
  if (s->fmt.dwMask & CFM_UNDERLINETYPE)
  {
    HPEN    hPen;
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
    if (hPen != NULL)
    {
      HPEN hOldPen = SelectObject(hDC, hPen);
      /* FIXME: should use textmetrics info for Descent info */
      MoveToEx(hDC, x, y - yOffset + 1, NULL);
      LineTo(hDC, x + sz.cx, y - yOffset + 1);
      SelectObject(hDC, hOldPen);
      DeleteObject(hPen);
    }
  }
  if (nSelFrom < nChars && nSelTo >= 0 && nSelFrom<nSelTo)
  {
    if (nSelFrom < 0) nSelFrom = 0;
    if (nSelTo > nChars) nSelTo = nChars;
    GetTextExtentPoint32W(hDC, szText, nSelFrom, &sz);
    x += sz.cx;
    GetTextExtentPoint32W(hDC, szText+nSelFrom, nSelTo-nSelFrom, &sz);
    
    /* Invert selection if not hidden by EM_HIDESELECTION */
    if (c->editor->bHideSelection == FALSE)
	PatBlt(hDC, x, ymin, sz.cx, cy, DSTINVERT);
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
  if (run->nFlags & MERF_ENDPARA && runofs >= nSelFrom && runofs < nSelTo)
    ME_DrawTextWithStyle(c, x, y, wszSpace, 1, run->style, NULL, 0, 1,
                         c->pt.y + start->member.row.nYPos,
                         start->member.row.nHeight);
          
  /* you can always comment it out if you need visible paragraph marks */
  if (run->nFlags & (MERF_ENDPARA | MERF_TAB | MERF_CELL)) 
    return;

  if (run->nFlags & MERF_GRAPHICS)
    ME_DrawOLE(c, x, y, run, para, (runofs >= nSelFrom) && (runofs < nSelTo));
  else
  {
    if (c->editor->cPasswordMask)
    {
      ME_String *szMasked = ME_MakeStringR(c->editor->cPasswordMask,ME_StrVLen(run->strText));
      ME_DrawTextWithStyle(c, x, y, 
        szMasked->szData, ME_StrVLen(szMasked), run->style, NULL, 
	nSelFrom-runofs,nSelTo-runofs, c->pt.y+start->member.row.nYPos, start->member.row.nHeight);
      ME_DestroyString(szMasked);
    }
    else
      ME_DrawTextWithStyle(c, x, y, 
        run->strText->szData, ME_StrVLen(run->strText), run->style, NULL, 
	nSelFrom-runofs,nSelTo-runofs, c->pt.y+start->member.row.nYPos, start->member.row.nHeight);
    }
}

static struct {unsigned width_num : 4, width_den : 4, pen_style : 4, dble : 1;} border_details[] = {
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

static COLORREF         pen_colors[16] = {
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

  SetRectEmpty(bounds);
  if (!(para->pFmt->dwMask & (PFM_BORDER | PFM_SPACEBEFORE | PFM_SPACEAFTER))) return;

  border_width = top_border = bottom_border = 0;
  idx = (para->pFmt->wBorders >> 8) & 0xF;
  if ((para->pFmt->dwMask & PFM_BORDER) && idx != 0 && (para->pFmt->wBorders & 0xF))
  {
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

  if ((para->pFmt->dwMask & PFM_BORDER) && idx != 0 && (para->pFmt->wBorders & 0xF)) {
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

void ME_DrawParagraph(ME_Context *c, ME_DisplayItem *paragraph) {
  int align = SetTextAlign(c->hDC, TA_BASELINE);
  ME_DisplayItem *p;
  ME_Run *run;
  ME_Paragraph *para = NULL;
  RECT rc, rcPara, bounds;
  int y = c->pt.y;
  int height = 0, baseline = 0, no=0, pno = 0;
  int xs = 0, xe = 0;
  BOOL visible = FALSE;

  c->pt.x = c->rcView.left;
  rcPara.left = c->rcView.left;
  rcPara.right = c->rcView.right;
  for (p = paragraph; p!=paragraph->member.para.next_para; p = p->next) {
    switch(p->type) {
      case diParagraph:
        para = &p->member.para;
        assert(para);
        pno = 0;
        xs = c->rcView.left + ME_twips2pointsX(c, para->pFmt->dxStartIndent);
        xe = c->rcView.right - ME_twips2pointsX(c, para->pFmt->dxRightIndent);
        ME_DrawParaDecoration(c, para, y, &bounds);
        y += bounds.top;
        break;
      case diStartRow:
        y += height;
        rcPara.top = y;
        rcPara.bottom = y+p->member.row.nHeight;
        visible = RectVisible(c->hDC, &rcPara);
        if (visible) {
          /* left margin */
          rc.left = c->rcView.left + bounds.left;
          rc.right = xs;
          rc.top = y;
          rc.bottom = y+p->member.row.nHeight;
          FillRect(c->hDC, &rc, c->editor->hbrBackground);
          /* right margin */
          rc.left = xe;
          rc.right = c->rcView.right - bounds.right;
          FillRect(c->hDC, &rc, c->editor->hbrBackground);
          rc.left = xs;
          rc.right = xe;
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
        if (!pno++)
          xe += ME_twips2pointsX(c, para->pFmt->dxOffset);
        break;
      case diRun:
        assert(para);
        run = &p->member.run;
        if (visible && me_debug) {
          rc.left = c->rcView.left+run->pt.x;
          rc.right = c->rcView.left+run->pt.x+run->nWidth;
          rc.top = c->pt.y+run->pt.y;
          rc.bottom = c->pt.y+run->pt.y+height;
          TRACE("rc = (%d, %d, %d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
          if (run->nFlags & MERF_SKIPPED)
            DrawFocusRect(c->hDC, &rc);
          else
            FrameRect(c->hDC, &rc, GetSysColorBrush(COLOR_GRAYTEXT));
        }
        if (visible)
          ME_DrawRun(c, run->pt.x, c->pt.y+run->pt.y+baseline, p, &paragraph->member.para);
        if (me_debug)
        {
          /* I'm using %ls, hope wsprintfW is not going to use wrong (4-byte) WCHAR version */
          const WCHAR wszRunDebug[] = {'[','%','d',':','%','x',']',' ','%','l','s',0};
          WCHAR buf[2560];
          POINT pt;
          pt.x = run->pt.x;
          pt.y = c->pt.y + run->pt.y;
          wsprintfW(buf, wszRunDebug, no, p->member.run.nFlags, p->member.run.strText->szData);
          ME_DebugWrite(c->hDC, &pt, buf);
        }
        /* c->pt.x += p->member.run.nWidth; */
        break;
      default:
        break;
    }
    no++;
  }
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
  
  nNewPos = SetScrollInfo(editor->hWnd, SB_VERT, &si, editor->bRedraw);
  nActualScroll = nOrigPos - nNewPos;
  if (editor->bRedraw)
  {
    if (abs(nActualScroll) > editor->sizeWindow.cy)
      InvalidateRect(editor->hWnd, NULL, TRUE);
    else
      ScrollWindowEx(editor->hWnd, 0, nActualScroll, NULL, NULL, NULL, NULL, SW_INVALIDATE);
    ME_Repaint(editor);
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
  bScrollBarWillBeVisible = editor->nHeight > editor->sizeWindow.cy;
  
  if (bScrollBarWasVisible != bScrollBarWillBeVisible)
  {
    ShowScrollBar(hWnd, SB_VERT, bScrollBarWillBeVisible);
    ME_MarkAllForWrapping(editor);
    ME_WrapMarkedParagraphs(editor);
  }
  
  si.fMask = SIF_PAGE | SIF_RANGE;
  if (GetWindowLongW(hWnd, GWL_STYLE) & ES_DISABLENOSCROLL)
    si.fMask |= SIF_DISABLENOSCROLL;
  
  si.nMin = 0;  
  si.nMax = editor->nTotalLength;
  
  si.nPage = editor->sizeWindow.cy;
     
  TRACE("min=%d max=%d page=%d\n", si.nMin, si.nMax, si.nPage);
  SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}

int ME_GetYScrollPos(ME_TextEditor *editor)
{
  SCROLLINFO si;
  si.cbSize = sizeof(si);
  si.fMask = SIF_POS;
  return GetScrollInfo(editor->hWnd, SB_VERT, &si) ? si.nPos : 0;
}

BOOL ME_GetYScrollVisible(ME_TextEditor *editor)
{ /* Returns true if the scrollbar is visible */
  SCROLLBARINFO sbi;
  sbi.cbSize = sizeof(sbi);
  GetScrollBarInfo(editor->hWnd, OBJID_VSCROLL, &sbi);
  return ((sbi.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0);
}

void ME_EnsureVisible(ME_TextEditor *editor, ME_DisplayItem *pRun)
{
  ME_DisplayItem *pRow = ME_FindItemBack(pRun, diStartRow);
  ME_DisplayItem *pPara = ME_FindItemBack(pRun, diParagraph);
  int y, yrel, yheight, yold;
  
  assert(pRow);
  assert(pPara);
  
  y = pPara->member.para.nYPos+pRow->member.row.nYPos;
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
