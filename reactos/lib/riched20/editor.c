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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* 
  API implementation status:
  
  Messages (ANSI versions not done yet)
  - EM_AUTOURLDETECT 2.0
  + EM_CANPASTE
  + EM_CANREDO 2.0
  + EM_CANUNDO
  - EM_CHARFROMPOS
  - EM_DISPLAYBAND
  + EM_EMPTYUNDOBUFFER
  + EM_EXGETSEL
  - EM_EXLIMITTEXT
  + EM_EXLINEFROMCHAR
  + EM_EXSETSEL
  + EM_FINDTEXT (only FR_DOWN flag implemented)
  + EM_FINDTEXTEX (only FR_DOWN flag implemented)
  - EM_FINDWORDBREAK
  - EM_FMTLINES
  - EM_FORMATRANGE
  - EM_GETAUTOURLDETECT 2.0
  - EM_GETBIDIOPTIONS 3.0
  - EM_GETCHARFORMAT (partly done)
  - EM_GETEDITSTYLE
  + EM_GETEVENTMASK
  - EM_GETFIRSTVISIBLELINE
  - EM_GETIMECOLOR 1.0asian
  - EM_GETIMECOMPMODE 2.0
  - EM_GETIMEOPTIONS 1.0asian
  - EM_GETIMESTATUS
  - EM_GETLANGOPTIONS 2.0
  - EM_GETLIMITTEXT
  - EM_GETLINE        
  + EM_GETLINECOUNT   returns number of rows, not of paragraphs
  + EM_GETMODIFY
  - EM_GETOLEINTERFACE
  - EM_GETOPTIONS
  + EM_GETPARAFORMAT
  - EM_GETPASSWORDCHAR 2.0
  - EM_GETPUNCTUATION 1.0asian
  - EM_GETRECT
  - EM_GETREDONAME 2.0
  + EM_GETSEL
  + EM_GETSELTEXT (ANSI&Unicode)
  - EM_GETSCROLLPOS 3.0
! - EM_GETTHUMB
  - EM_GETTEXTEX 2.0
  + EM_GETTEXTLENGTHEX (GTL_PRECISE unimplemented)
  - EM_GETTEXTMODE 2.0
