/*
 * RichEdit - operations on runs (diRun, rectangular pieces of paragraphs).
 * Splitting/joining runs. Adjusting offsets after deleting/adding content.
 * Character/pixel conversions.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

int ME_CanJoinRuns(ME_Run *run1, ME_Run *run2)
{
  if ((run1->nFlags | run2->nFlags) & (MERF_ENDPARA | MERF_GRAPHICS))
    return 0;
  if (run1->style != run2->style)
    return 0;
  if ((run1->nFlags & MERF_STYLEFLAGS) != (run2->nFlags & MERF_STYLEFLAGS))
    return 0;
  return 1;
}

void ME_SkipAndPropagateCharOffset(ME_DisplayItem *p, int shift)
{
  p = ME_FindItemFwd(p, diRunOrParagraphOrEnd);
  assert(p);
  ME_PropagateCharOffset(p, shift);
}

void ME_PropagateCharOffset(ME_DisplayItem *p, int shift)
{
  if (p->type == diRun) /* propagate in all runs in this para */
  {
    TRACE("PropagateCharOffset(%s, %d)\n", debugstr_w(p->member.run.strText->szData), shift);
    do {
      p->member.run.nCharOfs += shift;
      assert(p->member.run.nCharOfs >= 0);
      p = ME_FindItemFwd(p, diRunOrParagraphOrEnd);
    } while(p->type == diRun);
  }
  if (p->type == diParagraph) /* propagate in all next paras */
  {
    do {
      p->member.para.nCharOfs += shift;
      assert(p->member.para.nCharOfs >= 0);
      p = p->member.para.next_para;
    } while(p->type == diParagraph);
  }
  if (p->type == diTextEnd)
  {
    p->member.para.nCharOfs += shift;
    assert(p->member.para.nCharOfs >= 0);
  }
}

void ME_CheckCharOffsets(ME_TextEditor *editor)
{
  ME_DisplayItem *p = editor->pBuffer->pFirst;
  int ofs = 0, ofsp = 0;
  if(TRACE_ON(richedit))
  {
    TRACE("---\n");
    ME_DumpDocument(editor->pBuffer);
  }
  do {
    p = ME_FindItemFwd(p, diRunOrParagraphOrEnd);
    switch(p->type) {
      case diTextEnd:
        TRACE("tend, real ofsp = %d, counted = %d\n", p->member.para.nCharOfs, ofsp+ofs);
        assert(ofsp+ofs == p->member.para.nCharOfs);
        return;
      case diParagraph:
        TRACE("para, real ofsp = %d, counted = %d\n", p->member.para.nCharOfs, ofsp+ofs);
        assert(ofsp+ofs == p->member.para.nCharOfs);
        ofsp = p->member.para.nCharOfs;
        ofs = 0;
        break;
      case diRun:
        TRACE("run, real ofs = %d (+ofsp = %d), counted = %d, len = %d, txt = \"%s\", flags=%08x, fx&mask = %08lx\n", 
          p->member.run.nCharOfs, p->member.run.nCharOfs+ofsp, ofsp+ofs, 
          p->member.run.strText->nLen, debugstr_w(p->member.run.strText->szData),
          p->member.run.nFlags,
          p->member.run.style->fmt.dwMask & p->member.run.style->fmt.dwEffects);
        assert(ofs == p->member.run.nCharOfs);
        ofs += ME_StrLen(p->member.run.strText);
        break;
      default:
        assert(0);
    }
  } while(1);
}

int ME_CharOfsFromRunOfs(ME_TextEditor *editor, ME_DisplayItem *pRun, int nOfs)
{
  ME_DisplayItem *pPara;
  
  assert(pRun->type == diRun);
  assert(pRun->member.run.nCharOfs != -1);

  pPara = ME_FindItemBack(pRun, diParagraph);
  assert(pPara);
  assert(pPara->type==diParagraph);
  return pPara->member.para.nCharOfs + pRun->member.run.nCharOfs 
    + ME_VPosToPos(pRun->member.run.strText, nOfs);
}

void ME_CursorFromCharOfs(ME_TextEditor *editor, int nCharOfs, ME_Cursor *pCursor)
{
  ME_RunOfsFromCharOfs(editor, nCharOfs, &pCursor->pRun, &pCursor->nOffset);
}

