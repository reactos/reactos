/*
 * RichEdit - functions dealing with editor object
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2005 by Cihan Altinay
 * Copyright 2005 by Phil Krylov
 * Copyright 2008 Eric Pouech
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
  API implementation status:
  
  Messages (ANSI versions not done yet)
  + EM_AUTOURLDETECT 2.0
  + EM_CANPASTE
  + EM_CANREDO 2.0
  + EM_CANUNDO
  + EM_CHARFROMPOS
  - EM_DISPLAYBAND
  + EM_EMPTYUNDOBUFFER
  + EM_EXGETSEL
  + EM_EXLIMITTEXT
  + EM_EXLINEFROMCHAR
  + EM_EXSETSEL
  + EM_FINDTEXT (only FR_DOWN flag implemented)
  + EM_FINDTEXTEX (only FR_DOWN flag implemented)
  - EM_FINDWORDBREAK
  - EM_FMTLINES
  - EM_FORMATRANGE
  + EM_GETAUTOURLDETECT 2.0
  - EM_GETBIDIOPTIONS 3.0
  - EM_GETCHARFORMAT (partly done)
  - EM_GETEDITSTYLE
  + EM_GETEVENTMASK
  + EM_GETFIRSTVISIBLELINE (can be optimized if needed)
  - EM_GETIMECOLOR 1.0asian
  - EM_GETIMECOMPMODE 2.0
  - EM_GETIMEOPTIONS 1.0asian
  - EM_GETIMESTATUS
  - EM_GETLANGOPTIONS 2.0
  + EM_GETLIMITTEXT
  + EM_GETLINE
  + EM_GETLINECOUNT   returns number of rows, not of paragraphs
  + EM_GETMODIFY
  - EM_GETOLEINTERFACE
  + EM_GETOPTIONS
  + EM_GETPARAFORMAT
  + EM_GETPASSWORDCHAR 2.0
  - EM_GETPUNCTUATION 1.0asian
  + EM_GETRECT
  - EM_GETREDONAME 2.0
  + EM_GETSEL
  + EM_GETSELTEXT (ANSI&Unicode)
  + EM_GETSCROLLPOS 3.0 (only Y value valid)
! - EM_GETTHUMB
  + EM_GETTEXTEX 2.0
  + EM_GETTEXTLENGTHEX (GTL_PRECISE unimplemented)
  - EM_GETTEXTMODE 2.0
? + EM_GETTEXTRANGE (ANSI&Unicode)
  - EM_GETTYPOGRAPHYOPTIONS 3.0
  - EM_GETUNDONAME
  + EM_GETWORDBREAKPROC
  - EM_GETWORDBREAKPROCEX
  - EM_GETWORDWRAPMODE 1.0asian
  + EM_GETZOOM 3.0
  + EM_HIDESELECTION
  + EM_LIMITTEXT (Also called EM_SETLIMITTEXT)
  + EM_LINEFROMCHAR
  + EM_LINEINDEX
  + EM_LINELENGTH
  + EM_LINESCROLL
  - EM_PASTESPECIAL
  + EM_POSFROMCHAR
  + EM_REDO 2.0
  + EM_REQUESTRESIZE
  + EM_REPLACESEL (proper style?) ANSI&Unicode
  + EM_SCROLL
  + EM_SCROLLCARET
  - EM_SELECTIONTYPE
  - EM_SETBIDIOPTIONS 3.0
  + EM_SETBKGNDCOLOR
  + EM_SETCHARFORMAT (partly done, no ANSI)
  - EM_SETEDITSTYLE
  + EM_SETEVENTMASK (few notifications supported)
  - EM_SETFONTSIZE
  - EM_SETIMECOLOR 1.0asian
  - EM_SETIMEOPTIONS 1.0asian
  - EM_SETLANGOPTIONS 2.0
  - EM_SETLIMITTEXT
  + EM_SETMODIFY (not sure if implementation is correct)
  - EM_SETOLECALLBACK
  + EM_SETOPTIONS (partially implemented)
  - EM_SETPALETTE 2.0
  + EM_SETPARAFORMAT
  + EM_SETPASSWORDCHAR 2.0
  - EM_SETPUNCTUATION 1.0asian
  + EM_SETREADONLY no beep on modification attempt
  + EM_SETRECT
  + EM_SETRECTNP (EM_SETRECT without repainting)
  + EM_SETSEL
  + EM_SETSCROLLPOS 3.0
  - EM_SETTABSTOPS 3.0
  - EM_SETTARGETDEVICE (partial)
  + EM_SETTEXTEX 3.0 (proper style?)
  - EM_SETTEXTMODE 2.0
  - EM_SETTYPOGRAPHYOPTIONS 3.0
  + EM_SETUNDOLIMIT 2.0
  + EM_SETWORDBREAKPROC (used only for word movement at the moment)
  - EM_SETWORDBREAKPROCEX
  - EM_SETWORDWRAPMODE 1.0asian
  + EM_SETZOOM 3.0
  + EM_SHOWSCROLLBAR 2.0
  + EM_STOPGROUPTYPING 2.0
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
  + WM_SETFONT
  + WM_SETTEXT (resets undo stack !) (proper style?) ANSI&Unicode
  - WM_STYLECHANGING
  - WM_STYLECHANGED (things like read-only flag)
  + WM_UNICHAR

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
  + EN_REQUESTRESIZE
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
  + ES_DISABLENOSCROLL (scrollbar is always visible)
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
  + WS_VSCROLL
*/

/*
 * RICHED20 TODO (incomplete):
 *
 * - messages/styles/notifications listed above 
 * - add remaining CHARFORMAT/PARAFORMAT fields
 * - right/center align should strip spaces from the beginning
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
 * - ListBox & ComboBox not implemented
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
#include "winreg.h"
#define NO_SHLWAPI_STREAM 
#include "shlwapi.h"
#include "rtf.h"
#include "imm.h"
#include "res.h"

#define STACK_SIZE_DEFAULT  100
#define STACK_SIZE_MAX     1000

#define TEXT_LIMIT_DEFAULT 32767
 
WINE_DEFAULT_DEBUG_CHANNEL(richedit);

static BOOL ME_RegisterEditorClass(HINSTANCE);

static const WCHAR RichEdit20W[] = {'R', 'i', 'c', 'h', 'E', 'd', 'i', 't', '2', '0', 'W', 0};
static const WCHAR RichEdit50W[] = {'R', 'i', 'c', 'h', 'E', 'd', 'i', 't', '5', '0', 'W', 0};
static const WCHAR REListBox20W[] = {'R','E','L','i','s','t','B','o','x','2','0','W', 0};
static const WCHAR REComboBox20W[] = {'R','E','C','o','m','b','o','B','o','x','2','0','W', 0};
static HCURSOR hLeft;

int me_debug = 0;
HANDLE me_heap = NULL;

static BOOL ME_ListBoxRegistered = FALSE;
static BOOL ME_ComboBoxRegistered = FALSE;

static inline int is_version_nt(void)
{
    return !(GetVersion() & 0x80000000);
}

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
  
  TRACE("%08x %p\n", dwFormat, stream);
  
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
    if (stream->dwSize == 0)
      break;
    stream->dwSize = 0;
  } while(1);
  ME_CommitUndo(editor);
  ME_UpdateRepaint(editor);
  return 0;
}

static void ME_ApplyBorderProperties(RTF_Info *info,
                                     ME_BorderRect *borderRect,
                                     RTFBorder *borderDef)
{
  int i, colorNum;
  ME_Border *pBorders[] = {&borderRect->top,
                           &borderRect->left,
                           &borderRect->bottom,
                           &borderRect->right};
  for (i = 0; i < 4; i++)
  {
    RTFColor *colorDef = info->colorList;
    pBorders[i]->width = borderDef[i].width;
    colorNum = borderDef[i].color;
    while (colorDef && colorDef->rtfCNum != colorNum)
      colorDef = colorDef->rtfNextColor;
    if (colorDef)
      pBorders[i]->colorRef = RGB(
                           colorDef->rtfCRed >= 0 ? colorDef->rtfCRed : 0,
                           colorDef->rtfCGreen >= 0 ? colorDef->rtfCGreen : 0,
                           colorDef->rtfCBlue >= 0 ? colorDef->rtfCBlue : 0);
    else
      pBorders[i]->colorRef = RGB(0, 0, 0);
  }
}

void ME_RTFCharAttrHook(RTF_Info *info)
{
  CHARFORMAT2W fmt;
  fmt.cbSize = sizeof(fmt);
  fmt.dwMask = 0;
  fmt.dwEffects = 0;

  switch(info->rtfMinor)
  {
    case rtfPlain:
      /* FIXME add more flags once they're implemented */
      fmt.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINETYPE | CFM_STRIKEOUT | CFM_COLOR | CFM_BACKCOLOR | CFM_SIZE | CFM_WEIGHT;
      fmt.dwEffects = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;
      fmt.yHeight = 12*20; /* 12pt */
      fmt.wWeight = FW_NORMAL;
      fmt.bUnderlineType = CFU_UNDERLINENONE;
      break;
    case rtfBold:
      fmt.dwMask = CFM_BOLD | CFM_WEIGHT;
      fmt.dwEffects = info->rtfParam ? CFE_BOLD : 0;
      fmt.wWeight = info->rtfParam ? FW_BOLD : FW_NORMAL;
      break;
    case rtfItalic:
      fmt.dwMask = CFM_ITALIC;
      fmt.dwEffects = info->rtfParam ? fmt.dwMask : 0;
      break;
    case rtfUnderline:
      fmt.dwMask = CFM_UNDERLINETYPE;
      fmt.bUnderlineType = info->rtfParam ? CFU_CF1UNDERLINE : CFU_UNDERLINENONE;
      break;
    case rtfDotUnderline:
      fmt.dwMask = CFM_UNDERLINETYPE;
      fmt.bUnderlineType = info->rtfParam ? CFU_UNDERLINEDOTTED : CFU_UNDERLINENONE;
      break;
    case rtfDbUnderline:
      fmt.dwMask = CFM_UNDERLINETYPE;
      fmt.bUnderlineType = info->rtfParam ? CFU_UNDERLINEDOUBLE : CFU_UNDERLINENONE;
      break;
    case rtfWordUnderline:
      fmt.dwMask = CFM_UNDERLINETYPE;
      fmt.bUnderlineType = info->rtfParam ? CFU_UNDERLINEWORD : CFU_UNDERLINENONE;
      break;
    case rtfNoUnderline:
      fmt.dwMask = CFM_UNDERLINETYPE;
      fmt.bUnderlineType = CFU_UNDERLINENONE;
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
    case rtfInvisible:
      fmt.dwMask = CFM_HIDDEN;
      fmt.dwEffects = info->rtfParam ? fmt.dwMask : 0;
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
        if (c)
          fmt.crTextColor = (c->rtfCBlue<<16)|(c->rtfCGreen<<8)|(c->rtfCRed);
        else
          fmt.crTextColor = 0;
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
          fmt.bPitchAndFamily = f->rtfFPitch | (f->rtfFFamily << 4);
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
    info->styleChanged = TRUE;
  }
}

/* FIXME this function doesn't get any information about context of the RTF tag, which is very bad,
   the same tags mean different things in different contexts */
void ME_RTFParAttrHook(RTF_Info *info)
{
  PARAFORMAT2 fmt;
  fmt.cbSize = sizeof(fmt);
  fmt.dwMask = 0;
  
  switch(info->rtfMinor)
  {
  case rtfParDef: /* restores default paragraph attributes */
    if (!info->editor->bEmulateVersion10) /* v4.1 */
      info->borderType = RTFBorderParaLeft;
    else /* v1.0 - 3.0 */
      info->borderType = RTFBorderParaTop;
    fmt.dwMask = PFM_ALIGNMENT | PFM_BORDER | PFM_LINESPACING | PFM_TABSTOPS |
        PFM_OFFSET | PFM_RIGHTINDENT | PFM_SPACEAFTER | PFM_SPACEBEFORE |
        PFM_STARTINDENT;
    /* TODO: numbering, shading */
    fmt.wAlignment = PFA_LEFT;
    fmt.cTabCount = 0;
    fmt.dxOffset = fmt.dxStartIndent = fmt.dxRightIndent = 0;
    fmt.wBorderWidth = fmt.wBorders = 0;
    fmt.wBorderSpace = 0;
    fmt.bLineSpacingRule = 0;
    fmt.dySpaceBefore = fmt.dySpaceAfter = 0;
    fmt.dyLineSpacing = 0;
    if (!info->editor->bEmulateVersion10) /* v4.1 */
    {
      if (info->tableDef && info->tableDef->tableRowStart &&
          info->tableDef->tableRowStart->member.para.nFlags & MEPF_ROWEND)
      {
        ME_Cursor cursor;
        ME_DisplayItem *para;
        /* We are just after a table row. */
        RTFFlushOutputBuffer(info);
        cursor = info->editor->pCursors[0];
        para = ME_GetParagraph(cursor.pRun);
        if (para  == info->tableDef->tableRowStart->member.para.next_para
            && !cursor.nOffset && !cursor.pRun->member.run.nCharOfs)
        {
          /* Since the table row end, no text has been inserted, and the \intbl
           * control word has not be used.  We can confirm that we are not in a
           * table anymore.
           */
          info->tableDef->tableRowStart = NULL;
          info->canInheritInTbl = FALSE;
        }
      }
    } else { /* v1.0 - v3.0 */
      fmt.dwMask |= PFM_TABLE;
      fmt.wEffects &= ~PFE_TABLE;
    }
    break;
  case rtfNestLevel:
    if (!info->editor->bEmulateVersion10) /* v4.1 */
    {
      while (info->rtfParam > info->nestingLevel) {
        RTFTable *tableDef = ALLOC_OBJ(RTFTable);
        ZeroMemory(tableDef, sizeof(RTFTable));
        tableDef->parent = info->tableDef;
        info->tableDef = tableDef;

        RTFFlushOutputBuffer(info);
        if (tableDef->tableRowStart &&
            tableDef->tableRowStart->member.para.nFlags & MEPF_ROWEND)
        {
          ME_DisplayItem *para = tableDef->tableRowStart;
          para = para->member.para.next_para;
          para = ME_InsertTableRowStartAtParagraph(info->editor, para);
          tableDef->tableRowStart = para;
        } else {
          ME_Cursor cursor;
          WCHAR endl = '\r';
          cursor = info->editor->pCursors[0];
          if (cursor.nOffset || cursor.pRun->member.run.nCharOfs)
            ME_InsertTextFromCursor(info->editor, 0, &endl, 1, info->style);
          tableDef->tableRowStart = ME_InsertTableRowStartFromCursor(info->editor);
        }

        info->nestingLevel++;
      }
      info->canInheritInTbl = FALSE;
    }
    break;
  case rtfInTable:
  {
    if (!info->editor->bEmulateVersion10) /* v4.1 */
    {
      if (info->nestingLevel < 1)
      {
        RTFTable *tableDef;
        if (!info->tableDef)
        {
            info->tableDef = ALLOC_OBJ(RTFTable);
            ZeroMemory(info->tableDef, sizeof(RTFTable));
        }
        tableDef = info->tableDef;
        RTFFlushOutputBuffer(info);
        if (tableDef->tableRowStart &&
            tableDef->tableRowStart->member.para.nFlags & MEPF_ROWEND)
        {
          ME_DisplayItem *para = tableDef->tableRowStart;
          para = para->member.para.next_para;
          para = ME_InsertTableRowStartAtParagraph(info->editor, para);
          tableDef->tableRowStart = para;
        } else {
          ME_Cursor cursor;
          WCHAR endl = '\r';
          cursor = info->editor->pCursors[0];
          if (cursor.nOffset || cursor.pRun->member.run.nCharOfs)
            ME_InsertTextFromCursor(info->editor, 0, &endl, 1, info->style);
          tableDef->tableRowStart = ME_InsertTableRowStartFromCursor(info->editor);
        }
        info->nestingLevel = 1;
        info->canInheritInTbl = TRUE;
      }
      return;
    } else { /* v1.0 - v3.0 */
      fmt.dwMask |= PFM_TABLE;
      fmt.wEffects |= PFE_TABLE;
    }
    break;
  }
  case rtfFirstIndent:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    fmt.dwMask = PFM_STARTINDENT | PFM_OFFSET;
    fmt.dxStartIndent += fmt.dxOffset + info->rtfParam;
    fmt.dxOffset = -info->rtfParam;
    break;
  case rtfLeftIndent:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    fmt.dwMask = PFM_STARTINDENT;
    fmt.dxStartIndent = info->rtfParam - fmt.dxOffset;
    break;
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
      fmt.cTabCount = 0;
    }
    if (fmt.cTabCount < MAX_TAB_STOPS && info->rtfParam < 0x1000000)
      fmt.rgxTabs[fmt.cTabCount++] = info->rtfParam;
    fmt.dwMask = PFM_TABSTOPS;
    break;
  case rtfKeep:
    fmt.dwMask = PFM_KEEP;
    fmt.wEffects = PFE_KEEP;
    break;
  case rtfNoWidowControl:
    fmt.dwMask = PFM_NOWIDOWCONTROL;
    fmt.wEffects = PFE_NOWIDOWCONTROL;
    break;
  case rtfKeepNext:
    fmt.dwMask = PFM_KEEPNEXT;
    fmt.wEffects = PFE_KEEPNEXT;
    break;
  case rtfSpaceAfter:
    fmt.dwMask = PFM_SPACEAFTER;
    fmt.dySpaceAfter = info->rtfParam;
    break;
  case rtfSpaceBefore:
    fmt.dwMask = PFM_SPACEBEFORE;
    fmt.dySpaceBefore = info->rtfParam;
    break;
  case rtfSpaceBetween:
    fmt.dwMask = PFM_LINESPACING;
    if ((int)info->rtfParam > 0)
    {
      fmt.dyLineSpacing = info->rtfParam;
      fmt.bLineSpacingRule = 3;
    }
    else
    {
      fmt.dyLineSpacing = info->rtfParam;
      fmt.bLineSpacingRule = 4;
    }
  case rtfSpaceMultiply:
    fmt.dwMask = PFM_LINESPACING;
    fmt.dyLineSpacing = info->rtfParam * 20;
    fmt.bLineSpacingRule = 5;
    break;
  case rtfParBullet:
    fmt.dwMask = PFM_NUMBERING;
    fmt.wNumbering = PFN_BULLET;
    break;
  case rtfParSimple:
    fmt.dwMask = PFM_NUMBERING;
    fmt.wNumbering = 2; /* FIXME: MSDN says it's not used ?? */
    break;
  case rtfParNumDecimal:
    fmt.dwMask = PFM_NUMBERING;
    fmt.wNumbering = 2; /* FIXME: MSDN says it's not used ?? */
    break;
  case rtfParNumIndent:
    fmt.dwMask = PFM_NUMBERINGTAB;
    fmt.wNumberingTab = info->rtfParam;
    break;
  case rtfParNumStartAt:
    fmt.dwMask = PFM_NUMBERINGSTART;
    fmt.wNumberingStart = info->rtfParam;
    break;
  case rtfBorderLeft:
    info->borderType = RTFBorderParaLeft;
    ME_GetSelectionParaFormat(info->editor, &fmt);
    if (!(fmt.dwMask & PFM_BORDER))
    {
      fmt.wBorderSpace = 0;
      fmt.wBorderWidth = 1;
      fmt.wBorders = 0;
    }
    fmt.wBorders |= 1;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderRight:
    info->borderType = RTFBorderParaRight;
    ME_GetSelectionParaFormat(info->editor, &fmt);
    if (!(fmt.dwMask & PFM_BORDER))
    {
      fmt.wBorderSpace = 0;
      fmt.wBorderWidth = 1;
      fmt.wBorders = 0;
    }
    fmt.wBorders |= 2;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderTop:
    info->borderType = RTFBorderParaTop;
    ME_GetSelectionParaFormat(info->editor, &fmt);
    if (!(fmt.dwMask & PFM_BORDER))
    {
      fmt.wBorderSpace = 0;
      fmt.wBorderWidth = 1;
      fmt.wBorders = 0;
    }
    fmt.wBorders |= 4;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderBottom:
    info->borderType = RTFBorderParaBottom;
    ME_GetSelectionParaFormat(info->editor, &fmt);
    if (!(fmt.dwMask & PFM_BORDER))
    {
      fmt.wBorderSpace = 0;
      fmt.wBorderWidth = 1;
      fmt.wBorders = 0;
    }
    fmt.wBorders |= 8;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderSingle:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    /* we assume that borders have been created before (RTF spec) */
    fmt.wBorders &= ~0x700;
    fmt.wBorders |= 1 << 8;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderThick:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    /* we assume that borders have been created before (RTF spec) */
    fmt.wBorders &= ~0x700;
    fmt.wBorders |= 2 << 8;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderShadow:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    /* we assume that borders have been created before (RTF spec) */
    fmt.wBorders &= ~0x700;
    fmt.wBorders |= 10 << 8;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderDouble:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    /* we assume that borders have been created before (RTF spec) */
    fmt.wBorders &= ~0x700;
    fmt.wBorders |= 7 << 8;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderDot:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    /* we assume that borders have been created before (RTF spec) */
    fmt.wBorders &= ~0x700;
    fmt.wBorders |= 11 << 8;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderWidth:
  {
    int borderSide = info->borderType & RTFBorderSideMask;
    RTFTable *tableDef = info->tableDef;
    ME_GetSelectionParaFormat(info->editor, &fmt);
    /* we assume that borders have been created before (RTF spec) */
    fmt.wBorderWidth |= ((info->rtfParam / 15) & 7) << 8;
    if ((info->borderType & RTFBorderTypeMask) == RTFBorderTypeCell)
    {
      RTFBorder *border;
      if (!tableDef || tableDef->numCellsDefined >= MAX_TABLE_CELLS)
        break;
      border = &tableDef->cells[tableDef->numCellsDefined].border[borderSide];
      border->width = info->rtfParam;
      break;
    }
    fmt.dwMask = PFM_BORDER;
    break;
  }
  case rtfBorderSpace:
    ME_GetSelectionParaFormat(info->editor, &fmt);
    /* we assume that borders have been created before (RTF spec) */
    fmt.wBorderSpace = info->rtfParam;
    fmt.dwMask = PFM_BORDER;
    break;
  case rtfBorderColor:
  {
    RTFTable *tableDef = info->tableDef;
    int borderSide = info->borderType & RTFBorderSideMask;
    int borderType = info->borderType & RTFBorderTypeMask;
    switch(borderType)
    {
    case RTFBorderTypePara:
      if (!info->editor->bEmulateVersion10) /* v4.1 */
        break;
      /* v1.0 - 3.0 treat paragraph and row borders the same. */
    case RTFBorderTypeRow:
      if (tableDef) {
        tableDef->border[borderSide].color = info->rtfParam;
      }
      break;
    case RTFBorderTypeCell:
      if (tableDef && tableDef->numCellsDefined < MAX_TABLE_CELLS) {
        tableDef->cells[tableDef->numCellsDefined].border[borderSide].color = info->rtfParam;
      }
      break;
    }
    break;
  }
  }
  if (fmt.dwMask) {
    RTFFlushOutputBuffer(info);
    /* FIXME too slow ? how come ?*/
    ME_SetSelectionParaFormat(info->editor, &fmt);
  }
}