? + EM_GETTEXTRANGE (ANSI&Unicode)
  - EM_GETTYPOGRAPHYOPTIONS 3.0
  - EM_GETUNDONAME
  - EM_GETWORDBREAKPROC
  - EM_GETWORDBREAKPROCEX
  - EM_GETWORDWRAPMODE 1.0asian
  + EM_GETZOOM 3.0
  - EM_HIDESELECTION
  - EM_LIMITTEXT
  + EM_LINEFROMCHAR
  + EM_LINEINDEX
  + EM_LINELENGTH
  + EM_LINESCROLL
  - EM_PASTESPECIAL
  - EM_POSFROMCHARS
  + EM_REDO 2.0
  - EM_REQUESTRESIZE
  + EM_REPLACESEL (proper style?) ANSI&Unicode
  - EM_SCROLL
  - EM_SCROLLCARET
  - EM_SELECTIONTYPE
  - EM_SETBIDIOPTIONS 3.0
  + EM_SETBKGNDCOLOR
  - EM_SETCHARFORMAT (partly done, no ANSI)
  - EM_SETEDITSTYLE
  + EM_SETEVENTMASK (few notifications supported)
  - EM_SETFONTSIZE
  - EM_SETIMECOLOR 1.0asian
  - EM_SETIMEOPTIONS 1.0asian
  - EM_SETLANGOPTIONS 2.0
  - EM_SETLIMITTEXT
  + EM_SETMODIFY (not sure if implementation is correct)
  - EM_SETOLECALLBACK
  - EM_SETOPTIONS
  - EM_SETPALETTE 2.0
  + EM_SETPARAFORMAT
  - EM_SETPASSWORDCHAR 2.0
  - EM_SETPUNCTUATION 1.0asian
  + EM_SETREADONLY no beep on modification attempt
  - EM_SETRECT
  - EM_SETRECTNP (EM_SETRECT without repainting) - not supported in RICHEDIT
  + EM_SETSEL
  - EM_SETSCROLLPOS 3.0
  - EM_SETTABSTOPS 3.0
  - EM_SETTARGETDEVICE
  + EM_SETTEXTEX 3.0 (unicode only, no rich text insertion handling, proper style?)
  - EM_SETTEXTMODE 2.0
  - EM_SETTYPOGRAPHYOPTIONS 3.0
  - EM_SETUNDOLIMIT 2.0
  - EM_SETWORDBREAKPROC
  - EM_SETWORDBREAKPROCEX
  - EM_SETWORDWRAPMODE 1.0asian
  + EM_SETZOOM 3.0
  - EM_SHOWSCROLLBAR 2.0
  - EM_STOPGROUPTYPING 2.0
  + EM_STREAMIN
  + EM_STREAMOUT
  + EM_UNDO
  + WM_CHAR
  + WM_CLEAR
  + WM_COPY
  + WM_CUT
  + WM_GETDLGCODE (the current implementation is incomplete)
  + WM_GETTEXT (ANSI&Unicode)
  + WM_GETTEXTLENGTH (ANSI version sucks)
  + WM_PASTE
  - WM_SETFONT
  + WM_SETTEXT (resets undo stack !) (proper style?) ANSI&Unicode
  - WM_STYLECHANGING
  - WM_STYLECHANGED (things like read-only flag)
  - WM_UNICHAR
  
  Notifications
  
  * EN_CHANGE (sent from the wrong place)
  - EN_CORRECTTEXT
  - EN_DROPFILES
  - EN_ERRSPACE
  - EN_HSCROLL
  - EN_IMECHANGE
  + EN_KILLFOCUS
  - EN_LINK
  - EN_MAXTEXT
  - EN_MSGFILTER
  - EN_OLEOPFAILED
  - EN_PROTECTED
  - EN_REQUESTRESIZE
  - EN_SAVECLIPBOARD
  + EN_SELCHANGE 
  + EN_SETFOCUS
  - EN_STOPNOUNDO
  * EN_UPDATE (sent from the wrong place)
  - EN_VSCROLL
  
  Styles
  
  - ES_AUTOHSCROLL
  - ES_AUTOVSCROLL
  - ES_CENTER
  - ES_DISABLENOSCROLL (scrollbar is always visible)
  - ES_EX_NOCALLOLEINIT
  - ES_LEFT
  - ES_MULTILINE (currently single line controls aren't supported)
  - ES_NOIME
  - ES_READONLY (I'm not sure if beeping is the proper behaviour)
  - ES_RIGHT
  - ES_SAVESEL
  - ES_SELFIME
  - ES_SUNKEN
  - ES_VERTICAL
  - ES_WANTRETURN (don't know how to do WM_GETDLGCODE part)
  - WS_SETFONT
  - WS_HSCROLL
  - WS_VSCROLL
*/

/*
 * RICHED20 TODO (incomplete):
 *
 * - messages/styles/notifications listed above 
 * - Undo coalescing 
 * - add remaining CHARFORMAT/PARAFORMAT fields
 * - right/center align should strip spaces from the beginning
 * - more advanced navigation (Ctrl-arrows)
 * - tabs
 * - pictures/OLE objects (not just smiling faces that lack API support ;-) )
 * - COM interface (looks like a major pain in the TODO list)
 * - calculate heights of pictures (half-done)
 * - horizontal scrolling (not even started)
 * - hysteresis during wrapping (related to scrollbars appearing/disappearing)
 * - find/replace
 * - how to implement EM_FORMATRANGE and EM_DISPLAYBAND ? (Mission Impossible)
 * - italic caret with italic fonts
 * - IME
 * - most notifications aren't sent at all (the most important ones are)
 * - when should EN_SELCHANGE be sent after text change ? (before/after EN_UPDATE?)
 * - WM_SETTEXT may use wrong style (but I'm 80% sure it's OK)
 * - EM_GETCHARFORMAT with SCF_SELECTION may not behave 100% like in original (but very close)
 * - full justification
 * - hyphenation
 * - tables
 *
 * Bugs that are probably fixed, but not so easy to verify:
 * - EN_UPDATE/EN_CHANGE are handled very incorrectly (should be OK now)
 * - undo for ME_JoinParagraphs doesn't store paragraph format ? (it does)
 * - check/fix artificial EOL logic (bCursorAtEnd, hardly logical)
 * - caret shouldn't be displayed when selection isn't empty
 * - check refcounting in style management functions (looks perfect now, but no bugs is suspicious)
 * - undo for setting default format (done, might be buggy)
 * - styles might be not released properly (looks like they work like charm, but who knows?
 *
 */

#include "editor.h"
#include "commdlg.h"
#include "ole2.h"
#include "richole.h"
#include "winreg.h"
#define NO_SHLWAPI_STREAM 
#include "shlwapi.h"

#include "rtf.h"
 
WINE_DEFAULT_DEBUG_CHANNEL(richedit);

int me_debug = 0;
HANDLE me_heap = NULL;

static ME_TextBuffer *ME_MakeText(void) {
  
  ME_TextBuffer *buf = ALLOC_OBJ(ME_TextBuffer);

  ME_DisplayItem *p1 = ME_MakeDI(diTextStart);
  ME_DisplayItem *p2 = ME_MakeDI(diTextEnd);
  
  p1->prev = NULL;
  p1->next = p2;
  p2->prev = p1;
  p2->next = NULL;
  p1->member.para.next_para = p2;
  p2->member.para.prev_para = p1;
  p2->member.para.nCharOfs = 0;  
  
  buf->pFirst = p1;
  buf->pLast = p2;
  buf->pCharStyle = NULL;
  
  return buf;
}


static LRESULT ME_StreamInText(ME_TextEditor *editor, DWORD dwFormat, ME_InStream *stream, ME_Style *style)
{
  WCHAR wszText[STREAMIN_BUFFER_SIZE+1];
  WCHAR *pText;
  
  TRACE("%08lx %p\n", dwFormat, stream);
  
  do {
    long nWideChars = 0;

    if (!stream->dwSize)
    {
      ME_StreamInFill(stream);
      if (stream->editstream->dwError)
        break;
      if (!stream->dwSize)
        break;
    }
      
    if (!(dwFormat & SF_UNICODE))
    {
      /* FIXME? this is doomed to fail on true MBCS like UTF-8, luckily they're unlikely to be used as CP_ACP */
      nWideChars = MultiByteToWideChar(CP_ACP, 0, stream->buffer, stream->dwSize, wszText, STREAMIN_BUFFER_SIZE);
      pText = wszText;
    }
    else
    {
      nWideChars = stream->dwSize >> 1;
      pText = (WCHAR *)stream->buffer;
    }
    
    ME_InsertTextFromCursor(editor, 0, pText, nWideChars, style);
    if (stream->dwSize < STREAMIN_BUFFER_SIZE)
      break;
    stream->dwSize = 0;
  } while(1);
  ME_CommitUndo(editor);
  ME_Repaint(editor);
  return 0;
}

static void ME_RTFCharAttrHook(RTF_Info *info)
{
  CHARFORMAT2W fmt;
  fmt.cbSize = sizeof(fmt);
  fmt.dwMask = 0;
  
  switch(info->rtfMinor)
  {
    case rtfPlain:
      /* FIXME add more flags once they're implemented */
      fmt.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_COLOR | CFM_BACKCOLOR | CFM_SIZE | CFM_WEIGHT;
      fmt.dwEffects = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;
      fmt.yHeight = 12*20; /* 12pt */
      fmt.wWeight = 400;
      break;
    case rtfBold:
      fmt.dwMask = CFM_BOLD;
      fmt.dwEffects = info->rtfParam ? fmt.dwMask : 0;
      break;
    case rtfItalic:
      fmt.dwMask = CFM_ITALIC;
      fmt.dwEffects = info->rtfParam ? fmt.dwMask : 0;
      break;
    case rtfUnderline:
      fmt.dwMask = CFM_UNDERLINE;
      fmt.dwEffects = info->rtfParam ? fmt.dwMask : 0;
      fmt.bUnderlineType = CFU_CF1UNDERLINE;
      break;
    case rtfNoUnderline:
      fmt.dwMask = CFM_UNDERLINE;
      fmt.dwEffects = 0;
      break;
    case rtfStrikeThru:
      fmt.dwMask = CFM_STRIKEOUT;
      fmt.dwEffects = info->rtfParam ? fmt.dwMask : 0;
      break;
    case rtfSubScript:
    case rtfSuperScript:
    case rtfSubScrShrink:
    case rtfSuperScrShrink:
    case rtfNoSuperSub:
      fmt.dwMask = CFM_SUBSCRIPT|CFM_SUPERSCRIPT;
      if (info->rtfMinor == rtfSubScrShrink) fmt.dwEffects = CFE_SUBSCRIPT;
      if (info->rtfMinor == rtfSuperScrShrink) fmt.dwEffects = CFE_SUPERSCRIPT;
      if (info->rtfMinor == rtfNoSuperSub) fmt.dwEffects = 0;
      break;
    case rtfBackColor:
      fmt.dwMask = CFM_BACKCOLOR;
      fmt.dwEffects = 0;
      if (info->rtfParam == 0)
        fmt.dwEffects = CFE_AUTOBACKCOLOR;
      else if (info->rtfParam != rtfNoParam)
      {
        RTFColor *c = RTFGetColor(info, info->rtfParam);
        fmt.crTextColor = (c->rtfCBlue<<16)|(c->rtfCGreen<<8)|(c->rtfCRed);
      }
      break;
    case rtfForeColor:
      fmt.dwMask = CFM_COLOR;
      fmt.dwEffects = 0;
      if (info->rtfParam == 0)
        fmt.dwEffects = CFE_AUTOCOLOR;
      else if (info->rtfParam != rtfNoParam)
      {
        RTFColor *c = RTFGetColor(info, info->rtfParam);
        fmt.crTextColor = (c->rtfCBlue<<16)|(c->rtfCGreen<<8)|(c->rtfCRed);
      }
      break;
    case rtfFontNum:
      if (info->rtfParam != rtfNoParam)
      {
        RTFFont *f = RTFGetFont(info, info->rtfParam);
        if (f)
        {
          MultiByteToWideChar(CP_ACP, 0, f->rtfFName, -1, fmt.szFaceName, sizeof(fmt.szFaceName)/sizeof(WCHAR));
          fmt.szFaceName[sizeof(fmt.szFaceName)/sizeof(WCHAR)-1] = '\0';
          fmt.bCharSet = f->rtfFCharSet;
          fmt.dwMask = CFM_FACE | CFM_CHARSET;
        }
      }
      break;
    case rtfFontSize:
      fmt.dwMask = CFM_SIZE;
      if (info->rtfParam != rtfNoParam)
        fmt.yHeight = info->rtfParam*10;
      break;
  }
  if (fmt.dwMask) {
    ME_Style *style2;
    RTFFlushOutputBuffer(info);
    /* FIXME too slow ? how come ? */
    style2 = ME_ApplyStyle(info->style, &fmt);
    ME_ReleaseStyle(info->style);
    info->style = style2;
  }
}

/* FIXME this function doesn't get any information about context of the RTF tag, which is very bad,
   the same tags mean different things in different contexts */
static void ME_RTFParAttrHook(RTF_Info *info)
{
  PARAFORMAT2 fmt;
  fmt.cbSize = sizeof(fmt);
  fmt.dwMask = 0;
  
  switch(info->rtfMinor)
  {
  case rtfParDef: /* I'm not 100% sure what does it do, but I guess it restores default paragraph attributes */
    fmt.dwMask = PFM_ALIGNMENT | PFM_TABSTOPS | PFM_OFFSET | PFM_STARTINDENT;
    fmt.wAlignment = PFA_LEFT;
    fmt.cTabCount = 0;
    fmt.dxOffset = fmt.dxStartIndent = 0;
    break;
  case rtfFirstIndent:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    fmt.dwMask = PFM_STARTINDENT;
    fmt.dxStartIndent = info->rtfParam + fmt.dxOffset;
    break;
  case rtfLeftIndent:
  {
    int first, left;
    ME_GetSelectionParaFormat(info->editor, &fmt);
    first = fmt.dxStartIndent;
    left = info->rtfParam;
    fmt.dwMask = PFM_STARTINDENT|PFM_OFFSET;
    fmt.dxStartIndent = first + left;
    fmt.dxOffset = -first;
    break;
  }
  case rtfRightIndent:
    fmt.dwMask = PFM_RIGHTINDENT;
    fmt.dxRightIndent = info->rtfParam;
    break;
  case rtfQuadLeft:
  case rtfQuadJust:
    fmt.dwMask = PFM_ALIGNMENT;
    fmt.wAlignment = PFA_LEFT;
    break;
  case rtfQuadRight:
    fmt.dwMask = PFM_ALIGNMENT;
    fmt.wAlignment = PFA_RIGHT;
    break;
  case rtfQuadCenter:
    fmt.dwMask = PFM_ALIGNMENT;
    fmt.wAlignment = PFA_CENTER;
    break;
  case rtfTabPos:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    if (!(fmt.dwMask & PFM_TABSTOPS))
    {
      fmt.dwMask |= PFM_TABSTOPS;
      fmt.cTabCount = 0;
    }
    if (fmt.cTabCount < MAX_TAB_STOPS)
      fmt.rgxTabs[fmt.cTabCount++] = info->rtfParam;
    break;
  }  
  if (fmt.dwMask) {
    RTFFlushOutputBuffer(info);
    /* FIXME too slow ? how come ?*/
    ME_SetSelectionParaFormat(info->editor, &fmt);
  }
}

static void ME_RTFReadHook(RTF_Info *info) {
  switch(info->rtfClass)
  {
    case rtfGroup:
      switch(info->rtfMajor)
      {
        case rtfBeginGroup:
          if (info->stackTop < maxStack) {
            memcpy(&info->stack[info->stackTop].fmt, &info->style->fmt, sizeof(CHARFORMAT2W));
            info->stack[info->stackTop].codePage = info->codePage;
            info->stack[info->stackTop].unicodeLength = info->unicodeLength;
          }
          info->stackTop++;
          break;
        case rtfEndGroup:
        {
          ME_Style *s;
          RTFFlushOutputBuffer(info);
          info->stackTop--;
          /* FIXME too slow ? how come ? */
          s = ME_ApplyStyle(info->style, &info->stack[info->stackTop].fmt);
          ME_ReleaseStyle(info->style);
          info->style = s;
          info->codePage = info->stack[info->stackTop].codePage;
          info->unicodeLength = info->stack[info->stackTop].unicodeLength;
          break;
        }
      }
      break;
    case rtfControl:
      switch(info->rtfMajor)
      {
        case rtfCharAttr:
          ME_RTFCharAttrHook(info);
          break;
        case rtfParAttr:
          ME_RTFParAttrHook(info);
          break;
      }
      break;
  }
}

void
ME_StreamInFill(ME_InStream *stream)
{
  stream->editstream->dwError = stream->editstream->pfnCallback(stream->editstream->dwCookie,
                                                                stream->buffer,
                                                                sizeof(stream->buffer),
                                                                &stream->dwSize);
  stream->dwUsed = 0;
}

static LRESULT ME_StreamIn(ME_TextEditor *editor, DWORD format, EDITSTREAM *stream)
{
  RTF_Info parser;
  ME_Style *style;
  int from, to, to2, nUndoMode;
  ME_UndoItem *pUI;
  int nEventMask = editor->nEventMask;
  ME_InStream inStream;

  TRACE("stream==%p hWnd==%p format==0x%X\n", stream, editor->hWnd, (UINT)format);
  editor->nEventMask = 0;
  
  ME_GetSelection(editor, &from, &to);
  if (format & SFF_SELECTION) {
    style = ME_GetSelectionInsertStyle(editor);

    ME_InternalDeleteText(editor, from, to-from);
  }
  else {
    style = editor->pBuffer->pDefaultStyle;
    ME_AddRefStyle(style);
    SendMessageA(editor->hWnd, EM_SETSEL, 0, 0);    
    ME_InternalDeleteText(editor, 0, ME_GetTextLength(editor));
    from = to = 0;
    ME_ClearTempStyle(editor);
    /* FIXME restore default paragraph formatting ! */
  }
  
  nUndoMode = editor->nUndoMode;
  editor->nUndoMode = umIgnore;

  inStream.editstream = stream;
  inStream.editstream->dwError = 0;
  inStream.dwSize = 0;
  inStream.dwUsed = 0;

  if (format & SF_RTF)
  {
    /* Check if it's really RTF, and if it is not, use plain text */
    ME_StreamInFill(&inStream);
    if (!inStream.editstream->dwError)
    {
      if (strncmp(inStream.buffer, "{\\rtf1", 6) && strncmp(inStream.buffer, "{\\urtf", 6))
      {
        format &= ~SF_RTF;
        format |= SF_TEXT;
      }
    }
  }

  if (!inStream.editstream->dwError)
  {
    if (format & SF_RTF) {
      /* setup the RTF parser */
      memset(&parser, 0, sizeof parser);
      RTFSetEditStream(&parser, &inStream);
      parser.rtfFormat = format&(SF_TEXT|SF_RTF);
      parser.hwndEdit = editor->hWnd;
      parser.editor = editor;
      parser.style = style;
      WriterInit(&parser);
      RTFInit(&parser);
      RTFSetReadHook(&parser, ME_RTFReadHook);
      BeginFile(&parser);
  
      /* do the parsing */
      RTFRead(&parser);
      RTFFlushOutputBuffer(&parser);
      RTFDestroy(&parser);

      style = parser.style;
    }
    else if (format & SF_TEXT)
      ME_StreamInText(editor, format, &inStream, style);
    else
      ERR("EM_STREAMIN without SF_TEXT or SF_RTF\n");
    ME_GetSelection(editor, &to, &to2);
    /* put the cursor at the top */
    if (!(format & SFF_SELECTION))
      SendMessageA(editor->hWnd, EM_SETSEL, 0, 0);
    else
    {
      /* FIXME where to put cursor now ? */
    }
  }
  
  editor->nUndoMode = nUndoMode;
  pUI = ME_AddUndoItem(editor, diUndoDeleteRun, NULL);
  TRACE("from %d to %d\n", from, to);
  if (pUI && from < to)
  {
    pUI->nStart = from;
    pUI->nLen = to-from;
  }
  ME_CommitUndo(editor);
  ME_ReleaseStyle(style); 
  editor->nEventMask = nEventMask;
  InvalidateRect(editor->hWnd, NULL, TRUE);
  ME_UpdateRepaint(editor);
  if (!(format & SFF_SELECTION)) {
    ME_ClearTempStyle(editor);
  }
  ME_MoveCaret(editor);
  ME_SendSelChange(editor);

  return 0;
}


ME_DisplayItem *
ME_FindItemAtOffset(ME_TextEditor *editor, ME_DIType nItemType, int nOffset, int *nItemOffset)
{
  ME_DisplayItem *item = ME_FindItemFwd(editor->pBuffer->pFirst, diParagraph);
  
  while (item && item->member.para.next_para->member.para.nCharOfs <= nOffset)
    item = ME_FindItemFwd(item, diParagraph);

  if (!item)
    return item;

  nOffset -= item->member.para.nCharOfs;
  if (nItemType == diParagraph) {
    if (nItemOffset)
      *nItemOffset = nOffset;
    return item;
  }
  
  do {
    item = ME_FindItemFwd(item, diRun);
  } while (item && (item->member.run.nCharOfs + ME_StrLen(item->member.run.strText) <= nOffset));
  if (item) {
    nOffset -= item->member.run.nCharOfs;
    if (nItemOffset)
      *nItemOffset = nOffset;
  }
  return item;
}


static int
ME_FindText(ME_TextEditor *editor, DWORD flags, CHARRANGE *chrg, WCHAR *text, CHARRANGE *chrgText)
{
  int nStart = chrg->cpMin;
  int nLen = lstrlenW(text);
  ME_DisplayItem *item = ME_FindItemAtOffset(editor, diRun, nStart, &nStart);
  ME_DisplayItem *para;
  
  if (!item)
    return -1;

  if (!nLen)
  {
    if (chrgText)
      chrgText->cpMin = chrgText->cpMax = chrg->cpMin;
    return chrg->cpMin;
  }
 
  if (!(flags & FR_DOWN))
    FIXME("Backward search not implemented\n");
  if (!(flags & FR_MATCHCASE))
    FIXME("Case-insensitive search not implemented\n");
  if (flags & ~(FR_DOWN | FR_MATCHCASE))
    FIXME("Flags 0x%08lx not implemented\n", flags & ~(FR_DOWN | FR_MATCHCASE));
  
  para = ME_GetParagraph(item);
  while (item && para->member.para.nCharOfs + item->member.run.nCharOfs + nStart + nLen < chrg->cpMax)
  {
    ME_DisplayItem *pCurItem = item;
    int nCurStart = nStart;
    int nMatched = 0;
    
    while (pCurItem->member.run.strText->szData[nCurStart + nMatched] == text[nMatched])
    {
      nMatched++;
      if (nMatched == nLen)
      {
        nStart += para->member.para.nCharOfs + item->member.run.nCharOfs;
        if (chrgText)
        {
          chrgText->cpMin = nStart;
          chrgText->cpMax = nStart + nLen;
        }
        return nStart;
      }
      if (nCurStart + nMatched == ME_StrLen(pCurItem->member.run.strText))
      {
        pCurItem = ME_FindItemFwd(pCurItem, diRun);
        nCurStart = -nMatched;
      }
    }
    nStart++;
    if (nStart == ME_StrLen(item->member.run.strText))
    {
      item = ME_FindItemFwd(item, diRun);
      para = ME_GetParagraph(item);
      nStart = 0;
    }
  }
  return -1;
}


ME_TextEditor *ME_MakeEditor(HWND hWnd) {
  ME_TextEditor *ed = ALLOC_OBJ(ME_TextEditor);
  HDC hDC;
  int i;
  ed->hWnd = hWnd;
  ed->bEmulateVersion10 = FALSE;
  ed->pBuffer = ME_MakeText();
  hDC = GetDC(hWnd);
  ME_MakeFirstParagraph(hDC, ed->pBuffer);
  ReleaseDC(hWnd, hDC);
  ed->bCaretShown = FALSE;
  ed->nCursors = 3;
  ed->pCursors = ALLOC_N_OBJ(ME_Cursor, ed->nCursors);
  ed->pCursors[0].pRun = ME_FindItemFwd(ed->pBuffer->pFirst, diRun);
  ed->pCursors[0].nOffset = 0;
  ed->pCursors[1].pRun = ME_FindItemFwd(ed->pBuffer->pFirst, diRun);
  ed->pCursors[1].nOffset = 0;
  ed->nLastTotalLength = ed->nTotalLength = 0;
  ed->nUDArrowX = -1;
  ed->nSequence = 0;
  ed->rgbBackColor = -1;
  ed->bCaretAtEnd = FALSE;
  ed->nEventMask = 0;
  ed->nModifyStep = 0;
  ed->pUndoStack = ed->pRedoStack = NULL;
  ed->nUndoMode = umAddToUndo;
  ed->nParagraphs = 1;
  ed->nLastSelStart = ed->nLastSelEnd = 0;
  ed->nScrollPosY = 0;
  ed->nZoomNumerator = ed->nZoomDenominator = 0;
  for (i=0; i<HFONT_CACHE_SIZE; i++)
  {
    ed->pFontCache[i].nRefs = 0;
    ed->pFontCache[i].nAge = 0;
    ed->pFontCache[i].hFont = NULL;
  }
  ME_CheckCharOffsets(ed);
  return ed;
}

typedef struct tagME_GlobalDestStruct
{
  HGLOBAL hData;
  int nLength;
} ME_GlobalDestStruct;

static DWORD CALLBACK ME_AppendToHGLOBAL(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, LONG *pcb)
{
  ME_GlobalDestStruct *pData = (ME_GlobalDestStruct *)dwCookie;
  int nMaxSize;
  BYTE *pDest;
  
  nMaxSize = GlobalSize(pData->hData);
  if (pData->nLength+cb+1 >= cb)
  {
    /* round up to 2^17 */
    int nNewSize = (((nMaxSize+cb+1)|0x1FFFF)+1) & 0xFFFE0000;
    pData->hData = GlobalReAlloc(pData->hData, nNewSize, 0);
  }
  pDest = (BYTE *)GlobalLock(pData->hData);
  memcpy(pDest + pData->nLength, lpBuff, cb);
  pData->nLength += cb;
  pDest[pData->nLength] = '\0';
  GlobalUnlock(pData->hData);
  *pcb = cb;
  
  return 0;
}

static DWORD CALLBACK ME_ReadFromHGLOBALUnicode(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, LONG *pcb)
{
  ME_GlobalDestStruct *pData = (ME_GlobalDestStruct *)dwCookie;
  int i;
  WORD *pSrc, *pDest;
  
  cb = cb >> 1;
  pDest = (WORD *)lpBuff;
  pSrc = (WORD *)GlobalLock(pData->hData);
  for (i = 0; i<cb && pSrc[pData->nLength+i]; i++) {
    pDest[i] = pSrc[pData->nLength+i];
  }    
  pData->nLength += i;
  *pcb = 2*i;
  GlobalUnlock(pData->hData);
  return 0;
}

static DWORD CALLBACK ME_ReadFromHGLOBALRTF(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, LONG *pcb)
{
  ME_GlobalDestStruct *pData = (ME_GlobalDestStruct *)dwCookie;
  int i;
  BYTE *pSrc, *pDest;
  
  pDest = lpBuff;
  pSrc = (BYTE *)GlobalLock(pData->hData);
  for (i = 0; i<cb && pSrc[pData->nLength+i]; i++) {
    pDest[i] = pSrc[pData->nLength+i];
  }    
  pData->nLength += i;
  *pcb = i;
  GlobalUnlock(pData->hData);
  return 0;
}


void ME_DestroyEditor(ME_TextEditor *editor)
{
  ME_DisplayItem *pFirst = editor->pBuffer->pFirst;
  ME_DisplayItem *p = pFirst, *pNext = NULL;
  int i;
  
  ME_ClearTempStyle(editor);
  ME_EmptyUndoStack(editor);
  while(p) {
    pNext = p->next;
    ME_DestroyDisplayItem(p);    
    p = pNext;
  }
  ME_ReleaseStyle(editor->pBuffer->pDefaultStyle);
  for (i=0; i<HFONT_CACHE_SIZE; i++)
  {
    if (editor->pFontCache[i].hFont)
      DeleteObject(editor->pFontCache[i].hFont);
  }
    
  FREE_OBJ(editor);
}

static WCHAR wszClassName[] = {'R', 'i', 'c', 'h', 'E', 'd', 'i', 't', '2', '0', 'W', 0};
static WCHAR wszClassName50[] = {'R', 'i', 'c', 'h', 'E', 'd', 'i', 't', '5', '0', 'W', 0};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("\n");
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hinstDLL);
      me_heap = HeapCreate (0, 0x10000, 0);
      ME_RegisterEditorClass(hinstDLL);
      break;

    case DLL_PROCESS_DETACH:
      UnregisterClassW(wszClassName, 0);
      UnregisterClassW(wszClassName50, 0);
      UnregisterClassA("RichEdit20A", 0);
      UnregisterClassA("RichEdit50A", 0);
      HeapDestroy (me_heap);
      me_heap = NULL;
      break;
    }
    return TRUE;
}