void ME_RunOfsFromCharOfs(ME_TextEditor *editor, int nCharOfs, ME_DisplayItem **ppRun, int *pOfs)
{
  ME_DisplayItem *pPara;
  int nParaOfs;
  
  pPara = editor->pBuffer->pFirst->member.para.next_para;
  assert(pPara);
  assert(ppRun);
  assert(pOfs);
  while (pPara->type == diParagraph)
  {
    nParaOfs = pPara->member.para.nCharOfs;
    assert(nCharOfs >= nParaOfs);
    
    if (nCharOfs < pPara->member.para.next_para->member.para.nCharOfs)
    {
      *ppRun = ME_FindItemFwd(pPara, diRun);
      assert(*ppRun);      
      while (!((*ppRun)->member.run.nFlags & MERF_ENDPARA))
      {
        ME_DisplayItem *pNext = ME_FindItemFwd(*ppRun, diRun);
        assert(pNext);
        assert(pNext->type == diRun);
        if (nCharOfs < nParaOfs + pNext->member.run.nCharOfs) {
          *pOfs = ME_PosToVPos((*ppRun)->member.run.strText, 
            nCharOfs - nParaOfs - (*ppRun)->member.run.nCharOfs);
          return;
        }
        *ppRun = pNext;
      }
      if (nCharOfs == nParaOfs + (*ppRun)->member.run.nCharOfs) {
        *pOfs = 0;
        return;
      }        
    }
    pPara = pPara->member.para.next_para;
  }
  *ppRun = ME_FindItemBack(editor->pBuffer->pLast, diRun);
  *pOfs = 0;  
  assert((*ppRun)->member.run.nFlags & MERF_ENDPARA);
}

void ME_JoinRuns(ME_TextEditor *editor, ME_DisplayItem *p)
{
  ME_DisplayItem *pNext = p->next;
  int i;
  assert(p->type == diRun && pNext->type == diRun);
  assert(p->member.run.nCharOfs != -1);
  ME_GetParagraph(p)->member.para.nFlags |= MEPF_REWRAP;

  for (i=0; i<editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == pNext) {
      editor->pCursors[i].pRun = p;
      editor->pCursors[i].nOffset += ME_StrVLen(p->member.run.strText);
    }
  }
  
  ME_AppendString(p->member.run.strText, pNext->member.run.strText);
  ME_Remove(pNext);
  ME_DestroyDisplayItem(pNext);
  ME_UpdateRunFlags(editor, &p->member.run);
  if(TRACE_ON(richedit))
  {
    TRACE("Before check after join\n");
    ME_CheckCharOffsets(editor);
    TRACE("After check after join\n");
  }
}

ME_DisplayItem *ME_SplitRun(ME_Context *c, ME_DisplayItem *item, int nVChar)
{
  ME_TextEditor *editor = c->editor;
  ME_DisplayItem *item2 = NULL;
  ME_Run *run, *run2;
  
  assert(item->member.run.nCharOfs != -1);
  if(TRACE_ON(richedit))
  {
    TRACE("Before check before split\n");
    ME_CheckCharOffsets(editor);
    TRACE("After check before split\n");
  }

  run = &item->member.run;

  TRACE("Before split: %s(%ld, %ld)\n", debugstr_w(run->strText->szData),
        run->pt.x, run->pt.y);

  item2 = ME_SplitRunSimple(editor, item, nVChar);
  
  run2 = &item2->member.run;
  
  ME_CalcRunExtent(c, run);
  ME_CalcRunExtent(c, run2);
    
  run2->pt.x = run->pt.x+run->nWidth;
  run2->pt.y = run->pt.y;
  
  if(TRACE_ON(richedit))
  {
    TRACE("Before check after split\n");
    ME_CheckCharOffsets(editor);
    TRACE("After check after split\n");
    TRACE("After split: %s(%ld, %ld), %s(%ld, %ld)\n", 
      debugstr_w(run->strText->szData), run->pt.x, run->pt.y,
      debugstr_w(run2->strText->szData), run2->pt.x, run2->pt.y);
  }

  return item2;
}
  