void ME_RTFTblAttrHook(RTF_Info *info)
{
  switch (info->rtfMinor)
  {
    case rtfRowDef:
    {
      if (!info->editor->bEmulateVersion10) /* v4.1 */
        info->borderType = 0; /* Not sure */
      else /* v1.0 - 3.0 */
        info->borderType = RTFBorderRowTop;
      if (!info->tableDef) {
        info->tableDef = ME_MakeTableDef(info->editor);
      } else {
        ME_InitTableDef(info->editor, info->tableDef);
      }
      break;
    }
    case rtfCellPos:
    {
      int cellNum;
      if (!info->tableDef)
      {
        info->tableDef = ME_MakeTableDef(info->editor);
      }
      cellNum = info->tableDef->numCellsDefined;
      if (cellNum >= MAX_TABLE_CELLS)
        break;
      info->tableDef->cells[cellNum].rightBoundary = info->rtfParam;
      if (cellNum < MAX_TAB_STOPS) {
        /* Tab stops were used to store cell positions before v4.1 but v4.1
         * still seems to set the tabstops without using them. */
        ME_DisplayItem *para = ME_GetParagraph(info->editor->pCursors[0].pRun);
        PARAFORMAT2 *pFmt = para->member.para.pFmt;
        pFmt->rgxTabs[cellNum] &= ~0x00FFFFFF;
        pFmt->rgxTabs[cellNum] = 0x00FFFFFF & info->rtfParam;
      }
      info->tableDef->numCellsDefined++;
      break;
    }
    case rtfRowBordTop:
      info->borderType = RTFBorderRowTop;
      break;
    case rtfRowBordLeft:
      info->borderType = RTFBorderRowLeft;
      break;
    case rtfRowBordBottom:
      info->borderType = RTFBorderRowBottom;
      break;
    case rtfRowBordRight:
      info->borderType = RTFBorderRowRight;
      break;
    case rtfCellBordTop:
      info->borderType = RTFBorderCellTop;
      break;
    case rtfCellBordLeft:
      info->borderType = RTFBorderCellLeft;
      break;
    case rtfCellBordBottom:
      info->borderType = RTFBorderCellBottom;
      break;
    case rtfCellBordRight:
      info->borderType = RTFBorderCellRight;
      break;
    case rtfRowGapH:
      if (info->tableDef)
        info->tableDef->gapH = info->rtfParam;
      break;
    case rtfRowLeftEdge:
      if (info->tableDef)
        info->tableDef->leftEdge = info->rtfParam;
      break;
  }
}

void ME_RTFSpecialCharHook(RTF_Info *info)
{
  RTFTable *tableDef = info->tableDef;
  switch (info->rtfMinor)
  {
    case rtfNestCell:
      if (info->editor->bEmulateVersion10) /* v1.0 - v3.0 */
        break;
      /* else fall through since v4.1 treats rtfNestCell and rtfCell the same */
    case rtfCell:
      if (!tableDef)
        break;
      RTFFlushOutputBuffer(info);
      if (!info->editor->bEmulateVersion10) { /* v4.1 */
        if (tableDef->tableRowStart)
        {
          if (!info->nestingLevel &&
              tableDef->tableRowStart->member.para.nFlags & MEPF_ROWEND)
          {
            ME_DisplayItem *para = tableDef->tableRowStart;
            para = para->member.para.next_para;
            para = ME_InsertTableRowStartAtParagraph(info->editor, para);
            tableDef->tableRowStart = para;
            info->nestingLevel = 1;
          }
          ME_InsertTableCellFromCursor(info->editor);
        }
      } else { /* v1.0 - v3.0 */
        ME_DisplayItem *para = ME_GetParagraph(info->editor->pCursors[0].pRun);
        PARAFORMAT2 *pFmt = para->member.para.pFmt;
        if (pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE &&
            tableDef->numCellsInserted < tableDef->numCellsDefined)
        {
          WCHAR tab = '\t';
          ME_InsertTextFromCursor(info->editor, 0, &tab, 1, info->style);
          tableDef->numCellsInserted++;
        }
      }
      break;
    case rtfNestRow:
      if (info->editor->bEmulateVersion10) /* v1.0 - v3.0 */
        break;
      /* else fall through since v4.1 treats rtfNestRow and rtfRow the same */
    case rtfRow:
    {
      ME_DisplayItem *para, *cell, *run;
      int i;

      if (!tableDef)
        break;
      RTFFlushOutputBuffer(info);
      if (!info->editor->bEmulateVersion10) { /* v4.1 */
        if (!tableDef->tableRowStart)
          break;
        if (!info->nestingLevel &&
            tableDef->tableRowStart->member.para.nFlags & MEPF_ROWEND)
        {
          para = tableDef->tableRowStart;
          para = para->member.para.next_para;
          para = ME_InsertTableRowStartAtParagraph(info->editor, para);
          tableDef->tableRowStart = para;
          info->nestingLevel++;
        }
        para = tableDef->tableRowStart;
        cell = ME_FindItemFwd(para, diCell);
        assert(cell && !cell->member.cell.prev_cell);
        if (tableDef->numCellsDefined < 1)
        {
          /* 2000 twips appears to be the cell size that native richedit uses
           * when no cell sizes are specified. */
          const int defaultCellSize = 2000;
          int nRightBoundary = defaultCellSize;
          cell->member.cell.nRightBoundary = nRightBoundary;
          while (cell->member.cell.next_cell) {
            cell = cell->member.cell.next_cell;
            nRightBoundary += defaultCellSize;
            cell->member.cell.nRightBoundary = nRightBoundary;
          }
          para = ME_InsertTableCellFromCursor(info->editor);
          cell = para->member.para.pCell;
          cell->member.cell.nRightBoundary = nRightBoundary;
        } else {
          for (i = 0; i < tableDef->numCellsDefined; i++)
          {
            RTFCell *cellDef = &tableDef->cells[i];
            cell->member.cell.nRightBoundary = cellDef->rightBoundary;
            ME_ApplyBorderProperties(info, &cell->member.cell.border,
                                     cellDef->border);
            cell = cell->member.cell.next_cell;
            if (!cell)
            {
              para = ME_InsertTableCellFromCursor(info->editor);
              cell = para->member.para.pCell;
            }
          }
          /* Cell for table row delimiter is empty */
          cell->member.cell.nRightBoundary = tableDef->cells[i-1].rightBoundary;
        }

        run = ME_FindItemFwd(cell, diRun);
        if (info->editor->pCursors[0].pRun != run ||
            info->editor->pCursors[0].nOffset)
        {
          int nOfs, nChars;
          /* Delete inserted cells that aren't defined. */
          info->editor->pCursors[1].pRun = run;
          info->editor->pCursors[1].nOffset = 0;
          nOfs = ME_GetCursorOfs(info->editor, 1);
          nChars = ME_GetCursorOfs(info->editor, 0) - nOfs;
          ME_InternalDeleteText(info->editor, nOfs, nChars, TRUE);
        }

        para = ME_InsertTableRowEndFromCursor(info->editor);
        para->member.para.pFmt->dxOffset = abs(info->tableDef->gapH);
        para->member.para.pFmt->dxStartIndent = info->tableDef->leftEdge;
        ME_ApplyBorderProperties(info, &para->member.para.border,
                                 tableDef->border);
        info->nestingLevel--;
        if (!info->nestingLevel)
        {
          if (info->canInheritInTbl) {
            tableDef->tableRowStart = para;
          } else {
            while (info->tableDef) {
              tableDef = info->tableDef;
              info->tableDef = tableDef->parent;
              heap_free(tableDef);
            }
          }
        } else {
          info->tableDef = tableDef->parent;
          heap_free(tableDef);
        }
      } else { /* v1.0 - v3.0 */
        WCHAR endl = '\r';
        ME_DisplayItem *para = ME_GetParagraph(info->editor->pCursors[0].pRun);
        PARAFORMAT2 *pFmt = para->member.para.pFmt;
        pFmt->dxOffset = info->tableDef->gapH;
        pFmt->dxStartIndent = info->tableDef->leftEdge;

        para = ME_GetParagraph(info->editor->pCursors[0].pRun);
        ME_ApplyBorderProperties(info, &para->member.para.border,
                                 tableDef->border);
        while (tableDef->numCellsInserted < tableDef->numCellsDefined)
        {
          WCHAR tab = '\t';
          ME_InsertTextFromCursor(info->editor, 0, &tab, 1, info->style);
          tableDef->numCellsInserted++;
        }
        pFmt->cTabCount = min(tableDef->numCellsDefined, MAX_TAB_STOPS);
        if (!tableDef->numCellsDefined)
          pFmt->wEffects &= ~PFE_TABLE;
        ME_InsertTextFromCursor(info->editor, 0, &endl, 1, info->style);
        tableDef->numCellsInserted = 0;
      }
      break;
    }
    case rtfTab:
    case rtfPar:
      if (info->editor->bEmulateVersion10) { /* v1.0 - 3.0 */
        ME_DisplayItem *para;
        PARAFORMAT2 *pFmt;
        RTFFlushOutputBuffer(info);
        para = ME_GetParagraph(info->editor->pCursors[0].pRun);
        pFmt = para->member.para.pFmt;
        if (pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE)
        {
          /* rtfPar is treated like a space within a table. */
          info->rtfClass = rtfText;
          info->rtfMajor = ' ';
        }
        else if (info->rtfMinor == rtfPar && tableDef)
          tableDef->numCellsInserted = 0;
      }
      break;
  }
}

static BOOL ME_RTFInsertOleObject(RTF_Info *info, HENHMETAFILE hemf, HBITMAP hbmp,
                                  const SIZEL* sz)
{
  LPOLEOBJECT         lpObject = NULL;
  LPSTORAGE           lpStorage = NULL;
  LPOLECLIENTSITE     lpClientSite = NULL;
  LPDATAOBJECT        lpDataObject = NULL;
  LPOLECACHE          lpOleCache = NULL;
  STGMEDIUM           stgm;
  FORMATETC           fm;
  CLSID               clsid;
  BOOL                ret = FALSE;
  DWORD               conn;

  if (hemf)
  {
      stgm.tymed = TYMED_ENHMF;
      stgm.u.hEnhMetaFile = hemf;
      fm.cfFormat = CF_ENHMETAFILE;
  }
  else if (hbmp)
  {
      stgm.tymed = TYMED_GDI;
      stgm.u.hBitmap = hbmp;
      fm.cfFormat = CF_BITMAP;
  }
  stgm.pUnkForRelease = NULL;

  fm.ptd = NULL;
  fm.dwAspect = DVASPECT_CONTENT;
  fm.lindex = -1;
  fm.tymed = stgm.tymed;

  if (!info->lpRichEditOle)
  {
    CreateIRichEditOle(info->editor, (VOID**)&info->lpRichEditOle);
  }

  if (OleCreateDefaultHandler(&CLSID_NULL, NULL, &IID_IOleObject, (void**)&lpObject) == S_OK &&
#if 0
      /* FIXME: enable it when rich-edit properly implements this method */
      IRichEditOle_GetClientSite(info->lpRichEditOle, &lpClientSite) == S_OK &&
      IOleObject_SetClientSite(lpObject, lpClientSite) == S_OK &&
#endif
      IOleObject_GetUserClassID(lpObject, &clsid) == S_OK &&
      IOleObject_QueryInterface(lpObject, &IID_IOleCache, (void**)&lpOleCache) == S_OK &&
      IOleCache_Cache(lpOleCache, &fm, 0, &conn) == S_OK &&
      IOleObject_QueryInterface(lpObject, &IID_IDataObject, (void**)&lpDataObject) == S_OK &&
      IDataObject_SetData(lpDataObject, &fm, &stgm, TRUE) == S_OK)
  {
    REOBJECT            reobject;

    reobject.cbStruct = sizeof(reobject);
    reobject.cp = REO_CP_SELECTION;
    reobject.clsid = clsid;
    reobject.poleobj = lpObject;
    reobject.pstg = lpStorage;
    reobject.polesite = lpClientSite;
    /* convert from twips to .01 mm */
    reobject.sizel.cx = MulDiv(sz->cx, 254, 144);
    reobject.sizel.cy = MulDiv(sz->cy, 254, 144);
    reobject.dvaspect = DVASPECT_CONTENT;
    reobject.dwFlags = 0; /* FIXME */
    reobject.dwUser = 0;

    /* FIXME: could be simpler */
    ret = IRichEditOle_InsertObject(info->lpRichEditOle, &reobject) == S_OK;
  }

  if (lpObject)       IOleObject_Release(lpObject);
  if (lpClientSite)   IOleClientSite_Release(lpClientSite);
  if (lpStorage)      IStorage_Release(lpStorage);
  if (lpDataObject)   IDataObject_Release(lpDataObject);
  if (lpOleCache)     IOleCache_Release(lpOleCache);

  return ret;
}

static void ME_RTFReadPictGroup(RTF_Info *info)
{
  SIZEL         sz;
  BYTE*         buffer = NULL;
  unsigned      bufsz, bufidx;
  BOOL          flip;
  BYTE          val;
  METAFILEPICT  mfp;
  HENHMETAFILE  hemf;
  HBITMAP       hbmp;
  enum gfxkind {gfx_unknown = 0, gfx_enhmetafile, gfx_metafile, gfx_dib} gfx = gfx_unknown;

  RTFGetToken (info);
  if (info->rtfClass == rtfEOF)
    return;
  mfp.mm = MM_TEXT;
  /* fetch picture type */
  if (RTFCheckMM (info, rtfPictAttr, rtfWinMetafile))
  {
    mfp.mm = info->rtfParam;
    gfx = gfx_metafile;
  }
  else if (RTFCheckMM (info, rtfPictAttr, rtfDevIndBitmap))
  {
    if (info->rtfParam != 0) FIXME("dibitmap should be 0 (%d)\n", info->rtfParam);
    gfx = gfx_dib;
  }
  else if (RTFCheckMM (info, rtfPictAttr, rtfEmfBlip))
  {
    gfx = gfx_enhmetafile;
  }
  else
  {
    FIXME("%d %d\n", info->rtfMajor, info->rtfMinor);
    goto skip_group;
  }
  sz.cx = sz.cy = 0;
  /* fetch picture attributes */
  for (;;)
  {
    RTFGetToken (info);
    if (info->rtfClass == rtfEOF)
      return;
    if (info->rtfClass == rtfText)
      break;
    if (!RTFCheckCM (info, rtfControl, rtfPictAttr))
    {
      ERR("Expected picture attribute (%d %d)\n",
        info->rtfClass, info->rtfMajor);
      goto skip_group;
    }
    else if (RTFCheckMM (info, rtfPictAttr, rtfPicWid))
    {
      if (gfx == gfx_metafile) mfp.xExt = info->rtfParam;
    }
    else if (RTFCheckMM (info, rtfPictAttr, rtfPicHt))
    {
      if (gfx == gfx_metafile) mfp.yExt = info->rtfParam;
    }
    else if (RTFCheckMM (info, rtfPictAttr, rtfPicGoalWid))
      sz.cx = info->rtfParam;
    else if (RTFCheckMM (info, rtfPictAttr, rtfPicGoalHt))
      sz.cy = info->rtfParam;
    else
      FIXME("Non supported attribute: %d %d %d\n", info->rtfClass, info->rtfMajor, info->rtfMinor);
  }
  /* fetch picture data */
  bufsz = 1024;
  bufidx = 0;
  buffer = HeapAlloc(GetProcessHeap(), 0, bufsz);
  val = info->rtfMajor;
  for (flip = TRUE;; flip = !flip)
  {
    RTFGetToken (info);
    if (info->rtfClass == rtfEOF)
    {
      HeapFree(GetProcessHeap(), 0, buffer);
      return; /* Warn ?? */
    }
    if (RTFCheckCM(info, rtfGroup, rtfEndGroup))
      break;
    if (info->rtfClass != rtfText) goto skip_group;
    if (flip)
    {
      if (bufidx >= bufsz &&
          !(buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, bufsz += 1024)))
        goto skip_group;
      buffer[bufidx++] = RTFCharToHex(val) * 16 + RTFCharToHex(info->rtfMajor);
    }
    else
      val = info->rtfMajor;
  }
  if (flip) FIXME("wrong hex string\n");

  switch (gfx)
  {
  case gfx_enhmetafile:
    if ((hemf = SetEnhMetaFileBits(bufidx, buffer)))
      ME_RTFInsertOleObject(info, hemf, NULL, &sz);
    break;
  case gfx_metafile:
    if ((hemf = SetWinMetaFileBits(bufidx, buffer, NULL, &mfp)))
        ME_RTFInsertOleObject(info, hemf, NULL, &sz);
    break;
  case gfx_dib:
    {
      BITMAPINFO* bi = (BITMAPINFO*)buffer;
      HDC         hdc = GetDC(0);
      unsigned    nc = bi->bmiHeader.biClrUsed;

      /* not quite right, especially for bitfields type of compression */
      if (!nc && bi->bmiHeader.biBitCount <= 8)
        nc = 1 << bi->bmiHeader.biBitCount;
      if ((hbmp = CreateDIBitmap(hdc, &bi->bmiHeader,
                                 CBM_INIT, (char*)(bi + 1) + nc * sizeof(RGBQUAD),
                                 bi, DIB_RGB_COLORS)))
          ME_RTFInsertOleObject(info, NULL, hbmp, &sz);
      ReleaseDC(0, hdc);
    }
    break;
  default:
    break;
  }
  HeapFree(GetProcessHeap(), 0, buffer);
  RTFRouteToken (info);	/* feed "}" back to router */
  return;