#define UNSUPPORTED_MSG(e) \
  case e: \
    FIXME(#e ": stub\n"); \
    return DefWindowProcW(hWnd, msg, wParam, lParam);

static const char * const edit_messages[] = {
  "EM_GETSEL",
  "EM_SETSEL",
  "EM_GETRECT",
  "EM_SETRECT",
  "EM_SETRECTNP",
  "EM_SCROLL",
  "EM_LINESCROLL",
  "EM_SCROLLCARET",
  "EM_GETMODIFY",
  "EM_SETMODIFY",
  "EM_GETLINECOUNT",
  "EM_LINEINDEX",
  "EM_SETHANDLE",
  "EM_GETHANDLE",
  "EM_GETTHUMB",
  "EM_UNKNOWN_BF",
  "EM_UNKNOWN_C0",
  "EM_LINELENGTH",
  "EM_REPLACESEL",
  "EM_UNKNOWN_C3",
  "EM_GETLINE",
  "EM_LIMITTEXT",
  "EM_CANUNDO",
  "EM_UNDO",
  "EM_FMTLINES",
  "EM_LINEFROMCHAR",
  "EM_UNKNOWN_CA",
  "EM_SETTABSTOPS",
  "EM_SETPASSWORDCHAR",
  "EM_EMPTYUNDOBUFFER",
  "EM_GETFIRSTVISIBLELINE",
  "EM_SETREADONLY",
  "EM_SETWORDBREAKPROC",
  "EM_GETWORDBREAKPROC",
  "EM_GETPASSWORDCHAR",
  "EM_SETMARGINS",
  "EM_GETMARGINS",
  "EM_GETLIMITTEXT",
  "EM_POSFROMCHAR",
  "EM_CHARFROMPOS"
};

static const char * const richedit_messages[] = {
  "EM_CANPASTE",
  "EM_DISPLAYBAND",
  "EM_EXGETSEL",
  "EM_EXLIMITTEXT",
  "EM_EXLINEFROMCHAR",
  "EM_EXSETSEL",
  "EM_FINDTEXT",
  "EM_FORMATRANGE",
  "EM_GETCHARFORMAT",
  "EM_GETEVENTMASK",
  "EM_GETOLEINTERFACE",
  "EM_GETPARAFORMAT",
  "EM_GETSELTEXT",
  "EM_HIDESELECTION",
  "EM_PASTESPECIAL",
  "EM_REQUESTRESIZE",
  "EM_SELECTIONTYPE",
  "EM_SETBKGNDCOLOR",
  "EM_SETCHARFORMAT",
  "EM_SETEVENTMASK",
  "EM_SETOLECALLBACK",
  "EM_SETPARAFORMAT",
  "EM_SETTARGETDEVICE",
  "EM_STREAMIN",
  "EM_STREAMOUT",
  "EM_GETTEXTRANGE",
  "EM_FINDWORDBREAK",
  "EM_SETOPTIONS",
  "EM_GETOPTIONS",
  "EM_FINDTEXTEX",
  "EM_GETWORDBREAKPROCEX",
  "EM_SETWORDBREAKPROCEX",
  "EM_SETUNDOLIMIT",
  "EM_UNKNOWN_USER_83",
  "EM_REDO",
  "EM_CANREDO",
  "EM_GETUNDONAME",
  "EM_GETREDONAME",
  "EM_STOPGROUPTYPING",
  "EM_SETTEXTMODE",
  "EM_GETTEXTMODE",
  "EM_AUTOURLDETECT",
  "EM_GETAUTOURLDETECT",
  "EM_SETPALETTE",
  "EM_GETTEXTEX",
  "EM_GETTEXTLENGTHEX",
  "EM_SHOWSCROLLBAR",
  "EM_SETTEXTEX",
  "EM_UNKNOWN_USER_98",
  "EM_UNKNOWN_USER_99",
  "EM_SETPUNCTUATION",
  "EM_GETPUNCTUATION",
  "EM_SETWORDWRAPMODE",
  "EM_GETWORDWRAPMODE",
  "EM_SETIMECOLOR",
  "EM_GETIMECOLOR",
  "EM_SETIMEOPTIONS",
  "EM_GETIMEOPTIONS",
  "EM_CONVPOSITION",
  "EM_UNKNOWN_USER_109",
  "EM_UNKNOWN_USER_110",
  "EM_UNKNOWN_USER_111",
  "EM_UNKNOWN_USER_112",
  "EM_UNKNOWN_USER_113",
  "EM_UNKNOWN_USER_114",
  "EM_UNKNOWN_USER_115",
  "EM_UNKNOWN_USER_116",
  "EM_UNKNOWN_USER_117",
  "EM_UNKNOWN_USER_118",
  "EM_UNKNOWN_USER_119",
  "EM_SETLANGOPTIONS",
  "EM_GETLANGOPTIONS",
  "EM_GETIMECOMPMODE",
  "EM_FINDTEXTW",
  "EM_FINDTEXTEXW",
  "EM_RECONVERSION",
  "EM_SETIMEMODEBIAS",
  "EM_GETIMEMODEBIAS"
};

static const char *
get_msg_name(UINT msg)
{
  if (msg >= EM_GETSEL && msg <= EM_SETLIMITTEXT)
    return edit_messages[msg - EM_GETSEL];
  if (msg >= EM_CANPASTE && msg <= EM_GETIMEMODEBIAS)
    return richedit_messages[msg - EM_CANPASTE];
  return "";
}

/******************************************************************
 *        RichEditANSIWndProc (RICHED20.10)
 */
LRESULT WINAPI RichEditANSIWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  HDC hDC;
  PAINTSTRUCT ps;
  SCROLLINFO si;
  ME_TextEditor *editor = (ME_TextEditor *)GetWindowLongW(hWnd, 0);
  
  TRACE("msg %d (%s) %08x %08lx\n", msg, get_msg_name(msg), wParam, lParam);
  
  switch(msg) {
  
  UNSUPPORTED_MSG(EM_AUTOURLDETECT)
  UNSUPPORTED_MSG(EM_CHARFROMPOS)
  UNSUPPORTED_MSG(EM_DISPLAYBAND)
  UNSUPPORTED_MSG(EM_EXLIMITTEXT)
  UNSUPPORTED_MSG(EM_FINDWORDBREAK)
  UNSUPPORTED_MSG(EM_FMTLINES)
  UNSUPPORTED_MSG(EM_FORMATRANGE)
  UNSUPPORTED_MSG(EM_GETAUTOURLDETECT)
  UNSUPPORTED_MSG(EM_GETBIDIOPTIONS)
  UNSUPPORTED_MSG(EM_GETEDITSTYLE)
  UNSUPPORTED_MSG(EM_GETFIRSTVISIBLELINE)
  UNSUPPORTED_MSG(EM_GETIMECOMPMODE)
  /* UNSUPPORTED_MSG(EM_GETIMESTATUS) missing in Wine headers */
  UNSUPPORTED_MSG(EM_GETLANGOPTIONS)
  UNSUPPORTED_MSG(EM_GETLIMITTEXT)
  UNSUPPORTED_MSG(EM_GETLINE)
  /* UNSUPPORTED_MSG(EM_GETOLEINTERFACE) separate stub */
  UNSUPPORTED_MSG(EM_GETOPTIONS)
  UNSUPPORTED_MSG(EM_GETPASSWORDCHAR)
  UNSUPPORTED_MSG(EM_GETRECT)
  UNSUPPORTED_MSG(EM_GETREDONAME)
  UNSUPPORTED_MSG(EM_GETSCROLLPOS)
  UNSUPPORTED_MSG(EM_GETTEXTMODE)
  UNSUPPORTED_MSG(EM_GETTYPOGRAPHYOPTIONS)
  UNSUPPORTED_MSG(EM_GETUNDONAME)
  UNSUPPORTED_MSG(EM_GETWORDBREAKPROC)
  UNSUPPORTED_MSG(EM_GETWORDBREAKPROCEX)
  UNSUPPORTED_MSG(EM_HIDESELECTION)
  UNSUPPORTED_MSG(EM_LIMITTEXT) /* also known as EM_SETLIMITTEXT */
  UNSUPPORTED_MSG(EM_PASTESPECIAL)
/*  UNSUPPORTED_MSG(EM_POSFROMCHARS) missing in Wine headers */
  UNSUPPORTED_MSG(EM_REQUESTRESIZE)
  UNSUPPORTED_MSG(EM_SCROLL)
  UNSUPPORTED_MSG(EM_SCROLLCARET)
  UNSUPPORTED_MSG(EM_SELECTIONTYPE)
  UNSUPPORTED_MSG(EM_SETBIDIOPTIONS)
  UNSUPPORTED_MSG(EM_SETEDITSTYLE)
  UNSUPPORTED_MSG(EM_SETFONTSIZE)
  UNSUPPORTED_MSG(EM_SETLANGOPTIONS)
  UNSUPPORTED_MSG(EM_SETOLECALLBACK)
  UNSUPPORTED_MSG(EM_SETOPTIONS)
  UNSUPPORTED_MSG(EM_SETPALETTE)
  UNSUPPORTED_MSG(EM_SETPASSWORDCHAR)
  UNSUPPORTED_MSG(EM_SETRECT)
  UNSUPPORTED_MSG(EM_SETRECTNP)
  UNSUPPORTED_MSG(EM_SETSCROLLPOS)
  UNSUPPORTED_MSG(EM_SETTABSTOPS)
  UNSUPPORTED_MSG(EM_SETTARGETDEVICE)
  UNSUPPORTED_MSG(EM_SETTEXTMODE)
  UNSUPPORTED_MSG(EM_SETTYPOGRAPHYOPTIONS)
  UNSUPPORTED_MSG(EM_SETUNDOLIMIT)
  UNSUPPORTED_MSG(EM_SETWORDBREAKPROC)
  UNSUPPORTED_MSG(EM_SETWORDBREAKPROCEX)
  UNSUPPORTED_MSG(EM_SHOWSCROLLBAR)
  UNSUPPORTED_MSG(WM_SETFONT)
  UNSUPPORTED_MSG(WM_STYLECHANGING)
  UNSUPPORTED_MSG(WM_STYLECHANGED)
/*  UNSUPPORTED_MSG(WM_UNICHAR) FIXME missing in Wine headers */
    
/* Messages specific to Richedit controls */
  
  case EM_STREAMIN:
   return ME_StreamIn(editor, wParam, (EDITSTREAM*)lParam);
  case EM_STREAMOUT:
   return ME_StreamOut(editor, wParam, (EDITSTREAM *)lParam);
  case WM_GETDLGCODE:
  {
    UINT code = DLGC_WANTCHARS|DLGC_WANTARROWS;
    if (GetWindowLongW(hWnd, GWL_STYLE)&ES_WANTRETURN)
      code |= 0; /* FIXME what can we do here ? ask for messages and censor them ? */
    return code;
  }
  case WM_NCCREATE:
  {
    CREATESTRUCTW *pcs = (CREATESTRUCTW *)lParam;
    TRACE("WM_NCCREATE: style 0x%08lx\n", pcs->style);
    editor = ME_MakeEditor(hWnd);
    SetWindowLongW(hWnd, 0, (long)editor);
    pcs = 0; /* ignore */
    return TRUE;
  }
  case EM_EMPTYUNDOBUFFER:
    ME_EmptyUndoStack(editor);
    return 0;
  case EM_GETSEL:
  {
    /* Note: wParam/lParam can be NULL */
    UINT from, to;
    PUINT pfrom = wParam ? (PUINT)wParam : &from;
    PUINT pto = lParam ? (PUINT)lParam : &to;
    ME_GetSelection(editor, pfrom, pto);
    if ((*pfrom|*pto) & 0xFFFF0000)
      return -1;
    return MAKELONG(*pfrom,*pto);
  }
  case EM_EXGETSEL:
  {
    CHARRANGE *pRange = (CHARRANGE *)lParam;
    ME_GetSelection(editor, (int *)&pRange->cpMin, (int *)&pRange->cpMax);
    TRACE("EM_EXGETSEL = (%ld,%ld)\n", pRange->cpMin, pRange->cpMax);
    return 0;
  }
  case EM_CANUNDO:
    return editor->pUndoStack != NULL;
  case EM_CANREDO:
    return editor->pRedoStack != NULL;
  case EM_UNDO:
    ME_Undo(editor);
    return 0;
  case EM_REDO:
    ME_Redo(editor);
    return 0;
  case EM_SETSEL:
  {
    ME_SetSelection(editor, wParam, lParam);
    ME_Repaint(editor);
    ME_SendSelChange(editor);
    return 0;
  }
  case EM_EXSETSEL:
  {
    CHARRANGE *pRange = (CHARRANGE *)lParam;
    TRACE("EM_EXSETSEL (%ld,%ld)\n", pRange->cpMin, pRange->cpMax);
    ME_SetSelection(editor, pRange->cpMin, pRange->cpMax);
    /* FIXME optimize */
    ME_Repaint(editor);
    ME_SendSelChange(editor);
    return 0;
  }
  case EM_SETTEXTEX:
  {
    LPWSTR wszText = (LPWSTR)lParam;
    SETTEXTEX *pStruct = (SETTEXTEX *)wParam;
    size_t len = lstrlenW(wszText);
    int from, to;
    ME_Style *style;
    TRACE("EM_SETTEXEX - %s, flags %d, cp %d\n", debugstr_w(wszText), (int)pStruct->flags, pStruct->codepage);
    if (pStruct->codepage != 1200) {
      FIXME("EM_SETTEXTEX only supports unicode right now!\n"); 
      return 0;
    }
    /* FIXME: this should support RTF strings too, according to MSDN */
    if (pStruct->flags & ST_SELECTION) {
      ME_GetSelection(editor, &from, &to);
      style = ME_GetSelectionInsertStyle(editor);
      ME_InternalDeleteText(editor, from, to - from);
      ME_InsertTextFromCursor(editor, 0, wszText, len, style);
      ME_ReleaseStyle(style);
    }
    else {
      ME_InternalDeleteText(editor, 0, ME_GetTextLength(editor));
      ME_InsertTextFromCursor(editor, 0, wszText, -1, editor->pBuffer->pDefaultStyle);
      len = 1;
    }
    ME_CommitUndo(editor);
    if (!(pStruct->flags & ST_KEEPUNDO))
      ME_EmptyUndoStack(editor);
    ME_UpdateRepaint(editor);
    return len;
  }
  case EM_SETBKGNDCOLOR:
  {
    LRESULT lColor = ME_GetBackColor(editor);
    if (wParam)
      editor->rgbBackColor = -1;
    else
      editor->rgbBackColor = lParam; 
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);
    return lColor;
  }
  case EM_GETMODIFY:
    return editor->nModifyStep == 0 ? 0 : 1;
  case EM_SETMODIFY:
  {
    if (wParam)
      editor->nModifyStep = 0x80000000;
    else
      editor->nModifyStep = 0;
    
    return 0;
  }
  case EM_SETREADONLY:
  {
    long nStyle = GetWindowLongW(hWnd, GWL_STYLE);
    if (wParam)
      nStyle |= ES_READONLY;
    else
      nStyle &= ~ES_READONLY;
    SetWindowLongW(hWnd, GWL_STYLE, nStyle);
    ME_Repaint(editor);
    return 0;
  }
  case EM_SETEVENTMASK:
    editor->nEventMask = lParam;
    return 0;
  case EM_GETEVENTMASK:
    return editor->nEventMask;
  case EM_SETCHARFORMAT:
  {
    CHARFORMAT2W buf, *p;
    BOOL bRepaint = TRUE;
    p = ME_ToCF2W(&buf, (CHARFORMAT2W *)lParam);
    if (!wParam)
      ME_SetDefaultCharFormat(editor, p);
    else if (wParam == (SCF_WORD | SCF_SELECTION))
      FIXME("EM_SETCHARFORMAT: word selection not supported\n");
    else if (wParam == SCF_ALL)
      ME_SetCharFormat(editor, 0, ME_GetTextLength(editor), p);
    else {
      int from, to;
      ME_GetSelection(editor, &from, &to);
      bRepaint = (from != to);
      ME_SetSelectionCharFormat(editor, p);
    }
    ME_CommitUndo(editor);
    if (bRepaint)
      ME_UpdateRepaint(editor);
    return 0;
  }
  case EM_GETCHARFORMAT:
  {
    CHARFORMAT2W tmp, *dst = (CHARFORMAT2W *)lParam;
    if (dst->cbSize != sizeof(CHARFORMATA) &&
        dst->cbSize != sizeof(CHARFORMATW) &&
        dst->cbSize != sizeof(CHARFORMAT2A) &&
        dst->cbSize != sizeof(CHARFORMAT2W))
      return 0;
    tmp.cbSize = sizeof(tmp);
    if (!wParam)
      ME_GetDefaultCharFormat(editor, &tmp);
    else
      ME_GetSelectionCharFormat(editor, &tmp);
    ME_CopyToCFAny(dst, &tmp);
    return tmp.dwMask;
  }
  case EM_SETPARAFORMAT:
    ME_SetSelectionParaFormat(editor, (PARAFORMAT2 *)lParam);
    ME_UpdateRepaint(editor);
    ME_CommitUndo(editor);
    return 0;
  case EM_GETPARAFORMAT:
    ME_GetSelectionParaFormat(editor, (PARAFORMAT2 *)lParam);
    return 0;
  case EM_LINESCROLL:
  {
    int nPos = editor->nScrollPosY, nEnd= editor->nTotalLength - editor->sizeWindow.cy;
    nPos += 8 * lParam; /* FIXME follow the original */
    if (nPos>=nEnd)
      nPos = nEnd;
    if (nPos<0)
      nPos = 0;
    if (nPos != editor->nScrollPosY) {
      ScrollWindow(hWnd, 0, editor->nScrollPosY-nPos, NULL, NULL);
      editor->nScrollPosY = nPos;
      SetScrollPos(hWnd, SB_VERT, nPos, TRUE);
      UpdateWindow(hWnd);
    }
    return TRUE; /* Should return false if a single line richedit control */
  }
  case WM_CLEAR:
  {
    int from, to;
    ME_GetSelection(editor, &from, &to);
    ME_InternalDeleteText(editor, from, to-from);
    ME_CommitUndo(editor);
    ME_UpdateRepaint(editor);
    return 0;
  }
  case EM_REPLACESEL:
  {
    int from, to;
    ME_Style *style;
    LPWSTR wszText = ME_ToUnicode(hWnd, (void *)lParam);
    size_t len = lstrlenW(wszText);
    TRACE("EM_REPLACESEL - %s\n", debugstr_w(wszText));
    
    ME_GetSelection(editor, &from, &to);
    style = ME_GetSelectionInsertStyle(editor);
    ME_InternalDeleteText(editor, from, to-from);
    ME_InsertTextFromCursor(editor, 0, wszText, len, style);
    ME_ReleaseStyle(style);
    ME_EndToUnicode(hWnd, wszText);
    /* drop temporary style if line end */
    /* FIXME question: does abc\n mean: put abc, clear temp style, put \n? (would require a change) */  
    if (len>0 && wszText[len-1] == '\n')
      ME_ClearTempStyle(editor);
      
    ME_CommitUndo(editor);
    if (!wParam)
      ME_EmptyUndoStack(editor);
    ME_UpdateRepaint(editor);
    return 0;
  }
  case WM_SETTEXT:
  {
    ME_InternalDeleteText(editor, 0, ME_GetTextLength(editor));
    if (lParam)
    {
      LPWSTR wszText = ME_ToUnicode(hWnd, (void *)lParam);
      TRACE("WM_SETTEXT lParam==%lx\n",lParam);
      TRACE("WM_SETTEXT - %s\n", debugstr_w(wszText)); /* debugstr_w() */
      if (lstrlenW(wszText) > 0)
      {
        /* uses default style! */
        ME_InsertTextFromCursor(editor, 0, wszText, -1, editor->pBuffer->pDefaultStyle);
      }
      ME_EndToUnicode(hWnd, wszText);
    }
    else
      TRACE("WM_SETTEXT - NULL\n");
    ME_CommitUndo(editor);
    ME_EmptyUndoStack(editor);
    ME_UpdateRepaint(editor);
    return 0;
  }
  case EM_CANPASTE:
  {
    UINT nRTFFormat = RegisterClipboardFormatA("Rich Text Format");
    if (IsClipboardFormatAvailable(nRTFFormat))
      return TRUE;
    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
      return TRUE;
    return FALSE;
  }
  case WM_PASTE:
  {    
    DWORD dwFormat = 0;
    EDITSTREAM es;
    ME_GlobalDestStruct gds;
    UINT nRTFFormat = RegisterClipboardFormatA("Rich Text Format");
    UINT cf = 0;

    if (IsClipboardFormatAvailable(nRTFFormat))
      cf = nRTFFormat, dwFormat = SF_RTF;
    else if (IsClipboardFormatAvailable(CF_UNICODETEXT))
      cf = CF_UNICODETEXT, dwFormat = SF_TEXT|SF_UNICODE;
    else
      return 0;

    if (!OpenClipboard(hWnd))
      return 0;
    gds.hData = GetClipboardData(cf);
    gds.nLength = 0;
    es.dwCookie = (DWORD)&gds;
    es.pfnCallback = dwFormat == SF_RTF ? ME_ReadFromHGLOBALRTF : ME_ReadFromHGLOBALUnicode;
    SendMessageW(hWnd, EM_STREAMIN, dwFormat|SFF_SELECTION, (LPARAM)&es);
    
    CloseClipboard();
    return 0;
  }
  case WM_CUT:
  case WM_COPY:
  {
    int from, to, pars;
    WCHAR *data;
    HANDLE hData;
    EDITSTREAM es;
    ME_GlobalDestStruct gds;
    
    if (!OpenClipboard(hWnd))
      return 0;
      
    EmptyClipboard();
    ME_GetSelection(editor, &from, &to);
    pars = ME_CountParagraphsBetween(editor, from, to);
    hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR)*(to-from+pars+1));
    data = (WCHAR *)GlobalLock(hData);
    ME_GetTextW(editor, data, from, to-from, TRUE);
    GlobalUnlock(hData);

    gds.hData = GlobalAlloc(GMEM_MOVEABLE, 0);
    gds.nLength = 0;
    es.dwCookie = (DWORD)&gds;
    es.pfnCallback = ME_AppendToHGLOBAL;
    SendMessageW(hWnd, EM_STREAMOUT, SFF_SELECTION|SF_RTF, (LPARAM)&es);
    GlobalReAlloc(gds.hData, gds.nLength+1, 0);
    
    SetClipboardData(CF_UNICODETEXT, hData);    
    SetClipboardData(RegisterClipboardFormatA("Rich Text Format"), gds.hData);
    
    CloseClipboard();
    if (msg == WM_CUT)
    {
      ME_InternalDeleteText(editor, from, to-from);
      ME_CommitUndo(editor);
      ME_UpdateRepaint(editor);
    }
    return 0;
  }
  case WM_GETTEXTLENGTH:
    return ME_GetTextLength(editor);
  case EM_GETTEXTLENGTHEX:
    return ME_GetTextLengthEx(editor, (GETTEXTLENGTHEX *)wParam);
  case WM_GETTEXT:
  {
    TEXTRANGEW tr; /* W and A differ only by rng->lpstrText */
    tr.chrg.cpMin = 0;
    tr.chrg.cpMax = wParam-1;
    tr.lpstrText = (WCHAR *)lParam;
    return RichEditANSIWndProc(hWnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
  }
  case EM_GETTEXTEX:
  {
    GETTEXTEX *ex = (GETTEXTEX*)wParam;

    if (ex->flags != 0)
        FIXME("Unhandled EM_GETTEXTEX flags 0x%lx\n",ex->flags);

    if (IsWindowUnicode(hWnd))
      return ME_GetTextW(editor, (LPWSTR)lParam, 0, ex->cb, FALSE);
    else
    {
        LPWSTR buffer = HeapAlloc(GetProcessHeap(),0,ex->cb*sizeof(WCHAR));
        DWORD buflen = ex->cb;
        LRESULT rc;
        DWORD flags = 0;

        buflen = ME_GetTextW(editor, buffer, 0, buflen, FALSE);
        rc = WideCharToMultiByte(ex->codepage, flags, buffer, buflen, (LPSTR)lParam, ex->cb, ex->lpDefaultChar, ex->lpUsedDefaultChar);

        HeapFree(GetProcessHeap(),0,buffer);
        return rc;
    }
  }
  case EM_GETSELTEXT:
  {
    int from, to;
    TEXTRANGEW tr; /* W and A differ only by rng->lpstrText */
    ME_GetSelection(editor, &from, &to);
    tr.chrg.cpMin = from;
    tr.chrg.cpMax = to;
    tr.lpstrText = (WCHAR *)lParam;
    return RichEditANSIWndProc(hWnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
  }
  case EM_GETTEXTRANGE:
  {
    TEXTRANGEW *rng = (TEXTRANGEW *)lParam;
    if (IsWindowUnicode(hWnd))
      return ME_GetTextW(editor, rng->lpstrText, rng->chrg.cpMin, rng->chrg.cpMax-rng->chrg.cpMin, FALSE);
    else
    {
      int nLen = rng->chrg.cpMax-rng->chrg.cpMin;
      WCHAR *p = ALLOC_N_OBJ(WCHAR, nLen+1);
      int nChars = ME_GetTextW(editor, p, rng->chrg.cpMin, nLen, FALSE);
      /* FIXME this is a potential security hole (buffer overrun) 
         if you know more about wchar->mbyte conversion please explain
      */
      WideCharToMultiByte(CP_ACP, 0, p, nChars+1, (char *)rng->lpstrText, nLen+1, NULL, NULL);
      FREE_OBJ(p);
      return nChars;
    }
    return ME_GetTextW(editor, rng->lpstrText, rng->chrg.cpMin, rng->chrg.cpMax-rng->chrg.cpMin, FALSE);
  }
  case EM_GETLINECOUNT:
  {
    ME_DisplayItem *item = editor->pBuffer->pFirst->next;
    int nRows = 0;

    while (item != editor->pBuffer->pLast)
    {
      assert(item->type == diParagraph);
      nRows += item->member.para.nRows;
      item = item->member.para.next_para;
    }
    TRACE("EM_GETLINECOUNT: nRows==%d\n", nRows);
    return max(1, nRows);
  }
  case EM_LINEFROMCHAR:
  {
    if (wParam == -1)
      return ME_RowNumberFromCharOfs(editor, ME_GetCursorOfs(editor, 1));
    else
      return ME_RowNumberFromCharOfs(editor, wParam);
  }
  case EM_EXLINEFROMCHAR:
  {
    return ME_RowNumberFromCharOfs(editor, lParam);
  }
  case EM_LINEINDEX:
  {
    ME_DisplayItem *item, *para;
    int nCharOfs;
    
    if (wParam == -1)
      item = ME_FindItemBack(editor->pCursors[0].pRun, diStartRow);
    else
      item = ME_FindRowWithNumber(editor, wParam);
    if (!item)
      return -1;
    para = ME_GetParagraph(item);
    item = ME_FindItemFwd(item, diRun);
    nCharOfs = para->member.para.nCharOfs + item->member.run.nCharOfs;
    TRACE("EM_LINEINDEX: nCharOfs==%d\n", nCharOfs);
    return nCharOfs;
  }
  case EM_LINELENGTH:
  {
    ME_DisplayItem *item, *item_end;
    int nChars = 0;
    
    if (wParam > ME_GetTextLength(editor))
      return 0;
    if (wParam == -1)
    {
      FIXME("EM_LINELENGTH: returning number of unselected characters on lines with selection unsupported.\n");
      return 0;
    }
    item = ME_FindItemAtOffset(editor, diRun, wParam, NULL);
    item = ME_RowStart(item);
    item_end = ME_RowEnd(item);
    if (!item_end)
    {
      /* Empty buffer, no runs */
      nChars = 0;
    }
    else
    {
      nChars = ME_CharOfsFromRunOfs(editor, item_end, ME_StrLen(item_end->member.run.strText));
      nChars -= ME_CharOfsFromRunOfs(editor, item, 0);
    }
    TRACE("EM_LINELENGTH(%d)==%d\n",wParam, nChars);
    return nChars;
  }
  case EM_FINDTEXT:
  {
    FINDTEXTA *ft = (FINDTEXTA *)lParam;
    int nChars = MultiByteToWideChar(CP_ACP, 0, ft->lpstrText, -1, NULL, 0);
    WCHAR *tmp;
    
    if ((tmp = ALLOC_N_OBJ(WCHAR, nChars)) != NULL)
      MultiByteToWideChar(CP_ACP, 0, ft->lpstrText, -1, tmp, nChars);
    return ME_FindText(editor, wParam, &ft->chrg, tmp, NULL);
  }
  case EM_FINDTEXTEX:
  {
    FINDTEXTEXA *ex = (FINDTEXTEXA *)lParam;
    int nChars = MultiByteToWideChar(CP_ACP, 0, ex->lpstrText, -1, NULL, 0);
    WCHAR *tmp;
    
    if ((tmp = ALLOC_N_OBJ(WCHAR, nChars)) != NULL)
      MultiByteToWideChar(CP_ACP, 0, ex->lpstrText, -1, tmp, nChars);
    return ME_FindText(editor, wParam, &ex->chrg, tmp, &ex->chrgText);
  }
  case EM_FINDTEXTW:
  {
    FINDTEXTW *ft = (FINDTEXTW *)lParam;
    return ME_FindText(editor, wParam, &ft->chrg, ft->lpstrText, NULL);
  }
  case EM_FINDTEXTEXW:
  {
    FINDTEXTEXW *ex = (FINDTEXTEXW *)lParam;
    return ME_FindText(editor, wParam, &ex->chrg, ex->lpstrText, &ex->chrgText);
  }
  case EM_GETZOOM:
    if (!wParam || !lParam)
      return FALSE;
    *(int *)wParam = editor->nZoomNumerator;
    *(int *)lParam = editor->nZoomDenominator;
    return TRUE;
  case EM_SETZOOM:
    return ME_SetZoom(editor, wParam, lParam);
  case WM_CREATE:
    ME_CommitUndo(editor);
    ME_WrapMarkedParagraphs(editor);
    ME_MoveCaret(editor);
    return 0;
  case WM_DESTROY:
    ME_DestroyEditor(editor);
    SetWindowLongW(hWnd, 0, 0);
    return 0;
  case WM_LBUTTONDOWN:
    SetFocus(hWnd);
    ME_LButtonDown(editor, (short)LOWORD(lParam), (short)HIWORD(lParam));
    SetCapture(hWnd);
    break;
  case WM_MOUSEMOVE:
    if (GetCapture() == hWnd)
      ME_MouseMove(editor, (short)LOWORD(lParam), (short)HIWORD(lParam));
    break;
  case WM_LBUTTONUP:
    if (GetCapture() == hWnd)
      ReleaseCapture();
    break;
  case WM_PAINT:
    hDC = BeginPaint(hWnd, &ps);
    ME_PaintContent(editor, hDC, FALSE, &ps.rcPaint);
    EndPaint(hWnd, &ps);
    break;
  case WM_SETFOCUS:
    ME_ShowCaret(editor);
    ME_SendOldNotify(editor, EN_SETFOCUS);
    return 0;
  case WM_KILLFOCUS:
    ME_HideCaret(editor);
    ME_SendOldNotify(editor, EN_KILLFOCUS);
    return 0;
  case WM_ERASEBKGND:
  {
    HDC hDC = (HDC)wParam;
    RECT rc;
    COLORREF rgbBG = ME_GetBackColor(editor);
    if (GetUpdateRect(hWnd,&rc,TRUE))
    {
      HBRUSH hbr = CreateSolidBrush(rgbBG);
      FillRect(hDC, &rc, hbr);
      DeleteObject(hbr);
    }
    return 1;
  }
  case WM_COMMAND:
    TRACE("editor wnd command = %d\n", LOWORD(wParam));
    return 0;
  case WM_KEYDOWN:
    if (ME_ArrowKey(editor, LOWORD(wParam), GetKeyState(VK_CONTROL)<0)) {
      ME_CommitUndo(editor);
      ME_EnsureVisible(editor, editor->pCursors[0].pRun);
      HideCaret(hWnd);
      ME_MoveCaret(editor);
      ShowCaret(hWnd);
      return 0;
    }
    if (GetKeyState(VK_CONTROL)<0)
    {
      if (LOWORD(wParam)=='W')
      {
        CHARFORMAT2W chf;
        char buf[2048];
        ME_GetSelectionCharFormat(editor, &chf);
        ME_DumpStyleToBuf(&chf, buf);
        MessageBoxA(NULL, buf, "Style dump", MB_OK);
      }
      if (LOWORD(wParam)=='Q')
      {
        ME_CheckCharOffsets(editor);
      }
    }
    goto do_default;
  case WM_CHAR: 
  {
    WCHAR wstr;
    if (GetWindowLongW(editor->hWnd, GWL_STYLE) & ES_READONLY) {
      MessageBeep(MB_ICONERROR);
      return 0; /* FIXME really 0 ? */
    }
    wstr = LOWORD(wParam);
    if (((unsigned)wstr)>=' ' || wstr=='\r' || wstr=='\t') {
      /* FIXME maybe it would make sense to call EM_REPLACESEL instead ? */
      ME_Style *style = ME_GetInsertStyle(editor, 0);
      ME_SaveTempStyle(editor);
      ME_InsertTextFromCursor(editor, 0, &wstr, 1, style);
      ME_ReleaseStyle(style);
      ME_CommitUndo(editor);
      ME_UpdateRepaint(editor);
    }
    return 0;
  }
  case WM_VSCROLL: 
  {
    int nPos = editor->nScrollPosY;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE|SIF_POS|SIF_RANGE|SIF_TRACKPOS;
    GetScrollInfo(hWnd, SB_VERT, &si);
    switch(LOWORD(wParam)) {
    case SB_LINEUP:
      nPos -= 24; /* FIXME follow the original */
      if (nPos<0) nPos = 0;
      break;
    case SB_LINEDOWN:
    {
      int nEnd = editor->nTotalLength - editor->sizeWindow.cy;
      nPos += 24; /* FIXME follow the original */
      if (nPos>=nEnd) nPos = nEnd;
      break;
    }
    case SB_PAGEUP:
      nPos -= editor->sizeWindow.cy;
      if (nPos<0) nPos = 0;
      break;
    case SB_PAGEDOWN:
      nPos += editor->sizeWindow.cy;
      if (nPos>=editor->nTotalLength) nPos = editor->nTotalLength-1;
      break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
      nPos = si.nTrackPos;
      break;
    }
    if (nPos != editor->nScrollPosY) {
      ScrollWindow(hWnd, 0, editor->nScrollPosY-nPos, NULL, NULL);
      editor->nScrollPosY = nPos;
      SetScrollPos(hWnd, SB_VERT, nPos, TRUE);
      UpdateWindow(hWnd);
    }
    break;
  }
  case WM_MOUSEWHEEL:
  {
    int gcWheelDelta = 0, nPos = editor->nScrollPosY, nEnd = editor->nTotalLength - editor->sizeWindow.cy; 
    UINT pulScrollLines;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);
    gcWheelDelta -= GET_WHEEL_DELTA_WPARAM(wParam);
    if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
      nPos += pulScrollLines * (gcWheelDelta / WHEEL_DELTA) * 8; /* FIXME follow the original */
    if (nPos>=nEnd)
      nPos = nEnd;
    if (nPos<0)
      nPos = 0;
    if (nPos != editor->nScrollPosY) {
      ScrollWindow(hWnd, 0, editor->nScrollPosY-nPos, NULL, NULL);
      editor->nScrollPosY = nPos;
      SetScrollPos(hWnd, SB_VERT, nPos, TRUE);
      UpdateWindow(hWnd);
    }
    break;
  }
  case WM_SIZE:
  {
    ME_RewrapRepaint(editor);
    return DefWindowProcW(hWnd, msg, wParam, lParam);
  }
  case EM_GETOLEINTERFACE:
  {
    LPVOID *ppvObj = (LPVOID*) lParam;
    FIXME("EM_GETOLEINTERFACE %p: stub\n", ppvObj);
    return CreateIRichEditOle(ppvObj);
  }
  default:
  do_default:
    return DefWindowProcW(hWnd, msg, wParam, lParam);
  }
  return 0L;
}