/* split a run starting from voffset */
ME_DisplayItem *ME_SplitRunSimple(ME_TextEditor *editor, ME_DisplayItem *item, int nVChar)
{
  ME_Run *run = &item->member.run;
  ME_DisplayItem *item2;
  ME_Run *run2;
  int i;
  assert(nVChar > 0 && nVChar < ME_StrVLen(run->strText));
  assert(item->type == diRun);
  assert(!(item->member.run.nFlags & MERF_GRAPHICS));
  assert(item->member.run.nCharOfs != -1);

  item2 = ME_MakeRun(run->style, 
      ME_VSplitString(run->strText, nVChar), run->nFlags&MERF_SPLITMASK);
  
  item2->member.run.nCharOfs = item->member.run.nCharOfs+
    ME_VPosToPos(item->member.run.strText, nVChar);

  run2 = &item2->member.run;
  ME_InsertBefore(item->next, item2);
  
  ME_UpdateRunFlags(editor, run);
  ME_UpdateRunFlags(editor, run2);
  for (i=0; i<editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == item && 
        editor->pCursors[i].nOffset >= nVChar) {
      assert(item2->type == diRun);
      editor->pCursors[i].pRun = item2;
      editor->pCursors[i].nOffset -= nVChar;
    }
  }
  ME_GetParagraph(item)->member.para.nFlags |= MEPF_REWRAP;
  return item2;
}

/* split the start and final whitespace into separate runs */
/* returns the last run added */
/*
ME_DisplayItem *ME_SplitFurther(ME_TextEditor *editor, ME_DisplayItem *item)
{
  int i, nVLen, nChanged;
  assert(item->type == diRun);
  assert(!(item->member.run.nFlags & MERF_GRAPHICS));
  return item;
}
*/

ME_DisplayItem *ME_MakeRun(ME_Style *s, ME_String *strData, int nFlags)
{
  ME_DisplayItem *item = ME_MakeDI(diRun);
  item->member.run.style = s;
  item->member.run.strText = strData;
  item->member.run.nFlags = nFlags;
  item->member.run.nCharOfs = -1;
  ME_AddRefStyle(s);
  return item;
}

ME_DisplayItem *ME_InsertRun(ME_TextEditor *editor, int nCharOfs, ME_DisplayItem *pItem)
{
  ME_Cursor tmp;
  ME_DisplayItem *pDI;
  ME_UndoItem *pUI;
  
  assert(pItem->type == diRun || pItem->type == diUndoInsertRun);
  
  pUI = ME_AddUndoItem(editor, diUndoDeleteRun, NULL);
  pUI->nStart = nCharOfs;
  pUI->nLen = pItem->member.run.strText->nLen;
  ME_CursorFromCharOfs(editor, nCharOfs, &tmp);
  if (tmp.nOffset) {
    tmp.pRun = ME_SplitRunSimple(editor, tmp.pRun, tmp.nOffset);
    tmp.nOffset = 0;
  }
  pDI = ME_MakeRun(pItem->member.run.style, ME_StrDup(pItem->member.run.strText), pItem->member.run.nFlags);
  pDI->member.run.nCharOfs = tmp.pRun->member.run.nCharOfs;
  ME_InsertBefore(tmp.pRun, pDI);
  TRACE("Shift length:%d\n", pDI->member.run.strText->nLen);
  ME_PropagateCharOffset(tmp.pRun, pDI->member.run.strText->nLen);
  ME_GetParagraph(tmp.pRun)->member.para.nFlags |= MEPF_REWRAP;
  
  return pDI;
}

static inline int ME_IsWSpace(WCHAR ch)
{
  return ch <= ' ';
}

void ME_UpdateRunFlags(ME_TextEditor *editor, ME_Run *run)
{
  assert(run->nCharOfs != -1);
  if (ME_IsSplitable(run->strText))
    run->nFlags |= MERF_SPLITTABLE;
  else
    run->nFlags &= ~MERF_SPLITTABLE;
    
  if (!(run->nFlags & MERF_GRAPHICS)) {
    if (ME_IsWhitespaces(run->strText))
      run->nFlags |= MERF_WHITESPACE | MERF_STARTWHITE | MERF_ENDWHITE;
    else
    {
      run->nFlags &= ~MERF_WHITESPACE;
    
      if (ME_IsWSpace(ME_GetCharFwd(run->strText,0)))
        run->nFlags |= MERF_STARTWHITE;
      else
        run->nFlags &= ~MERF_STARTWHITE;
    
      if (ME_IsWSpace(ME_GetCharBack(run->strText,0)))
        run->nFlags |= MERF_ENDWHITE;
      else
        run->nFlags &= ~MERF_ENDWHITE;
    }
  }
  else
    run->nFlags &= ~(MERF_WHITESPACE | MERF_STARTWHITE | MERF_ENDWHITE);
}