skip_group:
  HeapFree(GetProcessHeap(), 0, buffer);
  RTFSkipGroup(info);
  RTFRouteToken(info);	/* feed "}" back to router */
}

/* for now, lookup the \result part and use it, whatever the object */
static void ME_RTFReadObjectGroup(RTF_Info *info)
{
  for (;;)
  {
    RTFGetToken (info);
    if (info->rtfClass == rtfEOF)
      return;
    if (RTFCheckCM(info, rtfGroup, rtfEndGroup))
      break;
    if (RTFCheckCM(info, rtfGroup, rtfBeginGroup))
    {
      RTFGetToken (info);
      if (info->rtfClass == rtfEOF)
        return;
      if (RTFCheckCMM(info, rtfControl, rtfDestination, rtfObjResult))
      {
	int	level = 1;

	while (RTFGetToken (info) != rtfEOF)
	{
          if (info->rtfClass == rtfGroup)
          {
            if (info->rtfMajor == rtfBeginGroup) level++;
            else if (info->rtfMajor == rtfEndGroup && --level < 0) break;
          }
          RTFRouteToken(info);
	}
      }
      else RTFSkipGroup(info);
      continue;
    }
    if (!RTFCheckCM (info, rtfControl, rtfObjAttr))
    {
      FIXME("Non supported attribute: %d %d %d\n", info->rtfClass, info->rtfMajor, info->rtfMinor);
      return;
    }
  }
  RTFRouteToken(info);	/* feed "}" back to router */
}

static void ME_RTFReadHook(RTF_Info *info)
{
  switch(info->rtfClass)
  {
    case rtfGroup:
      switch(info->rtfMajor)
      {
        case rtfBeginGroup:
          if (info->stackTop < maxStack) {
            info->stack[info->stackTop].fmt = info->style->fmt;
            info->stack[info->stackTop].codePage = info->codePage;
            info->stack[info->stackTop].unicodeLength = info->unicodeLength;
          }
          info->stackTop++;
          info->styleChanged = FALSE;
          break;
        case rtfEndGroup:
        {
          ME_Style *s;
          RTFFlushOutputBuffer(info);
          info->stackTop--;
          if (info->stackTop<=0) {
            info->rtfClass = rtfEOF;
            return;
          }
          assert(info->stackTop >= 0);
          if (info->styleChanged)
          {
            /* FIXME too slow ? how come ? */
            s = ME_ApplyStyle(info->style, &info->stack[info->stackTop].fmt);
            ME_ReleaseStyle(info->style);
            info->style = s;
            info->codePage = info->stack[info->stackTop].codePage;
            info->unicodeLength = info->stack[info->stackTop].unicodeLength;
          }
          break;
        }
      }
      break;
  }
}

void
ME_StreamInFill(ME_InStream *stream)
{
  stream->editstream->dwError = stream->editstream->pfnCallback(stream->editstream->dwCookie,
                                                                (BYTE *)stream->buffer,
                                                                sizeof(stream->buffer),
                                                                (LONG *)&stream->dwSize);
  stream->dwUsed = 0;
}

static LRESULT ME_StreamIn(ME_TextEditor *editor, DWORD format, EDITSTREAM *stream, BOOL stripLastCR)
{
  RTF_Info parser;
  ME_Style *style;
  int from, to, to2, nUndoMode;
  int nEventMask = editor->nEventMask;
  ME_InStream inStream;
  BOOL invalidRTF = FALSE;

  TRACE("stream==%p hWnd==%p format==0x%X\n", stream, editor->hWnd, format);
  editor->nEventMask = 0;

  ME_GetSelection(editor, &from, &to);
  if ((format & SFF_SELECTION) && (editor->mode & TM_RICHTEXT)) {
    style = ME_GetSelectionInsertStyle(editor);

    ME_InternalDeleteText(editor, from, to-from, FALSE);

    /* Don't insert text at the end of the table row */
    if (!editor->bEmulateVersion10) { /* v4.1 */
      ME_DisplayItem *para = ME_GetParagraph(editor->pCursors->pRun);
      if (para->member.para.nFlags & MEPF_ROWEND)
      {
        para = para->member.para.next_para;
        editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
        editor->pCursors[0].nOffset = 0;
      }
      if (para->member.para.nFlags & MEPF_ROWSTART)
      {
        para = para->member.para.next_para;
        editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
        editor->pCursors[0].nOffset = 0;
      }
      editor->pCursors[1] = editor->pCursors[0];
    } else { /* v1.0 - 3.0 */
      if (editor->pCursors[0].pRun->member.run.nFlags & MERF_ENDPARA &&
          ME_IsInTable(editor->pCursors[0].pRun))
        return 0;
    }
  }
  else {
    ME_DisplayItem *para_item;
    style = editor->pBuffer->pDefaultStyle;
    ME_AddRefStyle(style);
    SendMessageA(editor->hWnd, EM_SETSEL, 0, 0);    
    ME_InternalDeleteText(editor, 0, ME_GetTextLength(editor), FALSE);
    from = to = 0;
    ME_ClearTempStyle(editor);

    para_item = ME_GetParagraph(editor->pCursors[0].pRun);
    ME_SetDefaultParaFormat(para_item->member.para.pFmt);
  }


  /* Back up undo mode to a local variable */
  nUndoMode = editor->nUndoMode;

  /* Only create an undo if SFF_SELECTION is set */
  if (!(format & SFF_SELECTION))
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
      if ((!editor->bEmulateVersion10 && strncmp(inStream.buffer, "{\\rtf", 5) && strncmp(inStream.buffer, "{\\urtf", 6))
	|| (editor->bEmulateVersion10 && *inStream.buffer != '{'))
      {
        invalidRTF = TRUE;
        inStream.editstream->dwError = -16;
      }
    }
  }

  if (!invalidRTF && !inStream.editstream->dwError)
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
      RTFSetDestinationCallback(&parser, rtfPict, ME_RTFReadPictGroup);
      RTFSetDestinationCallback(&parser, rtfObject, ME_RTFReadObjectGroup);
      if (!parser.editor->bEmulateVersion10) /* v4.1 */
      {
        RTFSetDestinationCallback(&parser, rtfNoNestTables, RTFSkipGroup);
        RTFSetDestinationCallback(&parser, rtfNestTableProps, RTFReadGroup);
      }
      BeginFile(&parser);

      /* do the parsing */
      RTFRead(&parser);
      RTFFlushOutputBuffer(&parser);
      if (!editor->bEmulateVersion10) { /* v4.1 */
        if (parser.tableDef && parser.tableDef->tableRowStart &&
            (parser.nestingLevel > 0 || parser.canInheritInTbl))
        {
          /* Delete any incomplete table row at the end of the rich text. */
          int nOfs, nChars;
          ME_DisplayItem *pCell;
          ME_DisplayItem *para;

          parser.rtfMinor = rtfRow;
          /* Complete the table row before deleting it.
           * By doing it this way we will have the current paragraph format set
           * properly to reflect that is not in the complete table, and undo items
           * will be added for this change to the current paragraph format. */
          if (parser.nestingLevel > 0)
          {
            while (parser.nestingLevel > 1)
              ME_RTFSpecialCharHook(&parser); /* Decrements nestingLevel */
            para = parser.tableDef->tableRowStart;
            ME_RTFSpecialCharHook(&parser);
          } else {
            para = parser.tableDef->tableRowStart;
            ME_RTFSpecialCharHook(&parser);
            assert(para->member.para.nFlags & MEPF_ROWEND);
            para = para->member.para.next_para;
          }
          pCell = para->member.para.pCell;

          editor->pCursors[1].pRun = ME_FindItemFwd(para, diRun);
          editor->pCursors[1].nOffset = 0;
          nOfs = ME_GetCursorOfs(editor, 1);
          nChars = ME_GetCursorOfs(editor, 0) - nOfs;
          ME_InternalDeleteText(editor, nOfs, nChars, TRUE);
          if (parser.tableDef)
            parser.tableDef->tableRowStart = NULL;
        }
      }
      ME_CheckTablesForCorruption(editor);
      RTFDestroy(&parser);
      if (parser.lpRichEditOle)
        IRichEditOle_Release(parser.lpRichEditOle);

      if (!inStream.editstream->dwError && parser.stackTop > 0)
        inStream.editstream->dwError = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

      /* Remove last line break, as mandated by tests. This is not affected by
         CR/LF counters, since RTF streaming presents only \para tokens, which
         are converted according to the standard rules: \r for 2.0, \r\n for 1.0
       */
      if (stripLastCR) {
        int newfrom, newto;
        ME_GetSelection(editor, &newfrom, &newto);
        if (newto > to + (editor->bEmulateVersion10 ? 1 : 0)) {
          WCHAR lastchar[3] = {'\0', '\0'};
          int linebreakSize = editor->bEmulateVersion10 ? 2 : 1;

          ME_GetTextW(editor, lastchar, newto - linebreakSize, linebreakSize, 0);
          if (lastchar[0] == '\r' && (lastchar[1] == '\n' || lastchar[1] == '\0')) {
            ME_InternalDeleteText(editor, newto - linebreakSize, linebreakSize, FALSE);
          }
        }
      }

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
  }

  /* Restore saved undo mode */
  editor->nUndoMode = nUndoMode;

  /* even if we didn't add an undo, we need to commit anything on the stack */
  ME_CommitUndo(editor);

  /* If SFF_SELECTION isn't set, delete any undos from before we started too */
  if (!(format & SFF_SELECTION))
    ME_EmptyUndoStack(editor);

  ME_ReleaseStyle(style);
  editor->nEventMask = nEventMask;
  ME_UpdateRepaint(editor);
  if (!(format & SFF_SELECTION)) {
    ME_ClearTempStyle(editor);
  }
  HideCaret(editor->hWnd);
  ME_MoveCaret(editor);
  ShowCaret(editor->hWnd);
  ME_SendSelChange(editor);
  ME_SendRequestResize(editor, FALSE);

  return 0;
}


typedef struct tagME_RTFStringStreamStruct
{
  char *string;
  int pos;
  int length;
} ME_RTFStringStreamStruct;

static DWORD CALLBACK ME_ReadFromRTFString(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, LONG *pcb)
{
  ME_RTFStringStreamStruct *pStruct = (ME_RTFStringStreamStruct *)dwCookie;
  int count;

  count = min(cb, pStruct->length - pStruct->pos);
  memmove(lpBuff, pStruct->string + pStruct->pos, count);
  pStruct->pos += count;
  *pcb = count;
  return 0;
}

static void
ME_StreamInRTFString(ME_TextEditor *editor, BOOL selection, char *string)
{
  EDITSTREAM es;
  ME_RTFStringStreamStruct data;

  data.string = string;
  data.length = strlen(string);
  data.pos = 0;
  es.dwCookie = (DWORD)&data;
  es.pfnCallback = ME_ReadFromRTFString;
  ME_StreamIn(editor, SF_RTF | (selection ? SFF_SELECTION : 0), &es, FALSE);
}


ME_DisplayItem *
ME_FindItemAtOffset(ME_TextEditor *editor, ME_DIType nItemType, int nOffset, int *nItemOffset)
{
  ME_DisplayItem *item = ME_FindItemFwd(editor->pBuffer->pFirst, diParagraph);
  int runLength;
  
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
    runLength = ME_StrLen(item->member.run.strText);
    if (item->member.run.nFlags & MERF_ENDPARA)
      runLength = item->member.run.nCR + item->member.run.nLF;
  } while (item && (item->member.run.nCharOfs + runLength <= nOffset));
  if (item) {
    nOffset -= item->member.run.nCharOfs;

    /* Special case: nOffset may not point exactly at the division between the
       \r and the \n in 1.0 emulation. If such a case happens, it is sent
       into the next run, if one exists
     */
    if (   item->member.run.nFlags & MERF_ENDPARA
        && nOffset == item->member.run.nCR
        && item->member.run.nLF > 0) {
      ME_DisplayItem *nextItem;
      nextItem = ME_FindItemFwd(item, diRun);
      if (nextItem) {
        nOffset = 0;
        item = nextItem;
      }
    }
    if (nItemOffset)
      *nItemOffset = nOffset;
  }
  return item;
}


static int
ME_FindText(ME_TextEditor *editor, DWORD flags, const CHARRANGE *chrg, const WCHAR *text, CHARRANGE *chrgText)
{
  const int nLen = lstrlenW(text);
  const int nTextLen = ME_GetTextLength(editor);
  int nStart, nEnd;
  int nMin, nMax;
  ME_DisplayItem *item;
  ME_DisplayItem *para;
  WCHAR wLastChar = ' ';

  TRACE("flags==0x%08x, chrg->cpMin==%d, chrg->cpMax==%d text==%s\n",
        flags, chrg->cpMin, chrg->cpMax, debugstr_w(text));
  
  if (flags & ~(FR_DOWN | FR_MATCHCASE | FR_WHOLEWORD))
    FIXME("Flags 0x%08x not implemented\n",
        flags & ~(FR_DOWN | FR_MATCHCASE | FR_WHOLEWORD));

  nMin = chrg->cpMin;
  if (chrg->cpMax == -1)
    nMax = nTextLen;
  else
    nMax = chrg->cpMax > nTextLen ? nTextLen : chrg->cpMax;
  
  /* In 1.0 emulation, if cpMax reaches end of text, add the FR_DOWN flag */
  if (editor->bEmulateVersion10 && nMax == nTextLen)
  {
    flags |= FR_DOWN;
  }

  /* In 1.0 emulation, cpMin must always be no greater than cpMax */
  if (editor->bEmulateVersion10 && nMax < nMin)
  {
    if (chrgText)
    {
      chrgText->cpMin = -1;
      chrgText->cpMax = -1;
    }
    return -1;
  }

  /* when searching up, if cpMin < cpMax, then instead of searching
   * on [cpMin,cpMax], we search on [0,cpMin], otherwise, search on
   * [cpMax, cpMin]. The exception is when cpMax is -1, in which
   * case, it is always bigger than cpMin.
   */
  if (!editor->bEmulateVersion10 && !(flags & FR_DOWN))
  {
    int nSwap = nMax;

    nMax = nMin > nTextLen ? nTextLen : nMin;
    if (nMin < nSwap || chrg->cpMax == -1)
      nMin = 0;
    else
      nMin = nSwap;
  }

  if (!nLen || nMin < 0 || nMax < 0 || nMax < nMin)
  {
    if (chrgText)
      chrgText->cpMin = chrgText->cpMax = -1;
    return -1;
  }
 
  if (flags & FR_DOWN) /* Forward search */
  {
    /* If possible, find the character before where the search starts */
    if ((flags & FR_WHOLEWORD) && nMin)
    {
      nStart = nMin - 1;
      item = ME_FindItemAtOffset(editor, diRun, nStart, &nStart);
      if (!item)
      {
        if (chrgText)
          chrgText->cpMin = chrgText->cpMax = -1;
        return -1;
      }
      wLastChar = item->member.run.strText->szData[nStart];
    }

    nStart = nMin;
    item = ME_FindItemAtOffset(editor, diRun, nStart, &nStart);
    if (!item)
    {
      if (chrgText)
        chrgText->cpMin = chrgText->cpMax = -1;
      return -1;
    }

    para = ME_GetParagraph(item);
    while (item
           && para->member.para.nCharOfs + item->member.run.nCharOfs + nStart + nLen <= nMax)
    {
      ME_DisplayItem *pCurItem = item;
      int nCurStart = nStart;
      int nMatched = 0;
    
      while (pCurItem && ME_CharCompare(pCurItem->member.run.strText->szData[nCurStart + nMatched], text[nMatched], (flags & FR_MATCHCASE)))
      {
        if ((flags & FR_WHOLEWORD) && isalnumW(wLastChar))
          break;

        nMatched++;
        if (nMatched == nLen)
        {
          ME_DisplayItem *pNextItem = pCurItem;
          int nNextStart = nCurStart;
          WCHAR wNextChar;

          /* Check to see if next character is a whitespace */
          if (flags & FR_WHOLEWORD)
          {
            if (nCurStart + nMatched == ME_StrLen(pCurItem->member.run.strText))
            {
              pNextItem = ME_FindItemFwd(pCurItem, diRun);
              nNextStart = -nMatched;
            }

            if (pNextItem)
              wNextChar = pNextItem->member.run.strText->szData[nNextStart + nMatched];
            else
              wNextChar = ' ';

            if (isalnumW(wNextChar))
              break;
          }

          nStart += para->member.para.nCharOfs + item->member.run.nCharOfs;
          if (chrgText)
          {
            chrgText->cpMin = nStart;
            chrgText->cpMax = nStart + nLen;
          }
          TRACE("found at %d-%d\n", nStart, nStart + nLen);
          return nStart;
        }
        if (nCurStart + nMatched == ME_StrLen(pCurItem->member.run.strText))
        {
          pCurItem = ME_FindItemFwd(pCurItem, diRun);
          para = ME_GetParagraph(pCurItem);
          nCurStart = -nMatched;
        }
      }
      if (pCurItem)
        wLastChar = pCurItem->member.run.strText->szData[nCurStart + nMatched];
      else
        wLastChar = ' ';

      nStart++;
      if (nStart == ME_StrLen(item->member.run.strText))
      {
        item = ME_FindItemFwd(item, diRun);
        para = ME_GetParagraph(item);
        nStart = 0;
      }
    }
  }
  else /* Backward search */
  {
    /* If possible, find the character after where the search ends */
    if ((flags & FR_WHOLEWORD) && nMax < nTextLen - 1)
    {
      nEnd = nMax + 1;
      item = ME_FindItemAtOffset(editor, diRun, nEnd, &nEnd);
      if (!item)
      {
        if (chrgText)
          chrgText->cpMin = chrgText->cpMax = -1;
        return -1;
      }
      wLastChar = item->member.run.strText->szData[nEnd];
    }

    nEnd = nMax;
    item = ME_FindItemAtOffset(editor, diRun, nEnd, &nEnd);
    if (!item)
    {
      if (chrgText)
        chrgText->cpMin = chrgText->cpMax = -1;
      return -1;
    }
    
    para = ME_GetParagraph(item);
    
    while (item
           && para->member.para.nCharOfs + item->member.run.nCharOfs + nEnd - nLen >= nMin)
    {
      ME_DisplayItem *pCurItem = item;
      int nCurEnd = nEnd;
      int nMatched = 0;
      
      if (nCurEnd - nMatched == 0)
      {
        pCurItem = ME_FindItemBack(pCurItem, diRun);
        para = ME_GetParagraph(pCurItem);
        nCurEnd = ME_StrLen(pCurItem->member.run.strText) + nMatched;
      }
      
      while (pCurItem && ME_CharCompare(pCurItem->member.run.strText->szData[nCurEnd - nMatched - 1], text[nLen - nMatched - 1], (flags & FR_MATCHCASE)))
      {
        if ((flags & FR_WHOLEWORD) && isalnumW(wLastChar))
          break;

        nMatched++;
        if (nMatched == nLen)
        {
          ME_DisplayItem *pPrevItem = pCurItem;
          int nPrevEnd = nCurEnd;
          WCHAR wPrevChar;

          /* Check to see if previous character is a whitespace */
          if (flags & FR_WHOLEWORD)
          {
            if (nPrevEnd - nMatched == 0)
            {
              pPrevItem = ME_FindItemBack(pCurItem, diRun);
              if (pPrevItem)
                nPrevEnd = ME_StrLen(pPrevItem->member.run.strText) + nMatched;
            }

            if (pPrevItem)
              wPrevChar = pPrevItem->member.run.strText->szData[nPrevEnd - nMatched - 1];
            else
              wPrevChar = ' ';

            if (isalnumW(wPrevChar))
              break;
          }

          nStart = para->member.para.nCharOfs + pCurItem->member.run.nCharOfs + nCurEnd - nMatched;
          if (chrgText)
          {
            chrgText->cpMin = nStart;
            chrgText->cpMax = nStart + nLen;
          }
          TRACE("found at %d-%d\n", nStart, nStart + nLen);
          return nStart;
        }
        if (nCurEnd - nMatched == 0)
        {
          pCurItem = ME_FindItemBack(pCurItem, diRun);
          /* Don't care about pCurItem becoming NULL here; it's already taken
           * care of in the exterior loop condition */
          para = ME_GetParagraph(pCurItem);
          nCurEnd = ME_StrLen(pCurItem->member.run.strText) + nMatched;
        }
      }
      if (pCurItem)
        wLastChar = pCurItem->member.run.strText->szData[nCurEnd - nMatched - 1];
      else
        wLastChar = ' ';

      nEnd--;
      if (nEnd < 0)
      {
        item = ME_FindItemBack(item, diRun);
        para = ME_GetParagraph(item);
        nEnd = ME_StrLen(item->member.run.strText);
      }
    }
  }
  TRACE("not found\n");
  if (chrgText)
    chrgText->cpMin = chrgText->cpMax = -1;
  return -1;
}