/******************************************************************
 *        RichEdit10ANSIWndProc (RICHED20.9)
 */
LRESULT WINAPI RichEdit10ANSIWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result;
  
  /* FIXME: this is NOT the same as 2.0 version */
  result = RichEditANSIWndProc(hWnd, msg, wParam, lParam);
  if (msg == WM_NCCREATE)
  {
    ME_TextEditor *editor = (ME_TextEditor *)GetWindowLongW(hWnd, 0);
    
    editor->bEmulateVersion10 = TRUE;
    editor->pBuffer->pLast->member.para.nCharOfs = 2;
  }
  return result;
}

void ME_SendOldNotify(ME_TextEditor *editor, int nCode)
{
  HWND hWnd = editor->hWnd;
  SendMessageA(GetParent(hWnd), WM_COMMAND, (nCode<<16)|GetWindowLongW(hWnd, GWLP_ID), (LPARAM)hWnd);
}

int ME_CountParagraphsBetween(ME_TextEditor *editor, int from, int to)
{
  ME_DisplayItem *item = ME_FindItemFwd(editor->pBuffer->pFirst, diParagraph);
  int i = 0;
  
  while(item && item->member.para.next_para->member.para.nCharOfs <= from)
    item = item->member.para.next_para;
  if (!item)
    return 0;
  while(item && item->member.para.next_para->member.para.nCharOfs <= to) {
    item = item->member.para.next_para;
    i++;
  }
  return i;
}