void ME_GetGraphicsSize(ME_TextEditor *editor, ME_Run *run, SIZE *pSize)
{
  assert(run->nFlags & MERF_GRAPHICS);
  pSize->cx = 64;
  pSize->cy = 64;
}

int ME_CharFromPoint(ME_TextEditor *editor, int cx, ME_Run *run)
{
  int fit = 0;
  HGDIOBJ hOldFont;
  HDC hDC;
  SIZE sz;
  if (!run->strText->nLen)
    return 0;

  if (run->nFlags & MERF_GRAPHICS)
  {
    SIZE sz;
    ME_GetGraphicsSize(editor, run, &sz);
    if (cx < sz.cx)
      return 0;
    return 1;
  }
  hDC = GetDC(editor->hWnd);
  hOldFont = ME_SelectStyleFont(editor, hDC, run->style);
  GetTextExtentExPointW(hDC, run->strText->szData, run->strText->nLen, 
    cx, &fit, NULL, &sz);
  ME_UnselectStyleFont(editor, hDC, run->style, hOldFont);  
  ReleaseDC(editor->hWnd, hDC);
  return fit;
}

int ME_CharFromPointCursor(ME_TextEditor *editor, int cx, ME_Run *run)
{
  int fit = 0, fit1 = 0;
  HGDIOBJ hOldFont;
  HDC hDC;
  SIZE sz, sz2, sz3;
  if (!run->strText->nLen)
    return 0;

  if (run->nFlags & MERF_GRAPHICS)
  {
    SIZE sz;
    ME_GetGraphicsSize(editor, run, &sz);
    if (cx < sz.cx/2)
      return 0;
    return 1;
  }

  hDC = GetDC(editor->hWnd);
  hOldFont = ME_SelectStyleFont(editor, hDC, run->style);
  GetTextExtentExPointW(hDC, run->strText->szData, run->strText->nLen, 
    cx, &fit, NULL, &sz);
  if (fit != run->strText->nLen)
  {
    int chars = 1;
    
    GetTextExtentPoint32W(hDC, run->strText->szData, fit, &sz2);
    fit1 = ME_StrRelPos(run->strText, fit, &chars);
    GetTextExtentPoint32W(hDC, run->strText->szData, fit1, &sz3);
    if (cx >= (sz2.cx+sz3.cx)/2)
      fit = fit1;
  }
  ME_UnselectStyleFont(editor, hDC, run->style, hOldFont);  
  ReleaseDC(editor->hWnd, hDC);
  return fit;
}

int ME_PointFromChar(ME_TextEditor *editor, ME_Run *pRun, int nOffset)
{
  SIZE size;
  HDC hDC = GetDC(editor->hWnd);
  HGDIOBJ hOldFont;
  
  if (pRun->nFlags & MERF_GRAPHICS)
  {
    if (!nOffset) return 0;
    ME_GetGraphicsSize(editor, pRun, &size);
    return 1;
  }
  hOldFont = ME_SelectStyleFont(editor, hDC, pRun->style);
  GetTextExtentPoint32W(hDC, pRun->strText->szData, nOffset, &size);
  ME_UnselectStyleFont(editor, hDC, pRun->style, hOldFont);  
  ReleaseDC(editor->hWnd, hDC);
  return size.cx;
}

void ME_GetTextExtent(ME_Context *c, LPCWSTR szText, int nChars, ME_Style *s, 
  SIZE *size)
{
  HDC hDC = c->hDC;
  HGDIOBJ hOldFont;
  hOldFont = ME_SelectStyleFont(c->editor, hDC, s);
  GetTextExtentPoint32W(hDC, szText, nChars, size);
  ME_UnselectStyleFont(c->editor, hDC, s, hOldFont);  
}