/* helper to send a msg filter notification */
static BOOL
ME_FilterEvent(ME_TextEditor *editor, UINT msg, WPARAM* wParam, LPARAM* lParam)
{
    MSGFILTER msgf;

    msgf.nmhdr.hwndFrom = editor->hWnd;
    msgf.nmhdr.idFrom = GetWindowLongW(editor->hWnd, GWLP_ID);
    msgf.nmhdr.code = EN_MSGFILTER;
    msgf.msg = msg;

    msgf.wParam = *wParam;
    msgf.lParam = *lParam;
    if (SendMessageW(GetParent(editor->hWnd), WM_NOTIFY, msgf.nmhdr.idFrom, (LPARAM)&msgf))
        return FALSE;
    *wParam = msgf.wParam;
    *lParam = msgf.lParam;
    msgf.wParam = *wParam;

    return TRUE;
}

static BOOL
ME_KeyDown(ME_TextEditor *editor, WORD nKey)
{
  BOOL ctrl_is_down = GetKeyState(VK_CONTROL) & 0x8000;
  BOOL shift_is_down = GetKeyState(VK_SHIFT) & 0x8000;

  if (nKey != VK_SHIFT && nKey != VK_CONTROL && nKey != VK_MENU)
      editor->nSelectionType = stPosition;

  switch (nKey)
  {
    case VK_LEFT:
    case VK_RIGHT:
    case VK_HOME:
    case VK_END:
        editor->nUDArrowX = -1;
        /* fall through */
    case VK_UP:
    case VK_DOWN:
    case VK_PRIOR:
    case VK_NEXT:
      ME_CommitUndo(editor); /* End coalesced undos for typed characters */
      ME_ArrowKey(editor, nKey, shift_is_down, ctrl_is_down);
      return TRUE;
    case VK_BACK:
    case VK_DELETE:
      editor->nUDArrowX = -1;
      /* FIXME backspace and delete aren't the same, they act different wrt paragraph style of the merged paragraph */
      if (GetWindowLongW(editor->hWnd, GWL_STYLE) & ES_READONLY)
        return FALSE;
      if (ME_IsSelection(editor))
      {
        ME_DeleteSelection(editor);
        ME_CommitUndo(editor);
      }
      else if (nKey == VK_DELETE)
      {
        /* Delete stops group typing.
         * (See MSDN remarks on EM_STOPGROUPTYPING message) */
        ME_DeleteTextAtCursor(editor, 1, 1);
        ME_CommitUndo(editor);
      }
      else if (ME_ArrowKey(editor, VK_LEFT, FALSE, FALSE))
      {
        BOOL bDeletionSucceeded;
        /* Backspace can be grouped for a single undo */
        ME_ContinueCoalescingTransaction(editor);
        bDeletionSucceeded = ME_DeleteTextAtCursor(editor, 1, 1);
        if (!bDeletionSucceeded && !editor->bEmulateVersion10) { /* v4.1 */
          /* Deletion was prevented so the cursor is moved back to where it was.
           * (e.g. this happens when trying to delete cell boundaries)
           */
          ME_ArrowKey(editor, VK_RIGHT, FALSE, FALSE);
        }
        ME_CommitCoalescingUndo(editor);
      }
      else
        return TRUE;
      ME_MoveCursorFromTableRowStartParagraph(editor);
      ME_UpdateSelectionLinkAttribute(editor);
      ME_UpdateRepaint(editor);
      ME_SendRequestResize(editor, FALSE);
      return TRUE;

    default:
      if (nKey != VK_SHIFT && nKey != VK_CONTROL && nKey && nKey != VK_MENU)
          editor->nUDArrowX = -1;
      if (ctrl_is_down)
      {
        if (nKey == 'W')
        {
          CHARFORMAT2W chf;
          char buf[2048];
          chf.cbSize = sizeof(chf);
          
          ME_GetSelectionCharFormat(editor, &chf);
          ME_DumpStyleToBuf(&chf, buf);
          MessageBoxA(NULL, buf, "Style dump", MB_OK);
        }
        if (nKey == 'Q')
        {
          ME_CheckCharOffsets(editor);
        }
      }
  }
  return FALSE;
}

/* Process the message and calculate the new click count.
 *
 * returns: The click count if it is mouse down event, else returns 0. */
static int ME_CalculateClickCount(HWND hWnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam)
{
    static int clickNum = 0;
    if (msg < WM_MOUSEFIRST || msg > WM_MOUSELAST)
        return 0;

    if ((msg == WM_LBUTTONDBLCLK) ||
        (msg == WM_RBUTTONDBLCLK) ||
        (msg == WM_MBUTTONDBLCLK) ||
        (msg == WM_XBUTTONDBLCLK))
    {
        msg -= (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
    }

    if ((msg == WM_LBUTTONDOWN) ||
        (msg == WM_RBUTTONDOWN) ||
        (msg == WM_MBUTTONDOWN) ||
        (msg == WM_XBUTTONDOWN))
    {
        static MSG prevClickMsg;
        MSG clickMsg;
        clickMsg.hwnd = hWnd;
        clickMsg.message = msg;
        clickMsg.wParam = wParam;
        clickMsg.lParam = lParam;
        clickMsg.time = GetMessageTime();
        clickMsg.pt.x = (short)LOWORD(lParam);
        clickMsg.pt.y = (short)HIWORD(lParam);
        if ((clickNum != 0) &&
            (clickMsg.message == prevClickMsg.message) &&
            (clickMsg.hwnd == prevClickMsg.hwnd) &&
            (clickMsg.wParam == prevClickMsg.wParam) &&
            (clickMsg.time - prevClickMsg.time < GetDoubleClickTime()) &&
            (abs(clickMsg.pt.x - prevClickMsg.pt.x) < GetSystemMetrics(SM_CXDOUBLECLK)/2) &&
            (abs(clickMsg.pt.y - prevClickMsg.pt.y) < GetSystemMetrics(SM_CYDOUBLECLK)/2))
        {
            clickNum++;
        } else {
            clickNum = 1;
        }
        prevClickMsg = clickMsg;
    } else {
        return 0;
    }
    return clickNum;
}

static BOOL ME_SetCursor(ME_TextEditor *editor)
{
  POINT pt;
  BOOL isExact;
  int offset;
  DWORD messagePos = GetMessagePos();
  pt.x = (short)LOWORD(messagePos);
  pt.y = (short)HIWORD(messagePos);
  ScreenToClient(editor->hWnd, &pt);
  if ((GetWindowLongW(editor->hWnd, GWL_STYLE) & ES_SELECTIONBAR) &&
      (pt.x < editor->selofs ||
       (editor->nSelectionType == stLine && GetCapture() == editor->hWnd)))
  {
      SetCursor(hLeft);
      return TRUE;
  }
  offset = ME_CharFromPos(editor, pt.x, pt.y, &isExact);
  if (isExact)
  {
      if (editor->AutoURLDetect_bEnable)
      {
          ME_Cursor cursor;
          ME_Run *run;
          ME_CursorFromCharOfs(editor, offset, &cursor);
          run = &cursor.pRun->member.run;
          if (editor->AutoURLDetect_bEnable &&
              run->style->fmt.dwMask & CFM_LINK &&
              run->style->fmt.dwEffects & CFE_LINK)
          {
              SetCursor(LoadCursorW(NULL, (WCHAR*)IDC_HAND));
              return TRUE;
          }
      }
      if (ME_IsSelection(editor))
      {
          int selStart, selEnd;
          ME_GetSelection(editor, &selStart, &selEnd);
          if (selStart <= offset && selEnd >= offset) {
              SetCursor(LoadCursorW(NULL, (WCHAR*)IDC_ARROW));
              return TRUE;
          }
      }
  }
  SetCursor(LoadCursorW(NULL, (WCHAR*)IDC_IBEAM));
  return TRUE;
}

static BOOL ME_ShowContextMenu(ME_TextEditor *editor, int x, int y)
{
  CHARRANGE selrange;
  HMENU menu;
  int seltype = 0;
  if(!editor->lpOleCallback)
    return FALSE;
  ME_GetSelection(editor, &selrange.cpMin, &selrange.cpMax);
  if(selrange.cpMin == selrange.cpMax)
    seltype |= SEL_EMPTY;
  else
  {
    /* FIXME: Handle objects */
    seltype |= SEL_TEXT;
    if(selrange.cpMax-selrange.cpMin > 1)
      seltype |= SEL_MULTICHAR;
  }
  if(SUCCEEDED(IRichEditOleCallback_GetContextMenu(editor->lpOleCallback, seltype, NULL, &selrange, &menu)))
  {
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, GetParent(editor->hWnd), NULL);
    DestroyMenu(menu);
  }
  return TRUE;
}

ME_TextEditor *ME_MakeEditor(HWND hWnd) {
  ME_TextEditor *ed = ALLOC_OBJ(ME_TextEditor);
  int i;
  ed->hWnd = hWnd;
  ed->bEmulateVersion10 = FALSE;
  ed->pBuffer = ME_MakeText();
  ed->nZoomNumerator = ed->nZoomDenominator = 0;
  ME_MakeFirstParagraph(ed);
  /* The four cursors are for:
   * 0 - The position where the caret is shown
   * 1 - The anchored end of the selection (for normal selection)
   * 2 & 3 - The anchored start and end respectively for word, line,
   * or paragraph selection.
   */
  ed->nCursors = 4;
  ed->pCursors = ALLOC_N_OBJ(ME_Cursor, ed->nCursors);
  ed->pCursors[0].pRun = ME_FindItemFwd(ed->pBuffer->pFirst, diRun);
  ed->pCursors[0].nOffset = 0;
  ed->pCursors[1].pRun = ME_FindItemFwd(ed->pBuffer->pFirst, diRun);
  ed->pCursors[1].nOffset = 0;
  ed->pCursors[2] = ed->pCursors[0];
  ed->pCursors[3] = ed->pCursors[1];
  ed->nLastTotalLength = ed->nTotalLength = 0;
  ed->nHeight = 0;
  ed->nUDArrowX = -1;
  ed->nSequence = 0;
  ed->rgbBackColor = -1;
  ed->hbrBackground = GetSysColorBrush(COLOR_WINDOW);
  ed->bCaretAtEnd = FALSE;
  ed->nEventMask = 0;
  ed->nModifyStep = 0;
  ed->nTextLimit = TEXT_LIMIT_DEFAULT;
  ed->pUndoStack = ed->pRedoStack = ed->pUndoStackBottom = NULL;
  ed->nUndoStackSize = 0;
  ed->nUndoLimit = STACK_SIZE_DEFAULT;
  ed->nUndoMode = umAddToUndo;
  ed->nParagraphs = 1;
  ed->nLastSelStart = ed->nLastSelEnd = 0;
  ed->pLastSelStartPara = ed->pLastSelEndPara = ME_FindItemFwd(ed->pBuffer->pFirst, diParagraph);
  ed->bRedraw = TRUE;
  ed->bWordWrap = (GetWindowLongW(hWnd, GWL_STYLE) & WS_HSCROLL) ? FALSE : TRUE;
  ed->bHideSelection = FALSE;
  ed->nInvalidOfs = -1;
  ed->pfnWordBreak = NULL;
  ed->lpOleCallback = NULL;
  ed->mode = TM_RICHTEXT | TM_MULTILEVELUNDO | TM_MULTICODEPAGE;
  ed->AutoURLDetect_bEnable = FALSE;
  ed->bHaveFocus = FALSE;
  for (i=0; i<HFONT_CACHE_SIZE; i++)
  {
    ed->pFontCache[i].nRefs = 0;
    ed->pFontCache[i].nAge = 0;
    ed->pFontCache[i].hFont = NULL;
  }
  
  ME_CheckCharOffsets(ed);
  if (GetWindowLongW(hWnd, GWL_STYLE) & ES_SELECTIONBAR)
    ed->selofs = SELECTIONBAR_WIDTH;
  else
    ed->selofs = 0;
  ed->nSelectionType = stPosition;

  if (GetWindowLongW(hWnd, GWL_STYLE) & ES_PASSWORD)
    ed->cPasswordMask = '*';
  else
    ed->cPasswordMask = 0;
  
  ed->notified_cr.cpMin = ed->notified_cr.cpMax = 0;

  /* Default vertical scrollbar information */
  ed->vert_si.cbSize = sizeof(SCROLLINFO);
  ed->vert_si.nMin = 0;
  ed->vert_si.nMax = 0;
  ed->vert_si.nPage = 0;
  ed->vert_si.nPos = 0;

  OleInitialize(NULL);

  return ed;
}

typedef struct tagME_GlobalDestStruct
{
  HGLOBAL hData;
  int nLength;
} ME_GlobalDestStruct;

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
  if (editor->rgbBackColor != -1)
    DeleteObject(editor->hbrBackground);
  if(editor->lpOleCallback)
    IUnknown_Release(editor->lpOleCallback);
  OleUninitialize();

  FREE_OBJ(editor->pBuffer);
  FREE_OBJ(editor->pCursors);

  FREE_OBJ(editor);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("\n");
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hinstDLL);
      me_heap = HeapCreate (0, 0x10000, 0);
      if (!ME_RegisterEditorClass(hinstDLL)) return FALSE;
      hLeft = LoadCursorW(hinstDLL, MAKEINTRESOURCEW(OCR_REVERSE));
      LookupInit();
      break;

    case DLL_PROCESS_DETACH:
      UnregisterClassW(RichEdit20W, 0);
      UnregisterClassW(RichEdit50W, 0);
      UnregisterClassA("RichEdit20A", 0);
      UnregisterClassA("RichEdit50A", 0);
      if (ME_ListBoxRegistered)
          UnregisterClassW(REListBox20W, 0);
      if (ME_ComboBoxRegistered)
          UnregisterClassW(REComboBox20W, 0);
      LookupCleanup();
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
  if (msg >= EM_GETSEL && msg <= EM_CHARFROMPOS)
    return edit_messages[msg - EM_GETSEL];
  if (msg >= EM_CANPASTE && msg <= EM_GETIMEMODEBIAS)
    return richedit_messages[msg - EM_CANPASTE];
  return "";
}