int ME_GetTextW(ME_TextEditor *editor, WCHAR *buffer, int nStart, int nChars, int bCRLF)
{
  ME_DisplayItem *item = ME_FindItemAtOffset(editor, diRun, nStart, &nStart);
  int nWritten = 0;
  
  if (!item) {
    *buffer = L'\0';
    return 0;
  }
  assert(item);    
  
  if (nStart)
  {
    int nLen = ME_StrLen(item->member.run.strText) - nStart;
    if (nLen > nChars)
      nLen = nChars;
    CopyMemory(buffer, item->member.run.strText->szData + nStart, sizeof(WCHAR)*nLen);
    nChars -= nLen;
    nWritten += nLen;
    if (!nChars)
      return nWritten;
    buffer += nLen;
    nStart = 0;
    item = ME_FindItemFwd(item, diRun);
  }
  
  while(nChars && item)
  {
    int nLen = ME_StrLen(item->member.run.strText);
    if (nLen > nChars)
      nLen = nChars;
      
    if (item->member.run.nFlags & MERF_ENDPARA)
    {
      if (bCRLF) {
        *buffer++ = '\r';
        nWritten++;
      }        
      *buffer = '\n';
      assert(nLen == 1);
    }
    else      
      CopyMemory(buffer, item->member.run.strText->szData, sizeof(WCHAR)*nLen);
    nChars -= nLen;
    nWritten += nLen;
    buffer += nLen;    
      
    if (!nChars)
    {
      *buffer = L'\0';
      return nWritten;
    }
    item = ME_FindItemFwd(item, diRun);
  }
  *buffer = L'\0';
  return nWritten;  
}