SIZE ME_GetRunSize(ME_Context *c, ME_Run *run, int nLen)
{
  SIZE size;
  int nMaxLen = ME_StrVLen(run->strText);

  if (nLen>nMaxLen)
    nLen = nMaxLen;
    
  if (run->nFlags & MERF_GRAPHICS)
  {
    ME_GetGraphicsSize(c->editor, run, &size);
    return size;
  }

  ME_GetTextExtent(c, run->strText->szData, nLen, run->style, &size);

  return size;
}

void ME_CalcRunExtent(ME_Context *c, ME_Run *run)
{
  SIZE size;
  int nEnd = ME_StrVLen(run->strText);
  
  if (run->nFlags & MERF_GRAPHICS) {
    ME_GetGraphicsSize(c->editor, run, &size);
    run->nWidth = size.cx;
    run->nAscent = size.cy;
    run->nDescent = 0;
    return;
  }
  ME_GetTextExtent(c, run->strText->szData, nEnd, run->style, &size);
  run->nWidth = size.cx;
  run->nAscent = run->style->tm.tmAscent;
  run->nDescent = run->style->tm.tmDescent;
}

void ME_MustBeWrapped(ME_Context *c, ME_DisplayItem *para)
{
  assert(para->type == diParagraph);
  /* FIXME */
}

void ME_SetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt)
{
  int nFrom, nTo;
  ME_GetSelection(editor, &nFrom, &nTo);
  if (nFrom == nTo)
  {
    ME_Style *s;
    if (!editor->pBuffer->pCharStyle)
      editor->pBuffer->pCharStyle = ME_GetInsertStyle(editor, 0);
    s = ME_ApplyStyle(editor->pBuffer->pCharStyle, pFmt);
    ME_ReleaseStyle(editor->pBuffer->pCharStyle);
    editor->pBuffer->pCharStyle = s;
  }
  else
    ME_SetCharFormat(editor, nFrom, nTo-nFrom, pFmt);
}

void ME_SetCharFormat(ME_TextEditor *editor, int nOfs, int nChars, CHARFORMAT2W *pFmt)
{
  ME_Cursor tmp, tmp2;
  ME_DisplayItem *para;
  
  ME_CursorFromCharOfs(editor, nOfs, &tmp);
  if (tmp.nOffset)
    tmp.pRun = ME_SplitRunSimple(editor, tmp.pRun, tmp.nOffset);

  ME_CursorFromCharOfs(editor, nOfs+nChars, &tmp2);
  if (tmp2.nOffset)
    tmp2.pRun = ME_SplitRunSimple(editor, tmp2.pRun, tmp2.nOffset);

  para = ME_GetParagraph(tmp.pRun);
  para->member.para.nFlags |= MEPF_REWRAP;
    
  while(tmp.pRun != tmp2.pRun)
  {
    ME_UndoItem *undo = NULL;
    ME_Style *new_style = ME_ApplyStyle(tmp.pRun->member.run.style, pFmt);
    /* ME_DumpStyle(new_style); */
    undo = ME_AddUndoItem(editor, diUndoSetCharFormat, NULL);
    if (undo) {
      undo->nStart = tmp.pRun->member.run.nCharOfs+para->member.para.nCharOfs;
      undo->nLen = tmp.pRun->member.run.strText->nLen;
      undo->di.member.ustyle = tmp.pRun->member.run.style;
      /* we'd have to addref undo..ustyle and release tmp...style
         but they'd cancel each other out so we can do nothing instead */
    }
    else
      ME_ReleaseStyle(tmp.pRun->member.run.style);
    tmp.pRun->member.run.style = new_style;
    tmp.pRun = ME_FindItemFwd(tmp.pRun, diRunOrParagraph);
    if (tmp.pRun->type == diParagraph)
    {
      para = tmp.pRun;
      tmp.pRun = ME_FindItemFwd(tmp.pRun, diRun);
      if (tmp.pRun != tmp2.pRun)
        para->member.para.nFlags |= MEPF_REWRAP;
    }
    assert(tmp.pRun);
  }
}