static LRESULT RichEditWndProc_common(HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam, BOOL unicode)
{
  ME_TextEditor *editor = (ME_TextEditor *)GetWindowLongPtrW(hWnd, 0);
  
  TRACE("hwnd %p msg %04x (%s) %lx %lx, unicode %d\n",
        hWnd, msg, get_msg_name(msg), wParam, lParam, unicode);
  
  if (!editor && msg != WM_NCCREATE && msg != WM_NCDESTROY) {
    ERR("called with invalid hWnd %p - application bug?\n", hWnd);
    return 0; 
  }

  switch(msg) {
  
  UNSUPPORTED_MSG(EM_DISPLAYBAND)
  UNSUPPORTED_MSG(EM_FINDWORDBREAK)
  UNSUPPORTED_MSG(EM_FMTLINES)
  UNSUPPORTED_MSG(EM_FORMATRANGE)
  UNSUPPORTED_MSG(EM_GETBIDIOPTIONS)
  UNSUPPORTED_MSG(EM_GETEDITSTYLE)
  UNSUPPORTED_MSG(EM_GETIMECOMPMODE)
  /* UNSUPPORTED_MSG(EM_GETIMESTATUS) missing in Wine headers */
  UNSUPPORTED_MSG(EM_GETLANGOPTIONS)
  /* UNSUPPORTED_MSG(EM_GETOLEINTERFACE) separate stub */
  UNSUPPORTED_MSG(EM_GETREDONAME)
  UNSUPPORTED_MSG(EM_GETTEXTMODE)
  UNSUPPORTED_MSG(EM_GETTYPOGRAPHYOPTIONS)
  UNSUPPORTED_MSG(EM_GETUNDONAME)
  UNSUPPORTED_MSG(EM_GETWORDBREAKPROCEX)
  UNSUPPORTED_MSG(EM_PASTESPECIAL)
  UNSUPPORTED_MSG(EM_SELECTIONTYPE)
  UNSUPPORTED_MSG(EM_SETBIDIOPTIONS)
  UNSUPPORTED_MSG(EM_SETEDITSTYLE)
  UNSUPPORTED_MSG(EM_SETFONTSIZE)
  UNSUPPORTED_MSG(EM_SETLANGOPTIONS)
  UNSUPPORTED_MSG(EM_SETPALETTE)
  UNSUPPORTED_MSG(EM_SETTABSTOPS)
  UNSUPPORTED_MSG(EM_SETTYPOGRAPHYOPTIONS)
  UNSUPPORTED_MSG(EM_SETWORDBREAKPROCEX)
  UNSUPPORTED_MSG(WM_STYLECHANGING)
  UNSUPPORTED_MSG(WM_STYLECHANGED)

/* Messages specific to Richedit controls */

  case EM_STREAMIN:
   return ME_StreamIn(editor, wParam, (EDITSTREAM*)lParam, TRUE);
  case EM_STREAMOUT:
   return ME_StreamOut(editor, wParam, (EDITSTREAM *)lParam);
  case WM_GETDLGCODE:
  {
    UINT code = DLGC_WANTCHARS|DLGC_WANTARROWS;
    if(lParam && (((LPMSG)lParam)->message == WM_KEYDOWN))
    {
      int vk = (int)((LPMSG)lParam)->wParam;
      /* if style says we want return key */
      if((vk == VK_RETURN) && (GetWindowLongW(hWnd, GWL_STYLE) & ES_WANTRETURN))
      {
        code |= DLGC_WANTMESSAGE;
      }
      /* we always handle ctrl-tab */
      if((vk == VK_TAB) && (GetKeyState(VK_CONTROL) & 0x8000))
      {
        code |= DLGC_WANTMESSAGE;
      }
    }
    return code;
  }
  case WM_NCCREATE:
  {
    CREATESTRUCTW *pcs = (CREATESTRUCTW *)lParam;
    TRACE("WM_NCCREATE: style 0x%08x\n", pcs->style);
    editor = ME_MakeEditor(hWnd);
    SetWindowLongPtrW(hWnd, 0, (LONG_PTR)editor);
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
    ME_GetSelection(editor, (int *)pfrom, (int *)pto);
    if ((*pfrom|*pto) & 0xFFFF0000)
      return -1;
    return MAKELONG(*pfrom,*pto);
  }
  case EM_EXGETSEL:
  {
    CHARRANGE *pRange = (CHARRANGE *)lParam;
    ME_GetSelection(editor, &pRange->cpMin, &pRange->cpMax);
    TRACE("EM_EXGETSEL = (%d,%d)\n", pRange->cpMin, pRange->cpMax);
    return 0;
  }
  case EM_SETUNDOLIMIT:
  {
    if ((int)wParam < 0)
      editor->nUndoLimit = STACK_SIZE_DEFAULT;
    else
      editor->nUndoLimit = min(wParam, STACK_SIZE_MAX);
    /* Setting a max stack size keeps wine from getting killed 
      for hogging memory. Windows allocates all this memory at once, so
      no program would realistically set a value above our maximum. */
    return editor->nUndoLimit;
  }
  case EM_CANUNDO:
    return editor->pUndoStack != NULL;
  case EM_CANREDO:
    return editor->pRedoStack != NULL;
  case WM_UNDO: /* FIXME: actually not the same */
  case EM_UNDO:
    return ME_Undo(editor);
  case EM_REDO:
    return ME_Redo(editor);
  case EM_GETOPTIONS:
  {
    /* these flags are equivalent to the ES_* counterparts */
    DWORD mask = ECO_VERTICAL | ECO_AUTOHSCROLL | ECO_AUTOVSCROLL |
                 ECO_NOHIDESEL | ECO_READONLY | ECO_WANTRETURN;
    DWORD settings = GetWindowLongW(hWnd, GWL_STYLE) & mask;

    return settings;
  }
  case EM_SETOPTIONS:
  {
    /* these flags are equivalent to ES_* counterparts                      
     * ECO_READONLY is already implemented in the code, only requires 
     * setting the bit to work                                        
     */
    DWORD mask = ECO_VERTICAL | ECO_AUTOHSCROLL | ECO_AUTOVSCROLL |
                 ECO_NOHIDESEL | ECO_READONLY | ECO_WANTRETURN | ECO_SELECTIONBAR;
    DWORD raw = GetWindowLongW(hWnd, GWL_STYLE);
    DWORD settings = mask & raw;

    switch(wParam)
    {
      case ECOOP_SET:
        settings = lParam;
        break;
      case ECOOP_OR:
        settings |= lParam;
        break;
      case ECOOP_AND:
        settings &= lParam;
        break;
      case ECOOP_XOR:
        settings ^= lParam;
    }
    SetWindowLongW(hWnd, GWL_STYLE, (raw & ~mask) | (settings & mask));

    if (settings & ECO_AUTOWORDSELECTION)
      FIXME("ECO_AUTOWORDSELECTION not implemented yet!\n");
    if (settings & ECO_SELECTIONBAR)
        editor->selofs = SELECTIONBAR_WIDTH;
    else
        editor->selofs = 0;
    ME_WrapMarkedParagraphs(editor);

    if (settings & ECO_VERTICAL)
      FIXME("ECO_VERTICAL not implemented yet!\n");
    if (settings & ECO_AUTOHSCROLL)
      FIXME("ECO_AUTOHSCROLL not implemented yet!\n");
    if (settings & ECO_AUTOVSCROLL)
      FIXME("ECO_AUTOVSCROLL not implemented yet!\n");
    if (settings & ECO_NOHIDESEL)
      FIXME("ECO_NOHIDESEL not implemented yet!\n");
    if (settings & ECO_WANTRETURN)
      FIXME("ECO_WANTRETURN not implemented yet!\n");

    return settings;
  }
  case EM_SETSEL:
  {
    ME_InvalidateSelection(editor);
    ME_SetSelection(editor, wParam, lParam);
    ME_InvalidateSelection(editor);
    HideCaret(editor->hWnd);
    ME_ShowCaret(editor);
    ME_SendSelChange(editor);
    return 0;
  }
  case EM_SETSCROLLPOS:
  {
    POINT *point = (POINT *)lParam;
    ME_ScrollAbs(editor, point->y);
    return 0;
  }
  case EM_AUTOURLDETECT:
  {
    if (wParam==1 || wParam ==0) 
    {
        editor->AutoURLDetect_bEnable = (BOOL)wParam;
        return 0;
    }
    return E_INVALIDARG;
  }
  case EM_GETAUTOURLDETECT:
  {
    return editor->AutoURLDetect_bEnable;
  }
  case EM_EXSETSEL:
  {
    int end;
    CHARRANGE range = *(CHARRANGE *)lParam;

    TRACE("EM_EXSETSEL (%d,%d)\n", range.cpMin, range.cpMax);

    ME_InvalidateSelection(editor);
    end = ME_SetSelection(editor, range.cpMin, range.cpMax);
    ME_InvalidateSelection(editor);
    HideCaret(editor->hWnd);
    ME_ShowCaret(editor);
    ME_SendSelChange(editor);

    return end;
  }
  case EM_SHOWSCROLLBAR:
  {
    ShowScrollBar(editor->hWnd, wParam, lParam);
    return 0;
  }
  case EM_SETTEXTEX:
  {
    LPWSTR wszText;
    SETTEXTEX *pStruct = (SETTEXTEX *)wParam;
    size_t len;
    int from, to;
    ME_Style *style;
    int oldModify = editor->nModifyStep;

    if (!pStruct) return 0;

    TRACE("EM_SETTEXTEX - %s, flags %d, cp %d\n",
          pStruct->codepage == 1200 ? debugstr_w((LPCWSTR)lParam) : debugstr_a((LPCSTR)lParam),
          pStruct->flags, pStruct->codepage);

    /* FIXME: make use of pStruct->codepage in the to unicode translation */
    wszText = lParam ? ME_ToUnicode(pStruct->codepage == 1200, (void *)lParam) : NULL;
    len = wszText ? lstrlenW(wszText) : 0;

    if (pStruct->flags & ST_SELECTION) {
      ME_GetSelection(editor, &from, &to);
      style = ME_GetSelectionInsertStyle(editor);
      ME_InternalDeleteText(editor, from, to - from, FALSE);
      if (pStruct->codepage != 1200 && lParam && !strncmp((char *)lParam, "{\\rtf", 5))
          ME_StreamInRTFString(editor, 1, (char *)lParam);
      else ME_InsertTextFromCursor(editor, 0, wszText, len, style);
      ME_ReleaseStyle(style);

      if (editor->AutoURLDetect_bEnable) ME_UpdateSelectionLinkAttribute(editor);
    }
    else {
      ME_InternalDeleteText(editor, 0, ME_GetTextLength(editor), FALSE);
      if (pStruct->codepage != 1200 && lParam && !strncmp((char *)lParam, "{\\rtf", 5))
          ME_StreamInRTFString(editor, 0, (char *)lParam);
      else ME_InsertTextFromCursor(editor, 0, wszText, len, editor->pBuffer->pDefaultStyle);
      len = 1;

      if (editor->AutoURLDetect_bEnable) ME_UpdateLinkAttribute(editor, 0, -1);
    }
    ME_CommitUndo(editor);
    if (!(pStruct->flags & ST_KEEPUNDO))
    {
      editor->nModifyStep = oldModify;
      ME_EmptyUndoStack(editor);
    }
    ME_UpdateRepaint(editor);
    return len;
  }
  case EM_SETBKGNDCOLOR:
  {
    LRESULT lColor;
    if (editor->rgbBackColor != -1) {
      DeleteObject(editor->hbrBackground);
      lColor = editor->rgbBackColor;
    }
    else lColor = GetSysColor(COLOR_WINDOW);

    if (wParam)
    {
      editor->rgbBackColor = -1;
      editor->hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    }
    else
    {
      editor->rgbBackColor = lParam;
      editor->hbrBackground = CreateSolidBrush(editor->rgbBackColor);
    }
    if (editor->bRedraw)
    {
      InvalidateRect(hWnd, NULL, TRUE);
      UpdateWindow(hWnd);
    }
    return lColor;
  }
  case EM_GETMODIFY:
    return editor->nModifyStep == 0 ? 0 : -1;
  case EM_SETMODIFY:
  {
    if (wParam)
      editor->nModifyStep = 1;
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
    return 0;
  }
  case EM_SETEVENTMASK:
  {
    DWORD nOldMask = editor->nEventMask;
    
    editor->nEventMask = lParam;
    return nOldMask;
  }
  case EM_GETEVENTMASK:
    return editor->nEventMask;
  case EM_SETCHARFORMAT:
  {
    CHARFORMAT2W buf, *p;
    BOOL bRepaint = TRUE;
    p = ME_ToCF2W(&buf, (CHARFORMAT2W *)lParam);
    if (p == NULL) return 0;
    if (!wParam)
      ME_SetDefaultCharFormat(editor, p);
    else if (wParam == (SCF_WORD | SCF_SELECTION)) {
      FIXME("EM_SETCHARFORMAT: word selection not supported\n");
      return 0;
    } else if (wParam == SCF_ALL) {
      if (editor->mode & TM_PLAINTEXT)
        ME_SetDefaultCharFormat(editor, p);
      else {
        ME_SetCharFormat(editor, 0, ME_GetTextLength(editor), p);
        editor->nModifyStep = 1;
      }
    } else if (editor->mode & TM_PLAINTEXT) {
      return 0;
    } else {
      int from, to;
      ME_GetSelection(editor, &from, &to);
      bRepaint = (from != to);
      ME_SetSelectionCharFormat(editor, p);
      if (from != to) editor->nModifyStep = 1;
    }
    ME_CommitUndo(editor);
    if (bRepaint)
      ME_RewrapRepaint(editor);
    return 1;
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
  {
    BOOL result = ME_SetSelectionParaFormat(editor, (PARAFORMAT2 *)lParam);
    ME_RewrapRepaint(editor);
    ME_CommitUndo(editor);
    return result;
  }
  case EM_GETPARAFORMAT:
    ME_GetSelectionParaFormat(editor, (PARAFORMAT2 *)lParam);
    return ((PARAFORMAT2 *)lParam)->dwMask;
  case EM_GETFIRSTVISIBLELINE:
  {
    ME_DisplayItem *p = editor->pBuffer->pFirst;
    int y = ME_GetYScrollPos(editor);
    int ypara = 0;
    int count = 0;
    int ystart, yend;
    while(p) {
      p = ME_FindItemFwd(p, diStartRowOrParagraphOrEnd);
      if (p->type == diTextEnd)
        break;
      if (p->type == diParagraph) {
        ypara = p->member.para.pt.y;
        continue;
      }
      ystart = ypara + p->member.row.pt.y;
      yend = ystart + p->member.row.nHeight;
      if (y < yend) {
        break;
      }
      count++;
    }
    return count;
  }
  case EM_HIDESELECTION:
  {
     editor->bHideSelection = (wParam != 0);
     ME_InvalidateSelection(editor);
     return 0;
  }
  case EM_LINESCROLL:
  {
    ME_ScrollDown(editor, lParam * 8); /* FIXME follow the original */
    return TRUE; /* Should return false if a single line richedit control */
  }
  case WM_CLEAR:
  {
    int from, to;
    ME_GetSelection(editor, &from, &to);
    ME_InternalDeleteText(editor, from, to-from, FALSE);
    ME_CommitUndo(editor);
    ME_UpdateRepaint(editor);
    return 0;
  }
  case EM_REPLACESEL:
  {
    int from, to;
    ME_Style *style;
    LPWSTR wszText = lParam ? ME_ToUnicode(unicode, (void *)lParam) : NULL;
    size_t len = wszText ? lstrlenW(wszText) : 0;
    TRACE("EM_REPLACESEL - %s\n", debugstr_w(wszText));

    ME_GetSelection(editor, &from, &to);
    style = ME_GetSelectionInsertStyle(editor);
    ME_InternalDeleteText(editor, from, to-from, FALSE);
    ME_InsertTextFromCursor(editor, 0, wszText, len, style);
    ME_ReleaseStyle(style);
    /* drop temporary style if line end */
    /*
     * FIXME question: does abc\n mean: put abc,
     * clear temp style, put \n? (would require a change)
     */
    if (len>0 && wszText[len-1] == '\n')
      ME_ClearTempStyle(editor);
    ME_EndToUnicode(unicode, wszText);
    ME_CommitUndo(editor);
    if (editor->AutoURLDetect_bEnable) ME_UpdateSelectionLinkAttribute(editor);
    if (!wParam)
      ME_EmptyUndoStack(editor);
    ME_UpdateRepaint(editor);
    return len;
  }
  case EM_SCROLLCARET:
  {
    int top, bottom; /* row's edges relative to document top */
    int nPos;
    ME_DisplayItem *para, *row;
    
    nPos = ME_GetYScrollPos(editor);
    row = ME_RowStart(editor->pCursors[0].pRun);
    para = ME_GetParagraph(row);
    top = para->member.para.pt.y + row->member.row.pt.y;
    bottom = top + row->member.row.nHeight;
    
    if (top < nPos) /* caret above window */
      ME_ScrollAbs(editor,  top);
    else if (nPos + editor->sizeWindow.cy < bottom) /*below*/
      ME_ScrollAbs(editor, bottom - editor->sizeWindow.cy);
    return 0;
  }
  case WM_SETFONT:
  {
    LOGFONTW lf;
    CHARFORMAT2W fmt;
    HDC hDC;
    BOOL bRepaint = LOWORD(lParam);
    
    if (!wParam)
      wParam = (WPARAM)GetStockObject(SYSTEM_FONT); 
    GetObjectW((HGDIOBJ)wParam, sizeof(LOGFONTW), &lf);
    hDC = GetDC(hWnd);
    ME_CharFormatFromLogFont(hDC, &lf, &fmt); 
    ReleaseDC(hWnd, hDC);   
    ME_SetCharFormat(editor, 0, ME_GetTextLength(editor), &fmt);
    ME_SetDefaultCharFormat(editor, &fmt);

    ME_CommitUndo(editor);
    if (bRepaint)
      ME_RewrapRepaint(editor);
    return 0;
  }
  case WM_SETTEXT:
  {
    ME_InternalDeleteText(editor, 0, ME_GetTextLength(editor), FALSE);
    if (lParam)
    {
      TRACE("WM_SETTEXT lParam==%lx\n",lParam);
      if (!unicode && !strncmp((char *)lParam, "{\\rtf", 5))
      {
        /* Undocumented: WM_SETTEXT supports RTF text */
        ME_StreamInRTFString(editor, 0, (char *)lParam);
      }
      else
      {
        LPWSTR wszText = ME_ToUnicode(unicode, (void *)lParam);
        TRACE("WM_SETTEXT - %s\n", debugstr_w(wszText)); /* debugstr_w() */
        if (lstrlenW(wszText) > 0)
        {
          int len = -1;

          /* uses default style! */
          if (!(GetWindowLongW(hWnd, GWL_STYLE) & ES_MULTILINE))
          {
            WCHAR * p;

            p = wszText;
            while (*p != '\0' && *p != '\r' && *p != '\n') p++;
            len = p - wszText;
          }
          ME_InsertTextFromCursor(editor, 0, wszText, len, editor->pBuffer->pDefaultStyle);
        }
        ME_EndToUnicode(unicode, wszText);
      }
    }
    else
      TRACE("WM_SETTEXT - NULL\n");
    if (editor->AutoURLDetect_bEnable)
    {
      ME_UpdateLinkAttribute(editor, 0, -1);
    }
    ME_SetSelection(editor, 0, 0);
    editor->nModifyStep = 0;
    ME_CommitUndo(editor);
    ME_EmptyUndoStack(editor);
    ME_UpdateRepaint(editor);
    return 1;
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
    ME_StreamIn(editor, dwFormat|SFF_SELECTION, &es, FALSE);

    CloseClipboard();
    return 0;
  }
  case WM_CUT:
  case WM_COPY:
  {
    LPDATAOBJECT dataObj = NULL;
    CHARRANGE range;
    HRESULT hr = S_OK;

    if (editor->cPasswordMask)
      return 0; /* Copying or Cutting masked text isn't allowed */

    ME_GetSelection(editor, &range.cpMin, &range.cpMax);
    if(editor->lpOleCallback)
        hr = IRichEditOleCallback_GetClipboardData(editor->lpOleCallback, &range, RECO_COPY, &dataObj);
    if(FAILED(hr) || !dataObj)
        hr = ME_GetDataObject(editor, &range, &dataObj);
    if(SUCCEEDED(hr)) {
        hr = OleSetClipboard(dataObj);
        IDataObject_Release(dataObj);
    }
    if (SUCCEEDED(hr) && msg == WM_CUT)
    {
      ME_InternalDeleteText(editor, range.cpMin, range.cpMax-range.cpMin, FALSE);
      ME_CommitUndo(editor);
      ME_UpdateRepaint(editor);
    }
    return 0;
  }
  case WM_GETTEXTLENGTH:
  {
    GETTEXTLENGTHEX how;

    /* CR/LF conversion required in 2.0 mode, verbatim in 1.0 mode */
    how.flags = GTL_CLOSE | (editor->bEmulateVersion10 ? 0 : GTL_USECRLF) | GTL_NUMCHARS;
    how.codepage = unicode ? 1200 : CP_ACP;
    return ME_GetTextLengthEx(editor, &how);
  }
  case EM_GETTEXTLENGTHEX:
    return ME_GetTextLengthEx(editor, (GETTEXTLENGTHEX *)wParam);
  case WM_GETTEXT:
  {
    GETTEXTEX ex;
    LRESULT rc;
    LPSTR bufferA = NULL;
    LPWSTR bufferW = NULL;

    if (unicode)
        bufferW = heap_alloc((wParam + 2) * sizeof(WCHAR));
    else
        bufferA = heap_alloc(wParam + 2);

    ex.cb = (wParam + 2) * (unicode ? sizeof(WCHAR) : sizeof(CHAR));
    ex.flags = GT_USECRLF;
    ex.codepage = unicode ? 1200 : CP_ACP;
    ex.lpDefaultChar = NULL;
    ex.lpUsedDefChar = NULL;
    rc = RichEditWndProc_common(hWnd, EM_GETTEXTEX, (WPARAM)&ex, unicode ? (LPARAM)bufferW : (LPARAM)bufferA, unicode);

    if (unicode)
    {
        memcpy((LPWSTR)lParam, bufferW, wParam * sizeof(WCHAR));
        if (lstrlenW(bufferW) >= wParam) rc = 0;
    }
    else
    {
        memcpy((LPSTR)lParam, bufferA, wParam);
        if (strlen(bufferA) >= wParam) rc = 0;
    }
    heap_free(bufferA);
    heap_free(bufferW);
    return rc;
  }
  case EM_GETTEXTEX:
  {
    GETTEXTEX *ex = (GETTEXTEX*)wParam;
    int nStart, nCount; /* in chars */

    if (ex->flags & ~(GT_SELECTION | GT_USECRLF))
      FIXME("GETTEXTEX flags 0x%08x not supported\n", ex->flags & ~(GT_SELECTION | GT_USECRLF));

    if (ex->flags & GT_SELECTION)
    {
      ME_GetSelection(editor, &nStart, &nCount);
      nCount -= nStart;
    }
    else
    {
      nStart = 0;
      nCount = 0x7fffffff;
    }
    if (ex->codepage == 1200)
    {
      nCount = min(nCount, ex->cb / sizeof(WCHAR) - 1);
      return ME_GetTextW(editor, (LPWSTR)lParam, nStart, nCount, ex->flags & GT_USECRLF);
    }
    else
    {
      /* potentially each char may be a CR, why calculate the exact value with O(N) when
        we can just take a bigger buffer? :)
        The above assumption still holds with CR/LF counters, since CR->CRLF expansion
        occurs only in richedit 2.0 mode, in which line breaks have only one CR
       */
      int crlfmul = (ex->flags & GT_USECRLF) ? 2 : 1;
      LPWSTR buffer;
      DWORD buflen = ex->cb;
      LRESULT rc;
      DWORD flags = 0;

      nCount = min(nCount, ex->cb - 1);
      buffer = heap_alloc((crlfmul*nCount + 1) * sizeof(WCHAR));

      buflen = ME_GetTextW(editor, buffer, nStart, nCount, ex->flags & GT_USECRLF);
      rc = WideCharToMultiByte(ex->codepage, flags, buffer, buflen+1, (LPSTR)lParam, ex->cb, ex->lpDefaultChar, ex->lpUsedDefChar);
      if (rc) rc--; /* do not count 0 terminator */

      heap_free(buffer);
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
    return RichEditWndProc_common(hWnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr, unicode);
  }
  case EM_GETSCROLLPOS:
  {
      POINT *point = (POINT *)lParam;
      point->x = 0; /* FIXME side scrolling not implemented */
      point->y = ME_GetYScrollPos(editor);
      return 1;
  }
  case EM_GETTEXTRANGE:
  {
    TEXTRANGEW *rng = (TEXTRANGEW *)lParam;
    TRACE("EM_GETTEXTRANGE min=%d max=%d unicode=%d emul1.0=%d length=%d\n",
      rng->chrg.cpMin, rng->chrg.cpMax, unicode,
      editor->bEmulateVersion10, ME_GetTextLength(editor));
    if (unicode)
      return ME_GetTextW(editor, rng->lpstrText, rng->chrg.cpMin, rng->chrg.cpMax-rng->chrg.cpMin, 0);
    else
    {
      int nLen = rng->chrg.cpMax-rng->chrg.cpMin;
      WCHAR *p = ALLOC_N_OBJ(WCHAR, nLen+1);
      int nChars = ME_GetTextW(editor, p, rng->chrg.cpMin, nLen, 0);
      /* FIXME this is a potential security hole (buffer overrun) 
         if you know more about wchar->mbyte conversion please explain
      */
      WideCharToMultiByte(CP_ACP, 0, p, nChars+1, (char *)rng->lpstrText, nLen+1, NULL, NULL);
      FREE_OBJ(p);
      return nChars;
    }
  }
  case EM_GETLINE:
  {
    ME_DisplayItem *run;
    const unsigned int nMaxChars = *(WORD *) lParam;
    unsigned int nCharsLeft = nMaxChars;
    char *dest = (char *) lParam;
    BOOL wroteNull = FALSE;

    TRACE("EM_GETLINE: row=%d, nMaxChars=%d (%s)\n", (int) wParam, nMaxChars,
          unicode ? "Unicode" : "Ansi");

    run = ME_FindRowWithNumber(editor, wParam);
    if (run == NULL)
      return 0;

    while (nCharsLeft && (run = ME_FindItemFwd(run, diRunOrStartRow))
           && !(run->member.run.nFlags & MERF_ENDPARA))
    {
      unsigned int nCopy;
      ME_String *strText;
      if (run->type != diRun)
        break;
      strText = run->member.run.strText;
      nCopy = min(nCharsLeft, strText->nLen);

      if (unicode)
        lstrcpynW((LPWSTR) dest, strText->szData, nCopy);
      else
        nCopy = WideCharToMultiByte(CP_ACP, 0, strText->szData, nCopy, dest,
                                    nCharsLeft, NULL, NULL);
      dest += nCopy * (unicode ? sizeof(WCHAR) : 1);
      nCharsLeft -= nCopy;
    }

    /* append line termination, space allowing */
    if (nCharsLeft > 0)
    {
      if (run && (run->member.run.nFlags & MERF_ENDPARA))
      {
        unsigned int i;
        /* Write as many \r as encoded in end-of-paragraph, space allowing */
        for (i = 0; i < run->member.run.nCR && nCharsLeft > 0; i++, nCharsLeft--)
        {
          *((WCHAR *)dest) = '\r';
          dest += unicode ? sizeof(WCHAR) : 1;
        }
        /* Write as many \n as encoded in end-of-paragraph, space allowing */
        for (i = 0; i < run->member.run.nLF && nCharsLeft > 0; i++, nCharsLeft--)
        {
          *((WCHAR *)dest) = '\n';
          dest += unicode ? sizeof(WCHAR) : 1;
        }
      }
      if (nCharsLeft > 0)
      {
        if (unicode)
          *((WCHAR *)dest) = '\0';
        else
          *dest = '\0';
        nCharsLeft--;
        wroteNull = TRUE;
      }
    }

    TRACE("EM_GETLINE: got %u characters\n", nMaxChars - nCharsLeft);
    return nMaxChars - nCharsLeft - (wroteNull ? 1 : 0);
  }
  case EM_GETLINECOUNT:
  {
    ME_DisplayItem *item = editor->pBuffer->pFirst->next;
    int nRows = 0;

    ME_DisplayItem *prev_para = NULL, *last_para = NULL;

    while (item != editor->pBuffer->pLast)
    {
      assert(item->type == diParagraph);
      prev_para = ME_FindItemBack(item, diRun);
      if (prev_para) {
        assert(prev_para->member.run.nFlags & MERF_ENDPARA);
      }
      nRows += item->member.para.nRows;
      item = item->member.para.next_para;
    }
    last_para = ME_FindItemBack(item, diRun);
    assert(last_para);
    assert(last_para->member.run.nFlags & MERF_ENDPARA);
    if (editor->bEmulateVersion10 && prev_para && last_para->member.run.nCharOfs == 0
        && prev_para->member.run.nCR == 1 && prev_para->member.run.nLF == 0)
    {
      /* In 1.0 emulation, the last solitary \r at the very end of the text
         (if one exists) is NOT a line break.
         FIXME: this is an ugly hack. This should have a more regular model. */
      nRows--;
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
    if (lParam == -1)
      return ME_RowNumberFromCharOfs(editor, ME_GetCursorOfs(editor,1));
    else    
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
    int nChars = 0, nThisLineOfs = 0, nNextLineOfs = 0;
    
    if (wParam > ME_GetTextLength(editor))
      return 0;
    if (wParam == -1)
    {
      FIXME("EM_LINELENGTH: returning number of unselected characters on lines with selection unsupported.\n");
      return 0;
    }
    item = ME_FindItemAtOffset(editor, diRun, wParam, NULL);
    item = ME_RowStart(item);
    nThisLineOfs = ME_CharOfsFromRunOfs(editor, ME_FindItemFwd(item, diRun), 0);
    item_end = ME_FindItemFwd(item, diStartRowOrParagraphOrEnd);
    if (item_end->type == diStartRow)
      nNextLineOfs = ME_CharOfsFromRunOfs(editor, ME_FindItemFwd(item_end, diRun), 0);
    else
    {
      ME_DisplayItem *endPara;

      nNextLineOfs = ME_FindItemFwd(item, diParagraphOrEnd)->member.para.nCharOfs;
      endPara = ME_FindItemFwd(item, diParagraphOrEnd);
      endPara = ME_FindItemBack(endPara, diRun);
      assert(endPara);
      assert(endPara->type == diRun);
      assert(endPara->member.run.nFlags & MERF_ENDPARA);
      nNextLineOfs -= endPara->member.run.nCR + endPara->member.run.nLF;
    }
    nChars = nNextLineOfs - nThisLineOfs;
    TRACE("EM_LINELENGTH(%ld)==%d\n",wParam, nChars);
    return nChars;
  }
  case EM_EXLIMITTEXT:
  {
    if ((int)lParam < 0)
     return 0;
    if (lParam == 0)
      editor->nTextLimit = 65536;
    else
      editor->nTextLimit = (int) lParam;
    return 0;
  }
  case EM_LIMITTEXT:
  {
    if (wParam == 0)
      editor->nTextLimit = 65536;
    else
      editor->nTextLimit = (int) wParam;
    return 0;
  }
  case EM_GETLIMITTEXT:
  {
    return editor->nTextLimit;
  }
  case EM_FINDTEXT:
  {
    FINDTEXTA *ft = (FINDTEXTA *)lParam;
    int nChars = MultiByteToWideChar(CP_ACP, 0, ft->lpstrText, -1, NULL, 0);
    WCHAR *tmp;
    LRESULT r;

    if ((tmp = ALLOC_N_OBJ(WCHAR, nChars)) != NULL)
      MultiByteToWideChar(CP_ACP, 0, ft->lpstrText, -1, tmp, nChars);
    r = ME_FindText(editor, wParam, &ft->chrg, tmp, NULL);
    FREE_OBJ( tmp );
    return r;
  }
  case EM_FINDTEXTEX:
  {
    FINDTEXTEXA *ex = (FINDTEXTEXA *)lParam;
    int nChars = MultiByteToWideChar(CP_ACP, 0, ex->lpstrText, -1, NULL, 0);
    WCHAR *tmp;
    LRESULT r;

    if ((tmp = ALLOC_N_OBJ(WCHAR, nChars)) != NULL)
      MultiByteToWideChar(CP_ACP, 0, ex->lpstrText, -1, tmp, nChars);
    r = ME_FindText(editor, wParam, &ex->chrg, tmp, &ex->chrgText);
    FREE_OBJ( tmp );
    return r;
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
  case EM_CHARFROMPOS:
    return ME_CharFromPos(editor, ((POINTL *)lParam)->x, ((POINTL *)lParam)->y, NULL);
  case EM_POSFROMCHAR:
  {
    ME_DisplayItem *pRun;
    int nCharOfs, nOffset, nLength;
    POINTL pt = {0,0};
    SCROLLINFO si;
    
    nCharOfs = wParam; 
    /* detect which API version we're dealing with */
    if (wParam >= 0x40000)
        nCharOfs = lParam;
    nLength = ME_GetTextLength(editor);
    
    if (nCharOfs < nLength) { 
        ME_RunOfsFromCharOfs(editor, nCharOfs, &pRun, &nOffset);
        assert(pRun->type == diRun);
        pt.y = pRun->member.run.pt.y;
        pt.x = pRun->member.run.pt.x + ME_PointFromChar(editor, &pRun->member.run, nOffset);
        pt.y += ME_GetParagraph(pRun)->member.para.pt.y;
    } else {
        pt.x = 0;
        pt.y = editor->pBuffer->pLast->member.para.pt.y;
    }
    pt.x += editor->selofs;

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    if (GetScrollInfo(editor->hWnd, SB_VERT, &si)) pt.y -= si.nPos;
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    if (GetScrollInfo(editor->hWnd, SB_HORZ, &si)) pt.x -= si.nPos;

    if (wParam >= 0x40000) {
        *(POINTL *)wParam = pt;
    }
    return (wParam >= 0x40000) ? 0 : MAKELONG( pt.x, pt.y );
  }
  case WM_CREATE:
  {
    SCROLLINFO si;

    GetClientRect(hWnd, &editor->rcFormat);
    if (GetWindowLongW(hWnd, GWL_STYLE) & WS_HSCROLL)
    { /* Squelch the default horizontal scrollbar it would make */
      ShowScrollBar(editor->hWnd, SB_HORZ, FALSE);
    }

    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE;
    if (GetWindowLongW(hWnd, GWL_STYLE) & ES_DISABLENOSCROLL)
      si.fMask |= SIF_DISABLENOSCROLL;
    si.nMax = (si.fMask & SIF_DISABLENOSCROLL) ? 1 : 0;
    si.nMin = 0;
    si.nPage = 0;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

    ME_CommitUndo(editor);
    ME_WrapMarkedParagraphs(editor);
    ME_MoveCaret(editor);
    return 0;
  }
  case WM_DESTROY:
    ME_DestroyEditor(editor);
    SetWindowLongPtrW(hWnd, 0, 0);
    return 0;
  case WM_SETCURSOR:
  {
    return ME_SetCursor(editor);
  }
  case WM_LBUTTONDBLCLK:
  case WM_LBUTTONDOWN:
  {
    ME_CommitUndo(editor); /* End coalesced undos for typed characters */
    if ((editor->nEventMask & ENM_MOUSEEVENTS) &&
        !ME_FilterEvent(editor, msg, &wParam, &lParam))
      return 0;
    SetFocus(hWnd);
    ME_LButtonDown(editor, (short)LOWORD(lParam), (short)HIWORD(lParam),
                   ME_CalculateClickCount(hWnd, msg, wParam, lParam));
    SetCapture(hWnd);
    ME_LinkNotify(editor,msg,wParam,lParam);
    if (!ME_SetCursor(editor)) goto do_default;
    break;
  }
  case WM_MOUSEMOVE:
    if ((editor->nEventMask & ENM_MOUSEEVENTS) &&
        !ME_FilterEvent(editor, msg, &wParam, &lParam))
      return 0;
    if (GetCapture() == hWnd)
      ME_MouseMove(editor, (short)LOWORD(lParam), (short)HIWORD(lParam));
    ME_LinkNotify(editor,msg,wParam,lParam);
    /* Set cursor if mouse is captured, since WM_SETCURSOR won't be received. */
    if (GetCapture() == hWnd)
        ME_SetCursor(editor);
    break;
  case WM_LBUTTONUP:
    if (GetCapture() == hWnd)
      ReleaseCapture();
    if (editor->nSelectionType == stDocument)
      editor->nSelectionType = stPosition;
    if ((editor->nEventMask & ENM_MOUSEEVENTS) &&
        !ME_FilterEvent(editor, msg, &wParam, &lParam))
      return 0;
    else
    {
      ME_SetCursor(editor);
      ME_LinkNotify(editor,msg,wParam,lParam);
    }
    break;
  case WM_RBUTTONUP:
  case WM_RBUTTONDOWN:
    ME_CommitUndo(editor); /* End coalesced undos for typed characters */
    if ((editor->nEventMask & ENM_MOUSEEVENTS) &&
        !ME_FilterEvent(editor, msg, &wParam, &lParam))
      return 0;
    goto do_default;
  case WM_CONTEXTMENU:
    if (!ME_ShowContextMenu(editor, (short)LOWORD(lParam), (short)HIWORD(lParam)))
      goto do_default;
    break;
  case WM_PAINT:
    if (editor->bRedraw)
    {
      HDC hDC;
      PAINTSTRUCT ps;

      hDC = BeginPaint(hWnd, &ps);
      ME_PaintContent(editor, hDC, FALSE, &ps.rcPaint);
      EndPaint(hWnd, &ps);
    }
    break;
  case WM_SETFOCUS:
    editor->bHaveFocus = TRUE;
    ME_ShowCaret(editor);
    ME_SendOldNotify(editor, EN_SETFOCUS);
    return 0;
  case WM_KILLFOCUS:
    ME_CommitUndo(editor); /* End coalesced undos for typed characters */
    editor->bHaveFocus = FALSE;
    ME_HideCaret(editor);
    ME_SendOldNotify(editor, EN_KILLFOCUS);
    return 0;
  case WM_ERASEBKGND:
  {
    if (editor->bRedraw)
    {
      HDC hDC = (HDC)wParam;
      RECT rc;
      if (GetUpdateRect(hWnd,&rc,TRUE))
      {
        FillRect(hDC, &rc, editor->hbrBackground);
      }
    }
    return 1;
  }
  case WM_COMMAND:
    TRACE("editor wnd command = %d\n", LOWORD(wParam));
    return 0;
  case WM_KEYUP:
    if ((editor->nEventMask & ENM_KEYEVENTS) &&
        !ME_FilterEvent(editor, msg, &wParam, &lParam))
      return 0;
    goto do_default;
  case WM_KEYDOWN:
    if ((editor->nEventMask & ENM_KEYEVENTS) &&
        !ME_FilterEvent(editor, msg, &wParam, &lParam))
      return 0;
    if (ME_KeyDown(editor, LOWORD(wParam)))
      return 0;
    goto do_default;
  case WM_CHAR: 
  {
    WCHAR wstr;

    if (unicode)
        wstr = (WCHAR)wParam;
    else
    {
        CHAR charA = wParam;
        MultiByteToWideChar(CP_ACP, 0, &charA, 1, &wstr, 1);
    }

    switch (wstr)
    {
    case 1: /* Ctrl-A */
      ME_SetSelection(editor, 0, -1);
      return 0;
    case 3: /* Ctrl-C */
      SendMessageW(editor->hWnd, WM_COPY, 0, 0);
      return 0;
    }
    
    if (GetWindowLongW(editor->hWnd, GWL_STYLE) & ES_READONLY) {
      MessageBeep(MB_ICONERROR);
      return 0; /* FIXME really 0 ? */
    }

    switch (wstr)
    {
    case 22: /* Ctrl-V */
      SendMessageW(editor->hWnd, WM_PASTE, 0, 0);
      return 0;
    case 24: /* Ctrl-X */
      SendMessageW(editor->hWnd, WM_CUT, 0, 0);
      return 0;
    case 25: /* Ctrl-Y */
      SendMessageW(editor->hWnd, EM_REDO, 0, 0);
      return 0;
    case 26: /* Ctrl-Z */
      SendMessageW(editor->hWnd, EM_UNDO, 0, 0);
      return 0;
    }
    if (((unsigned)wstr)>=' '
        || (wstr=='\r' && (GetWindowLongW(hWnd, GWL_STYLE) & ES_MULTILINE))
        || wstr=='\t') {
      ME_Cursor cursor = editor->pCursors[0];
      ME_DisplayItem *para = ME_GetParagraph(cursor.pRun);
      int from, to;
      BOOL ctrl_is_down = GetKeyState(VK_CONTROL) & 0x8000;
      ME_GetSelection(editor, &from, &to);
      if (wstr=='\t'
          /* v4.1 allows tabs to be inserted with ctrl key down */
          && !(ctrl_is_down && !editor->bEmulateVersion10)
          )
      {
        ME_DisplayItem *para;
        BOOL bSelectedRow = FALSE;

        para = ME_GetParagraph(cursor.pRun);
        if (ME_IsSelection(editor) &&
            cursor.pRun->member.run.nCharOfs + cursor.nOffset == 0 &&
            to == ME_GetCursorOfs(editor, 0) &&
            para->member.para.prev_para->type == diParagraph)
        {
          para = para->member.para.prev_para;
          bSelectedRow = TRUE;
        }
        if (ME_IsInTable(para))
        {
          ME_TabPressedInTable(editor, bSelectedRow);
          ME_CommitUndo(editor);
          return 0;
        }
      } else if (!editor->bEmulateVersion10) { /* v4.1 */
        if (para->member.para.nFlags & MEPF_ROWEND) {
          if (wstr=='\r') {
            /* Add a new table row after this row. */
            para = ME_AppendTableRow(editor, para);
            para = para->member.para.next_para;
            editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
            editor->pCursors[0].nOffset = 0;
            editor->pCursors[1] = editor->pCursors[0];
            ME_CommitUndo(editor);
            ME_CheckTablesForCorruption(editor);
            ME_UpdateRepaint(editor);
            return 0;
          } else if (from == to) {
            para = para->member.para.next_para;
            if (para->member.para.nFlags & MEPF_ROWSTART)
              para = para->member.para.next_para;
            editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
            editor->pCursors[0].nOffset = 0;
            editor->pCursors[1] = editor->pCursors[0];
          }
        }
        else if (para == ME_GetParagraph(editor->pCursors[1].pRun) &&
                 cursor.nOffset + cursor.pRun->member.run.nCharOfs == 0 &&
                 para->member.para.prev_para->member.para.nFlags & MEPF_ROWSTART &&
                 !para->member.para.prev_para->member.para.nCharOfs)
        {
          /* Insert a newline before the table. */
          WCHAR endl = '\r';
          para = para->member.para.prev_para;
          para->member.para.nFlags &= ~MEPF_ROWSTART;
          editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
          editor->pCursors[1] = editor->pCursors[0];
          ME_InsertTextFromCursor(editor, 0, &endl, 1,
                                  editor->pCursors[0].pRun->member.run.style);
          para = editor->pBuffer->pFirst->member.para.next_para;
          ME_SetDefaultParaFormat(para->member.para.pFmt);
          para->member.para.nFlags = MEPF_REWRAP;
          editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
          editor->pCursors[1] = editor->pCursors[0];
          para->member.para.next_para->member.para.nFlags |= MEPF_ROWSTART;
          ME_CommitCoalescingUndo(editor);
          ME_CheckTablesForCorruption(editor);
          ME_UpdateRepaint(editor);
          return 0;
        }
      } else { /* v1.0 - 3.0 */
        ME_DisplayItem *para = ME_GetParagraph(cursor.pRun);
        if (ME_IsInTable(cursor.pRun))
        {
          if (cursor.pRun->member.run.nFlags & MERF_ENDPARA)
          {
            if (from == to) {
              if (wstr=='\r') {
                ME_ContinueCoalescingTransaction(editor);
                para = ME_AppendTableRow(editor, para);
                editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
                editor->pCursors[0].nOffset = 0;
                editor->pCursors[1] = editor->pCursors[0];
                ME_CommitCoalescingUndo(editor);
                ME_UpdateRepaint(editor);
              } else {
                /* Text should not be inserted at the end of the table. */
                MessageBeep(-1);
              }
              return 0;
            }
          } else if (wstr == '\r') {
            ME_ContinueCoalescingTransaction(editor);
            if (cursor.pRun->member.run.nCharOfs + cursor.nOffset == 0 &&
                !ME_IsInTable(para->member.para.prev_para))
            {
              /* Insert newline before table */
              WCHAR endl = '\r';
              cursor.pRun = ME_FindItemBack(para, diRun);
              if (cursor.pRun)
                editor->pCursors[0].pRun = cursor.pRun;
              editor->pCursors[0].nOffset = 0;
              editor->pCursors[1] = editor->pCursors[0];
              ME_InsertTextFromCursor(editor, 0, &endl, 1,
                                      editor->pCursors[0].pRun->member.run.style);
            } else {
              editor->pCursors[1] = editor->pCursors[0];
              para = ME_AppendTableRow(editor, para);
              editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
              editor->pCursors[0].nOffset = 0;
              editor->pCursors[1] = editor->pCursors[0];
            }
            ME_CommitCoalescingUndo(editor);
            ME_UpdateRepaint(editor);
            return 0;
          }
        }
      }
      /* FIXME maybe it would make sense to call EM_REPLACESEL instead ? */
      /* WM_CHAR is restricted to nTextLimit */
      if(editor->nTextLimit > ME_GetTextLength(editor) - (to-from))
      {
        ME_Style *style = ME_GetInsertStyle(editor, 0);
        ME_SaveTempStyle(editor);
        ME_ContinueCoalescingTransaction(editor);
        if (wstr == '\r' && (GetKeyState(VK_SHIFT) & 0x8000))
          ME_InsertEndRowFromCursor(editor, 0);
        else
          ME_InsertTextFromCursor(editor, 0, &wstr, 1, style);
        ME_ReleaseStyle(style);
        ME_CommitCoalescingUndo(editor);
        SetCursor(NULL);
      }

      if (editor->AutoURLDetect_bEnable) ME_UpdateSelectionLinkAttribute(editor);

      ME_UpdateRepaint(editor);
    }
    return 0;
  }
  case WM_UNICHAR:
    if (unicode)
    {
        if(wParam == UNICODE_NOCHAR) return TRUE;
        if(wParam <= 0x000fffff)
        {
            if(wParam > 0xffff) /* convert to surrogates */
            {
                wParam -= 0x10000;
                SendMessageW(editor->hWnd, WM_CHAR, (wParam >> 10) + 0xd800, 0);
                SendMessageW(editor->hWnd, WM_CHAR, (wParam & 0x03ff) + 0xdc00, 0);
            }
            else SendMessageW(editor->hWnd, WM_CHAR, wParam, 0);
        }
        return 0;
    }
    break;
  case EM_STOPGROUPTYPING:
    ME_CommitUndo(editor); /* End coalesced undos for typed characters */
    return 0;
  case EM_SCROLL: /* fall through */
  case WM_VSCROLL: 
  {
    int origNPos;
    int lineHeight;
    
    origNPos = ME_GetYScrollPos(editor);
    lineHeight = 24;
    
    if (editor && editor->pBuffer && editor->pBuffer->pDefaultStyle)
      lineHeight = editor->pBuffer->pDefaultStyle->tm.tmHeight;
    if (lineHeight <= 0) lineHeight = 24;
    
    switch(LOWORD(wParam)) 
    {
      case SB_LINEUP:
        ME_ScrollUp(editor,lineHeight);
        break;
      case SB_LINEDOWN:
        ME_ScrollDown(editor,lineHeight);
        break;
      case SB_PAGEUP:
        ME_ScrollUp(editor,editor->sizeWindow.cy);
        break;
      case SB_PAGEDOWN:
        ME_ScrollDown(editor,editor->sizeWindow.cy);
        break;
      case SB_THUMBTRACK:
      case SB_THUMBPOSITION:
        ME_ScrollAbs(editor,HIWORD(wParam));
        break;
    }
    if (msg == EM_SCROLL)
      return 0x00010000 | (((ME_GetYScrollPos(editor) - origNPos)/lineHeight) & 0xffff);
    break;
  }
  case WM_MOUSEWHEEL:
  {
    int gcWheelDelta;
    UINT pulScrollLines;

    if ((editor->nEventMask & ENM_MOUSEEVENTS) &&
        !ME_FilterEvent(editor, msg, &wParam, &lParam))
      return 0;

    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);
    gcWheelDelta = -GET_WHEEL_DELTA_WPARAM(wParam);
    
    if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
    {
      /* FIXME follow the original */
      ME_ScrollDown(editor,pulScrollLines * (gcWheelDelta / WHEEL_DELTA) * 8); 
    }
    break;
  }
  case EM_GETRECT:
  {
    *((RECT *)lParam) = editor->rcFormat;
    return 0;
  }
  case EM_SETRECT:
  case EM_SETRECTNP:
  {
    if (lParam)
    {
      RECT *rc = (RECT *)lParam;
      
      if (wParam)
      {
        editor->rcFormat.left += rc->left;
        editor->rcFormat.top += rc->top;
        editor->rcFormat.right += rc->right;
        editor->rcFormat.bottom += rc->bottom;
      }
      else
      {
        editor->rcFormat = *rc;
      }
    }
    else
    {
      GetClientRect(hWnd, &editor->rcFormat);
    }
    if (msg != EM_SETRECTNP)
      ME_RewrapRepaint(editor);
    return 0;
  }
  case EM_REQUESTRESIZE:
    ME_SendRequestResize(editor, TRUE);
    return 0;
  case WM_SETREDRAW:
    if ((editor->bRedraw = wParam))
      ME_RewrapRepaint(editor);
    return 0;
  case WM_SIZE:
  {
    GetClientRect(hWnd, &editor->rcFormat);
    ME_RewrapRepaint(editor);
    return DefWindowProcW(hWnd, msg, wParam, lParam);
  }
  /* IME messages to make richedit controls IME aware */
  case WM_IME_SETCONTEXT:
  case WM_IME_CONTROL:
  case WM_IME_SELECT:
  case WM_IME_COMPOSITIONFULL:
    return 0;
  case WM_IME_STARTCOMPOSITION:
  {
    editor->imeStartIndex=ME_GetCursorOfs(editor,0);
    ME_DeleteSelection(editor);
    ME_CommitUndo(editor);
    ME_UpdateRepaint(editor);
    return 0;
  }
  case WM_IME_COMPOSITION:
  {
    HIMC hIMC;

    ME_Style *style = ME_GetInsertStyle(editor, 0);
    hIMC = ImmGetContext(hWnd);
    ME_DeleteSelection(editor);
    ME_CommitUndo(editor);
    ME_SaveTempStyle(editor);
    if (lParam & GCS_RESULTSTR)
    {
        LPWSTR lpCompStr = NULL;
        DWORD dwBufLen;

        dwBufLen = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
        lpCompStr = HeapAlloc(GetProcessHeap(),0,dwBufLen + sizeof(WCHAR));
        ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, lpCompStr, dwBufLen);
        lpCompStr[dwBufLen/sizeof(WCHAR)] = 0;
        ME_InsertTextFromCursor(editor,0,lpCompStr,dwBufLen/sizeof(WCHAR),style);
    }
    else if (lParam & GCS_COMPSTR)
    {
        LPWSTR lpCompStr = NULL;
        DWORD dwBufLen;

        dwBufLen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
        lpCompStr = HeapAlloc(GetProcessHeap(),0,dwBufLen + sizeof(WCHAR));
        ImmGetCompositionStringW(hIMC, GCS_COMPSTR, lpCompStr, dwBufLen);
        lpCompStr[dwBufLen/sizeof(WCHAR)] = 0;

        ME_InsertTextFromCursor(editor,0,lpCompStr,dwBufLen/sizeof(WCHAR),style);
        ME_SetSelection(editor,editor->imeStartIndex,
                        editor->imeStartIndex + dwBufLen/sizeof(WCHAR));
    }
    ME_ReleaseStyle(style);
    ME_UpdateRepaint(editor);
    return 0;
  }
  case WM_IME_ENDCOMPOSITION:
  {
    ME_DeleteSelection(editor);
    editor->imeStartIndex=-1;
    return 0;
  }
  case EM_GETOLEINTERFACE:
  {
    LPVOID *ppvObj = (LPVOID*) lParam;
    return CreateIRichEditOle(editor, ppvObj);
  }
  case EM_GETPASSWORDCHAR:
  {
    return editor->cPasswordMask;
  }
  case EM_SETOLECALLBACK:
    if(editor->lpOleCallback)
      IUnknown_Release(editor->lpOleCallback);
    editor->lpOleCallback = (LPRICHEDITOLECALLBACK)lParam;
    if(editor->lpOleCallback)
      IUnknown_AddRef(editor->lpOleCallback);
    return TRUE;
  case EM_GETWORDBREAKPROC:
    return (LRESULT)editor->pfnWordBreak;
  case EM_SETWORDBREAKPROC:
  {
    EDITWORDBREAKPROCW pfnOld = editor->pfnWordBreak;

    editor->pfnWordBreak = (EDITWORDBREAKPROCW)lParam;
    return (LRESULT)pfnOld;
  }
  case EM_SETTEXTMODE:
  {
    LRESULT ret;
    int mask = 0;
    int changes = 0;
    ret = RichEditWndProc_common(hWnd, WM_GETTEXTLENGTH, 0, 0, unicode);
    if (!ret)
    {
      /*Check for valid wParam*/
      if ((((wParam & TM_RICHTEXT) && ((wParam & TM_PLAINTEXT) << 1))) ||
	  (((wParam & TM_MULTILEVELUNDO) && ((wParam & TM_SINGLELEVELUNDO) << 1))) ||
	  (((wParam & TM_MULTICODEPAGE) && ((wParam & TM_SINGLECODEPAGE) << 1))))
	return 1;
      else
      {
	if (wParam & (TM_RICHTEXT | TM_PLAINTEXT))
	{
	  mask |= (TM_RICHTEXT | TM_PLAINTEXT);
	  changes |= (wParam & (TM_RICHTEXT | TM_PLAINTEXT));
	}
	/*FIXME: Currently no support for undo level and code page options*/ 
	editor->mode = (editor->mode & (~mask)) | changes;
	return 0;
      }
    }
    return ret;
  }
  case EM_SETPASSWORDCHAR:
  {
    editor->cPasswordMask = wParam;
    ME_RewrapRepaint(editor);
    return 0;
  }
  case EM_SETTARGETDEVICE:
    if (wParam == 0)
    {
      BOOL new = (lParam == 0);
      if (editor->bWordWrap != new)
      {
        editor->bWordWrap = new;
        ME_RewrapRepaint(editor);
      }
    }
    else FIXME("Unsupported yet non NULL device in EM_SETTARGETDEVICE\n");
    break;
  default:
  do_default:
    return DefWindowProcW(hWnd, msg, wParam, lParam);
  }
  return 0L;
}

static LRESULT WINAPI RichEditWndProcW(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL unicode = TRUE;

    /* Under Win9x RichEdit20W returns ANSI strings, see the tests. */
    if (msg == WM_GETTEXT && (GetVersion() & 0x80000000))
        unicode = FALSE;

    return RichEditWndProc_common(hWnd, msg, wParam, lParam, unicode);
}

static LRESULT WINAPI RichEditWndProcA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return RichEditWndProc_common(hWnd, msg, wParam, lParam, FALSE);
}

/******************************************************************
 *        RichEditANSIWndProc (RICHED20.10)
 */
LRESULT WINAPI RichEditANSIWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return RichEditWndProcA(hWnd, msg, wParam, lParam);
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
    ME_TextEditor *editor = (ME_TextEditor *)GetWindowLongPtrW(hWnd, 0);
    
    editor->bEmulateVersion10 = TRUE;
    editor->pBuffer->pLast->member.para.nCharOfs = 2;
    assert(editor->pBuffer->pLast->prev->type == diRun);
    assert(editor->pBuffer->pLast->prev->member.run.nFlags & MERF_ENDPARA);
    editor->pBuffer->pLast->prev->member.run.nLF = 1;
  }
  return result;
}