void ME_RegisterEditorClass(HINSTANCE hInstance)
{
  BOOL bResult;
  WNDCLASSW wcW;
  WNDCLASSA wcA;
  
  wcW.style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
  wcW.lpfnWndProc = RichEditANSIWndProc;
  wcW.cbClsExtra = 0;
  wcW.cbWndExtra = 4;
  wcW.hInstance = NULL; /* hInstance would register DLL-local class */
  wcW.hIcon = NULL;
  wcW.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_IBEAM));
  wcW.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wcW.lpszMenuName = NULL;
  wcW.lpszClassName = wszClassName;
  bResult = RegisterClassW(&wcW);  
  assert(bResult);
  wcW.lpszClassName = wszClassName50;
  bResult = RegisterClassW(&wcW);  
  assert(bResult);

  wcA.style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
  wcA.lpfnWndProc = RichEditANSIWndProc;
  wcA.cbClsExtra = 0;
  wcA.cbWndExtra = 4;
  wcA.hInstance = NULL; /* hInstance would register DLL-local class */
  wcA.hIcon = NULL;
  wcA.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_IBEAM));
  wcA.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wcA.lpszMenuName = NULL;
  wcA.lpszClassName = "RichEdit20A";
  bResult = RegisterClassA(&wcA);  
  assert(bResult);
  wcA.lpszClassName = "RichEdit50A";
  bResult = RegisterClassA(&wcA);  
  assert(bResult);
}
/******************************************************************
 *        CreateTextServices (RICHED20.4)
 *
 * FIXME should be ITextHost instead of void*
 */
HRESULT WINAPI CreateTextServices(IUnknown *punkOuter, void *pITextHost,
    IUnknown **ppUnk)
{
  FIXME("stub\n");
  /* FIXME should support aggregation */
  if (punkOuter)
    return CLASS_E_NOAGGREGATION;
    
  return E_FAIL; /* E_NOTIMPL isn't allowed by MSDN */
}

/******************************************************************
 *        REExtendedRegisterClass (RICHED20.8)
 *
 * FIXME undocumented
 */
void WINAPI REExtendedRegisterClass(void)
{
  FIXME("stub\n");
}
