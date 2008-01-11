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
        bPaint = c.pt.y<rcUpdate->bottom && 
          c.pt.y+item->member.para.nHeight>rcUpdate->top;
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
    if (ys == c.pt.y) /* don't overwrite the top bar */
      ys++;
  }
  if (editor->nTotalLength != editor->nLastTotalLength)
    ME_SendRequestResize(editor, FALSE);
  editor->nLastTotalLength = editor->nTotalLength;
  ME_DestroyContext(&c);
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
  
  if (ME_WrapMarkedParagraphs(editor))
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


static void ME_DrawTextWithStyle(ME_Context *c, int x, int y, LPCWSTR szText, int nChars, 
  ME_Style *s, int *width, int nSelFrom, int nSelTo, int ymin, int cy) {
  HDC hDC = c->hDC;
  HGDIOBJ hOldFont;
  COLORREF rgbOld, rgbBack;
  int yOffset = 0, yTwipsOffset = 0;
  hOldFont = ME_SelectStyleFont(c->editor, hDC, s);
  rgbBack = ME_GetBackColor(c->editor);
  if ((s->fmt.dwMask & CFM_LINK) && (s->fmt.dwEffects & CFE_LINK))
    rgbOld = SetTextColor(hDC, RGB(0,0,255));  
  else if ((s->fmt.dwMask & CFM_COLOR) && (s->fmt.dwEffects & CFE_AUTOCOLOR))
    rgbOld = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
  else
    rgbOld = SetTextColor(hDC, s->fmt.crTextColor);
  if ((s->fmt.dwMask & s->fmt.dwEffects) & CFM_OFFSET) {
    yTwipsOffset = s->fmt.yOffset;
  }
  if ((s->fmt.dwMask & s->fmt.dwEffects) & (CFM_SUPERSCRIPT | CFM_SUBSCRIPT)) {
    if (s->fmt.dwEffects & CFE_SUPERSCRIPT) yTwipsOffset = s->fmt.yHeight/3;
    if (s->fmt.dwEffects & CFE_SUBSCRIPT) yTwipsOffset = -s->fmt.yHeight/12;
  }
  if (yTwipsOffset)
  {
    int numerator = 1;
    int denominator = 1;
    
    if (c->editor->nZoomNumerator)
    {
      numerator = c->editor->nZoomNumerator;
      denominator = c->editor->nZoomDenominator;
    }
    yOffset = yTwipsOffset * GetDeviceCaps(hDC, LOGPIXELSY) * numerator / denominator / 1440;
  }
  ExtTextOutW(hDC, x, y-yOffset, 0, NULL, szText, nChars, NULL);
  if (width) {
    SIZE sz;
    GetTextExtentPoint32W(hDC, szText, nChars, &sz);
    *width = sz.cx;
  }
  if (nSelFrom < nChars && nSelTo >= 0 && nSelFrom<nSelTo)
  {
    SIZE sz;
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
  ME_UnselectStyleFont(c->editor, hDC, s, hOldFont);
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

static void ME_DrawGraphics(ME_Context *c, int x, int y, ME_Run *run,
                            ME_Paragraph *para, BOOL selected) {
  SIZE sz;
  int xs, ys, xe, ye, h, ym, width, eyes;
  ME_GetGraphicsSize(c->editor, run, &sz);
  xs = run->pt.x;
  ys = y-sz.cy;
  xe = xs+sz.cx;
  ye = y;
  h = ye-ys;
  ym = ys+h/4;
  width = sz.cx;
  eyes = width/8;
  /* draw a smiling face :) */
  Ellipse(c->hDC, xs, ys, xe, ye);
  Ellipse(c->hDC, xs+width/8, ym, x+width/8+eyes, ym+eyes);
  Ellipse(c->hDC, xs+7*width/8-eyes, ym, xs+7*width/8, ym+eyes);
  MoveToEx(c->hDC, xs+width/8, ys+3*h/4-eyes, NULL);
  LineTo(c->hDC, xs+width/8, ys+3*h/4);
  LineTo(c->hDC, xs+7*width/8, ys+3*h/4);
  LineTo(c->hDC, xs+7*width/8, ys+3*h/4-eyes);
  if (selected)
  {
    /* descent is usually (always?) 0 for graphics */
    PatBlt(c->hDC, x, y-run->nAscent, sz.cx, run->nAscent+run->nDescent, DSTINVERT);    
  }
}

static void ME_DrawRun(ME_Context *c, int x, int y, ME_DisplayItem *rundi, ME_Paragraph *para) 
{
  ME_Run *run = &rundi->member.run;
  ME_DisplayItem *start = ME_FindItemBack(rundi, diStartRow);
  int runofs = run->nCharOfs+para->nCharOfs;
  int nSelFrom, nSelTo;
  const WCHAR wszSpace[] = {' ', 0};
  
  if (run->nFlags & MERF_HIDDEN)
    return;

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
    ME_DrawGraphics(c, x, y, run, para, (runofs >= nSelFrom) && (runofs < nSelTo));
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

COLORREF ME_GetBackColor(const ME_TextEditor *editor)
{
/* Looks like I was seriously confused
    return GetSysColor((GetWindowLong(editor->hWnd, GWL_STYLE) & ES_READONLY) ? COLOR_3DFACE: COLOR_WINDOW);
*/
  if (editor->rgbBackColor == -1)
    return GetSysColor(COLOR_WINDOW);
  else
    return editor->rgbBackColor;
}

void ME_DrawParagraph(ME_Context *c, ME_DisplayItem *paragraph) {
  int align = SetTextAlign(c->hDC, TA_BASELINE);
  ME_DisplayItem *p;
  ME_Run *run;
  ME_Paragraph *para = NULL;
  RECT rc, rcPara;
  int y = c->pt.y;
  int height = 0, baseline = 0, no=0, pno = 0;
  int xs, xe;
  int visible = 0;
  int nMargWidth = 0;
  
  c->pt.x = c->rcView.left;
  rcPara.left = c->rcView.left;
  rcPara.right = c->rcView.right;
  for (p = paragraph; p!=paragraph->member.para.next_para; p = p->next) {
    switch(p->type) {
      case diParagraph:
        para = &p->member.para;
        break;
      case diStartRow:
        assert(para);
        nMargWidth = (pno==0?para->nFirstMargin:para->nLeftMargin);
        xs = c->rcView.left+nMargWidth;
        xe = c->rcView.right-para->nRightMargin;
        y += height;
        rcPara.top = y;
        rcPara.bottom = y+p->member.row.nHeight;
        visible = RectVisible(c->hDC, &rcPara);
        if (visible) {
          HBRUSH hbr;
          hbr = CreateSolidBrush(ME_GetBackColor(c->editor));
          /* left margin */
          rc.left = c->rcView.left;
          rc.right = c->rcView.left+nMargWidth;
          rc.top = y;
          rc.bottom = y+p->member.row.nHeight;
          FillRect(c->hDC, &rc, hbr/* c->hbrMargin */);
          /* right margin */
          rc.left = xe;
          rc.right = c->rcView.right;
          FillRect(c->hDC, &rc, hbr/* c->hbrMargin */);
          rc.left = c->rcView.left+nMargWidth;
          rc.right = xe;
          FillRect(c->hDC, &rc, hbr);
          DeleteObject(hbr);
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
        pno++;
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
      /*Scroll absolutly*/
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
  /* Note that this is the only funciton that should ever call SetScrolLInfo 
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
  GetScrollInfo(editor->hWnd, SB_VERT, &si);
  return si.nPos;
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
  if (editor->nLastSelStart > len)
    editor->nLastSelEnd = len; 
  if (editor->nLastSelEnd > len)
    editor->nLastSelEnd = len; 
    
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