void ME_SendOldNotify(ME_TextEditor *editor, int nCode)
{
  HWND hWnd = editor->hWnd;
  SendMessageA(GetParent(hWnd), WM_COMMAND, (nCode<<16)|GetWindowLongW(hWnd, GWLP_ID), (LPARAM)hWnd);
}

void ME_LinkNotify(ME_TextEditor *editor, UINT msg, WPARAM wParam, LPARAM lParam)
{
  int x,y;
  ME_Cursor tmpCursor;
  BOOL isExact;
  int nCharOfs; /* The start of the clicked text. Absolute character offset */

  ME_Run *tmpRun;

  ENLINK info;
  x = (short)LOWORD(lParam);
  y = (short)HIWORD(lParam);
  nCharOfs = ME_CharFromPos(editor, x, y, &isExact);
  if (!isExact) return;

  ME_CursorFromCharOfs(editor, nCharOfs, &tmpCursor);
  tmpRun = &tmpCursor.pRun->member.run;

  if ((tmpRun->style->fmt.dwMask & CFM_LINK)
    && (tmpRun->style->fmt.dwEffects & CFE_LINK))
  { /* The clicked run has CFE_LINK set */
    info.nmhdr.hwndFrom = editor->hWnd;
    info.nmhdr.idFrom = GetWindowLongW(editor->hWnd, GWLP_ID);
    info.nmhdr.code = EN_LINK;
    info.msg = msg;
    info.wParam = wParam;
    info.lParam = lParam;
    info.chrg.cpMin = ME_CharOfsFromRunOfs(editor,tmpCursor.pRun,0);
    info.chrg.cpMax = info.chrg.cpMin + ME_StrVLen(tmpRun->strText);
    SendMessageW(GetParent(editor->hWnd), WM_NOTIFY,info.nmhdr.idFrom, (LPARAM)&info);
  }  
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
  WCHAR *pStart = buffer;
  
  if (!item) {
    *buffer = 0;
    return 0;
  }
  
  /* bCRLF flag is only honored in 2.0 and up. 1.0 must always return text verbatim */
  if (editor->bEmulateVersion10) bCRLF = 0;

  if (nStart)
  {
    int nLen = ME_StrLen(item->member.run.strText) - nStart;
    if (nLen > nChars)
      nLen = nChars;
    CopyMemory(buffer, item->member.run.strText->szData + nStart, sizeof(WCHAR)*nLen);
    nChars -= nLen;
    nWritten += nLen;
    buffer += nLen;
    if (!nChars) {
      *buffer = 0;
      return nWritten;
    }
    nStart = 0;
    item = ME_FindItemFwd(item, diRun);
  }
  
  while(nChars && item)
  {
    int nLen = ME_StrLen(item->member.run.strText);
    if (item->member.run.nFlags & MERF_ENDPARA)
       nLen = item->member.run.nCR + item->member.run.nLF;
    if (nLen > nChars)
      nLen = nChars;

    if (item->member.run.nFlags & MERF_ENDCELL &&
        item->member.run.nFlags & MERF_ENDPARA)
    {
      *buffer = '\t';
    }
    else if (item->member.run.nFlags & MERF_ENDPARA)
    {
      if (!ME_FindItemFwd(item, diRun))
        /* No '\r' is appended to the last paragraph. */
        nLen = 0;
      else if (bCRLF && nChars == 1) {
        nLen = 0;
        nChars = 0;
      } else {
        if (bCRLF)
        {
          /* richedit 2.0 case - actual line-break is \r but should report \r\n */
          if (ME_GetParagraph(item)->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND))
            assert(nLen == 2);
          else
            assert(nLen == 1);
          *buffer++ = '\r';
          *buffer = '\n'; /* Later updated by nLen==1 at the end of the loop */
          if (nLen == 1)
            nWritten++;
          else
            buffer--;
        }
        else
        {
          int i, j;

          /* richedit 2.0 verbatim has only \r. richedit 1.0 should honor encodings */
          i = 0;
          while (nChars - i > 0 && i < item->member.run.nCR)
          {
            buffer[i] = '\r'; i++;
          }
          j = 0;
          while (nChars - i - j > 0 && j < item->member.run.nLF)
          {
            buffer[i+j] = '\n'; j++;
          }
        }
      }
    }
    else
      CopyMemory(buffer, item->member.run.strText->szData, sizeof(WCHAR)*nLen);
    nChars -= nLen;
    nWritten += nLen;
    buffer += nLen;    
      
    if (!nChars)
    {
      TRACE("nWritten=%d, actual=%d\n", nWritten, buffer-pStart);
      *buffer = 0;
      return nWritten;
    }
    item = ME_FindItemFwd(item, diRun);
  }
  *buffer = 0;
  TRACE("nWritten=%d, actual=%d\n", nWritten, buffer-pStart);
  return nWritten;  
}