void ME_SetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *mod)
{
  ME_Style *style; 
  ME_UndoItem *undo;
  
  assert(mod->cbSize == sizeof(CHARFORMAT2W));
  undo = ME_AddUndoItem(editor, diUndoSetDefaultCharFormat, NULL);
  if (undo) {
    undo->nStart = -1;
    undo->nLen = -1;
    undo->di.member.ustyle = editor->pBuffer->pDefaultStyle;
    ME_AddRefStyle(undo->di.member.ustyle);
  }
  style = ME_ApplyStyle(editor->pBuffer->pDefaultStyle, mod);
  editor->pBuffer->pDefaultStyle->fmt = style->fmt;
  editor->pBuffer->pDefaultStyle->tm = style->tm;
  ME_ReleaseStyle(style);
  ME_MarkAllForWrapping(editor);
  /*  pcf = editor->pBuffer->pDefaultStyle->fmt; */
}

void ME_GetRunCharFormat(ME_TextEditor *editor, ME_DisplayItem *run, CHARFORMAT2W *pFmt)
{
  ME_CopyCharFormat(pFmt, &run->member.run.style->fmt);
}

void ME_GetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt)
{
  int nFrom, nTo;
  ME_GetSelection(editor, &nFrom, &nTo);
  ME_CopyCharFormat(pFmt, &editor->pBuffer->pDefaultStyle->fmt);
}

void ME_GetSelectionCharFormat(ME_TextEditor *editor, CHARFORMAT2W *pFmt)
{
  int nFrom, nTo;
  ME_GetSelection(editor, &nFrom, &nTo);
  if (nFrom == nTo && editor->pBuffer->pCharStyle)
  {
    ME_CopyCharFormat(pFmt, &editor->pBuffer->pCharStyle->fmt);
    return;
  }
  ME_GetCharFormat(editor, nFrom, nTo, pFmt);
}

void ME_GetCharFormat(ME_TextEditor *editor, int nFrom, int nTo, CHARFORMAT2W *pFmt)
{
  ME_DisplayItem *run, *run_end;
  int nOffset, nOffset2;
  CHARFORMAT2W tmp;
  
  if (nTo>nFrom) /* selection consists of chars from nFrom up to nTo-1 */
    nTo--;
  
  ME_RunOfsFromCharOfs(editor, nFrom, &run, &nOffset);
  if (nFrom == nTo) /* special case - if selection is empty, take previous char's formatting */
  {
    if (!nOffset)
    {
      ME_DisplayItem *tmp_run = ME_FindItemBack(run, diRunOrParagraph);
      if (tmp_run->type == diRun) {
        ME_GetRunCharFormat(editor, tmp_run, pFmt);
        return;
      }
    }
    ME_GetRunCharFormat(editor, run, pFmt);
    return;
  }
  ME_RunOfsFromCharOfs(editor, nTo, &run_end, &nOffset2);
  
  ME_GetRunCharFormat(editor, run, pFmt);
  
  if (run == run_end) return;
  
  do {
    /* FIXME add more style feature comparisons */
    int nAttribs = CFM_SIZE | CFM_FACE | CFM_COLOR;
    int nEffects = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;

    run = ME_FindItemFwd(run, diRun);
    
    ZeroMemory(&tmp, sizeof(tmp));
    tmp.cbSize = sizeof(tmp);
    ME_GetRunCharFormat(editor, run, &tmp);
    
    assert((tmp.dwMask & nAttribs) == nAttribs);
    assert((tmp.dwMask & nEffects) == nEffects);
    /* reset flags that differ */

    if (pFmt->yHeight != tmp.yHeight)
      pFmt->dwMask &= ~CFM_SIZE;
    if (pFmt->dwMask & CFM_FACE)
    {
      if (!(tmp.dwMask & CFM_FACE))
        pFmt->dwMask &= ~CFM_FACE;
      else if (lstrcmpW(pFmt->szFaceName, tmp.szFaceName))
        pFmt->dwMask &= ~CFM_FACE;
    }
    if (pFmt->yHeight != tmp.yHeight)
      pFmt->dwMask &= ~CFM_SIZE;
    if (pFmt->dwMask & CFM_COLOR)
    {
      if (!((pFmt->dwEffects&CFE_AUTOCOLOR) & (tmp.dwEffects&CFE_AUTOCOLOR)))
      {
        if (pFmt->crTextColor != tmp.crTextColor)
          pFmt->dwMask &= ~CFM_COLOR;
      }
    }
      
    pFmt->dwMask &= ~((pFmt->dwEffects ^ tmp.dwEffects) & nEffects);
    
  } while(run != run_end);
}