static BOOL ME_RegisterEditorClass(HINSTANCE hInstance)
{
  WNDCLASSW wcW;
  WNDCLASSA wcA;
  
  wcW.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
  wcW.lpfnWndProc = RichEditWndProcW;
  wcW.cbClsExtra = 0;
  wcW.cbWndExtra = sizeof(ME_TextEditor *);
  wcW.hInstance = NULL; /* hInstance would register DLL-local class */
  wcW.hIcon = NULL;
  wcW.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_IBEAM));
  wcW.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wcW.lpszMenuName = NULL;

  if (is_version_nt())
  {
    wcW.lpszClassName = RichEdit20W;
    if (!RegisterClassW(&wcW)) return FALSE;
    wcW.lpszClassName = RichEdit50W;
    if (!RegisterClassW(&wcW)) return FALSE;
  }
  else
  {
    /* WNDCLASSA/W have the same layout */
    wcW.lpszClassName = (LPCWSTR)"RichEdit20W";
    if (!RegisterClassA((WNDCLASSA *)&wcW)) return FALSE;
    wcW.lpszClassName = (LPCWSTR)"RichEdit50W";
    if (!RegisterClassA((WNDCLASSA *)&wcW)) return FALSE;
  }

  wcA.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
  wcA.lpfnWndProc = RichEditWndProcA;
  wcA.cbClsExtra = 0;
  wcA.cbWndExtra = sizeof(ME_TextEditor *);
  wcA.hInstance = NULL; /* hInstance would register DLL-local class */
  wcA.hIcon = NULL;
  wcA.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_IBEAM));
  wcA.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wcA.lpszMenuName = NULL;
  wcA.lpszClassName = "RichEdit20A";
  if (!RegisterClassA(&wcA)) return FALSE;
  wcA.lpszClassName = "RichEdit50A";
  if (!RegisterClassA(&wcA)) return FALSE;

  return TRUE;
}

LRESULT WINAPI REComboWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  /* FIXME: Not implemented */
  TRACE("hWnd %p msg %04x (%s) %08lx %08lx\n",
        hWnd, msg, get_msg_name(msg), wParam, lParam);
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT WINAPI REListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  /* FIXME: Not implemented */
  TRACE("hWnd %p msg %04x (%s) %08lx %08lx\n",
        hWnd, msg, get_msg_name(msg), wParam, lParam);
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

/******************************************************************
 *        REExtendedRegisterClass (RICHED20.8)
 *
 * FIXME undocumented
 * Need to check for errors and implement controls and callbacks 
 */
LRESULT WINAPI REExtendedRegisterClass(void)
{
  WNDCLASSW wcW;
  UINT result;

  FIXME("semi stub\n");

  wcW.cbClsExtra = 0;
  wcW.cbWndExtra = 4;
  wcW.hInstance = NULL;
  wcW.hIcon = NULL;
  wcW.hCursor = NULL;
  wcW.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wcW.lpszMenuName = NULL;

  if (!ME_ListBoxRegistered)
  {
      wcW.style = CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS;
      wcW.lpfnWndProc = REListWndProc;
      wcW.lpszClassName = REListBox20W;
      if (RegisterClassW(&wcW)) ME_ListBoxRegistered = TRUE;
  }

  if (!ME_ComboBoxRegistered)
  {
      wcW.style = CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
      wcW.lpfnWndProc = REComboWndProc;
      wcW.lpszClassName = REComboBox20W;
      if (RegisterClassW(&wcW)) ME_ComboBoxRegistered = TRUE;  
  }

  result = 0;
  if (ME_ListBoxRegistered)
      result += 1;
  if (ME_ComboBoxRegistered)
      result += 2;

  return result;
}

int ME_AutoURLDetect(ME_TextEditor *editor, WCHAR curChar) 
{
  struct prefix_s {
    const char *text;
    int length;
  } prefixes[12] = {
    {"http:", 5},
    {"file:", 6},
    {"mailto:", 8},
    {"ftp:", 5},
    {"https:", 7},
    {"gopher:", 8},
    {"nntp:", 6},
    {"prospero:", 10},
    {"telnet:", 8},
    {"news:", 6},
    {"wais:", 6},
    {"www.", 5}
  };
  CHARRANGE ins_pt;
  int curf_ef, link_ef, def_ef;
  int cur_prefx, prefx_cnt;
  int sel_min, sel_max;
  int car_pos = 0;
  int text_pos=-1;
  int URLmin, URLmax = 0;
  FINDTEXTA ft;
  CHARFORMAT2W cur_format;
  CHARFORMAT2W default_format;
  CHARFORMAT2W link;
  RichEditANSIWndProc(editor->hWnd, EM_EXGETSEL, (WPARAM) 0, (LPARAM) &ins_pt);
  sel_min = ins_pt.cpMin;
  sel_max = ins_pt.cpMax;
  if (sel_min==sel_max) 
    car_pos = sel_min;
  if (sel_min!=sel_max)
    car_pos = ME_GetTextLength(editor)+1;   
  cur_format.cbSize = sizeof(cur_format);
  default_format.cbSize = sizeof(default_format);
  RichEditANSIWndProc(editor->hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cur_format);
  RichEditANSIWndProc(editor->hWnd, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM) &default_format);
  link.cbSize = sizeof(link);
  link.dwMask = CFM_LINK;
  link.dwEffects = CFE_LINK;
  curf_ef = cur_format.dwEffects & link.dwEffects;
  def_ef = default_format.dwEffects & link.dwEffects;
  link_ef = link.dwEffects & link.dwEffects;
  if (curf_ef == link_ef) 
  {
    if( curChar == '\n' || curChar=='\r' || curChar==' ') 
    {
      ME_SetSelection(editor, car_pos, car_pos);
      RichEditANSIWndProc(editor->hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &default_format);
      text_pos=-1;
      return 0;
    }
  }
  if (curf_ef == def_ef)
  {
    cur_prefx = 0;
    prefx_cnt = (sizeof(prefixes)/sizeof(struct prefix_s))-1;
    while (cur_prefx<=prefx_cnt) 
    {
      if (text_pos == -1) 
      {
        ft.lpstrText = prefixes[cur_prefx].text;
        URLmin=max(0,(car_pos-prefixes[cur_prefx].length));
        URLmax=max(0, car_pos);
        if ((car_pos == 0) && (ME_GetTextLength(editor) != 0))
        {
        URLmax = ME_GetTextLength(editor)+1;
        }
        ft.chrg.cpMin = URLmin;
        ft.chrg.cpMax = URLmax;
        text_pos=RichEditANSIWndProc(editor->hWnd, EM_FINDTEXT, FR_DOWN, (LPARAM)&ft);   
        cur_prefx++;
      }
      if (text_pos != -1) 
      {
        ME_SetCharFormat(editor, text_pos, (URLmax-text_pos), &link);
        ME_RewrapRepaint(editor);
        break;
      }
    }
  }
  return 0;
}


static BOOL isurlspecial(WCHAR c)
{
  static const WCHAR special_chars[] = {'.','/','%','@','*','|','\\','+','#',0};
  return strchrW( special_chars, c ) != NULL;
}

/**
 * This proc takes a selection, and scans it forward in order to select the span
 * of a possible URL candidate. A possible URL candidate must start with isalnum
 * or one of the following special characters: *|/\+%#@ and must consist entirely
 * of the characters allowed to start the URL, plus : (colon) which may occur
 * at most once, and not at either end.
 *
 * sel_max == -1 indicates scan to end of text.
 */
BOOL ME_FindNextURLCandidate(ME_TextEditor *editor, int sel_min, int sel_max,
        int * candidate_min, int * candidate_max)
{
  ME_DisplayItem * item;
  ME_DisplayItem * para;
  int nStart;
  BOOL foundColon = FALSE;
  WCHAR lastAcceptedChar = '\0';

  TRACE("sel_min = %d sel_max = %d\n", sel_min, sel_max);

  *candidate_min = *candidate_max = -1;
  item = ME_FindItemAtOffset(editor, diRun, sel_min, &nStart);
  if (!item) return FALSE;
  TRACE("nStart = %d\n", nStart);
  para = ME_GetParagraph(item);
  if (sel_max == -1) sel_max = ME_GetTextLength(editor);
  while (item && para->member.para.nCharOfs + item->member.run.nCharOfs + nStart < sel_max)
  {
    ME_DisplayItem * next_item;

    if (!(item->member.run.nFlags & MERF_ENDPARA)) {
      /* Find start of candidate */
      if (*candidate_min == -1) {
        while (nStart < ME_StrLen(item->member.run.strText) &&
                !(isalnumW(item->member.run.strText->szData[nStart]) ||
                  isurlspecial(item->member.run.strText->szData[nStart]))) {
          nStart++;
        }
        if (nStart < ME_StrLen(item->member.run.strText) &&
                (isalnumW(item->member.run.strText->szData[nStart]) ||
                 isurlspecial(item->member.run.strText->szData[nStart]))) {
          *candidate_min = para->member.para.nCharOfs + item->member.run.nCharOfs + nStart;
          lastAcceptedChar = item->member.run.strText->szData[nStart];
          nStart++;
        }
      }

      /* Find end of candidate */
      if (*candidate_min >= 0) {
        while (nStart < ME_StrLen(item->member.run.strText) &&
                (isalnumW(item->member.run.strText->szData[nStart]) ||
                 isurlspecial(item->member.run.strText->szData[nStart]) ||
                 (!foundColon && item->member.run.strText->szData[nStart] == ':') )) {
          if (item->member.run.strText->szData[nStart] == ':') foundColon = TRUE;
          lastAcceptedChar = item->member.run.strText->szData[nStart];
          nStart++;
        }
        if (nStart < ME_StrLen(item->member.run.strText) &&
                !(isalnumW(item->member.run.strText->szData[nStart]) ||
                 isurlspecial(item->member.run.strText->szData[nStart]) )) {
          *candidate_max = para->member.para.nCharOfs + item->member.run.nCharOfs + nStart;
          nStart++;
          if (lastAcceptedChar == ':') (*candidate_max)--;
          return TRUE;
        }
      }
    } else {
      /* End of paragraph: skip it if before candidate span, or terminates
         current active span */
      if (*candidate_min >= 0) {
        *candidate_max = para->member.para.nCharOfs + item->member.run.nCharOfs;
        if (lastAcceptedChar == ':') (*candidate_max)--;
        return TRUE;
      }
    }

    /* Reaching this point means no span was found, so get next span */
    next_item = ME_FindItemFwd(item, diRun);
    if (!next_item) {
      if (*candidate_min >= 0) {
        /* There are no further runs, so take end of text as end of candidate */
        *candidate_max = para->member.para.nCharOfs + item->member.run.nCharOfs + nStart;
        if (lastAcceptedChar == ':') (*candidate_max)--;
        return TRUE;
      }
    }
    item = next_item;
    para = ME_GetParagraph(item);
    nStart = 0;
  }

  if (item) {
    if (*candidate_min >= 0) {
      /* There are no further runs, so take end of text as end of candidate */
      *candidate_max = para->member.para.nCharOfs + item->member.run.nCharOfs + nStart;
      if (lastAcceptedChar == ':') (*candidate_max)--;
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * This proc evaluates the selection and returns TRUE if it can be considered an URL
 */
BOOL ME_IsCandidateAnURL(ME_TextEditor *editor, int sel_min, int sel_max)
{
  struct prefix_s {
    const char *text;
    int length;
  } prefixes[12] = {
    /* Code below depends on these being in decreasing length order! */
    {"prospero:", 10},
    {"telnet:", 8},
    {"gopher:", 8},
    {"mailto:", 8},
    {"https:", 7},
    {"file:", 6},
    {"news:", 6},
    {"wais:", 6},
    {"nntp:", 6},
    {"http:", 5},
    {"www.", 5},
    {"ftp:", 5},
  };
  LPWSTR bufferW = NULL;
  WCHAR bufW[32];
  int i;

  if (sel_max == -1) sel_max = ME_GetTextLength(editor);
  assert(sel_min <= sel_max);
  for (i = 0; i < sizeof(prefixes) / sizeof(struct prefix_s); i++)
  {
    if (sel_max - sel_min < prefixes[i].length) continue;
    if (bufferW == NULL) {
      bufferW = (LPWSTR)heap_alloc((sel_max - sel_min + 1) * sizeof(WCHAR));
    }
    ME_GetTextW(editor, bufferW, sel_min, min(sel_max - sel_min, strlen(prefixes[i].text)), 0);
    MultiByteToWideChar(CP_ACP, 0, prefixes[i].text, -1, bufW, 32);
    if (!lstrcmpW(bufW, bufferW))
    {
      heap_free(bufferW);
      return TRUE;
    }
  }
  heap_free(bufferW);
  return FALSE;
}

/**
 * This proc walks through the indicated selection and evaluates whether each
 * section identified by ME_FindNextURLCandidate and in-between sections have
 * their proper CFE_LINK attributes set or unset. If the CFE_LINK attribute is
 * not what it is supposed to be, this proc sets or unsets it as appropriate.
 *
 * Returns TRUE if at least one section was modified.
 */
BOOL ME_UpdateLinkAttribute(ME_TextEditor *editor, int sel_min, int sel_max)
{
  BOOL modified = FALSE;
  int cMin, cMax;

  if (sel_max == -1) sel_max = ME_GetTextLength(editor);
  do
  {
    int beforeURL[2];
    int inURL[2];
    CHARFORMAT2W link;

    if (ME_FindNextURLCandidate(editor, sel_min, sel_max, &cMin, &cMax))
    {
      /* Section before candidate is not an URL */
      beforeURL[0] = sel_min;
      beforeURL[1] = cMin;

      if (ME_IsCandidateAnURL(editor, cMin, cMax))
      {
        inURL[0] = cMin; inURL[1] = cMax;
      }
      else
      {
        beforeURL[1] = cMax;
        inURL[0] = inURL[1] = -1;
      }
      sel_min = cMax;
    }
    else
    {
      /* No more candidates until end of selection */
      beforeURL[0] = sel_min;
      beforeURL[1] = sel_max;
      inURL[0] = inURL[1] = -1;
      sel_min = sel_max;
    }

    if (beforeURL[0] < beforeURL[1])
    {
      /* CFE_LINK effect should be consistently unset */
      link.cbSize = sizeof(link);
      ME_GetCharFormat(editor, beforeURL[0], beforeURL[1], &link);
      if (!(link.dwMask & CFM_LINK) || (link.dwEffects & CFE_LINK))
      {
        /* CFE_LINK must be unset from this range */
        memset(&link, 0, sizeof(CHARFORMAT2W));
        link.cbSize = sizeof(link);
        link.dwMask = CFM_LINK;
        link.dwEffects = 0;
        ME_SetCharFormat(editor, beforeURL[0], beforeURL[1] - beforeURL[0], &link);
        modified = TRUE;
      }
    }
    if (inURL[0] < inURL[1])
    {
      /* CFE_LINK effect should be consistently set */
      link.cbSize = sizeof(link);
      ME_GetCharFormat(editor, inURL[0], inURL[1], &link);
      if (!(link.dwMask & CFM_LINK) || !(link.dwEffects & CFE_LINK))
      {
        /* CFE_LINK must be set on this range */
        memset(&link, 0, sizeof(CHARFORMAT2W));
        link.cbSize = sizeof(link);
        link.dwMask = CFM_LINK;
        link.dwEffects = CFE_LINK;
        ME_SetCharFormat(editor, inURL[0], inURL[1] - inURL[0], &link);
        modified = TRUE;
      }
    }
  } while (sel_min < sel_max);
  return modified;
}

void ME_UpdateSelectionLinkAttribute(ME_TextEditor *editor)
{
  ME_DisplayItem * startPara, * endPara;
  ME_DisplayItem * item;
  int dummy;
  int from, to;

  ME_GetSelection(editor, &from, &to);
  if (from > to) from ^= to, to ^=from, from ^= to;
  startPara = NULL; endPara = NULL;

  /* Find paragraph previous to the one that contains start cursor */
  item = ME_FindItemAtOffset(editor, diRun, from, &dummy);
  if (item) {
    startPara = ME_FindItemBack(item, diParagraph);
    item = ME_FindItemBack(startPara, diParagraph);
    if (item) startPara = item;
  }

  /* Find paragraph that contains end cursor */
  item = ME_FindItemAtOffset(editor, diRun, to, &dummy);
  if (item) {
    endPara = ME_FindItemFwd(item, diParagraph);
  }

  if (startPara && endPara) {
    ME_UpdateLinkAttribute(editor,
      startPara->member.para.nCharOfs,
      endPara->member.para.nCharOfs);
  } else if (startPara) {
    ME_UpdateLinkAttribute(editor,
      startPara->member.para.nCharOfs,
      -1);
  }
}
